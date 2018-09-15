/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
#include <iterator>
#include <boost/foreach.hpp>
#include <QDebug>

#include "VisualLayers.h"

#include "ViewState.h"
#include "VisualLayerRegistry.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/LayerTaskRegistry.h"
#include "app-logic/ReconstructGraph.h"

#include "gui/DrawStyleManager.h"
#include "gui/Symbol.h"

#include "view-operations/RenderedGeometryLayer.h"
#include "view-operations/RenderedGeometryParameters.h"

#include "utils/Profile.h"


GPlatesPresentation::VisualLayers::VisualLayers(
		GPlatesAppLogic::ApplicationState &application_state,
		ViewState &view_state,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection) :
	d_application_state(application_state),
	d_view_state(view_state),
	d_rendered_geometry_collection(rendered_geometry_collection),
	d_next_visual_layer_number(1)
{
	make_signal_slot_connections();

	// Go through the reconstruct graph and add all the existing layers, if any.
	GPlatesAppLogic::ReconstructGraph &reconstruct_graph = application_state.get_reconstruct_graph();
	BOOST_FOREACH(GPlatesAppLogic::Layer layer, reconstruct_graph)
	{
		add_layer(layer);
	}
}


size_t
GPlatesPresentation::VisualLayers::size() const
{
	return d_layer_order.size();
}


boost::weak_ptr<const GPlatesPresentation::VisualLayer>
GPlatesPresentation::VisualLayers::get_visual_layer(
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type index) const
{
	index_map_type::const_iterator iter = d_index_map.find(index);
	if (iter == d_index_map.end())
	{
		return boost::weak_ptr<VisualLayer>();
	}
	else
	{
		return iter->second;
	}
}


boost::weak_ptr<GPlatesPresentation::VisualLayer>
GPlatesPresentation::VisualLayers::get_visual_layer(
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type index)
{
	index_map_type::iterator iter = d_index_map.find(index);
	if (iter == d_index_map.end())
	{
		return boost::weak_ptr<VisualLayer>();
	}
	else
	{
		return iter->second;
	}
}


boost::weak_ptr<const GPlatesPresentation::VisualLayer>
GPlatesPresentation::VisualLayers::visual_layer_at(
		size_t index) const
{
	return get_visual_layer(d_layer_order[index]);
}


boost::weak_ptr<GPlatesPresentation::VisualLayer>
GPlatesPresentation::VisualLayers::visual_layer_at(
		size_t index)
{
	return get_visual_layer(d_layer_order[index]);
}


GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type
GPlatesPresentation::VisualLayers::child_layer_index_at(
		size_t index) const
{
	return d_layer_order[index];
}


GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type
GPlatesPresentation::VisualLayers::child_layer_index_at(
		size_t index)
{
	return d_layer_order[index];
}


boost::weak_ptr<const GPlatesPresentation::VisualLayer>
GPlatesPresentation::VisualLayers::get_visual_layer(
		const GPlatesAppLogic::Layer &layer) const
{
	visual_layer_map_type::const_iterator iter = d_visual_layers.find(layer);
	if (iter == d_visual_layers.end())
	{
		return boost::weak_ptr<const GPlatesPresentation::VisualLayer>();
	}
	else
	{
		return iter->second;
	}
}


boost::weak_ptr<GPlatesPresentation::VisualLayer>
GPlatesPresentation::VisualLayers::get_visual_layer(
		const GPlatesAppLogic::Layer &layer)
{
	visual_layer_map_type::iterator iter = d_visual_layers.find(layer);
	if (iter == d_visual_layers.end())
	{
		return boost::weak_ptr<GPlatesPresentation::VisualLayer>();
	}
	else
	{
		return iter->second;
	}
}


GPlatesPresentation::VisualLayers::const_iterator
GPlatesPresentation::VisualLayers::order_begin() const
{
	return d_layer_order.begin();
}


GPlatesPresentation::VisualLayers::const_iterator
GPlatesPresentation::VisualLayers::order_end() const
{
	return d_layer_order.end();
}

void
GPlatesPresentation::VisualLayers::show_all()
{
	for (unsigned int n = 0; n < size() ; ++n)
	{
		boost::shared_ptr<GPlatesPresentation::VisualLayer> visual_layer =
				visual_layer_at(n).lock();
		if (visual_layer)
		{
			visual_layer->set_visible(true);
		}
	}
}

void
GPlatesPresentation::VisualLayers::hide_all()
{
	for (unsigned int n = 0; n < size() ; ++n)
	{
		boost::shared_ptr<GPlatesPresentation::VisualLayer> visual_layer =
				visual_layer_at(n).lock();
		if (visual_layer)
		{
			visual_layer->set_visible(false);
		}
	}
}


const GPlatesPresentation::VisualLayers::rendered_geometry_layer_seq_type &
GPlatesPresentation::VisualLayers::get_layer_order() const
{
	return d_layer_order;
}


void
GPlatesPresentation::VisualLayers::make_signal_slot_connections()
{
	// Create rendered geometries each time a reconstruction has happened.
	QObject::connect(
			&d_application_state,
			SIGNAL(reconstructed(
					GPlatesAppLogic::ApplicationState &)),
			this,
			SLOT(create_rendered_geometries()));

	// Connect to ReconstructGraph signals.
	GPlatesAppLogic::ReconstructGraph *reconstruct_graph = &d_application_state.get_reconstruct_graph();
	QObject::connect(
			reconstruct_graph,
			SIGNAL(begin_add_or_remove_layers()),
			this,
			SLOT(handle_begin_add_or_remove_layers()));
	QObject::connect(
			reconstruct_graph,
			SIGNAL(end_add_or_remove_layers()),
			this,
			SLOT(handle_end_add_or_remove_layers()));
	QObject::connect(
			reconstruct_graph,
			SIGNAL(layer_added(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer)),
			this,
			SLOT(handle_layer_added(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer)));
	QObject::connect(
			reconstruct_graph,
			SIGNAL(layer_about_to_be_removed(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer)),
			this,
			SLOT(handle_layer_about_to_be_removed(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer)));
	QObject::connect(
			reconstruct_graph,
			SIGNAL(layer_removed(
					GPlatesAppLogic::ReconstructGraph &)),
			this,
			SLOT(handle_layer_removed(
					GPlatesAppLogic::ReconstructGraph &)));
	QObject::connect(
			reconstruct_graph,
			SIGNAL(layer_activation_changed(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer,
					bool)),
			this,
			SLOT(handle_layer_activation_changed(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer,
					bool)));
	QObject::connect(
			reconstruct_graph,
			SIGNAL(layer_params_changed(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer,
					GPlatesAppLogic::LayerParams &)),
			this,
			SLOT(handle_layer_params_changed(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer,
					GPlatesAppLogic::LayerParams &)));
	QObject::connect(
			reconstruct_graph,
			SIGNAL(layer_added_input_connection(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer,
					GPlatesAppLogic::Layer::InputConnection)),
			this,
			SLOT(handle_layer_added_input_connection(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer,
					GPlatesAppLogic::Layer::InputConnection)));
	QObject::connect(
			reconstruct_graph,
			SIGNAL(layer_removed_input_connection(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer)),
			this,
			SLOT(handle_layer_removed_input_connection(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer)));
	QObject::connect(
			reconstruct_graph,
			SIGNAL(default_reconstruction_tree_layer_changed(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer,
					GPlatesAppLogic::Layer)),
			this,
			SLOT(handle_default_reconstruction_tree_layer_changed(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer,
					GPlatesAppLogic::Layer)));

	// Connect to FeatureCollectionFileState signals.
	GPlatesAppLogic::FeatureCollectionFileState *file_state = &d_application_state.get_feature_collection_file_state();
	QObject::connect(
			file_state,
			SIGNAL(file_state_changed(
					GPlatesAppLogic::FeatureCollectionFileState &)),
			SLOT(handle_file_state_changed(
					GPlatesAppLogic::FeatureCollectionFileState &)));

	// Create new rendered geometries when RenderedGeometryParameters changes.
	QObject::connect(
			&d_view_state.get_rendered_geometry_parameters(),
			SIGNAL(parameters_changed(GPlatesViewOperations::RenderedGeometryParameters &)),
			this,
			SLOT(create_rendered_geometries()));

	// Connect to DrawStyleManager signals.
	QObject::connect(
			GPlatesGui::DrawStyleManager::instance(),
			SIGNAL(draw_style_changed()),
			this,
			SLOT(create_rendered_geometries()));
}


void
GPlatesPresentation::VisualLayers::handle_begin_add_or_remove_layers()
{
	Q_EMIT begin_add_or_remove_layers();
}


void
GPlatesPresentation::VisualLayers::handle_end_add_or_remove_layers()
{
	Q_EMIT end_add_or_remove_layers();
}


void
GPlatesPresentation::VisualLayers::handle_layer_added(
		GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
		GPlatesAppLogic::Layer layer)
{
	add_layer(layer);
}


void
GPlatesPresentation::VisualLayers::add_layer(
		GPlatesAppLogic::Layer &layer)
{
	// Work out where the new layer should go.
	std::size_t new_index = get_index_of_new_layer(static_cast<VisualLayerType::Type>(layer.get_type()));;
	Q_EMIT layer_about_to_be_added(new_index);

	// Create a new visual layer.
	const visual_layer_ptr_type visual_layer = create_visual_layer(layer);

	// Associate the visual layer with the layer.
	d_visual_layers.insert(std::make_pair(layer, visual_layer));

	// Add new layer's rendered geometry layer index to the ordered sequence.
	d_layer_order.insert(
			d_layer_order.begin() + new_index,
			visual_layer->get_rendered_geometry_layer_index());

	// Associate rendered geometry layer index with the visual layer.
	d_index_map.insert(
			std::make_pair(
				visual_layer->get_rendered_geometry_layer_index(),
				visual_layer));

	Q_EMIT layer_added(new_index);
	Q_EMIT layer_added(visual_layer);
	Q_EMIT changed();
}


std::size_t
GPlatesPresentation::VisualLayers::get_index_of_new_layer(
		VisualLayerType::Type new_type) const
{
	// Searching from the back of the ordering to the front (because that is how it
	// is displayed on screen):
	//  - If the @a visual_layer_type is already in @a d_layer_order, put the
	//    new layer after the first layer found of the same type.
	//  - If we don't already have that visual layer type, put the new layer after
	//    the first layer found that, according to the @a layer_type_order,
	//    should belong before the new layer.
	const VisualLayerRegistry::visual_layer_type_order_map_type &order_map =
		d_view_state.get_visual_layer_registry().get_visual_layer_type_order_map();
	std::size_t fallback_index = 0;
	bool fallback_found = false;

	for (std::size_t i = d_layer_order.size(); i != 0; --i)
	{
		boost::weak_ptr<VisualLayer> visual_layer = d_index_map.find(d_layer_order[i - 1])->second;
		if (boost::shared_ptr<VisualLayer> locked_visual_layer = visual_layer.lock())
		{
			VisualLayerType::Type curr_type = locked_visual_layer->get_layer_type();
			if (curr_type == new_type)
			{
				return i;
			}

			if (!fallback_found)
			{
				typedef VisualLayerRegistry::visual_layer_type_order_map_type::const_iterator order_map_iter;
				order_map_iter curr_type_order_iter = order_map.find(curr_type);
				order_map_iter new_type_order_iter = order_map.find(new_type);

				if (curr_type_order_iter != order_map.end() && new_type_order_iter != order_map.end() &&
					curr_type_order_iter->second < new_type_order_iter->second)
				{
					// The current layer belongs before the new layer, so insert new layer after it.
					fallback_found = true;
					fallback_index = i;
				}
			}
		}
	}

	if (fallback_found)
	{
		return fallback_index;
	}
	else
	{
		return 0;
	}
}


void
GPlatesPresentation::VisualLayers::handle_layer_about_to_be_removed(
		GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
		GPlatesAppLogic::Layer layer)
{
	remove_layer(layer);

	// Note that @a refresh_all_layers is called from @a handle_layer_removed, not
	// here, because we need to wait for the layer to be actually removed first
	// before causing any refreshes to occur.
}


void
GPlatesPresentation::VisualLayers::handle_layer_removed(
		GPlatesAppLogic::ReconstructGraph &reconstruct_graph)
{
	// We need to refresh all layers now in case other layers were referencing the
	// layer that just got removed.
	refresh_all_layers();
}


void
GPlatesPresentation::VisualLayers::remove_layer(
		const GPlatesAppLogic::Layer &layer)
{
	// Retrieve the visual layer associated with the layer.
	visual_layer_map_type::iterator visual_layers_iter = d_visual_layers.find(layer);
	if (visual_layers_iter == d_visual_layers.end())
	{
		return;
	}
	const visual_layer_ptr_type &visual_layer = visual_layers_iter->second;

	// Remove the layer's rendered geometry layer index from the ordered sequence
	// of layer indices.
	GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type geometry_layer_index =
		visual_layer->get_rendered_geometry_layer_index();
	rendered_geometry_layer_seq_type::iterator layer_order_iter =
		std::find(d_layer_order.begin(), d_layer_order.end(), geometry_layer_index);
	if (layer_order_iter != d_layer_order.end())
	{
		size_t order_seq_index = layer_order_iter - d_layer_order.begin();

		Q_EMIT layer_about_to_be_removed(order_seq_index);
		Q_EMIT layer_about_to_be_removed(visual_layer);

		d_layer_order.erase(layer_order_iter);

		Q_EMIT layer_removed(order_seq_index);
		Q_EMIT changed();
	}

	// Also remove the entry from the map from layer index to visual layer.
	d_index_map.erase(geometry_layer_index);

	// Finally, destroy the visual layer associated with the layer.
	d_visual_layers.erase(layer);
}


void
GPlatesPresentation::VisualLayers::create_rendered_geometries()
{
	PROFILE_FUNC();
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without any canvas' redrawing themselves after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Get the reconstruction rendered layer.
	GPlatesViewOperations::RenderedGeometryLayer *reconstruction_layer =
		d_rendered_geometry_collection.get_main_rendered_layer(
				GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER);

	// Activate the main layer.
	d_rendered_geometry_collection.set_main_layer_active(
			GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER);

	// Clear all RenderedGeometry's before adding new ones.
	// Actually this is not really necessary since each VisualLayer has its own child
	// rendered layer that it renders into (but we'll keep this here just in case).
	reconstruction_layer->clear_rendered_geometries();

	// Iterate over the visual layers and get each one to create its own
	// rendered geometries.
	BOOST_FOREACH(const visual_layer_map_type::value_type &visual_layer_map_entry, d_visual_layers)
	{
		const visual_layer_ptr_type &visual_layer = visual_layer_map_entry.second;

		visual_layer->create_rendered_geometries();
	}
}


GPlatesPresentation::VisualLayers::visual_layer_ptr_type
GPlatesPresentation::VisualLayers::create_visual_layer(
		GPlatesAppLogic::Layer &layer)
{
	// Create a new visual layer.
	boost::shared_ptr<VisualLayer> visual_layer(
			new VisualLayer(
				*this,
				d_view_state.get_visual_layer_registry(),
				layer,
				d_rendered_geometry_collection,
				d_view_state.get_rendered_geometry_parameters(),
				d_view_state.get_render_settings(),
				d_view_state.get_feature_type_symbol_map(),
				d_next_visual_layer_number));

	++d_next_visual_layer_number;

	return visual_layer;
}


void
GPlatesPresentation::VisualLayers::move_layer(
		size_t from_index,
		size_t to_index)
{
	if (from_index == to_index)
	{
		// Nothing to do.
		return;
	}

	// We'll create a new sequence and then swap it into place, so it's all nice
	// and exception safe.
	rendered_geometry_layer_seq_type new_order;
	new_order.reserve(d_layer_order.size());

	rendered_geometry_layer_seq_type::iterator begin = d_layer_order.begin();
	rendered_geometry_layer_seq_type::iterator end = d_layer_order.end();

	if (from_index < to_index)
	{
		// Moving an item down the list towards the back.
		// Keep [0, from_index - 1] as is.
		std::copy(
				begin,
				begin + from_index,
				std::back_inserter(new_order));

		// Then copy [from_index + 1, to_index] to [from_index, to_index - 1],
		// effectively shifting the range one place towards the front.
		std::copy(
				begin + from_index + 1,
				begin + to_index + 1,
				std::back_inserter(new_order));

		// Put element originally at from_index at to_index.
		new_order.push_back(d_layer_order[from_index]);

		// Then keep [to_index + 1, end) as is.
		std::copy(
				begin + to_index + 1,
				end,
				std::back_inserter(new_order));

		std::swap(d_layer_order, new_order);
		Q_EMIT layer_order_changed(from_index, to_index);
	}
	else // (to_index < from_index)
	{
		// Moving an item up the list towards the front.
		// Keep [0, to_index - 1] as is.
		std::copy(
				begin,
				begin + to_index,
				std::back_inserter(new_order));

		// Put element at from_index at to_index.
		new_order.push_back(d_layer_order[from_index]);

		// Then copy [to_index, from_index - 1] to [to_index + 1, from_index],
		// effectively shifting the range one place towards the back.
		std::copy(
				begin + to_index,
				begin + from_index,
				std::back_inserter(new_order));

		// Then keep [from_index + 1, end) as is.
		std::copy(
				begin + from_index + 1,
				end,
				std::back_inserter(new_order));

		std::swap(d_layer_order, new_order);
		Q_EMIT layer_order_changed(to_index, from_index);
	}
	Q_EMIT changed();

	// FIXME: There has to be a better way, but let's just do a full reconstruction
	// so the changes in layer ordering get reflected in the main window.
	d_application_state.reconstruct();
}


void
GPlatesPresentation::VisualLayers::handle_layer_activation_changed(
		GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
		GPlatesAppLogic::Layer layer,
		bool activation)
{
	handle_layer_modified(layer);
}


void
GPlatesPresentation::VisualLayers::handle_layer_params_changed(
		GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
		GPlatesAppLogic::Layer layer,
		GPlatesAppLogic::LayerParams &layer_params)
{
	// First notify the visual layer parameters (in case they depend on the app-logic layer params).
	// Then refresh the layer (in case layer options widget needs changing).
	notify_visual_layer_params(layer);
	handle_layer_modified(layer);
}


void
GPlatesPresentation::VisualLayers::handle_layer_added_input_connection(
		GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
		GPlatesAppLogic::Layer layer,
		GPlatesAppLogic::Layer::InputConnection input_connection)
{
	notify_visual_layer_params(layer);

	// When an input connection has been added, all layers need to be refreshed,
	// because a change in input connections can result in a change in the name of
	// a visual layer.
	refresh_all_layers();
}


void
GPlatesPresentation::VisualLayers::handle_layer_removed_input_connection(
		GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
		GPlatesAppLogic::Layer layer)
{
	notify_visual_layer_params(layer);

	// When an input connection has been removed, all layers need to be refreshed,
	// because a change in input connections can result in a change in the name of
	// a visual layer.
	refresh_all_layers();
}


void
GPlatesPresentation::VisualLayers::notify_visual_layer_params(
		const GPlatesAppLogic::Layer &layer)
{
	visual_layer_map_type::const_iterator iter = d_visual_layers.find(layer);
	if (iter != d_visual_layers.end())
	{
		iter->second->get_visual_layer_params()->handle_layer_modified(layer);
	}
}


void
GPlatesPresentation::VisualLayers::handle_file_state_changed(
		GPlatesAppLogic::FeatureCollectionFileState &file_state)
{
	// When the file info for a loaded file has changed, or a loaded file is
	// unloaded, we need to get all the layers to refresh themselves, not just the
	// layer(s) corresponding to the file so modified - because other layers could
	// be using that file as input.
	refresh_all_layers();
}


void
GPlatesPresentation::VisualLayers::handle_default_reconstruction_tree_layer_changed(
		GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
		GPlatesAppLogic::Layer prev_default_reconstruction_tree_layer,
		GPlatesAppLogic::Layer new_default_reconstruction_tree_layer)
{
	handle_layer_modified(prev_default_reconstruction_tree_layer);
	handle_layer_modified(new_default_reconstruction_tree_layer);
}


void
GPlatesPresentation::VisualLayers::handle_layer_modified(
		const GPlatesAppLogic::Layer &layer)
{
	visual_layer_map_type::const_iterator iter = d_visual_layers.find(layer);
	if (iter != d_visual_layers.end())
	{
		emit_layer_modified(iter->second->get_rendered_geometry_layer_index());
	}
}


void
GPlatesPresentation::VisualLayers::refresh_all_layers()
{
	// Emit version of layer_modified that provides a pointer to a VisualLayer.
	for (index_map_type::const_iterator index_iter = d_index_map.begin();
			index_iter != d_index_map.end(); ++index_iter)
	{
		Q_EMIT layer_modified(index_iter->second);
	}

	// Emit version of layer_modified that provides an index.
	for (size_t i = 0; i != d_layer_order.size(); ++i)
	{
		Q_EMIT layer_modified(i);
	}

	Q_EMIT changed();
}


void
GPlatesPresentation::VisualLayers::emit_layer_modified(
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type index)
{
	// Find the corresponding visual layer.
	index_map_type::const_iterator index_iter = d_index_map.find(index);
	if (index_iter == d_index_map.end())
	{
		return;
	}

	// Find the position of the rendered geometry layer index in the ordering.
	rendered_geometry_layer_seq_type::const_iterator order_iter =
		std::find(d_layer_order.begin(), d_layer_order.end(), index);
	if (order_iter != d_layer_order.end())
	{
		Q_EMIT layer_modified(index_iter->second);
		Q_EMIT layer_modified(order_iter - d_layer_order.begin());
		Q_EMIT changed();
	}
}

