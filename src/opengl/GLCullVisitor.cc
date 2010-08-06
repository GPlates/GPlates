/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <boost/foreach.hpp>

#include "GLCullVisitor.h"

#include "GLBindTextureState.h"
#include "GLMultiResolutionRaster.h"
#include "GLMultiResolutionRasterNode.h"
#include "GLRenderOperation.h"
#include "GLRenderGraph.h"
#include "GLRenderGraphDrawableNode.h"
#include "GLRenderGraphInternalNode.h"
#include "GLText3DNode.h"
#include "GLViewportNode.h"
#include "GLViewportState.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GLCullVisitor::GLCullVisitor() :
	d_current_render_target_state(NULL),
	d_render_queue(GLRenderQueue::create())
{
	// Push a render target corresponding to the frame buffer (of the window).
	// This will be the render target that the main scene is rendered to.
	push_render_target(GLRenderTarget::create());
}


void
GPlatesOpenGL::GLCullVisitor::visit(
		const GPlatesUtils::non_null_intrusive_ptr<render_graph_type> &render_graph)
{
	render_graph->get_root_node().accept_visitor(*this);
}


void
GPlatesOpenGL::GLCullVisitor::visit(
		const GPlatesUtils::non_null_intrusive_ptr<render_graph_internal_node_type> &internal_node)
{
	preprocess_node(*internal_node);

	// Visit the child nodes.
	internal_node->visit_child_nodes(*this);

	postprocess_node(*internal_node);
}


void
GPlatesOpenGL::GLCullVisitor::visit(
		const GPlatesUtils::non_null_intrusive_ptr<viewport_node_type> &viewport_node)
{
	preprocess_node(*viewport_node);

	// Get the old viewport.
	boost::optional<GLViewport> old_viewport =
			d_current_render_target_state->d_transform_state->get_current_viewport();

	// Get the new viewport.
	const GLViewport &new_viewport = viewport_node->get_viewport();

	// Create a state set to set and restore the viewport.
	GLViewportState::non_null_ptr_type viewport_state = GLViewportState::create(
			old_viewport, new_viewport);

	// Push the viewport state set.
	push_state_set(viewport_state);

	// Let the transform state know of the new viewport.
	// This is necessary since it is used to determine pixel projections in world space
	// which are in turn used for level-of-detail selection for rasters.
	d_current_render_target_state->d_transform_state->gl_viewport(new_viewport);

	// Visit the child nodes.
	viewport_node->visit_child_nodes(*this);

	// Restore the old viewport if there was one.
	if (old_viewport)
	{
		d_current_render_target_state->d_transform_state->gl_viewport(old_viewport.get());
	}

	// Pop the viewport state set.
	pop_state_set();

	postprocess_node(*viewport_node);
}


void
GPlatesOpenGL::GLCullVisitor::visit(
		const GPlatesUtils::non_null_intrusive_ptr<render_graph_drawable_node_type> &drawable_node)
{
	preprocess_node(*drawable_node);

	// Create a render operation (contains enough information to render one drawable).
	GLRenderOperation::non_null_ptr_type render_operation = GLRenderOperation::create(
			drawable_node->get_drawable(),
			d_current_render_target_state->d_transform_state->get_current_model_view_matrix(),
			d_current_render_target_state->d_transform_state->get_current_projection_matrix());

	// Add the render operation to the current render operations target.
	add_render_operation(render_operation);

	postprocess_node(*drawable_node);
}


void
GPlatesOpenGL::GLCullVisitor::visit(
		const GPlatesUtils::non_null_intrusive_ptr<multi_resolution_raster_node_type> &raster_node)
{
	preprocess_node(*raster_node);

	// Get the current model-view-projection transform.
	GLMatrix::non_null_ptr_to_const_type model_view_transform =
			d_current_render_target_state->d_transform_state->get_current_model_view_matrix();
	GLMatrix::non_null_ptr_to_const_type projection_transform =
			d_current_render_target_state->d_transform_state->get_current_projection_matrix();

	GLMultiResolutionRaster::non_null_ptr_to_const_type raster =
			raster_node->get_multi_resolution_raster();

	// Get the currently visible raster tiles.
	std::vector<GLMultiResolutionRaster::tile_handle_type> visible_tile_handles;
	raster->get_visible_tiles(
			*d_current_render_target_state->d_transform_state, visible_tile_handles);

	// Create a render operation for each visible tile.
	BOOST_FOREACH(GLMultiResolutionRaster::tile_handle_type tile_handle, visible_tile_handles)
	{
		GLMultiResolutionRaster::Tile tile = raster->get_tile(tile_handle);

		// Create a render operation (contains enough information to render one raster tile).
		GLRenderOperation::non_null_ptr_type render_operation = GLRenderOperation::create(
				tile.get_drawable(),
				model_view_transform,
				projection_transform);

		// Create a state set that binds the tile texture to texture unit 0.
		GLBindTextureState::non_null_ptr_type bind_tile_texture = GLBindTextureState::create();
		bind_tile_texture->gl_bind_texture(GL_TEXTURE_2D, tile.get_texture());

		// Push the state set onto the state graph.
		push_state_set(bind_tile_texture);

		// Add the render operation to the current render operations target.
		add_render_operation(render_operation);

		// Pop the state set.
		pop_state_set();
	}

	postprocess_node(*raster_node);
}


void
GPlatesOpenGL::GLCullVisitor::visit(
		const GPlatesUtils::non_null_intrusive_ptr<text_3d_node_type> &text_3d_node)
{
	preprocess_node(*text_3d_node);

	// Create a render operation (contains enough information to render one drawable).
	GLRenderOperation::non_null_ptr_type render_operation = GLRenderOperation::create(
			// Converts 3D text position to 2D window coordinates...
			text_3d_node->get_drawable(*d_current_render_target_state->d_transform_state),
			// The model-view and projection matrices aren't used but we have to provide them anyway...
			d_current_render_target_state->d_transform_state->get_current_model_view_matrix(),
			d_current_render_target_state->d_transform_state->get_current_projection_matrix());

	// Add the render operation to the current render operations target.
	add_render_operation(render_operation);

	postprocess_node(*text_3d_node);
}


void
GPlatesOpenGL::GLCullVisitor::preprocess_node(
		const GLRenderGraphNode &node)
{
	// If the current node has some state to set then push it.
	if (node.get_state_set())
	{
		push_state_set(node.get_state_set().get());
	}

	// If the current node has a transform then push it.
	if (node.get_transform())
	{
		const GLTransform &node_transform = *node.get_transform().get();

		d_current_render_target_state->d_transform_state->gl_matrix_mode(
				node_transform.get_matrix_mode());
		d_current_render_target_state->d_transform_state->gl_push_matrix();
		d_current_render_target_state->d_transform_state->gl_mult_matrix(
				node_transform.get_matrix());
	}
}


void
GPlatesOpenGL::GLCullVisitor::postprocess_node(
		const GLRenderGraphNode &node)
{
	// If the current node pushed a transform then pop it.
	if (node.get_transform())
	{
		d_current_render_target_state->d_transform_state->gl_matrix_mode(
				node.get_transform().get()->get_matrix_mode());
		d_current_render_target_state->d_transform_state->gl_pop_matrix();
	}

	// If the current node pushed some state then pop it.
	if (node.get_state_set())
	{
		pop_state_set();
	}
}


void
GPlatesOpenGL::GLCullVisitor::push_render_target(
		const GLRenderTarget::non_null_ptr_type &render_target)
{
	// Create a new state for the render target.
	RenderTargetState render_target_state(render_target, *d_render_queue);

	d_render_target_state_stack.push(render_target_state);
	d_current_render_target_state = &d_render_target_state_stack.top();
}


void
GPlatesOpenGL::GLCullVisitor::pop_render_target()
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!d_render_target_state_stack.empty(),
			GPLATES_ASSERTION_SOURCE);

	d_render_queue->pop_render_target();

	// Pop the state created for the render target.
	d_render_target_state_stack.pop();
	d_current_render_target_state = d_render_target_state_stack.empty()
			? NULL
			: &d_render_target_state_stack.top();
}


void
GPlatesOpenGL::GLCullVisitor::push_state_set(
		const GLStateSet::non_null_ptr_to_const_type &state_set)
{
	// Push the state set and get a new, or existing, state in return depending on whether
	// the 'state_set' has been seen before and has the same ancestor lineage up to the root
	// render graph node.
	GLStateGraphNode::non_null_ptr_to_const_type state_graph_node =
			d_current_render_target_state->d_state_graph_builder->push_state_set(state_set);

	// Notify the render operations target of the new state.
	d_current_render_target_state->d_render_operations_target->set_state(state_graph_node);
}


void
GPlatesOpenGL::GLCullVisitor::pop_state_set()
{
	// Pop the most recently pushed state set and get state that results after the pop.
	GLStateGraphNode::non_null_ptr_to_const_type state_graph_node =
			d_current_render_target_state->d_state_graph_builder->pop_state_set();

	// Notify the render operations target of the new state.
	d_current_render_target_state->d_render_operations_target->set_state(state_graph_node);
}


GPlatesOpenGL::GLCullVisitor::RenderTargetState::RenderTargetState(
		const GLRenderTarget::non_null_ptr_type &render_target,
		GLRenderQueue &render_queue) :
	d_transform_state(GLTransformState::create()),
	d_state_graph_builder(GLStateGraphBuilder::create()),
	d_state_graph(d_state_graph_builder->get_state_graph()),
	// Create a new target for current render operations...
	d_render_operations_target(
			render_queue.push_render_target(render_target, d_state_graph))
{
}
