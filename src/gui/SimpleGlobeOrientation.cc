/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2010 The University of Sydney, Australia
 * Copyright (C) 2011 Geological Survey of Norway
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

#include "SimpleGlobeOrientation.h"

#include "maths/MathsUtils.h"

const double GPlatesGui::SimpleGlobeOrientation::s_nudge_camera_amount = 5.0;

void
GPlatesGui::SimpleGlobeOrientation::move_handle_to_pos(
		const GPlatesMaths::PointOnSphere &pos)
{
	if (d_handle_pos == pos) {
		// There's no difference between the positions, so nothing to do.
		return;
	}
	GPlatesMaths::Rotation rot = GPlatesMaths::Rotation::create(d_handle_pos, pos);

	d_accum_rot = rot * d_accum_rot;
	d_rev_accum_rot = d_accum_rot.get_reverse();
	d_handle_pos = pos;

	emit orientation_changed();
}

void
GPlatesGui::SimpleGlobeOrientation::move_camera_up(
		double zoom_factor)
{
	static const GPlatesMaths::Rotation rot = GPlatesMaths::Rotation::create(
			GPlatesMaths::UnitVector3D::yBasis(), 
			GPlatesMaths::convert_deg_to_rad(s_nudge_camera_amount / zoom_factor));

	d_accum_rot = rot * d_accum_rot;
	d_rev_accum_rot = d_accum_rot.get_reverse();

	emit orientation_changed();
}

void
GPlatesGui::SimpleGlobeOrientation::move_camera_down(
		double zoom_factor)
{
	static const GPlatesMaths::Rotation rot = GPlatesMaths::Rotation::create(
			GPlatesMaths::UnitVector3D::yBasis(), 
			GPlatesMaths::convert_deg_to_rad(0 - s_nudge_camera_amount / zoom_factor));

	d_accum_rot = rot * d_accum_rot;
	d_rev_accum_rot = d_accum_rot.get_reverse();

	emit orientation_changed();
}

void
GPlatesGui::SimpleGlobeOrientation::move_camera_left(
		double zoom_factor)
{
	static const GPlatesMaths::Rotation rot = GPlatesMaths::Rotation::create(
			GPlatesMaths::UnitVector3D::zBasis(),
			GPlatesMaths::convert_deg_to_rad(s_nudge_camera_amount / zoom_factor));

	d_accum_rot = rot * d_accum_rot;
	d_rev_accum_rot = d_accum_rot.get_reverse();

	emit orientation_changed();
}

void
GPlatesGui::SimpleGlobeOrientation::move_camera_right(
		double zoom_factor)
{
	static const GPlatesMaths::Rotation rot = GPlatesMaths::Rotation::create(
			GPlatesMaths::UnitVector3D::zBasis(),
			GPlatesMaths::convert_deg_to_rad(0 - s_nudge_camera_amount / zoom_factor));

	d_accum_rot = rot * d_accum_rot;
	d_rev_accum_rot = d_accum_rot.get_reverse();

	emit orientation_changed();
}

void
GPlatesGui::SimpleGlobeOrientation::rotate_camera_clockwise()
{
	static const GPlatesMaths::Rotation rot = GPlatesMaths::Rotation::create(
			GPlatesMaths::UnitVector3D::xBasis(), 
			GPlatesMaths::convert_deg_to_rad(0 - s_nudge_camera_amount));

	d_accum_rot = rot * d_accum_rot;
	d_rev_accum_rot = d_accum_rot.get_reverse();

	emit orientation_changed();
}

void
GPlatesGui::SimpleGlobeOrientation::rotate_camera_anticlockwise()
{
	static const GPlatesMaths::Rotation rot = GPlatesMaths::Rotation::create(
			GPlatesMaths::UnitVector3D::xBasis(), 
			GPlatesMaths::convert_deg_to_rad(s_nudge_camera_amount));

	d_accum_rot = rot * d_accum_rot;
	d_rev_accum_rot = d_accum_rot.get_reverse();

	emit orientation_changed();
}

void
GPlatesGui::SimpleGlobeOrientation::rotate_camera(
		double angle)
{
	const GPlatesMaths::Rotation rot = GPlatesMaths::Rotation::create(
			GPlatesMaths::UnitVector3D::xBasis(), 
			GPlatesMaths::convert_deg_to_rad(angle));

	d_accum_rot = rot * d_accum_rot;
	d_rev_accum_rot = d_accum_rot.get_reverse();

	emit orientation_changed();
}

namespace
{
	/**
	 * Projects the vector @a v onto the plane defined by @a normal_to_plane.
	 * This function returns a zero vector if @a v is colinear to @a normal_to_plane.
	 */
	GPlatesMaths::Vector3D
	project_vector_onto_plane(
			const GPlatesMaths::Vector3D &normal_to_plane,
			const GPlatesMaths::Vector3D &v)
	{
		// First find the projection of v along normal_to_plane.
		GPlatesMaths::real_t length = GPlatesMaths::dot(v, normal_to_plane);
		GPlatesMaths::Vector3D projected = normal_to_plane * length;
		return v - projected;
	}

	/**
	 * Calculates the angle (in radians) required to rotate vector @a v1 to
	 * line up with vector @a v2. Angle will be positive if an anticlockwise
	 * rotation is necessary, negative if a clockwise rotation is necessary.
	 *
	 * This function assumes that @a normal_to_plane defines the normal vector
	 * for the plane that @a v1 and @a v2 are coplanar to. It also assumes
	 * that all the vectors supplied have a magnitude, i.e. are not the zero
	 * vector.
	 */
	GPlatesMaths::real_t
	calculate_rotation_angle_for_coplanar_vectors(
			const GPlatesMaths::Vector3D &normal_to_plane,
			const GPlatesMaths::Vector3D &v1,
			const GPlatesMaths::Vector3D &v2)
	{
		// Get the angle from the dot product of v1 and v2.
		GPlatesMaths::real_t dp = GPlatesMaths::dot(v1.get_normalisation(),
				v2.get_normalisation());
		GPlatesMaths::real_t angle = GPlatesMaths::acos(dp);
	
		// But which direction to go in? For that, we need a cross product.
		GPlatesMaths::Vector3D cp = GPlatesMaths::cross(v1, v2);
		
		GPlatesMaths::real_t direction = GPlatesMaths::dot(cp, normal_to_plane);
		if (direction < 0) {
			angle = 0 - angle;
		}
		return angle;
	}
}

void
GPlatesGui::SimpleGlobeOrientation::orient_poles_vertically()
{
	static const GPlatesMaths::Vector3D canvas_north = 
			GPlatesMaths::Vector3D(GPlatesMaths::UnitVector3D::zBasis());
	static const GPlatesMaths::Vector3D canvas_centre = 
			GPlatesMaths::Vector3D(GPlatesMaths::UnitVector3D::xBasis());

	// First find out where the north pole is currently.
	static const GPlatesMaths::PointOnSphere north_pole =
			GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(90, 0));
	GPlatesMaths::PointOnSphere oriented_north_pole = orient_point(north_pole);
	
	// Then get the angle between the current north (after it is projected onto the
	// canvas plane) and the canvas north.
	GPlatesMaths::Vector3D projected = project_vector_onto_plane(
			canvas_centre, GPlatesMaths::Vector3D(oriented_north_pole.position_vector()));
	if (projected.magSqrd() == 0) {
		// Special case: we are looking directly at the north or south pole.
		// Nothing we can do in this situation except return early.
		// Attempting to do projected.get_normalisation() will throw an IndeterminateResultException.
		return;
	}
	GPlatesMaths::real_t angle = calculate_rotation_angle_for_coplanar_vectors(
			canvas_centre, projected, canvas_north);
	
	// Perform the rotation.
	GPlatesMaths::Rotation rot = GPlatesMaths::Rotation::create(
			GPlatesMaths::UnitVector3D::xBasis(), angle);
	
	d_accum_rot = rot * d_accum_rot;
	d_rev_accum_rot = d_accum_rot.get_reverse();

	emit orientation_changed();
}

void
GPlatesGui::SimpleGlobeOrientation::set_rotation(
	const GPlatesMaths::Rotation &rotation_ 
/*	bool should_emit_external_signal */)
{
	d_accum_rot = rotation_;
	d_rev_accum_rot = d_accum_rot.get_reverse();

	emit orientation_changed(/*should_emit_external_signal*/);
}