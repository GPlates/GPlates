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
 
#ifndef GPLATES_OPENGL_GLRENDERTARGET_H
#define GPLATES_OPENGL_GLRENDERTARGET_H

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLRenderTarget :
			public GPlatesUtils::ReferenceCount<GLRenderTarget>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLRenderTarget.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLRenderTarget> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLRenderTarget.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLRenderTarget> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLRenderTarget object.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLRenderTarget());
		}


		/**
		 * Call before rendering to 'this' render target.
		 *
		 * Makes this render target active which could involve switching OpenGL contexts.
		 *
		 * TODO: Implement this for render targets other than the main frame buffer.
		 */
		void
		begin_render_to_target()
		{  }


		/**
		 * Call after rendering to 'this' render target (and before rendering to a new target).
		 *
		 * Does any post-rendering tasks (such as copying render target to a texture).
		 *
		 * TODO: Implement this for render targets other than the main frame buffer.
		 */
		void
		end_render_to_target()
		{  }


	private:


		//! Constructor.
		GLRenderTarget()
		{  }
	};
}

#endif // GPLATES_OPENGL_GLRENDERTARGET_H
