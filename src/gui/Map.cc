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
#include <boost/utility/in_place_factory.hpp>
#include <opengl/OpenGL.h>

#include "Map.h"

#include "Colour.h"
#include "GraticuleSettings.h"
#include "MapRenderedGeometryCollectionPainter.h"

#include "maths/LatLonPoint.h"
#include "maths/Real.h"

#include "opengl/GLRenderer.h"

#include "utils/Profile.h"


GPlatesGui::Map::Map(
		GPlatesPresentation::ViewState &view_state,
		const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		const GPlatesPresentation::VisualLayers &visual_layers,
		ViewportZoom &viewport_zoom,
		const ColourScheme::non_null_ptr_type &colour_scheme,
		int device_pixel_ratio) :
	d_map_projection(MapProjection::create()),
	d_view_state(view_state),
	d_gl_visual_layers(gl_visual_layers),
	d_rendered_geometry_collection(&rendered_geometry_collection),
	d_visual_layers(visual_layers),
	d_viewport_zoom(viewport_zoom),
	d_colour_scheme(colour_scheme),
	d_rendered_geom_collection_painter(
			d_map_projection,
			rendered_geometry_collection,
			gl_visual_layers,
			visual_layers,
			colour_scheme,
			device_pixel_ratio)
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
	d_background = boost::in_place(boost::ref(renderer), *d_map_projection, boost::ref(d_view_state));

	// Initialise the rendered geometry collection painter.
	d_rendered_geom_collection_painter.initialise(renderer);
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
	return d_map_projection->central_meridian();
}


void
GPlatesGui::Map::set_central_meridian(
		double central_meridian_)
{
	d_map_projection->set_central_meridian(central_meridian_);
}


GPlatesGui::Map::cache_handle_type
GPlatesGui::Map::paint(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &viewport_zoom_factor,
		const double &device_independent_pixel_to_map_space_ratio,
		float scale)
{
	cache_handle_type cache_handle;

	try
	{
		// Get the OpenGL light if the runtime system supports it.
		boost::optional<GPlatesOpenGL::GLLight::non_null_ptr_type> gl_light =
				d_gl_visual_layers->get_light(renderer);
		// Set the scene lighting parameters on the light.
		if (gl_light)
		{
			gl_light.get()->set_scene_lighting(
					renderer,
					d_view_state.get_scene_lighting_parameters(),
					renderer.gl_get_matrix(GL_MODELVIEW)/*view_orientation*/,
					MapProjection::non_null_ptr_to_const_type(d_map_projection));
		}

		// Clear the depth buffer of the framebuffer.
		//
		// NOTE: We don't use the depth buffer in the map view but clear it anyway so that we can
		// use common layer painting code with the 3D globe rendering that enables depth testing.
		// In our case the depth testing will always return true - depth testing is very fast
		// in modern graphics hardware so we don't need to optimise it away.
		// We also clear the stencil buffer in case it is used - also it's usually interleaved
		// with depth so it's more efficient to clear both depth and stencil.
		renderer.gl_clear_depth(); // Clear depth to 1.0
		renderer.gl_clear_stencil();
		renderer.gl_clear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		// Set the scale factor.
		d_rendered_geom_collection_painter.set_scale(scale);

		// Render the background of the map.
		d_background->paint(renderer);

		// Render the rendered geometry layers onto the map.
		cache_handle = d_rendered_geom_collection_painter.paint(
				renderer,
				viewport_zoom_factor,
				device_independent_pixel_to_map_space_ratio);

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
