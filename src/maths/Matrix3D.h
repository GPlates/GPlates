/* $Id$ */

/**
 * \file 
 * No file specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   Hamish Law <hlaw@es.usyd.edu.au>
 *   James Boyden <jboyden@es.usyd.edu.au>
 */

#ifndef _GPLATES_MATHS_MATRIX3D_H_
#define _GPLATES_MATHS_MATRIX3D_H_

#include "PointOnSphere.h"
#include "Rotation.h"
#include "global/types.h"  /* real_t */

namespace GPlatesMaths
{
	using namespace GPlatesGlobal;

	/** 
	 * 3x3 Matrix. 
	 * Instances of class Matrix3D are created using 
	 * Matrix3D::CreateRotationMatrix().
	 */
	class Matrix3D
	{
		public:
			/** 
			 * Create a Matrix3D that performs a rotation.
			 * @param E The pole around which the rotation will take
			 *   place.
			 * @param phi The number of degrees, in radians, to rotate
			 *   around @a E.
			 * @return The Matrix3D corresponding to a rotation of @a phi
			 *   degrees around the pole @a E.
			 * @todo Yet to be implemented.
			 * @note A direct translation of formula (9) from Greiner will
			 *   have problems, since PointOnSphere is in spherical coordinates,
			 *   whereas Greiner (I think) uses Cartesian.
			 * @role ConcreteCreator::FactoryMethod() in the Factory 
			 *   Method design pattern (p107). 
			 */
			static Matrix3D
			CreateRotationMatrix(const PointOnSphere& E, const real_t& phi);
			
			/** 
			 * Create a Matrix that performs the given Rotation.
			 * @overload 
			 */
			static Matrix3D
			CreateRotationMatrix(const Rotation&);

			real_t
			GetXX() const { return _xx; }

			real_t
			GetXY() const { return _xy; }

			real_t
			GetXZ() const { return _xz; }

			real_t
			GetYX() const { return _yx; }

			real_t
			GetYY() const { return _yy; }

			real_t
			GetYZ() const { return _yz; }

			real_t
			GetZX() const { return _zx; }

			real_t
			GetZY() const { return _zy; }

			real_t
			GetZZ() const { return _zz; }

		private:
			/**
			 * The row and column entries of this matrix.
			 */
			real_t _xx, _xy, _xz, _yx, _yy, _yz, _zx, _zy, _zz;

			/** 
			 * Private to prevent construction.
			 */
			Matrix3D() { }
	};
}

#endif  // _GPLATES_MATHS_MATRIX3D_H_
