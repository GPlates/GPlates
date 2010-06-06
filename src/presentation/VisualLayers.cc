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

#include <boost/foreach.hpp>

#include "VisualLayers.h"

#include "ViewState.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/LayerTaskRegistry.h"

#include "view-operations/RenderedGeometryLayer.h"


GPlatesPresentation::VisualLayers::VisualLayers(
		GPlatesPresentation::ViewState &view_state) :
	d_application_state(view_state.get_application_state()),
	d_rendered_geometry_collection(view_state.get_rendered_geometry_collection())
{
	connect_to_application_state_signals();
}


void
GPlatesPresentation::VisualLayers::connect_to_application_state_signals()
{
	// Create rendered geometries each time a reconstruction has happened.
	QObject::connect(
			&d_application_state,
			SIGNAL(reconstructed(
					GPlatesAppLogic::ApplicationState &)),
			this,
			SLOT(create_rendered_geometries()));

	QObject::connect(
			&d_application_state.get_reconstruct_graph(),
			SIGNAL(layer_added(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer)),
			this,
			SLOT(handle_layer_added(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer)));
	QObject::connect(
			&d_application_state.get_reconstruct_graph(),
			SIGNAL(layer_about_to_be_removed(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer)),
			this,
			SLOT(handle_layer_about_to_be_removed(
					GPlatesAppLogic::ReconstructGraph &,
					GPlatesAppLogic::Layer)));
}


void
GPlatesPresentation::VisualLayers::handle_layer_added(
		GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
		GPlatesAppLogic::Layer layer)
{
	const boost::shared_ptr<VisualLayer> visual_layer = create_visual_layer(layer);

	// Associate the visual layer with the layer.
	d_visual_layers.insert(std::make_pair(layer, visual_layer));
}


void
GPlatesPresentation::VisualLayers::handle_layer_about_to_be_removed(
		GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
		GPlatesAppLogic::Layer layer)
{
	// Destroy the visual layer associated with the layer.
	d_visual_layers.erase(layer);
}


void
GPlatesPresentation::VisualLayers::create_rendered_geometries()
{
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

	// Activate the layer.
	reconstruction_layer->set_active();

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


boost::shared_ptr<GPlatesPresentation::VisualLayer>
GPlatesPresentation::VisualLayers::create_visual_layer(
		const GPlatesAppLogic::Layer &layer)
{
	// Create a new visual layer.
	boost::shared_ptr<VisualLayer> visual_layer(
			new VisualLayer(layer, d_rendered_geometry_collection));

	return visual_layer;
}
