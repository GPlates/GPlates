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
#include "Vector3D.h"
#include "ConstGeometryOnSphereVisitor.h"
#include "MathsUtils.h"
#include "InvalidOperationException.h"


namespace
{
	inline
	const GPlatesMaths::real_t
	calculate_d_value(
			const GPlatesMaths::UnitQuaternion3D &uq)
	{
		const GPlatesMaths::real_t &s = uq.scalar_part();
		const GPlatesMaths::Vector3D &v = uq.vector_part();

		return ((s * s) - dot(v, v));
	}


	inline
	const GPlatesMaths::Vector3D
	calculate_e_value(
			const GPlatesMaths::UnitQuaternion3D &uq)
	{
		const GPlatesMaths::real_t &s = uq.scalar_part();
		const GPlatesMaths::Vector3D &v = uq.vector_part();

		return ((2.0 * s) * v);
	}
}


const GPlatesMaths::Rotation
GPlatesMaths::Rotation::create(
		const UnitVector3D &rotation_axis,
		const real_t &rotation_angle)
{
	UnitQuaternion3D uq = UnitQuaternion3D::create_rotation(rotation_axis, rotation_angle);

	// These values are used to optimise rotation of points on the sphere.
	real_t d = calculate_d_value(uq);
	Vector3D e = calculate_e_value(uq);

	return Rotation(rotation_axis, rotation_angle, uq, d, e);
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


const GPlatesMaths::UnitVector3D
GPlatesMaths::Rotation::operator*(
		const UnitVector3D &uv) const
{
	Vector3D v(uv);

	Vector3D v_rot = d_d * v +
			(2.0 * dot(d_quat.vector_part(), v)) * d_quat.vector_part() +
			cross(d_e, v);

	return UnitVector3D(v_rot.x(), v_rot.y(), v_rot.z());
}


const GPlatesMaths::Rotation
GPlatesMaths::Rotation::create(
		const UnitQuaternion3D &uq,
		const UnitVector3D &rotation_axis,
		const real_t &rotation_angle)
{
	// These values are used to optimise rotation of points on the sphere.
	real_t d = calculate_d_value(uq);
	Vector3D e = calculate_e_value(uq);

	return Rotation(rotation_axis, rotation_angle, uq, d, e);
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
				 resultant_uq.get_rotation_params(boost::none);

		return Rotation::create(resultant_uq, params.axis, params.angle);
	}
}

namespace
{
	const GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type
	rotate_multi_point(
			const GPlatesMaths::Rotation &rotation,
			GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point)
	{
		using namespace GPlatesMaths;

		std::vector<PointOnSphere> rotated_points;
		rotated_points.reserve(multi_point->number_of_points());

		MultiPointOnSphere::const_iterator iter = multi_point->begin();
		MultiPointOnSphere::const_iterator end = multi_point->end();
		for ( ; iter != end; ++iter) {
			rotated_points.push_back(rotation * (*iter));
		}

		MultiPointOnSphere::non_null_ptr_to_const_type rotated_multi_point(
				MultiPointOnSphere::create_on_heap(rotated_points));
		return rotated_multi_point;
	}


	const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
	rotate_point(
			const GPlatesMaths::Rotation &rotation,
			GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point)
	{
		using namespace GPlatesMaths;

		UnitVector3D rotated_position_vector = rotation * point->position_vector();
		PointOnSphere::non_null_ptr_to_const_type rotated_point(
				PointOnSphere::create_on_heap(rotated_position_vector));
		return rotated_point;
	}


	const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
	rotate_polygon(
			const GPlatesMaths::Rotation &rotation,
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon)
	{
		using namespace GPlatesMaths;

		std::vector<PointOnSphere> rotated_points;
		rotated_points.reserve(polygon->number_of_vertices());

		PolygonOnSphere::vertex_const_iterator iter = polygon->vertex_begin();
		PolygonOnSphere::vertex_const_iterator end = polygon->vertex_end();
		for ( ; iter != end; ++iter) {
			rotated_points.push_back(rotation * (*iter));
		}

		PolygonOnSphere::non_null_ptr_to_const_type rotated_polygon(
				PolygonOnSphere::create_on_heap(rotated_points));
		return rotated_polygon;
	}


	const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
	rotate_polyline(
			const GPlatesMaths::Rotation &rotation,
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline)
	{
		using namespace GPlatesMaths;

		std::vector<PointOnSphere> rotated_points;
		rotated_points.reserve(polyline->number_of_vertices());

		PolylineOnSphere::vertex_const_iterator iter = polyline->vertex_begin();
		PolylineOnSphere::vertex_const_iterator end = polyline->vertex_end();
		for ( ; iter != end; ++iter) {
			rotated_points.push_back(rotation * (*iter));
		}

		PolylineOnSphere::non_null_ptr_to_const_type rotated_polyline(
				PolylineOnSphere::create_on_heap(rotated_points));
		return rotated_polyline;
	}


	/**
	 * This is a Visitor to rotate a geometry-on-sphere.
	 */
	class GeometryOnSphereRotater:
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		explicit
		GeometryOnSphereRotater(
				const GPlatesMaths::Rotation &r):
			d_rotation(r)
		{  }

		~GeometryOnSphereRotater()
		{  }

		const GPlatesMaths::GeometryOnSphere::maybe_null_ptr_to_const_type
		rotated_geometry() const
		{
			return d_rotated_geometry;
		}

		// Please keep these geometries ordered alphabetically.

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			d_rotated_geometry = rotate_multi_point(d_rotation, multi_point_on_sphere).get();
		}

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			d_rotated_geometry = rotate_point(d_rotation, point_on_sphere).get();
		}

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			d_rotated_geometry = rotate_polygon(d_rotation, polygon_on_sphere).get();
		}

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			d_rotated_geometry = rotate_polyline(d_rotation, polyline_on_sphere).get();
		}

	private:
		const GPlatesMaths::Rotation d_rotation;
		GPlatesMaths::GeometryOnSphere::maybe_null_ptr_to_const_type d_rotated_geometry;
	};
}


const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesMaths::operator*(
		const Rotation &r1,
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type g)
{
	GeometryOnSphereRotater rotater(r1);
	g->accept_visitor(rotater);
	if ( ! rotater.rotated_geometry()) {
		// FIXME:  How did this happen?  We should throw an exception!
		// For now, let's be very lazy and very bad...
		return g;
	}
	GeometryOnSphere::non_null_ptr_to_const_type non_null_ptr(
			rotater.rotated_geometry().get(),
			GPlatesUtils::NullIntrusivePointerHandler());

	return non_null_ptr;
}
