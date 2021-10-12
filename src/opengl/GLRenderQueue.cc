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

#include <boost/cast.hpp>

#include "GLRenderQueue.h"
#include "GLRenderTarget.h"
#include "GLRenderTargetManager.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GLRenderQueue::GLRenderQueue() :
	d_current_render_pass_depth(-1),
	d_current_render_pass(NULL)
{
}


GPlatesOpenGL::GLRenderOperationsTarget::non_null_ptr_type
GPlatesOpenGL::GLRenderQueue::push_render_target(
		const GLRenderTargetType::non_null_ptr_type &render_target_type,
		const GLStateGraph::non_null_ptr_type &render_target_state_graph)
{
	++d_current_render_pass_depth;

	// Create a new render pass if needed.
	if (d_current_render_pass_depth >= boost::numeric_cast<int>(d_render_passes.size()))
	{
		d_render_passes.push_back(GLRenderPass::create());
	}

	// Point to the next render pass.
	d_current_render_pass = &*d_render_passes[d_current_render_pass_depth];

	// Add the new render target onto the current render pass.
	return d_current_render_pass->add_render_target(render_target_type, render_target_state_graph);
}


void
GPlatesOpenGL::GLRenderQueue::pop_render_target()
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_render_pass != NULL,
			GPLATES_ASSERTION_SOURCE);

	--d_current_render_pass_depth;

	if (d_current_render_pass_depth < 0)
	{
		// Point to no render pass.
		d_current_render_pass = NULL;
	}
	else
	{
		// Point to the previous render pass.
		d_current_render_pass = &*d_render_passes[d_current_render_pass_depth];
	}
}


void
GPlatesOpenGL::GLRenderQueue::draw(
		GLRenderTargetManager &render_target_manager)
{
	GLRenderTarget::non_null_ptr_type initial_render_target =
			render_target_manager.get_frame_buffer_render_target();

	GLRenderTarget::non_null_ptr_type current_render_target = initial_render_target;

	// Draw the render passes in reverse order to how they were pushed.
	// This is because the render passes pushed first depend on the render passes pushed later.
	render_pass_seq_type::reverse_iterator render_pass_iter = d_render_passes.rbegin();
	render_pass_seq_type::reverse_iterator render_pass_end = d_render_passes.rend();
	for ( ; render_pass_iter != render_pass_end; ++render_pass_iter)
	{
		const GLRenderPass::non_null_ptr_type &render_pass = *render_pass_iter;

		current_render_target = render_pass->draw(render_target_manager, current_render_target);
	}

	// Restore render target to the initial render target which is the main frame buffer.
	if (current_render_target != initial_render_target)
	{
		current_render_target->unbind();
		initial_render_target->bind();
	}
}
