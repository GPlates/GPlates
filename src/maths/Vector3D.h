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

#ifndef _GPLATES_MATHS_VECTOR3D_H_
#define _GPLATES_MATHS_VECTOR3D_H_

#include <iosfwd>

#include "types.h"  /* real_t */
#include "GenericVectorOps3D.h"

#include "utils/QtStreamable.h"


namespace GPlatesMaths
{
	class UnitVector3D;

	/** 
	 * A three-dimensional vector.
	 * In contrast to UnitVector3D and DirVector3D, there are no
	 * conditions upon this vector.  Its components may take any values.
	 * It may be of any magnitude.  Hence, there is no invariant which
	 * must be maintained.
	 */
	class Vector3D :
			// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
			public GPlatesUtils::QtStreamable<Vector3D>
	{
	public:

		/**
		 * Zero vector.
		 */
		Vector3D() :
			d_x(0.0),
			d_y(0.0),
			d_z(0.0)
		{  }

		/**
		 * Create a 3D vector from the specified
		 * x, y and z components.
		 * @param x_comp The x-component.
		 * @param y_comp The y-component.
		 * @param z_comp The z-component.
		 */
		Vector3D(
				const real_t& x_,
				const real_t& y_,
				const real_t& z_) :
			d_x(x_),
			d_y(y_),
			d_z(z_)
		{  }


		explicit
		Vector3D(
				const UnitVector3D &u);


		~Vector3D() {  }


		const real_t &
		x() const
		{
			return d_x;
		}

		const real_t &
		y() const
		{
			return d_y;
		}

		const real_t &
		z() const
		{
			return d_z;
		}

		/**
		 * Returns the square of the magnitude; that is,
		 * \f$ ( x^2 + y^2 + z^2 ) \f$
		 */
		real_t
		magSqrd() const
		{
			// Using double (dval()) instead of real_t generates more efficient assembly code.
			return d_x.dval() * d_x.dval() + d_y.dval() * d_y.dval() + d_z.dval() * d_z.dval();
		}

		/**
		 * Returns the magnitude of the vector; that is,
		 * \f$ \sqrt{x^2 + y^2 + z^2} \f$
		 */
		real_t
		magnitude() const
		{
			return sqrt(magSqrd());
		}

		/**
		 * Returns true if the magnitude is zero, or close enough to zero that
		 * @a get_normalisation would throw @a UnableToNormaliseZeroVectorException.
		 */
		bool
		is_zero_magnitude() const;

		/**
		 * Generate a vector having the same direction as @a this,
		 * but which has unit magnitude.
		 *
		 * @throws UnableToNormaliseZeroVectorException if @a this has zero magnitude.
		 * If @a is_zero_magnitude returns true then this exception will get thrown.
		 */
		UnitVector3D
		get_normalisation() const;

	protected:
		real_t d_x, d_y, d_z;
	};
}


#include "UnitVector3D.h"


namespace GPlatesMaths
{
	inline
	Vector3D::Vector3D(
			const UnitVector3D &u) :
		d_x(u.x()),
		d_y(u.y()),
		d_z(u.z())
	{  }


	inline
	const real_t
	dot(
		const Vector3D &v1,
		const Vector3D &v2)
	{
		return GenericVectorOps3D::dot(v1, v2);
	}


	inline
	bool
	operator==(
			const Vector3D &v1,
			const Vector3D &v2)
	{
		return (v1.x() == v2.x()
		     && v1.y() == v2.y()
		     && v1.z() == v2.z());
	}


	inline
	bool
	operator!=(
			const Vector3D &v1,
			const Vector3D &v2)
	{
		return (v1.x() != v2.x()
		     || v1.y() != v2.y()
		     || v1.z() != v2.z());
	}


	inline
	const Vector3D
	operator-(
			const Vector3D &v)
	{
		return GenericVectorOps3D::negate(v);
	}


	inline
	const Vector3D
	operator*(
			const real_t &s,
			const Vector3D &v)
	{
		return GenericVectorOps3D::ReturnType<Vector3D>::scale(s, v);
	}


	inline
	const Vector3D
	operator*(
		const Vector3D &v,
		const real_t &s)
	{
		return s * v;
	}


	inline
	Vector3D
	operator+(
			const Vector3D &v1,
			const Vector3D &v2)
	{
		return Vector3D(
				v1.x().dval() + v2.x().dval(),
				v1.y().dval() + v2.y().dval(),
				v1.z().dval() + v2.z().dval());
	}


	inline
	Vector3D
	operator-(
			const Vector3D &v1,
			const Vector3D &v2)
	{
		return Vector3D(
				v1.x().dval() - v2.x().dval(),
				v1.y().dval() - v2.y().dval(),
				v1.z().dval() - v2.z().dval());
	}


	std::ostream &
	operator<<(
			std::ostream &os,
			const Vector3D &v);


	/**
	 * This algorithm for testing whether two vectors are parallel
	 * is intended to remove the requirements that:
	 *  - the magnitudes of the vectors are already known
	 *  - any of the components are non-zero.
	 *
	 * Note that zero vectors are defined to be parallel to everything.
	 */
	inline
	bool
	parallel(
			const Vector3D &v1,
			const Vector3D &v2)
	{
		real_t dp = dot(v1, v2);
		return dp >= v1.magnitude() * v2.magnitude();
	}

	/**
	 * Test whether two vectors are perpendicular.
	 */
	inline
	bool
	perpendicular(
			const Vector3D &v1,
			const Vector3D &v2)
	{
		return GenericVectorOps3D::perpendicular(v1, v2);
	}


	/**
	 * This algorithm for testing whether two vectors are collinear
	 * (ie. parallel or antiparallel) is intended to remove the
	 * requirements that:
	 *  - the magnitudes of the vectors are already known
	 *  - any of the components are non-zero.
	 *
	 * Note that zero vectors are defined to be collinear to everything.
	 */
	inline
	bool
	collinear(
			const Vector3D &v1,
			const Vector3D &v2)
	{
		real_t adp = abs(dot(v1, v2));
		return adp >= v1.magnitude() * v2.magnitude();
	}


	/**
	 * Returns cross product of two vectors.
	 */
	const Vector3D
	cross(
			const Vector3D &v1,
			const Vector3D &v2);
}

#endif  // _GPLATES_MATHS_VECTOR3D_H_
