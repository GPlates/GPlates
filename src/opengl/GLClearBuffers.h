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
 
#ifndef GPLATES_OPENGL_GLCLEARBUFFERS_H
#define GPLATES_OPENGL_GLCLEARBUFFERS_H

#include <opengl/OpenGL.h>

#include "GLDrawable.h"


namespace GPlatesOpenGL
{
	/**
	 * A drawable to clear the frame buffer(s).
	 *
	 * This is a @a GLDrawable because it directly modifies the frame buffer(s) which
	 * is something that all @a GLDrawable derivations do.
	 *
	 * NOTE: The clear state such as the clear colour in 'glClearColour()' is not
	 * set here as it should be set by a @a GLStateSet (via @a GLStateSet) since
	 * it is OpenGL state. A @a GLClearBuffers and a @a GLStateSet can then be added to
	 * a @a GLRenderGraphNode which will clear the buffers using the appropriate clear state.
	 */
	class GLClearBuffers :
			public GLDrawable
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLClearBuffers.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLClearBuffers> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLClearBuffers.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLClearBuffers> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLClearBuffers object (by default doesn't clear anything).
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLClearBuffers());
		}


		/**
		 * Sets the bitmask used for the OpenGL 'glClear()' function.
		 *
		 * @a clear_mask is the same as the argument to the OpenGL function 'glClear()'.
		 * That is a bitwise combination of GL_COLOR_BUFFER_BIT, GL_DEPTH_BUFFER_BIT, etc.
		 *
		 * NOTE: The OpenGL 'glClear()' isn't actually called until @a draw is called.
		 */
		void
		gl_clear(
				GLbitfield clear_mask)
		{
			d_clear_mask = clear_mask;
		}


		virtual
		void
		bind() const
		{
			// No geometry to bind.
		}


		virtual
		void
		draw() const
		{
			glClear(d_clear_mask);
		}

	private:
		GLbitfield d_clear_mask;


		/**
		 * Constructor.
		 */
		GLClearBuffers() :
			d_clear_mask(0)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLCLEARBUFFERS_H
