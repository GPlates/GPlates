/* $Id: CartesianConvMatrix3D.h,v 1.1.4.2 2006/03/14 00:29:01 mturner Exp $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author: mturner $
 *   $Date: 2006/03/14 00:29:01 $
 * 
 * Copyright (C) 2003, 2005, 2006 The University of Sydney, Australia.
 *
 * --------------------------------------------------------------------
 *
 * This file is part of GPlates.  GPlates is free software; you can
 * redistribute it and/or modify it under the terms of the GNU General
 * Public License, version 2, as published by the Free Software
 * Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to 
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 */


#ifndef _GPLATES_MATHS_CARTESIANCONVMATRIX3D_H_
#define _GPLATES_MATHS_CARTESIANCONVMATRIX3D_H_

#include <ostream>
#include "types.h"  /* real_t */
#include "PointOnSphere.h"
#include "Vector3D.h"


namespace GPlatesMaths
{
	/** 
	 * A 3x3 matrix used to convert the components of a global Cartesian
	 * Vector3D (x, y, z) into the components of a local Cartesian Vector3D
	 * (n, e, d) at a given PointOnSphere.
	 */
	class CartesianConvMatrix3D
	{
		public:
			/**
			 * Create a cartesian conversion matrix to operate at
			 * the PointOnSphere @a pos.
			 */
			explicit 
			CartesianConvMatrix3D(
					const PointOnSphere &pos);

			real_t
			nx() const
			{
				return _nx;
			}

			real_t
			ny() const
			{
				return _ny;
			}

			real_t
			nz() const
			{
				return _nz;
			}

			real_t
			ex() const
			{
				return _ex;
			}

			real_t
			ey() const
			{
				return _ey;
			}

			real_t
			ez() const
			{
				return _ez;
			}

			real_t
			dx() const
			{
				return _dx;
			}

			real_t
			dy() const
			{
				return _dy;
			}

			real_t
			dz() const
			{
				return _dz;
			}

		private:
			real_t _nx, _ny, _nz;
			real_t _ex, _ey, _ez;
			real_t _dx, _dy, _dz;
	};


	Vector3D
	operator*(
			const CartesianConvMatrix3D &ccm,
			const Vector3D &v);

	Vector3D
	inverse_multiply(
			const CartesianConvMatrix3D &ccm,
			const Vector3D &v);
}

#endif  // _GPLATES_MATHS_CARTESIANCONVMATRIX3D_H_


