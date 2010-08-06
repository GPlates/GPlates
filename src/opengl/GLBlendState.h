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
 
#ifndef GPLATES_OPENGL_GLBLENDSTATE_H
#define GPLATES_OPENGL_GLBLENDSTATE_H

#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLStateSet.h"


namespace GPlatesOpenGL
{
	/**
	 * Sets 'GL_BLEND' state.
	 */
	class GLBlendState :
			public GLStateSet
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLBlendState> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLBlendState> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLBlendState object with no state.
		 *
		 * Call @a gl_enable, gl_blend_func, etc to initialise the state.
		 * For example:
		 *   blend_state->gl_enable().gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLBlendState());
		}

		GLBlendState &
		gl_enable(
				GLboolean enable)
		{
			d_enable = enable;
			return *this;
		}

		//! Stores 'glBlendFunc' state.
		GLBlendState &
		gl_blend_func(
				GLenum sfactor = GL_ONE,
				GLenum dfactor = GL_ZERO)
		{
			const BlendFactors blend_factors = { sfactor, dfactor };
			d_blend_factors = blend_factors;
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
					glEnable(GL_BLEND);
				}
				else
				{
					glDisable(GL_BLEND);
				}
			}

			if (d_blend_factors)
			{
				glBlendFunc(d_blend_factors->sfactor, d_blend_factors->dfactor);
			}
		}


		virtual
		void
		leave_state_set() const
		{
			// Set states back to the default state.
			if (d_enable)
			{
				glDisable(GL_BLEND);
			}

			if (d_blend_factors)
			{
				glBlendFunc(GL_ONE, GL_ZERO);
			}
		}

	private:
		struct BlendFactors
		{
			GLenum sfactor;
			GLenum dfactor;
		};

		boost::optional<GLboolean> d_enable;
		boost::optional<BlendFactors> d_blend_factors;

		//! Constructor.
		GLBlendState()
		{  }
	};
}

#endif // GPLATES_OPENGL_GLBLENDSTATE_H
