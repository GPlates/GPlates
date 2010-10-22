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
#include "opengl/GLMaskBuffersState.h"
#include "opengl/GLFragmentTestStates.h"
#include "opengl/GLRenderGraphInternalNode.h"
#include "opengl/GLTransform.h"

#include "view-operations/RenderedGeometryCollection.h"


namespace
{
	const GPlatesGui::Colour STARS_COLOUR(0.75f, 0.75f, 0.75f);
	const GPlatesGui::Colour SPHERE_COLOUR(0.35f, 0.35f, 0.35f);
}


GPlatesGui::Globe::Globe(
		GPlatesPresentation::ViewState &view_state,
		const PersistentOpenGLObjects::non_null_ptr_type &persistent_opengl_objects,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		const GPlatesPresentation::VisualLayers &visual_layers,
		RenderSettings &render_settings,
		RasterColourSchemeMap &raster_colour_scheme_map,
		TextRenderer::ptr_to_const_type text_renderer_ptr,
		const GlobeVisibilityTester &visibility_tester,
		ColourScheme::non_null_ptr_type colour_scheme) :
	d_view_state(view_state),
	d_persistent_opengl_objects(persistent_opengl_objects),
	d_render_settings(render_settings),
	d_rendered_geom_collection(rendered_geom_collection),
	d_visual_layers(visual_layers),
	d_nurbs_renderer(GPlatesOpenGL::GLUNurbsRenderer::create()),
	d_stars(view_state, STARS_COLOUR),
	d_sphere(SPHERE_COLOUR),
	d_grid(NUM_CIRCLES_LAT, NUM_CIRCLES_LON),
	d_globe_orientation_ptr(new SimpleGlobeOrientation()),
	d_rendered_geom_collection_painter(
			rendered_geom_collection,
			persistent_opengl_objects,
			visual_layers,
			d_render_settings,
			raster_colour_scheme_map,
			text_renderer_ptr,
			visibility_tester,
			colour_scheme)
{
}

GPlatesGui::Globe::Globe(
		Globe &existing_globe,
		const PersistentOpenGLObjects::non_null_ptr_type &persistent_opengl_objects,
		RasterColourSchemeMap &raster_colour_scheme_map,
		TextRenderer::ptr_to_const_type text_renderer_ptr,
		const GlobeVisibilityTester &visibility_tester,
		ColourScheme::non_null_ptr_type colour_scheme) :
	d_view_state(existing_globe.d_view_state),
	d_persistent_opengl_objects(persistent_opengl_objects),
	d_render_settings(existing_globe.d_render_settings),
	d_rendered_geom_collection(existing_globe.d_rendered_geom_collection),
	d_visual_layers(existing_globe.d_visual_layers),
	d_nurbs_renderer(GPlatesOpenGL::GLUNurbsRenderer::create()),
	d_stars(d_view_state, STARS_COLOUR),
	d_sphere(SPHERE_COLOUR),
	d_grid(NUM_CIRCLES_LAT, NUM_CIRCLES_LON),
	d_globe_orientation_ptr(existing_globe.d_globe_orientation_ptr),
	d_rendered_geom_collection_painter(
			d_rendered_geom_collection,
			persistent_opengl_objects,
			d_visual_layers,
			d_render_settings,
			raster_colour_scheme_map,
			text_renderer_ptr,
			visibility_tester,
			colour_scheme)
{
}

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
		const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &render_graph_node,
		const double &viewport_zoom_factor,
		float scale)
{
	// Set up the globe orientation transform.
	GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type globe_orientation_transform_node =
			setup_globe_orientation_transform(*render_graph_node);

	// Render stars.
	GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type stars_node =
			create_rendered_layer_node(globe_orientation_transform_node);
	d_stars.paint(stars_node);

	// Render opaque sphere.
	// Note that this writes a disk cutting through the centre of the globe
	// parallel to the *screen* to the depth buffer.
	GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type sphere_node =
			create_rendered_layer_node(globe_orientation_transform_node, GL_TRUE);
	d_sphere.paint(
			sphere_node,
			d_globe_orientation_ptr->rotation_axis(),
			GPlatesMaths::convert_rad_to_deg(d_globe_orientation_ptr->rotation_angle()).dval());

	// Draw the rendered geometries.
	d_rendered_geom_collection_painter.set_scale(scale);
	d_rendered_geom_collection_painter.paint(
			globe_orientation_transform_node,
			viewport_zoom_factor,
			d_nurbs_renderer);

	// Render the grid lines on the sphere.
	GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type grid_node =
			create_rendered_layer_node(globe_orientation_transform_node);
	Colour grid_colour = Colour::get_silver();
	grid_colour.alpha() = 0.5f;
	d_grid.paint(grid_node, grid_colour);
}

void
GPlatesGui::Globe::paint_vector_output(
		const boost::shared_ptr<GPlatesOpenGL::GLContext::SharedState> &gl_context_shared_state,
		const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &render_graph_node,
		const double &viewport_zoom_factor,
		float scale)
{
	// Set up the globe orientation transform.
	GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type globe_orientation_transform_node =
			setup_globe_orientation_transform(*render_graph_node);

	// Paint the circumference of the Earth.
	GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type grid_circumference_node =
			create_rendered_layer_node(globe_orientation_transform_node);
	d_grid.paint_circumference(grid_circumference_node, GPlatesGui::Colour::get_grey());

	// Paint the grid lines.
	GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type grid_lines_node =
			create_rendered_layer_node(globe_orientation_transform_node);
	d_grid.paint(grid_lines_node, Colour::get_grey());

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
			globe_orientation_transform_node,
			viewport_zoom_factor,
			d_nurbs_renderer);

	// Restore previous rendered layer active state.
	d_rendered_geom_collection.restore_main_layer_active_state(
		prev_rendered_layer_active_state);
}


GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type
GPlatesGui::Globe::setup_globe_orientation_transform(
			GPlatesOpenGL::GLRenderGraphInternalNode &render_graph_node)
{
	GPlatesMaths::UnitVector3D axis = d_globe_orientation_ptr->rotation_axis();
	GPlatesMaths::real_t angle_in_deg =
			GPlatesMaths::convert_rad_to_deg(d_globe_orientation_ptr->rotation_angle());

	GPlatesOpenGL::GLTransform::non_null_ptr_type globe_orientation_transform =
			GPlatesOpenGL::GLTransform::create(GL_MODELVIEW);
	globe_orientation_transform->get_matrix().gl_rotate(
			angle_in_deg.dval(), axis.x().dval(), axis.y().dval(), axis.z().dval());
	
	// Create an internal node to store the globe orientation transform.
	GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type globe_orientation_transform_node =
			GPlatesOpenGL::GLRenderGraphInternalNode::create();
	globe_orientation_transform_node->set_transform(globe_orientation_transform);
	// Add the globe orientation transform node to the parent render graph node.
	render_graph_node.add_child_node(globe_orientation_transform_node);

	return globe_orientation_transform_node;
}


GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type
GPlatesGui::Globe::create_rendered_layer_node(
		const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &parent_render_graph_node,
		GLboolean depth_write_flag)
{
	// Create an internal node to represent the depth range.
	GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type rendered_layer_node =
			GPlatesOpenGL::GLRenderGraphInternalNode::create();

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

	// Turn on depth testing - it's off by default.
	GPlatesOpenGL::GLDepthTestState::non_null_ptr_type depth_test_state =
			GPlatesOpenGL::GLDepthTestState::create();
	depth_test_state->gl_enable(GL_TRUE);
	state_set->add_state_set(depth_test_state);

	// Turn depth writes on or off as specified.
	GPlatesOpenGL::GLMaskBuffersState::non_null_ptr_type depth_mask_state =
			GPlatesOpenGL::GLMaskBuffersState::create();
	depth_mask_state->gl_depth_mask(depth_write_flag);
	state_set->add_state_set(depth_mask_state);

	rendered_layer_node->set_state_set(state_set);

	// Add the render layer node to the parent render graph node.
	parent_render_graph_node->add_child_node(rendered_layer_node);

	return rendered_layer_node;
}

