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

#include "Globe.h"

#include "maths/MathsUtils.h"

#include "opengl/GLCompositeStateSet.h"
#include "opengl/GLFragmentTestStates.h"
#include "opengl/GLMaskBuffersState.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLTransform.h"

#include "presentation/ViewState.h"

#include "view-operations/RenderedGeometryCollection.h"


namespace
{
	const GPlatesGui::Colour STARS_COLOUR(0.75f, 0.75f, 0.75f);
}

namespace GPlatesOpenGL
{
	class GLRenderer;
}

GPlatesGui::Globe::Globe(
		GPlatesPresentation::ViewState &view_state,
		const PersistentOpenGLObjects::non_null_ptr_type &persistent_opengl_objects,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		const GPlatesPresentation::VisualLayers &visual_layers,
		RenderSettings &render_settings,
		const TextRenderer::non_null_ptr_to_const_type &text_renderer_ptr,
		const GlobeVisibilityTester &visibility_tester,
		ColourScheme::non_null_ptr_type colour_scheme) :
	d_view_state(view_state),
	d_persistent_opengl_objects(persistent_opengl_objects),
	d_render_settings(render_settings),
	d_rendered_geom_collection(rendered_geom_collection),
	d_visual_layers(visual_layers),
	d_nurbs_renderer(GPlatesOpenGL::GLUNurbsRenderer::create()),
	d_stars(view_state, STARS_COLOUR),
	d_sphere(view_state),
	d_black_sphere(Colour::get_black()),
	d_grid(view_state.get_graticule_settings()),
	d_globe_orientation_ptr(new SimpleGlobeOrientation()),
	d_rendered_geom_collection_painter(
			rendered_geom_collection,
			persistent_opengl_objects,
			visual_layers,
			d_render_settings,
			text_renderer_ptr,
			visibility_tester,
			colour_scheme)
{  }


GPlatesGui::Globe::Globe(
		Globe &existing_globe,
		const PersistentOpenGLObjects::non_null_ptr_type &persistent_opengl_objects,
		const TextRenderer::non_null_ptr_to_const_type &text_renderer_ptr,
		const GlobeVisibilityTester &visibility_tester,
		ColourScheme::non_null_ptr_type colour_scheme) :
	d_view_state(existing_globe.d_view_state),
	d_persistent_opengl_objects(persistent_opengl_objects),
	d_render_settings(existing_globe.d_render_settings),
	d_rendered_geom_collection(existing_globe.d_rendered_geom_collection),
	d_visual_layers(existing_globe.d_visual_layers),
	d_nurbs_renderer(GPlatesOpenGL::GLUNurbsRenderer::create()),
	d_stars(d_view_state, STARS_COLOUR),
	d_sphere(d_view_state),
	d_black_sphere(Colour::get_black()),
	d_grid(d_view_state.get_graticule_settings()),
	d_globe_orientation_ptr(existing_globe.d_globe_orientation_ptr),
	d_rendered_geom_collection_painter(
			d_rendered_geom_collection,
			persistent_opengl_objects,
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
GPlatesGui::Globe::paint(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &viewport_zoom_factor,
		float scale)
{
	// Set up the globe orientation transform.
	GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type globe_orientation_transform =
			get_globe_orientation_transform();
	renderer.push_transform(*globe_orientation_transform);

	// Determine whether the globe is transparent or not.
	rgba8_t background_colour = Colour::to_rgba8(d_view_state.get_background_colour());
	bool transparent = (background_colour.alpha != 255);

	// Set up common state.
	d_rendered_geom_collection_painter.set_scale(scale);

	if (transparent)
	{
		// To render the far side of the globe, we first render a black disk to draw
		// onto the depth buffer, set the depth function to be the reverse of the usual
		// and then render everything in reverse order.
		renderer.push_state_set(get_rendered_layer_state(GL_TRUE, GL_TRUE));
		d_black_sphere.paint(
				renderer,
				d_globe_orientation_ptr->rotation_axis(),
				GPlatesMaths::convert_rad_to_deg(d_globe_orientation_ptr->rotation_angle()).dval());
		renderer.pop_state_set();
	}

	// Render stars.
	renderer.push_state_set(get_rendered_layer_state(GL_FALSE));
	d_stars.paint(renderer);
	renderer.pop_state_set();

	if (transparent)
	{
		// Set the depth func to GL_GREATER.
		GPlatesOpenGL::GLDepthTestState::non_null_ptr_type depth_test_state =
			GPlatesOpenGL::GLDepthTestState::create();
		depth_test_state->gl_depth_func(GL_GREATER);
		renderer.push_state_set(depth_test_state);

		// Render the grid lines on the far side of the sphere.
		renderer.push_state_set(get_rendered_layer_state());
		d_grid.paint(renderer);
		renderer.pop_state_set();

		// Draw the rendered geometries in reverse order.
		d_rendered_geom_collection_painter.set_visual_layers_reversed(true);
		d_rendered_geom_collection_painter.paint(
				renderer,
				viewport_zoom_factor,
				d_nurbs_renderer);

		renderer.pop_state_set(); // 'depth_test_state'
	}

	// Render opaque sphere.
	// Only write to the depth buffer if not transparent (because the depth buffer
	// is written to by the black disk if transparent).
	renderer.push_state_set(
			get_rendered_layer_state(
					transparent ? GL_FALSE : GL_TRUE,
					transparent ? GL_FALSE : GL_TRUE));
	d_sphere.paint(
			renderer,
			d_globe_orientation_ptr->rotation_axis(),
			GPlatesMaths::convert_rad_to_deg(d_globe_orientation_ptr->rotation_angle()).dval());
	renderer.pop_state_set();

	// Draw the rendered geometries.
	d_rendered_geom_collection_painter.set_visual_layers_reversed(false);
	d_rendered_geom_collection_painter.paint(
			renderer,
			viewport_zoom_factor,
			d_nurbs_renderer);

	// Render the grid lines on the sphere.
	renderer.push_state_set(get_rendered_layer_state());
	d_grid.paint(renderer);
	renderer.pop_state_set();

	renderer.pop_transform(); // 'globe_orientation_transform'
}


void
GPlatesGui::Globe::paint_vector_output(
		const boost::shared_ptr<GPlatesOpenGL::GLContext::SharedState> &gl_context_shared_state,
		GPlatesOpenGL::GLRenderer &renderer,
		const double &viewport_zoom_factor,
		float scale)
{
	// Set up the globe orientation transform.
	GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type globe_orientation_transform =
			get_globe_orientation_transform();
	renderer.push_transform(*globe_orientation_transform);

	// Paint the circumference of the Earth.
	renderer.push_state_set(get_rendered_layer_state());
	d_grid.paint_circumference(
			renderer,
			d_globe_orientation_ptr->rotation_axis(),
			GPlatesMaths::convert_rad_to_deg(d_globe_orientation_ptr->rotation_angle()).dval());
	renderer.pop_state_set();

	// Paint the grid lines.
	renderer.push_state_set(get_rendered_layer_state());
	d_grid.paint(renderer);
	renderer.pop_state_set();

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
			viewport_zoom_factor,
			d_nurbs_renderer);

	// Restore previous rendered layer active state.
	d_rendered_geom_collection.restore_main_layer_active_state(
		prev_rendered_layer_active_state);

	renderer.pop_transform(); // 'globe_orientation_transform'
}


GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type
GPlatesGui::Globe::get_globe_orientation_transform() const
{
	GPlatesMaths::UnitVector3D axis = d_globe_orientation_ptr->rotation_axis();
	GPlatesMaths::real_t angle_in_deg =
			GPlatesMaths::convert_rad_to_deg(d_globe_orientation_ptr->rotation_angle());

	GPlatesOpenGL::GLTransform::non_null_ptr_type globe_orientation_transform =
			GPlatesOpenGL::GLTransform::create(GL_MODELVIEW);
	globe_orientation_transform->get_matrix().gl_rotate(
			angle_in_deg.dval(), axis.x().dval(), axis.y().dval(), axis.z().dval());

	return globe_orientation_transform;
}


GPlatesOpenGL::GLStateSet::non_null_ptr_to_const_type
GPlatesGui::Globe::get_rendered_layer_state(
		GLboolean depth_test_flag,
		GLboolean depth_write_flag) const
{
	// If we have any state to set we can do it here (such as giving each rendered layer
	// its own depth range - used to do this but don't need it anymore).
	GPlatesOpenGL::GLCompositeStateSet::non_null_ptr_type state_set =
			GPlatesOpenGL::GLCompositeStateSet::create();

	// Create a state set that ensures this rendered layer will form a render sub group
	// that will not get reordered with other layers by the renderer (to minimise state changes).
	state_set->set_enable_render_sub_group();

	//
	// See comment in 'GlobeRenderedGeometryLayerPainter::paint()' for why
	// depth test is turned on but depth writes are turned off.
	//

	// Turn on depth testing as specified - it's off by default.
	GPlatesOpenGL::GLDepthTestState::non_null_ptr_type depth_test_state =
			GPlatesOpenGL::GLDepthTestState::create();
	depth_test_state->gl_enable(depth_test_flag);
	state_set->add_state_set(depth_test_state);

	// Turn depth writes on or off as specified.
	GPlatesOpenGL::GLMaskBuffersState::non_null_ptr_type depth_mask_state =
			GPlatesOpenGL::GLMaskBuffersState::create();
	depth_mask_state->gl_depth_mask(depth_write_flag);
	state_set->add_state_set(depth_mask_state);

	return state_set;
}

