/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2011 The University of Sydney, Australia
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


#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <QDebug>
#include <QDomDocument>
#include <QFileInfo>
#include <QMap>
#include <QString>

#include "DeprecatedSessionRestore.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/ReconstructGraph.h"
#include "app-logic/Layer.h"
#include "app-logic/LayerInputChannelName.h"
#include "app-logic/LayerTask.h"
#include "app-logic/LayerTaskType.h"
#include "app-logic/LayerTaskRegistry.h"

#include "file-io/FileInfo.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


namespace
{
	typedef QMap<QString, GPlatesAppLogic::Layer> IdLayerMap;

	typedef QMap<QString, GPlatesAppLogic::LayerTaskType::Type> IdLayerTaskTypeMap;


	/**
	 * Returns the layer task type id.
	 */
	const IdLayerTaskTypeMap &
	get_id_layer_task_type_map(
			int session_version)
	{
		// Prior to version 3 the layer task type was an integer directly mapped to the
		// layer task type enumeration. This proved a bit error-prone when new enumerations were
		// added so later versions convert the enumerations to strings.
		if (session_version < 3)
		{
			static bool initialised_map = false;
			static IdLayerTaskTypeMap ID_LAYER_TASK_TYPE_MAP;
			if (!initialised_map)
			{
				ID_LAYER_TASK_TYPE_MAP["0"] = GPlatesAppLogic::LayerTaskType::RECONSTRUCTION;
				ID_LAYER_TASK_TYPE_MAP["1"] = GPlatesAppLogic::LayerTaskType::RECONSTRUCT;
				ID_LAYER_TASK_TYPE_MAP["2"] = GPlatesAppLogic::LayerTaskType::RASTER;
				ID_LAYER_TASK_TYPE_MAP["3"] = GPlatesAppLogic::LayerTaskType::TOPOLOGY_GEOMETRY_RESOLVER;
				ID_LAYER_TASK_TYPE_MAP["4"] = GPlatesAppLogic::LayerTaskType::TOPOLOGY_NETWORK_RESOLVER;
				ID_LAYER_TASK_TYPE_MAP["5"] = GPlatesAppLogic::LayerTaskType::VELOCITY_FIELD_CALCULATOR;
				ID_LAYER_TASK_TYPE_MAP["6"] = GPlatesAppLogic::LayerTaskType::CO_REGISTRATION;

				initialised_map = true;
			}

			return ID_LAYER_TASK_TYPE_MAP;
		}

		static bool initialised_map = false;
		static IdLayerTaskTypeMap ID_LAYER_TASK_TYPE_MAP;
		if (!initialised_map)
		{
			ID_LAYER_TASK_TYPE_MAP["Reconstruction"] = GPlatesAppLogic::LayerTaskType::RECONSTRUCTION;
			ID_LAYER_TASK_TYPE_MAP["Reconstruct"] = GPlatesAppLogic::LayerTaskType::RECONSTRUCT;
			ID_LAYER_TASK_TYPE_MAP["Raster"] = GPlatesAppLogic::LayerTaskType::RASTER;
			ID_LAYER_TASK_TYPE_MAP["ScalarField3D"] = GPlatesAppLogic::LayerTaskType::SCALAR_FIELD_3D;
			ID_LAYER_TASK_TYPE_MAP["TopologyGeometryResolver"] = GPlatesAppLogic::LayerTaskType::TOPOLOGY_GEOMETRY_RESOLVER;
			ID_LAYER_TASK_TYPE_MAP["TopologyNetworkResolver"] = GPlatesAppLogic::LayerTaskType::TOPOLOGY_NETWORK_RESOLVER;
			ID_LAYER_TASK_TYPE_MAP["VelocityFieldCalculator"] = GPlatesAppLogic::LayerTaskType::VELOCITY_FIELD_CALCULATOR;
			ID_LAYER_TASK_TYPE_MAP["CoRegistration"] = GPlatesAppLogic::LayerTaskType::CO_REGISTRATION;

			// For the latest session version we check to make sure all the layer task type enumerations
			// have been mapped - this helps detect situations where an enumeration is added or removed.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					ID_LAYER_TASK_TYPE_MAP.size() == GPlatesAppLogic::LayerTaskType::NUM_BUILT_IN_TYPES,
					GPLATES_ASSERTION_SOURCE);

			initialised_map = true;
		}

		return ID_LAYER_TASK_TYPE_MAP;
	}


	/**
	 * Returns the layer task type id.
	 */
	boost::optional<GPlatesAppLogic::LayerTaskType::Type>
	load_layer_task_type(
			QDomElement el,
			int session_version)
	{
		// Get the id-to-layer-task-type map depending on the session version.
		const IdLayerTaskTypeMap &id_layer_task_type_map = get_id_layer_task_type_map(session_version);

		const QString id_layer_task_type = el.attribute("type");
		IdLayerTaskTypeMap::const_iterator id_layer_task_type_iter =
				id_layer_task_type_map.find(id_layer_task_type);
		if (id_layer_task_type_iter == id_layer_task_type_map.end())
		{
			return boost::none;
		}

		return id_layer_task_type_iter.value();
	}


	GPlatesAppLogic::LayerTaskRegistry::LayerTaskType
	get_layer_task_type(
			GPlatesAppLogic::LayerTaskRegistry &ltr,
			GPlatesAppLogic::LayerTaskType::Type layer_type)
	{
		std::vector<GPlatesAppLogic::LayerTaskRegistry::LayerTaskType> layer_task_types = ltr.get_all_layer_task_types();
		BOOST_FOREACH(GPlatesAppLogic::LayerTaskRegistry::LayerTaskType ltt, layer_task_types) {
			if (ltt.get_layer_type() == layer_type) {
				return ltt;
			}
		}
		return GPlatesAppLogic::LayerTaskRegistry::LayerTaskType();	// An invalid LayerTaskType.
	}


	/**
	 * Load a Layer into the ReconstructGraph from a QDomElement.
	 * Also inserts ID into the idmap.
	 */
	GPlatesAppLogic::Layer
	load_layer(
			GPlatesAppLogic::LayerTaskRegistry &ltr,
			GPlatesAppLogic::ReconstructGraph &rg,
			QDomElement el,
			IdLayerMap &idmap,
			int session_version)
	{
		// Before we can create a Layer, we must first know the LayerTaskType.
		boost::optional<GPlatesAppLogic::LayerTaskType::Type> layer_task_type_type =
				load_layer_task_type(el, session_version);
		if (!layer_task_type_type)
		{
			return GPlatesAppLogic::Layer();
		}

		GPlatesAppLogic::LayerTaskRegistry::LayerTaskType layer_task_type =
				get_layer_task_type(ltr, layer_task_type_type.get());
		if (!layer_task_type.is_valid())
		{
			return GPlatesAppLogic::Layer();
		}

		const int is_active = el.attribute("is_active").toInt();
		const int auto_created = el.attribute("auto_created").toInt();

		// Before we can create a Layer, we must first create a LayerTask.
		boost::shared_ptr<GPlatesAppLogic::LayerTask> lt_ptr = layer_task_type.create_layer_task();

		// Finally, can we create a Layer?
		GPlatesAppLogic::Layer layer = rg.add_layer(lt_ptr);
		layer.activate( is_active == 1 ? true : false );
		// Was the layer originally auto-created ?
		// This is needed so the layer can be auto-destroyed if the input file on its
		// main input channel is later unloaded by the user.
		layer.set_auto_created( auto_created == 1 ? true : false );

		// Store ID for this layer.
		idmap.insert(el.attribute("id"), layer);

		return layer;
	}


	/**
	 * A bit hackish, probably better to use an *IdMap style system as we do for the Layers,
	 * But for now file path as ID should work fine and is easier.
	 */
	GPlatesAppLogic::Layer::InputFile
	get_input_file_by_id(
			GPlatesAppLogic::FeatureCollectionFileState &fs,
			GPlatesAppLogic::ReconstructGraph &rg,
			const QString &id)
	{
		const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_files = fs.get_loaded_files();
		BOOST_FOREACH(const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_ref, loaded_files) {
			if (file_ref.get_file().get_file_info().get_qfileinfo().absoluteFilePath() == id) {
				return rg.get_input_file(file_ref);
			}
		}
		return GPlatesAppLogic::Layer::InputFile(); // None found - return an invalid InputFile.
	}


	/**
	 * Layer input channel names are now enumerations (not strings).
	 *
	 * This function converts the deprecated string input channel names to enumeration values.
	 */
	boost::optional<GPlatesAppLogic::LayerInputChannelName::Type>
	get_layer_input_channel_name(
			const QString &layer_input_channel_name)
	{
		if (layer_input_channel_name == "Reconstruction features")
		{
			return GPlatesAppLogic::LayerInputChannelName::RECONSTRUCTION_FEATURES;
		}
		else if (layer_input_channel_name == "Reconstruction tree")
		{
			return GPlatesAppLogic::LayerInputChannelName::RECONSTRUCTION_TREE;
		}
		else if (layer_input_channel_name == "Reconstructable features")
		{
			return GPlatesAppLogic::LayerInputChannelName::RECONSTRUCTABLE_FEATURES;
		}
		else if (layer_input_channel_name == "Deformation surfaces (topological networks)")
		{
			return GPlatesAppLogic::LayerInputChannelName::DEFORMATION_SURFACES;
		}
		else if (layer_input_channel_name == "Topological geometry features")
		{
			return GPlatesAppLogic::LayerInputChannelName::TOPOLOGICAL_GEOMETRY_FEATURES;
		}
		else if (layer_input_channel_name == "Topological sections")
		{
			return GPlatesAppLogic::LayerInputChannelName::TOPOLOGICAL_SECTION_LAYERS;
		}
		else if (layer_input_channel_name == "Topological network features")
		{
			return GPlatesAppLogic::LayerInputChannelName::TOPOLOGICAL_NETWORK_FEATURES;
		}
		else if (layer_input_channel_name == "Velocity domains (points/multi-points/polylines/polygons)")
		{
			return GPlatesAppLogic::LayerInputChannelName::VELOCITY_DOMAIN_LAYERS;
		}
		else if (layer_input_channel_name == "Velocity surfaces (static/dynamic polygons/networks)")
		{
			return GPlatesAppLogic::LayerInputChannelName::VELOCITY_SURFACE_LAYERS;
		}
		else if (layer_input_channel_name == "Raster feature")
		{
			return GPlatesAppLogic::LayerInputChannelName::RASTER_FEATURE;
		}
		else if (layer_input_channel_name == "Reconstructed polygons")
		{
			return GPlatesAppLogic::LayerInputChannelName::RECONSTRUCTED_POLYGONS;
		}
		else if (layer_input_channel_name == "Age grid raster")
		{
			return GPlatesAppLogic::LayerInputChannelName::AGE_GRID_RASTER;
		}
		else if (layer_input_channel_name == "Surface relief raster")
		{
			return GPlatesAppLogic::LayerInputChannelName::NORMAL_MAP_RASTER;
		}
		else if (layer_input_channel_name == "Scalar field feature")
		{
			return GPlatesAppLogic::LayerInputChannelName::SCALAR_FIELD_FEATURE;
		}
		else if (layer_input_channel_name == "Cross sections")
		{
			return GPlatesAppLogic::LayerInputChannelName::CROSS_SECTIONS;
		}
		else if (layer_input_channel_name == "Surface polygons mask")
		{
			return GPlatesAppLogic::LayerInputChannelName::SURFACE_POLYGONS_MASK;
		}
		else if (layer_input_channel_name == "Reconstructed seed geometries")
		{
			return GPlatesAppLogic::LayerInputChannelName::CO_REGISTRATION_SEED_GEOMETRIES;
		}
		else if (layer_input_channel_name == "Reconstructed target geometries/rasters")
		{
			return GPlatesAppLogic::LayerInputChannelName::CO_REGISTRATION_TARGET_GEOMETRIES;
		}

		return boost::none;
	}


	/**
	 * Load a Layer::InputConnection into the ReconstructGraph from a QDomElement.
	 */
	GPlatesAppLogic::Layer::InputConnection
	load_layer_connection(
			GPlatesAppLogic::FeatureCollectionFileState &fs,
			GPlatesAppLogic::LayerTaskRegistry &ltr,
			GPlatesAppLogic::ReconstructGraph &rg,
			QDomElement el,
			IdLayerMap &idmap,
			int session_version)
	{
		// What layer are we going to connect things to?
		GPlatesAppLogic::Layer to_layer = idmap.value(el.attribute("to"));
		if ( ! to_layer.is_valid()) {
			// Fail, destination Layer is not valid.
			return GPlatesAppLogic::Layer::InputConnection();	// an invalid InputConnection.
		}

		// Before we can create a InputConnection, we must first know what type of connection to make.
		QString deprecated_input_channel = el.attribute("input_channel_name");

		// Handle deprecated connections from old session versions.
		if (session_version < 2)
		{
			// Version 1 added a connection for topological boundary sections in topology layers.
			// Version 2 then deprecated this connection and so versions 2 and above can simply
			// ignore the connection without loss of functionality.
			if (to_layer.get_type() == GPlatesAppLogic::LayerTaskType::TOPOLOGY_GEOMETRY_RESOLVER)
			{
				// Note that the following string literal is deprecated and so this is now
				// the only instance of it in the GPlates source code.
				if (deprecated_input_channel == "Topological boundary section features")
				{
					return GPlatesAppLogic::Layer::InputConnection();	// a deprecated InputConnection.
				}
			}
			if (to_layer.get_type() == GPlatesAppLogic::LayerTaskType::TOPOLOGY_NETWORK_RESOLVER)
			{
				// Note that the following string literal is deprecated and so this is now
				// the only instance of it in the GPlates source code.
				if (deprecated_input_channel == "Topological section features")
				{
					return GPlatesAppLogic::Layer::InputConnection();	// a deprecated InputConnection.
				}
			}
		}

		boost::optional<GPlatesAppLogic::LayerInputChannelName::Type> input_channel =
				get_layer_input_channel_name(deprecated_input_channel);
		if (!input_channel)
		{
			return GPlatesAppLogic::Layer::InputConnection();	// an invalid InputConnection.
		}

		if (el.attribute("type") == "InputFile") {
			// What file are we going to take the data from?
			GPlatesAppLogic::Layer::InputFile from_file = get_input_file_by_id(fs, rg, el.attribute("from"));
			if ( ! from_file.is_valid()) {
				// Fail, source InputFile is not valid.
				return GPlatesAppLogic::Layer::InputConnection();	// an invalid InputConnection.
			}
			GPlatesAppLogic::Layer::InputConnection ic =
					to_layer.connect_input_to_file(from_file, input_channel.get());
			return ic;

		} else if (el.attribute("type") == "Layer") {
			// What layer are we going to take the data from?
			GPlatesAppLogic::Layer from_layer = idmap.value(el.attribute("from"));
			if ( ! from_layer.is_valid()) {
				// Fail, source Layer is not valid.
				return GPlatesAppLogic::Layer::InputConnection();	// an invalid InputConnection.
			}
			GPlatesAppLogic::Layer::InputConnection ic =
					to_layer.connect_input_to_layer_output(from_layer, input_channel.get());
			return ic;

		} else {
			// FIXME: Is supposed to be one or the other. Should maybe throw an exception here,...
			return GPlatesAppLogic::Layer::InputConnection();	// an invalid InputConnection.
		}
	}


	/**
	 * Convert xml-domified layers state to actual connections in the ReconstructGraph.
	 */
	void
	load_layers_state(
			const QDomDocument &dom,
			int session_version,
			GPlatesAppLogic::ApplicationState &app_state)
	{
		// We should already have Impl::Data objects loaded due to the way we suppressed the auto-layer-creation code.
		// So we'll have the InputFile objects available. We *could* load those separately later, but I'm happy enough
		// to assume that the InputFiles match the actual loaded feature collections. Our current means of identifying
		// an InputFile connection is from absolute file path, so we don't need to actually load the InputFile state
		// from the LayersStateType, not for now anyway.

		// We need the ReconstructGraph to reset the logical state of the graph.
		GPlatesAppLogic::ReconstructGraph &rg = app_state.get_reconstruct_graph();
		// And the LayerTaskRegistry before we can create Layers.
		GPlatesAppLogic::LayerTaskRegistry &ltr = app_state.get_layer_task_registry();
		// We also need a means of tracking IDs for layers. I think I'd prefer to put this in the (save_layers_state()) abovementioned
		// specialist subclass of the DOM, to keep all of that in one class that can help us serialise things,
		// but for now stick with this ID map and a bunch of anon namespace fns.
		IdLayerMap idmap;

		// Put all layer additions in a single add layers group.
		GPlatesAppLogic::ReconstructGraph::AddOrRemoveLayersGroup add_layers_group(rg);
		add_layers_group.begin_add_or_remove_layers();

		// Keep track of the loaded layers so we can later remove those that reference files
		// that we unable to be loaded/restored (because they have been moved or are missing).
		std::vector<GPlatesAppLogic::Layer> loaded_layers;

		// First we need to re-instate the Layers that are specified in the LayersStateType though.
		QDomElement el_root = dom.firstChildElement("LayersState");
		QDomElement el_layers = el_root.firstChildElement("Layers");
		for (QDomElement el_layer = el_layers.firstChildElement("Layer");
			  ! el_layer.isNull();
			  el_layer = el_layer.nextSiblingElement("Layer"))
		{
			const GPlatesAppLogic::Layer layer = load_layer(ltr, rg, el_layer, idmap, session_version);
			loaded_layers.push_back(layer);
		}

		// Once that's done, we can reference Layers by ID. One such relationship we need to load is
		// the "Default Reconstruction Tree" layer, if there is one.
		QDomElement el_default_recon = el_root.firstChildElement("DefaultReconstructionTree");
		if ( ! el_default_recon.isNull() && el_default_recon.hasAttribute("layer")) {
			GPlatesAppLogic::Layer default_recon_layer = idmap.value(el_default_recon.attribute("layer"));
			if (default_recon_layer.is_valid()) {
				rg.set_default_reconstruction_tree_layer(default_recon_layer);
			}
		}

		// Then we need to reconnect Layers.
		QDomElement el_connections = el_root.firstChildElement("Connections");
		for (QDomElement el_con = el_connections.firstChildElement("InputConnection");
			  ! el_con.isNull();
			  el_con = el_con.nextSiblingElement("InputConnection")) {
			// Only attempt to load <InputConnection>s that don't look broken (with an empty "to" or "from" attribute)
			if ( ! el_con.attribute("from").isEmpty() && ! el_con.attribute("to").isEmpty()) {
				load_layer_connection(
						app_state.get_feature_collection_file_state(), ltr, rg, el_con, idmap, session_version);
			}
		}

		// Remove any loaded layers that reference files, on the main input channel, that don't exist.
		// This can happen when files have been moved or deleted since the session was saved.
		//
		// NOTE: We *only* do this for the *deprecated* session restore since we know all layer types,
		// at the time of deprecation, should have something connected to their main input connection
		// in order to be operable. The one exception to this is co-registration layers.
		BOOST_FOREACH(const GPlatesAppLogic::Layer &layer, loaded_layers)
		{
			// Never remove a co-registration layer - it does not use the *main* input connection.
			if (layer.get_type() == GPlatesAppLogic::LayerTaskType::CO_REGISTRATION)
			{
				continue;
			}

			if (layer.get_channel_inputs(layer.get_main_input_feature_collection_channel()).empty())
			{
				rg.remove_layer(layer);
			}
		}

		// End the add layers group.
		add_layers_group.end_add_or_remove_layers();

		// Aaaand we're done.
	}


	/**
	 * Enable RAII style 'lock' on temporarily disabling automatic layer creation
	 * within app-state for as long as the current scope holds onto this object.
	 */
	class SuppressAutoLayerCreationRAII :
			private boost::noncopyable
	{
	public:
		SuppressAutoLayerCreationRAII(
				GPlatesAppLogic::ApplicationState &_app_state):
			d_app_state_ptr(&_app_state)
		{
			// Suppress auto-creation of layers because we have session information regarding which
			// layers should be created and what their connections should be.
			d_app_state_ptr->suppress_auto_layer_creation(true);
		}
		
		~SuppressAutoLayerCreationRAII()
		{
			d_app_state_ptr->suppress_auto_layer_creation(false);
		}
		
		GPlatesAppLogic::ApplicationState *d_app_state_ptr;
	};
	
	
	/**
	 * Since attempting to load some files which do not exist (amongst a list of otherwise-okay files)
	 * will currently fail part-way through with an exception, we apply this function to remove any
	 * such problematic files from a Session's file-list prior to asking FeatureCollectionFileIO to load
	 * them.
	 *
	 * FIXME:
	 * Ideally, this modification of the file list would not be done, and the file-io layer would have
	 * a nice means of triggering a GUI action to open a dialog listing the problem files and ask the
	 * user if they would like to:-
	 *    a) Skip over the problem files, load the others
	 *    b) Try again, I've fixed it now
	 *    c) Abort the entire file-loading endeavour
	 * Of course, this requires quite a bit of structural enhancements to the code to allow file-io to
	 * signal the gui level (and go back again) cleanly. So as a cheaper bugfix, I'm just stripping out
	 * the bad filenames. The only problem is, the Layers state will still get loaded as though such
	 * a file exists and I'm not entirely sure if that'll work.
	 */
	QSet<QString>
	strip_bad_filenames(
			QSet<QString> filenames)
	{
		QSet<QString> good_filenames;
		Q_FOREACH(QString filename, filenames) {
			if (QFile::exists(filename)) {
				good_filenames.insert(filename);
			}
		}
		return good_filenames;
	}
}


QStringList
GPlatesPresentation::DeprecatedSessionRestore::restore_session(
		int version,
		const QDateTime &time,
		const QStringList &loaded_files,
		const QString &layers_state,
		GPlatesAppLogic::ApplicationState &app_state)
{
	GPlatesAppLogic::FeatureCollectionFileIO &file_io = app_state.get_feature_collection_file_io();

	// Loading session depends on the version...
	switch (version)
	{
	case 0:
		// Layers state not saved in this version so allow application state to auto-create layers.
		// The layers won't be connected though, but when the session is saved they will be because
		// the session will be saved with the latest version.
		file_io.load_files(
				QStringList::fromSet(
						strip_bad_filenames(
								QSet<QString>::fromList(loaded_files))));
		break;

	case 1:
	case 2:
	case 3:
		{
			// Suppress auto-creation of layers during this scope because we have session information
			// regarding which layers should be created and what their connections should be.
			// Needs to be RAII in case load_files() throws an exception which it totally will do as
			// soon as you take your eyes off it.
			SuppressAutoLayerCreationRAII raii(app_state);

			file_io.load_files(
					QStringList::fromSet(
							strip_bad_filenames(
									QSet<QString>::fromList(loaded_files))));

			// New in version 1 is save/restore of layer type and connections.
			QDomDocument layers_state_dom;
			layers_state_dom.setContent(layers_state);
			load_layers_state(layers_state_dom, version, app_state);
		}
		break;

	default:
		// Shouldn't get here.
		// Versions 4 and above should be handled by the general scribe system used by session management.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}

	// Return the files that were *not* loaded.
	return QStringList::fromSet(
			// Remove items from 'loaded_files' that are also in 'strip_bad_filenames(...)'.
			QSet<QString>::fromList(loaded_files).subtract(
					strip_bad_filenames(QSet<QString>::fromList(loaded_files))));
}
