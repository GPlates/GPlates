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
#include <boost/integer_traits.hpp>
#include <boost/optional.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <QDebug>

#include "GLContext.h"
#include "GLMatrix.h"
#include "GLRenderOperationsTarget.h"
#include "GLState.h"
#include "GLUtils.h"

#include "global/CompilerWarnings.h"

#include "utils/Profile.h"


GPlatesOpenGL::GLRenderOperationsTarget::GLRenderOperationsTarget(
		const GLRenderTargetType::non_null_ptr_type &render_target_type,
		const GLStateGraph::non_null_ptr_type &state_graph) :
	d_render_target_type(render_target_type),
	d_state_graph(state_graph),
	d_current_render_group_id( // Set to a value that should never be valid
			boost::integer_traits<GLStateSet::render_group_type>::const_max),
	d_current_render_group(NULL),
	d_current_render_sequence(NULL),
	d_current_render_state(NULL)
{
	// Set the initial state.
	set_state(state_graph->get_root_state_graph_node());
}


void
GPlatesOpenGL::GLRenderOperationsTarget::set_state(
		const GLStateGraphNode::non_null_ptr_to_const_type &state_graph_node)
{
	d_current_render_state = state_graph_node.get();

	// Get the render group from the new state.
	const GLStateSet::render_group_type render_group_id =
			d_current_render_state->get_render_group();

	// If the state has changed the render group then point to a new internal render group.
	if (render_group_id != d_current_render_group_id)
	{
		d_current_render_group_id = render_group_id;

		// Point to the current render group - creates one if it doesn't exist.
		d_current_render_group = &d_render_groups[render_group_id];
	}

	// Keep track of the state graph nodes that belong to the current render group.
	// Each state graph node will have its own sequence of render operations.
	std::pair<render_sequence_map_type::iterator, bool> render_sequence_result =
			d_current_render_group->render_sequences.insert(
					render_sequence_map_type::value_type(
							state_graph_node,
							RenderSequence(state_graph_node)));

	// Point to the current render sequence.
	d_current_render_sequence = &render_sequence_result.first->second;

	// If the state graph node was inserted then it's the first time we've encountered it
	// and so we add it to the end of our draw order list.
	if (render_sequence_result.second)
	{
		d_current_render_group->render_sequence_draw_order.push_back(d_current_render_sequence);
	}
}


void
GPlatesOpenGL::GLRenderOperationsTarget::add_render_operation(
		const GLStateGraphNode::non_null_ptr_to_const_type &state,
		const GLRenderOperation::non_null_ptr_to_const_type &render_operation)
{
	// If the state hasn't changed then there's no need to set it.
	if (state.get() != d_current_render_state)
	{
		set_state(state);
	}

	d_current_render_sequence->render_operations.push_back(render_operation);
}


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

GPlatesOpenGL::GLRenderTarget::non_null_ptr_type
GPlatesOpenGL::GLRenderOperationsTarget::draw(
		GLRenderTargetManager &render_target_manager,
		const GLRenderTarget::non_null_ptr_type &previous_render_target)
{
	PROFILE_FUNC();

	GLRenderTarget::non_null_ptr_type current_render_target =
			d_render_target_type->get_render_target(render_target_manager);

	// If the render target has changed then bind the new render target.
	if (current_render_target != previous_render_target)
	{
		previous_render_target->unbind();
		current_render_target->bind();
	}

	// About to start rendering to the render target.
	current_render_target->begin_render_to_target();

	// Used to apply the state directly to OpenGL.
	GLState state;

	// Save the current model-view and projection matrices since we'll be loading
	// them as we draw.
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	// Start the model-view and projection matrices off as identity - doesn't really matter
	// as long as they point to a matrix other than those referenced by the render operations.
	GLTransform::non_null_ptr_to_const_type current_model_view_matrix = GLTransform::create(GL_MODELVIEW);
	GLTransform::non_null_ptr_to_const_type current_projection_matrix = GLTransform::create(GL_PROJECTION);

	// The current drawable.
	boost::optional<GLDrawable::non_null_ptr_to_const_type> current_drawable;

	// Iterate through the render groups - the order of iteration will follow
	// the integer render group numbers which is what we want.
	BOOST_FOREACH(const render_group_map_type::value_type &render_group_entry, d_render_groups)
	{
		const RenderGroup &render_group = render_group_entry.second;

		// Iterate over the render sequences in the current render group.
		BOOST_FOREACH(RenderSequence *render_sequence, render_group.render_sequence_draw_order)
		{
			// Set the state to match the state graph node for the current render sequence.
			d_state_graph->change_state(state, *render_sequence->state);

			// Draw the render operations in the current state graph node.
			BOOST_FOREACH(
					const GLRenderOperation::non_null_ptr_to_const_type &render_operation,
					render_sequence->render_operations)
			{
				// Load the model-view matrix into OpenGL if it's changed.
				const GLTransform::non_null_ptr_to_const_type &model_view_matrix =
						render_operation->get_model_view_matrix();
				if (model_view_matrix != current_model_view_matrix)
				{
					glMatrixMode(GL_MODELVIEW);
					glLoadMatrixd(model_view_matrix->get_matrix().get_matrix());
					current_model_view_matrix = model_view_matrix;
				}

				// Load the projection matrix into OpenGL if it's changed.
				const GLTransform::non_null_ptr_to_const_type &projection_matrix =
						render_operation->get_projection_matrix();
				if (projection_matrix != current_projection_matrix)
				{
					glMatrixMode(GL_PROJECTION);
					glLoadMatrixd(projection_matrix->get_matrix().get_matrix());
					current_projection_matrix = projection_matrix;
				}

				//
				// Bind and draw the drawable
				//

				const GLDrawable::non_null_ptr_to_const_type &drawable =
						render_operation->get_drawable();

				// Only need to bind the same drawable once.
				if (drawable != current_drawable)
				{
					drawable->bind();
					current_drawable = drawable;
				}

				drawable->draw();
			}
		}
	}

	// Change the state to the root of the state graph to effectively restore the
	// OpenGL state to the default state.
	d_state_graph->change_state_to_root_node(state);

	// Restore the model-view and projection matrices.
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	// Finished rendering to the render target.
	current_render_target->end_render_to_target();

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);

	return current_render_target;
}

// See above
ENABLE_GCC_WARNING("-Wshadow")
