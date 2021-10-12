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
 
#ifndef GPLATES_OPENGL_GLDEPTHRANGESTATE_H
#define GPLATES_OPENGL_GLDEPTHRANGESTATE_H

#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLStateSet.h"


namespace GPlatesOpenGL
{
	/**
	 * Sets 'glDepthRange' state.
	 */
	class GLDepthRangeState :
			public GLStateSet
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLDepthRangeState> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLDepthRangeState> non_null_ptr_to_const_type;


		//! Creates a @a GLDepthRangeState object.
		static
		non_null_ptr_type
		create(
				GLclampd zNear = 0.0,
				GLclampd zFar = 1.0)
		{
			return non_null_ptr_type(new GLDepthRangeState(zNear, zFar));
		}


		//! Stores 'glDepthRange' state.
		void
		gl_depth_range(
				GLclampd zNear = 0.0,
				GLclampd zFar = 1.0)
		{
			d_near = zNear;
			d_far = zFar;
		}


		virtual
		void
		enter_state_set() const
		{
			glDepthRange(d_near, d_far);
		}


		virtual
		void
		leave_state_set() const
		{
			// Set states back to the default state.
			glDepthRange(0.0, 1.0);
		}

	private:
		GLclampd d_near;
		GLclampd d_far;


		//! Constructor.
		GLDepthRangeState(
				GLclampd zNear,
				GLclampd zFar) :
			d_near(zNear),
			d_far(zFar)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLDEPTHRANGESTATE_H
