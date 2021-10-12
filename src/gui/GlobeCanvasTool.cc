/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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

#include "GlobeCanvasTool.h"
#include "Globe.h"
#include "qt-widgets/GlobeCanvas.h"
#include "maths/PointOnSphere.h"


GPlatesGui::GlobeCanvasTool::~GlobeCanvasTool()
{  }


void
GPlatesGui::GlobeCanvasTool::reorient_globe_by_drag_update(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if ( ! d_is_in_reorientation_op) {
		d_globe_ptr->set_new_handle_pos(initial_pos_on_globe);
		d_is_in_reorientation_op = true;
	}
	d_globe_ptr->update_handle_pos(current_pos_on_globe);
}


void
GPlatesGui::GlobeCanvasTool::reorient_globe_by_drag_release(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if ( ! d_is_in_reorientation_op) {
		d_globe_ptr->set_new_handle_pos(initial_pos_on_globe);
		d_is_in_reorientation_op = true;
	}
	d_globe_ptr->update_handle_pos(current_pos_on_globe);
	d_is_in_reorientation_op = false;
}


namespace
{
	const boost::optional<GPlatesMaths::PointOnSphere>
	get_closest_point_on_horizon(
			const GPlatesMaths::PointOnSphere &oriented_point_within_horizon,
			const GPlatesMaths::PointOnSphere &oriented_center_of_viewport)
	{
		using namespace GPlatesMaths;

		if (collinear(oriented_point_within_horizon.position_vector(),
					oriented_center_of_viewport.position_vector())) {
			// The point (which is meant to be) within the horizon is either coincident
			// with the centre of the viewport, or (somehow) antipodal to the centre of
			// the viewport (which should not be possible, but right now, we don't care
			// about the history, we just care about the maths).
			//
			// Hence, it's not mathematically possible to calculate a closest point on
			// the horizon.
			return boost::none;
		}
		Vector3D cross_result =
				cross(oriented_point_within_horizon.position_vector(),
						oriented_center_of_viewport.position_vector());
		// Since the two unit-vectors are non-collinear, we can assume the cross-product is
		// a non-zero vector.
		UnitVector3D normal_to_plane = cross_result.get_normalisation();

		Vector3D point_on_horizon =
				cross(oriented_center_of_viewport.position_vector(), normal_to_plane);
		// Since both the center-of-viewport and normal-to-plane are unit-vectors, and they
		// are (by definition) perpendicular, we will assume the result is of unit length.
		return PointOnSphere(point_on_horizon.get_normalisation());
	}
}


void
GPlatesGui::GlobeCanvasTool::rotate_globe_by_drag_update(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	static const GPlatesMaths::PointOnSphere centre_of_viewport =
			GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(0, 0));

	if ( ! d_is_in_reorientation_op) {
		boost::optional<GPlatesMaths::PointOnSphere> initial_point_on_horizon =
				get_closest_point_on_horizon(initial_pos_on_globe, centre_of_viewport);
		if ( ! initial_point_on_horizon) {
			// The mouse position could not be converted to a point on the horizon. 
			// Presumably the it was at the centre of the viewport.  Hence, nothing to
			// be done.
			return;
		}

		d_globe_ptr->set_new_handle_pos(*initial_point_on_horizon);
		d_is_in_reorientation_op = true;
	}

	boost::optional<GPlatesMaths::PointOnSphere> current_point_on_horizon =
			get_closest_point_on_horizon(current_pos_on_globe, centre_of_viewport);
	if ( ! current_point_on_horizon) {
		// The mouse position could not be converted to a point on the horizon.  Presumably
		// the it was at the centre of the viewport.  Hence, nothing to be done.
		return;
	}
	d_globe_ptr->update_handle_pos(*current_point_on_horizon);
}


void
GPlatesGui::GlobeCanvasTool::rotate_globe_by_drag_release(
		const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
		bool was_on_globe,
		const GPlatesMaths::PointOnSphere &current_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_current_pos_on_globe,
		bool is_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	static const GPlatesMaths::PointOnSphere centre_of_viewport =
			GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(0, 0));

	if ( ! d_is_in_reorientation_op) {
		boost::optional<GPlatesMaths::PointOnSphere> initial_point_on_horizon =
				get_closest_point_on_horizon(initial_pos_on_globe, centre_of_viewport);
		if ( ! initial_point_on_horizon) {
			// The mouse position could not be converted to a point on the horizon. 
			// Presumably the it was at the centre of the viewport.  Hence, nothing to
			// be done.
			return;
		}

		d_globe_ptr->set_new_handle_pos(*initial_point_on_horizon);
		d_is_in_reorientation_op = true;
	}

	boost::optional<GPlatesMaths::PointOnSphere> current_point_on_horizon =
			get_closest_point_on_horizon(current_pos_on_globe, centre_of_viewport);
	if ( ! current_point_on_horizon) {
		// The mouse position could not be converted to a point on the horizon.  Presumably
		// the it was at the centre of the viewport.  Hence, nothing to be done.
		return;
	}
	d_globe_ptr->update_handle_pos(*current_point_on_horizon);
	d_is_in_reorientation_op = false;
}
