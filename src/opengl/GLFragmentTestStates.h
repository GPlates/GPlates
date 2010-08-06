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
 
#ifndef GPLATES_OPENGL_GLFRAGMENTTESTSTATES_H
#define GPLATES_OPENGL_GLFRAGMENTTESTSTATES_H

#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLStateSet.h"


namespace GPlatesOpenGL
{
	/**
	 * Sets 'GL_DEPTH_TEST' state.
	 */
	class GLDepthTestState :
			public GLStateSet
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLDepthTestState> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLDepthTestState> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLDepthTestState object with no state.
		 *
		 * Call @a gl_enable, gl_depth_func, etc to initialise the state.
		 * For example:
		 *   depth_test_state->gl_enable().gl_depth_func(GL_LESS);
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLDepthTestState());
		}

		GLDepthTestState &
		gl_enable(
				GLboolean enable)
		{
			d_enable = enable;
			return *this;
		}

		//! Stores 'glDepthFunc' state.
		GLDepthTestState &
		gl_depth_func(
				GLenum func = GL_LESS)
		{
			d_func = func;
			return *this;
		}


		virtual
		void
		enter_state_set() const
		{
			if (d_enable)
			{
				if (d_enable.get())
				{
					glEnable(GL_DEPTH_TEST);
				}
				else
				{
					glDisable(GL_DEPTH_TEST);
				}
			}

			if (d_func)
			{
				glDepthFunc(d_func.get());
			}
		}


		virtual
		void
		leave_state_set() const
		{
			// Set states back to the default state.
			if (d_enable)
			{
				glDisable(GL_DEPTH_TEST);
			}

			if (d_func)
			{
				glDepthFunc(GL_LESS);
			}
		}

	private:
		boost::optional<GLboolean> d_enable;
		boost::optional<GLenum> d_func;

		//! Constructor.
		GLDepthTestState()
		{  }
	};
}

#endif // GPLATES_OPENGL_GLFRAGMENTTESTSTATES_H
