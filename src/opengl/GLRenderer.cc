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

#include "GLRenderer.h"

#include "GLRenderOperation.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GLRenderer::GLRenderer(
		const GLRenderQueue::non_null_ptr_type &render_queue) :
	d_current_render_target_state(NULL),
	d_render_queue(render_queue)
{
}


void
GPlatesOpenGL::GLRenderer::push_render_target(
		const GLRenderTargetType::non_null_ptr_type &render_target_type)
{
	// Create a new state for the render target.
	//
	// NOTE: The call to 'push_render_target()' on the render queue is done
	// inside the 'RenderTargetState' constructor.
	RenderTargetState render_target_state(render_target_type, *d_render_queue);

	d_render_target_state_stack.push(render_target_state);
	d_current_render_target_state = &d_render_target_state_stack.top();
}


void
GPlatesOpenGL::GLRenderer::pop_render_target()
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
GPlatesOpenGL::GLRenderer::push_state_set(
		const GLStateSet::non_null_ptr_to_const_type &state_set)
{
	// Make sure there's a currently pushed render target.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_render_target_state != NULL,
			GPLATES_ASSERTION_SOURCE);

	// Push the state set and get a new, or existing, state in return depending on whether
	// the 'state_set' has been seen before and has the same ancestor lineage up to the root
	// state graph node.
	GLStateGraphNode::non_null_ptr_to_const_type state_graph_node =
			d_current_render_target_state->d_state_graph_builder->push_state_set(state_set);

	// Notify the render operations target of the new state.
	d_current_render_target_state->d_render_operations_target->set_state(state_graph_node);
}


void
GPlatesOpenGL::GLRenderer::pop_state_set()
{
	// Make sure there's a currently pushed render target.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_render_target_state != NULL,
			GPLATES_ASSERTION_SOURCE);

	// Pop the most recently pushed state set and get state that results after the pop.
	GLStateGraphNode::non_null_ptr_to_const_type state_graph_node =
			d_current_render_target_state->d_state_graph_builder->pop_state_set();

	// Notify the render operations target of the new state.
	d_current_render_target_state->d_render_operations_target->set_state(state_graph_node);
}


void
GPlatesOpenGL::GLRenderer::push_transform(
		const GLTransform &transform)
{
	// Make sure there's a currently pushed render target.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_render_target_state != NULL,
			GPLATES_ASSERTION_SOURCE);

	GLTransformState &transform_state = *d_current_render_target_state->d_transform_state;

	transform_state.push_transform(transform);
}


void
GPlatesOpenGL::GLRenderer::pop_transform()
{
	// Make sure there's a currently pushed render target.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_render_target_state != NULL,
			GPLATES_ASSERTION_SOURCE);

	GLTransformState &transform_state = *d_current_render_target_state->d_transform_state;

	transform_state.pop_transform();
}


const GPlatesOpenGL::GLTransformState &
GPlatesOpenGL::GLRenderer::get_transform_state() const
{
	// Make sure there's a currently pushed render target.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_render_target_state != NULL,
			GPLATES_ASSERTION_SOURCE);

	return *d_current_render_target_state->d_transform_state;
}


GPlatesOpenGL::GLTransformState &
GPlatesOpenGL::GLRenderer::get_transform_state()
{
	// Make sure there's a currently pushed render target.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_render_target_state != NULL,
			GPLATES_ASSERTION_SOURCE);

	return *d_current_render_target_state->d_transform_state;
}


void
GPlatesOpenGL::GLRenderer::add_drawable(
		const GLDrawable::non_null_ptr_to_const_type &drawable)
{
	// Make sure there's a currently pushed render target.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_render_target_state != NULL,
			GPLATES_ASSERTION_SOURCE);

	// Get the transform state of current render target.
	GLTransformState &transform_state = *d_current_render_target_state->d_transform_state;

	// Create a render operation.
	GLRenderOperation::non_null_ptr_type render_operation = GLRenderOperation::create(
			drawable,
			transform_state.get_current_model_view_transform(),
			transform_state.get_current_projection_transform());

	d_current_render_target_state->d_render_operations_target->add_render_operation(render_operation);
}


GPlatesOpenGL::GLRenderer::RenderTargetState::RenderTargetState(
		const GLRenderTargetType::non_null_ptr_type &render_target_type,
		GLRenderQueue &render_queue) :
	d_transform_state(GLTransformState::create()),
	d_state_graph_builder(GLStateGraphBuilder::create()),
	d_state_graph(d_state_graph_builder->get_state_graph()),
	// Create a new target for current render operations...
	d_render_operations_target(
			render_queue.push_render_target(render_target_type, d_state_graph))
{
}
