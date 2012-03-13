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

#include "Globe.h"

#include "maths/MathsUtils.h"

#include "opengl/GLRenderer.h"

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
		RenderSettings &render_settings,
		const TextRenderer::non_null_ptr_to_const_type &text_renderer_ptr,
		const GlobeVisibilityTester &visibility_tester,
		ColourScheme::non_null_ptr_type colour_scheme) :
	d_view_state(view_state),
	d_gl_visual_layers(gl_visual_layers),
	d_render_settings(render_settings),
	d_rendered_geom_collection(rendered_geom_collection),
	d_visual_layers(visual_layers),
	d_globe_orientation_ptr(new SimpleGlobeOrientation()),
	d_rendered_geom_collection_painter(
			rendered_geom_collection,
			gl_visual_layers,
			visual_layers,
			d_render_settings,
			text_renderer_ptr,
			visibility_tester,
			colour_scheme)
{  }


GPlatesGui::Globe::Globe(
		Globe &existing_globe,
		const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
		const TextRenderer::non_null_ptr_to_const_type &text_renderer_ptr,
		const GlobeVisibilityTester &visibility_tester,
		ColourScheme::non_null_ptr_type colour_scheme) :
	d_view_state(existing_globe.d_view_state),
	d_gl_visual_layers(gl_visual_layers),
	d_render_settings(existing_globe.d_render_settings),
	d_rendered_geom_collection(existing_globe.d_rendered_geom_collection),
	d_visual_layers(existing_globe.d_visual_layers),
	d_globe_orientation_ptr(existing_globe.d_globe_orientation_ptr),
	d_rendered_geom_collection_painter(
			d_rendered_geom_collection,
			gl_visual_layers,
			d_visual_layers,
			d_render_settings,
			text_renderer_ptr,
			visibility_tester,
			colour_scheme)
{  }


void
GPlatesGui::Globe::set_new_handle_pos(
		const GPlatesMaths::PointOnSphere &pos)
{
	d_globe_orientation_ptr->set_new_handle_at_pos(pos);
}


void
GPlatesGui::Globe::update_handle_pos(
		const GPlatesMaths::PointOnSphere &pos)
{
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
	d_stars = boost::in_place(boost::ref(renderer), boost::ref(d_view_state), STARS_COLOUR);
	d_sphere = boost::in_place(boost::ref(renderer), boost::ref(d_view_state));
	d_black_sphere = boost::in_place(boost::ref(renderer), Colour::get_black());
	d_grid = boost::in_place(boost::ref(renderer), d_view_state.get_graticule_settings());
}


GPlatesGui::Globe::cache_handle_type
GPlatesGui::Globe::paint(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &viewport_zoom_factor,
		float scale,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_half_globe,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_full_globe,
		const GPlatesOpenGL::GLMatrix &projection_transform_include_stars)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_globe_state_scope(renderer);

	// The cached view is a sequence of caches for the caller to keep alive until the next frame.
	boost::shared_ptr<std::vector<cache_handle_type> > cache_handle(new std::vector<cache_handle_type>());

	// Most rendering should be done with depth testing *on* and depth writes *off*.
	// The exceptions will need to save/modify/restore state.
	renderer.gl_enable(GL_DEPTH_TEST, GL_TRUE);
	renderer.gl_depth_mask(GL_FALSE);

	// Set up the globe orientation transform.
	GPlatesOpenGL::GLMatrix globe_orientation_transform;
	get_globe_orientation_transform(globe_orientation_transform);
	renderer.gl_mult_matrix(GL_MODELVIEW, globe_orientation_transform);

	// Render stars.
	//
	// NOTE: We draw the stars first so they don't appear in front of the globe.
	//
	// NOTE: We also *disable* depth testing and depth writes because we are using a different
	// projection transform and the depth buffer values depend on the projection transform
	// so we don't want to mix them up with depth buffer values written with a different
	// projection transform later below.
	//
	{
		// Make sure we leave the OpenGL state the way it was.
		GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state_scope(renderer);

		// Turn depth testing off.
		renderer.gl_enable(GL_DEPTH_TEST, GL_FALSE);
		// Turn depth writes off.
		renderer.gl_depth_mask(GL_FALSE);

		// Set up the projection transform for the stars.
		renderer.gl_load_matrix(GL_PROJECTION, projection_transform_include_stars);

		d_stars->paint(renderer);
	}

	// Determine whether the globe is transparent or not.
	rgba8_t background_colour = Colour::to_rgba8(d_view_state.get_background_colour());
	bool transparent = (background_colour.alpha != 255);

	// If the globe is transparent then use a projection transform that has a far clip plane that
	// includes the full globe.
	// Otherwise use one that includes only the visible front half of the globe since 
	//
	// NOTE: Using a far clip plane that does *not* include the rear of the globe is important
	// because the raster rendering code then does not generate raster tiles for the rear of the
	// globe (which are not visible - if globe is opaque) and hence is noticeably faster.
	//
	// NOTE: We also use the same projection transform for the remainder of the rendering
	// so that depth buffering works - if we mixed different projection transform then the
	// depth buffer values written would be affected and hence couldn't be compared to each other
	// rendering the depth test undefined.
	renderer.gl_load_matrix(
			GL_PROJECTION,
			transparent
				? projection_transform_include_full_globe
				: projection_transform_include_half_globe);

	// Set up common state.
	d_rendered_geom_collection_painter.set_scale(scale);

	if (transparent)
	{
		// Make sure we leave the OpenGL state the way it was.
		GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state_scope(renderer);

		// To render the far side of the globe, we first render a black disk to draw
		// onto the depth buffer, set the depth function to be the reverse of the usual
		// and then render everything in reverse order.

		// Turn depth testing on.
		renderer.gl_enable(GL_DEPTH_TEST, GL_TRUE);
		// Turn depth writes on.
		renderer.gl_depth_mask(GL_TRUE);
		d_black_sphere->paint(
				renderer,
				d_globe_orientation_ptr->rotation_axis(),
				GPlatesMaths::convert_rad_to_deg(d_globe_orientation_ptr->rotation_angle()).dval());

		// Set the depth func to GL_GREATER.
		renderer.gl_depth_func(GL_GREATER);
		// Turn depth writes off.
		renderer.gl_depth_mask(GL_FALSE);

		// Render the grid lines on the far side of the sphere.
		d_grid->paint(renderer);

		// Draw the rendered geometries in reverse order.
		d_rendered_geom_collection_painter.set_visual_layers_reversed(true);
		const cache_handle_type rendered_geoms_cache_far_side_globe =
				d_rendered_geom_collection_painter.paint(
						renderer,
						viewport_zoom_factor);
		cache_handle->push_back(rendered_geoms_cache_far_side_globe);
	}

	// Render opaque sphere.
	{
		GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_state_scope(renderer);

		// Only enable depth testing if not transparent because the depth function is now GL_LESS
		// and opaque sphere (really a disk) will not get rendered due to being in the exact
		// position of the already rendered black disk (would need GL_LEQUAL for that to work).
		renderer.gl_enable(GL_DEPTH_TEST, transparent ? GL_FALSE : GL_TRUE);

		// Only write to the depth buffer if not transparent (because the depth buffer
		// is written to by the black disk if transparent).
		renderer.gl_depth_mask(transparent ? GL_FALSE : GL_TRUE);

		// Note that if the sphere is transparent it will cause objects rendered to the rear half
		// of the globe to be dimmer than normal due to alpha blending (this is the intended effect).
		d_sphere->paint(
				renderer,
				d_globe_orientation_ptr->rotation_axis(),
				GPlatesMaths::convert_rad_to_deg(d_globe_orientation_ptr->rotation_angle()).dval());
	}

	// Draw the rendered geometries.
	d_rendered_geom_collection_painter.set_visual_layers_reversed(false);
	const cache_handle_type rendered_geoms_cache_near_side_globe =
			d_rendered_geom_collection_painter.paint(
					renderer,
					viewport_zoom_factor);
	cache_handle->push_back(rendered_geoms_cache_near_side_globe);

	// Render the grid lines on the sphere.
	d_grid->paint(renderer);

	return cache_handle;
}


void
GPlatesGui::Globe::paint_vector_output(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &viewport_zoom_factor,
		float scale)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_globe_state_scope(renderer);

	// Most rendering should be done with depth testing *on* and depth writes *off*.
	// The exceptions will need to save/modify/restore state.
	renderer.gl_enable(GL_DEPTH_TEST, GL_TRUE);
	renderer.gl_depth_mask(GL_FALSE);

	// Set up the globe orientation transform.
	GPlatesOpenGL::GLMatrix globe_orientation_transform;
	get_globe_orientation_transform(globe_orientation_transform);
	renderer.gl_mult_matrix(GL_MODELVIEW, globe_orientation_transform);

	// Paint the circumference of the Earth.
	d_grid->paint_circumference(
			renderer,
			d_globe_orientation_ptr->rotation_axis(),
			GPlatesMaths::convert_rad_to_deg(d_globe_orientation_ptr->rotation_angle()).dval());

	// Paint the grid lines.
	d_grid->paint(renderer);

	// Get current rendered layer active state so we can restore later.
	const GPlatesViewOperations::RenderedGeometryCollection::MainLayerActiveState
		prev_rendered_layer_active_state =
			d_rendered_geom_collection.capture_main_layer_active_state();

	// Turn off rendering of digitisation layer.
	d_rendered_geom_collection.set_main_layer_active(
			GPlatesViewOperations::RenderedGeometryCollection::DIGITISATION_LAYER,
			false);

	// Draw the rendered geometries in the depth range [0, 0.7].
	d_rendered_geom_collection_painter.set_scale(scale);
	d_rendered_geom_collection_painter.paint(
			renderer,
			viewport_zoom_factor);

	// Restore previous rendered layer active state.
	d_rendered_geom_collection.restore_main_layer_active_state(
		prev_rendered_layer_active_state);
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
