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
 
#ifndef GPLATES_OPENGL_GLMASKBUFFERSSTATE_H
#define GPLATES_OPENGL_GLMASKBUFFERSSTATE_H

#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLStateSet.h"


namespace GPlatesOpenGL
{
	/**
	 * Used to set the frame buffer mask (such as depth mask).
	 */
	class GLMaskBuffersState :
			public GLStateSet
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMaskBuffersState> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMaskBuffersState> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLMaskBuffersState object with no state.
		 *
		 * Call @a gl_depth_mask, etc to initialise the state.
		 * For example:
		 *   mask_buffers_state->gl_depth_mask().gl_color_mask(GL_FALSE, GL_FALSE, GL_FALSE);
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLMaskBuffersState());
		}


		/**
		 * Sets the OpenGL colour mask and returns reference to 'this' so can chain calls.
		 */
		GLMaskBuffersState &
		gl_color_mask(
				GLboolean red = GL_TRUE,
				GLboolean green = GL_TRUE,
				GLboolean blue = GL_TRUE,
				GLboolean alpha = GL_TRUE)
		{
			const ColourMask colour_mask = { red, green, blue, alpha };
			d_colour_mask = colour_mask;
			return *this;
		}


		/**
		 * Sets the OpenGL depth mask and returns reference to 'this' so can chain calls.
		 */
		GLMaskBuffersState &
		gl_depth_mask(
				GLboolean flag = GL_TRUE)
		{
			d_depth_mask = flag;
			return *this;
		}


		/**
		 * Sets the OpenGL stencil mask and returns reference to 'this' so can chain calls.
		 */
		GLMaskBuffersState &
		gl_clear_stencil(
				GLint stencil = 0)
		{
			d_stencil_mask = stencil;
			return *this;
		}


		virtual
		void
		enter_state_set() const
		{
			if (d_colour_mask)
			{
				glColorMask(d_colour_mask->red, d_colour_mask->green, d_colour_mask->blue, d_colour_mask->alpha);
			}
			if (d_depth_mask)
			{
				glDepthMask(d_depth_mask.get());
			}
			if (d_stencil_mask)
			{
				glStencilMask(d_stencil_mask.get());
			}
		}


		virtual
		void
		leave_state_set() const
		{
			// Set states back to the default state.
			if (d_colour_mask)
			{
				glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			}
			if (d_depth_mask)
			{
				glDepthMask(GL_TRUE);
			}
			if (d_stencil_mask)
			{
				glStencilMask(~GLuint(0)/*all ones*/);
			}
		}

	private:
		struct ColourMask
		{
			GLboolean red;
			GLboolean green;
			GLboolean blue;
			GLboolean alpha;
		};

		boost::optional<ColourMask> d_colour_mask;
		boost::optional<GLboolean> d_depth_mask;
		boost::optional<GLuint> d_stencil_mask;


		GLMaskBuffersState()
		{  }
	};
}

#endif // GPLATES_OPENGL_GLMASKBUFFERSSTATE_H
