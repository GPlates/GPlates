/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_GLDEPTHRANGE_H
#define GPLATES_OPENGL_GLDEPTHRANGE_H

#include <boost/operators.hpp>
#include <opengl/OpenGL.h>

#include "maths/types.h"


namespace GPlatesOpenGL
{
	/**
	 * OpenGL depth range parameters.
	 */
	class GLDepthRange :
			public boost::equality_comparable<GLDepthRange>
	{
	public:
		//! Constructor.
		GLDepthRange(
				GLclampd z_near = 0.0,
				GLclampd z_far = 1.0)
		{
			set_depth_range(z_near, z_far);
		}


		/**
		 * Sets the depth range parameters.
		 */
		void
		set_depth_range(
				GLclampd z_near = 0.0,
				GLclampd z_far = 1.0)
		{
			d_z_near = z_near;
			d_z_far = z_far;
		}


		GLclampd
		get_z_near() const
		{
			return d_z_near.dval();
		}


		GLclampd
		get_z_far() const
		{
			return d_z_far.dval();
		}


		//! Equality operator - and operator!= provided by boost::equality_comparable.
		bool
		operator==(
				const GLDepthRange &other) const
		{
			// NOTE: This is an epsilon test...
			return d_z_near == other.d_z_near && d_z_far == other.d_z_far;
		}

	private:
		GPlatesMaths::real_t d_z_near;
		GPlatesMaths::real_t d_z_far;
	};
}

#endif // GPLATES_OPENGL_GLDEPTHRANGE_H
