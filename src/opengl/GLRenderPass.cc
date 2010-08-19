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

#include "GLRenderPass.h"


GPlatesOpenGL::GLRenderOperationsTarget::non_null_ptr_type
GPlatesOpenGL::GLRenderPass::add_render_target(
		const GLRenderTargetType::non_null_ptr_type &render_target_type,
		const GLStateGraph::non_null_ptr_type &state_graph)
{
	GLRenderOperationsTarget::non_null_ptr_type render_operations_target =
			GLRenderOperationsTarget::create(render_target_type, state_graph);

	d_render_operations_target_seq.push_back(render_operations_target);

	return render_operations_target;
}


GPlatesOpenGL::GLRenderTarget::non_null_ptr_type
GPlatesOpenGL::GLRenderPass::draw(
		GLRenderTargetManager &render_target_manager,
		const GLRenderTarget::non_null_ptr_type &previous_render_target)
{
	GLRenderTarget::non_null_ptr_type current_render_target = previous_render_target;

	// Iterate over the render operations targets and draw each one.
	BOOST_FOREACH(
			const GLRenderOperationsTarget::non_null_ptr_type &render_operations_target,
			d_render_operations_target_seq)
	{
		current_render_target =
				render_operations_target->draw(render_target_manager, current_render_target);
	}

	return current_render_target;
}
