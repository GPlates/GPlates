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

#include <boost/utility/in_place_factory.hpp>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "Globe.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/MathsUtils.h"

#include "opengl/GLCompiledDrawState.h"
#include "opengl/GLContext.h"
#include "opengl/GLLight.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLScreenRenderTarget.h"
#include "opengl/GLViewport.h"

#include "presentation/ViewState.h"

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
		const GlobeVisibilityTester &visibility_tester,
		ColourScheme::non_null_ptr_type colour_scheme,
		int device_pixel_ratio) :
	d_view_state(view_state),
	d_gl_visual_layers(gl_visual_layers),
	d_rendered_geom_collection(rendered_geom_collection),
	d_visual_layers(visual_layers),
	d_globe_orientation_ptr(new SimpleGlobeOrientation()),
	d_globe_orientation_changing_during_mouse_drag(false),
	d_rendered_geom_collection_painter(
			rendered_geom_collection,
			gl_visual_layers,
			visual_layers,
			visibility_tester,
			colour_scheme,
			device_pixel_ratio),
	d_device_pixel_ratio(device_pixel_ratio)
{  }


GPlatesGui::Globe::Globe(
		Globe &existing_globe,
		const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
		const GlobeVisibilityTester &visibility_tester,
		ColourScheme::non_null_ptr_type colour_scheme,
		int device_pixel_ratio) :
	d_view_state(existing_globe.d_view_state),
	d_gl_visual_layers(gl_visual_layers),
	d_rendered_geom_collection(existing_globe.d_rendered_geom_collection),
	d_visual_layers(existing_globe.d_visual_layers),
	d_globe_orientation_ptr(existing_globe.d_globe_orientation_ptr),
	d_globe_orientation_changing_during_mouse_drag(false),
	d_rendered_geom_collection_painter(
			d_rendered_geom_collection,
			gl_visual_layers,
			d_visual_layers,
			visibility_tester,
			colour_scheme,
			device_pixel_ratio),
	d_device_pixel_ratio(device_pixel_ratio)
{  }


void
GPlatesGui::Globe::set_new_handle_pos(
		const GPlatesMaths::PointOnSphere &pos)
{
	d_globe_orientation_ptr->set_new_handle_at_pos(pos);
}


void
GPlatesGui::Globe::update_handle_pos(
		const GPlatesMaths::PointOnSphere &pos,
		bool in_mouse_drag)
{
	// Gets set to true when mouse button (left) is pressed (during drag) and reset to false when released.
	d_globe_orientation_changing_during_mouse_drag = in_mouse_drag;

	d_globe_orientation_ptr->move_handle_to_pos(pos);
}


const GPlatesMaths::PointOnSphere
GPlatesGui::Globe::orient(
		const GPlatesMaths::PointOnSphere &pos) const
{
	return d_globe_orientation_ptr->reverse_orient_point(pos);
}


void
GPlatesGui::Globe::initialiseGL(
		GPlatesOpenGL::GLRenderer &renderer)
{
	//
	// We now have a valid OpenGL context bound so we can initialise members that have OpenGL objects.
	//

	// Create these objects in place (some as non-copy-constructable).
	d_stars = boost::in_place(boost::ref(renderer), boost::ref(d_view_state), STARS_COLOUR, d_device_pixel_ratio);
	d_sphere = boost::in_place(boost::ref(renderer), boost::ref(d_view_state));
	d_grid = boost::in_place(boost::ref(renderer), d_view_state.get_graticule_settings());

	// Initialise the rendered geometry collection painter.
	d_rendered_geom_collection_painter.initialise(renderer);
}


GPlatesGui::Globe::cache_handle_type
GPlatesGui::Globe::paint(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &viewport_zoom_factor,
		const double &device_independent_pixel_to_world_space_ratio,
		float scale,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_front_half_globe,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_rear_half_globe,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_full_globe,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_stars)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_globe_state_scope(renderer);

	// The cached view is a sequence of caches for the caller to keep alive until the next frame.
	boost::shared_ptr<std::vector<cache_handle_type> > cache_handle(new std::vector<cache_handle_type>());

	// Set up the globe orientation transform.
	GPlatesOpenGL::GLMatrix globe_orientation_transform;
	get_globe_orientation_transform(globe_orientation_transform);
	renderer.gl_mult_matrix(GL_MODELVIEW, globe_orientation_transform);

	// Set up the scene lighting if lighting is supported.
	set_scene_lighting(renderer, globe_orientation_transform);

	// Render stars.
	// NOTE: We draw the stars first so they don't appear in front of the globe.
	render_stars(renderer, projection_transform_include_stars);

	// Determine whether the globe is transparent or not. This happens if either:
	//  1) the background colour is transparent, or
	//  2) there are sub-surface geometries to renderer.

	// Is background colour transparent.
	const Colour original_background_colour = d_view_state.get_background_colour();
	bool has_transparent_background_colour =
			!GPlatesMaths::are_almost_exactly_equal(original_background_colour.alpha(), 1.0);

	// See if there's any sub-surface geometries (below globe's surface) to render.
	const bool render_sub_surface_geometries =
			d_rendered_geom_collection_painter.has_renderable_sub_surface_geometries(renderer);
	// When rendering sub-surface geometries the sphere needs to be transparent so that the
	// rear of the globe is visible (to provide visual clues when rendering sub-surface geometries).
	if (render_sub_surface_geometries)
	{
		// If the sphere background colour is currently opaque then make it semi-transparent,
		// otherwise leave it as it is (the user has probably chosen an alpha value they like).
		if (!has_transparent_background_colour)
		{
			// Temporarily change the background colour in the view state.
			// This will get picked up by 'OpaqueSphere'.
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
		// The globe is transparent so use the rear half globe projection transform to render
		// the rear half of the globe.
		render_globe_hemisphere_surface(
				renderer,
				*cache_handle,
				viewport_zoom_factor,
				device_independent_pixel_to_world_space_ratio,
				projection_transform_include_rear_half_globe,
				false/*is_front_half_globe*/);
	}

	// Render opaque sphere - can actually be transparent depending on the alpha value in its colour.
	render_sphere_background(renderer, projection_transform_include_full_globe);

	// The rendering of sub-surface geometries and the front surface of the globe are intermingled
	// because the latter can be used to occlude the former.
	if (render_sub_surface_geometries)
	{
		//
		// See if we can render the front half of the globe as a surface occlusion texture for sub-surface geometries.
		// This reduces the workload of rendering sub-surface data that is occluded by surface data.
		//
		boost::optional<GPlatesOpenGL::GLScreenRenderTarget::shared_ptr_type> screen_render_target =
				renderer.get_context().get_non_shared_state()->acquire_screen_render_target(
						renderer,
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
			renderer.rendering_to_context_framebuffer())
		{
			// Prepare for rendering the front half of the globe into a viewport-size render texture.
			// This will be used as a surface occlusion texture when rendering the globe sub-surface
			// in order to early terminate occluded volume-tracing rays for efficient sub-surface rendering.
			const GPlatesOpenGL::GLViewport screen_viewport = renderer.gl_get_viewport();
			GPlatesOpenGL::GLScreenRenderTarget::RenderScope screen_render_target_scope(
					*screen_render_target.get(),
					renderer,
					screen_viewport.width(),
					screen_viewport.height());

			// Save/restore the OpenGL state changed over the scope of the screen render target.
			GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_screen_render_target_scope(renderer);

			// Set the new viewport in case the current viewport has non-zero x and y offsets which
			// happens when the scene is rendered as overlapping tiles (for rendering very large images).
			// It's also important that, later when accessing the screen render texture, the NDC
			// coordinates (-1,-1) and (1,1) map to the corners of the screen render texture.
			renderer.gl_viewport(0, 0, screen_viewport.width(), screen_viewport.height());
			// Also change the scissor rectangle in case scissoring is enabled.
			renderer.gl_scissor(0, 0, screen_viewport.width(), screen_viewport.height());

			// Clear only the colour buffer.
			// The depth buffer is cleared in 'render_globe_hemisphere_surface()'.
			renderer.gl_clear_color();
			renderer.gl_clear(GL_COLOR_BUFFER_BIT);

			// Render the front half of the globe surface to the viewport-size render texture.
			render_globe_hemisphere_surface(
					renderer,
					*cache_handle,
					viewport_zoom_factor,
					device_independent_pixel_to_world_space_ratio,
					projection_transform_include_front_half_globe,
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
					renderer,
					*cache_handle,
					viewport_zoom_factor,
					device_independent_pixel_to_world_space_ratio,
					projection_transform_include_full_globe,
					front_globe_surface_texture);

			// Blend the front half of the globe surface (the surface occlusion texture) in the main framebuffer.
			// Note that this must use the original viewport.
			render_front_globe_hemisphere_surface_texture(
					renderer,
					front_globe_surface_texture);
		}
		else // surface occlusion not supported so render sub-surface followed by surface...
		{
			// Render the globe sub-surface first.
			render_globe_sub_surface(
					renderer,
					*cache_handle,
					viewport_zoom_factor,
					device_independent_pixel_to_world_space_ratio,
					projection_transform_include_full_globe);

			// Render the front half of the globe surface next.
			render_globe_hemisphere_surface(
					renderer,
					*cache_handle,
					viewport_zoom_factor,
					device_independent_pixel_to_world_space_ratio,
					projection_transform_include_front_half_globe,
					true/*is_front_half_globe*/);
		}
	}
	else // there are no sub-surface geometries...
	{
		// Render the front half of the globe surface.
		render_globe_hemisphere_surface(
				renderer,
				*cache_handle,
				viewport_zoom_factor,
				device_independent_pixel_to_world_space_ratio,
				projection_transform_include_front_half_globe,
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
	GPlatesMaths::UnitVector3D axis = d_globe_orientation_ptr->rotation_axis();
	GPlatesMaths::real_t angle_in_deg =
			GPlatesMaths::convert_rad_to_deg(d_globe_orientation_ptr->rotation_angle());

	transform.gl_rotate(angle_in_deg.dval(), axis.x().dval(), axis.y().dval(), axis.z().dval());
}


void
GPlatesGui::Globe::set_scene_lighting(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesOpenGL::GLMatrix &view_orientation)
{
	// Get the OpenGL light if the runtime system supports it.
	boost::optional<GPlatesOpenGL::GLLight::non_null_ptr_type> gl_light =
			d_gl_visual_layers->get_light(renderer);

	// Set the scene lighting parameters on the light (includes the current view orientation).
	if (gl_light)
	{
		gl_light.get()->set_scene_lighting(
				renderer,
				d_view_state.get_scene_lighting_parameters(),
				view_orientation);
	}
}


void
GPlatesGui::Globe::render_stars(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_stars)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state_scope(renderer);

	// Disable depth testing and depth writes.
	//
	// This is because we are using a different projection transform and the depth buffer values
	// depend on the projection transform.
	// This means we avoid a subsequent depth buffer clear.
	renderer.gl_enable(GL_DEPTH_TEST, GL_FALSE);
	renderer.gl_depth_mask(GL_FALSE);

	// Set up the projection transform for the stars.
	renderer.gl_load_matrix(GL_PROJECTION, projection_transform_include_stars);

	d_stars->paint(renderer);
}


void
GPlatesGui::Globe::render_sphere_background(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_full_globe)
{
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state_scope(renderer);

	// Disable depth testing and depth writes.
	// The opaque sphere is only used to render the sphere colour (and, for a transparent sphere,
	// blend the sphere colour with the rendered geometries on the rear of the globe).
	//
	// NOTE: Because we are using a different projection transform and the depth buffer values depend
	// on the projection transform then normally each projection transform needs its own depth buffer clear.
	// However since we're not reading or writing to depth buffer we don't need to clear depth buffer.
	renderer.gl_enable(GL_DEPTH_TEST, GL_FALSE);
	renderer.gl_depth_mask(GL_FALSE);

	// Set up the projection transform for entire globe.
	//
	// NOTE: It's actually rendered as a disk facing the camera so it will be inside the rear half
	// frustum (projection matrix) due to expansion by epsilon at globe centre distance.
	// However we will use the full globe frustum in case this changes.
	renderer.gl_load_matrix(GL_PROJECTION, projection_transform_include_full_globe);

	// Note that if the sphere is transparent it will cause objects rendered to the rear half
	// of the globe to be dimmer than normal due to alpha blending (this is the intended effect).
	d_sphere->paint(
			renderer,
			d_globe_orientation_ptr->rotation_axis(),
			GPlatesMaths::convert_rad_to_deg(d_globe_orientation_ptr->rotation_angle()).dval());
}


void
GPlatesGui::Globe::render_globe_hemisphere_surface(
		GPlatesOpenGL::GLRenderer &renderer,
		std::vector<cache_handle_type> &cache_handle,
		const double &viewport_zoom_factor,
		const double &device_independent_pixel_to_world_space_ratio,
		const GPlatesOpenGL::GLMatrix &projection_transform,
		bool is_front_half_globe)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state_scope(renderer);

	renderer.gl_load_matrix(GL_PROJECTION, projection_transform);

	// NOTE: Because we are using a different projection transform and the depth buffer values depend
	// on the projection transform then each projection transform needs its own depth buffer clear.
	// We also clear the stencil buffer in case it is used.
	renderer.gl_clear_depth(); // Clear depth to 1.0
	renderer.gl_clear_stencil(); // Clear stencil to 0
	// NOTE: Depth/stencil writes must be enabled for depth/stencil clears to work.
	renderer.gl_depth_mask(GL_TRUE);
	renderer.gl_stencil_mask(~0/*all ones*/);
	renderer.gl_clear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// Enable depth testing but disable depth writes (for the grid lines).
	renderer.gl_enable(GL_DEPTH_TEST, GL_TRUE);
	renderer.gl_depth_mask(GL_FALSE);

	if (!is_front_half_globe)
	{
		// Render the grid lines on the rear side of the sphere first.
		d_grid->paint(renderer);
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
					renderer,
					viewport_zoom_factor,
					device_independent_pixel_to_world_space_ratio,
					vector_geometries_override_colour);
	cache_handle.push_back(rendered_geoms_cache_half_globe);

	if (is_front_half_globe)
	{
		// Render the grid lines on the front side of the sphere last.
		d_grid->paint(renderer);
	}
}


void
GPlatesGui::Globe::render_front_globe_hemisphere_surface_texture(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesOpenGL::GLTexture::shared_ptr_to_const_type &front_globe_surface_texture)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state_scope(renderer);

	// Used to draw a full-screen quad into render texture.
	const GPlatesOpenGL::GLCompiledDrawState::non_null_ptr_to_const_type full_screen_quad_drawable =
			renderer.get_context().get_shared_state()->get_full_screen_2D_textured_quad(renderer);

	// Full-screen rendering requires no model-view-projection transform.
	renderer.gl_load_matrix(GL_MODELVIEW, GPlatesOpenGL::GLMatrix::IDENTITY);
	renderer.gl_load_matrix(GL_PROJECTION, GPlatesOpenGL::GLMatrix::IDENTITY);

	// Set up alpha blending for pre-multiplied alpha.
	// This has (src,dst) blend factors of (1, 1-src_alpha) instead of (src_alpha, 1-src_alpha).
	// This is where the RGB channels have already been multiplied by the alpha channel.
	// This necessary in order to be able to blend a render texture into the main framebuffer and
	// have it look the same as if we had blended directly into the main framebuffer in the first place.
	// However everything rendered into the render target must use pre-multiplied alpha.
	renderer.gl_enable(GL_BLEND);
	renderer.gl_blend_func(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	// Enable alpha testing as an optimisation for culling transparent raster pixels.
	renderer.gl_enable(GL_ALPHA_TEST);
	renderer.gl_alpha_func(GL_GREATER, GLclampf(0));

	// Disable depth testing and depth writes - we're just doing a full-screen copy and blend.
	renderer.gl_enable(GL_DEPTH_TEST, GL_FALSE);
	renderer.gl_depth_mask(GL_FALSE);

	// Bind the texture and enable fixed-function texturing on texture unit 0.
	const GLenum texture_unit_0 = renderer.get_capabilities().texture.gl_TEXTURE0;
	renderer.gl_bind_texture(front_globe_surface_texture, texture_unit_0, GL_TEXTURE_2D);
	renderer.gl_enable_texture(texture_unit_0, GL_TEXTURE_2D);
	renderer.gl_tex_env(texture_unit_0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	// Render the full-screen quad.
	renderer.apply_compiled_draw_state(*full_screen_quad_drawable);
}


void
GPlatesGui::Globe::render_globe_sub_surface(
		GPlatesOpenGL::GLRenderer &renderer,
		std::vector<cache_handle_type> &cache_handle,
		const double &viewport_zoom_factor,
		const double &device_independent_pixel_to_world_space_ratio,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_full_globe,
		boost::optional<GPlatesOpenGL::GLTexture::shared_ptr_to_const_type> surface_occlusion_texture)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state_scope(renderer);

	renderer.gl_load_matrix(GL_PROJECTION, projection_transform_include_full_globe);

	// NOTE: Because we are using a different projection transform and the depth buffer values depend
	// on the projection transform then each projection transform needs its own depth buffer clear.
	// We also clear the stencil buffer in case it is used.
	renderer.gl_clear_depth(); // Clear depth to 1.0
	renderer.gl_clear_stencil(); // Clear stencil to 0
	// NOTE: Depth/stencil writes must be enabled for depth/stencil clears to work.
	renderer.gl_depth_mask(GL_TRUE);
	renderer.gl_stencil_mask(~0/*all ones*/);
	renderer.gl_clear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// Draw the sub-surface geometries.
	// Draw in normal layer order.
	d_rendered_geom_collection_painter.set_visual_layers_reversed(false);
	const cache_handle_type rendered_geoms_cache_sub_surface_globe =
			d_rendered_geom_collection_painter.paint_sub_surface(
					renderer,
					viewport_zoom_factor,
					device_independent_pixel_to_world_space_ratio,
					surface_occlusion_texture,
					d_globe_orientation_changing_during_mouse_drag/*improve_performance_reduce_quality_hint*/);
	cache_handle.push_back(rendered_geoms_cache_sub_surface_globe);
}
