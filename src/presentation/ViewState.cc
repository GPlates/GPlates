/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "ViewState.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/PaleomagWorkflow.h"
#include "app-logic/PlateVelocityWorkflow.h"
#include "app-logic/Reconstruct.h"

#include "gui/ColourSchemeFactory.h"
#include "gui/ColourSchemeDelegator.h"
#include "gui/FeatureFocus.h"
#include "gui/GeometryFocusHighlight.h"
#include "gui/MapTransform.h"
#include "gui/RenderSettings.h"
#include "gui/SingleColourScheme.h"
#include "gui/ViewportProjection.h"
#include "gui/ViewportZoom.h"

#include "view-operations/ReconstructView.h"
#include "view-operations/RenderedGeometryCollection.h"


GPlatesPresentation::ViewState::ViewState(
		GPlatesAppLogic::ApplicationState &application_state) :
	d_application_state(application_state),
	d_other_view_state(NULL), // FIXME: remove this when refactored
	d_reconstruct(
			new GPlatesAppLogic::Reconstruct(
					application_state.get_model_interface(),
					application_state.get_feature_collection_file_state())),
	d_rendered_geometry_collection(
			new GPlatesViewOperations::RenderedGeometryCollection()),
	d_colour_scheme(
			new GPlatesGui::ColourSchemeDelegator(
					GPlatesGui::ColourSchemeFactory::create_default_plate_id_colour_scheme())),
	d_viewport_zoom(
			new GPlatesGui::ViewportZoom()),
	d_viewport_projection(
			new GPlatesGui::ViewportProjection(GPlatesGui::ORTHOGRAPHIC)),
	d_geometry_focus_highlight(
			new GPlatesGui::GeometryFocusHighlight(*d_rendered_geometry_collection)),
	d_feature_focus(
			new GPlatesGui::FeatureFocus(application_state, *d_reconstruct)),
	d_comp_mesh_point_layer(
			d_rendered_geometry_collection->create_child_rendered_layer_and_transfer_ownership(
					GPlatesViewOperations::RenderedGeometryCollection::COMPUTATIONAL_MESH_LAYER,
					0.175f)),
	d_comp_mesh_arrow_layer(
			d_rendered_geometry_collection->create_child_rendered_layer_and_transfer_ownership(
					GPlatesViewOperations::RenderedGeometryCollection::COMPUTATIONAL_MESH_LAYER,
					0.175f)),
	d_paleomag_layer(
			d_rendered_geometry_collection->create_child_rendered_layer_and_transfer_ownership(
					GPlatesViewOperations::RenderedGeometryCollection::PALEOMAG_LAYER,
					0.175f)),					
	d_plate_velocity_workflow(
			new GPlatesAppLogic::PlateVelocityWorkflow(
					application_state, d_comp_mesh_point_layer, d_comp_mesh_arrow_layer)),
	d_plate_velocity_unregister(
			GPlatesAppLogic::FeatureCollectionWorkflow::register_and_create_auto_unregister_handle(
					d_plate_velocity_workflow.get(),
					application_state.get_feature_collection_file_state())),
	d_paleomag_workflow(
			new GPlatesAppLogic::PaleomagWorkflow(
					application_state, d_paleomag_layer)),
	d_paleomag_unregister(
			GPlatesAppLogic::FeatureCollectionWorkflow::register_and_create_auto_unregister_handle(
					d_paleomag_workflow.get(),
					application_state.get_feature_collection_file_state())),
	d_reconstruct_view(
			new GPlatesViewOperations::ReconstructView(
					*d_plate_velocity_workflow, *d_paleomag_workflow,*d_rendered_geometry_collection)),
	d_last_single_colour(Qt::white),
	d_render_settings(
			new GPlatesGui::RenderSettings()),
	d_map_transform(
			new GPlatesGui::MapTransform()),
	d_main_viewport_min_dimension(0)
{
	// Call the operations in ReconstructView whenever a reconstruction is generated.
	d_reconstruct->set_reconstruction_hook(d_reconstruct_view.get());

	connect_to_viewport_zoom();

	// Connect to signals from FeatureCollectionFileState.
	connect_to_file_state();

	// Connect to signals from FeatureCollectionFileIO.
	connect_to_file_io();

	// Connect to signals from FeatureFocus.
	connect_to_feature_focus();

	// Setup RenderedGeometryCollection.
	setup_rendered_geometry_collection();
}


GPlatesPresentation::ViewState::~ViewState()
{
	// boost::scoped_ptr destructor requires complete type.
}


GPlatesAppLogic::ApplicationState &
GPlatesPresentation::ViewState::get_application_state()
{
	return d_application_state;
}


GPlatesAppLogic::Reconstruct &
GPlatesPresentation::ViewState::get_reconstruct()
{
	return *d_reconstruct;
}


GPlatesViewOperations::RenderedGeometryCollection &
GPlatesPresentation::ViewState::get_rendered_geometry_collection()
{
	return *d_rendered_geometry_collection;
}


GPlatesGui::FeatureFocus &
GPlatesPresentation::ViewState::get_feature_focus()
{
	return *d_feature_focus;
}


GPlatesGui::ViewportZoom &
GPlatesPresentation::ViewState::get_viewport_zoom()
{
	return *d_viewport_zoom;
}


GPlatesGui::ViewportProjection &
GPlatesPresentation::ViewState::get_viewport_projection()
{
	return *d_viewport_projection;
}


const GPlatesAppLogic::PlateVelocityWorkflow &
GPlatesPresentation::ViewState::get_plate_velocity_workflow() const
{
	return *d_plate_velocity_workflow;
}


void
GPlatesPresentation::ViewState::choose_colour_by_feature_type()
{
	d_colour_scheme->set_colour_scheme(
			GPlatesGui::ColourSchemeFactory::create_default_feature_colour_scheme());
}


void
GPlatesPresentation::ViewState::choose_colour_by_age()
{
	d_colour_scheme->set_colour_scheme(
			GPlatesGui::ColourSchemeFactory::create_default_age_colour_scheme(
				get_reconstruct()));
}


void
GPlatesPresentation::ViewState::choose_colour_by_single_colour(
		const QColor &qcolor)
{
	d_colour_scheme->set_colour_scheme(
			GPlatesGui::ColourSchemeFactory::create_single_colour_scheme(qcolor));
	d_last_single_colour = qcolor;
}


void
GPlatesPresentation::ViewState::choose_colour_by_plate_id_default()
{
	d_colour_scheme->set_colour_scheme(
			GPlatesGui::ColourSchemeFactory::create_default_plate_id_colour_scheme());
}


void
GPlatesPresentation::ViewState::choose_colour_by_plate_id_regional()
{
	d_colour_scheme->set_colour_scheme(
			GPlatesGui::ColourSchemeFactory::create_regional_plate_id_colour_scheme());
}


QColor
GPlatesPresentation::ViewState::get_last_single_colour() const
{
	return d_last_single_colour;
}


boost::shared_ptr<GPlatesGui::ColourScheme>
GPlatesPresentation::ViewState::get_colour_scheme()
{
	return d_colour_scheme;
}


GPlatesGui::RenderSettings &
GPlatesPresentation::ViewState::get_render_settings()
{
	return *d_render_settings;
}


GPlatesGui::MapTransform &
GPlatesPresentation::ViewState::get_map_transform()
{
	return *d_map_transform;
}


int
GPlatesPresentation::ViewState::get_main_viewport_min_dimension()
{
	return d_main_viewport_min_dimension;
}


void
GPlatesPresentation::ViewState::set_main_viewport_min_dimension(
		int min_dimension)
{
	d_main_viewport_min_dimension = min_dimension;
}


void
GPlatesPresentation::ViewState::handle_zoom_change()
{
	d_rendered_geometry_collection->set_viewport_zoom_factor(d_viewport_zoom->zoom_factor());
}


void
GPlatesPresentation::ViewState::setup_rendered_geometry_collection()
{
	// Reconstruction rendered layer is always active.
	d_rendered_geometry_collection->set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER);

	d_rendered_geometry_collection->set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::COMPUTATIONAL_MESH_LAYER);
		
	d_rendered_geometry_collection->set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::SMALL_CIRCLE_TOOL_LAYER);	
		
	d_rendered_geometry_collection->set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::PALEOMAG_LAYER);			
			

	// Activate the main rendered layer.
	// Specify which main rendered layers are orthogonal to each other - when
	// one is activated the others are automatically deactivated.
	GPlatesViewOperations::RenderedGeometryCollection::orthogonal_main_layers_type orthogonal_main_layers;
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::DIGITISATION_LAYER);
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_LAYER);
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::GEOMETRY_FOCUS_HIGHLIGHT_LAYER);
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::MEASURE_DISTANCE_LAYER);
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_TOOL_LAYER);

	d_rendered_geometry_collection->set_orthogonal_main_layers(orthogonal_main_layers);
}


void
GPlatesPresentation::ViewState::connect_to_viewport_zoom()
{
	// Handle zoom changes.
	QObject::connect(d_viewport_zoom.get(), SIGNAL(zoom_changed()),
			this, SLOT(handle_zoom_change()));
}


void
GPlatesPresentation::ViewState::connect_to_file_state()
{
	QObject::connect(
			&d_application_state.get_feature_collection_file_state(),
			SIGNAL(file_state_changed(
					GPlatesAppLogic::FeatureCollectionFileState &)),
			d_reconstruct.get(),
			SLOT(reconstruct()));
}


void
GPlatesPresentation::ViewState::connect_to_file_io()
{
	QObject::connect(
			&d_application_state.get_feature_collection_file_io(),
			SIGNAL(remapped_shapefile_attributes(
					GPlatesAppLogic::FeatureCollectionFileIO &,
					GPlatesAppLogic::FeatureCollectionFileState::file_iterator)),
			d_reconstruct.get(),
			SLOT(reconstruct()));
}


void
GPlatesPresentation::ViewState::connect_to_feature_focus()
{
	// If the focused feature is modified, we may need to reconstruct to update the view.
	// FIXME:  If the FeatureFocus emits the 'focused_feature_modified' signal, the view will
	// be reconstructed twice -- once here, and once as a result of the 'set_focus' slot in the
	// GeometryFocusHighlight below.
	QObject::connect(
			&get_feature_focus(),
			SIGNAL(focused_feature_modified(GPlatesGui::FeatureFocus &)),
			&get_reconstruct(),
			SLOT(reconstruct()));

	// Connect the geometry-focus highlight to the feature focus.
	QObject::connect(
			&get_feature_focus(),
			SIGNAL(focus_changed(
					GPlatesGui::FeatureFocus &)),
			d_geometry_focus_highlight.get(),
			SLOT(set_focus(
					GPlatesGui::FeatureFocus &)));

	QObject::connect(
			&get_feature_focus(),
			SIGNAL(focused_feature_modified(
					GPlatesGui::FeatureFocus &)),
			d_geometry_focus_highlight.get(),
			SLOT(set_focus(
					GPlatesGui::FeatureFocus &)));
}
