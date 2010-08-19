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
 
#ifndef GPLATES_OPENGL_GLRENDERQUEUE_H
#define GPLATES_OPENGL_GLRENDERQUEUE_H

#include <cstddef> // For std::size_t
#include <vector>

#include "GLRenderOperationsTarget.h"
#include "GLRenderPass.h"
#include "GLRenderTargetType.h"
#include "GLStateGraph.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLRenderTargetManager;

	/**
	 * Orders the render passes so that all textures are rendered to
	 * before they are used as input to render the main scene.
	 */
	class GLRenderQueue :
			public GPlatesUtils::ReferenceCount<GLRenderQueue>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLRenderQueue.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLRenderQueue> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLRenderQueue.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLRenderQueue> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLRenderQueue object.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLRenderQueue());
		}


		/**
		 * Starts a new render target using @a render_target.
		 *
		 * This also increments the render pass depth at which render operations are added.
		 * This creates a new render pass if one has not already been created at the current depth.
		 *
		 * Render operations can be added to the returned @a GLRenderOperationsTarget.
		 */
		GLRenderOperationsTarget::non_null_ptr_type
		push_render_target(
				const GLRenderTargetType::non_null_ptr_type &render_target_type,
				const GLStateGraph::non_null_ptr_type &render_target_state_graph);


		/**
		 * Stops using the most recently pushed render target.
		 *
		 * This also decrements the render pass depth at which render operations are added.
		 */
		void
		pop_render_target();


		/**
		 * Draws all render targets in their respective render passes in the appropriate order.
		 */
		void
		draw(
				GLRenderTargetManager &render_target_manager);

	private:
		//! Typedef for a sequence of render passes.
		typedef std::vector<GLRenderPass::non_null_ptr_type> render_pass_seq_type;


		/**
		 * The render pass stack.
		 */
		render_pass_seq_type d_render_passes;

		/**
		 * The render pass to which render operations are currently being added.
		 *
		 * '-1' means no render passes.
		 */
		int d_current_render_pass_depth;

		/**
		 * The render pass corresponding to the currently pushed render target.
		 *
		 * NOTE: This is not necessarily the top of the render pass stack because when
		 * a render target is popped the associated render pass is *not* popped - this is
		 * so a subsequent render target push goes to the existing render pass.
		 */
		GLRenderPass *d_current_render_pass;


		//! Constructor.
		GLRenderQueue();
	};
}

#endif // GPLATES_OPENGL_GLRENDERQUEUE_H
