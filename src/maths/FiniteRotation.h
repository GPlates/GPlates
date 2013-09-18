/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_FINITEROTATION_H
#define GPLATES_MATHS_FINITEROTATION_H

#include <iosfwd>
#include <boost/optional.hpp>

#include "UnitQuaternion3D.h"
#include "Rotation.h"
#include "types.h"  /* real_t */

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/QtStreamable.h"


namespace GPlatesMaths
{
	// Forward declaration for the member function 'FiniteRotation::operator*'.
	class UnitVector3D;

	// Forward declarations for the non-member function 'operator*'.
	class MultiPointOnSphere;
	class PointOnSphere;
	class PolylineOnSphere;
	class PolygonOnSphere;
	class GeometryOnSphere;
	class GreatCircleArc;
	class GreatCircle;
	class SmallCircle;


	/** 
	 * This class represents a so-called "finite rotation" of plate tectonics.
	 *
	 * Plate tectonics theory states that the motion of plates on the surface of the globe can
	 * be described by "finite rotations".
	 *
	 * A finite rotation is a rotation about an "Euler pole" (a point on the surface of the
	 * globe, which is the intersection point of a rotation vector (the semi-axis of rotation)
	 * which extends from the centre of the globe), by an angular distance.
	 *
	 * An Euler pole is specified by a point on the surface of the globe.
	 *
	 * A rotation angle is specified in radians, with the usual sense of rotation:  a positive
	 * angle represents an anti-clockwise rotation around the rotation vector; a negative angle
	 * corresponds to a clockwise rotation.
	 */
	class FiniteRotation :
			// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
			public GPlatesUtils::QtStreamable<FiniteRotation>
	{

	public:

		/**
		 * Create a finite rotation with the Euler pole @a pole and rotation angle
		 * @a angle.
		 */
		static
		const FiniteRotation
		create(
				const PointOnSphere &pole,
				const real_t &angle);

		/**
		 * Create a finite rotation corresponding to the rotation effected by the
		 * unit quaternion @a uq.
		 */
		static
		const FiniteRotation
		create(
				const UnitQuaternion3D &uq,
				const boost::optional<UnitVector3D> &axis_hint_);

		/**
		 * Return a unit quaternion which would effect the rotation of this finite
		 * rotation.
		 */
		const UnitQuaternion3D &
		unit_quat() const
		{
			return d_unit_quat;
		}

		/**
		 * Return the axis hint.
		 */
		const boost::optional<UnitVector3D> &
		axis_hint() const
		{
			return d_axis_hint;
		}

		/**
		 * Apply this rotation to a unit vector @a unit_vect.
		 *
		 * This operation is not supposed to be symmetrical.
		 */
		const UnitVector3D
		operator*(
				const UnitVector3D &unit_vect) const;

		bool
		operator==(
				const FiniteRotation &other) const;

		bool
		operator!=(
				const FiniteRotation &other) const
		{
			return !(*this == other);
		}

	protected:

		explicit
		FiniteRotation(
				const UnitQuaternion3D &unit_quat_,
				const boost::optional<UnitVector3D> &axis_hint_);

	private:

		// This unit-quaternion is used to effect the rotation operation.
		UnitQuaternion3D d_unit_quat;

		// This provides a hint as to what the rotation axis might approx be.
		boost::optional<UnitVector3D> d_axis_hint;

		// These values are used to optimise rotation operations.
		real_t   d_d;
		Vector3D d_e;

	};


	/**
	 * Calculate the reverse of the given finite rotation @a r.
	 */
	inline
	const FiniteRotation
	get_reverse(
			const FiniteRotation &r)
	{
		return FiniteRotation::create(r.unit_quat().get_inverse(), r.axis_hint());
	}


	/**
	 * Calculate the finite rotation which is the interpolation of the finite rotations @a r1
	 * and @a r2 according to the interpolation parameters @a t1, @a t2 and @a t_target.
	 *
	 * The parameters @a t1 and @a t2 correspond to @a r1 and @a r2, respectively; @a t_target
	 * corresponds to the result of the interpolation.  The ratio of the difference between
	 * @a r1 and the interpolated result to the difference between the interpolated result and
	 * @a r2 will be equal to the ratio of the difference between @a t1 and @a t_target to the
	 * difference between @a t_target and @a t2.
	 *
	 * Obviously, no interpolation can occur if the value of @a t1 is equal to the value of
	 * @a t2 -- intuitively, because there is no difference between the values; and
	 * arithmetically, because a divide-by-zero would occur when calculating the ratio of the
	 * interpolation.
	 *
	 * Hence, if the value of @a t1 is equal to the value of @a t2, an
	 * @a IndeterminateResultException will be thrown.
	 *
	 * Note that @em any real-valued floating-point value is acceptable as the value of
	 * @a t_target, whether between @a t1 and @a t2, equal to either of them, or less-than or
	 * greater-than both of them.
	 *
	 * This operation invokes the awesome power of quaternion SLERP (spherical linear
	 * interpolation).
	 */
	const FiniteRotation
	interpolate(
			const FiniteRotation &r1,
			const FiniteRotation &r2,
			const real_t &t1,
			const real_t &t2,
			const real_t &t_target,
			const boost::optional<UnitVector3D> &axis_hint);

	/**
	 * Calculate a spatial interpolated rotation between three finite rotations r1, r2 and r3,
	 * using associated barycentric coordinate weights w1, w2 and w3.
	 *
	 * Note that the weights must sum to 1.0.
	 */ 
	const FiniteRotation
	interpolate(
			const FiniteRotation &r1,
			const FiniteRotation &r2,
			const FiniteRotation &r3,
			const real_t &w1,
			const real_t &w2,
			const real_t &w3);

	/**
	 * Compose two FiniteRotations.
	 *
	 * Note: order of composition is important!
	 * Quaternion multiplication is not commutative!
	 * This operation is not commutative!
	 *
	 * This composition of rotations is very much in the style of matrix
	 * composition by premultiplication:  You take 'r2', then apply 'r1'
	 * to it (in front of it).
	 *
	 * If r1 describes the rotation of a moving plate 'M1' with respect
	 * to a fixed plate 'F1', and r2 describes the rotation of a moving
	 * plate 'M2' with respect to 'F2', then:
	 *
	 *  + M1 should equal F2  ("should equal" instead of "must equal",
	 *     since this function cannot enforce this equality).
	 *
	 *  + if the result of this operation is called 'rr', then rr will
	 *     describe the motion of the moving plate M2 with respect to
	 *     the fixed plate F1.  Thus, the unit vector which is rotated
	 *     by the resulting finite rotation will "sit" on M2.
	 *
	 * If these finite rotations are considered the branches of a tree-
	 * like hierarchy of plate-motion (with the stationary "globe" at
	 * the root of the tree, and the motion of any given plate specified
	 * relative to the plate root-ward of it), then the finite rotation
	 * r1 should be one branch root-ward of the finite rotation r2.
	 */
	const FiniteRotation
	compose(
	 const FiniteRotation &r1,
	 const FiniteRotation &r2);


	/**
	 * Apply a Rotation to a FiniteRotation.
	 *
	 * Note: order of composition is important!
	 * Quaternion multiplication is not commutative!
	 * This operation is not commutative!
	 *
	 * This composition of rotations is very much in the style of matrix
	 * composition by premultiplication:  You take 'fr', then apply 'r'
	 * to it (in front of it).
	 *
	 * Note that, in contrast to the composition of two FiniteRotations
	 * (which is used in the building of the ReconstructionTree), the
	 * composition of a Rotation onto a FiniteRotation is intended for use
	 * in the interactive manipulation of total reconstruction poles.  As
	 * the user drags geometries around on the globe (thus accumulating
	 * rotations), the FiniteRotation will be modified.
	 */
	const FiniteRotation
	compose(
			const Rotation &r,
			const FiniteRotation &fr);


	/**
	 * Apply the given rotation to the given point-on-sphere.
	 *
	 * This operation is not supposed to be symmetrical.
	 */
	const PointOnSphere
	operator*(
			const FiniteRotation &r,
			const PointOnSphere &p);


	/**
	 * Apply the given rotation to the given intrusive-pointer to point-on-sphere.
	 *
	 * This operation is not supposed to be symmetrical.
	 */
	const GPlatesUtils::non_null_intrusive_ptr<const PointOnSphere>
	operator*(
			const FiniteRotation &r,
			const GPlatesUtils::non_null_intrusive_ptr<const PointOnSphere> &p);


	/**
	 * Apply the given rotation to the given intrusive-pointer to multi-point-on-sphere.
	 *
	 * This operation is not supposed to be symmetrical.
	 */
	const GPlatesUtils::non_null_intrusive_ptr<const MultiPointOnSphere>
	operator*(
			const FiniteRotation &r,
			const GPlatesUtils::non_null_intrusive_ptr<const MultiPointOnSphere> &mp);


	/**
	 * Apply the given rotation to the given intrusive-pointer to polyline.
	 *
	 * This operation is not supposed to be symmetrical.
	 */
	const GPlatesUtils::non_null_intrusive_ptr<const PolylineOnSphere>
	operator*(
			const FiniteRotation &r,
			const GPlatesUtils::non_null_intrusive_ptr<const PolylineOnSphere> &p);


	/**
	 * Apply the given rotation to the given intrusive-pointer to polygon.
	 *
	 * This operation is not supposed to be symmetrical.
	 */
	const GPlatesUtils::non_null_intrusive_ptr<const PolygonOnSphere>
	operator*(
			const FiniteRotation &r,
			const GPlatesUtils::non_null_intrusive_ptr<const PolygonOnSphere> &p);


	/**
	 * Apply the given rotation to the given intrusive-pointer to @a GeometryOnSphere.
	 *
	 * This operation is not supposed to be symmetrical.
	 */
	const GPlatesUtils::non_null_intrusive_ptr<const GeometryOnSphere>
	operator*(
			const FiniteRotation &r,
			const GPlatesUtils::non_null_intrusive_ptr<const GeometryOnSphere> &g);


	/**
	 * Apply the given rotation to the given great circle arc.
	 *
	 * This operation is not supposed to be symmetrical.
	 */
	const GreatCircleArc
	operator*(
			const FiniteRotation &r,
			const GreatCircleArc &g);


	/**
	 * Apply the given rotation to the given great circle.
	 *
	 * This operation is not supposed to be symmetrical.
	 */
	const GreatCircle
	operator*(
			const FiniteRotation &r,
			const GreatCircle &g);


	/**
	 * Apply the given rotation to the given small circle.
	 *
	 * This operation is not supposed to be symmetrical.
	 */
	const SmallCircle
	operator*(
			const FiniteRotation &r,
			const SmallCircle &s);


	std::ostream &
	operator<<(
			std::ostream &os,
			const FiniteRotation &fr);

}

#endif  // GPLATES_MATHS_FINITEROTATION_H
