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
 
#ifndef GPLATES_OPENGL_GLCLEARBUFFERSSTATE_H
#define GPLATES_OPENGL_GLCLEARBUFFERSSTATE_H

#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLStateSet.h"


namespace GPlatesOpenGL
{
	/**
	 * Used to optionally set the colour, depth and stencil values
	 * used to clear the frame buffers.
	 */
	class GLClearBuffersState :
			public GLStateSet
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLClearBuffersState> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLClearBuffersState> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLClearBuffersState object with no state.
		 *
		 * Call @a gl_clear_color, etc to initialise the state.
		 * For example:
		 *   clear_buffers_state->gl_clear_color().gl_clear_depth();
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLClearBuffersState());
		}


		/**
		 * Sets the OpenGL clear colour and returns reference to 'this' so can chain calls.
		 */
		GLClearBuffersState &
		gl_clear_color(
				GLclampf red = GLclampf(0.0),
				GLclampf green = GLclampf(0.0),
				GLclampf blue = GLclampf(0.0),
				GLclampf alpha = GLclampf(0.0))
		{
			const ClearColour colour = { red, green, blue, alpha };
			d_colour = colour;
			return *this;
		}


		/**
		 * Sets the OpenGL clear depth value and returns reference to 'this' so can chain calls.
		 */
		GLClearBuffersState &
		gl_clear_depth(
				GLclampd depth = GLclampd(1.0))
		{
			d_depth = depth;
			return *this;
		}


		/**
		 * Sets the OpenGL clear stencil value and returns reference to 'this' so can chain calls.
		 */
		GLClearBuffersState &
		gl_clear_stencil(
				GLint stencil = 0)
		{
			d_stencil = stencil;
			return *this;
		}


		virtual
		void
		enter_state_set() const
		{
			if (d_colour)
			{
				glClearColor(d_colour->red, d_colour->green, d_colour->blue, d_colour->alpha);
			}
			if (d_depth)
			{
				glClearDepth(d_depth.get());
			}
			if (d_stencil)
			{
				glClearStencil(d_stencil.get());
			}
		}


		virtual
		void
		leave_state_set() const
		{
			// Set states back to the default state.
			if (d_colour)
			{
				glClearColor(0, 0, 0, 0);
			}
			if (d_depth)
			{
				glClearDepth(1.0);
			}
			if (d_stencil)
			{
				glClearStencil(0);
			}
		}

	private:
		struct ClearColour
		{
			GLclampf red;
			GLclampf green;
			GLclampf blue;
			GLclampf alpha;
		};

		boost::optional<ClearColour> d_colour;
		boost::optional<GLclampd> d_depth;
		boost::optional<GLint> d_stencil;


		GLClearBuffersState()
		{  }
	};
}

#endif // GPLATES_OPENGL_GLCLEARBUFFERSSTATE_H
