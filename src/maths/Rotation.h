/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_ROTATION_H
#define GPLATES_MATHS_ROTATION_H

#include "UnitVector3D.h"
#include "UnitQuaternion3D.h"
#include "PointOnSphere.h"
#include "types.h"  // real_t


namespace GPlatesMaths
{
	using namespace GPlatesGlobal;

	// Forward declarations for the non-member function 'operator*'.
	class MultiPointOnSphere;
	class PointOnSphere;
	class PolylineOnSphere;
	class PolygonOnSphere;
	class GeometryOnSphere;


	/** 
	 * Represents a rotation by a particular angle about a particular axis.
	 *
	 * NOTE: This class should NOT be used for plate tectonic rotations
	 * -- for those rotations, you should use the 'FiniteRotation' and
	 * 'StageRotation' classes.  The rotations effected by this class are
	 * independent of time.
	 *
	 * Rotation angles are specified in radians, with the usual sense of
	 * rotation:  a positive angle represents an anti-clockwise rotation
	 * around the rotation vector;  a negative angle corresponds to a
	 * clockwise rotation.
	 */
	class Rotation
	{
		/*
		 * Declare this as a friend to enable it to use
		 * the protected creation function.
		 */
		friend
		const Rotation
		operator*(
				const Rotation &r1,
				const Rotation &r2);

	public:
		/**
		 * Create a rotation with the given rotation axis
		 * and rotation angle.
		 *
		 * As always, the rotation angle is in radians.
		 */
		static
		const Rotation
		create(
				const UnitVector3D &rotation_axis,
				const real_t &rotation_angle);


		/**
		 * Create a rotation which transforms @a initial to @a final.
		 */
		static inline
		const Rotation
		create(
				const PointOnSphere &initial,
				const PointOnSphere &final)
		{
			return Rotation::create(initial.position_vector(), final.position_vector());
		}


		/**
		 * Create a rotation which transforms @a initial to @a final.
		 */
		static
		const Rotation
		create(
				const UnitVector3D &initial,
				const UnitVector3D &final);


		const UnitVector3D &
		axis() const
		{
			return d_axis;
		}


		const real_t &
		angle() const
		{
			return d_angle;
		}


		const UnitQuaternion3D &
		quat() const
		{
			return d_quat;
		}


		/**
		 * Return the reverse of this rotation.
		 */
		const Rotation
		get_reverse() const
		{
			UnitQuaternion3D reversed_quat = quat().get_inverse();
			return Rotation::create(reversed_quat, axis(), -angle());
		}


		/**
		 * Apply this rotation to a unit vector.
		 *
		 * Note that this function is a member function for
		 * two (2) reasons:
		 *
		 *  (i) to enable it to access the private member data
		 *       '_d' and '_e'.
		 *
		 *  (ii) to enforce the concept that the operation of
		 *        a finite rotation is APPLIED TO a vector --
		 *        it is very much a PREmultiplication, in the
		 *        style of traditional matrix operations.
		 *
		 * This operation is not supposed to be symmetrical.
		 */
		const UnitVector3D
		operator*(
				const UnitVector3D &uv) const;

		/**
		 * Apply this rotation to a (not necessarily unit) vector.
		 *
		 * Note that this function is a member function for
		 * two (2) reasons:
		 *
		 *  (i) to enable it to access the private member data
		 *       '_d' and '_e'.
		 *
		 *  (ii) to enforce the concept that the operation of
		 *        a finite rotation is APPLIED TO a vector --
		 *        it is very much a PREmultiplication, in the
		 *        style of traditional matrix operations.
		 *
		 * This operation is not supposed to be symmetrical.
		 */
		const Vector3D
		operator*(
				const Vector3D &v) const;

	protected:
		/**
		 * Create the rotation described by the supplied
		 * quaternion.  The supplied axis and angle
		 * must match those inside the quaternion.
		 * This requirement will not be checked!
		 *
		 * As always, the rotation angle is in radians.
		 */
		static
		const Rotation
		create(
				const UnitQuaternion3D &uq,
				const UnitVector3D &rotation_axis,
				const real_t &rotation_angle);


		Rotation(
				const UnitVector3D &axis_,
				const real_t &angle_,
				const UnitQuaternion3D &quat_,
				const real_t &d,
				const Vector3D &e):
			d_axis(axis_),
			d_angle(angle_),
			d_quat(quat_),
			d_d(d),
			d_e(e)
		{  }

	private:
		/**
		 * The axis of the rotation.
		 */
		UnitVector3D     d_axis;

		/**
		 * The angle of the rotation, in radians.
		 */
		real_t           d_angle;

		/**
		 * The unit quaternion which effects the rotation
		 * described by the rotation axis and angle.
		 */
		UnitQuaternion3D d_quat;

		/*
		 * And now for the mysterious values of 'd_d' and 'd_e' !
		 *
		 * These are only used to rotate points on the sphere,
		 * and are calculated purely for optimisation purposes.
		 * I don't know whether they have any physical meaning.
		 * I suspect not.
		 */
		real_t   d_d;
		Vector3D d_e;
	};


	/**
	 * Compose two Rotations.
	 *
	 * Note: order of composition is important!
	 * Quaternion multiplication is not commutative!
	 * This operation is not commutative!
	 *
	 * This composition of rotations is very much in the style of matrix
	 * composition by premultiplication:  You take 'r2', then apply 'r1'
	 * to it (in front of it).
	 */
	const Rotation
	operator*(
			const Rotation &r1,
			const Rotation &r2);


	/**
	 * Apply the given rotation to the given point-on-sphere.
	 *
	 * This operation is not supposed to be symmetrical.
	 */
	const PointOnSphere
	operator*(
			const Rotation &r,
			const PointOnSphere &p);


	/**
	 * Apply the given rotation to the given intrusive-pointer to point-on-sphere.
	 *
	 * This operation is not supposed to be symmetrical.
	 */
	const GPlatesUtils::non_null_intrusive_ptr<const PointOnSphere>
	operator*(
			const Rotation &r,
			const GPlatesUtils::non_null_intrusive_ptr<const PointOnSphere> &p);


	/**
	 * Apply the given rotation to the given intrusive-pointer to multi-point-on-sphere.
	 *
	 * This operation is not supposed to be symmetrical.
	 */
	const GPlatesUtils::non_null_intrusive_ptr<const MultiPointOnSphere>
	operator*(
			const Rotation &r,
			const GPlatesUtils::non_null_intrusive_ptr<const MultiPointOnSphere> &mp);


	/**
	 * Apply the given rotation to the given intrusive-pointer to polyline.
	 *
	 * This operation is not supposed to be symmetrical.
	 */
	const GPlatesUtils::non_null_intrusive_ptr<const PolylineOnSphere>
	operator*(
			const Rotation &r,
			const GPlatesUtils::non_null_intrusive_ptr<const PolylineOnSphere> &p);


	/**
	 * Apply the given rotation to the given intrusive-pointer to polygon.
	 *
	 * This operation is not supposed to be symmetrical.
	 */
	const GPlatesUtils::non_null_intrusive_ptr<const PolygonOnSphere>
	operator*(
			const Rotation &r,
			const GPlatesUtils::non_null_intrusive_ptr<const PolygonOnSphere> &p);


	/**
	 * Apply the given rotation to the given intrusive-pointer to @a GeometryOnSphere.
	 *
	 * This operation is not supposed to be symmetrical.
	 */
	const GPlatesUtils::non_null_intrusive_ptr<const GeometryOnSphere>
	operator*(
			const Rotation &r,
			const GPlatesUtils::non_null_intrusive_ptr<const GeometryOnSphere> &g);
}

#endif  // GPLATES_MATHS_ROTATION_H
