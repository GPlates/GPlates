/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 The University of Sydney, Australia
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

#ifndef _GPLATES_MATHS_UNITVECTOR3D_H_
#define _GPLATES_MATHS_UNITVECTOR3D_H_

#include <iosfwd>

#include "types.h"  /* real_t */
#include "Vector3D.h"
#include "GenericVectorOps3D.h"


namespace GPlatesMaths
{
	/** 
	 * A three-dimensional unit vector.
	 * Thus, the magnitude of this vector must be identical to 1.
	 * This invariant will be enforced upon construction (the values
	 * of x, y and z passed to the constructor will be checked), and
	 * assumed true for all subsequent usage.  No operations may be
	 * provided for this class which would allow the invariant to be
	 * violated.
	 * @invariant
	 *  - magnitude of vector is identical to 1
	 */
	class UnitVector3D
	{
	public:
		/**
		 * Create a 3D vector from the specified
		 * x, y and z components.
		 * @param x_comp The x-component.
		 * @param y_comp The y-component.
		 * @param z_comp The z-component.
		 *
		 * @throw ViolatedUnitVectorInvariantException if the
		 *   resulting vector does not have unit magnitude.
		 *
		 * FIXME:  This should become a 'create' function,
		 * invoking a private ctor.
		 */
		UnitVector3D(
				const real_t &x_comp,
				const real_t &y_comp,
				const real_t &z_comp);


		explicit
		UnitVector3D(
				const Vector3D &v);


		// FIXME:  Add a magnitude-checking cctor.


		~UnitVector3D() {  }


		const real_t &
		x() const {
			return d_x;
		}


		const real_t &
		y() const {
			return d_y;
		}


		const real_t &
		z() const {
			return d_z;
		}


		UnitVector3D &
		operator=(
				const UnitVector3D &u)
		{
			d_x = u.x();
			d_y = u.y();
			d_z = u.z();
			// FIXME: Check for accumulated magnitude errs.
			return *this;
		}


		static inline
		UnitVector3D
		xBasis()
		{
			return UnitVector3D(1.0, 0.0, 0.0);
		}

		static inline
		UnitVector3D
		yBasis()
		{
			return UnitVector3D(0.0, 1.0, 0.0);
		}


		static inline
		UnitVector3D
		zBasis()
		{
			return UnitVector3D(0.0, 0.0, 1.0);
		}

	private:

		/** 
		 * Assert the class invariant.
		 * @throw ViolatedUnitVectorInvariantException
		 *   if the invariant has been violated.
		 */
		void AssertInvariant(int line) const;

		real_t d_x, d_y, d_z;

	};


	inline
	const real_t
	dot(
			const UnitVector3D &u1,
			const UnitVector3D &u2)
	{
		return GenericVectorOps3D::dot(u1, u2);
	}


	inline
	bool
	operator==(
			const UnitVector3D &u1,
			const UnitVector3D &u2)
	{
		return (dot(u1, u2) >= 1.0);
	}


	inline
	bool
	operator!=(
			const UnitVector3D &u1,
			const UnitVector3D &u2)
	{
		return (dot(u1, u2) < 1.0);
	}


	inline
	bool
	perpendicular(
			const UnitVector3D &u1,
			const UnitVector3D &u2)
	{
		return GenericVectorOps3D::perpendicular(u1, u2);
	}


	inline
	bool
	parallel(
			const UnitVector3D &u,
			const Vector3D &v)
	{
		real_t dot_prod = GenericVectorOps3D::dot(u, v);
		return (dot_prod >= v.magnitude());
	}


	/**
	 * Evaluate whether the unit-vectors @a v1 and @a v2 are parallel.
	 */
	inline
	bool
	unit_vectors_are_parallel(
			const UnitVector3D &u1,
			const UnitVector3D &u2)
	{
		return (dot(u1, u2) >= 1.0);
	}


	/**
	 * Evaluate whether the unit-vectors @a v1 and @a v2 are antiparallel.
	 */
	inline
	bool
	unit_vectors_are_antiparallel(
			const UnitVector3D &u1,
			const UnitVector3D &u2)
	{
		return (dot(u1, u2) <= -1.0);
	}


	inline bool
	collinear(
			const UnitVector3D &s1,
			const UnitVector3D &s2)
	{
		real_t adp = abs(dot(s1, s2));
		return (adp >= 1.0);
	}


	inline
	const UnitVector3D
	operator-(
			const UnitVector3D &u)
	{
		return GenericVectorOps3D::negate(u);
	}


	const Vector3D
	operator*(
			const real_t &s, 
			const UnitVector3D &u);


	inline 
	const Vector3D
	operator*(
			const UnitVector3D &u,
			const real_t &s)
	{
		return (s * u);
	}


	/**
	 * Given the unit vector @a u, generate a unit vector perpendicular
	 * to it.
	 */
	UnitVector3D generate_perpendicular(
			const UnitVector3D &u);


	std::ostream &
	operator<<(
			std::ostream &os,
			const UnitVector3D &u);


	const Vector3D
	cross(
			const UnitVector3D &u1,
			const UnitVector3D &u2);


	const Vector3D
	cross(
			const UnitVector3D &u,
			const Vector3D &v);


	const Vector3D
	cross(
			const Vector3D &v,
			const UnitVector3D &u);


	/**
	 * This routine exports the Python wrapper class and associated functionality
	 */
	void export_UnitVector3D();
}

#endif  // _GPLATES_MATHS_UNITVECTOR3D_H_
