/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "VisualLayerRegistry.h"
#include "VisualLayers.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/VGPRenderSettings.h"

#include "file-io/CptReader.h"
#include "file-io/ReadErrorAccumulation.h"
#include "file-io/TemporaryFileRegistry.h"

#include "gui/ColourSchemeContainer.h"
#include "gui/ColourSchemeDelegator.h"
#include "gui/ColourPaletteAdapter.h"
#include "gui/CptColourPalette.h"
#include "gui/FeatureFocus.h"
#include "gui/GeometryFocusHighlight.h"
#include "gui/GraticuleSettings.h"
#include "gui/MapTransform.h"
#include "gui/RasterColourPalette.h"
#include "gui/RasterColourSchemeMap.h"
#include "gui/RenderSettings.h"
#include "gui/ViewportProjection.h"
#include "gui/ViewportZoom.h"

#include "maths/MathsUtils.h"
#include "maths/Real.h"

#include "view-operations/RenderedGeometryCollection.h"


namespace
{
	const double DEFAULT_GRATICULES_DELTA_LAT = GPlatesMaths::PI / 6.0; // 30 degrees
	const double DEFAULT_GRATICULES_DELTA_LON = GPlatesMaths::PI / 6.0; // 30 degrees

	GPlatesGui::Colour
	get_default_background_colour()
	{
		return GPlatesGui::Colour(0.35f, 0.35f, 0.35f);
	}

	GPlatesGui::Colour
	get_default_graticules_colour()
	{
		GPlatesGui::Colour result = GPlatesGui::Colour::get_silver();
		result.alpha() = 0.5f;
		return result;
	}
}


GPlatesPresentation::ViewState::ViewState(
		GPlatesAppLogic::ApplicationState &application_state) :
	d_application_state(application_state),
	d_other_view_state(NULL), // FIXME: remove this when refactored
	d_rendered_geometry_collection(
			new GPlatesViewOperations::RenderedGeometryCollection()),
	d_colour_scheme_container(
			new GPlatesGui::ColourSchemeContainer(*this)),
	d_colour_scheme(
			new GPlatesGui::ColourSchemeDelegator(*d_colour_scheme_container)),
	d_viewport_zoom(
			new GPlatesGui::ViewportZoom()),
	d_viewport_projection(
			new GPlatesGui::ViewportProjection(GPlatesGui::ORTHOGRAPHIC)),
	d_geometry_focus_highlight(
			new GPlatesGui::GeometryFocusHighlight(*d_rendered_geometry_collection)),
	d_feature_focus(
			new GPlatesGui::FeatureFocus(application_state)),
	d_visual_layers(
			new VisualLayers(
				d_application_state,
				*d_rendered_geometry_collection)),
	d_visual_layer_registry(
			new VisualLayerRegistry()),
	d_render_settings(
			new GPlatesGui::RenderSettings()),
	d_map_transform(
			new GPlatesGui::MapTransform(
				*d_viewport_zoom)),
	d_main_viewport_dimensions(0, 0),
	d_vgp_render_settings(
			GPlatesAppLogic::VGPRenderSettings::instance()),
	d_raster_colour_scheme_map(
			new GPlatesGui::RasterColourSchemeMap(
				application_state.get_reconstruct_graph())),
	d_open_file_path(QDir::currentPath()),
	d_show_stars(true),
	d_background_colour(
			new GPlatesGui::Colour(get_default_background_colour())),
	d_graticule_settings(
			new GPlatesGui::GraticuleSettings(
				DEFAULT_GRATICULES_DELTA_LAT,
				DEFAULT_GRATICULES_DELTA_LON,
				get_default_graticules_colour()))

{
	connect_to_viewport_zoom();

	// Connect to signals from FeatureFocus.
	connect_to_feature_focus();

	connect_to_raster_colour_scheme_map();

	// Set up RenderedGeometryCollection.
	setup_rendered_geometry_collection();

	// Set up VisualLayerRegistry.
	register_default_visual_layers(
			*d_visual_layer_registry,
			d_application_state,
			*this);
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


GPlatesGui::ColourSchemeContainer &
GPlatesPresentation::ViewState::get_colour_scheme_container()
{
	return *d_colour_scheme_container;
}


GPlatesGlobal::PointerTraits<GPlatesGui::ColourScheme>::non_null_ptr_type
GPlatesPresentation::ViewState::get_colour_scheme()
{
	return d_colour_scheme;
}


GPlatesGlobal::PointerTraits<GPlatesGui::ColourSchemeDelegator>::non_null_ptr_type
GPlatesPresentation::ViewState::get_colour_scheme_delegator()
{
	return d_colour_scheme;
}


GPlatesPresentation::VisualLayers &
GPlatesPresentation::ViewState::get_visual_layers()
{
	return *d_visual_layers;
}


GPlatesPresentation::VisualLayerRegistry &
GPlatesPresentation::ViewState::get_visual_layer_registry()
{
	return *d_visual_layer_registry;
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


		
	d_rendered_geometry_collection->set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::SMALL_CIRCLE_LAYER);	
		
	
			

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
GPlatesPresentation::ViewState::connect_to_feature_focus()
{
	// If the focused feature is modified, we may need to reconstruct to update the view.
	// FIXME:  If the FeatureFocus emits the 'focused_feature_modified' signal, the view will
	// be reconstructed twice -- once here, and once as a result of the 'set_focus' slot in the
	// GeometryFocusHighlight below.
	QObject::connect(
			&get_feature_focus(),
			SIGNAL(focused_feature_modified(GPlatesGui::FeatureFocus &)),
			&get_application_state(),
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


GPlatesAppLogic::VGPRenderSettings &
GPlatesPresentation::ViewState::get_vgp_render_settings()
{
	return *d_vgp_render_settings;
}


GPlatesGui::RasterColourSchemeMap &
GPlatesPresentation::ViewState::get_raster_colour_scheme_map()
{
	return *d_raster_colour_scheme_map;
}


const GPlatesGui::RasterColourSchemeMap &
GPlatesPresentation::ViewState::get_raster_colour_scheme_map() const
{
	return *d_raster_colour_scheme_map;
}


void
GPlatesPresentation::ViewState::connect_to_raster_colour_scheme_map()
{
	QObject::connect(
			d_raster_colour_scheme_map.get(),
			SIGNAL(colour_scheme_changed(const GPlatesAppLogic::Layer &)),
			&d_application_state,
			SLOT(reconstruct()));
}


QString &
GPlatesPresentation::ViewState::get_open_file_path()
{
	return d_open_file_path;
}


const QString &
GPlatesPresentation::ViewState::get_open_file_path() const
{
	return d_open_file_path;
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

