/* $Id$ */

/**
 * @file 
 * Contains the implementation of the Map class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 The University of Sydney, Australia
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

#include <exception>
#include <iostream>
#include <vector>
#include <utility>
#include <opengl/OpenGL.h>

#include "Map.h"

#include "Colour.h"
#include "GraticuleSettings.h"
#include "MapRenderedGeometryCollectionPainter.h"

#include "maths/LatLonPoint.h"
#include "maths/Real.h"

#include "opengl/GLRenderer.h"


GPlatesGui::Map::Map(
		GPlatesPresentation::ViewState &view_state,
		const PersistentOpenGLObjects::non_null_ptr_type &persistent_opengl_objects,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		const GPlatesPresentation::VisualLayers &visual_layers,
		RenderSettings &render_settings,
		ViewportZoom &viewport_zoom,
		const ColourScheme::non_null_ptr_type &colour_scheme,
		const TextRenderer::non_null_ptr_to_const_type &text_renderer) :
	d_map_projection(MapProjection::create()),
	d_view_state(view_state),
	d_rendered_geometry_collection(&rendered_geometry_collection),
	d_visual_layers(visual_layers),
	d_render_settings(render_settings),
	d_viewport_zoom(viewport_zoom),
	d_colour_scheme(colour_scheme),
	d_text_renderer_ptr(text_renderer),
	d_rendered_geom_collection_painter(
			d_map_projection,
			rendered_geometry_collection,
			persistent_opengl_objects,
			visual_layers,
			d_render_settings,
			text_renderer,
			colour_scheme)
{  }



void
GPlatesGui::Map::initialiseGL(
		GPlatesOpenGL::GLRenderer &renderer)
{
	//
	// We now have a valid OpenGL context bound so we can initialise members that have OpenGL objects.
	//

	// Create these objects in place (some as non-copy-constructable).
	d_grid = boost::in_place(boost::ref(renderer), *d_map_projection, d_view_state.get_graticule_settings());
}


GPlatesGui::MapProjection &
GPlatesGui::Map::projection()
{
	return *d_map_projection;
}


const GPlatesGui::MapProjection &
GPlatesGui::Map::projection() const
{
	return *d_map_projection;
}


GPlatesGui::MapProjection::Type
GPlatesGui::Map::projection_type() const
{
	return d_map_projection->projection_type();
}


void
GPlatesGui::Map::set_projection_type(
		GPlatesGui::MapProjection::Type projection_type_)
{
	d_map_projection->set_projection_type(projection_type_);
}


double
GPlatesGui::Map::central_meridian()
{
	return d_map_projection->central_llp().longitude();
}


void
GPlatesGui::Map::set_central_meridian(
		double central_meridian_)
{
	GPlatesMaths::LatLonPoint llp(0.0, central_meridian_);
	d_map_projection->set_central_llp(llp);
}


GPlatesGui::Map::cache_handle_type
GPlatesGui::Map::paint(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &viewport_zoom_factor,
		float scale)
{
	cache_handle_type cache_handle;

	try
	{
		// Draw the background colour and clear the depth buffer of the main framebuffer.
		// TODO: Only draw the map area in the background colour (not the entire viewport).
		//
		// NOTE: We don't use the depth buffer in the map view but clear it anyway because
		// so we can use common code with the 3D globe rendering that enables depth testing.
		// In our case the depth testing will always return true - depth testing is very faster
		// in modern graphics hardware so we don't need to optimise it away.
		const GPlatesGui::Colour &colour = d_view_state.get_background_colour();
		renderer.gl_clear_color(colour.red(), colour.green(), colour.blue(), 1.0f);
		renderer.gl_clear_depth(); // Clear depth to 1.0
		renderer.gl_clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Set the scale factor.
		d_rendered_geom_collection_painter.set_scale(scale);

		// Render the rendered geometry layers onto the map.
		cache_handle = d_rendered_geom_collection_painter.paint(renderer, viewport_zoom_factor);

		// Render the grid lines on the map.
		d_grid->paint(renderer);
	}
	catch (const GPlatesGui::ProjectionException &exc)
	{
		// Ignore.
		qWarning() << exc;
	}

	return cache_handle;
}
