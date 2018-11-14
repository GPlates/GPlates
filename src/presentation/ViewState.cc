/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010, 2011 The University of Sydney, Australia
 * Copyright (C) 2010 Geological Survey of Norway
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

#include <QDir>
#include <QDebug>

#include "ViewState.h"

#include "SessionManagement.h"
#include "VisualLayerRegistry.h"
#include "VisualLayers.h"

#include "api/Sleeper.h"
#include "app-logic/ApplicationState.h"
#include "app-logic/UserPreferences.h"

#include "file-io/CptReader.h"
#include "file-io/ReadErrorAccumulation.h"
#include "file-io/TemporaryFileRegistry.h"

#include "gui/AnimationController.h"
#include "gui/ColourSchemeContainer.h"
#include "gui/ColourSchemeDelegator.h"
#include "gui/ColourPaletteAdapter.h"
#include "gui/CptColourPalette.h"
#include "gui/ExportAnimationRegistry.h"
#include "gui/FeatureFocus.h"
#include "gui/FeatureTableModel.h"
#include "gui/FileIODirectoryConfigurations.h"
#include "gui/GraticuleSettings.h"
#include "gui/MapTransform.h"
#include "gui/PythonManager.h"
#include "gui/RasterColourPalette.h"
#include "gui/RenderSettings.h"
#include "gui/SceneLightingParameters.h"
#include "gui/Symbol.h"
#include "gui/TextOverlaySettings.h"
#include "gui/TopologySectionsContainer.h"
#include "gui/VelocityLegendOverlaySettings.h"
#include "gui/ViewportProjection.h"
#include "gui/ViewportZoom.h"

#include "maths/MathsUtils.h"
#include "maths/Real.h"

#include "qt-widgets/PreferencesPaneFiles.h"

#include "view-operations/FocusedFeatureGeometryManipulator.h"
#include "view-operations/GeometryBuilder.h"
#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryParameters.h"


namespace
{
	GPlatesGui::Colour
	get_default_background_colour()
	{
		return GPlatesGui::Colour(0.35f, 0.35f, 0.35f);
	}
}


GPlatesPresentation::ViewState::ViewState(
		GPlatesAppLogic::ApplicationState &application_state) :
	d_application_state(application_state),
	d_other_view_state(NULL), // FIXME: remove this when refactored
	d_animation_controller(
			new GPlatesGui::AnimationController(application_state)),
	d_session_management_ptr(
			new SessionManagement(application_state)),
	d_rendered_geometry_collection(
			new GPlatesViewOperations::RenderedGeometryCollection()),
	d_feature_focus(
			new GPlatesGui::FeatureFocus(*d_rendered_geometry_collection)),
	d_feature_table_model_ptr(
			new GPlatesGui::FeatureTableModel(
				*d_feature_focus,
				*d_rendered_geometry_collection)),
	d_colour_scheme_container(
			new GPlatesGui::ColourSchemeContainer(application_state)),
	d_colour_scheme(
			new GPlatesGui::ColourSchemeDelegator(*d_colour_scheme_container)),
	d_viewport_zoom(
			new GPlatesGui::ViewportZoom()),
	d_viewport_projection(
			new GPlatesGui::ViewportProjection(GPlatesGui::MapProjection::ORTHOGRAPHIC)),
	d_digitise_geometry_builder(
			new GPlatesViewOperations::GeometryBuilder()),
	d_focused_feature_geometry_builder(
			new GPlatesViewOperations::GeometryBuilder()),
	d_focused_feature_geom_manipulator(
			new GPlatesViewOperations::FocusedFeatureGeometryManipulator(
				*d_focused_feature_geometry_builder,
				*this)),
	d_render_settings(
			new GPlatesGui::RenderSettings()),
	d_rendered_geometry_parameters(
			new GPlatesViewOperations::RenderedGeometryParameters()),
	d_scene_lighting_parameters(
			new GPlatesGui::SceneLightingParameters()),
	d_visual_layers(
			new VisualLayers(d_application_state, *this)),
	d_visual_layer_registry(
			new VisualLayerRegistry()),
	d_map_transform(
			new GPlatesGui::MapTransform(
				*d_viewport_zoom)),
	d_main_viewport_dimensions(0, 0),
	d_file_io_directory_configurations(
			new GPlatesGui::FileIODirectoryConfigurations(
				*this)),
	d_show_stars(false),	// Overriden by UserPreferences: view/show_stars
	d_background_colour(
			new GPlatesGui::Colour(get_default_background_colour())),
	d_graticule_settings(
			new GPlatesGui::GraticuleSettings()),
	d_text_overlay_settings(
			new GPlatesGui::TextOverlaySettings()),
	d_velocity_legend_overlay_settings(
			new GPlatesGui::VelocityLegendOverlaySettings()),
	d_export_animation_registry(
			new GPlatesGui::ExportAnimationRegistry()),
	d_topology_boundary_sections_container_ptr(
			new GPlatesGui::TopologySectionsContainer()),
	d_topology_interior_sections_container_ptr(
			new GPlatesGui::TopologySectionsContainer()),
	d_python_manager_ptr(GPlatesGui::PythonManager::instance())
{
	// Override a few defaults to the user's taste.
	initialise_from_user_preferences();

	connect_to_viewport_zoom();

	// Connect to signals from FeatureFocus.
	connect_to_feature_focus();

	// Set up RenderedGeometryCollection.
	setup_rendered_geometry_collection();

	// Set up VisualLayerRegistry.
	register_default_visual_layers(
			*d_visual_layer_registry,
			d_application_state,
			*this);

	// Set up the ExportAnimationRegistry.
	register_default_export_animation_types(
			*d_export_animation_registry);

}


GPlatesPresentation::ViewState::~ViewState()
{
	// boost::scoped_ptr destructor requires complete type.
}


void
GPlatesPresentation::ViewState::initialise_from_user_preferences()
{
	const GPlatesAppLogic::UserPreferences &prefs = get_application_state().get_user_preferences();
	
	d_show_stars = prefs.get_value("view/show_stars").toBool();

	// By default, geometry visibility is enabled for all geometry types except topological sections
	// (which are hidden, in "DefaultPreferences.conf").
	//
	// Topological sections are features referenced by topologies for *all* reconstruction times.
	// As soon as a topology that references an already loaded feature is loaded, that feature then
	// becomes a topological section. Most users don't want to see these 'dangling bits' around topologies
	// (ie, they just want to see the topologies). The small percentage of users who actually build
	// topologies will have to turn this on manually.
	d_render_settings->set_show_topological_sections(
			prefs.get_value("view/geometry_visibility/show_topological_sections").toBool());
}


GPlatesAppLogic::ApplicationState &
GPlatesPresentation::ViewState::get_application_state()
{
	return d_application_state;
}

const GPlatesAppLogic::ApplicationState &
GPlatesPresentation::ViewState::get_application_state() const
{
	return d_application_state;
}


GPlatesGui::AnimationController&
GPlatesPresentation::ViewState::get_animation_controller()
{
	return *d_animation_controller;
}

const GPlatesGui::AnimationController&
GPlatesPresentation::ViewState::get_animation_controller() const
{
	return *d_animation_controller;
}


GPlatesPresentation::SessionManagement &
GPlatesPresentation::ViewState::get_session_management()
{
	return *d_session_management_ptr;
}

const GPlatesPresentation::SessionManagement &
GPlatesPresentation::ViewState::get_session_management() const
{
	return *d_session_management_ptr;
}


GPlatesViewOperations::RenderedGeometryCollection &
GPlatesPresentation::ViewState::get_rendered_geometry_collection()
{
	return *d_rendered_geometry_collection;
}

const GPlatesViewOperations::RenderedGeometryCollection &
GPlatesPresentation::ViewState::get_rendered_geometry_collection() const
{
	return *d_rendered_geometry_collection;
}


GPlatesGui::FeatureFocus &
GPlatesPresentation::ViewState::get_feature_focus()
{
	return *d_feature_focus;
}

const GPlatesGui::FeatureFocus &
GPlatesPresentation::ViewState::get_feature_focus() const
{
	return *d_feature_focus;
}


GPlatesGui::FeatureTableModel &
GPlatesPresentation::ViewState::get_feature_table_model()
{
	return *d_feature_table_model_ptr;
}

const GPlatesGui::FeatureTableModel &
GPlatesPresentation::ViewState::get_feature_table_model() const
{
	return *d_feature_table_model_ptr;
}


GPlatesGui::ViewportZoom &
GPlatesPresentation::ViewState::get_viewport_zoom()
{
	return *d_viewport_zoom;
}

const GPlatesGui::ViewportZoom &
GPlatesPresentation::ViewState::get_viewport_zoom() const
{
	return *d_viewport_zoom;
}


GPlatesGui::ViewportProjection &
GPlatesPresentation::ViewState::get_viewport_projection()
{
	return *d_viewport_projection;
}

const GPlatesGui::ViewportProjection &
GPlatesPresentation::ViewState::get_viewport_projection() const
{
	return *d_viewport_projection;
}


GPlatesViewOperations::GeometryBuilder &
GPlatesPresentation::ViewState::get_digitise_geometry_builder()
{
	return *d_digitise_geometry_builder;
}

const GPlatesViewOperations::GeometryBuilder &
GPlatesPresentation::ViewState::get_digitise_geometry_builder() const
{
	return *d_digitise_geometry_builder;
}


GPlatesViewOperations::GeometryBuilder &
GPlatesPresentation::ViewState::get_focused_feature_geometry_builder()
{
	return *d_focused_feature_geometry_builder;
}

const GPlatesViewOperations::GeometryBuilder &
GPlatesPresentation::ViewState::get_focused_feature_geometry_builder() const
{
	return *d_focused_feature_geometry_builder;
}


GPlatesGui::ColourSchemeContainer &
GPlatesPresentation::ViewState::get_colour_scheme_container()
{
	return *d_colour_scheme_container;
}

const GPlatesGui::ColourSchemeContainer &
GPlatesPresentation::ViewState::get_colour_scheme_container() const
{
	return *d_colour_scheme_container;
}


GPlatesGlobal::PointerTraits<GPlatesGui::ColourScheme>::non_null_ptr_type
GPlatesPresentation::ViewState::get_colour_scheme()
{
	return d_colour_scheme;
}

GPlatesGlobal::PointerTraits<const GPlatesGui::ColourScheme>::non_null_ptr_type
GPlatesPresentation::ViewState::get_colour_scheme() const
{
	return d_colour_scheme;
}


GPlatesGlobal::PointerTraits<GPlatesGui::ColourSchemeDelegator>::non_null_ptr_type
GPlatesPresentation::ViewState::get_colour_scheme_delegator()
{
	return d_colour_scheme;
}

GPlatesGlobal::PointerTraits<const GPlatesGui::ColourSchemeDelegator>::non_null_ptr_type
GPlatesPresentation::ViewState::get_colour_scheme_delegator() const
{
	return d_colour_scheme;
}


GPlatesGui::RenderSettings &
GPlatesPresentation::ViewState::get_render_settings()
{
	return *d_render_settings;
}

const GPlatesGui::RenderSettings &
GPlatesPresentation::ViewState::get_render_settings() const
{
	return *d_render_settings;
}


GPlatesViewOperations::RenderedGeometryParameters &
GPlatesPresentation::ViewState::get_rendered_geometry_parameters()
{
	return *d_rendered_geometry_parameters;
}

const GPlatesViewOperations::RenderedGeometryParameters &
GPlatesPresentation::ViewState::get_rendered_geometry_parameters() const
{
	return *d_rendered_geometry_parameters;
}


GPlatesGui::SceneLightingParameters &
GPlatesPresentation::ViewState::get_scene_lighting_parameters()
{
	return *d_scene_lighting_parameters;
}

const GPlatesGui::SceneLightingParameters &
GPlatesPresentation::ViewState::get_scene_lighting_parameters() const
{
	return *d_scene_lighting_parameters;
}


GPlatesPresentation::VisualLayers &
GPlatesPresentation::ViewState::get_visual_layers()
{
	return *d_visual_layers;
}

const GPlatesPresentation::VisualLayers &
GPlatesPresentation::ViewState::get_visual_layers() const
{
	return *d_visual_layers;
}


GPlatesPresentation::VisualLayerRegistry &
GPlatesPresentation::ViewState::get_visual_layer_registry()
{
	return *d_visual_layer_registry;
}

const GPlatesPresentation::VisualLayerRegistry &
GPlatesPresentation::ViewState::get_visual_layer_registry() const
{
	return *d_visual_layer_registry;
}


GPlatesGui::MapTransform &
GPlatesPresentation::ViewState::get_map_transform()
{
	return *d_map_transform;
}

const GPlatesGui::MapTransform &
GPlatesPresentation::ViewState::get_map_transform() const
{
	return *d_map_transform;
}


const std::pair<int, int> &
GPlatesPresentation::ViewState::get_main_viewport_dimensions() const
{
	return d_main_viewport_dimensions;
}


void
GPlatesPresentation::ViewState::set_main_viewport_dimensions(
		const std::pair<int, int> &dimensions)
{
	d_main_viewport_dimensions = dimensions;
}


int
GPlatesPresentation::ViewState::get_main_viewport_min_dimension() const
{
	int width = d_main_viewport_dimensions.first;
	int height = d_main_viewport_dimensions.second;
	if (width < height)
	{
		return width;
	}
	else
	{
		return height;
	}
}


int
GPlatesPresentation::ViewState::get_main_viewport_max_dimension() const
{
	int width = d_main_viewport_dimensions.first;
	int height = d_main_viewport_dimensions.second;
	if (width > height)
	{
		return width;
	}
	else
	{
		return height;
	}
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
}


void
GPlatesPresentation::ViewState::connect_to_viewport_zoom()
{
	// Handle zoom changes.
	QObject::connect(d_viewport_zoom.get(), SIGNAL(zoom_changed()),
			this, SLOT(handle_zoom_change()));
}


void
GPlatesPresentation::ViewState::connect_to_feature_focus()
{
	// If the focused feature is modified, we may need to reconstruct to update the view.
	QObject::connect(
			&get_feature_focus(),
			SIGNAL(focused_feature_modified(GPlatesGui::FeatureFocus &)),
			&get_application_state(),
			SLOT(reconstruct()));
}


QString &
GPlatesPresentation::ViewState::get_last_open_directory()
{
	return d_last_open_directory;
}


const QString &
GPlatesPresentation::ViewState::get_last_open_directory() const
{
	return d_last_open_directory;
}

bool
GPlatesPresentation::ViewState::get_show_stars() const
{
	return d_show_stars;
}


void
GPlatesPresentation::ViewState::set_show_stars(
		bool show_stars)
{
	d_show_stars = show_stars;

	// If the user changes the "Show Stars" setting, we remember it for next time.
	GPlatesAppLogic::UserPreferences &prefs = get_application_state().get_user_preferences();
	prefs.set_value("view/show_stars", d_show_stars);
}


const GPlatesGui::symbol_map_type &
GPlatesPresentation::ViewState::get_feature_type_symbol_map() const
{
    return d_feature_type_symbol_map;
}

GPlatesGui::symbol_map_type &
GPlatesPresentation::ViewState::get_feature_type_symbol_map()
{
    return d_feature_type_symbol_map;
}

const GPlatesGui::Colour &
GPlatesPresentation::ViewState::get_background_colour() const
{
	return *d_background_colour;
}


void
GPlatesPresentation::ViewState::set_background_colour(
		const GPlatesGui::Colour &colour)
{
	*d_background_colour = colour;
}


GPlatesGui::GraticuleSettings &
GPlatesPresentation::ViewState::get_graticule_settings()
{
	return *d_graticule_settings;
}


const GPlatesGui::GraticuleSettings &
GPlatesPresentation::ViewState::get_graticule_settings() const
{
	return *d_graticule_settings;
}


GPlatesGui::TextOverlaySettings &
GPlatesPresentation::ViewState::get_text_overlay_settings()
{
	return *d_text_overlay_settings;
}

const GPlatesGui::TextOverlaySettings &
GPlatesPresentation::ViewState::get_text_overlay_settings() const
{
	return *d_text_overlay_settings;
}

GPlatesGui::VelocityLegendOverlaySettings &
GPlatesPresentation::ViewState::get_velocity_legend_overlay_settings()
{
	return *d_velocity_legend_overlay_settings;
}

const GPlatesGui::VelocityLegendOverlaySettings &
GPlatesPresentation::ViewState::get_velocity_legend_overlay_settings() const
{
	return *d_velocity_legend_overlay_settings;
}


GPlatesGui::ExportAnimationRegistry &
GPlatesPresentation::ViewState::get_export_animation_registry()
{
	return *d_export_animation_registry;
}

const GPlatesGui::ExportAnimationRegistry &
GPlatesPresentation::ViewState::get_export_animation_registry() const
{
	return *d_export_animation_registry;
}


GPlatesGui::TopologySectionsContainer &
GPlatesPresentation::ViewState::get_topology_boundary_sections_container()
{
	return *d_topology_boundary_sections_container_ptr;	
}

const GPlatesGui::TopologySectionsContainer &
GPlatesPresentation::ViewState::get_topology_boundary_sections_container() const
{
	return *d_topology_boundary_sections_container_ptr;	
}


GPlatesGui::TopologySectionsContainer &
GPlatesPresentation::ViewState::get_topology_interior_sections_container()
{
	return *d_topology_interior_sections_container_ptr;
}

const GPlatesGui::TopologySectionsContainer &
GPlatesPresentation::ViewState::get_topology_interior_sections_container() const
{
	return *d_topology_interior_sections_container_ptr;
}


GPlatesGui::FileIODirectoryConfigurations &
GPlatesPresentation::ViewState::get_file_io_directory_configurations()
{
	return *d_file_io_directory_configurations;
}

const GPlatesGui::FileIODirectoryConfigurations &
GPlatesPresentation::ViewState::get_file_io_directory_configurations() const
{
	return *d_file_io_directory_configurations;
}
