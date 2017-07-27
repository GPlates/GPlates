/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#include "GnomonicProjection.h"

#include "MathsUtils.h"
#include "types.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesMaths
{
	namespace
	{
		/**
		 * Projects a unit vector point onto the plane whose normal is @a plane_normal and
		 * returns normalised version of projected point.
		 */
		UnitVector3D
		get_orthonormal_vector(
				const UnitVector3D &point,
				const UnitVector3D &plane_normal)
		{
			// The projection of 'point' in the direction of 'plane_normal'.
			Vector3D proj = dot(point, plane_normal) * plane_normal;

			// The projection of 'point' perpendicular to the direction of
			// 'plane_normal'.
			return (Vector3D(point) - proj).get_normalisation();
		}
	}
}


GPlatesMaths::GnomonicProjection::TangentPlaneFrame
GPlatesMaths::GnomonicProjection::get_tangent_plane_frame(
		const UnitVector3D &tangent_plane_normal)
{
	// First try starting with the global x axix.
	// If it's too close to the tangent plane normal then choose the global y axis.
	UnitVector3D tangent_plane_x_axis_test_point(0, 0, 1); // global x-axis
	if (abs(dot(tangent_plane_x_axis_test_point, tangent_plane_normal)) > 1 - 1e-2)
	{
		tangent_plane_x_axis_test_point = UnitVector3D(0, 1, 0); // global y-axis
	}
	const UnitVector3D tangent_plane_axis_x = get_orthonormal_vector(
			tangent_plane_x_axis_test_point, tangent_plane_normal);

	// Determine the y axis of the tangent plane.
	const UnitVector3D tangent_plane_axis_y(cross(tangent_plane_normal, tangent_plane_axis_x));

	return TangentPlaneFrame(
			tangent_plane_axis_x,
			tangent_plane_axis_y,
			tangent_plane_normal);
}


GPlatesMaths::GnomonicProjection::GnomonicProjection(
		const PointOnSphere &tangent_point,
		const AngularDistance &maximum_projection_angle) :
	d_tangent_plate_frame(get_tangent_plane_frame(tangent_point.position_vector())),
	d_minimum_projection_cosine(maximum_projection_angle.get_cosine().dval())
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			// Note that this *excludes* PI/2 radians (ie, 90 degrees) since 3D points on
			// the equator (with respect to the tangent point) project to infinity.
			maximum_projection_angle < AngularDistance::HALF_PI,
			GPLATES_ASSERTION_SOURCE);
}


GPlatesMaths::GnomonicProjection::GnomonicProjection(
		const UnitVector3D &tangent_plane_normal,
		const UnitVector3D &tangent_plane_x_axis,
		const UnitVector3D &tangent_plane_y_axis,
		const AngularDistance &maximum_projection_angle) :
	d_tangent_plate_frame(
			tangent_plane_x_axis,
			tangent_plane_y_axis,
			tangent_plane_normal),
	d_minimum_projection_cosine(maximum_projection_angle.get_cosine().dval())
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			cross(tangent_plane_x_axis, tangent_plane_y_axis) == Vector3D(tangent_plane_normal),
			GPLATES_ASSERTION_SOURCE);

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			// Note that this *excludes* PI/2 radians (ie, 90 degrees) since 3D points on
			// the equator (with respect to the tangent point) project to infinity.
			maximum_projection_angle < AngularDistance::HALF_PI,
			GPLATES_ASSERTION_SOURCE);
}


boost::optional<QPointF>
GPlatesMaths::GnomonicProjection::project_from_point_on_sphere(
		const PointOnSphere &point) const
{
	const real_t proj_point_z = dot(d_tangent_plate_frame.z_axis, point.position_vector());

	// Return none if the angle between 'point' and the tangent point is too large.
	if (proj_point_z < d_minimum_projection_cosine)
	{
		return boost::none;
	}
	const real_t inv_proj_point_z = 1.0 / proj_point_z;

	const real_t proj_point_x = inv_proj_point_z * dot(d_tangent_plate_frame.x_axis, point.position_vector());
	const real_t proj_point_y = inv_proj_point_z * dot(d_tangent_plate_frame.y_axis, point.position_vector());

	return QPointF(proj_point_x.dval(), proj_point_y.dval());
}


GPlatesMaths::PointOnSphere
GPlatesMaths::GnomonicProjection::unproject_to_point_on_sphere(
		const QPointF &point) const
{
	const Vector3D unnormalised_unprojected_point =
			Vector3D(d_tangent_plate_frame.z_axis) +
					point.x() * d_tangent_plate_frame.x_axis +
					point.y() * d_tangent_plate_frame.y_axis;

	return PointOnSphere(unnormalised_unprojected_point.get_normalisation());
}
