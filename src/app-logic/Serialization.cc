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


#include <QDebug>
#include <QFileInfo>
#include <QMap>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>

#include "Serialization.h"

#include "ApplicationState.h"
#include "FeatureCollectionFileState.h"
#include "ReconstructGraph.h"
#include "Layer.h"
#include "LayerTask.h"
#include "LayerTaskType.h"
#include "LayerTaskRegistry.h"
#include "Session.h"

#include "file-io/FileInfo.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

namespace
{
	typedef QMap<GPlatesAppLogic::Layer, QString> LayerIdMap;
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


	/**
	 * Saves the specified layer task type.
	 */
	void
	save_layer_task_type(
			QDomElement el,
			GPlatesAppLogic::LayerTaskType::Type layer_task_type)
	{
		// Get the id-to-layer-task-type map for the latest session version.
		const IdLayerTaskTypeMap &id_layer_task_type_map =
				get_id_layer_task_type_map(
						GPlatesAppLogic::Session::get_latest_session_version());

		// Lookup the id string from the layer task type.
		QList<QString> layer_task_type_ids = id_layer_task_type_map.keys(layer_task_type);
		if (layer_task_type_ids.size() != 1)
		{
			// Shouldn't happen because mapping should be one-to-one.
			return;
		}
		const QString layer_task_type_id = layer_task_type_ids.front();

		el.setAttribute("type", layer_task_type_id);
	}


	/**
	 * Turn a Layer into a QDomElement.
	 * Does not add this element anywhere in the DOM tree, just makes it.
	 */
	QDomElement
	save_layer(
			GPlatesAppLogic::Session::LayersStateType dom,
			GPlatesAppLogic::ReconstructGraph &/*rg*/,
			const GPlatesAppLogic::Layer &layer,
			LayerIdMap &idmap)
	{
		// Generate a new ID for this layer.
		idmap.insert(layer, QString("L%1").arg(idmap.size()));

		QDomElement el = dom.createElement("Layer");
		el.setAttribute("id", idmap.value(layer));
		save_layer_task_type(el, layer.get_type());
		el.setAttribute("main_input_channel", layer.get_main_input_feature_collection_channel());
		el.setAttribute("is_active", layer.is_active() ? 1 : 0);
		el.setAttribute("auto_created", layer.get_auto_created() ? 1 : 0);
		return el;
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
	 * Turn a Layer::InputConnection into a QDomElement.
	 * Does not add this element anywhere in the DOM tree, just makes it.
	 */
	QDomElement
	save_layer_connection(
			GPlatesAppLogic::Session::LayersStateType dom,
			GPlatesAppLogic::ReconstructGraph &/*rg*/,
			const GPlatesAppLogic::Layer::InputConnection &con,
			LayerIdMap &idmap)
	{
		QDomElement el = dom.createElement("InputConnection");
		if (con.get_input_file()) {
			el.setAttribute("type", "InputFile");
			// Identify InputFiles by filepath.
			el.setAttribute("from", con.get_input_file()->get_file_info().get_qfileinfo().absoluteFilePath());

		} else if (con.get_input_layer()) {
			el.setAttribute("type", "Layer");
			// Identify Layers by previously set ID
			el.setAttribute("from", idmap.value(*con.get_input_layer()));

		} else {
			// FIXME: Is supposed to be one or the other. Should maybe throw an exception here,...
			el.setAttribute("type", "unknown");
		}
		el.setAttribute("input_channel_name", con.get_input_channel_name());
		// Identify Parent layer (i.e. connect data "to") using previously set ID
		el.setAttribute("to", idmap.value(con.get_layer()));

		return el;
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
		QString input_channel = el.attribute("input_channel_name");

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
				if (input_channel == "Topological boundary section features")
				{
					return GPlatesAppLogic::Layer::InputConnection();	// a deprecated InputConnection.
				}
			}
			if (to_layer.get_type() == GPlatesAppLogic::LayerTaskType::TOPOLOGY_NETWORK_RESOLVER)
			{
				// Note that the following string literal is deprecated and so this is now
				// the only instance of it in the GPlates source code.
				if (input_channel == "Topological section features")
				{
					return GPlatesAppLogic::Layer::InputConnection();	// a deprecated InputConnection.
				}
			}
		}

		if (el.attribute("type") == "InputFile") {
			// What file are we going to take the data from?
			GPlatesAppLogic::Layer::InputFile from_file = get_input_file_by_id(fs, rg, el.attribute("from"));
			if ( ! from_file.is_valid()) {
				// Fail, source InputFile is not valid.
				return GPlatesAppLogic::Layer::InputConnection();	// an invalid InputConnection.
			}
			GPlatesAppLogic::Layer::InputConnection ic = to_layer.connect_input_to_file(from_file, input_channel);
			return ic;

		} else if (el.attribute("type") == "Layer") {
			// What layer are we going to take the data from?
			GPlatesAppLogic::Layer from_layer = idmap.value(el.attribute("from"));
			if ( ! from_layer.is_valid()) {
				// Fail, source Layer is not valid.
				return GPlatesAppLogic::Layer::InputConnection();	// an invalid InputConnection.
			}
			GPlatesAppLogic::Layer::InputConnection ic = to_layer.connect_input_to_layer_output(from_layer, input_channel);
			return ic;

		} else {
			// FIXME: Is supposed to be one or the other. Should maybe throw an exception here,...
			return GPlatesAppLogic::Layer::InputConnection();	// an invalid InputConnection.
		}
	}


	/**
	 * Turn an InputFile into a QDomElement.
	 * Does not add this element anywhere in the DOM tree, just makes it.
	 */
	QDomElement
	save_input_file(
			GPlatesAppLogic::Session::LayersStateType dom,
			GPlatesAppLogic::ReconstructGraph &/*rg*/,
			const GPlatesAppLogic::Layer::InputFile &inputfile)
	{
		QDomElement el = dom.createElement("InputFile");
		el.setAttribute("name", inputfile.get_file_info().get_qfileinfo().absoluteFilePath());
		el.setAttribute("id", inputfile.get_file_info().get_qfileinfo().absoluteFilePath());
		return el;
	}


	/**
	 * Confirms whether the given InputConnection is valid for the purposes of serialisation.
	 */
	bool
	valid_input_connection(
			const GPlatesAppLogic::Layer::InputConnection &con)
	{
		// If an InputConnection is a file-type connection, and the InputFile has an empty filename
		// (i.e. a temporary in-memory feature collection only), it is not valid for session saving.
		if (con.get_input_file() && con.get_input_file()->get_file_info().get_qfileinfo().absoluteFilePath().isEmpty()) {
			return false;
		}
		// Otherwise, just ensure the InputConnection reference is still a valid reference (it should be).
		return con.is_valid();
	}		
}



GPlatesAppLogic::Serialization::Serialization(
		GPlatesAppLogic::ApplicationState &app_state):
	QObject(NULL),
	d_app_state_ptr(&app_state)
{  }


// For reference while I unravel some of the boost serialisation stuff that didn't work.
// We may need this boost -> Qt coercion code later on.
#if 0
QString
GPlatesAppLogic::Serialization::save_layers_state()
{
	// First, create a stringstream to buffer the result of serialising all the layers state with boost.
	std::stringstream ss;
	// We will use XML as the archive format because it is slightly less unreadable than the alternatives.
	boost::archive::xml_oarchive oa(ss);

	// Serialising the reconstruct graph will also serialise all the relevant members recursively.
	oa << boost::serialization::make_nvp("reconstruct_graph", d_app_state_ptr->get_reconstruct_graph());

	// Now let's coerce that stupid STL stuff into a QString.
	QString state = QString::fromUtf8(ss.str().data());
	return state;
}
void
GPlatesAppLogic::Serialization::load_layers_state(
		const QString &state)
{
	// First, we need to shove the noble QString into one of those ugly STL classes.
	std::string str(state.toUtf8().data());
	std::stringstream ss(str);
	// We are using XML as the archive format because it is slightly less unreadable than the alternatives.
	boost::archive::xml_iarchive ia(ss);	// ia! ia! Cthulhu fhtagn!

	// Deserialising the reconstruct graph will deserialise the "logical layers state" struct and
	// go on to configure the layer connections appropriately.
	ia >> boost::serialization::make_nvp("reconstruct_graph", d_app_state_ptr->get_reconstruct_graph());
}
#endif // boost serialisation reference


GPlatesAppLogic::Session::LayersStateType
GPlatesAppLogic::Serialization::save_layers_state()
{
	// A LayersStateType is actually just a DOM document (for now. I'd prefer to roll a subclass for our use)
	GPlatesAppLogic::Session::LayersStateType dom("LayersState");
	QDomElement el_root = dom.createElement("LayersState");
	dom.appendChild(el_root);

	// We need the ReconstructGraph to get at the logical state of the graph.
	ReconstructGraph &rg = d_app_state_ptr->get_reconstruct_graph();
	// We also need a means of tracking IDs for layers. I think I'd prefer to put this in the abovementioned
	// specialist subclass of the DOM, to keep all of that in one class that can help us serialise things,
	// but for now stick with this ID map and a bunch of anon namespace fns.
	LayerIdMap idmap;

	// Index all the InputFiles files that can be referenced by the Layers
	QDomElement el_files = dom.createElement("Files");
	el_root.appendChild(el_files);

	const std::vector<FeatureCollectionFileState::file_reference> loaded_files =
			d_app_state_ptr->get_feature_collection_file_state().get_loaded_files();
	BOOST_FOREACH(const FeatureCollectionFileState::file_reference &file_ref, loaded_files) {
		// List all *valid* InputFiles, excluding those with empty filenames.
		if ( ! file_ref.get_file().get_file_info().get_qfileinfo().absoluteFilePath().isEmpty()) {
			Layer::InputFile infile = rg.get_input_file(file_ref);
			if (infile.is_valid()) {
				el_files.appendChild(save_input_file(dom, rg, infile));
			}
		}
	}


	// Index all the Layer objects themselves.
	QDomElement el_layers = dom.createElement("Layers");
	el_root.appendChild(el_layers);

	for (ReconstructGraph::iterator it = rg.begin(); it != rg.end(); ++it) {
		el_layers.appendChild(save_layer(dom, rg, *it, idmap));
	}

	// Once that's done, we can reference Layers by ID. One such relationship we need to save is
	// the "Default Reconstruction Tree" layer, if there is one.
	Layer default_recon_layer = rg.get_default_reconstruction_tree_layer();
	if (default_recon_layer.is_valid()) {
		QDomElement el_default_recon = dom.createElement("DefaultReconstructionTree");
		el_default_recon.setAttribute("layer", idmap.value(default_recon_layer));
		el_root.appendChild(el_default_recon);
	}


	// Finally, index all the Layer connections.
	QDomElement el_connections = dom.createElement("Connections");
	el_root.appendChild(el_connections);

	for (ReconstructGraph::iterator it = rg.begin(); it != rg.end(); ++it) {
		std::vector<Layer::InputConnection> connections = it->get_all_inputs();
		BOOST_FOREACH(const Layer::InputConnection &con, connections) {
			// Make sure to skip over InputConnections that refer to an empty-filename InputFile.
			if (valid_input_connection(con)) {
				el_connections.appendChild(save_layer_connection(dom, rg, con, idmap));
			}
		}
	}

	return dom;
}


void
GPlatesAppLogic::Serialization::load_layers_state(
		const GPlatesAppLogic::Session::LayersStateType &dom,
		int session_version)
{
	// We should already have Impl::Data objects loaded due to the way we suppressed the auto-layer-creation code.
	// So we'll have the InputFile objects available. We *could* load those separately later, but I'm happy enough
	// to assume that the InputFiles match the actual loaded feature collections. Our current means of identifying
	// an InputFile connection is from absolute file path, so we don't need to actually load the InputFile state
	// from the LayersStateType, not for now anyway.

	// We need the ReconstructGraph to reset the logical state of the graph.
	ReconstructGraph &rg = d_app_state_ptr->get_reconstruct_graph();
	// And the LayerTaskRegistry before we can create Layers.
	LayerTaskRegistry &ltr = d_app_state_ptr->get_layer_task_registry();
	// We also need a means of tracking IDs for layers. I think I'd prefer to put this in the (save_layers_state()) abovementioned
	// specialist subclass of the DOM, to keep all of that in one class that can help us serialise things,
	// but for now stick with this ID map and a bunch of anon namespace fns.
	IdLayerMap idmap;

	// First we need to re-instate the Layers that are specified in the LayersStateType though.
	QDomElement el_root = dom.firstChildElement("LayersState");
	QDomElement el_layers = el_root.firstChildElement("Layers");
	for (QDomElement el_layer = el_layers.firstChildElement("Layer");
		  ! el_layer.isNull();
		  el_layer = el_layer.nextSiblingElement("Layer")) {
		load_layer(ltr, rg, el_layer, idmap, session_version);
	}

	// Once that's done, we can reference Layers by ID. One such relationship we need to load is
	// the "Default Reconstruction Tree" layer, if there is one.
	QDomElement el_default_recon = el_root.firstChildElement("DefaultReconstructionTree");
	if ( ! el_default_recon.isNull() && el_default_recon.hasAttribute("layer")) {
		Layer default_recon_layer = idmap.value(el_default_recon.attribute("layer"));
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
					d_app_state_ptr->get_feature_collection_file_state(), ltr, rg, el_con, idmap, session_version);
		}
	}


	// Aaaand we're done.
}



void
GPlatesAppLogic::Serialization::debug_serialise()
{
	qDebug() << "\nSERIALISING:-\n";

	GPlatesAppLogic::Session::LayersStateType state = save_layers_state();
	qDebug() << state.toString();
}

