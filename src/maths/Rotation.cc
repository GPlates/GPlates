/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 The University of Sydney, Australia
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

#include <sstream>
#include <cmath>
#include <boost/none.hpp>

#include "Rotation.h"

#include "ConstGeometryOnSphereVisitor.h"
#include "InvalidOperationException.h"
#include "MathsUtils.h"
#include "Vector3D.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


namespace
{
	/**
	 * Visits a @a GeometryOnSphere, rotates it and returns as a @a GeometryOnSphere.
	 */
	class RotateGeometryOnSphere :
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		/**
		 * Construct with the @a Rotation to use for rotating.
		 */
		explicit
		RotateGeometryOnSphere(
				const GPlatesMaths::Rotation &rotation) :
			d_rotation(rotation)
		{  }


		/**
		 * Rotates @a geometry using @a Rotation passed into constructor
		 * and returns rotated @a GeometryOnSphere.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		rotate(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry)
		{
			d_rotated_geometry = boost::none;

			geometry->accept_visitor(*this);

			// Unless there's a new derived type of GeometryOnSphere we should
			// be able to dereference 'd_rotated_geometry'.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_rotated_geometry, GPLATES_ASSERTION_SOURCE);

			return *d_rotated_geometry;
		}

	protected:
		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			d_rotated_geometry = d_rotation * multi_point_on_sphere;
		}

		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			d_rotated_geometry = d_rotation * point_on_sphere;
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			d_rotated_geometry = d_rotation * polygon_on_sphere;
		}

		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			d_rotated_geometry = d_rotation * polyline_on_sphere;
		}

	private:
		const GPlatesMaths::Rotation &d_rotation;
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> d_rotated_geometry;
	};
}


const GPlatesMaths::Rotation
GPlatesMaths::Rotation::create(
		const UnitVector3D &rotation_axis,
		const real_t &rotation_angle)
{
	UnitQuaternion3D uq = UnitQuaternion3D::create_rotation(rotation_axis, rotation_angle);

	return Rotation(rotation_axis, rotation_angle, uq);
}


const GPlatesMaths::Rotation
GPlatesMaths::Rotation::create(
		const UnitVector3D &initial,
		const UnitVector3D &final)
{
	real_t dp = dot(initial, final);
	if (abs(dp) >= 1.0) {

		/*
		 * The unit-vectors 'initial' and 'final' are collinear.
		 * This means they do not define a unique plane, which means
		 * it is not possible to determine a unique axis of rotation.
		 * We will need to pick some arbitrary values out of the air.
		 */
		if (dp >= 1.0) {

			/*
			 * The unit-vectors are parallel.
			 * Hence, the rotation which transforms 'initial'
			 * into 'final' is the identity rotation.
			 * 
			 * The identity rotation is represented by a rotation
			 * of zero radians about any arbitrary axis. 
			 * 
			 * For simplicity, let's use 'initial' as the axis.
			 */
			return Rotation::create(initial, 0.0);

		} else {

			/*
			 * The unit-vectors are anti-parallel.
			 * Hence, the rotation which transforms 'initial'
			 * into 'final' is the reflection rotation.
			 *
			 * The reflection rotation is represented by a rotation
			 * of PI radians about any arbitrary axis orthogonal to
			 * 'initial'.
			 */
			UnitVector3D axis = generate_perpendicular(initial);
			return Rotation::create(axis, PI);
		}

	} else {

		/*
		 * The unit-vectors 'initial' and 'final' are NOT collinear.
		 * This means they DO define a unique plane, with a unique
		 * axis about which 'initial' may be rotated to become 'final'.
		 *
		 * Since the unit-vectors are not collinear, the result of
		 * their cross-product will be a vector of non-zero length,
		 * which we can safely normalise.
		 */
		UnitVector3D axis = cross(initial, final).get_normalisation();
		real_t angle = acos(dp);
		return Rotation::create(axis, angle);
	}
}


const GPlatesMaths::Rotation
GPlatesMaths::Rotation::create_identity_rotation()
{
	UnitQuaternion3D uq = UnitQuaternion3D::create_identity_rotation();

	// Since it's an identity rotation we can use any axis we like (because rotation angle is zero).
	return Rotation(UnitVector3D::zBasis(), 0, uq);
}


const GPlatesMaths::UnitVector3D
GPlatesMaths::Rotation::operator*(
		const UnitVector3D &uv) const
{
	// Re-use the operator associated with 'Vector3D'.
	const Vector3D v = operator*(Vector3D(uv));

	return UnitVector3D(v.x(), v.y(), v.z());
}

const GPlatesMaths::Vector3D
GPlatesMaths::Rotation::operator*(
		const Vector3D &v) const
{
	const GPlatesMaths::real_t &uq_s = d_quat.scalar_part();
	const Vector3D &uq_v = d_quat.vector_part();

	//
	// Quaternion (uq_s, uq_v) rotates vector v to v' as:
	//
	//   v' = v + 2 * uq_v x (uq_s * v + uq_v x v)
	//
	// ...and using the vector triple product rule:
	//
	//   a x (b x c) = (a.c)b - (a.b)c
	//
	// ...we get:
	//
	//   v' = v + 2 * uq_s * uq_v x v + 2 * uq_v x (uq_v x v)
	//      = v + 2 * uq_s * uq_v x v + 2 * (uq_v . v) * uq_v - 2 * (uq_v . uq_v) * v
	//      = (1 - 2 * (uq_v . uq_v)) * v + 2 * uq_s * uq_v x v + 2 * (uq_v . v) * uq_v
	//
	// ...and using the norm of a unit quaternion:
	//
	//   uq_s * uq_s + uq_v . uq_v = 1
	//                 uq_v . uq_v = 1 - uq_s * uq_s
	//       1 - 2 * (uq_v . uq_v) = 1 - 2 * (1 - uq_s * uq_s)
	//                             = 2 * uq_s * uq_s - 1
	//
	// ...we get:
	//
	//   v' = (1 - 2 * (uq_v . uq_v)) * v + 2 * uq_s * uq_v x v + 2 * (uq_v . v) * uq_v
	//      = (2 * uq_s * uq_s - 1) * v + 2 * uq_s * uq_v x v + 2 * (uq_v . v) * uq_v
	//      = (2 * uq_s * uq_s - 1) * v + 2 * [uq_s * uq_v x v + (uq_v . v) * uq_v]
	//
	return (2.0 * uq_s * uq_s - 1.0) * v + 2.0 * (cross(uq_s * uq_v, v) + dot(uq_v, v) * uq_v);
}


const GPlatesMaths::Rotation
GPlatesMaths::operator*(
		const Rotation &r1,
		const Rotation &r2)
{
	UnitQuaternion3D resultant_uq = r1.quat() * r2.quat();
	if (represents_identity_rotation(resultant_uq)) {

		/*
		 * The identity rotation is represented by a rotation of
		 * zero radians about any arbitrary axis.
		 *
		 * For simplicity, let's use the axis of 'r1' as the axis.
		 */
		return Rotation::create(resultant_uq, r1.axis(), 0.0);

	} else {

		/*
		 * The resultant quaternion has a clearly-defined axis
		 * and a non-zero angle of rotation.
		 */
		UnitQuaternion3D::RotationParams params =
				// Use the axis of 'r1' as the axis hint...
				resultant_uq.get_rotation_params(r1.axis());

		return Rotation::create(resultant_uq, params.axis, params.angle);
	}
}


const GPlatesMaths::PointOnSphere
GPlatesMaths::operator*(
		const Rotation &r,
		const PointOnSphere &p) {

	UnitVector3D rotated_position_vector = r * p.position_vector();
	return PointOnSphere(rotated_position_vector);
}


const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere>
GPlatesMaths::operator*(
		const Rotation &r,
		const GPlatesUtils::non_null_intrusive_ptr<const PointOnSphere> &p)
{
	UnitVector3D rotated_position_vector = r * p->position_vector();
	return PointOnSphere::create_on_heap(rotated_position_vector);
}


const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::MultiPointOnSphere>
GPlatesMaths::operator*(
		const Rotation &r,
		const GPlatesUtils::non_null_intrusive_ptr<const MultiPointOnSphere> &mp)
{
	std::vector<PointOnSphere> rotated_points;
	rotated_points.reserve(mp->number_of_points());

	MultiPointOnSphere::const_iterator iter = mp->begin();
	MultiPointOnSphere::const_iterator end = mp->end();
	for ( ; iter != end; ++iter) {
		rotated_points.push_back(r * (*iter));
	}

	return MultiPointOnSphere::create_on_heap(rotated_points);
}


const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PolylineOnSphere>
GPlatesMaths::operator*(
		const Rotation &r,
		const GPlatesUtils::non_null_intrusive_ptr<const PolylineOnSphere> &p)
{
	std::vector<PointOnSphere> rotated_points;
	rotated_points.reserve(p->number_of_vertices());

	PolylineOnSphere::vertex_const_iterator iter = p->vertex_begin();
	PolylineOnSphere::vertex_const_iterator end = p->vertex_end();
	for ( ; iter != end; ++iter) {
		rotated_points.push_back(r * (*iter));
	}

	return PolylineOnSphere::create_on_heap(rotated_points);
}


const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PolygonOnSphere>
GPlatesMaths::operator*(
		const Rotation &r,
		const GPlatesUtils::non_null_intrusive_ptr<const PolygonOnSphere> &p)
{
	std::vector<PointOnSphere> rotated_exterior_ring;
	rotated_exterior_ring.reserve(p->number_of_vertices_in_exterior_ring());

	// Rotate the exterior ring.
	PolygonOnSphere::ring_vertex_const_iterator exterior_iter = p->exterior_ring_vertex_begin();
	PolygonOnSphere::ring_vertex_const_iterator exterior_end = p->exterior_ring_vertex_end();
	for ( ; exterior_iter != exterior_end; ++exterior_iter)
	{
		rotated_exterior_ring.push_back(PointOnSphere(r * exterior_iter->position_vector()));
	}

	const unsigned int num_interior_rings = p->number_of_interior_rings();
	if (num_interior_rings == 0)
	{
		return PolygonOnSphere::create_on_heap(rotated_exterior_ring);
	}

	std::vector< std::vector<PointOnSphere> > rotated_interior_rings(num_interior_rings);

	// Rotate the interior rings.
	for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; interior_ring_index++)
	{
		rotated_interior_rings[interior_ring_index].reserve(p->number_of_vertices_in_interior_ring(interior_ring_index));

		PolygonOnSphere::ring_vertex_const_iterator interior_iter = p->interior_ring_vertex_begin(interior_ring_index);
		PolygonOnSphere::ring_vertex_const_iterator interior_end = p->interior_ring_vertex_end(interior_ring_index);
		for ( ; interior_iter != interior_end; ++interior_iter)
		{
			rotated_interior_rings[interior_ring_index].push_back(PointOnSphere(r * interior_iter->position_vector()));
		}
	} // loop over interior rings

	return PolygonOnSphere::create_on_heap(rotated_exterior_ring, rotated_interior_rings);
}


const GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::GeometryOnSphere>
GPlatesMaths::operator*(
		const Rotation &r,
		const GPlatesUtils::non_null_intrusive_ptr<const GeometryOnSphere> &g)
{
	return RotateGeometryOnSphere(r).rotate(g);
}
