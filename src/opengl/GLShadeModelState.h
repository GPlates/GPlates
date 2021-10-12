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
 
#ifndef GPLATES_OPENGL_GLSHADEMODELSTATE_H
#define GPLATES_OPENGL_GLSHADEMODELSTATE_H

#include <opengl/OpenGL.h>

#include "GLStateSet.h"


namespace GPlatesOpenGL
{
	/**
	 * Sets 'glShadeModel' state.
	 */
	class GLShadeModelState :
			public GLStateSet
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLShadeModelState> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLShadeModelState> non_null_ptr_to_const_type;


		//! Creates a @a GLShadeModelState object.
		static
		non_null_ptr_type
		create(
				GLenum mode = GL_SMOOTH)
		{
			return non_null_ptr_type(new GLShadeModelState(mode));
		}


		//! Stores 'glShadeModel' state.
		void
		gl_shade_model(
				GLenum mode = GL_SMOOTH)
		{
			d_mode = mode;
		}


		virtual
		void
		enter_state_set() const
		{
			glShadeModel(d_mode);
		}


		virtual
		void
		leave_state_set() const
		{
			// Set states back to the default state.
			glShadeModel(GL_SMOOTH);
		}

	private:
		GLenum d_mode;


		//! Constructor.
		explicit
		GLShadeModelState(
				GLenum mode) :
			d_mode(mode)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLSHADEMODELSTATE_H
