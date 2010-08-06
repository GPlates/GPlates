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
 
#ifndef GPLATES_OPENGL_GLRENDERPASS_H
#define GPLATES_OPENGL_GLRENDERPASS_H

#include <vector>

#include "GLRenderOperationsTarget.h"
#include "GLStateGraph.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLState;

	/**
	 * A render pass is a grouping of render operations that need to be drawn before
	 * those in another render pass - all render operations in one render pass are drawn
	 * before any render operations in the next render pass.
	 *
	 * This is useful when a render operation requires a texture that in turn needs its
	 * own render operation to generate the texture data (ie, render to texture).
	 * By placing the render operation that uses the texture in a separate render pass
	 * to the render operation that generates the texture we can be sure the texture data
	 * is valid by the time it accessed.
	 */
	class GLRenderPass :
			public GPlatesUtils::ReferenceCount<GLRenderPass>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLRenderPass.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLRenderPass> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLRenderPass.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLRenderPass> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLRenderPass object.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLRenderPass());
		}


		/**
		 * Adds a new render target and the state graph associated with it.
		 *
		 * Render operations can be added to the returned @a GLRenderOperationsTarget.
		 */
		GLRenderOperationsTarget::non_null_ptr_type
		add_render_target(
				const GLRenderTarget::non_null_ptr_type &render_target,
				const GLStateGraph::non_null_ptr_type &render_target_state_graph);


		/**
		 * Draws all render operations of all added render targets in the appropriate order.
		 */
		void
		draw(
				GLState &state);

	private:
		//! Typedef for a sequence of render operations targets.
		typedef std::vector<GLRenderOperationsTarget::non_null_ptr_type>
				render_operations_target_seq_type;


		/**
		 * The list of targets for render operations.
		 */
		render_operations_target_seq_type d_render_operations_target_seq;


		//! Constructor.
		GLRenderPass()
		{  }
	};
}

#endif // GPLATES_OPENGL_GLRENDERPASS_H
