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
#include <cmath>
#include <map>
#include <memory> // std::unique_ptr
#include <ostream>
#include <utility>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/noncopyable.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <QDebug>
#include <QFileInfo>
#include <QRegExp>

#include "TranscribeSession.h"

#include "Application.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/CoRegistrationLayerParams.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Layer.h"
#include "app-logic/LayerParamsVisitor.h"
#include "app-logic/LayerTaskRegistry.h"
#include "app-logic/ReconstructGraph.h"
#include "app-logic/RasterLayerParams.h"
#include "app-logic/ReconstructionLayerParams.h"
#include "app-logic/ReconstructLayerParams.h"
#include "app-logic/ReconstructScalarCoverageLayerParams.h"
#include "app-logic/ScalarField3DLayerParams.h"
#include "app-logic/TopologyNetworkLayerParams.h"
#include "app-logic/VelocityFieldCalculatorLayerParams.h"

#include "file-io/ErrorOpeningFileForReadingException.h"
#include "file-io/ErrorOpeningPipeFromGzipException.h"
#include "file-io/FileFormatNotSupportedException.h"
#include "file-io/FileLoadAbortedException.h"
#include "file-io/ReadErrorAccumulation.h"
#include "file-io/ReadErrorOccurrence.h"
#include "file-io/ReadErrors.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/GPlatesException.h"
#include "global/PreconditionViolationError.h"

#include "gui/AnimationController.h"
#include "gui/Colour.h"
#include "gui/ColourPaletteUtils.h"
#include "gui/Dialogs.h"
#include "gui/DrawStyleAdapters.h"
#include "gui/DrawStyleManager.h"
#include "gui/GraticuleSettings.h"
#include "gui/PythonConfiguration.h"
#include "gui/RasterColourPalette.h"
#include "gui/RenderSettings.h"
#include "gui/Symbol.h"

#include "model/TranscribeQualifiedXmlName.h"
#include "model/TranscribeStringContentTypeGenerator.h"

#include "presentation/RasterVisualLayerParams.h"
#include "presentation/ReconstructScalarCoverageVisualLayerParams.h"
#include "presentation/ReconstructVisualLayerParams.h"
#include "presentation/ScalarField3DVisualLayerParams.h"
#include "presentation/TopologyGeometryVisualLayerParams.h"
#include "presentation/TopologyNetworkVisualLayerParams.h"
#include "presentation/VelocityFieldCalculatorVisualLayerParams.h"
#include "presentation/ViewState.h"
#include "presentation/VisualLayer.h"
#include "presentation/VisualLayerParamsVisitor.h"
#include "presentation/VisualLayers.h"

#include "qt-widgets/DrawStyleDialog.h"
#include "qt-widgets/ViewportWindow.h"

#include "scribe/Scribe.h"
#include "scribe/ScribeExceptions.h"
#include "scribe/TranscribeDelegateProtocol.h"
#include "scribe/TranscribeUtils.h"

#include "view-operations/RenderedGeometryParameters.h"


namespace GPlatesPresentation
{
	namespace TranscribeSession
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


		typedef std::vector<GPlatesAppLogic::FeatureCollectionFileState::const_file_reference> const_file_reference_seq_type;

		typedef std::vector< boost::optional<GPlatesAppLogic::FeatureCollectionFileState::file_reference> >
				file_reference_on_load_seq_type;

		typedef std::vector<GPlatesAppLogic::Layer> layer_seq_type;


		/**
		 * Save the feature collection filenames.
		 */
 		void
		save_feature_collection_filenames(
				const GPlatesScribe::ObjectTag &session_state_tag,
				GPlatesScribe::Scribe &scribe,
				const_file_reference_seq_type &file_references,
				QStringList &feature_collection_filenames,
				const GPlatesAppLogic::ApplicationState &application_state)
		{
			const GPlatesAppLogic::FeatureCollectionFileState &file_state = application_state.get_feature_collection_file_state();

			QStringList feature_collection_file_paths;

			BOOST_FOREACH(
					const GPlatesAppLogic::FeatureCollectionFileState::const_file_reference &file_ref,
					file_state.get_loaded_files())
			{
				const QString absolute_filename = file_ref.get_file().get_file_info().get_qfileinfo().absoluteFilePath();

				// Ignore files with no filename (i.e. "New Feature Collection"s that only exist in memory).
				if (!absolute_filename.isEmpty())
				{
					file_references.push_back(file_ref);
					feature_collection_filenames.push_back(absolute_filename);
					feature_collection_file_paths.append(absolute_filename);
				}
			}

			// Save feature collection filenames.
			// Use the TranscribeUtils::FilePath API to generate smaller archives/transcriptions.
			GPlatesScribe::TranscribeUtils::save_file_paths(
					scribe,
					TRANSCRIBE_SOURCE,
					feature_collection_file_paths,
					session_state_tag("feature_collection_filenames"));
		}


		/**
		 * Load the feature collection filenames.
		 */
 		void
		load_feature_collection_filenames(
				const GPlatesScribe::ObjectTag &session_state_tag,
				GPlatesScribe::Scribe &scribe,
				QStringList &feature_collection_filenames)
		{
			// Load feature collection filenames.
			// Use the TranscribeUtils::FilePath API to generate smaller archives/transcriptions.
			boost::optional<QStringList> feature_collection_file_paths =
					GPlatesScribe::TranscribeUtils::load_file_paths(
							scribe,
							TRANSCRIBE_SOURCE,
							session_state_tag("feature_collection_filenames"));
			if (feature_collection_file_paths)
			{
				feature_collection_filenames = feature_collection_file_paths.get();
			}
		}


		/**
		 * Load the feature collection files and return any files not loaded (eg, due to file not existing).
		 */
 		void
		load_feature_collection_files(
				const QStringList &feature_collection_filenames,
				file_reference_on_load_seq_type &file_references_on_load)
		{
			GPlatesAppLogic::ApplicationState &application_state = Application::instance().get_application_state();
			GPlatesAppLogic::FeatureCollectionFileIO &file_io = application_state.get_feature_collection_file_io();

			// Suppress auto-creation of layers during this scope because we have session information
			// regarding which layers should be created and what their connections should be.
			SuppressAutoLayerCreationRAII raii(application_state);

			// Any files that fail to load will have a 'boost::none' file reference.
			// This is so failed loads don't stuff up our file indexing.
			file_references_on_load.resize(feature_collection_filenames.size());

			for (int file_index = 0; file_index < feature_collection_filenames.size(); ++file_index)
			{
				QString filename = feature_collection_filenames[file_index];

				// Attempt to load the current file.
				//
				// If it fails it'll report error messages in the read errors dialog,
				// and then we'll skip to the next file.
				try
				{
					GPlatesAppLogic::FeatureCollectionFileState::file_reference
							file_reference = file_io.load_file(filename);

					file_references_on_load[file_index] = file_reference;
				}
				catch (GPlatesGlobal::Exception &exc)
				{
					qWarning() << exc; // Log the detailed error message.
				}
			}
		}


		void
		save_default_reconstruction_tree_layer(
				const GPlatesScribe::ObjectTag &session_state_tag,
				GPlatesScribe::Scribe &scribe,
				const layer_seq_type &layers,
				const GPlatesAppLogic::ApplicationState &application_state)
		{
			const GPlatesAppLogic::ReconstructGraph &reconstruct_graph = application_state.get_reconstruct_graph();

			boost::optional<unsigned int> default_reconstruction_tree_layer_index;

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
					default_reconstruction_tree_layer_index = layer_index;
				}
			}

			scribe.save(
					TRANSCRIBE_SOURCE,
					default_reconstruction_tree_layer_index,
					session_state_tag("d_default_reconstruction_tree_layer_index"));
		}


		void
		load_default_reconstruction_tree_layer(
				const GPlatesScribe::ObjectTag &session_state_tag,
				GPlatesScribe::Scribe &scribe,
				const layer_seq_type &layers,
				GPlatesAppLogic::ApplicationState &application_state)
		{
			boost::optional<unsigned int> default_reconstruction_tree_layer_index;
			if (scribe.transcribe(
					TRANSCRIBE_SOURCE,
					default_reconstruction_tree_layer_index,
					session_state_tag("d_default_reconstruction_tree_layer_index")))
			{
				if (default_reconstruction_tree_layer_index)
				{
					// If layer index is in-bounds, otherwise abort setting of default reconstruction tree layer.
					if (default_reconstruction_tree_layer_index.get() < layers.size())
					{
						GPlatesAppLogic::Layer default_reconstruction_tree_layer =
								layers[default_reconstruction_tree_layer_index.get()];

						// Set the default reconstruction tree layer.
						//
						// We might have already removed it if its main input channel files were
						// not loaded (eg, didn't exist), or if layer failed to load in the first place.
						// If so then we don't set it as the default.
						if (default_reconstruction_tree_layer.is_valid())
						{
							GPlatesAppLogic::ReconstructGraph &reconstruct_graph = application_state.get_reconstruct_graph();
							reconstruct_graph.set_default_reconstruction_tree_layer(default_reconstruction_tree_layer);
						}
					}
				}
			}
		}


		void
		save_layers_visual_order(
				const GPlatesScribe::ObjectTag &session_state_tag,
				GPlatesScribe::Scribe &scribe,
				const layer_seq_type &layers,
				const ViewState &view_state)
		{
			const VisualLayers &visual_layers = view_state.get_visual_layers();

			// Visual order of layers (from front-to-back) as indices into the rendered geometry layers.
			const VisualLayers::rendered_geometry_layer_seq_type &
					rendered_geometry_layer_order = visual_layers.get_layer_order();

			// Visual order of layers (from front-to-back) as indices into 'layers'.
			std::vector<unsigned int> layer_order;

			//
			// Determine the layer ordering in terms of our layer indices instead of rendered geometry layer indices.
			//
			for (unsigned int layer_index = 0; layer_index < layers.size(); ++layer_index)
			{
				const GPlatesAppLogic::Layer &layer = layers[layer_index];

				boost::shared_ptr<const VisualLayer> visual_layer = visual_layers.get_visual_layer(layer).lock();
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						visual_layer,
						GPLATES_ASSERTION_SOURCE);

				// Find the index of the current layer in the layer ordering.
				const VisualLayers::rendered_geometry_layer_seq_type::const_iterator
						rendered_geometry_layer_order_iter = std::find(
								rendered_geometry_layer_order.begin(),
								rendered_geometry_layer_order.end(),
								visual_layer->get_rendered_geometry_layer_index());
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						rendered_geometry_layer_order_iter != rendered_geometry_layer_order.end(),
						GPLATES_ASSERTION_SOURCE);

				const unsigned int order_index =
						rendered_geometry_layer_order_iter - rendered_geometry_layer_order.begin();

				layer_order.push_back(order_index);
			}

			// Save the layer ordering (uses sequence protocol since saving a sequence).
			scribe.save(TRANSCRIBE_SOURCE, layer_order, session_state_tag("layer_order"));
		}


		void
		load_layers_visual_order(
				const GPlatesScribe::ObjectTag &session_state_tag,
				GPlatesScribe::Scribe &scribe,
				const layer_seq_type &layers,
				ViewState &view_state)
		{
			VisualLayers &visual_layers = view_state.get_visual_layers();

			// Visual order of layers (from front-to-back) as indices into 'layers'.
			std::vector<unsigned int> layer_order;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, layer_order, session_state_tag("layer_order")))
			{
				return;
			}

			if (layer_order.size() != layers.size())
			{
				// This shouldn't normally happen.
				// The transcribed data is somehow corrupted so just return and leave ordering unchanged.
				qWarning() << "Number of transcribed layers does not match number in visual layer ordering.";
				return;
			}

			//
			// Not all layers were necessarily successfully loaded and so our layer order numbers
			// might skip layers. For example, if the layer ordering of 5 transcribed layers is...
			//
			//   3 2 0 4 1
			//
			// ...but we failed to load the layer at index 1 then our ordering essentially becomes...
			//
			//   3 0 4 1
			//
			// ...but we want it to be...
			//
			//   2 0 3 1
			//
			// ...so that we can compare it to the current layer ordering of our 4 'loaded' layers.
			// To do this we add the layers to a map (to sort the order numbers) and then convert
			// that back to a vector.
			//

			typedef std::map<
					unsigned int/*layer order*/,
					GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type>
							layer_order_to_geometry_layer_map_type;
			layer_order_to_geometry_layer_map_type layer_order_to_geometry_layer_map;

			for (unsigned int layer_index = 0; layer_index < layers.size(); ++layer_index)
			{
				const GPlatesAppLogic::Layer &layer = layers[layer_index];

				if (layer.is_valid())
				{
					boost::shared_ptr<const VisualLayer> visual_layer = visual_layers.get_visual_layer(layer).lock();
					GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
							visual_layer,
							GPLATES_ASSERTION_SOURCE);

					// VisualLayers::get_layer_order() uses rendered geometry layer indices so we'll use that too.
					layer_order_to_geometry_layer_map[layer_order[layer_index]] =
							visual_layer->get_rendered_geometry_layer_index();
				}
			}

			VisualLayers::rendered_geometry_layer_seq_type final_rendered_geometry_layer_order;

			// Convert the map to an ordered sequence.
			layer_order_to_geometry_layer_map_type::const_iterator layer_order_to_geometry_layer_iter =
					layer_order_to_geometry_layer_map.begin();
			layer_order_to_geometry_layer_map_type::const_iterator layer_order_to_geometry_layer_end =
					layer_order_to_geometry_layer_map.end();
			for ( ; layer_order_to_geometry_layer_iter != layer_order_to_geometry_layer_end; ++layer_order_to_geometry_layer_iter)
			{
				final_rendered_geometry_layer_order.push_back(layer_order_to_geometry_layer_iter->second);
			}

			// Iterate over the loaded layers traversing from the back to the front of the final (desired) ordering.
			// In each iteration, if the current layer does not match the final (desired) layer in the
			// layer ordering (at the iteration index) then move it there.
			// Note that we iterate backwards because subsequent moves will not affect previous moves
			// (this would not have been the case if we had moved forwards).
			for (unsigned int n = final_rendered_geometry_layer_order.size(); n != 0; --n)
			{
				const VisualLayers::rendered_geometry_layer_seq_type &
						current_rendered_geometry_layer_order = visual_layers.get_layer_order();

				if (final_rendered_geometry_layer_order.size() != current_rendered_geometry_layer_order.size())
				{
					// This shouldn't normally happen - the number of valid layers transcribed should match
					// the number of layers we've created/loaded.
					//
					// Just return and leave the rest of the ordering unchanged.
					qWarning() << "Number of loaded layers does not match current number in visual layer ordering.";
					return;
				}

				if (current_rendered_geometry_layer_order[n - 1] != final_rendered_geometry_layer_order[n - 1])
				{
					// Find the index of the desired layer in the current layer ordering.
					const VisualLayers::rendered_geometry_layer_seq_type::const_iterator
							rendered_geometry_layer_order_iter = std::find(
									current_rendered_geometry_layer_order.begin(),
									current_rendered_geometry_layer_order.end(),
									final_rendered_geometry_layer_order[n - 1]);
					GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
							rendered_geometry_layer_order_iter != current_rendered_geometry_layer_order.end(),
							GPLATES_ASSERTION_SOURCE);

					const unsigned int from_index =
							rendered_geometry_layer_order_iter - current_rendered_geometry_layer_order.begin();

					visual_layers.move_layer(from_index, n - 1 /*to_index*/);
				}
			}
		}


		void
		save_layer_connection(
				const GPlatesScribe::ObjectTag &connection_tag,
				GPlatesScribe::Scribe &scribe,
				const GPlatesAppLogic::Layer::InputConnection &input_connection,
				const const_file_reference_seq_type &file_references,
				const layer_seq_type &layers)
		{
			GPlatesAppLogic::LayerInputChannelName::Type input_channel_name = input_connection.get_input_channel_name();

			boost::optional<GPlatesAppLogic::Layer::InputFile> input_file = input_connection.get_input_file();
			if (input_file)
			{
				// Find the input file in our list of loaded file references.
				const_file_reference_seq_type::const_iterator file_iter = std::find(
						file_references.begin(),
						file_references.end(),
						input_file->get_file());
				if (file_iter != file_references.end())
				{
					const QString absolute_filename = file_iter->get_file().get_file_info().get_qfileinfo().absoluteFilePath();

					// Ignore files with no filename (i.e. "New Feature Collection"s that only exist in memory).
					if (!absolute_filename.isEmpty())
					{
						const unsigned int file_index = file_iter - file_references.begin();

						scribe.save(TRANSCRIBE_SOURCE, input_channel_name, connection_tag("d_input_channel_name"));
						scribe.save(TRANSCRIBE_SOURCE, file_index, connection_tag("d_input_index"));
						scribe.save(TRANSCRIBE_SOURCE, true/*is_input_file*/, connection_tag("d_is_input_file"));
					}
				}
			}
			else
			{
				// The input is not a file so it must be a layer.
				GPlatesAppLogic::Layer input_layer = input_connection.get_input_layer().get();

				// Find the input layer in our list of layers.
				layer_seq_type::const_iterator layer_iter = std::find(layers.begin(), layers.end(), input_layer);
				if (layer_iter != layers.end())
				{
					const unsigned int input_layer_index = layer_iter - layers.begin();

					scribe.save(TRANSCRIBE_SOURCE, input_channel_name, connection_tag("d_input_channel_name"));
					scribe.save(TRANSCRIBE_SOURCE, input_layer_index, connection_tag("d_input_index"));
					scribe.save(TRANSCRIBE_SOURCE, false/*is_input_file*/, connection_tag("d_is_input_file"));
				}
			}
		}


		void
		load_layer_connection(
				const GPlatesScribe::ObjectTag &connection_tag,
				GPlatesScribe::Scribe &scribe,
				GPlatesAppLogic::Layer layer,
				bool &main_input_channel_file_not_loaded,
				const file_reference_on_load_seq_type &file_references_on_load,
				const layer_seq_type &layers,
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph)
		{
			// Load some parameters to help us create the layer connection.
			//
			// If failed to load parameters then skip current layer connection -
			// probably the transcription is incompatible in some way (eg, a future version
			// of GPlates saved a new layer channel name that we don't know about).
			GPlatesAppLogic::LayerInputChannelName::Type input_channel_name;
			unsigned int input_index;
			bool is_input_file;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, input_channel_name, connection_tag("d_input_channel_name")) &&
				scribe.transcribe(TRANSCRIBE_SOURCE, input_index, connection_tag("d_input_index")) &&
				scribe.transcribe(TRANSCRIBE_SOURCE, is_input_file, connection_tag("d_is_input_file")))
			{
				// Input is either a file or a layer.
				if (is_input_file)
				{
					// If file index is in-bounds, otherwise abort layer connection.
					if (input_index < file_references_on_load.size())
					{
						// Connect if the input file loaded, otherwise abort layer connection.
						if (file_references_on_load[input_index])
						{
							GPlatesAppLogic::FeatureCollectionFileState::file_reference file_reference =
									file_references_on_load[input_index].get();
							GPlatesAppLogic::Layer::InputFile input_file =
									reconstruct_graph.get_input_file(file_reference);

							layer.connect_input_to_file(input_file, input_channel_name);
						}
						else // input file not loaded...
						{
							if (input_channel_name ==
								layer.get_main_input_feature_collection_channel())
							{
								main_input_channel_file_not_loaded = true;
							}
						}
					}
				}
				else // is a layer...
				{
					// Connect if layer index is in-bounds, otherwise abort layer connection.
					if (input_index < layers.size())
					{
						GPlatesAppLogic::Layer input_layer = layers[input_index];

						// Connect to the input layer.
						//
						// We might have already removed the input layer if its main
						// input channel files were not loaded (eg, didn't exist), or
						// the layer might not have successfully loaded in the first place.
						// If so then we don't connect to it.
						if (input_layer.is_valid())
						{
							layer.connect_input_to_layer_output(input_layer, input_channel_name);
						}
					}
				}
			}
		}


		/**
		 * Saves the app-logic LayerParams of a layer.
		 */
		class SaveLayerParamsVisitor :
				public GPlatesAppLogic::ConstLayerParamsVisitor
		{
		public:

			SaveLayerParamsVisitor(
					const GPlatesScribe::ObjectTag &layer_params_tag,
					GPlatesScribe::Scribe &scribe,
					const layer_seq_type &layers) :
				d_layer_params_tag(layer_params_tag),
				d_scribe(scribe),
				d_layers(layers)
			{  }

			virtual
			void
			visit_co_registration_layer_params(
					co_registration_layer_params_type &params)
			{
				// Let 'GPlatesDataMining::ConfigurationTableRow' know about the layers (it transcribes layer indices).
				GPlatesScribe::TranscribeContext<GPlatesDataMining::ConfigurationTableRow>
						transcribe_cfg_table_row_context(d_layers);
				GPlatesScribe::Scribe::ScopedTranscribeContextGuard<GPlatesDataMining::ConfigurationTableRow>
						transcribe_cfg_table_row_context_guard(d_scribe, transcribe_cfg_table_row_context);

				// Save the config table.
				d_scribe.save(TRANSCRIBE_SOURCE, params.get_cfg_table(), d_layer_params_tag("cfg_table"));
			}

			virtual
			void
			visit_raster_layer_params(
					raster_layer_params_type &params)
			{
				// Save the band name.
				// We don't save the feature since that comes from the input file.
				d_scribe.save(TRANSCRIBE_SOURCE, params.get_band_name(), d_layer_params_tag("band_name"));
			}

			virtual
			void
			visit_reconstruction_layer_params(
					reconstruction_layer_params_type &params)
			{
				// Save the reconstruction params.
				d_scribe.save(TRANSCRIBE_SOURCE, params.get_reconstruction_params(), d_layer_params_tag("reconstruction_params"));
			}

			virtual
			void
			visit_reconstruct_layer_params(
					reconstruct_layer_params_type &params)
			{
				// Save the reconstruct params.
				d_scribe.save(TRANSCRIBE_SOURCE, params.get_reconstruct_params(), d_layer_params_tag("reconstruct_params"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_prompt_to_change_topology_reconstruction_parameters(),
						d_layer_params_tag("prompt_to_change_topology_reconstruction_parameters"));
			}

			virtual
			void
			visit_reconstruct_scalar_coverage_layer_params(
					reconstruct_scalar_coverage_layer_params_type &params)
			{
				// Save the ReconstructScalarCoverageParams.
				d_scribe.save(
						TRANSCRIBE_SOURCE,
						params.get_reconstruct_scalar_coverage_params(),
						d_layer_params_tag("reconstruct_scalar_coverage_params"));

				// Save the scalar type.
				d_scribe.save(TRANSCRIBE_SOURCE, params.get_scalar_type(), d_layer_params_tag("scalar_type"));
			}

			virtual
			void
			visit_scalar_field_3d_layer_params(
					scalar_field_3d_layer_params_type &params)
			{
				// Nothing needs to be transcribed.
			}

			virtual
			void
			visit_topology_network_layer_params(
					topology_network_layer_params_type &params)
			{
				// Save the topology network params.
				d_scribe.save(TRANSCRIBE_SOURCE, params.get_topology_network_params(), d_layer_params_tag("topology_network_params"));
			}

			virtual
			void
			visit_velocity_field_calculator_layer_params(
					velocity_field_calculator_layer_params_type &params)
			{
				// Save the velocity params.
				d_scribe.save(TRANSCRIBE_SOURCE, params.get_velocity_params(), d_layer_params_tag("velocity_params"));
			}

		private:
			GPlatesScribe::ObjectTag d_layer_params_tag;
			GPlatesScribe::Scribe &d_scribe;
			const layer_seq_type &d_layers;
		};


		/**
		 * Loads the app-logic LayerParams of a layer.
		 */
		class LoadLayerParamsVisitor :
				public GPlatesAppLogic::LayerParamsVisitor
		{
		public:

			LoadLayerParamsVisitor(
					const GPlatesScribe::ObjectTag &layer_params_tag,
					GPlatesScribe::Scribe &scribe,
					const layer_seq_type &layers) :
				d_layer_params_tag(layer_params_tag),
				d_scribe(scribe),
				d_layers(layers)
			{  }

			virtual
			void
			visit_co_registration_layer_params(
					co_registration_layer_params_type &params)
			{
				// Let 'GPlatesDataMining::ConfigurationTableRow' know about the layers (it transcribes layer indices).
				GPlatesScribe::TranscribeContext<GPlatesDataMining::ConfigurationTableRow>
						transcribe_cfg_table_row_context(d_layers);
				GPlatesScribe::Scribe::ScopedTranscribeContextGuard<GPlatesDataMining::ConfigurationTableRow>
						transcribe_cfg_table_row_context_guard(d_scribe, transcribe_cfg_table_row_context);

				// Load the config table.
				GPlatesDataMining::CoRegConfigurationTable cfg_table;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, cfg_table, d_layer_params_tag("cfg_table")))
				{
					params.set_cfg_table(cfg_table);
				}
			}

			virtual
			void
			visit_raster_layer_params(
					raster_layer_params_type &params)
			{
				// Load the band name.
				// We don't load the feature since that comes from the loaded input file.
				GPlatesScribe::LoadRef<GPlatesPropertyValues::TextContent> band_name =
						d_scribe.load<GPlatesPropertyValues::TextContent>(TRANSCRIBE_SOURCE, d_layer_params_tag("band_name"));
				if (band_name.is_valid())
				{
					params.set_band_name(band_name);
				}
			}

			virtual
			void
			visit_reconstruction_layer_params(
					reconstruction_layer_params_type &params)
			{
				// Load the reconstruction params.
				GPlatesAppLogic::ReconstructionParams reconstruction_params;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, reconstruction_params, d_layer_params_tag("reconstruction_params")))
				{
					params.set_reconstruction_params(reconstruction_params);
				}
			}

			virtual
			void
			visit_reconstruct_layer_params(
					reconstruct_layer_params_type &params)
			{
				// Load the reconstruct params.
				GPlatesAppLogic::ReconstructParams reconstruct_params;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, reconstruct_params, d_layer_params_tag("reconstruct_params")))
				{
					params.set_reconstruct_params(reconstruct_params);
				}

				bool prompt_to_change_topology_reconstruction_parameters;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, prompt_to_change_topology_reconstruction_parameters,
						d_layer_params_tag("prompt_to_change_topology_reconstruction_parameters")))
				{
					params.set_prompt_to_change_topology_reconstruction_parameters(
							prompt_to_change_topology_reconstruction_parameters);
				}
			}

			virtual
			void
			visit_reconstruct_scalar_coverage_layer_params(
					reconstruct_scalar_coverage_layer_params_type &params)
			{
				// Load the ReconstructScalarCoverageParams.
				GPlatesAppLogic::ReconstructScalarCoverageParams reconstruct_scalar_coverage_params;
				if (d_scribe.transcribe(
						TRANSCRIBE_SOURCE,
						reconstruct_scalar_coverage_params,
						d_layer_params_tag("reconstruct_scalar_coverage_params")))
				{
					params.set_reconstruct_scalar_coverage_params(reconstruct_scalar_coverage_params);
				}

				// Load the scalar type.
				GPlatesScribe::LoadRef<GPlatesPropertyValues::ValueObjectType> scalar_type =
						d_scribe.load<GPlatesPropertyValues::ValueObjectType>(TRANSCRIBE_SOURCE, d_layer_params_tag("scalar_type"));
				if (scalar_type.is_valid())
				{
					params.set_scalar_type(scalar_type);
				}
			}

			virtual
			void
			visit_scalar_field_3d_layer_params(
					scalar_field_3d_layer_params_type &params)
			{
				// Nothing needs to be transcribed.
			}

			virtual
			void
			visit_topology_network_layer_params(
					topology_network_layer_params_type &params)
			{
				// Load the topology network params.
				GPlatesAppLogic::TopologyNetworkParams topology_network_params;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, topology_network_params, d_layer_params_tag("topology_network_params")))
				{
					params.set_topology_network_params(topology_network_params);
				}
			}

			virtual
			void
			visit_velocity_field_calculator_layer_params(
					velocity_field_calculator_layer_params_type &params)
			{
				// Load the velocity params.
				GPlatesAppLogic::VelocityParams velocity_params;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, velocity_params, d_layer_params_tag("velocity_params")))
				{
					params.set_velocity_params(velocity_params);
				}
			}

		private:
			GPlatesScribe::ObjectTag d_layer_params_tag;
			GPlatesScribe::Scribe &d_scribe;
			const layer_seq_type &d_layers;
		};


		void
		save_remapped_colour_palette_parameters(
				const GPlatesScribe::ObjectTag &colour_palette_params_tag,
				GPlatesScribe::Scribe &scribe,
				const RemappedColourPaletteParameters &colour_palette_params)
		{
			// Save the built-in colour palette parameters.
			// Note that we save this even if a built-in colour palette is not loaded.
			// This is useful for keeping track of the built-in parameters for use in the built-in palette dialog.
			scribe.save(TRANSCRIBE_SOURCE, colour_palette_params.get_builtin_colour_palette_parameters(),
					colour_palette_params_tag("builtin_colour_palette_parameters"));

			boost::optional<GPlatesGui::BuiltinColourPaletteType> builtin_colour_palette_type =
					colour_palette_params.get_builtin_colour_palette_type();

			scribe.save(TRANSCRIBE_SOURCE, builtin_colour_palette_type,
					colour_palette_params_tag("builtin_colour_palette_type"));

			if (!builtin_colour_palette_type)
			{
				// Not a built-in colour palette type - so write out the palette filename.
				GPlatesScribe::TranscribeUtils::save_file_path(
						scribe, TRANSCRIBE_SOURCE, colour_palette_params.get_colour_palette_filename(),
						colour_palette_params_tag("colour_palette_filename"));
			}

			scribe.save(TRANSCRIBE_SOURCE, colour_palette_params.is_palette_range_mapped(),
					colour_palette_params_tag("is_palette_range_mapped"));

			scribe.save(TRANSCRIBE_SOURCE, colour_palette_params.get_mapped_palette_range(),
					colour_palette_params_tag("mapped_palette_range"));

			scribe.save(TRANSCRIBE_SOURCE, colour_palette_params.get_deviation_from_mean(),
					colour_palette_params_tag("deviation_from_mean"));
		}


		void
		load_remapped_colour_palette_parameters(
				const GPlatesScribe::ObjectTag &colour_palette_params_tag,
				GPlatesScribe::Scribe &scribe,
				RemappedColourPaletteParameters &colour_palette_params,
				GPlatesFileIO::ReadErrorAccumulation &read_errors)
		{
			boost::optional<GPlatesGui::BuiltinColourPaletteType> builtin_colour_palette_type;
			bool is_palette_range_mapped;
			std::pair<double, double> mapped_palette_range;
			double deviation_from_mean;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, builtin_colour_palette_type,
					colour_palette_params_tag("builtin_colour_palette_type")) ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, is_palette_range_mapped,
					colour_palette_params_tag("is_palette_range_mapped")) ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, mapped_palette_range,
					colour_palette_params_tag("mapped_palette_range")) ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, deviation_from_mean,
					colour_palette_params_tag("deviation_from_mean")))
			{
				// Return without loading the colour palette parameters (just leave it as default).
				return;
			}

			// Load the built-in colour palette parameters.
			// Note that we load this even if a built-in colour palette is not loaded.
			// This is useful for keeping track of the built-in parameters for use in the built-in palette dialog.
			GPlatesGui::BuiltinColourPaletteType::Parameters builtin_colour_palette_parameters;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, builtin_colour_palette_parameters,
					colour_palette_params_tag("builtin_colour_palette_parameters")))
			{
				builtin_colour_palette_parameters = GPlatesGui::BuiltinColourPaletteType::Parameters();
			}
			colour_palette_params.set_builtin_colour_palette_parameters(builtin_colour_palette_parameters);

			if (builtin_colour_palette_type)
			{
				colour_palette_params.load_builtin_colour_palette(builtin_colour_palette_type.get());
			}
			else
			{
				// Only load the colour palette filename if we're *not* using a convenient (internal) palette.
				// This is because the convenient palette filenames are not actually files and we don't
				// want to query the user to find it (thinking that it's a missing file).
				boost::optional<QString> colour_palette_filename =
							GPlatesScribe::TranscribeUtils::load_file_path(
									scribe, TRANSCRIBE_SOURCE, colour_palette_params_tag("colour_palette_filename"));
				if (!colour_palette_filename)
				{
					// Return without loading the colour palette parameters (just leave it as default).
					return;
				}

				if (colour_palette_filename->isEmpty())
				{
					colour_palette_params.use_default_colour_palette();
				}
				else
				{
					colour_palette_params.load_colour_palette(colour_palette_filename.get(), read_errors);
				}
			}

			// Map the palette range (even if not currently mapped) just to set up the mapped range.
			colour_palette_params.map_palette_range(mapped_palette_range.first, mapped_palette_range.second);
			if (!is_palette_range_mapped)
			{
				colour_palette_params.unmap_palette_range();
			}

			colour_palette_params.set_deviation_from_mean(deviation_from_mean);
		}


		/**
		 * Regular expression for a variant of a draw style name that ends with
		 * an underscore and a number (eg, "_1").
		 */
		const QRegExp DRAW_STYLE_NAME_VARIANT_REGEXP("^(.*)_\\d+$");

		/**
		 * Return the draw style name with any integer suffixes (eg, "_1") removed.
		 */
		QString
		get_draw_style_base_name(
				const QString &draw_style_name)
		{
			// Return the base part if ends with "_1" for example.
			if (DRAW_STYLE_NAME_VARIANT_REGEXP.indexIn(draw_style_name) >= 0)
			{
				return DRAW_STYLE_NAME_VARIANT_REGEXP.cap(1);
			}

			return draw_style_name;
		}

		/**
		 * The value in a mapping of draw style configuration item name/type to value.
		 *
		 * It's either a regular string (colour name, inbuilt palette name, etc) or a file path (for a CPT file).
		 *
		 * NOTE: File paths are transcribed using 'GPlatesScribe::TranscribeUtils::FilePath' since
		 * that enables relative file paths (when a project file is moved).
		 */
		class DrawStyleCfgItemValue :
				public boost::equality_comparable<DrawStyleCfgItemValue>
		{
		public:

			DrawStyleCfgItemValue()
			{  }

			void
			set_value(
					const QString &string_value)
			{
				d_value = string_value;
			}

			void
			set_value(
					const GPlatesScribe::TranscribeUtils::FilePath &file_path_value)
			{
				d_value = file_path_value;
			}

			QString
			get_value() const
			{
				if (const QString *string_value = boost::get<QString>(&d_value))
				{
					return *string_value;
				}

				return boost::get<GPlatesScribe::TranscribeUtils::FilePath>(d_value).get_file_path();
			}

			bool
			operator==(
					const DrawStyleCfgItemValue &rhs) const
			{
				return get_value() == rhs.get_value();
			}

		private:

			boost::variant<QString, GPlatesScribe::TranscribeUtils::FilePath> d_value;

		private: // Transcribe for sessions/projects...

			friend class GPlatesScribe::Access;

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data)
			{
				// Might as well make transcription compatible with the boost variant we are wrapping.
				// Helps if later we remove the wrapper for some reason.
				return transcribe_delegate_protocol(TRANSCRIBE_SOURCE, scribe, d_value);
			}
		};

		/**
		 * Typedef for a mapping of draw style configuration item name/type to value.
		 */
		typedef std::map<
				std::pair<QString/*name*/, QString/*type*/>,
				DrawStyleCfgItemValue/*value*/>
						draw_style_cfg_item_map_type;

		//
		// Strings representing the derived class types of 'GPlatesGui::ConfigurationItem'.
		//
		const QString DRAW_STYLE_PYTHON_CFG_COLOR_TYPE = QString("PythonCfgColor");
		const QString DRAW_STYLE_PYTHON_CFG_STRING_TYPE = QString("PythonCfgString");
		const QString DRAW_STYLE_PYTHON_CFG_PALETTE_TYPE = QString("PythonCfgPalette");

		/**
		 * Convert the draw style configuration to a map of configuration item name/type to value.
		 */
		void
		get_draw_style_cfg_item_map(
				draw_style_cfg_item_map_type &draw_style_cfg_item_map,
				const GPlatesGui::Configuration &configuration)
		{
			const std::vector<QString> cfg_item_names = configuration.all_cfg_item_names();
			for (unsigned int n = 0; n < cfg_item_names.size(); ++n)
			{
				const QString &cfg_item_name = cfg_item_names[n];

				const GPlatesGui::ConfigurationItem *cfg_item = configuration.get(cfg_item_name);

				// Ideally we should be transcribing the configuration items directly rather than
				// transcribing their types as strings, but we don't always need to create configuration
				// items (just need to match them) and they also contain Python objects, so we end up
				// just transcribing a description of the configuration items.
				QString cfg_item_type;
				DrawStyleCfgItemValue cfg_item_value;
				if (dynamic_cast<const GPlatesGui::PythonCfgColor *>(cfg_item))
				{
					cfg_item_type = DRAW_STYLE_PYTHON_CFG_COLOR_TYPE;
					cfg_item_value.set_value(cfg_item->value().toString());
				}
				else if (dynamic_cast<const GPlatesGui::PythonCfgString *>(cfg_item))
				{
					cfg_item_type = DRAW_STYLE_PYTHON_CFG_STRING_TYPE;
					cfg_item_value.set_value(cfg_item->value().toString());
				}
				else if (const GPlatesGui::PythonCfgPalette *palette_cfg_item =
						dynamic_cast<const GPlatesGui::PythonCfgPalette *>(cfg_item))
				{
					cfg_item_type = DRAW_STYLE_PYTHON_CFG_PALETTE_TYPE;

					// If it's a built-in palette then set a string value (name of built-in palette),
					// otherwise set a 'GPlatesScribe::TranscribeUtils::FilePath'
					// (filename of palette file) since that enables relative file paths
					// (when a project file is moved).

					if (palette_cfg_item->is_built_in_palette())
					{
						cfg_item_value.set_value(cfg_item->value().toString());
					}
					else
					{
						// Use QFileInfo to ensure the format of the file path is platform independent.
						// This is because we later compare file paths when searching for matching draw styles.
						const QFileInfo palette_file_info(cfg_item->value().toString());

						cfg_item_value.set_value(
								GPlatesScribe::TranscribeUtils::FilePath(
										palette_file_info.absoluteFilePath()));
					}
				}
				else
				{
					// There's another concrete derived class of 'GPlatesGui::ConfigurationItem' that
					// needs to be tested above. All derived classes should be tested above - this is
					// regardless of whether we're on the save or load path - it's a programmer error.
					GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
							false,
							GPLATES_ASSERTION_SOURCE);
				}

				draw_style_cfg_item_map.insert(
						draw_style_cfg_item_map_type::value_type(
								std::make_pair(cfg_item_name, cfg_item_type),
								cfg_item_value));
			}
		}

		/**
		 * See if the template draw style has configuration item names and types matching
		 * the specified draw style configuration mapping.
		 *
		 * Note that the configuration item values are not matched (only names and types).
		 */
		bool
		is_draw_style_compatible_with_template(
				const draw_style_cfg_item_map_type &draw_style_cfg_item_map,
				const GPlatesGui::StyleAdapter *template_draw_style)
		{
			// Convert the template draw style configuration to a map of configuration item name/type to value.
			draw_style_cfg_item_map_type template_draw_style_cfg_item_map;
			get_draw_style_cfg_item_map(template_draw_style_cfg_item_map, template_draw_style->configuration());

			// Make sure the map keys (configuration item name/type) of the template draw style match
			// the transcribed configuration keys.
			if (draw_style_cfg_item_map.size() != template_draw_style_cfg_item_map.size())
			{
				return false;
			}
			const unsigned int num_draw_style_cfg_items = draw_style_cfg_item_map.size();
			draw_style_cfg_item_map_type::const_iterator draw_style_cfg_item_map_iter = draw_style_cfg_item_map.begin();
			draw_style_cfg_item_map_type::const_iterator template_draw_style_cfg_item_map_iter = template_draw_style_cfg_item_map.begin();
			for (unsigned int cfg_item_index = 0;
				cfg_item_index < num_draw_style_cfg_items;
				++cfg_item_index, ++draw_style_cfg_item_map_iter, ++template_draw_style_cfg_item_map_iter)
			{
				// Compare configuration item name and type (but not value).
				if (draw_style_cfg_item_map_iter->first != template_draw_style_cfg_item_map_iter->first)
				{
					return false;
				}
			}

			return true;
		}

		/**
		 * Find a new draw style name (based on the specified style name) that doesn't match
		 * any style names in @a draw_styles.
		 */
		QString
		get_new_draw_style_name(
				const QString &draw_style_name,
				const GPlatesGui::DrawStyleManager::StyleContainer &draw_styles)
		{
			// Use the base name (eg, without "_1") if possible.
			const QString draw_style_base_name = get_draw_style_base_name(draw_style_name);

			// See if the draw style base name already exists.
			bool draw_style_base_name_already_exists = false;
			BOOST_FOREACH(GPlatesGui::StyleAdapter *draw_style, draw_styles)
			{
				if (draw_style_base_name == draw_style->name())
				{
					draw_style_base_name_already_exists = true;
					break;
				}
			}

			// Return draw style base name if it doesn't match any existing style names.
			if (!draw_style_base_name_already_exists)
			{
				return draw_style_base_name;
			}

			// Keep incrementing the suffix index until we get a style name that doesn't exist.
			for (unsigned int index = 1; true; ++index)
			{
				const QString suffixed_draw_style_name = QString(draw_style_base_name + "_%1").arg(index);

				bool suffixed_draw_style_name_already_exists = false;
				BOOST_FOREACH(GPlatesGui::StyleAdapter *draw_style, draw_styles)
				{
					if (suffixed_draw_style_name == draw_style->name())
					{
						suffixed_draw_style_name_already_exists = true;
						break;
					}
				}

				// Return suffixed draw style name if it doesn't match any existing style names.
				if (!suffixed_draw_style_name_already_exists)
				{
					return suffixed_draw_style_name;
				}
			}

			// Can't get here due to infinite loop above.
			return QString();
		}

		/**
		 * Add a 'ReadErrors::ErrorOpeningFileForReading' read error for any palette files in the
		 * draw style configuration that are missing.
		 */
		void
		emit_read_errors_for_missing_palette_files(
				const GPlatesGui::Configuration &configuration,
				GPlatesFileIO::ReadErrorAccumulation &read_errors)
		{
			const std::vector<QString> cfg_item_names = configuration.all_cfg_item_names();
			for (unsigned int n = 0; n < cfg_item_names.size(); ++n)
			{
				const GPlatesGui::ConfigurationItem *cfg_item = configuration.get(cfg_item_names[n]);

				// Test for a palette.
				if (const GPlatesGui::PythonCfgPalette *palette_cfg_item =
						dynamic_cast<const GPlatesGui::PythonCfgPalette *>(cfg_item))
				{
					// If it's not a built-in palette then it's a palette filename.
					if (!palette_cfg_item->is_built_in_palette())
					{
						// If the palette file doesn't exist then emit a read error.
						const QString palette_filename = palette_cfg_item->get_value();
						if (!QFileInfo(palette_filename).exists())
						{
							read_errors.d_failures_to_begin.push_back(
									GPlatesFileIO::make_read_error_occurrence(
										palette_filename,
										GPlatesFileIO::DataFormats::Unspecified,
										0/*line_num*/,
										GPlatesFileIO::ReadErrors::ErrorOpeningFileForReading,
										GPlatesFileIO::ReadErrors::FileNotLoaded));
						}
					}
				}
			}
		}

		void
		set_draw_style_on_layer(
				const GPlatesGui::StyleAdapter *draw_style,
				GPlatesPresentation::VisualLayerParams &visual_layer_params,
				boost::shared_ptr<VisualLayer> visual_layer,
				GPlatesFileIO::ReadErrorAccumulation &read_errors)
		{
			// First emit any read errors for missing palette files.
			//
			// We do this both for any new draw styles we created and any existing styles.
			// We didn't really load existing draw styles but they still might be referencing
			// a CPT file that doesn't exist, so it helps to notify the user of this.
			// Also if a project file and its data has been obtained from elsewhere (eg, zipped up
			// by another user) then this won't happen if the CPT file was included in the zip package
			// and referenced by the draw style (because the CPT file should exist in that case).
			emit_read_errors_for_missing_palette_files(draw_style->configuration(), read_errors);

#if 0
			// Set the draw style in the visual layer params.
			visual_layer_params.set_style_adapter(draw_style);
#else
			//
			// FIXME: DrawStyleDialog should update its GUI when the draw style changes in visual layer params.
			//
			// Currently DrawStyleDialog clobbers the draw style in the visual layer params.
			// DrawStyleDialog should just be one observer of visual layer params
			// (ie, it is not the only one who can change its state).
			//
			// As a temporary hack to get around this we set the draw style on the DrawStyleDialog
			// (which then sets it in the visual layer params). This means that when DrawStyleDialog is
			// popped up by the user it will reset the draw style (to the state that is stored in its GUI)
			// but that state will be up-to-date (ie, not old state).
			//
			GPlatesQtWidgets::DrawStyleDialog &draw_style_dialog =
					Application::instance().get_main_window().dialogs().draw_style_dialog();
			draw_style_dialog.reset(visual_layer, draw_style);
#endif
		}

		void
		save_draw_style(
				const GPlatesScribe::ObjectTag &draw_style_tag,
				GPlatesScribe::Scribe &scribe,
				const GPlatesPresentation::VisualLayerParams &visual_layer_params)
		{
			const GPlatesGui::StyleAdapter *draw_style = visual_layer_params.style_adapter();
			GPlatesGui::DrawStyleManager *draw_style_manager = GPlatesGui::DrawStyleManager::instance();

			if (draw_style == NULL ||
				draw_style == draw_style_manager->default_style())
			{
				// No need to save default style.
				return;
			}

			// Save the category name of the draw style.
			scribe.save(TRANSCRIBE_SOURCE, draw_style->catagory().name(), draw_style_tag("category_name"));

			// Save the draw style name.
			scribe.save(TRANSCRIBE_SOURCE, draw_style->name(), draw_style_tag("style_name"));

			// Convert the draw style configuration to a map of configuration item name/type to value.
			draw_style_cfg_item_map_type cfg_item_map;
			get_draw_style_cfg_item_map(cfg_item_map, draw_style->configuration());

			// Transcribe the configuration of the draw style.
			scribe.save(TRANSCRIBE_SOURCE, cfg_item_map, draw_style_tag("configuration"));
		}


		void
		load_draw_style(
				const GPlatesScribe::ObjectTag &draw_style_tag,
				GPlatesScribe::Scribe &scribe,
				GPlatesPresentation::VisualLayerParams &visual_layer_params,
				boost::shared_ptr<VisualLayer> visual_layer,
				GPlatesFileIO::ReadErrorAccumulation &read_errors)
		{
			GPlatesGui::DrawStyleManager *draw_style_manager = GPlatesGui::DrawStyleManager::instance();

			// Get the style category from the transcribed category name.
			QString style_category_name;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, style_category_name, draw_style_tag("category_name")))
			{
				// Return early and leave as default style.
				return;
			}

			const GPlatesGui::StyleCategory *style_category = draw_style_manager->get_catagory(style_category_name);
			if (style_category == NULL)
			{
				// Unable to find a style category.
				// Probably a new category (saved by a future version of GPlates) or a deprecated category.
				// Return early and leave as default style.
				return;
			}

			// Load the draw style name.
			QString draw_style_name;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, draw_style_name, draw_style_tag("style_name")))
			{
				// Return early and leave as default style.
				return;
			}

			// Load the configuration of the draw style.
			draw_style_cfg_item_map_type draw_style_cfg_item_map;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, draw_style_cfg_item_map, draw_style_tag("configuration")))
			{
				// Return early and leave as default style.
				return;
			}

			//
			// Find an existing draw style (if any) that matches the transcribed style name and configuration.
			//

			const QString draw_style_base_name = get_draw_style_base_name(draw_style_name);
			const GPlatesGui::DrawStyleManager::StyleContainer draw_styles = draw_style_manager->get_styles(*style_category);

			BOOST_FOREACH(GPlatesGui::StyleAdapter *draw_style, draw_styles)
			{
				// See if the current draw style name is compatible.
				if (draw_style_base_name == get_draw_style_base_name(draw_style->name()))
				{
					// Convert the current draw style configuration to a map of configuration item name/type to value.
					draw_style_cfg_item_map_type cfg_item_map;
					get_draw_style_cfg_item_map(cfg_item_map, draw_style->configuration());

					// See if the current draw style configuration matches.
					if (draw_style_cfg_item_map == cfg_item_map)
					{
						// Set current existing draw style on the layer and return.
						set_draw_style_on_layer(draw_style, visual_layer_params, visual_layer, read_errors);
						return;
					}
				}
			}

			//
			// We didn't find a matching draw style, but if the template draw style has matching
			// configuration item names and types then we can create a new draw style from it.
			//

			const GPlatesGui::StyleAdapter *template_draw_style = draw_style_manager->get_template_style(*style_category);
			if (template_draw_style == NULL ||
				!is_draw_style_compatible_with_template(draw_style_cfg_item_map, template_draw_style))
			{
				// Return early and leave as default style.
				return;
			}

			// Find a new draw style name that doesn't match any existing style names.
			const QString new_draw_style_name = get_new_draw_style_name(draw_style_name, draw_styles);

			// Create a new draw style from the template.
			std::unique_ptr<GPlatesGui::StyleAdapter> new_draw_style_owner(template_draw_style->deep_clone());
			GPlatesGui::StyleAdapter *new_draw_style = new_draw_style_owner.get();
			if (new_draw_style == NULL)
			{
				// Return early and leave as default style.
				return;
			}
			new_draw_style->set_name(new_draw_style_name);

			// Get the configuration of the new draw style.
			GPlatesGui::Configuration& new_draw_style_configuration = new_draw_style->configuration();

			// Set configuration using the transcribed configuration items.
			draw_style_cfg_item_map_type::const_iterator draw_style_cfg_item_map_iter = draw_style_cfg_item_map.begin();
			draw_style_cfg_item_map_type::const_iterator draw_style_cfg_item_map_end = draw_style_cfg_item_map.end();
			for ( ; draw_style_cfg_item_map_iter != draw_style_cfg_item_map_end; ++draw_style_cfg_item_map_iter)
			{
				const QString cfg_item_name = draw_style_cfg_item_map_iter->first.first;
				const QString cfg_item_type = draw_style_cfg_item_map_iter->first.second;
				const DrawStyleCfgItemValue &cfg_item_value = draw_style_cfg_item_map_iter->second;

				GPlatesGui::ConfigurationItem *cfg_item = new_draw_style_configuration.get(cfg_item_name);
				if (cfg_item == NULL)
				{
					// This shouldn't happen because the template draw style already passed our
					// compatibility test and the new draw style is a clone of it.
					//
					// Return early and leave as default style.
					return;
				}

				cfg_item->set_value(QVariant(cfg_item_value.get_value()));
			}

			// Register the new draw style - this also transfers ownership.
			draw_style_manager->register_style(new_draw_style);
			new_draw_style_owner.release();

			// Set new draw style on the layer.
			set_draw_style_on_layer(new_draw_style, visual_layer_params, visual_layer, read_errors);
		}


		/**
		 * Saves the VisualLayerParams of a layer.
		 */
		class SaveVisualLayerParamsVisitor :
				public ConstVisualLayerParamsVisitor
		{
		public:

			SaveVisualLayerParamsVisitor(
					const GPlatesScribe::ObjectTag &layer_params_tag,
					GPlatesScribe::Scribe &scribe) :
				d_layer_params_tag(layer_params_tag),
				d_scribe(scribe)
			{  }

			virtual
			void
			visit_raster_visual_layer_params(
					raster_visual_layer_params_type &params)
			{
				save_remapped_colour_palette_parameters(
						d_layer_params_tag("colour_palette_params"),
						d_scribe,
						params.get_colour_palette_parameters());

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_opacity(),
						d_layer_params_tag("opacity"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_intensity(),
						d_layer_params_tag("intensity"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_surface_relief_scale(),
						d_layer_params_tag("surface_relief_scale"));
			}

			virtual
			void
			visit_reconstruct_scalar_coverage_visual_layer_params(
					reconstruct_scalar_coverage_visual_layer_params_type &params)
			{
				//
				// Save the color palette parameters associated with each scalar type (using mapping protocol).
				//

				const GPlatesScribe::ObjectTag colour_palette_params_tag = d_layer_params_tag("colour_palette_params");

				std::vector<GPlatesPropertyValues::ValueObjectType> scalar_types;
				params.get_scalar_types(scalar_types);

				unsigned int num_colour_palettes_saved = 0;
				for (unsigned int s = 0; s < scalar_types.size(); ++s)
				{
					const GPlatesPropertyValues::ValueObjectType &scalar_type = scalar_types[s];

					const RemappedColourPaletteParameters &colour_palette_params =
							params.get_colour_palette_parameters(scalar_type);

					// Save map key.
					d_scribe.save(TRANSCRIBE_SOURCE, scalar_type,
							colour_palette_params_tag.map_item_key(num_colour_palettes_saved));

					// Save map value.
					save_remapped_colour_palette_parameters(
							colour_palette_params_tag.map_item_value(num_colour_palettes_saved),
							d_scribe,
							colour_palette_params);

					++num_colour_palettes_saved;
				}

				// Save map size.
				d_scribe.save(TRANSCRIBE_SOURCE, num_colour_palettes_saved,
						colour_palette_params_tag.map_size());
			}

			virtual
			void
			visit_reconstruct_visual_layer_params(
					reconstruct_visual_layer_params_type &params)
			{
				// Save the draw style (colouring scheme).
				save_draw_style(d_layer_params_tag("draw_style"), d_scribe, params);

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_vgp_earliest_time(), d_layer_params_tag("vgp_earliest_time"));
				// Loaded by GPlates 2.2 and older (when these params were in app-logic layer params)...
				d_scribe.save(TRANSCRIBE_SOURCE, params.get_vgp_earliest_time(), d_layer_params_tag("reconstruct_params")("vgp_earliest_time"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_vgp_latest_time(), d_layer_params_tag("vgp_latest_time"));
				// Loaded by GPlates 2.2 and older (when these params were in app-logic layer params)...
				d_scribe.save(TRANSCRIBE_SOURCE, params.get_vgp_latest_time(), d_layer_params_tag("reconstruct_params")("vgp_latest_time"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_vgp_delta_t(), d_layer_params_tag("vgp_delta_t"));
				// Loaded by GPlates 2.2 and older (when these params were in app-logic layer params)...
				d_scribe.save(TRANSCRIBE_SOURCE, params.get_vgp_delta_t(), d_layer_params_tag("reconstruct_params")("vgp_delta_t"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_vgp_visibility_setting(), d_layer_params_tag("vgp_visibility_setting"));
				// Loaded by GPlates 2.2 and older (when these params were in app-logic layer params)...
				d_scribe.save(TRANSCRIBE_SOURCE, params.get_vgp_visibility_setting(), d_layer_params_tag("reconstruct_params")("vgp_visibility_setting"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_vgp_draw_circular_error(), d_layer_params_tag("vgp_draw_circular_error"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_fill_polygons(), d_layer_params_tag("fill_polygons"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_fill_polylines(), d_layer_params_tag("fill_polylines"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_fill_opacity(), d_layer_params_tag("fill_opacity"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_fill_intensity(), d_layer_params_tag("fill_intensity"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_show_topology_reconstructed_feature_geometries(),
						// Keeping original tag name for compatibility...
						d_layer_params_tag("show_deformed_feature_geometries"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_show_strain_accumulation(), d_layer_params_tag("show_strain_accumulation"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_strain_accumulation_scale(), d_layer_params_tag("strain_accumulation_scale"));
			}

			virtual
			void
			visit_scalar_field_3d_visual_layer_params(
					scalar_field_3d_visual_layer_params_type &params)
			{
				save_remapped_colour_palette_parameters(
						d_layer_params_tag("scalar_colour_palette_params"),
						d_scribe,
						params.get_scalar_colour_palette_parameters());

				save_remapped_colour_palette_parameters(
						d_layer_params_tag("gradient_colour_palette_params"),
						d_scribe,
						params.get_gradient_colour_palette_parameters());

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_render_mode(),
						d_layer_params_tag("render_mode"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_isosurface_deviation_window_mode(),
						d_layer_params_tag("isosurface_deviation_window_mode"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_isosurface_colour_mode(),
						d_layer_params_tag("isosurface_colour_mode"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_cross_section_colour_mode(),
						d_layer_params_tag("cross_section_colour_mode"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_isovalue_parameters(),
						d_layer_params_tag("isovalue_parameters"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_deviation_window_render_options(),
						d_layer_params_tag("deviation_window_render_options"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_surface_polygons_mask(),
						d_layer_params_tag("surface_polygons_mask"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_depth_restriction(),
						d_layer_params_tag("depth_restriction"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_quality_performance(),
						d_layer_params_tag("quality_performance"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_shader_test_variables(),
						d_layer_params_tag("shader_test_variables"));
			}

			virtual
			void
			visit_topology_geometry_visual_layer_params(
					topology_geometry_visual_layer_params_type &params)
			{
				// Save the draw style (colouring scheme).
				save_draw_style(d_layer_params_tag("draw_style"), d_scribe, params);

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_fill_polygons(),
						d_layer_params_tag("fill_polygons"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_fill_opacity(),
						d_layer_params_tag("fill_opacity"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_fill_intensity(),
						d_layer_params_tag("fill_intensity"));
			}

			virtual
			void
			visit_topology_network_visual_layer_params(
					topology_network_visual_layer_params_type &params)
			{
				// Save the draw style (colouring scheme).
				save_draw_style(d_layer_params_tag("draw_style"), d_scribe, params);

				// Save the dilatation colour palette filename (an empty filename means use default palette).
				GPlatesScribe::TranscribeUtils::save_file_path(
						d_scribe, TRANSCRIBE_SOURCE, params.get_dilatation_colour_palette_filename(),
						d_layer_params_tag("dilatation_colour_palette_filename"));

				// Save the second invariant colour palette filename (an empty filename means use default palette).
				GPlatesScribe::TranscribeUtils::save_file_path(
						d_scribe, TRANSCRIBE_SOURCE, params.get_second_invariant_colour_palette_filename(),
						d_layer_params_tag("second_invariant_colour_palette_filename"));

				// Save the strain rate style colour palette filename (an empty filename means use default palette).
				GPlatesScribe::TranscribeUtils::save_file_path(
						d_scribe, TRANSCRIBE_SOURCE, params.get_strain_rate_style_colour_palette_filename(),
						d_layer_params_tag("strain_rate_style_colour_palette_filename"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.show_segment_velocity(),
						d_layer_params_tag("show_segment_velocity"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_fill_rigid_blocks(),
						d_layer_params_tag("fill_rigid_blocks"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_fill_opacity(),
						d_layer_params_tag("fill_opacity"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_fill_intensity(),
						d_layer_params_tag("fill_intensity"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_triangulation_colour_mode(),
						d_layer_params_tag("colour_mode"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_triangulation_draw_mode(),
						d_layer_params_tag("draw_mode"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_min_abs_dilatation(),
						d_layer_params_tag("min_abs_dilatation"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_max_abs_dilatation(),
						d_layer_params_tag("max_abs_dilatation"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_min_abs_second_invariant(),
						d_layer_params_tag("min_abs_second_invariant"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_max_abs_second_invariant(),
						d_layer_params_tag("max_abs_second_invariant"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_min_strain_rate_style(),
						d_layer_params_tag("min_strain_rate_style"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_max_strain_rate_style(),
						d_layer_params_tag("max_strain_rate_style"));

				// Only used by GPlates 2.0 (removed in 2.1) ...
				d_scribe.save(TRANSCRIBE_SOURCE,
						bool(params.get_triangulation_draw_mode() == TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_FILL),
						d_layer_params_tag("fill_triangulation"));

				// Only used by GPlates internal versions after 1.5 but before 2.0 ...
				d_scribe.save(TRANSCRIBE_SOURCE,
						bool(params.get_triangulation_draw_mode() == TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_FILL),
						d_layer_params_tag("show_fill"));
				d_scribe.save(TRANSCRIBE_SOURCE, -std::log10(params.get_max_abs_dilatation()),
						d_layer_params_tag("range1_min"));
				d_scribe.save(TRANSCRIBE_SOURCE, -std::log10(params.get_min_abs_dilatation()),
						d_layer_params_tag("range1_max"));
				d_scribe.save(TRANSCRIBE_SOURCE, std::log10(params.get_min_abs_dilatation()),
						d_layer_params_tag("range2_min"));
				d_scribe.save(TRANSCRIBE_SOURCE, std::log10(params.get_max_abs_dilatation()),
						d_layer_params_tag("range2_max", 0));
			}

			virtual
			void
			visit_velocity_field_calculator_visual_layer_params(
					velocity_field_calculator_visual_layer_params_type &params)
			{
				d_scribe.save(TRANSCRIBE_SOURCE, params.get_arrow_body_scale(),
						d_layer_params_tag("arrow_body_scale"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_arrowhead_scale(),
						d_layer_params_tag("arrowhead_scale"));

				d_scribe.save(TRANSCRIBE_SOURCE, params.get_arrow_spacing(),
						d_layer_params_tag("arrow_spacing"));
			}

		private:
			GPlatesScribe::ObjectTag d_layer_params_tag;
			GPlatesScribe::Scribe &d_scribe;
		};


		/**
		 * Loads the VisualLayerParams of a layer.
		 */
		class LoadVisualLayerParamsVisitor :
				public VisualLayerParamsVisitor
		{
		public:

			LoadVisualLayerParamsVisitor(
					const GPlatesScribe::ObjectTag &layer_params_tag,
					GPlatesScribe::Scribe &scribe,
					boost::shared_ptr<VisualLayer> visual_layer,
					GPlatesFileIO::ReadErrorAccumulation &read_errors) :
				d_layer_params_tag(layer_params_tag),
				d_scribe(scribe),
				d_visual_layer(visual_layer),
				d_read_errors(read_errors)
			{  }

			virtual
			void
			visit_raster_visual_layer_params(
					raster_visual_layer_params_type &params)
			{
				RemappedColourPaletteParameters colour_palette_params =
						raster_visual_layer_params_type::create_default_colour_palette_parameters();
				load_remapped_colour_palette_parameters(
						d_layer_params_tag("colour_palette_params"),
						d_scribe,
						colour_palette_params,
						d_read_errors);
				params.set_colour_palette_parameters(colour_palette_params);

				double opacity;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, opacity,
						d_layer_params_tag("opacity")))
				{
					params.set_opacity(opacity);
				}

				double intensity;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, intensity,
						d_layer_params_tag("intensity")))
				{
					params.set_intensity(intensity);
				}

				float surface_relief_scale;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, surface_relief_scale,
						d_layer_params_tag("surface_relief_scale")))
				{
					params.set_surface_relief_scale(surface_relief_scale);
				}
			}

			virtual
			void
			visit_reconstruct_scalar_coverage_visual_layer_params(
					reconstruct_scalar_coverage_visual_layer_params_type &params)
			{
				//
				// Load the color palette parameters associated with each scalar type (using mapping protocol).
				//

				const GPlatesScribe::ObjectTag colour_palette_params_tag = d_layer_params_tag("colour_palette_params");

				// Load map size.
				unsigned int num_colour_palettes;
				if (!d_scribe.transcribe(TRANSCRIBE_SOURCE, num_colour_palettes,
						colour_palette_params_tag.map_size()))
				{
					// Return without loading the colour palette parameters (just leave it as default).
					return;
				}

				for (unsigned int c = 0; c < num_colour_palettes; ++c)
				{
					// Load map key.
					GPlatesScribe::LoadRef<GPlatesPropertyValues::ValueObjectType> scalar_type =
							d_scribe.load<GPlatesPropertyValues::ValueObjectType>(TRANSCRIBE_SOURCE,
									colour_palette_params_tag.map_item_key(c));
					if (!scalar_type.is_valid())
					{
						// Skip to the next colour palette.
						continue;
					}

					// Load map value.
					RemappedColourPaletteParameters colour_palette_params =
							reconstruct_scalar_coverage_visual_layer_params_type::create_default_colour_palette_parameters();
					load_remapped_colour_palette_parameters(
							colour_palette_params_tag.map_item_value(c),
							d_scribe,
							colour_palette_params,
							d_read_errors);

					params.set_colour_palette_parameters(scalar_type, colour_palette_params);
				}
			}

			virtual
			void
			visit_reconstruct_visual_layer_params(
					reconstruct_visual_layer_params_type &params)
			{
				// Load the draw style (colouring scheme).
				load_draw_style(d_layer_params_tag("draw_style"), d_scribe, params, d_visual_layer, d_read_errors);

				GPlatesPropertyValues::GeoTimeInstant vgp_earliest_time(0);
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, vgp_earliest_time, d_layer_params_tag("vgp_earliest_time")) ||
					// Saved by GPlates 2.2 and older (when these params were in app-logic layer params)...
					d_scribe.transcribe(TRANSCRIBE_SOURCE, vgp_earliest_time, d_layer_params_tag("reconstruct_params")("vgp_earliest_time")))
				{
					params.set_vgp_earliest_time(vgp_earliest_time);
				}

				GPlatesPropertyValues::GeoTimeInstant vgp_latest_time(0);
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, vgp_latest_time, d_layer_params_tag("vgp_latest_time")) ||
					// Saved by GPlates 2.2 and older (when these params were in app-logic layer params)...
					d_scribe.transcribe(TRANSCRIBE_SOURCE, vgp_latest_time, d_layer_params_tag("reconstruct_params")("vgp_latest_time")))
				{
					params.set_vgp_latest_time(vgp_latest_time);
				}

				double vgp_delta_t;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, vgp_delta_t, d_layer_params_tag("vgp_delta_t")) ||
					// Saved by GPlates 2.2 and older (when these params were in app-logic layer params)...
					d_scribe.transcribe(TRANSCRIBE_SOURCE, vgp_delta_t, d_layer_params_tag("reconstruct_params")("vgp_delta_t")))
				{
					params.set_vgp_delta_t(vgp_delta_t);
				}

				ReconstructVisualLayerParams::VGPVisibilitySetting vgp_visibility_setting;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, vgp_visibility_setting, d_layer_params_tag("vgp_visibility_setting")) ||
					// Saved by GPlates 2.2 and older (when these params were in app-logic layer params)...
					d_scribe.transcribe(TRANSCRIBE_SOURCE, vgp_visibility_setting, d_layer_params_tag("reconstruct_params")("vgp_visibility_setting")))
				{
					params.set_vgp_visibility_setting(vgp_visibility_setting);
				}

				bool vgp_draw_circular_error;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, vgp_draw_circular_error,
						d_layer_params_tag("vgp_draw_circular_error")))
				{
					params.set_vgp_draw_circular_error(vgp_draw_circular_error);
				}

				bool fill_polygons;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, fill_polygons,
						d_layer_params_tag("fill_polygons")))
				{
					params.set_fill_polygons(fill_polygons);
				}

				bool fill_polylines;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, fill_polylines,
						d_layer_params_tag("fill_polylines")))
				{
					params.set_fill_polylines(fill_polylines);
				}

				double fill_opacity;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, fill_opacity,
						d_layer_params_tag("fill_opacity")))
				{
					params.set_fill_opacity(fill_opacity);
				}

				double fill_intensity;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, fill_intensity,
						d_layer_params_tag("fill_intensity")))
				{
					params.set_fill_intensity(fill_intensity);
				}

				bool show_topology_reconstructed_feature_geometries;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, show_topology_reconstructed_feature_geometries,
						// Keeping original tag name for compatibility...
						d_layer_params_tag("show_deformed_feature_geometries")))
				{
					params.set_show_topology_reconstructed_feature_geometries(show_topology_reconstructed_feature_geometries);
				}

				bool show_strain_accumulation;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, show_strain_accumulation,
						d_layer_params_tag("show_strain_accumulation")))
				{
					params.set_show_strain_accumulation(show_strain_accumulation);
				}

				double strain_accumulation_scale;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, strain_accumulation_scale,
						d_layer_params_tag("strain_accumulation_scale")))
				{
					params.set_strain_accumulation_scale(strain_accumulation_scale);
				}
			}

			virtual
			void
			visit_scalar_field_3d_visual_layer_params(
					scalar_field_3d_visual_layer_params_type &params)
			{
				RemappedColourPaletteParameters scalar_colour_palette_params =
						scalar_field_3d_visual_layer_params_type::create_default_scalar_colour_palette_parameters();
				load_remapped_colour_palette_parameters(
						d_layer_params_tag("scalar_colour_palette_params"),
						d_scribe,
						scalar_colour_palette_params,
						d_read_errors);
				params.set_scalar_colour_palette_parameters(scalar_colour_palette_params);

				RemappedColourPaletteParameters gradient_colour_palette_params =
						scalar_field_3d_visual_layer_params_type::create_default_gradient_colour_palette_parameters();
				load_remapped_colour_palette_parameters(
						d_layer_params_tag("gradient_colour_palette_params"),
						d_scribe,
						gradient_colour_palette_params,
						d_read_errors);
				params.set_gradient_colour_palette_parameters(gradient_colour_palette_params);

				GPlatesViewOperations::ScalarField3DRenderParameters::RenderMode render_mode;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, render_mode,
						d_layer_params_tag("render_mode")))
				{
					params.set_render_mode(render_mode);
				}

				GPlatesViewOperations::ScalarField3DRenderParameters::IsosurfaceDeviationWindowMode isosurface_deviation_window_mode;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, isosurface_deviation_window_mode,
						d_layer_params_tag("isosurface_deviation_window_mode")))
				{
					params.set_isosurface_deviation_window_mode(isosurface_deviation_window_mode);
				}

				GPlatesViewOperations::ScalarField3DRenderParameters::IsosurfaceColourMode isosurface_colour_mode;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, isosurface_colour_mode,
						d_layer_params_tag("isosurface_colour_mode")))
				{
					params.set_isosurface_colour_mode(isosurface_colour_mode);
				}

				GPlatesViewOperations::ScalarField3DRenderParameters::CrossSectionColourMode cross_section_colour_mode;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, cross_section_colour_mode,
						d_layer_params_tag("cross_section_colour_mode")))
				{
					params.set_cross_section_colour_mode(cross_section_colour_mode);
				}

				GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters isovalue_parameters;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, isovalue_parameters,
						d_layer_params_tag("isovalue_parameters")))
				{
					params.set_isovalue_parameters(isovalue_parameters);
				}

				GPlatesViewOperations::ScalarField3DRenderParameters::DeviationWindowRenderOptions deviation_window_render_options;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, deviation_window_render_options,
						d_layer_params_tag("deviation_window_render_options")))
				{
					params.set_deviation_window_render_options(deviation_window_render_options);
				}

				GPlatesViewOperations::ScalarField3DRenderParameters::SurfacePolygonsMask surface_polygons_mask;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, surface_polygons_mask,
						d_layer_params_tag("surface_polygons_mask")))
				{
					params.set_surface_polygons_mask(surface_polygons_mask);
				}

				GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction depth_restriction;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, depth_restriction,
						d_layer_params_tag("depth_restriction")))
				{
					params.set_depth_restriction(depth_restriction);
				}

				GPlatesViewOperations::ScalarField3DRenderParameters::QualityPerformance quality_performance;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, quality_performance,
						d_layer_params_tag("quality_performance")))
				{
					params.set_quality_performance(quality_performance);
				}

				std::vector<float> shader_test_variables;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, shader_test_variables,
						d_layer_params_tag("shader_test_variables")))
				{
					params.set_shader_test_variables(shader_test_variables);
				}
			}

			virtual
			void
			visit_topology_geometry_visual_layer_params(
					topology_geometry_visual_layer_params_type &params)
			{
				// Load the draw style (colouring scheme).
				load_draw_style(d_layer_params_tag("draw_style"), d_scribe, params, d_visual_layer, d_read_errors);

				bool fill_polygons;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, fill_polygons,
						d_layer_params_tag("fill_polygons")))
				{
					params.set_fill_polygons(fill_polygons);
				}

				double fill_opacity;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, fill_opacity,
						d_layer_params_tag("fill_opacity")))
				{
					params.set_fill_opacity(fill_opacity);
				}

				double fill_intensity;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, fill_intensity,
						d_layer_params_tag("fill_intensity")))
				{
					params.set_fill_intensity(fill_intensity);
				}
			}

			virtual
			void
			visit_topology_network_visual_layer_params(
					topology_network_visual_layer_params_type &params)
			{
				// Load the draw style (colouring scheme).
				load_draw_style(d_layer_params_tag("draw_style"), d_scribe, params, d_visual_layer, d_read_errors);

				bool show_segment_velocity;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, show_segment_velocity,
						d_layer_params_tag("show_segment_velocity")))
				{
					params.set_show_segment_velocity(show_segment_velocity);
				}

				bool fill_rigid_blocks;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, fill_rigid_blocks,
						d_layer_params_tag("fill_rigid_blocks")))
				{
					params.set_fill_rigid_blocks(fill_rigid_blocks);
				}

				double fill_opacity;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, fill_opacity,
						d_layer_params_tag("fill_opacity")))
				{
					params.set_fill_opacity(fill_opacity);
				}

				double fill_intensity;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, fill_intensity,
						d_layer_params_tag("fill_intensity")))
				{
					params.set_fill_intensity(fill_intensity);
				}

				double max_abs_dilatation;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, max_abs_dilatation,
						d_layer_params_tag("max_abs_dilatation")))
				{
					params.set_max_abs_dilatation(max_abs_dilatation);
				}
				// Saved by GPlates internal versions after 1.5 but before 2.0 ...
				else if (d_scribe.transcribe(TRANSCRIBE_SOURCE, max_abs_dilatation,
						d_layer_params_tag("range1_min")))
				{
					max_abs_dilatation = std::pow(10.0, -max_abs_dilatation);
					params.set_max_abs_dilatation(max_abs_dilatation);
				}

				double min_abs_dilatation;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, min_abs_dilatation,
						d_layer_params_tag("min_abs_dilatation")))
				{
					params.set_min_abs_dilatation(min_abs_dilatation);
				}
				// Saved by GPlates internal versions after 1.5 but before 2.0 ...
				else if (d_scribe.transcribe(TRANSCRIBE_SOURCE, min_abs_dilatation,
						d_layer_params_tag("range1_max")))
				{
					min_abs_dilatation = std::pow(10.0, -min_abs_dilatation);
					params.set_min_abs_dilatation(min_abs_dilatation);
				}

				double max_abs_second_invariant;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, max_abs_second_invariant,
						d_layer_params_tag("max_abs_second_invariant")))
				{
					params.set_max_abs_second_invariant(max_abs_second_invariant);
				}
				// Saved by GPlates internal versions after 1.5 but before 2.0 ...
				else if (d_scribe.transcribe(TRANSCRIBE_SOURCE, max_abs_second_invariant,
						d_layer_params_tag("range1_min")))
				{
					max_abs_second_invariant = std::pow(10.0, -max_abs_second_invariant);
					params.set_max_abs_second_invariant(max_abs_second_invariant);
				}

				double min_abs_second_invariant;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, min_abs_second_invariant,
						d_layer_params_tag("min_abs_second_invariant")))
				{
					params.set_min_abs_second_invariant(min_abs_second_invariant);
				}
				// Saved by GPlates internal versions after 1.5 but before 2.0 ...
				else if (d_scribe.transcribe(TRANSCRIBE_SOURCE, min_abs_second_invariant,
						d_layer_params_tag("range1_max")))
				{
					min_abs_second_invariant = std::pow(10.0, -min_abs_second_invariant);
					params.set_min_abs_second_invariant(min_abs_second_invariant);
				}

				double max_strain_rate_style;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, max_strain_rate_style,
						d_layer_params_tag("max_strain_rate_style")))
				{
					params.set_max_strain_rate_style(max_strain_rate_style);
				}

				double min_strain_rate_style;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, min_strain_rate_style,
						d_layer_params_tag("min_strain_rate_style")))
				{
					params.set_min_strain_rate_style(min_strain_rate_style);
				}

				TopologyNetworkVisualLayerParams::TriangulationColourMode colour_mode;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, colour_mode,
						d_layer_params_tag("colour_mode")))
				{
					params.set_triangulation_colour_mode(colour_mode);
				}

				TopologyNetworkVisualLayerParams::TriangulationDrawMode draw_mode;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, draw_mode, d_layer_params_tag("draw_mode")))
				{
					params.set_triangulation_draw_mode(draw_mode);
				}
				else
				{
					bool fill_triangulation;
					// Saved by GPlates 2.0 (removed in 2.1) ...
					if (d_scribe.transcribe(TRANSCRIBE_SOURCE, fill_triangulation, d_layer_params_tag("fill_triangulation")) ||
						// Saved by GPlates internal versions after 1.5 but before 2.0 ...
						d_scribe.transcribe(TRANSCRIBE_SOURCE, fill_triangulation, d_layer_params_tag("show_fill")))
					{
						if (fill_triangulation)
						{
							params.set_triangulation_draw_mode(TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_FILL);
						}
						else
						{
							// Unfilled triangulations were previously:
							// - drawn as a boundary when colouring by draw style (ie, only boundary was drawn, not triangulation),
							// - drawn as a mesh when colouring by strain rate.
							params.set_triangulation_draw_mode(
									colour_mode == TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_DRAW_STYLE
									? TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_NONE
									: TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_MESH);
						}
					}
				}

				//
				// Load the strain dilatation colour palette from a file (if a non-empty filename specified).
				//

				boost::optional<QString> dilatation_colour_palette_filename =
						GPlatesScribe::TranscribeUtils::load_file_path(
								d_scribe, TRANSCRIBE_SOURCE, d_layer_params_tag("dilatation_colour_palette_filename"));
				if (!dilatation_colour_palette_filename)
				{
					// GPlates internal versions after 1.5 but before 2.0 used a different tag...
					dilatation_colour_palette_filename = GPlatesScribe::TranscribeUtils::load_file_path(
							d_scribe, TRANSCRIBE_SOURCE, d_layer_params_tag("colour_palette_filename"));
				}
				if (dilatation_colour_palette_filename &&
					!dilatation_colour_palette_filename->isEmpty())
				{
					GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type raster_colour_palette =
							GPlatesGui::ColourPaletteUtils::read_cpt_raster_colour_palette(
									dilatation_colour_palette_filename.get(),
									// We only allow real-valued colour palettes since our data is real-valued...
									false/*allow_integer_colour_palette*/,
									d_read_errors);

					// If we successfully read a real-valued colour palette.
					boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type> colour_palette =
							GPlatesGui::RasterColourPaletteExtract::get_colour_palette<double>(*raster_colour_palette);
					if (colour_palette)
					{
						params.set_dilatation_colour_palette(dilatation_colour_palette_filename.get(), colour_palette.get());
					}
					else
					{
						// Load the default strain dilatation colour palette.
						params.use_default_dilatation_colour_palette();
					}
				}
				else
				{
					// Load the default strain dilatation colour palette.
					params.use_default_dilatation_colour_palette();
				}

				//
				// Load the strain second invariant colour palette from a file (if a non-empty filename specified).
				//

				boost::optional<QString> second_invariant_colour_palette_filename =
						GPlatesScribe::TranscribeUtils::load_file_path(
								d_scribe, TRANSCRIBE_SOURCE, d_layer_params_tag("second_invariant_colour_palette_filename"));
				if (second_invariant_colour_palette_filename &&
					!second_invariant_colour_palette_filename->isEmpty())
				{
					GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type raster_colour_palette =
							GPlatesGui::ColourPaletteUtils::read_cpt_raster_colour_palette(
									second_invariant_colour_palette_filename.get(),
									// We only allow real-valued colour palettes since our data is real-valued...
									false/*allow_integer_colour_palette*/,
									d_read_errors);

					// If we successfully read a real-valued colour palette.
					boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type> colour_palette =
							GPlatesGui::RasterColourPaletteExtract::get_colour_palette<double>(*raster_colour_palette);
					if (colour_palette)
					{
						params.set_second_invariant_colour_palette(second_invariant_colour_palette_filename.get(), colour_palette.get());
					}
					else
					{
						// Load the default strain second invariant colour palette.
						params.use_default_second_invariant_colour_palette();
					}
				}
				else
				{
					// Load the default strain second invariant colour palette.
					params.use_default_second_invariant_colour_palette();
				}

				//
				// Load the strain rate style colour palette from a file (if a non-empty filename specified).
				//

				boost::optional<QString> strain_rate_style_colour_palette_filename =
						GPlatesScribe::TranscribeUtils::load_file_path(
								d_scribe, TRANSCRIBE_SOURCE, d_layer_params_tag("strain_rate_style_colour_palette_filename"));
				if (strain_rate_style_colour_palette_filename &&
					!strain_rate_style_colour_palette_filename->isEmpty())
				{
					GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type raster_colour_palette =
							GPlatesGui::ColourPaletteUtils::read_cpt_raster_colour_palette(
									strain_rate_style_colour_palette_filename.get(),
									// We only allow real-valued colour palettes since our data is real-valued...
									false/*allow_integer_colour_palette*/,
									d_read_errors);

					// If we successfully read a real-valued colour palette.
					boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type> colour_palette =
							GPlatesGui::RasterColourPaletteExtract::get_colour_palette<double>(*raster_colour_palette);
					if (colour_palette)
					{
						params.set_strain_rate_style_colour_palette(strain_rate_style_colour_palette_filename.get(), colour_palette.get());
					}
					else
					{
						// Load the default strain strain rate style colour palette.
						params.use_default_strain_rate_style_colour_palette();
					}
				}
				else
				{
					// Load the default strain strain rate style colour palette.
					params.use_default_strain_rate_style_colour_palette();
				}
			}

			virtual
			void
			visit_velocity_field_calculator_visual_layer_params(
					velocity_field_calculator_visual_layer_params_type &params)
			{
				float arrow_body_scale;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, arrow_body_scale,
						d_layer_params_tag("arrow_body_scale")))
				{
					params.set_arrow_body_scale(arrow_body_scale);
				}

				float arrowhead_scale;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, arrowhead_scale,
						d_layer_params_tag("arrowhead_scale")))
				{
					params.set_arrowhead_scale(arrowhead_scale);
				}

				float arrow_spacing;
				if (d_scribe.transcribe(TRANSCRIBE_SOURCE, arrow_spacing,
						d_layer_params_tag("arrow_spacing")))
				{
					params.set_arrow_spacing(arrow_spacing);
				}
			}

		private:
			GPlatesScribe::ObjectTag d_layer_params_tag;
			GPlatesScribe::Scribe &d_scribe;
			boost::shared_ptr<VisualLayer> d_visual_layer;
			GPlatesFileIO::ReadErrorAccumulation &d_read_errors;
		};


		void
		save_layer_params(
				const GPlatesScribe::ObjectTag &layer_params_tag,
				GPlatesScribe::Scribe &scribe,
				const GPlatesAppLogic::Layer &layer,
				const VisualLayer &visual_layer,
				const layer_seq_type &layers)
		{
			// Save the app-logic layer parameters.
			SaveLayerParamsVisitor save_layer_params_visitor(layer_params_tag, scribe, layers);
			layer.get_layer_params()->accept_visitor(save_layer_params_visitor);

			// Save the visual layer parameters.
			SaveVisualLayerParamsVisitor save_visual_layer_params_visitor(layer_params_tag, scribe);
			visual_layer.get_visual_layer_params()->accept_visitor(save_visual_layer_params_visitor);
		}


		void
		load_layer_params(
				const GPlatesScribe::ObjectTag &layer_params_tag,
				GPlatesScribe::Scribe &scribe,
				GPlatesAppLogic::Layer &layer,
				const layer_seq_type &layers,
				VisualLayers &visual_layers,
				GPlatesFileIO::ReadErrorAccumulation &read_errors)
		{
			// Load the app-logic layer parameters.
			LoadLayerParamsVisitor load_layer_params_visitor(layer_params_tag, scribe, layers);
			layer.get_layer_params()->accept_visitor(load_layer_params_visitor);

			boost::shared_ptr<VisualLayer> visual_layer = visual_layers.get_visual_layer(layer).lock();
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					visual_layer,
					GPLATES_ASSERTION_SOURCE);

			// Load the visual layer parameters.
			LoadVisualLayerParamsVisitor load_visual_layer_params_visitor(layer_params_tag, scribe, visual_layer, read_errors);
			visual_layer->get_visual_layer_params()->accept_visitor(load_visual_layer_params_visitor);
		}


		void
		save_layer(
				const GPlatesScribe::ObjectTag &layer_tag,
				GPlatesScribe::Scribe &scribe,
				const GPlatesAppLogic::Layer &layer,
				const layer_seq_type &layers,
				const VisualLayers &visual_layers)
		{
			// Save the app-logic layer parameters.
			scribe.save(TRANSCRIBE_SOURCE, layer.get_type(), layer_tag("d_layer_task_type"));
			scribe.save(TRANSCRIBE_SOURCE, layer.is_active(), layer_tag("d_is_active"));
			scribe.save(TRANSCRIBE_SOURCE, layer.get_auto_created(), layer_tag("d_is_auto_created"));

			// Save the associated visual layer parameters.
			boost::shared_ptr<const VisualLayer> visual_layer = visual_layers.get_visual_layer(layer).lock();
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					visual_layer,
					GPLATES_ASSERTION_SOURCE);

			scribe.save(TRANSCRIBE_SOURCE, visual_layer->get_name(), layer_tag("layer_name"));
			scribe.save(TRANSCRIBE_SOURCE, visual_layer->is_visible(), layer_tag("is_visible"));

			//
			// No longer transcribe layer widget expanded statuses.
			// Not many applications restore the GUI layout.
			// Also means user expanding or contracting widget won't trigger unsaved session state
			// changes (for project files).
			// Also project/session restore is faster and layers dialog less finicky (eg, not scrolling properly, etc).
			//
#if 0
			// Whether individual layer widgets in the layer are expanded.
			scribe.save(TRANSCRIBE_SOURCE, visual_layer->is_expanded(VisualLayer::ALL),
					layer_tag("is_expanded_all"));
			scribe.save(TRANSCRIBE_SOURCE, visual_layer->is_expanded(VisualLayer::INPUT_CHANNELS),
					layer_tag("is_expanded_input_channels"));
			scribe.save(TRANSCRIBE_SOURCE, visual_layer->is_expanded(VisualLayer::LAYER_OPTIONS),
					layer_tag("is_expanded_layer_options"));
			scribe.save(TRANSCRIBE_SOURCE, visual_layer->is_expanded(VisualLayer::ADVANCED_OPTIONS),
					layer_tag("is_expanded_advanced_options"));
#endif

			// Save the layer parameters.
			save_layer_params(layer_tag("layer_params"), scribe, layer, *visual_layer, layers);
		}


		/**
		 * Loads and returns layer if successful (otherwise returns an invalid layer).
		 */
		GPlatesAppLogic::Layer
		load_layer(
				const GPlatesScribe::ObjectTag &layer_tag,
				GPlatesScribe::Scribe &scribe,
				const std::vector<GPlatesAppLogic::LayerTaskRegistry::LayerTaskType> &layer_task_types,
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				VisualLayers &visual_layers)
		{
			// Load some parameters to help us create the layer.
			GPlatesAppLogic::LayerTaskType::Type layer_task_type;
			bool is_active;
			bool is_auto_created;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, layer_task_type, layer_tag("d_layer_task_type")) ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, is_active, layer_tag("d_is_active")) ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, is_auto_created, layer_tag("d_is_auto_created")))
			{
				// Most likely a new unknown layer type (from a future version of GPlates).
				return GPlatesAppLogic::Layer();
			}

			// Create the layer task based on the layer type.
			boost::shared_ptr<GPlatesAppLogic::LayerTask> layer_task;
			for (unsigned int t = 0; t < layer_task_types.size(); ++t)
			{
				if (layer_task_types[t].get_layer_type() == layer_task_type)
				{
					layer_task = layer_task_types[t].create_layer_task();
					break;
				}
			}

			if (!layer_task)
			{
				// Couldn't find appropriate layer task for some reason
				return GPlatesAppLogic::Layer();
			}

			GPlatesAppLogic::Layer layer = reconstruct_graph.add_layer(layer_task);

			layer.activate(is_active);
			// Was the layer originally auto-created ?
			// This is needed so the layer can be auto-destroyed if the input file
			// on its main input channel is later unloaded by the user.
			layer.set_auto_created(is_auto_created);

			// Load the associated visual layer parameters (if they exist).
			boost::shared_ptr<VisualLayer> visual_layer = visual_layers.get_visual_layer(layer).lock();
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					visual_layer,
					GPLATES_ASSERTION_SOURCE);

			// If layer name loaded and its different than the auto-generated name then set it.
			QString layer_name;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, layer_name, layer_tag("layer_name")))
			{
				if (layer_name != visual_layer->get_name())
				{
					visual_layer->set_custom_name(layer_name);
				}
			}

			// If layer visibility loaded and its different than the default setting then set it.
			bool is_visible;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, is_visible, layer_tag("is_visible")))
			{
				if (is_visible != visual_layer->is_visible())
				{
					visual_layer->set_visible(is_visible);
				}
			}

			//
			// No longer transcribe layer widget expanded statuses.
			// Not many applications restore the GUI layout.
			// Also means user expanding or contracting widget won't trigger unsaved session state
			// changes (for project files).
			// Also project/session restore is faster and layers dialog less finicky (eg, not scrolling properly, etc).
			//
#if 0
			//
			// Whether individual layer widgets in the layer are expanded.
			//

			bool is_expanded_all;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, is_expanded_all, layer_tag("is_expanded_all")))
			{
				visual_layer->set_expanded(VisualLayer::ALL, is_expanded_all);
			}

			bool is_expanded_input_channels;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, is_expanded_input_channels, layer_tag("is_expanded_input_channels")))
			{
				visual_layer->set_expanded(VisualLayer::INPUT_CHANNELS, is_expanded_input_channels);
			}

			bool is_expanded_layer_options;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, is_expanded_layer_options, layer_tag("is_expanded_layer_options")))
			{
				visual_layer->set_expanded(VisualLayer::LAYER_OPTIONS, is_expanded_layer_options);
			}

			bool is_expanded_advanced_options;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, is_expanded_advanced_options, layer_tag("is_expanded_advanced_options")))
			{
				visual_layer->set_expanded(VisualLayer::ADVANCED_OPTIONS, is_expanded_advanced_options);
			}
#endif

			return layer;
		}


		void
		save_layers(
				const GPlatesScribe::ObjectTag &session_state_tag,
				GPlatesScribe::Scribe &scribe,
				const const_file_reference_seq_type &file_references,
				const GPlatesAppLogic::ApplicationState &application_state,
				const ViewState &view_state)
		{
			const GPlatesAppLogic::ReconstructGraph &reconstruct_graph = application_state.get_reconstruct_graph();
			const VisualLayers &visual_layers = view_state.get_visual_layers();

			const GPlatesScribe::ObjectTag layers_tag = session_state_tag("d_layers");

			// Get the layers.
			layer_seq_type layers;
			layers.assign(reconstruct_graph.begin(), reconstruct_graph.end());

			// Save the transcribe information from the layers and their connections.
			for (unsigned int layer_index = 0; layer_index < layers.size(); ++layer_index)
			{
				const GPlatesAppLogic::Layer &layer = layers[layer_index];

				const GPlatesScribe::ObjectTag layer_tag = layers_tag[layer_index];

				save_layer(layer_tag, scribe, layer, layers, visual_layers);

				const GPlatesScribe::ObjectTag connections_tag = layer_tag("d_input_connections");

				// Iterate over the layer's input connections.
				std::vector<GPlatesAppLogic::Layer::InputConnection> input_connections = layer.get_all_inputs();
				for (unsigned int connection_index = 0; connection_index < input_connections.size(); ++connection_index)
				{
					save_layer_connection(
							connections_tag[connection_index],
							scribe,
							input_connections[connection_index],
							file_references,
							layers);
				}

				// Save number of connections.
				scribe.save(TRANSCRIBE_SOURCE, input_connections.size(), connections_tag.sequence_size());
			}

			// Save number of layers.
			scribe.save(TRANSCRIBE_SOURCE, layers.size(), layers_tag.sequence_size());

			// Save the visual ordering of the layers.
			save_layers_visual_order(session_state_tag, scribe, layers, view_state);

			// Transcribe the default reconstruction tree layer.
			save_default_reconstruction_tree_layer(session_state_tag, scribe, layers, application_state);
		}


		void
		load_layers(
				const GPlatesScribe::ObjectTag &session_state_tag,
				GPlatesScribe::Scribe &scribe,
				const file_reference_on_load_seq_type &file_references_on_load,
				GPlatesFileIO::ReadErrorAccumulation &read_errors,
				GPlatesAppLogic::ApplicationState &application_state,
				ViewState &view_state)
		{
			GPlatesAppLogic::ReconstructGraph &reconstruct_graph = application_state.get_reconstruct_graph();
			GPlatesAppLogic::LayerTaskRegistry &layer_task_registry = application_state.get_layer_task_registry();
			VisualLayers &visual_layers = view_state.get_visual_layers();

			// FIXME: We close the DrawStyleDialog as an optimisation that avoids drawing preview
			// icons every time we set the draw style for each layer. This can half the project/session
			// loading time in some cases.
			GPlatesQtWidgets::DrawStyleDialog &draw_style_dialog =
					Application::instance().get_main_window().dialogs().draw_style_dialog();
			draw_style_dialog.close();

			// FIXME: We need to reset the DrawStyleDialog's draw style for "all" layers
			// (ie, when "All" layers is selected in the Manage Colouring dialog) so that any layers
			// that don't have their draw style restored will use the default draw style.
			//
			// Currently we achieve this by explicitly setting the default style for all layers.
			draw_style_dialog.reset(
					boost::weak_ptr<GPlatesPresentation::VisualLayer>(), // 'all' layers
					GPlatesGui::DrawStyleManager::instance()->default_style()); // default style

			// Put all layer additions in a single add layers group.
			GPlatesAppLogic::ReconstructGraph::AddOrRemoveLayersGroup add_layers_group(reconstruct_graph);
			add_layers_group.begin_add_or_remove_layers();

			std::vector<GPlatesAppLogic::LayerTaskRegistry::LayerTaskType> layer_task_types =
					layer_task_registry.get_all_layer_task_types();

			//
			// We first need to create the layers before we can make connections.
			//

			const GPlatesScribe::ObjectTag layers_tag = session_state_tag("d_layers");

			// Load number of layers.
			unsigned int num_layers;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, num_layers, layers_tag.sequence_size()))
			{
				// Skip all layers if cannot load the number of layers.
				return;
			}

			layer_seq_type layers;

			// Load the layers.
			for (unsigned int layer_index = 0; layer_index < num_layers; ++layer_index)
			{
				const GPlatesScribe::ObjectTag layer_tag = layers_tag[layer_index];

				// If unable to load layer then an invalid layer is returned and we keep the invalid
				// layer in our layer sequence so our transcribed layer indices don't get messed up -
				// probably the transcription is incompatible in some way (eg, a future version
				// of GPlates saved a new layer type that we don't know about).
				const GPlatesAppLogic::Layer layer = load_layer(
						layer_tag,
						scribe,
						layer_task_types,
						reconstruct_graph,
						visual_layers);

				layers.push_back(layer);
			}

			//
			// Next we can make input connections for the layers.
			//

			for (unsigned int layer_index = 0; layer_index < num_layers; ++layer_index)
			{
				GPlatesAppLogic::Layer layer = layers[layer_index];

				// Skip any layers that failed to load.
				if (!layer.is_valid())
				{
					continue;
				}

				const GPlatesScribe::ObjectTag layer_tag = layers_tag[layer_index];
				const GPlatesScribe::ObjectTag connections_tag = layer_tag("d_input_connections");

				// Load number of connections.
				unsigned int num_connections;
				if (!scribe.transcribe(TRANSCRIBE_SOURCE, num_connections, connections_tag.sequence_size()))
				{
					// Skip to next layer if cannot load the number of connections.
					continue;
				}

				// Whether one or more files connected to the current layer's main input channel were
				// not loaded (if all files on this channel were not loaded then we'll delete the layer).
				bool main_input_channel_file_not_loaded = false;

				// Iterate over the layer's input connections.
				for (unsigned int connection_index = 0; connection_index < num_connections; ++connection_index)
				{
					// If layer connection failed then connection was not added and we try the next connection.
					load_layer_connection(
							connections_tag[connection_index],
							scribe,
							layer,
							main_input_channel_file_not_loaded,
							file_references_on_load,
							layers,
							reconstruct_graph);
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

			//
			// Next we can load the layer-specific parameters of each layer.
			//
			// Note: We need to do this after all layers have been created and connected because
			// some of the parameters we set rely on the layers being set up and connected so
			// they can validate allowed settings based on the features coming into a layer for example.
			//

			for (unsigned int layer_index = 0; layer_index < num_layers; ++layer_index)
			{
				GPlatesAppLogic::Layer layer = layers[layer_index];

				// Skip any layers that failed to load.
				if (!layer.is_valid())
				{
					continue;
				}

				const GPlatesScribe::ObjectTag layer_tag = layers_tag[layer_index];

				// Load the layer's parameters.
				load_layer_params(layer_tag("layer_params"), scribe, layer, layers, visual_layers, read_errors);
			}

			// End the add layers group.
			add_layers_group.end_add_or_remove_layers();

			// Transcribe the visual ordering of the layers.
			load_layers_visual_order(session_state_tag, scribe, layers, view_state);

			// Transcribe the default reconstruction tree layer
			//
			// If fail to set default reconstruction tree layer then let it keep its current default.
			load_default_reconstruction_tree_layer(session_state_tag, scribe, layers, application_state);
		}


		void
		save_application_state(
				const GPlatesScribe::ObjectTag &session_state_tag,
				GPlatesScribe::Scribe &scribe,
				const GPlatesAppLogic::ApplicationState &application_state)
		{
			////////////////////////////////////////////////////////////////////////////////////////
			// NOTE: Generally only transcribe state relevant to the individual layers.           //
			//       Global settings should usually be avoided in most cases.                     //
			////////////////////////////////////////////////////////////////////////////////////////

			// Save whether to update default reconstruction tree layer.
			scribe.save(
					TRANSCRIBE_SOURCE,
					application_state.is_updating_default_reconstruction_tree_layer(),
					session_state_tag("updating_default_reconstruction_tree_layer"));

			// Save the anchored plate ID.
			scribe.save(
					TRANSCRIBE_SOURCE,
					application_state.get_current_anchored_plate_id(),
					session_state_tag("anchored_plate_id"));
		}


		void
		load_application_state(
				const GPlatesScribe::ObjectTag &session_state_tag,
				GPlatesScribe::Scribe &scribe,
				GPlatesAppLogic::ApplicationState &application_state)
		{
			////////////////////////////////////////////////////////////////////////////////////////
			// NOTE: Generally only transcribe state relevant to the individual layers.           //
			//       Global settings should usually be avoided in most cases.                     //
			////////////////////////////////////////////////////////////////////////////////////////

			// Load whether to update default reconstruction tree layer.
			bool updating_default_reconstruction_tree_layer;
			if (scribe.transcribe(
					TRANSCRIBE_SOURCE,
					updating_default_reconstruction_tree_layer,
					session_state_tag("updating_default_reconstruction_tree_layer")))
			{
				application_state.set_update_default_reconstruction_tree_layer(
						updating_default_reconstruction_tree_layer);
			}

			// Load the anchored plate ID.
			//
			// Note that if there's no anchored plate ID to load (eg, loading from an old version project file)
			// then the default anchored plate ID (zero) at GPlates startup will be used (it has already been set
			// since the session state is always cleared to the default state just before loading a new session).
			GPlatesModel::integer_plate_id_type anchored_plate_id;
			if (scribe.transcribe(
					TRANSCRIBE_SOURCE,
					anchored_plate_id,
					session_state_tag("anchored_plate_id")))
			{
				application_state.set_anchored_plate_id(anchored_plate_id);
			}
		}


		void
		save_geometry_visibility(
				const GPlatesScribe::ObjectTag &geometry_visibility_tag,
				GPlatesScribe::Scribe &scribe,
				const GPlatesGui::RenderSettings &render_settings)
		{
			scribe.save(TRANSCRIBE_SOURCE, render_settings.show_static_points(),
					// Keeping original tag name for compatibility...
					geometry_visibility_tag("show_points"));
			scribe.save(TRANSCRIBE_SOURCE, render_settings.show_static_multipoints(),
					// Keeping original tag name for compatibility...
					geometry_visibility_tag("show_multipoints"));
			scribe.save(TRANSCRIBE_SOURCE, render_settings.show_static_lines(),
					// Keeping original tag name for compatibility...
					geometry_visibility_tag("show_lines"));
			scribe.save(TRANSCRIBE_SOURCE, render_settings.show_static_polygons(),
					// Keeping original tag name for compatibility...
					geometry_visibility_tag("show_polygons"));
			scribe.save(TRANSCRIBE_SOURCE, render_settings.show_topological_sections(), geometry_visibility_tag("show_topological_sections"));
			scribe.save(TRANSCRIBE_SOURCE, render_settings.show_topological_lines(), geometry_visibility_tag("show_topological_lines"));
			scribe.save(TRANSCRIBE_SOURCE, render_settings.show_topological_polygons(), geometry_visibility_tag("show_topological_polygons"));
			scribe.save(TRANSCRIBE_SOURCE, render_settings.show_topological_networks(), geometry_visibility_tag("show_topological_networks"));
			scribe.save(TRANSCRIBE_SOURCE, render_settings.show_velocity_arrows(),
					// Keeping original tag name for compatibility...
					geometry_visibility_tag("show_arrows"));
			scribe.save(TRANSCRIBE_SOURCE, render_settings.show_rasters(), geometry_visibility_tag("show_rasters"));
			scribe.save(TRANSCRIBE_SOURCE, render_settings.show_3d_scalar_fields(), geometry_visibility_tag("show_3d_scalar_fields"));
			scribe.save(TRANSCRIBE_SOURCE, render_settings.show_scalar_coverages(), geometry_visibility_tag("show_scalar_coverages"));
			scribe.save(TRANSCRIBE_SOURCE, render_settings.show_strings(), geometry_visibility_tag("show_strings"));
		}

		void
		load_geometry_visibility(
				const GPlatesScribe::ObjectTag &geometry_visibility_tag,
				GPlatesScribe::Scribe &scribe,
				GPlatesGui::RenderSettings &render_settings)
		{
			bool show_static_points;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, show_static_points,
					// Keeping original tag name for compatibility...
					geometry_visibility_tag("show_points")))
			{
				render_settings.set_show_static_points(show_static_points);
			}

			bool show_static_multipoints;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, show_static_multipoints,
					// Keeping original tag name for compatibility...
					geometry_visibility_tag("show_multipoints")))
			{
				render_settings.set_show_static_multipoints(show_static_multipoints);
			}

			bool show_static_lines;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, show_static_lines,
					// Keeping original tag name for compatibility...
					geometry_visibility_tag("show_lines")))
			{
				render_settings.set_show_static_lines(show_static_lines);
			}

			bool show_static_polygons;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, show_static_polygons,
					// Keeping original tag name for compatibility...
					geometry_visibility_tag("show_polygons")))
			{
				render_settings.set_show_static_polygons(show_static_polygons);
			}

			bool show_topological_sections;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, show_topological_sections, geometry_visibility_tag("show_topological_sections")))
			{
				render_settings.set_show_topological_sections(show_topological_sections);
			}

			bool show_topological_lines;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, show_topological_lines, geometry_visibility_tag("show_topological_lines")))
			{
				render_settings.set_show_topological_lines(show_topological_lines);
			}

			bool show_topological_polygons;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, show_topological_polygons, geometry_visibility_tag("show_topological_polygons")))
			{
				render_settings.set_show_topological_polygons(show_topological_polygons);
			}

			bool show_topological_networks;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, show_topological_networks, geometry_visibility_tag("show_topological_networks")))
			{
				render_settings.set_show_topological_networks(show_topological_networks);
			}

			bool show_velocity_arrows;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, show_velocity_arrows,
					// Keeping original tag name for compatibility...
					geometry_visibility_tag("show_arrows")))
			{
				render_settings.set_show_velocity_arrows(show_velocity_arrows);
			}

			bool show_rasters;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, show_rasters, geometry_visibility_tag("show_rasters")))
			{
				render_settings.set_show_rasters(show_rasters);
			}

			bool show_3d_scalar_fields;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, show_3d_scalar_fields, geometry_visibility_tag("show_3d_scalar_fields")))
			{
				render_settings.set_show_3d_scalar_fields(show_3d_scalar_fields);
			}

			bool show_scalar_coverages;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, show_scalar_coverages, geometry_visibility_tag("show_scalar_coverages")))
			{
				render_settings.set_show_scalar_coverages(show_scalar_coverages);
			}

			bool show_strings;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, show_strings, geometry_visibility_tag("show_strings")))
			{
				render_settings.set_show_strings(show_strings);
			}
		}


		void
		save_animation_configuration(
				const GPlatesScribe::ObjectTag &animation_configuration_tag,
				GPlatesScribe::Scribe &scribe,
				const GPlatesGui::AnimationController &animation_controller)
		{
			// Only save the animation start and end times.
			scribe.save(TRANSCRIBE_SOURCE, animation_controller.start_time(), animation_configuration_tag("start_time"));
			scribe.save(TRANSCRIBE_SOURCE, animation_controller.end_time(), animation_configuration_tag("end_time"));
		}

		void
		load_animation_configuration(
				const GPlatesScribe::ObjectTag &animation_configuration_tag,
				GPlatesScribe::Scribe &scribe,
				GPlatesGui::AnimationController &animation_controller)
		{
			//
			// Only transcribe the animation start and end times.
			//

			double start_time;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, start_time, animation_configuration_tag("start_time")))
			{
				animation_controller.set_start_time(start_time);
			}

			double end_time;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, end_time, animation_configuration_tag("end_time")))
			{
				animation_controller.set_end_time(end_time);
			}
		}


		void
		save_reconstruction_layer_geometry_parameters(
				const GPlatesScribe::ObjectTag &reconstruction_layer_geometry_parameters_tag,
				GPlatesScribe::Scribe &scribe,
				const GPlatesViewOperations::RenderedGeometryParameters &rendered_geometry_parameters)
		{
			scribe.save(
					TRANSCRIBE_SOURCE,
					rendered_geometry_parameters.get_reconstruction_layer_point_size_hint(),
					reconstruction_layer_geometry_parameters_tag("point_size_hint"));
			scribe.save(
					TRANSCRIBE_SOURCE,
					rendered_geometry_parameters.get_reconstruction_layer_line_width_hint(),
					reconstruction_layer_geometry_parameters_tag("line_width_hint"));
			scribe.save(
					TRANSCRIBE_SOURCE,
					rendered_geometry_parameters.get_reconstruction_layer_topology_size_multiplier(),
					reconstruction_layer_geometry_parameters_tag("topology_size_multiplier"));
			scribe.save(
					TRANSCRIBE_SOURCE,
					rendered_geometry_parameters.get_reconstruction_layer_ratio_arrow_unit_vector_direction_to_globe_radius(),
					reconstruction_layer_geometry_parameters_tag("ratio_arrow_unit_vector_direction_to_globe_radius"));
			scribe.save(
					TRANSCRIBE_SOURCE,
					rendered_geometry_parameters.get_reconstruction_layer_ratio_arrowhead_size_to_globe_radius(),
					reconstruction_layer_geometry_parameters_tag("ratio_arrowhead_size_to_globe_radius"));
			scribe.save(
					TRANSCRIBE_SOURCE,
					rendered_geometry_parameters.get_reconstruction_layer_arrow_spacing(),
					reconstruction_layer_geometry_parameters_tag("arrow_spacing"));
		}

		void
		load_reconstruction_layer_geometry_parameters(
				const GPlatesScribe::ObjectTag &reconstruction_layer_geometry_parameters_tag,
				GPlatesScribe::Scribe &scribe,
				GPlatesViewOperations::RenderedGeometryParameters &rendered_geometry_parameters)
		{
			float reconstruction_layer_point_size_hint;
			if (scribe.transcribe(
					TRANSCRIBE_SOURCE,
					reconstruction_layer_point_size_hint,
					reconstruction_layer_geometry_parameters_tag("point_size_hint")))
			{
				rendered_geometry_parameters.set_reconstruction_layer_point_size_hint(reconstruction_layer_point_size_hint);
			}

			float reconstruction_layer_line_width_hint;
			if (scribe.transcribe(
					TRANSCRIBE_SOURCE,
					reconstruction_layer_line_width_hint,
					reconstruction_layer_geometry_parameters_tag("line_width_hint")))
			{
				rendered_geometry_parameters.set_reconstruction_layer_line_width_hint(reconstruction_layer_line_width_hint);
			}

			float reconstruction_layer_topology_size_multiplier;
			if (scribe.transcribe(
					TRANSCRIBE_SOURCE,
					reconstruction_layer_topology_size_multiplier,
					reconstruction_layer_geometry_parameters_tag("topology_size_multiplier")))
			{
				rendered_geometry_parameters.set_reconstruction_layer_topology_size_multiplier(reconstruction_layer_topology_size_multiplier);
			}

			float reconstruction_layer_ratio_arrow_unit_vector_direction_to_globe_radius;
			if (scribe.transcribe(
					TRANSCRIBE_SOURCE,
					reconstruction_layer_ratio_arrow_unit_vector_direction_to_globe_radius,
					reconstruction_layer_geometry_parameters_tag("ratio_arrow_unit_vector_direction_to_globe_radius")))
			{
				rendered_geometry_parameters.set_reconstruction_layer_ratio_arrow_unit_vector_direction_to_globe_radius(
						reconstruction_layer_ratio_arrow_unit_vector_direction_to_globe_radius);
			}

			float reconstruction_layer_ratio_arrowhead_size_to_globe_radius;
			if (scribe.transcribe(
					TRANSCRIBE_SOURCE,
					reconstruction_layer_ratio_arrowhead_size_to_globe_radius,
					reconstruction_layer_geometry_parameters_tag("ratio_arrowhead_size_to_globe_radius")))
			{
				rendered_geometry_parameters.set_reconstruction_layer_ratio_arrowhead_size_to_globe_radius(
						reconstruction_layer_ratio_arrowhead_size_to_globe_radius);
			}

			float reconstruction_layer_arrow_spacing;
			if (scribe.transcribe(
					TRANSCRIBE_SOURCE,
					reconstruction_layer_arrow_spacing,
					reconstruction_layer_geometry_parameters_tag("arrow_spacing")))
			{
				rendered_geometry_parameters.set_reconstruction_layer_arrow_spacing(reconstruction_layer_arrow_spacing);
			}
		}


		void
		save_view_state(
				const GPlatesScribe::ObjectTag &session_state_tag,
				GPlatesScribe::Scribe &scribe,
				const ViewState &view_state)
		{
			////////////////////////////////////////////////////////////////////////////////////////
			// NOTE: Generally only transcribe state relevant to the individual layers.           //
			//       Global settings should usually be avoided in most cases.                     //
			////////////////////////////////////////////////////////////////////////////////////////

			// Save the background colour.
			scribe.save(TRANSCRIBE_SOURCE, view_state.get_background_colour(), session_state_tag("background_colour"));

			// Save the graticule settings.
			//
			// Some users set the alpha channel of graticule to zero to make them disappear so they can
			// load their own graticule lines (eg, from gpml file) that have plate ID zero and hence move
			// when the anchored plate ID is non-zero. And saving/loading graticule state means the
			// internal session or project file will set the alpha channel to zero for them.
			scribe.save(TRANSCRIBE_SOURCE, view_state.get_graticule_settings(), session_state_tag("graticule_settings"));

			//
			// Save the feature type symbol map (might be empty if no symbol file loaded).
			//
			const GPlatesGui::symbol_map_type &symbol_map = view_state.get_feature_type_symbol_map();
			scribe.save(TRANSCRIBE_SOURCE, symbol_map, session_state_tag("symbol_map"));

			// Geometry visibility settings are only transcribed because in future we should be
			// applying them individually to each layer (ie, each layer should have its own settings).
			save_geometry_visibility(session_state_tag("geometry_visibility"), scribe, view_state.get_render_settings());

			// Animation configuration is transcribed because rotation models have a time span and
			// so it makes sense that it should be preserved when the rotation model is loaded.
			save_animation_configuration(
					session_state_tag("animation_configuration"),
					scribe,
					view_state.get_animation_controller());

			// Reconstruction layer line/point sizes are only transcribed because in future we should be
			// applying them individually to each layer (ie, each layer should have its own symbology).
			save_reconstruction_layer_geometry_parameters(
					session_state_tag("reconstruction_layer_geometry_parameters"),
					scribe,
					view_state.get_rendered_geometry_parameters());
		}


		void
		load_view_state(
				const GPlatesScribe::ObjectTag &session_state_tag,
				GPlatesScribe::Scribe &scribe,
				ViewState &view_state)
		{
			////////////////////////////////////////////////////////////////////////////////////////
			// NOTE: Generally only transcribe state relevant to the individual layers.           //
			//       Global settings should usually be avoided in most cases.                     //
			////////////////////////////////////////////////////////////////////////////////////////

			// Load the background colour.
			//
			// Note that if there's no background colour to load (eg, loading from an old version project file)
			// then the default background colour at GPlates startup will be used (it has already been set since
			// the session state is always cleared to the default state just before loading a new session).
			GPlatesGui::Colour background_colour;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, background_colour, session_state_tag("background_colour")))
			{
				view_state.set_background_colour(background_colour);
			}

			// Load the graticule settings.
			//
			// Note that if there's no graticule settings to load (eg, loading from an old version project file)
			// then the default graticule settings at GPlates startup will be used (it has already been set since
			// the session state is always cleared to the default state just before loading a new session).
			GPlatesGui::GraticuleSettings graticule_settings;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, graticule_settings, session_state_tag("graticule_settings")))
			{
				view_state.get_graticule_settings() = graticule_settings;
			}

			//
			// Load the feature type symbol map (might be empty if no symbol file was loaded when the project/session was saved).
			//
			GPlatesGui::symbol_map_type symbol_map;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, symbol_map, session_state_tag("symbol_map")))
			{
				view_state.get_feature_type_symbol_map() = symbol_map;
			}

			// Animation configuration is transcribed because rotation models have a time span and
			// so it makes sense that it should be preserved when the rotation model is loaded.
			load_animation_configuration(
				session_state_tag("animation_configuration"),
				scribe,
				view_state.get_animation_controller());

			// Geometry visibility settings are only transcribed because in future we should be
			// applying them individually to each layer (ie, each layer should have its own settings).
			load_geometry_visibility(session_state_tag("geometry_visibility"), scribe, view_state.get_render_settings());

			// Reconstruction layer line/point sizes are only transcribed because in future we should be
			// applying them individually to each layer (ie, each layer should have its own symbology).
			load_reconstruction_layer_geometry_parameters(
					session_state_tag("reconstruction_layer_geometry_parameters"),
					scribe,
					view_state.get_rendered_geometry_parameters());
		}


		/**
		 * Save the session using the specified Scribe.
		 *
		 * The feature collection filenames are returned in @a feature_collection_filenames.
		 * Files with no filename are ignored (i.e. "New Feature Collection"s that only exist in memory).
		 */
		void
		save_session(
				const GPlatesScribe::ObjectTag &session_state_tag,
				GPlatesScribe::Scribe &scribe,
				const_file_reference_seq_type &file_references,
				QStringList &feature_collection_filenames)
		{
			const GPlatesAppLogic::ApplicationState &application_state = Application::instance().get_application_state();
			const ViewState &view_state = Application::instance().get_view_state();

			// Save the application state.
			save_application_state(session_state_tag, scribe, application_state);

			// Save the view state.
			save_view_state(session_state_tag, scribe, view_state);

			// Save the feature collection filenames.
			save_feature_collection_filenames(
					session_state_tag,
					scribe,
					file_references,
					feature_collection_filenames,
					application_state);

			// Save the layers.
			save_layers(session_state_tag, scribe, file_references, application_state, view_state);
		}


		/**
		 * Load the session using the specified Scribe.
		 *
		 * Returns a list of feature collection files that were not loaded
		 * (either they don't exist or the load failed).
		 */
 		void
		load_session(
				const GPlatesScribe::ObjectTag &session_state_tag,
				GPlatesScribe::Scribe &scribe,
				GPlatesFileIO::ReadErrorAccumulation &read_errors)
		{
			GPlatesAppLogic::ApplicationState &application_state = Application::instance().get_application_state();
			ViewState &view_state = Application::instance().get_view_state();

			// Block any signaled calls to 'ApplicationState::reconstruct' until we exit this scope.
			// Blocking calls to 'reconstruct' during this scope prevents multiple calls caused by
			// layer signals, etc, which is unnecessary if we're going to call 'reconstruct' anyway.
			GPlatesAppLogic::ApplicationState::ScopedReconstructGuard scoped_reconstruct_guard(
					application_state, true/*reconstruct_on_scope_exit*/);

			// Load the application state.
			load_application_state(session_state_tag, scribe, application_state);

			// Load the view state.
			//
			// Note: We load the view state before the layers since it's a bit faster.
			// If we loaded after the layers then each view state setting will likely signal
			// a redraw of all the layers.
			load_view_state(session_state_tag, scribe, view_state);

			// Load the feature collection files.
			QStringList feature_collection_filenames;
			load_feature_collection_filenames(session_state_tag, scribe, feature_collection_filenames);
			file_reference_on_load_seq_type file_references_on_load;
			load_feature_collection_files(feature_collection_filenames, file_references_on_load);

			// Load the layers.
			load_layers(session_state_tag, scribe, file_references_on_load, read_errors, application_state, view_state);
		}


		/**
		 * Unfortunately due to a mistake (in GPlates 1.5) we also need to save the deprecated
		 * session state required to support GPlates 1.5.
		 *
		 * The scribe system was introduced in GPlates 1.5 and the mistake was made to not ignore
		 * unknown (to GPlates 1.5) layer types and layer channel names.
		 * This meant forward compatibility was broken because GPlates 1.5 will report an unrecognised
		 * session state if it encounters an unknown layer type or layer channel name.
		 * It should have just ignored unknown layers and ignored connections to unknown channel names
		 * in which case it would have loaded most of the layers and their connections and not failed.
		 *
		 * So to allow GPlates 1.5 to load our (future) version of session state we need to
		 * isolate the session state it reads to a separate session state tag and make sure we don't
		 * save unknown (to GPlates 1.5) layer types and layer channel names. This is basically a
		 * compatible subset of the proper session state we write to a different session state tag.
		 */
 		void
		save_session_gplates_1_5(
				const GPlatesScribe::ObjectTag &session_state_tag,
				GPlatesScribe::Scribe &scribe,
				const const_file_reference_seq_type &file_references)
		{
			GPlatesAppLogic::ApplicationState &application_state = Application::instance().get_application_state();
			GPlatesAppLogic::ReconstructGraph &reconstruct_graph = application_state.get_reconstruct_graph();

			//
			// Transcribe the app-logic layers
			//

			const GPlatesScribe::ObjectTag layers_tag = session_state_tag("d_layers");

			// Get the layers.
			layer_seq_type layers(reconstruct_graph.begin(), reconstruct_graph.end());

			// Remove any layers that are unknown by GPlates 1.5 so that we don't save them and
			// we don't connect other layers to them.
			for (layer_seq_type::iterator layers_iter = layers.begin(); layers_iter != layers.end(); )
			{
				const GPlatesAppLogic::LayerTaskType::Type layer_type = layers_iter->get_type();

				// These are the layer types known by GPlates 1.5.
				if (layer_type != GPlatesAppLogic::LayerTaskType::RECONSTRUCTION &&
					layer_type != GPlatesAppLogic::LayerTaskType::RECONSTRUCT &&
					layer_type != GPlatesAppLogic::LayerTaskType::RASTER &&
					layer_type != GPlatesAppLogic::LayerTaskType::SCALAR_FIELD_3D &&
					layer_type != GPlatesAppLogic::LayerTaskType::TOPOLOGY_GEOMETRY_RESOLVER &&
					layer_type != GPlatesAppLogic::LayerTaskType::TOPOLOGY_NETWORK_RESOLVER &&
					layer_type != GPlatesAppLogic::LayerTaskType::VELOCITY_FIELD_CALCULATOR &&
					layer_type != GPlatesAppLogic::LayerTaskType::CO_REGISTRATION)
				{
					layers_iter = layers.erase(layers_iter);
					continue;
				}

				++layers_iter;
			}

			// Save the transcribe information from the layers and their connections.
			for (unsigned int layer_index = 0; layer_index < layers.size(); ++layer_index)
			{
				const GPlatesAppLogic::Layer &layer = layers[layer_index];

				const GPlatesScribe::ObjectTag layer_tag = layers_tag[layer_index];

				scribe.save(TRANSCRIBE_SOURCE, layer.get_type(), layer_tag("d_layer_task_type"));
				scribe.save(TRANSCRIBE_SOURCE, layer.is_active(), layer_tag("d_is_active"));
				scribe.save(TRANSCRIBE_SOURCE, layer.get_auto_created(), layer_tag("d_is_auto_created"));

				std::vector<GPlatesAppLogic::Layer::InputConnection> input_connections = layer.get_all_inputs();

				// Remove any layer connections that are unknown by GPlates 1.5 so that we don't save them.
				for (std::vector<GPlatesAppLogic::Layer::InputConnection>::iterator input_connections_iter = input_connections.begin();
					input_connections_iter != input_connections.end();
					)
				{
					const GPlatesAppLogic::LayerInputChannelName::Type input_channel_name = input_connections_iter->get_input_channel_name();

					// These are the layer channel names known by GPlates 1.5.
					if (input_channel_name != GPlatesAppLogic::LayerInputChannelName::RECONSTRUCTION_FEATURES &&
						input_channel_name != GPlatesAppLogic::LayerInputChannelName::RECONSTRUCTABLE_FEATURES &&
						input_channel_name != GPlatesAppLogic::LayerInputChannelName::TOPOLOGICAL_GEOMETRY_FEATURES &&
						input_channel_name != GPlatesAppLogic::LayerInputChannelName::TOPOLOGICAL_NETWORK_FEATURES &&
						input_channel_name != GPlatesAppLogic::LayerInputChannelName::RASTER_FEATURE &&
						input_channel_name != GPlatesAppLogic::LayerInputChannelName::SCALAR_FIELD_FEATURE &&
						input_channel_name != GPlatesAppLogic::LayerInputChannelName::RECONSTRUCTION_TREE &&
						input_channel_name != GPlatesAppLogic::LayerInputChannelName::TOPOLOGY_SURFACES &&
						input_channel_name != GPlatesAppLogic::LayerInputChannelName::TOPOLOGICAL_SECTION_LAYERS &&
						input_channel_name != GPlatesAppLogic::LayerInputChannelName::VELOCITY_DOMAIN_LAYERS &&
						input_channel_name != GPlatesAppLogic::LayerInputChannelName::VELOCITY_SURFACE_LAYERS &&
						input_channel_name != GPlatesAppLogic::LayerInputChannelName::RECONSTRUCTED_POLYGONS &&
						input_channel_name != GPlatesAppLogic::LayerInputChannelName::AGE_GRID_RASTER &&
						input_channel_name != GPlatesAppLogic::LayerInputChannelName::NORMAL_MAP_RASTER &&
						input_channel_name != GPlatesAppLogic::LayerInputChannelName::CROSS_SECTIONS &&
						input_channel_name != GPlatesAppLogic::LayerInputChannelName::SURFACE_POLYGONS_MASK &&
						input_channel_name != GPlatesAppLogic::LayerInputChannelName::CO_REGISTRATION_SEED_GEOMETRIES &&
						input_channel_name != GPlatesAppLogic::LayerInputChannelName::CO_REGISTRATION_TARGET_GEOMETRIES)
					{
						input_connections_iter = input_connections.erase(input_connections_iter);
						continue;
					}

					++input_connections_iter;
				}

				const GPlatesScribe::ObjectTag connections_tag = layer_tag("d_input_connections");

				// Iterate over the layer's input connections.
				for (unsigned int connection_index = 0; connection_index < input_connections.size(); ++connection_index)
				{
					const GPlatesScribe::ObjectTag connection_tag = connections_tag[connection_index];

					const GPlatesAppLogic::Layer::InputConnection &input_connection = input_connections[connection_index];
					const GPlatesAppLogic::LayerInputChannelName::Type input_channel_name = input_connection.get_input_channel_name();
					boost::optional<GPlatesAppLogic::Layer::InputFile> input_file = input_connection.get_input_file();
					if (input_file)
					{
						// Find the input file in our list of loaded file references.
						const_file_reference_seq_type::const_iterator file_iter = std::find(
								file_references.begin(),
								file_references.end(),
								input_file->get_file());
						if (file_iter != file_references.end())
						{
							// Make sure doesn't reference an empty filename.
							const GPlatesFileIO::FileInfo &file_info = file_iter->get_file().get_file_info();
							if (!file_info.get_qfileinfo().absoluteFilePath().isEmpty())
							{
								const unsigned int file_index = file_iter - file_references.begin();

								scribe.save(TRANSCRIBE_SOURCE, input_channel_name, connection_tag("d_input_channel_name"));
								scribe.save(TRANSCRIBE_SOURCE, file_index, connection_tag("d_input_index"));
								scribe.save(TRANSCRIBE_SOURCE, true/*is_input_file*/, connection_tag("d_is_input_file"));
							}
						}
					}
					else
					{
						// The input is not a file so it must be a layer.
						GPlatesAppLogic::Layer input_layer = input_connection.get_input_layer().get();

						// Find the input layer in our list of layers.
						layer_seq_type::const_iterator layer_iter = std::find(layers.begin(), layers.end(), input_layer);
						if (layer_iter != layers.end())
						{
							const unsigned int input_layer_index = layer_iter - layers.begin();

							scribe.save(TRANSCRIBE_SOURCE, input_channel_name, connection_tag("d_input_channel_name"));
							scribe.save(TRANSCRIBE_SOURCE, input_layer_index, connection_tag("d_input_index"));
							scribe.save(TRANSCRIBE_SOURCE, false/*is_input_file*/, connection_tag("d_is_input_file"));
						}
					}
				}

				// Save number of connections.
				scribe.save(TRANSCRIBE_SOURCE, input_connections.size(), connections_tag.sequence_size());
			}

			// Save number of layers.
			scribe.save(TRANSCRIBE_SOURCE, layers.size(), layers_tag.sequence_size());

			//
			// Transcribe the default reconstruction tree layer
			//

			boost::optional<unsigned int> default_reconstruction_tree_layer_index;

			GPlatesAppLogic::Layer default_reconstruction_tree_layer = reconstruct_graph.get_default_reconstruction_tree_layer();
			if (default_reconstruction_tree_layer.is_valid())
			{
				// Find the default reconstruction tree layer in our list of layers.
				layer_seq_type::const_iterator layer_iter =
						std::find(layers.begin(), layers.end(), default_reconstruction_tree_layer);
				if (layer_iter != layers.end())
				{
					const unsigned int layer_index = layer_iter - layers.begin();
					default_reconstruction_tree_layer_index = layer_index;
				}
			}

			scribe.save(
					TRANSCRIBE_SOURCE,
					default_reconstruction_tree_layer_index,
					session_state_tag("d_default_reconstruction_tree_layer_index"));
		}


 		void
		load_session_gplates_1_5(
				const GPlatesScribe::ObjectTag &session_state_tag,
				GPlatesScribe::Scribe &scribe,
				const QStringList &feature_collection_filenames,
				GPlatesFileIO::ReadErrorAccumulation &read_errors)
		{
			GPlatesAppLogic::ApplicationState &application_state = Application::instance().get_application_state();
			ViewState &view_state = Application::instance().get_view_state();

			// Block any signaled calls to 'ApplicationState::reconstruct' until we exit this scope.
			// Blocking calls to 'reconstruct' during this scope prevents multiple calls caused by
			// layer signals, etc, which is unnecessary if we're going to call 'reconstruct' anyway.
			GPlatesAppLogic::ApplicationState::ScopedReconstructGuard scoped_reconstruct_guard(
					application_state, true/*reconstruct_on_scope_exit*/);

			// Load the feature collection files.
			file_reference_on_load_seq_type file_references_on_load;
			load_feature_collection_files(feature_collection_filenames, file_references_on_load);

			// Load the layers.
			load_layers(session_state_tag, scribe, file_references_on_load, read_errors, application_state, view_state);
		}
	}
}


QStringList
GPlatesPresentation::TranscribeSession::save(
		GPlatesScribe::Scribe &scribe,
		boost::optional<GPlatesScribe::Scribe &> scribe_gplates_1_5)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			scribe.is_saving(),
			GPLATES_ASSERTION_SOURCE);

	// Save the session state.
	//
	// Also returns the loaded feature collection filenames/files.
	// Files with no filename are ignored (i.e. "New Feature Collection"s that only exist in memory).
	const_file_reference_seq_type file_references;
	QStringList feature_collection_filenames;
	save_session("session_state", scribe, file_references, feature_collection_filenames);

	// Unfortunately due to a mistake (in GPlates 1.5) we also need to save the deprecated
	// session state required to support GPlates 1.5.
	//
	// The scribe system was introduced in GPlates 1.5 and the mistake was made to not ignore
	// unknown (to GPlates 1.5) layer types and layer channel names.
	// This meant forward compatibility was broken because GPlates 1.5 will report an unrecognised
	// session state if it encounters an unknown layer type or layer channel name.
	// It should have just ignored unknown layers and ignored connections to unknown channel names
	// in which case it would have loaded most of the layers and their connections and not failed.
	//
	// So to allow GPlates 1.5 to load our (future) version of session state we need to
	// isolate the session state that it reads (ie, the tag "session_state_version4") and make
	// sure we don't save unknown (to GPlates 1.5) layer types and layer channel names.
	// This is basically a compatible subset of the proper session state we wrote to tag "session_state".
 	save_session_gplates_1_5(
			"session_state_version4",
			// If 'scribe_gplates_1_5' not specified then use 'scribe' (ie, save to same transcription)...
			scribe_gplates_1_5 ? scribe_gplates_1_5.get() : scribe,
			file_references);

	return feature_collection_filenames;
}


void
GPlatesPresentation::TranscribeSession::load(
		GPlatesScribe::Scribe &scribe,
		const QStringList &feature_collection_filenames)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			scribe.is_loading(),
			GPLATES_ASSERTION_SOURCE);

	GPlatesFileIO::ReadErrorAccumulation read_errors;

	// Unfortunately due to a mistake (in GPlates 1.5) we need to detect whether the session state
	// is in the usual "session_state" tag or a special deprecated "session_state_version4" tag
	// that supports GPlates 1.5.
	//
	// GPlates 1.5 only writes to "session_state_version4". But versions after that write to
	// "session_state", while also writing to "session_state_version4" to support GPlates 1.5.
	//
	// So if "session_state" is present we'll use that (since it will contain more up-to-date session state)
	// otherwise we default to "session_state_version4" (ie, we'll be reading a session saved by GPlates 1.5).
	if (scribe.is_in_transcription("session_state"))
	{
		// Load the session.
 		load_session("session_state", scribe, read_errors);
	}
	else if (scribe.is_in_transcription("session_state_version4"))
	{
		// Load the GPlates 1.5 session state.
 		load_session_gplates_1_5("session_state_version4", scribe, feature_collection_filenames, read_errors);
	}
	else
	{
		// The transcription is incompatible.
		GPlatesGlobal::Assert<TranscribeSession::UnsupportedVersion>(
				false,
				GPLATES_ASSERTION_SOURCE,
				scribe.get_transcribe_incompatible_call_stack());
	}

	// Show file read errors (if any).
	//
	// Note that the feature collection read errors have already been handled by
	// FeatureCollectionFileIO, so this handles other read errors such as CPT files.
	if (read_errors.size() > 0)
	{
		Application::instance().get_main_window().handle_read_errors(read_errors);
	}
}


void
GPlatesPresentation::TranscribeSession::UnsupportedVersion::write_message(
		std::ostream &os) const
{
	os << "Attempted to load a session created from a version of GPlates that is either too old or too new."
		<< std::endl;
	
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
