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
#include "opengl/GLScreenRenderTarget.h"
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
		const GlobeVisibilityTester &visibility_tester) :
	d_view_state(view_state),
	d_gl_visual_layers(gl_visual_layers),
	d_rendered_geom_collection(rendered_geom_collection),
	d_visual_layers(visual_layers),
	d_rendered_geom_collection_painter(
			rendered_geom_collection,
			gl_visual_layers,
			visual_layers,
			visibility_tester)
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
	const bool render_sub_surface_geometries =
			d_rendered_geom_collection_painter.has_renderable_sub_surface_geometries(gl);
	// When rendering sub-surface geometries the sphere needs to be transparent so that the
	// rear of the globe is visible (to provide visual clues when rendering sub-surface geometries).
	if (render_sub_surface_geometries)
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
				*cache_handle,
				viewport_zoom_factor,
				false/*is_front_half_globe*/);
	}

	// Render background sphere - can actually be transparent depending on the alpha value in its colour.
	render_sphere_background(gl, view_projection);

	// The rendering of sub-surface geometries and the front surface of the globe are intermingled
	// because the latter can be used to occlude the former.
	if (render_sub_surface_geometries)
	{
		//
		// See if we can render the front half of the globe as a surface occlusion texture for sub-surface geometries.
		// This reduces the workload of rendering sub-surface data that is occluded by surface data.
		//
		boost::optional<GPlatesOpenGL::GLScreenRenderTarget::shared_ptr_type> screen_render_target =
				gl.get_context().get_non_shared_state()->acquire_screen_render_target(
						gl,
						GL_RGBA8/*texture_internalformat*/,
						true/*include_depth_buffer*/,
						// Enable stencil buffer for filled surface polygons...
						true/*include_stencil_buffer*/);

		if (screen_render_target &&
			// If we're not rendering directly to the framebuffer (because we're rendering to a feedback
			// QPainter device) then just render normally because we want the sub-surface data to be
			// completely rendered (instead of only rendered where it's not occluded by surface data).
			// We also want things rendered in correct back-to-front depth order so that they are then
			// drawn correctly by an external SVG renderer for example (ie, sub-surface then surface)...
			gl.rendering_to_context_framebuffer())
		{
			// Prepare for rendering the front half of the globe into a viewport-size render texture.
			// This will be used as a surface occlusion texture when rendering the globe sub-surface
			// in order to early terminate occluded volume-tracing rays for efficient sub-surface rendering.
			const GPlatesOpenGL::GLViewport screen_viewport = gl.get_viewport();
			GPlatesOpenGL::GLScreenRenderTarget::RenderScope screen_render_target_scope(
					*screen_render_target.get(),
					gl,
					screen_viewport.width(),
					screen_viewport.height());

			// Save/restore the OpenGL state changed over the scope of the screen render target.
			GPlatesOpenGL::GL::StateScope save_restore_screen_render_target_scope(gl);

			// Set the new viewport in case the current viewport has non-zero x and y offsets which
			// happens when the scene is rendered as overlapping tiles (for rendering very large images).
			// It's also important that, later when accessing the screen render texture, the NDC
			// coordinates (-1,-1) and (1,1) map to the corners of the screen render texture.
			gl.Viewport(0, 0, screen_viewport.width(), screen_viewport.height());
			// Also change the scissor rectangle in case scissoring is enabled.
			gl.Scissor(0, 0, screen_viewport.width(), screen_viewport.height());

			// Clear the render target (colour and depth) before rendering the image.
			// We also clear the stencil buffer in case it is used - also it's usually interleaved
			// with depth so it's more efficient to clear both depth and stencil.
			gl.ClearColor(); // Clear to transparent (alpha=0) so regions not rendered are transparent.
			gl.ClearDepth(); // Clear depth to 1.0
			gl.ClearStencil(); // Clear stencil to 0
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

			// Render the front half of the globe surface to the viewport-size render texture.
			render_globe_hemisphere_surface(
					gl,
					view_projection,
					*cache_handle,
					viewport_zoom_factor,
					true/*is_front_half_globe*/);

			// Finished rendering surface occlusion texture.
			save_restore_screen_render_target_scope.end_state_block();
			screen_render_target_scope.end_render();
			// Get the texture just rendered to.
			GPlatesOpenGL::GLTexture::shared_ptr_to_const_type front_globe_surface_texture =
					screen_render_target.get()->get_texture();

			// Render the globe sub-surface (using the surface occlusion texture) to the main framebuffer.
			// Note that this must use the original viewport.
			render_globe_sub_surface(
					gl,
					*cache_handle,
					viewport_zoom_factor,
					front_globe_surface_texture);

			// Blend the front half of the globe surface (the surface occlusion texture) in the main framebuffer.
			// Note that this must use the original viewport.
			render_front_globe_hemisphere_surface_texture(
					gl,
					front_globe_surface_texture);
		}
		else // surface occlusion not supported so render sub-surface followed by surface...
		{
			// Render the globe sub-surface first.
			render_globe_sub_surface(
					gl,
					*cache_handle,
					viewport_zoom_factor);

			// Render the front half of the globe surface next.
			render_globe_hemisphere_surface(
					gl,
					view_projection,
					*cache_handle,
					viewport_zoom_factor,
					true/*is_front_half_globe*/);
		}
	}
	else // there are no sub-surface geometries...
	{
		// Render the front half of the globe surface.
		render_globe_hemisphere_surface(
				gl,
				view_projection,
				*cache_handle,
				viewport_zoom_factor,
				true/*is_front_half_globe*/);
	}

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

	d_stars->paint(gl, view_projection);
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
		d_grid->paint(gl, view_projection);
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
					viewport_zoom_factor,
					vector_geometries_override_colour);
	cache_handle.push_back(rendered_geoms_cache_half_globe);

	if (is_front_half_globe)
	{
		// Render the grid lines on the front side of the sphere last.
		d_grid->paint(gl, view_projection);
	}
}


void
GPlatesGui::Globe::render_front_globe_hemisphere_surface_texture(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLTexture::shared_ptr_to_const_type &front_globe_surface_texture)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state_scope(gl);

	// Used to draw a full-screen quad into render texture.
	const GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type full_screen_quad_drawable =
			gl.get_context().get_shared_state()->get_full_screen_quad(gl);

	// Full-screen rendering requires no model-view-projection transform.
	gl.gl_load_matrix(GL_MODELVIEW, GPlatesOpenGL::GLMatrix::IDENTITY);
	gl.gl_load_matrix(GL_PROJECTION, GPlatesOpenGL::GLMatrix::IDENTITY);

	// Set up alpha blending for pre-multiplied alpha.
	// This has (src,dst) blend factors of (1, 1-src_alpha) instead of (src_alpha, 1-src_alpha).
	// This is where the RGB channels have already been multiplied by the alpha channel.
	// This necessary in order to be able to blend a render texture into the main framebuffer and
	// have it look the same as if we had blended directly into the main framebuffer in the first place.
	// However everything rendered into the render target must use pre-multiplied alpha.
	gl.Enable(GL_BLEND);
	gl.BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	// Enable alpha testing as an optimisation for culling transparent raster pixels.
	gl.Enable(GL_ALPHA_TEST);
	gl.gl_alpha_func(GL_GREATER, GLclampf(0));

	// Disable depth testing and depth writes - we're just doing a full-screen copy and blend.
	gl.Disable(GL_DEPTH_TEST);
	gl.DepthMask(GL_FALSE);

	// Bind the texture and enable fixed-function texturing on texture unit 0.
	const GLenum texture_unit_0 = gl.get_capabilities().texture.gl_TEXTURE0;
	gl.ActiveTexture(GL_TEXTURE0);
	gl.BindTexture(GL_TEXTURE_2D, front_globe_surface_texture);
	gl.gl_enable_texture(texture_unit_0, GL_TEXTURE_2D);
	gl.gl_tex_env(texture_unit_0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	// Render the full-screen quad.
	gl.apply_compiled_draw_state(*full_screen_quad_drawable);
}


void
GPlatesGui::Globe::render_globe_sub_surface(
		GPlatesOpenGL::GL &gl,
		std::vector<cache_handle_type> &cache_handle,
		const double &viewport_zoom_factor,
		boost::optional<GPlatesOpenGL::GLTexture::shared_ptr_to_const_type> surface_occlusion_texture)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state_scope(gl);

	// Draw the sub-surface geometries.
	// Draw in normal layer order.
	d_rendered_geom_collection_painter.set_visual_layers_reversed(false);
	const cache_handle_type rendered_geoms_cache_sub_surface_globe =
			d_rendered_geom_collection_painter.paint_sub_surface(
					gl,
					viewport_zoom_factor,
					surface_occlusion_texture,
					// Set to true when active globe canvas tool is currently in a
					// mouse drag operation that changes the view.
					// All globe canvas tools share the sole globe view operation...
					d_view_state.get_globe_view_operation().in_drag()/*improve_performance_reduce_quality_hint*/);
	cache_handle.push_back(rendered_geoms_cache_sub_surface_globe);
}
