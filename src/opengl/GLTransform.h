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
 
#ifndef GPLATES_OPENGL_GLTRANSFORM_H
#define GPLATES_OPENGL_GLTRANSFORM_H

#include "GLMatrix.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesMaths
{
	class UnitQuaternion3D;
}

namespace GPlatesOpenGL
{
	/**
	 * Simply contains a 4x4 matrix allocated on the heap and managed by reference-counted shared pointers.
	 */
	class GLTransform :
			public GPlatesUtils::ReferenceCount<GLTransform>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLTransform.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLTransform> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLTransform.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLTransform> non_null_ptr_to_const_type;

		/**
		 * Constructs identity matrix.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLTransform());
		}

		/**
		 * Constructs arbitrary matrix.
		 */
		static
		non_null_ptr_type
		create(
				const GLMatrix &matrix)
		{
			return non_null_ptr_type(new GLTransform(matrix));
		}

		/**
		 * Constructs arbitrary matrix.
		 *
		 * The format of @a m must be column-major:
		 *
		 * | m0 m4 m8  m12 |
		 * | m1 m5 m9  m13 |
		 * | m2 m6 m10 m14 |
		 * | m3 m7 m11 m15 |
		 */
		static
		non_null_ptr_type
		create(
				const GLdouble *matrix)
		{
			return non_null_ptr_type(new GLTransform(matrix));
		}

		/**
		 * Constructs 4x4 matrix from specified unit quaternion (note only the 3x3 rotation
		 * part of the matrix is initialised - the rest is set to zero).
		 */
		static
		non_null_ptr_type
		create(
				const GPlatesMaths::UnitQuaternion3D &quaternion)
		{
			return non_null_ptr_type(new GLTransform(quaternion));
		}


		/**
		 * Returns a clone of 'this' transform.
		 */
		non_null_ptr_type
		clone() const
		{
			// This class is not copy-constructable due to ReferenceCount base class
			// so use a non-copy constructor instead that achieves the same effect.
			return non_null_ptr_type(new GLTransform(d_matrix));
		}


		/**
		 * Returns the 4x4 matrix of 'this' transform in OpenGL format.
		 *
		 * This can be used to alter the matrix via methods in @a GLMatrix.
		 */
		GLMatrix &
		get_matrix()
		{
			return d_matrix;
		}

		/**
		 * Returns the 4x4 matrix of 'this' transform in OpenGL format.
		 */
		const GLMatrix &
		get_matrix() const
		{
			return d_matrix;
		}

	private:
		GLMatrix d_matrix;


		//! Default constructor.
		GLTransform()
		{  }

		//! Constructor.
		explicit
		GLTransform(
				const GLMatrix &matrix) :
			d_matrix(matrix)
		{  }

		//! Constructor.
		explicit
		GLTransform(
				const GLdouble *matrix) :
			d_matrix(matrix)
		{  }

		//! Constructor.
		explicit
		GLTransform(
				const GPlatesMaths::UnitQuaternion3D &quaternion) :
			d_matrix(quaternion)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLTRANSFORM_H
