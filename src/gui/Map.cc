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

#include "Map.h"

#include "Colour.h"
#include "GraticuleSettings.h"
#include "MapRenderedGeometryCollectionPainter.h"

#include "maths/LatLonPoint.h"
#include "maths/Real.h"

#include "opengl/GLViewProjection.h"

#include "utils/Profile.h"


GPlatesGui::Map::Map(
		GPlatesPresentation::ViewState &view_state,
		const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		const GPlatesPresentation::VisualLayers &visual_layers,
		int device_pixel_ratio) :
	d_view_state(view_state),
	d_gl_visual_layers(gl_visual_layers),
	d_rendered_geometry_collection(&rendered_geometry_collection),
	d_visual_layers(visual_layers),
	d_rendered_geom_collection_painter(
			view_state.get_map_projection(),
			rendered_geometry_collection,
			gl_visual_layers,
			visual_layers,
			device_pixel_ratio)
{  }



void
GPlatesGui::Map::initialiseGL(
		GPlatesOpenGL::GL &gl)
{
	//
	// We now have a valid OpenGL context bound so we can initialise members that have OpenGL objects.
	//

	// Create these objects in place (some as non-copy-constructable).
	d_grid = boost::in_place(boost::ref(gl), d_view_state.get_map_projection(), d_view_state.get_graticule_settings());
	d_background = boost::in_place(boost::ref(gl), d_view_state.get_map_projection(), boost::ref(d_view_state));

	// Initialise the rendered geometry collection painter.
	d_rendered_geom_collection_painter.initialise(gl);
}


GPlatesGui::Map::cache_handle_type
GPlatesGui::Map::paint(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		const double &viewport_zoom_factor,
		float scale)
{
	cache_handle_type cache_handle;

	try
	{
		// Get the OpenGL light if the runtime system supports it.
		boost::optional<GPlatesOpenGL::GLLight::non_null_ptr_type> gl_light =
				d_gl_visual_layers->get_light(gl);
		// Set the scene lighting parameters on the light.
		if (gl_light)
		{
			gl_light.get()->set_scene_lighting(
					gl,
					d_view_state.get_scene_lighting_parameters(),
					view_projection.get_view_transform()/*view_orientation*/,
					d_view_state.get_map_projection());
		}

		// Set the scale factor.
		d_rendered_geom_collection_painter.set_scale(scale);

		// Render the background of the map.
		d_background->paint(gl, view_projection);

		// Render the rendered geometry layers onto the map.
		cache_handle = d_rendered_geom_collection_painter.paint(gl, view_projection, viewport_zoom_factor);

		// Render the grid lines on the map.
		d_grid->paint(gl, view_projection);
	}
	catch (const GPlatesGui::ProjectionException &exc)
	{
		// Ignore.
		qWarning() << exc;
	}

	return cache_handle;
}
