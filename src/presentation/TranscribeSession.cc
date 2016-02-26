/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <algorithm>
#include <map>
#include <ostream>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "TranscribeSession.h"

#include "Application.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Layer.h"
#include "app-logic/LayerTaskRegistry.h"
#include "app-logic/ReconstructGraph.h"

#include "file-io/ErrorOpeningFileForReadingException.h"
#include "file-io/ErrorOpeningPipeFromGzipException.h"
#include "file-io/FileFormatNotSupportedException.h"
#include "file-io/FileLoadAbortedException.h"

#include "global/GPlatesAssert.h"
#include "global/GPlatesException.h"
#include "global/PreconditionViolationError.h"

#include "scribe/Scribe.h"
#include "scribe/ScribeExceptions.h"


namespace GPlatesPresentation
{
	namespace TranscribeSession
	{
		/**
		 * Since this is a first cut at transcribing session state we'll just keep it similar
		 * to the previous version (3) which used XML instead of the scribe system.
		 *
		 *
		 * A lot of this isn't actually using the scribe system as it was intended.
		 * It's transcribing information into separate structures and then using that information
		 * to create the real objects (layers, files) using the layers and file API.
		 * The scribe system was more intended for re-creating the real objects by transcribing
		 * them directly. However transcribing separate structures and then using encapsulated APIs
		 * is a better approach (more stable since things get signaled properly and set up properly).
		 * Doing this by transcribing the object's internals directly turns out to be quite difficult.
		 *
		 *
		 * TODO: As a result of the above comment we will need to add general tag retrieval to the
		 * scribe system. This would enable access to the transcription data without necessarily
		 * having to create separate transcribe info structures just to transcribe info into.
		 *
		 * For example, a nice way to access sequences (like something transcribed via
		 * a std::vector<>) could be something like (which is a bit like boost::python::object):
		 *
		 *	unsigned int size = scribe.get_object("d_input_connections").size();
		 *	for (unsigned int n = 0; n < size; ++n)
		 *	{
		 *		LoadRef<Connection> connection =
		 *				scribe.get_object("d_input_connections")[n].load<Connection>();
		 *		if (!connection.is_valid())
		 *		{
		 *			if (scribe.get_transcribe_result() == TRANSCRIBE_UNKNOWN_TYPE)
		 *			{
		 *				// Skip unknown elements.
		 *				continue;
		 *			}
		 *			
		 *			return scribe.get_transcribe_result();
		 *		}
		 *		...
		 *	}
		 *
		 * ...where the '.size()' and '[n]' only work for sequences transcribed using
		 * the sequence protocol (or explicitly using "size" and "item" object tags)
		 * and '.get_object()' returns a proxy so that it doesn't actually load the
		 * entire sequence (or other type of object) on each call.
		 * And could chain these together:
		 *
		 *	scribe.set_object("layers")[l].set_object("connections")[c].save(connection);
		 *	scribe.set_object("layers")[l].set_object("is_active").save(true);
		 *
		 *	scribe.get_object("layers")[l].get_object("connections")[c].load<Connection>();
		 *	scribe.get_object("layers")[l].get_object("is_active").load<bool>();
		 *
		 * ...and the 'transcribe' alternative that combines both save and load for
		 * default-constructible objects...
		 *
		 *	scribe.transcribe_object("layers")[l].transcribe_object("connections")[c].transcribe(connection);
		 *	scribe.transcribe_object("layers")[l].transcribe_object("is_active").transcribe(is_active);
		 *
		 * ...where each layer in 'layers' contains multiple connections.
		 */
		namespace Version4
		{
			/**
			 * Enable RAII style 'lock' on temporarily disabling automatic layer creation
			 * within app-state for as long as the current scope holds onto this object.
			 */
			class SuppressAutoLayerCreationRAII :
					private boost::noncopyable
			{
			public:
				SuppressAutoLayerCreationRAII(
						GPlatesAppLogic::ApplicationState &application_state) :
					d_application_state(application_state)
				{
					// Suppress auto-creation of layers because we have session information regarding which
					// layers should be created and what their connections should be.
					d_application_state.suppress_auto_layer_creation(true);
				}
				
				~SuppressAutoLayerCreationRAII()
				{
					d_application_state.suppress_auto_layer_creation(false);
				}
				
				GPlatesAppLogic::ApplicationState &d_application_state;
			};


			/**
			 * Since this is a first cut at transcribing session state we'll just keep it similar
			 * to the previous version (3) which used XML instead of the scribe system.
			 */
			struct SessionState
			{
				struct TranscribeLayerInputConnection
				{
					TranscribeLayerInputConnection() :
						d_input_channel_name(GPlatesAppLogic::LayerInputChannelName::UNUSED),
						d_input_index(0),
						d_is_input_file(false)
					{  }

					TranscribeLayerInputConnection(
							GPlatesAppLogic::LayerInputChannelName::Type input_channel_name,
							unsigned int input_index,
							bool is_input_file) :
						d_input_channel_name(input_channel_name),
						d_input_index(input_index),
						d_is_input_file(is_input_file)
					{  }

					GPlatesAppLogic::LayerInputChannelName::Type d_input_channel_name;
					unsigned int d_input_index;
					bool d_is_input_file;


					GPlatesScribe::TranscribeResult
					transcribe(
							GPlatesScribe::Scribe &scribe,
							bool transcribed_construct_data)
					{
						if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_input_channel_name, "d_input_channel_name") ||
							!scribe.transcribe(TRANSCRIBE_SOURCE, d_input_index, "d_input_index") ||
							!scribe.transcribe(TRANSCRIBE_SOURCE, d_is_input_file, "d_is_input_file"))
						{
							return scribe.get_transcribe_result();
						}

						return GPlatesScribe::TRANSCRIBE_SUCCESS;
					}
				};


				struct TranscribeLayer
				{
					TranscribeLayer() :
						d_layer_task_type(GPlatesAppLogic::LayerTaskType::NUM_TYPES),
						d_is_active(false),
						d_is_auto_created(false)
					{  }

					TranscribeLayer(
							GPlatesAppLogic::LayerTaskType::Type layer_task_type,
							bool is_active,
							bool is_auto_created,
							const std::vector<TranscribeLayerInputConnection> &input_connections) :
						d_layer_task_type(layer_task_type),
						d_is_active(is_active),
						d_is_auto_created(is_auto_created),
						d_input_connections(input_connections)
					{  }

					GPlatesAppLogic::LayerTaskType::Type d_layer_task_type;
					bool d_is_active;
					bool d_is_auto_created;
					std::vector<TranscribeLayerInputConnection> d_input_connections;


					GPlatesScribe::TranscribeResult
					transcribe(
							GPlatesScribe::Scribe &scribe,
							bool transcribed_construct_data)
					{
						if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_layer_task_type, "d_layer_task_type") ||
							!scribe.transcribe(TRANSCRIBE_SOURCE, d_is_active, "d_is_active") ||
							!scribe.transcribe(TRANSCRIBE_SOURCE, d_is_auto_created, "d_is_auto_created") ||
							!scribe.transcribe(TRANSCRIBE_SOURCE, d_input_connections, "d_input_connections"))
						{
							return scribe.get_transcribe_result();
						}

						return GPlatesScribe::TRANSCRIBE_SUCCESS;
					}
				};


				explicit
				SessionState(
						const QList<QString> &feature_collection_files) :
					d_feature_collection_filenames(feature_collection_files)
				{  }


				QList<QString> d_feature_collection_filenames;
				QList<QString> d_feature_collection_filenames_not_loaded;

				std::vector<TranscribeLayer> d_transcribe_layers;
				boost::optional<unsigned int> d_default_reconstruction_tree_layer_index;


				GPlatesScribe::TranscribeResult
				transcribe(
						GPlatesScribe::Scribe &scribe,
						bool transcribed_construct_data)
				{
					GPlatesAppLogic::ApplicationState &application_state =
							Application::instance().get_application_state();

					GPlatesAppLogic::FeatureCollectionFileIO &file_io =
							application_state.get_feature_collection_file_io();

					GPlatesAppLogic::FeatureCollectionFileState &file_state =
							application_state.get_feature_collection_file_state();

					GPlatesAppLogic::LayerTaskRegistry &layer_task_registry =
							application_state.get_layer_task_registry();

					GPlatesAppLogic::ReconstructGraph &reconstruct_graph =
							application_state.get_reconstruct_graph();


					//
					// Transcribe the feature collection filenames
					//

					typedef std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference>
							file_reference_on_save_seq_type;
					typedef std::vector< boost::optional<GPlatesAppLogic::FeatureCollectionFileState::file_reference> >
							file_reference_on_load_seq_type;

					file_reference_on_save_seq_type file_references_on_save;
					file_reference_on_load_seq_type file_references_on_load;

					// Get the file references of currently loaded files in the save path.
					if (scribe.is_saving())
					{
						// Map the loaded feature collection filenames to file references.
						typedef std::map<QString, GPlatesAppLogic::FeatureCollectionFileState::file_reference>
								save_file_reference_map_type;
						save_file_reference_map_type save_file_reference_map;
						BOOST_FOREACH(
								const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_ref,
								file_state.get_loaded_files())
						{
							const GPlatesFileIO::FileInfo &file_info = file_ref.get_file().get_file_info();
							const QString absolute_file_path = file_info.get_qfileinfo().absoluteFilePath();
							if (!absolute_file_path.isEmpty())
							{
								save_file_reference_map.insert(
										save_file_reference_map_type::value_type(absolute_file_path, file_ref));
							}
						}

						// Get the file references in the same order as the filenames.
						// This is necessary because we are transcribing these file indices
						// (eg, layer connections reference files by integer index).
						for (int file_index = 0; file_index < d_feature_collection_filenames.size(); ++file_index)
						{
							// We should be able to find the filename - we're doing pretty much exactly
							// what 'TranscribeSession::get_save_session_files()' is doing.
							save_file_reference_map_type::const_iterator file_ref_iter =
									save_file_reference_map.find(d_feature_collection_filenames[file_index]);
							GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
									file_ref_iter != save_file_reference_map.end(),
									GPLATES_ASSERTION_SOURCE);

							GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref =
									file_ref_iter->second;
							file_references_on_save.push_back(file_ref);
						}
					}

					// Load files using the filenames in the load path.
					if (scribe.is_loading())
					{
						// Suppress auto-creation of layers during this scope because we have session
						// information regarding which layers should be created and what their connections
						// should be.
						SuppressAutoLayerCreationRAII raii(application_state);

						// Any files that fail to load will have a 'boost::none' file reference.
						// This is so failed loads don't stuff up our file indexing.
						file_references_on_load.resize(d_feature_collection_filenames.size());

						for (int file_index = 0; file_index < d_feature_collection_filenames.size(); ++file_index)
						{
							QString filename = d_feature_collection_filenames[file_index];

							// Attempt to load the current file.
							try
							{
								GPlatesAppLogic::FeatureCollectionFileState::file_reference
										file_reference = file_io.load_file(filename);

								file_references_on_load[file_index] = file_reference;
							}
							catch (GPlatesFileIO::ErrorOpeningPipeFromGzipException &exc)
							{
								d_feature_collection_filenames_not_loaded.push_back(filename);
								qWarning() << exc; // Also log the detailed error message.
							}
							catch (GPlatesFileIO::FileFormatNotSupportedException &exc)
							{
								d_feature_collection_filenames_not_loaded.push_back(filename);
								qWarning() << exc; // Also log the detailed error message.
							}
							catch (GPlatesFileIO::ErrorOpeningFileForReadingException &exc)
							{
								d_feature_collection_filenames_not_loaded.push_back(filename);
								qWarning() << exc; // Also log the detailed error message.
							}
							catch (GPlatesFileIO::FileLoadAbortedException &exc)
							{
								d_feature_collection_filenames_not_loaded.push_back(filename);
								qWarning() << exc; // Also log the detailed error message.
							}
							catch (GPlatesGlobal::Exception &exc)
							{
								d_feature_collection_filenames_not_loaded.push_back(filename);
								qWarning() << exc; // Also log the detailed error message.
							}
						}
					}


					//
					// Transcribe the app-logic layers
					//

					typedef std::vector<GPlatesAppLogic::Layer> layer_seq_type;

					layer_seq_type layers;

					if (scribe.is_saving())
					{
						// Get the layers.
						layers.assign(reconstruct_graph.begin(), reconstruct_graph.end());

						// Get the transcribe information from the layers and their connections.
						BOOST_FOREACH(GPlatesAppLogic::Layer layer, layers)
						{
							std::vector<TranscribeLayerInputConnection> transcribe_input_connections;

							// Iterate over the layer's input connections.
							std::vector<GPlatesAppLogic::Layer::InputConnection> input_connections =
									layer.get_all_inputs();
							for (unsigned int n = 0; n < input_connections.size(); ++n)
							{
								const GPlatesAppLogic::Layer::InputConnection &input_connection =
										input_connections[n];

								GPlatesAppLogic::LayerInputChannelName::Type input_channel_name =
										input_connection.get_input_channel_name();

								boost::optional<GPlatesAppLogic::Layer::InputFile> input_file =
										input_connection.get_input_file();
								if (input_file)
								{
									// Find the input file in our list of loaded file references.
									file_reference_on_save_seq_type::const_iterator file_iter =
											std::find(file_references_on_save.begin(), file_references_on_save.end(),
													input_file->get_file());
									if (file_iter != file_references_on_save.end())
									{
										// Make sure doesn't reference an empty filename.
										const GPlatesFileIO::FileInfo &file_info = file_iter->get_file().get_file_info();
										if (!file_info.get_qfileinfo().absoluteFilePath().isEmpty())
										{
											const unsigned int file_index = file_iter - file_references_on_save.begin();
											transcribe_input_connections.push_back(
													TranscribeLayerInputConnection(
															input_channel_name,
															file_index,
															true/*is_input_file*/));
										}
									}
								}
								else
								{
									// The input is not a file so it must be a layer.
									GPlatesAppLogic::Layer input_layer = input_connection.get_input_layer().get();

									// Find the input layer in our list of layers.
									layer_seq_type::const_iterator layer_iter =
											std::find(layers.begin(), layers.end(), input_layer);
									if (layer_iter != layers.end())
									{
										const unsigned int layer_index = layer_iter - layers.begin();
										transcribe_input_connections.push_back(
												TranscribeLayerInputConnection(
															input_channel_name,
															layer_index,
															false/*is_input_file*/));
									}
								}
							}

							d_transcribe_layers.push_back(
									TranscribeLayer(
											layer.get_type(),
											layer.is_active(),
											layer.get_auto_created(),
											transcribe_input_connections));
						}
					}

					// Transcribe the layers.
					if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_transcribe_layers, "d_layers"))
					{
						return scribe.get_transcribe_result();
					}

					if (scribe.is_loading())
					{
						// Put all layer additions in a single add layers group.
						GPlatesAppLogic::ReconstructGraph::AddOrRemoveLayersGroup add_layers_group(reconstruct_graph);
						add_layers_group.begin_add_or_remove_layers();

						std::vector<GPlatesAppLogic::LayerTaskRegistry::LayerTaskType> layer_task_types =
								layer_task_registry.get_all_layer_task_types();

						// We first need to create the layers before we can make connections.
						BOOST_FOREACH(const TranscribeLayer &transcribe_layer, d_transcribe_layers)
						{
							boost::shared_ptr<GPlatesAppLogic::LayerTask> layer_task;
							for (unsigned int n = 0; n < layer_task_types.size(); ++n)
							{
								const GPlatesAppLogic::LayerTaskRegistry::LayerTaskType &layer_task_type =
										layer_task_types[n];

								if (layer_task_type.get_layer_type() == transcribe_layer.d_layer_task_type)
								{
									layer_task = layer_task_type.create_layer_task();
									break;
								}
							}

							if (!layer_task)
							{
								// Remove all layers before returning.
								for (unsigned int l = 0; l < layers.size(); ++l)
								{
									reconstruct_graph.remove_layer(layers[l]);
								}

								return GPlatesScribe::TRANSCRIBE_INCOMPATIBLE;
							}

							GPlatesAppLogic::Layer layer = reconstruct_graph.add_layer(layer_task);
							layer.activate(transcribe_layer.d_is_active);
							// Was the layer originally auto-created ?
							// This is needed so the layer can be auto-destroyed if the input file
							// on its main input channel is later unloaded by the user.
							layer.set_auto_created(transcribe_layer.d_is_auto_created);

							layers.push_back(layer);
						}

						// Next we can make input connections for the layers.
						for (unsigned int layer_index = 0; layer_index < layers.size(); ++layer_index)
						{
							GPlatesAppLogic::Layer layer = layers[layer_index];

							const TranscribeLayer &transcribe_layer = d_transcribe_layers[layer_index];
							const std::vector<TranscribeLayerInputConnection> &transcribe_input_connections =
									transcribe_layer.d_input_connections;

							// Whether one or more files connected to the current layer's main input channel were
							// not loaded (if all files on this channel were not loaded then we'll delete the layer).
							bool main_input_channel_file_not_loaded = false;

							// Iterate over the layer's input connections.
							for (unsigned int n = 0; n < transcribe_input_connections.size(); ++n)
							{
								const TranscribeLayerInputConnection &transcribe_input_connection =
										transcribe_input_connections[n];

								// Input is either a file or a layer.
								if (transcribe_input_connection.d_is_input_file)
								{
									if (transcribe_input_connection.d_input_index >= file_references_on_load.size())
									{
										return GPlatesScribe::TRANSCRIBE_INCOMPATIBLE;
									}

									// If the input file did not load then skip this connection.
									if (!file_references_on_load[transcribe_input_connection.d_input_index])
									{
										if (transcribe_input_connection.d_input_channel_name ==
											layer.get_main_input_feature_collection_channel())
										{
											main_input_channel_file_not_loaded = true;
										}

										continue;
									}

									GPlatesAppLogic::FeatureCollectionFileState::file_reference file_reference =
											file_references_on_load[transcribe_input_connection.d_input_index].get();
									GPlatesAppLogic::Layer::InputFile input_file =
											reconstruct_graph.get_input_file(file_reference);

									layer.connect_input_to_file(
											input_file,
											transcribe_input_connection.d_input_channel_name);
								}
								else
								{
									if (transcribe_input_connection.d_input_index >= layers.size())
									{
										return GPlatesScribe::TRANSCRIBE_INCOMPATIBLE;
									}

									GPlatesAppLogic::Layer input_layer =
											layers[transcribe_input_connection.d_input_index];

									// Connect to the input layer.
									//
									// We might have already removed the input layer if its main
									// input channel files were not loaded (eg, didn't exist).
									// If so then we don't connect to it.
									if (input_layer.is_valid())
									{
										layer.connect_input_to_layer_output(
												input_layer,
												transcribe_input_connection.d_input_channel_name);
									}
								}
							}

							//
							// Remove layer if connected to files that were not successfully loaded.
							//
							// Remove layer if references files, on the main input channel,
							// that don't exist. This can happen when files have been moved or deleted
							// since the session/project was saved.
							//

							if (main_input_channel_file_not_loaded)
							{
								std::vector<GPlatesAppLogic::Layer::InputConnection> layer_input_connections =
										layer.get_channel_inputs(
												layer.get_main_input_feature_collection_channel());
								if (layer_input_connections.empty())
								{
									// Remove layer - also removes any connections made to layer so far.
									reconstruct_graph.remove_layer(layer);

									// Subsequently connected layers won't be able to connect to this layer.
									layers[layer_index] = layer = GPlatesAppLogic::Layer();
								}
							}
						}

						// End the add layers group.
						add_layers_group.end_add_or_remove_layers();
					}

					//
					// Transcribe the default reconstruction tree layer
					//

					if (scribe.is_saving())
					{
						GPlatesAppLogic::Layer default_reconstruction_tree_layer =
								reconstruct_graph.get_default_reconstruction_tree_layer();
						if (default_reconstruction_tree_layer.is_valid())
						{
							// Find the default reconstruction tree layer in our list of layers.
							layer_seq_type::const_iterator layer_iter =
									std::find(layers.begin(), layers.end(), default_reconstruction_tree_layer);
							if (layer_iter != layers.end())
							{
								const unsigned int layer_index = layer_iter - layers.begin();
								d_default_reconstruction_tree_layer_index = layer_index;
							}
						}
					}

					if (!scribe.transcribe(
							TRANSCRIBE_SOURCE,
							d_default_reconstruction_tree_layer_index,
							"d_default_reconstruction_tree_layer_index"))
					{
						return scribe.get_transcribe_result();
					}

					if (scribe.is_loading())
					{
						if (d_default_reconstruction_tree_layer_index)
						{
							if (d_default_reconstruction_tree_layer_index.get() >= layers.size())
							{
								return GPlatesScribe::TRANSCRIBE_INCOMPATIBLE;
							}

							GPlatesAppLogic::Layer default_reconstruction_tree_layer =
									layers[d_default_reconstruction_tree_layer_index.get()];

							// Set the default reconstruction tree layer.
							//
							// We might have already removed it if its main input channel files were
							// not loaded (eg, didn't exist). If so then we don't set it as the default.
							if (default_reconstruction_tree_layer.is_valid())
							{
								reconstruct_graph.set_default_reconstruction_tree_layer(
										default_reconstruction_tree_layer);
							}
						}
					}

					return GPlatesScribe::TRANSCRIBE_SUCCESS;
				}

				static
				GPlatesScribe::TranscribeResult
				transcribe_construct_data(
						GPlatesScribe::Scribe &scribe,
						GPlatesScribe::ConstructObject<SessionState> &session_state)
				{
					// Shouldn't construct object - always transcribe existing object.
					GPlatesGlobal::Assert<GPlatesScribe::Exceptions::ConstructNotAllowed>(
							false,
							GPLATES_ASSERTION_SOURCE,
							typeid(SessionState));

					// Shouldn't be able to get here - keep compiler happy.
					return GPlatesScribe::TRANSCRIBE_INCOMPATIBLE;
				}
			};
		}
	}
}


QList<QString>
GPlatesPresentation::TranscribeSession::transcribe(
		GPlatesScribe::Scribe &scribe,
		const QList<QString> &feature_collection_files,
		boost::optional<QString> project_filename)
{
	Version4::SessionState session_state_version4(feature_collection_files);

	// The way the session state is transcribed is likely to change significantly in the next version,
	// since this is just a first cut, so we'll keep it all tucked away in a separate tag so that
	// future versions can save both the "session_state_version4" state (so this version can load it)
	// and its rearranged version (which it can put in another tag). Also future versions can easily
	// detect, when loading, whether a session came from this version (by checking "session_state_version4").
	//
	// But once things have stabilised, in terms of how the transcribing of GPlates state is handled,
	// there will no longer be a complete separation of state like this and changes in new
	// versions will be more interspersed with older versions.
 	if (!scribe.transcribe(TRANSCRIBE_SOURCE, session_state_version4, "session_state_version4"))
	{
		// The transcription is incompatible.
		GPlatesGlobal::Assert<TranscribeSession::UnsupportedVersion>(
				false,
				GPLATES_ASSERTION_SOURCE,
				project_filename,
				scribe.get_transcribe_incompatible_call_stack());
	}

	// Make sure the transcription is complete.
	GPlatesGlobal::Assert<GPlatesScribe::Exceptions::TranscriptionIncomplete>(
			scribe.is_transcription_complete(),
			GPLATES_ASSERTION_SOURCE);

	return session_state_version4.d_feature_collection_filenames_not_loaded;
}


QList<QString>
GPlatesPresentation::TranscribeSession::get_save_session_files()
{
	QList<QString> filenames;

	GPlatesAppLogic::FeatureCollectionFileState &file_state =
			Application::instance().get_application_state().get_feature_collection_file_state();

	BOOST_FOREACH(
			const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_ref,
			file_state.get_loaded_files())
	{
		const GPlatesFileIO::FileInfo &file_info = file_ref.get_file().get_file_info();
		if (!file_info.get_qfileinfo().absoluteFilePath().isEmpty())
		{
			filenames.append(file_info.get_qfileinfo().absoluteFilePath());
		}
	}

	return filenames;
}


void
GPlatesPresentation::TranscribeSession::UnsupportedVersion::write_message(
		std::ostream &os) const
{
	os << "Attempted to load a session";

	if (d_project_filename)
	{
		os << ", from project file '" << d_project_filename->toStdString() << "',";
	}

	os << " created from a version of GPlates that is either too old or too new." << std::endl;
	
	if (d_transcribe_incompatible_call_stack)
	{
		os << "Transcribe incompatible call stack trace:" << std::endl;

		std::vector<GPlatesUtils::CallStack::Trace>::const_iterator trace_iter =
				d_transcribe_incompatible_call_stack->begin();
		std::vector<GPlatesUtils::CallStack::Trace>::const_iterator trace_end =
				d_transcribe_incompatible_call_stack->end();
		for ( ; trace_iter != trace_end; ++trace_iter)
		{
			const GPlatesUtils::CallStack::Trace &trace = *trace_iter;

			os
				<< '('
				<< trace.get_filename()
				<< ", "
				<< trace.get_line_num()
				<< ')'
				<< std::endl;
		}
	}
}
