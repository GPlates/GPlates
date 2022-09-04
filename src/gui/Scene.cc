/**
 * Copyright (C) 2022 The University of Sydney, Australia
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

#include "Scene.h"

#include "presentation/ViewState.h"


GPlatesGui::Scene::Scene(
		GPlatesPresentation::ViewState &view_state,
		int device_pixel_ratio) :
	d_gl_visual_layers(
			GPlatesOpenGL::GLVisualLayers::create(
					view_state.get_application_state())),
	d_globe(
			view_state,
			d_gl_visual_layers,
			view_state.get_rendered_geometry_collection(),
			view_state.get_visual_layers(),
			device_pixel_ratio),
	d_map(
			view_state,
			d_gl_visual_layers,
			view_state.get_rendered_geometry_collection(),
			view_state.get_visual_layers(),
			device_pixel_ratio)
{
}


void
GPlatesGui::Scene::initialise_gl(
		GPlatesOpenGL::GL &gl)
{
	d_gl_visual_layers->initialise_gl(gl);

	d_globe.initialise_gl(gl);
	d_map.initialise_gl(gl);
}


void
GPlatesGui::Scene::shutdown_gl(
		GPlatesOpenGL::GL &gl)
{
	d_globe.shutdown_gl(gl);
	d_map.shutdown_gl(gl);

	d_gl_visual_layers->shutdown_gl(gl);
}


GPlatesGui::Scene::cache_handle_type
GPlatesGui::Scene::render_globe(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		const double &viewport_zoom_factor,
		const GPlatesOpenGL::GLIntersect::Plane &front_globe_horizon_plane)
{
	return d_globe.paint(gl, view_projection, viewport_zoom_factor, front_globe_horizon_plane);
}


GPlatesGui::Scene::cache_handle_type
GPlatesGui::Scene::render_map(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		const double &viewport_zoom_factor)
{
	return d_map.paint(gl, view_projection,  viewport_zoom_factor);
}
