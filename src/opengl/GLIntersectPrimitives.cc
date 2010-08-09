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

#include <limits>

#include "GLIntersectPrimitives.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GLIntersect::Plane::Plane(
		const GPlatesMaths::Vector3D &normal,
		const GPlatesMaths::Vector3D &point_on_plane) :
	d_normal(normal),
	d_signed_distance_to_origin(-dot(point_on_plane, normal))
{
}


GPlatesOpenGL::GLIntersect::Plane::HalfSpaceType
GPlatesOpenGL::GLIntersect::Plane::classify_point(
		const GPlatesMaths::Vector3D& point) const
{
	const GPlatesMaths::real_t d =
			dot(point, d_normal) + d_signed_distance_to_origin;

	if (is_strictly_positive(d))
	{
		return POSITIVE;
	}

	if (is_strictly_negative(d))
	{
		return NEGATIVE;
	}

	return ON_PLANE;
}


GPlatesOpenGL::GLIntersect::Ray::Ray(
		const GPlatesMaths::Vector3D &ray_origin,
		const GPlatesMaths::UnitVector3D &ray_direction) :
	d_origin(ray_origin),
	d_direction(ray_direction)
{
}


GPlatesOpenGL::GLIntersect::Sphere::Sphere(
		const GPlatesMaths::Vector3D &sphere_centre,
		const GPlatesMaths::real_t &sphere_radius) :
	d_centre(sphere_centre),
	d_radius(sphere_radius)
{
}


GPlatesOpenGL::GLIntersect::OrientedBoundingBox::OrientedBoundingBox(
		const GPlatesMaths::Vector3D &centre,
		const GPlatesMaths::Vector3D &half_length_x_axis,
		const GPlatesMaths::Vector3D &half_length_y_axis,
		const GPlatesMaths::Vector3D &half_length_z_axis) :
	d_centre(centre),
	d_half_length_x_axis(half_length_x_axis),
	d_half_length_y_axis(half_length_y_axis),
	d_half_length_z_axis(half_length_z_axis)
{
}


// This is the equivalent of about 6 metres (since globe has radius of ~6e+6 Kms).
// We just don't want the OBB vector to be too small that numerical errors become a problem.
const GPlatesMaths::real_t
GPlatesOpenGL::GLIntersect::OrientedBoundingBoxBuilder::DEGENERATE_HALF_LENGTH_THRESHOLD(1e-6);

GPlatesOpenGL::GLIntersect::OrientedBoundingBoxBuilder::OrientedBoundingBoxBuilder(
		const GPlatesMaths::UnitVector3D &obb_x_axis,
		const GPlatesMaths::UnitVector3D &obb_y_axis,
		const GPlatesMaths::UnitVector3D &obb_z_axis) :
	d_x_axis(obb_x_axis),
	d_y_axis(obb_y_axis),
	d_z_axis(obb_z_axis),
	// The parentheses around min/max are to prevent the windows min/max macros
	// from stuffing numeric_limits' min/max. Using 'float' instead of 'double'
	// in case 'real_t' changes its internal type for some reason.
	d_min_dot_x_axis((std::numeric_limits<float>::max)()),
	d_max_dot_x_axis(-(std::numeric_limits<float>::max)()),
	d_min_dot_y_axis((std::numeric_limits<float>::max)()),
	d_max_dot_y_axis(-(std::numeric_limits<float>::max)()),
	d_min_dot_z_axis((std::numeric_limits<float>::max)()),
	d_max_dot_z_axis(-(std::numeric_limits<float>::max)())
{
}


void
GPlatesOpenGL::GLIntersect::OrientedBoundingBoxBuilder::add(
		const GPlatesMaths::PointOnSphere &point_on_sphere)
{
	//
	// Project the point onto each axis of the oriented bounding box and
	// expand min/max projections if necessary.
	//

	const GPlatesMaths::UnitVector3D &point = point_on_sphere.position_vector();

	const GPlatesMaths::real_t dot_x_axis = dot(point, d_x_axis);
	if (dot_x_axis < d_min_dot_x_axis)
	{
		d_min_dot_x_axis = dot_x_axis;
	}
	if (dot_x_axis > d_max_dot_x_axis)
	{
		d_max_dot_x_axis = dot_x_axis;
	}

	const GPlatesMaths::real_t dot_y_axis = dot(point, d_y_axis);
	if (dot_y_axis < d_min_dot_y_axis)
	{
		d_min_dot_y_axis = dot_y_axis;
	}
	if (dot_y_axis > d_max_dot_y_axis)
	{
		d_max_dot_y_axis = dot_y_axis;
	}

	const GPlatesMaths::real_t dot_z_axis = dot(point, d_z_axis);
	if (dot_z_axis < d_min_dot_z_axis)
	{
		d_min_dot_z_axis = dot_z_axis;
	}
	if (dot_z_axis > d_max_dot_z_axis)
	{
		d_max_dot_z_axis = dot_z_axis;
	}
}


void
GPlatesOpenGL::GLIntersect::OrientedBoundingBoxBuilder::add(
		const OrientedBoundingBox &obb)
{
	// Project 'obb' along our x-axis and expand as necessary.
	add_projection(obb, d_x_axis, d_min_dot_x_axis, d_max_dot_x_axis);

	// Project 'obb' along our y-axis and expand as necessary.
	add_projection(obb, d_y_axis, d_min_dot_y_axis, d_max_dot_y_axis);

	// Project 'obb' along our z-axis and expand as necessary.
	add_projection(obb, d_z_axis, d_min_dot_z_axis, d_max_dot_z_axis);
}


void
GPlatesOpenGL::GLIntersect::OrientedBoundingBoxBuilder::add_projection(
		const OrientedBoundingBox &obb,
		GPlatesMaths::UnitVector3D &axis,
		GPlatesMaths::real_t &min_dot_axis,
		GPlatesMaths::real_t &max_dot_axis)
{
	// Project 'obb's centre point onto our axis.
	const GPlatesMaths::real_t dot_obb_centre_with_axis = dot(obb.get_centre(), axis);

	// The maximum deviation of any corner point of 'obb' (from its centre) along our axis.
	const GPlatesMaths::real_t max_abs_deviation_of_obb_along_axis =
			abs(dot(obb.get_half_length_x_axis(), axis)) +
			abs(dot(obb.get_half_length_y_axis(), axis)) +
			abs(dot(obb.get_half_length_z_axis(), axis));

	// The min/max projection of 'obb' onto our axis.
	const GPlatesMaths::real_t min_projection_onto_axis =
			dot_obb_centre_with_axis - max_abs_deviation_of_obb_along_axis;
	const GPlatesMaths::real_t max_projection_onto_axis =
			dot_obb_centre_with_axis + max_abs_deviation_of_obb_along_axis;

	// Expand the axis bounds as necessary.
	if (min_projection_onto_axis < min_dot_axis)
	{
		min_dot_axis = min_projection_onto_axis;
	}
	if (max_projection_onto_axis > max_dot_axis)
	{
		max_dot_axis = max_projection_onto_axis;
	}
}


GPlatesOpenGL::GLIntersect::OrientedBoundingBox
GPlatesOpenGL::GLIntersect::OrientedBoundingBoxBuilder::get_oriented_bounding_box() const
{
	// Make sure points have been added to form the bounding volume.
	// We can detect this by testing any min/max dot product is not
	// it's initial value of min or max float.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_min_dot_x_axis < (std::numeric_limits<float>::max)(),
			GPLATES_ASSERTION_SOURCE);

	// The box half-lengths.
	GPlatesMaths::real_t half_length_x_axis = 0.5 * (d_max_dot_x_axis - d_min_dot_x_axis);
	GPlatesMaths::real_t half_length_y_axis = 0.5 * (d_max_dot_y_axis - d_min_dot_y_axis);
	GPlatesMaths::real_t half_length_z_axis = 0.5 * (d_max_dot_z_axis - d_min_dot_z_axis);

	// Make sure the half lengths are not too small that we can't
	// form reasonable half-length oriented bounding box axes from them.
	if (half_length_x_axis < DEGENERATE_HALF_LENGTH_THRESHOLD)
	{
		half_length_x_axis = DEGENERATE_HALF_LENGTH_THRESHOLD;
	}
	if (half_length_y_axis < DEGENERATE_HALF_LENGTH_THRESHOLD)
	{
		half_length_y_axis = DEGENERATE_HALF_LENGTH_THRESHOLD;
	}
	if (half_length_z_axis < DEGENERATE_HALF_LENGTH_THRESHOLD)
	{
		half_length_z_axis = DEGENERATE_HALF_LENGTH_THRESHOLD;
	}

	// The centre of the box.
	const GPlatesMaths::Vector3D centre =
			(d_min_dot_x_axis + half_length_x_axis) * d_x_axis +
			(d_min_dot_y_axis + half_length_y_axis) * d_y_axis +
			(d_min_dot_z_axis + half_length_z_axis) * d_z_axis;

	// Create and return the oriented bounding box.
	// The format of the returned OBB is more convenient for general purpose use.
	return OrientedBoundingBox(
			centre,
			half_length_x_axis * d_x_axis,
			half_length_y_axis * d_y_axis,
			half_length_z_axis * d_z_axis);
}
