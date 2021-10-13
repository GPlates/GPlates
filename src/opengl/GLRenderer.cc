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
		const GLRenderTargetType::non_null_ptr_type &render_target_type,
		RenderTargetUsageType render_target_usage)
{
	switch (render_target_usage)
	{
	case RENDER_TARGET_USAGE_SERIAL:
		// If we have a parent render target state *and* the render target about to be pushed
		// is 'serial' then force a new render operations target to be created when the render
		// target being pushed is subsequently popped. This ensures that any render operations
		// submitted after the pop will get rendered after the render target (being pushed) is
		// rendered to. This effectively causes render targets to be serialised wrt draw order.
		if (d_current_render_target_state)
		{
			// This forces the parent to create a new render target when/if we later return
			// to continue drawing to it.
			d_current_render_target_state->d_render_operations_target = boost::none;
		}
		break;

	case RENDER_TARGET_USAGE_PARALLEL:
		// If the render target usage is parallel then render to a deeper render pass.
		d_render_queue->push_render_pass();
		break;

	default:
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		break;
	}

	// Create a new render target state for the render target.
	RenderTargetState render_target_state(render_target_type, render_target_usage);

	// Note that we don't actually push or add a render target yet - we wait until the first
	// drawable is added to ensure our render target will always have drawables.

	d_render_target_state_stack.push(render_target_state);
	d_current_render_target_state = &d_render_target_state_stack.top();
}


void
GPlatesOpenGL::GLRenderer::pop_render_target()
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!d_render_target_state_stack.empty(),
			GPLATES_ASSERTION_SOURCE);

	// If the render target usage is 'parallel' then pop the render pass off the render queue.
	if (d_current_render_target_state->d_render_target_usage == RENDER_TARGET_USAGE_PARALLEL)
	{
		d_render_queue->pop_render_pass();
	}

	// Pop the render target state created for the render target.
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
	d_current_render_target_state->d_state_graph_builder->push_state_set(state_set);
}


void
GPlatesOpenGL::GLRenderer::pop_state_set()
{
	// Make sure there's a currently pushed render target.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_render_target_state != NULL,
			GPLATES_ASSERTION_SOURCE);

	// Pop the most recently pushed state set and get state that results after the pop.
	d_current_render_target_state->d_state_graph_builder->pop_state_set();
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

	// Get the current state.
	const GLStateGraphNode::non_null_ptr_to_const_type state =
			d_current_render_target_state->d_state_graph_builder->get_current_state_set();

	// Make sure we have a render operations target before adding the drawable.
	// We delay the creation of the target until a drawable is added so that a target
	// only gets created if a drawable is added.
	if (!d_current_render_target_state->d_render_operations_target)
	{
		// Create a new target for current render operations.
		d_current_render_target_state->d_render_operations_target =
				d_render_queue->add_render_target(
						d_current_render_target_state->d_render_target_type,
						d_current_render_target_state->d_state_graph);
	}

	// Add the render operation and its associated state - if the state is the same as
	// the last render operation added to this target then the GLRenderOperationsTarget object
	// will make sure the state only gets set once and reused by both render operations.
	d_current_render_target_state->d_render_operations_target.get()->add_render_operation(
			state, render_operation);
}


GPlatesOpenGL::GLRenderer::RenderTargetState::RenderTargetState(
		const GLRenderTargetType::non_null_ptr_type &render_target_type,
		RenderTargetUsageType render_target_usage) :
	d_transform_state(GLTransformState::create()),
	d_state_graph_builder(GLStateGraphBuilder::create()),
	d_state_graph(d_state_graph_builder->get_state_graph()),
	d_render_target_type(render_target_type),
	d_render_target_usage(render_target_usage)
{
}
