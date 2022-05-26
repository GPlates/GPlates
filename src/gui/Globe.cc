/* $Id$ */

/**
 * @file 
 * File specific comments.
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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include <boost/utility/in_place_factory.hpp>
#include <QDebug>

#include "Globe.h"

#include "GlobeCamera.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/MathsUtils.h"

#include "opengl/GL.h"
#include "opengl/GLContext.h"
#include "opengl/GLLight.h"
#include "opengl/GLViewport.h"
#include "opengl/GLViewProjection.h"

#include "presentation/ViewState.h"

#include "view-operations/GlobeViewOperation.h"
#include "view-operations/RenderedGeometryCollection.h"


namespace
{
	const GPlatesGui::Colour STARS_COLOUR(0.75f, 0.75f, 0.75f);
}


GPlatesGui::Globe::Globe(
		GPlatesPresentation::ViewState &view_state,
		const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		const GPlatesPresentation::VisualLayers &visual_layers,
		int device_pixel_ratio) :
	d_view_state(view_state),
	d_gl_visual_layers(gl_visual_layers),
	d_rendered_geom_collection(rendered_geom_collection),
	d_visual_layers(visual_layers),
	d_rendered_geom_collection_painter(
			rendered_geom_collection,
			gl_visual_layers,
			visual_layers,
			device_pixel_ratio),
	d_device_pixel_ratio(device_pixel_ratio)
{  }


void
GPlatesGui::Globe::initialiseGL(
		GPlatesOpenGL::GL &gl)
{
	//
	// We now have a valid OpenGL context bound so we can initialise members that have OpenGL objects.
	//

	// Create these objects in place (some as non-copy-constructable).
	d_stars = boost::in_place(boost::ref(gl), boost::ref(d_view_state), STARS_COLOUR);
	d_background_sphere = boost::in_place(boost::ref(gl), boost::ref(d_view_state));
	d_grid = boost::in_place(boost::ref(gl), d_view_state.get_graticule_settings());

	// Initialise the rendered geometry collection painter.
	d_rendered_geom_collection_painter.initialise(gl);
}


GPlatesGui::Globe::cache_handle_type
GPlatesGui::Globe::paint(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		const GPlatesOpenGL::GLIntersect::Plane &front_globe_horizon_plane,
		const double &viewport_zoom_factor,
		float scale)
{
	// Make sure we leave the OpenGL global state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	// The cached view is a sequence of caches for the caller to keep alive until the next frame.
	boost::shared_ptr<std::vector<cache_handle_type> > cache_handle(new std::vector<cache_handle_type>());

	// Set up the scene lighting if lighting is supported.
	GPlatesOpenGL::GLMatrix globe_orientation_transform;
	get_globe_orientation_transform(globe_orientation_transform);
	set_scene_lighting(gl, globe_orientation_transform);

	// Render stars.
	// NOTE: We draw the stars first so they don't appear in front of the globe.
	render_stars(gl, view_projection);

	// Determine whether the globe is transparent or not. This happens if either:
	//  1) the background colour is transparent, or
	//  2) there are sub-surface geometries to render.

	// Is background colour transparent.
	const Colour original_background_colour = d_view_state.get_background_colour();
	bool has_transparent_background_colour =
			!GPlatesMaths::are_almost_exactly_equal(original_background_colour.alpha(), 1.0);

	// See if there's any sub-surface geometries (below globe's surface) to render.
	const bool have_sub_surface_geometries = d_rendered_geom_collection_painter.has_sub_surface_geometries(gl);

	// When rendering sub-surface geometries the sphere needs to be transparent so that the
	// rear of the globe is visible (to provide visual clues when rendering sub-surface geometries).
	if (have_sub_surface_geometries)
	{
		// If the sphere background colour is currently opaque then make it semi-transparent,
		// otherwise leave it as it is (the user has probably chosen an alpha value they like).
		if (!has_transparent_background_colour)
		{
			// Temporarily change the background colour in the view state.
			// This will get picked up by 'BackgroundSphere'.
			Colour transparent_background_colour = d_view_state.get_background_colour();
			transparent_background_colour.alpha() = 0.3f;
			d_view_state.set_background_colour(transparent_background_colour);

			has_transparent_background_colour = true;
		}
	}

	// Set up rendered geometry collection state.
	d_rendered_geom_collection_painter.set_scale(scale);

	// If the background colour is transparent then render the rear half of the globe.
	if (has_transparent_background_colour)
	{
		// The globe is transparent so render the rear half of the globe.
		render_globe_hemisphere_surface(
				gl,
				view_projection,
				front_globe_horizon_plane.get_reversed_half_space(),  // render rear of globe
				*cache_handle,
				viewport_zoom_factor,
				false/*is_front_half_globe*/);
	}

	// Render background sphere - can actually be transparent depending on the alpha value in its colour.
	render_sphere_background(gl, view_projection);

	// Render the globe sub-surface (if have any sub-surface geometries).
	//
	// Previously we first rendered front half of globe to a surface occlusion texture and then passed
	// that to sub-surface rendering so it could early terminate (where surface occludes sub-surface),
	// and finally blended that occlusion texture on top in a final pass.
	//
	// We no longer do that for two reasons:
	//
	// (1) Multisampling makes this difficult. While it's possible to render surface geometries
	//     (eg, filled polygons) to a multisample texture and then have isosurface read that texture
	//     and early terminate if *all* samples of a fragment are opaque - it's not possible to then
	//     draw/copy/blit that surface multisample texture into default framebuffer (which would also
	//     be multisample) and only have those the samples with opaque coverage copied across so that a
	//     subsequent multisample resolve of default framebuffer will blend the surface with the sub-surface
	//     (at edges of surface polygons). Instead you'd have to render the surface polygons again, but this
	//     time into the default framebuffer (to get proper multisample antialiasing). It's better to render
	//     isosurface (into default framebuffer) and then render front-half of globe into default framebuffer.
	//
	// (2) We might be rendering to an SVG renderer (instead of default framebuffer) and therefore we want
	//     the sub-surface data to be completely rendered (instead of only rendered where it's not occluded
	//     by surface data) in case user loads SVG file and removes surface polygons (to reveal hidden sub-surface).
	//     We also want things rendered in correct back-to-front depth order so that they are then drawn correctly
	//     by the SVG renderer (ie, sub-surface then surface) since SVG files are typically drawn back-to-front.
	//
	if (have_sub_surface_geometries)
	{
		render_globe_sub_surface(
				gl,
				view_projection,
				*cache_handle,
				viewport_zoom_factor);
	}

	// Render the front half of the globe surface.
	render_globe_hemisphere_surface(
			gl,
			view_projection,
			front_globe_horizon_plane,  // render front of globe
			*cache_handle,
			viewport_zoom_factor,
			true/*is_front_half_globe*/);

	// Restore the background colour if we temporarily changed it.
	if (original_background_colour != d_view_state.get_background_colour())
	{
		d_view_state.set_background_colour(original_background_colour);
	}

	return cache_handle;
}


void
GPlatesGui::Globe::get_globe_orientation_transform(
		GPlatesOpenGL::GLMatrix &transform) const
{
	// TODO: Change this to 'view' orientation and investigate the effect of view tilt.
	const GPlatesMaths::Rotation globe_orientation_relative_to_view =
			d_view_state.get_globe_camera().get_globe_orientation_relative_to_view();

	const GPlatesMaths::UnitVector3D &axis = globe_orientation_relative_to_view.axis();
	const GPlatesMaths::real_t angle_in_deg = GPlatesMaths::convert_rad_to_deg(
			globe_orientation_relative_to_view.angle());

	transform.gl_rotate(angle_in_deg.dval(), axis.x().dval(), axis.y().dval(), axis.z().dval());
}


void
GPlatesGui::Globe::set_scene_lighting(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLMatrix &view_orientation)
{
	// Get the OpenGL light if the runtime system supports it.
	boost::optional<GPlatesOpenGL::GLLight::non_null_ptr_type> gl_light =
			d_gl_visual_layers->get_light(gl);

	// Set the scene lighting parameters on the light (includes the current view orientation).
	if (gl_light)
	{
		gl_light.get()->set_scene_lighting(
				gl,
				d_view_state.get_scene_lighting_parameters(),
				view_orientation);
	}
}


void
GPlatesGui::Globe::render_stars(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state_scope(gl);

	// Disable depth testing and depth writes.
	// Stars are rendered in the background and don't really need depth sorting.
	gl.Disable(GL_DEPTH_TEST);
	gl.DepthMask(GL_FALSE);

	d_stars->paint(gl, view_projection, d_device_pixel_ratio);
}


void
GPlatesGui::Globe::render_sphere_background(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection)
{
	GPlatesOpenGL::GL::StateScope save_restore_state_scope(gl);

	// Disable depth testing and depth writes.
	// The background sphere is only used to render the sphere colour (and, for a transparent sphere,
	// blend the sphere colour with the rendered geometries on the rear of the globe).
	gl.Disable(GL_DEPTH_TEST);
	const bool enable_depth_writes = false;
	gl.DepthMask(enable_depth_writes);

	// Note that if the sphere is transparent it will cause objects rendered to the rear half
	// of the globe to be dimmer than normal due to alpha blending (this is the intended effect).
	d_background_sphere->paint(gl, view_projection, enable_depth_writes);
}


void
GPlatesGui::Globe::render_globe_hemisphere_surface(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		const GPlatesOpenGL::GLIntersect::Plane &globe_horizon_plane,
		std::vector<cache_handle_type> &cache_handle,
		const double &viewport_zoom_factor,
		bool is_front_half_globe)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state_scope(gl);

	// Enable depth testing but disable depth writes (for the grid lines, the globe sets its own state).
	gl.Enable(GL_DEPTH_TEST);
	gl.DepthMask(GL_FALSE);

	if (!is_front_half_globe)
	{
		// Render the grid lines on the rear side of the sphere first.
		d_grid->paint(gl, view_projection, globe_horizon_plane);
	}

	// Colour vector geometries on the rear of the globe the same colour as background so that
	// they're less intrusive to the geometries on the front of the globe.
	boost::optional<Colour> vector_geometries_override_colour;
	if (!is_front_half_globe)
	{
		// Note that the background colour may include transparency, in which case the vector
		// geometries will become doubly-transparent due to rendering sphere background
		// (translucent sphere) that also uses background colour.
		// But it produces a nice subtle effect to the rear geometries and also means the
		// rear geometries will disappear when the background alpha approaches zero, which is
		// useful if user wants to remove the rear geometries.
		vector_geometries_override_colour = d_view_state.get_background_colour();
	}

	// Draw the rendered geometries.
	// Draw in reverse order if drawing to the rear half of the globe.
	d_rendered_geom_collection_painter.set_visual_layers_reversed(!is_front_half_globe);
	const cache_handle_type rendered_geoms_cache_half_globe =
			d_rendered_geom_collection_painter.paint_surface(
					gl,
					view_projection,
					globe_horizon_plane,
					viewport_zoom_factor,
					vector_geometries_override_colour);
	cache_handle.push_back(rendered_geoms_cache_half_globe);

	if (is_front_half_globe)
	{
		// Render the grid lines on the front side of the sphere last.
		d_grid->paint(gl, view_projection, globe_horizon_plane);
	}
}


void
GPlatesGui::Globe::render_globe_sub_surface(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		std::vector<cache_handle_type> &cache_handle,
		const double &viewport_zoom_factor)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state_scope(gl);

	// Draw the sub-surface geometries.
	// Draw in normal layer order.
	d_rendered_geom_collection_painter.set_visual_layers_reversed(false);
	const cache_handle_type rendered_geoms_cache_sub_surface_globe =
			d_rendered_geom_collection_painter.paint_sub_surface(
					gl,
					view_projection,
					viewport_zoom_factor,
					// Set to true when active globe canvas tool is currently in a
					// mouse drag operation that changes the view.
					// All globe canvas tools share the sole globe view operation...
					d_view_state.get_globe_view_operation().in_drag()/*improve_performance_reduce_quality_hint*/);
	cache_handle.push_back(rendered_geoms_cache_sub_surface_globe);
}
