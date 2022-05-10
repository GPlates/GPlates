/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2022 The University of Sydney, Australia
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

#include <cmath>

#include "MapCamera.h"

#include "MapProjection.h"
#include "ViewportZoom.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/MathsUtils.h"
#include "maths/Rotation.h"

#include "opengl/GLIntersect.h"


constexpr double GPlatesGui::MapCamera::MAP_LONGITUDE_TO_LATITUDE_EXTENT_RATIO_IN_MAP_SPACE;
constexpr double GPlatesGui::MapCamera::MAP_LONGITUDE_EXTENT_IN_MAP_SPACE;
constexpr double GPlatesGui::MapCamera::MAP_LATITUDE_EXTENT_IN_MAP_SPACE;

const double GPlatesGui::MapCamera::FRAMING_RATIO_OF_MAP_IN_VIEWPORT = 1.07;
// Use a standard field-of-view of 90 degrees for the smaller viewport dimension.
const double GPlatesGui::MapCamera::PERSPECTIVE_FIELD_OF_VIEW_DEGREES = 90.0;
const double GPlatesGui::MapCamera::TAN_HALF_PERSPECTIVE_FIELD_OF_VIEW =
		std::tan(GPlatesMaths::convert_deg_to_rad(GPlatesGui::MapCamera::PERSPECTIVE_FIELD_OF_VIEW_DEGREES) / 2.0);
// How far to nudge or pan the camera when incrementally moving the camera.
// Note that the extent of our map is roughly 360 degrees in longitude direction, and that's in map (post-projection) units.
const double GPlatesGui::MapCamera::NUDGE_CAMERA_IN_MAP_SPACE = 5.0;

// Our universe coordinate system is:
//
//   Z points out of the map plane (z=0)
//   Y increases from South to North
//   X increases from West to East
//
// We set up our initial camera look-at position to the origin.
// We set up our initial camera view direction to look down the negative z-axis.
// We set up our initial camera 'up' direction along the y-axis.
//
const QPointF GPlatesGui::MapCamera::INITIAL_LOOK_AT_POSITION(0, 0);
const GPlatesMaths::UnitVector3D GPlatesGui::MapCamera::INITIAL_VIEW_DIRECTION(0, 0, -1);
const GPlatesMaths::UnitVector3D GPlatesGui::MapCamera::INITIAL_UP_DIRECTION(0, 1, 0);


double
GPlatesGui::MapCamera::get_distance_eye_to_look_at_for_perspective_viewing_at_default_zoom()
{
	static const double distance_eye_to_look_at_for_perspective_viewing_at_default_zoom =
			calc_distance_eye_to_look_at_for_perspective_viewing_at_default_zoom();

	return distance_eye_to_look_at_for_perspective_viewing_at_default_zoom;
}


double
GPlatesGui::MapCamera::calc_distance_eye_to_look_at_for_perspective_viewing_at_default_zoom()
{
	//
	// Find the initial eye distance to the look-at position (for perspective viewing) such that
	// the map, (perspectively) projected onto the viewport, is just encompassed by the viewport
	// (and a little extra due to the framing ratio).
	//
	// Note: Unlike the globe view, in the map view (when there's no tilt) the map will appear the same size
	//       in both the orthographic and perspective view projections (because it is flat and perpendicular
	//       to the view direction) *and* objects on the map will also appear the same size in both view projections.
	//
	// This means 'tan(FOVY/2) = FRAMING_RATIO_OF_MAP_IN_VIEWPORT * 90 / distance', where 90 is approximately
	// half the latitude distance spanned by the map projection (at least in Rectangular projection), which means
	// 'distance = FRAMING_RATIO_OF_MAP_IN_VIEWPORT * 90 / tan(FOVX/2)'.
	return FRAMING_RATIO_OF_MAP_IN_VIEWPORT * (MAP_LATITUDE_EXTENT_IN_MAP_SPACE / 2.0) / TAN_HALF_PERSPECTIVE_FIELD_OF_VIEW;
}


GPlatesGui::MapCamera::MapCamera(
	ViewportZoom &viewport_zoom) :
	d_viewport_zoom(viewport_zoom),
	d_view_projection_type(GlobeProjection::ORTHOGRAPHIC),
	d_look_at_position(INITIAL_LOOK_AT_POSITION),
	d_tilt_angle(0)
{
	// View zoom changes affect our camera.
	QObject::connect(
			&viewport_zoom, SIGNAL(zoom_changed()),
			this, SLOT(handle_zoom_changed()));
}


void
GPlatesGui::MapCamera::set_view_projection_type(
		GlobeProjection::Type projection_type)
{
	if (d_view_projection_type == projection_type)
	{
		return;
	}

	d_view_projection_type = projection_type;

	Q_EMIT camera_changed();
}


const QPointF &
GPlatesGui::MapCamera::get_look_at_position() const
{
	return d_look_at_position;
}


const GPlatesMaths::UnitVector3D &
GPlatesGui::MapCamera::get_view_direction() const
{
	return INITIAL_VIEW_DIRECTION;
}


const GPlatesMaths::UnitVector3D &
GPlatesGui::MapCamera::get_up_direction() const
{
	return INITIAL_UP_DIRECTION;
}


void
GPlatesGui::MapCamera::set_tilt_angle(
		const GPlatesMaths::real_t &tilt_angle,
		bool only_emit_if_changed)
{
}


void
GPlatesGui::MapCamera::move_look_at_position(
		const QPointF &new_look_at_position,
		bool only_emit_if_changed)
{
}


void
GPlatesGui::MapCamera::reorient_up_direction(
		const GPlatesMaths::real_t &reorientation_angle,
		bool only_emit_if_changed)
{
}


void
GPlatesGui::MapCamera::pan_up(
		const GPlatesMaths::real_t &angle,
		bool only_emit_if_changed)
{
	const double nudge_y = NUDGE_CAMERA_IN_MAP_SPACE / d_viewport_zoom.zoom_factor();
	pan(QPointF(0, nudge_y), only_emit_if_changed);
}


void
GPlatesGui::MapCamera::pan_down(
		const GPlatesMaths::real_t &angle,
		bool only_emit_if_changed)
{
	const double nudge_y = -NUDGE_CAMERA_IN_MAP_SPACE / d_viewport_zoom.zoom_factor();
	pan(QPointF(0, nudge_y), only_emit_if_changed);
}


void
GPlatesGui::MapCamera::pan_left(
		const GPlatesMaths::real_t &angle,
		bool only_emit_if_changed)
{
	const double nudge_x = -NUDGE_CAMERA_IN_MAP_SPACE / d_viewport_zoom.zoom_factor();
	pan(QPointF(nudge_x, 0), only_emit_if_changed);
}


void
GPlatesGui::MapCamera::pan_right(
		const GPlatesMaths::real_t &angle,
		bool only_emit_if_changed)
{
	const double nudge_x = NUDGE_CAMERA_IN_MAP_SPACE / d_viewport_zoom.zoom_factor();
	pan(QPointF(nudge_x, 0), only_emit_if_changed);
}


void
GPlatesGui::MapCamera::pan(
		const QPointF &delta,
		bool only_emit_if_changed)
{
	if (only_emit_if_changed &&
		GPlatesMaths::are_almost_exactly_equal(delta.x(), 0) &&
		GPlatesMaths::are_almost_exactly_equal(delta.y(), 0))
	{
		return;
	}

	d_look_at_position += delta;

	Q_EMIT camera_changed();
}


void
GPlatesGui::MapCamera::rotate_anticlockwise(
		const GPlatesMaths::real_t &angle,
		bool only_emit_if_changed)
{
}


void
GPlatesGui::MapCamera::get_orthographic_left_right_bottom_top(
		const double &aspect_ratio,
		double &ortho_left,
		double &ortho_right,
		double &ortho_bottom,
		double &ortho_top) const
{
	// If the aspect ratio (viewport width/height) exceeds the ratio of the map rectangle aspect ratio then set the
	// top/bottom frustum to bound the latitude extent, otherwise set left/right frustum to bound longitude extent.
	//
	// Note: This makes the map view look well contained in the viewport regardless of aspect ratio, but
	//       only when it's not rotated. However that's the most common orientation in most cases.
	if (aspect_ratio > MAP_LONGITUDE_TO_LATITUDE_EXTENT_RATIO_IN_MAP_SPACE)
	{
		// This is used for the coordinates of the symmetrical clipping planes which bound the latitude direction.
		const double latitude_clipping = FRAMING_RATIO_OF_MAP_IN_VIEWPORT *
				(MAP_LATITUDE_EXTENT_IN_MAP_SPACE / 2.0) / d_viewport_zoom.zoom_factor();

		// right - left > top - bottom
		ortho_left = -latitude_clipping * aspect_ratio;
		ortho_right = latitude_clipping * aspect_ratio;
		ortho_bottom = -latitude_clipping;
		ortho_top = latitude_clipping;
	}
	else
	{
		// This is used for the coordinates of the symmetrical clipping planes which bound the longitude direction.
		const double longitude_clipping = FRAMING_RATIO_OF_MAP_IN_VIEWPORT *
				(MAP_LONGITUDE_EXTENT_IN_MAP_SPACE / 2.0) / d_viewport_zoom.zoom_factor();

		// right - left <= top - bottom
		ortho_left = -longitude_clipping;
		ortho_right = longitude_clipping;
		ortho_bottom = -longitude_clipping / aspect_ratio;
		ortho_top = longitude_clipping / aspect_ratio;
	}
}


GPlatesMaths::Vector3D
GPlatesGui::MapCamera::get_perspective_eye_position() const
{
	// Zooming brings us closer to the globe surface but never quite reaches it.
	// Move 1/zoom_factor times the default zoom distance between the look-at location and the eye location.
	const double distance_eye_to_look_at =
			get_distance_eye_to_look_at_for_perspective_viewing_at_default_zoom() / d_viewport_zoom.zoom_factor();

	const QPointF &look_at_position_map = get_look_at_position();
	return GPlatesMaths::Vector3D(look_at_position_map.x(), look_at_position_map.y(), 0.0) -
			distance_eye_to_look_at * get_view_direction();
}


void
GPlatesGui::MapCamera::get_perspective_fovy(
		const double &aspect_ratio,
		double &fovy_degrees) const
{
	const double tan_half_fovy = get_perspective_tan_half_fovy(aspect_ratio);
	fovy_degrees = GPlatesMaths::convert_rad_to_deg(2.0 * std::atan(tan_half_fovy));
}


double
GPlatesGui::MapCamera::get_perspective_tan_half_fovy(
		const double &aspect_ratio) const
{
	// If the aspect ratio (viewport width/height) exceeds the ratio of the map rectangle aspect ratio then set the
	// frustum to bound the latitude extent, otherwise set frustum to bound longitude extent.
	//
	// Note: This makes the map view look well contained in the viewport regardless of aspect ratio, but
	//       only when it's not rotated. However that's the most common orientation in most cases.
	if (aspect_ratio > MAP_LONGITUDE_TO_LATITUDE_EXTENT_RATIO_IN_MAP_SPACE)
	{
		// Set frustum to bound *latitude* extent...
		//
		//   tan(fovy/2) = TAN_HALF_PERSPECTIVE_FIELD_OF_VIEW
		//
		return TAN_HALF_PERSPECTIVE_FIELD_OF_VIEW;
	}
	else
	{
		// Set frustum to bound *longitude* extent.
		//
		// Easiest is to increase the fovy from what it is when 'aspect_ratio == MAP_LONGITUDE_TO_LATITUDE_EXTENT_RATIO_IN_MAP_SPACE'.
		// Noting that this factor is 1.0 when 'aspect_ratio == MAP_LONGITUDE_TO_LATITUDE_EXTENT_RATIO_IN_MAP_SPACE'.
		const double fovy_increase_factor = MAP_LONGITUDE_TO_LATITUDE_EXTENT_RATIO_IN_MAP_SPACE / aspect_ratio;
		return TAN_HALF_PERSPECTIVE_FIELD_OF_VIEW * fovy_increase_factor;
	}
}


GPlatesOpenGL::GLIntersect::Ray
GPlatesGui::MapCamera::get_camera_ray_at_window_coord(
		double window_x,
		double window_y,
		int window_width,
		int window_height) const
{
	// The aspect ratio (width/height) of the window.
	const double aspect_ratio = double(window_width) / window_height;

	// The view orientation axes.
	const GPlatesMaths::Vector3D view_z_axis(get_view_direction());
	const GPlatesMaths::Vector3D view_y_axis(get_up_direction());
	const GPlatesMaths::Vector3D view_x_axis = cross(view_z_axis, view_y_axis);

	if (get_view_projection_type() == GlobeProjection::ORTHOGRAPHIC)
	{
		double ortho_left;
		double ortho_right;
		double ortho_bottom;
		double ortho_top;
		get_orthographic_left_right_bottom_top(
				aspect_ratio,
				ortho_left, ortho_right, ortho_bottom, ortho_top);

		// Convert window coordinates to the range [0, 1] and then to [left, right] for x and
		// [bottom, top] for y.
		const GPlatesMaths::real_t view_x_component = ortho_left +
				(window_x / window_width) * (ortho_right - ortho_left);
		const GPlatesMaths::real_t view_y_component = ortho_bottom +
				(window_y / window_height) * (ortho_top - ortho_bottom);

		const QPointF &look_at_position_map = get_look_at_position();
		const GPlatesMaths::Vector3D look_at_position(look_at_position_map.x(), look_at_position_map.y(), 0.0);

		// Choose an arbitrary position on the ray. We know the look-at position projects to the
		// centre of the viewport (where 'view_x_component = 0' and 'view_y_component = 0').
		const GPlatesMaths::Vector3D position_on_ray =
				look_at_position +
				view_x_component * view_x_axis +
				view_y_component * view_y_axis;

		// In orthographic projection all view rays are parallel and so there's no real eye position.
		// Instead place the ray origin an arbitrary distance (currently 1.0) back along the view direction.
		const GPlatesMaths::Vector3D camera_eye = position_on_ray - GPlatesMaths::Vector3D(get_view_direction());

		const GPlatesMaths::UnitVector3D &ray_direction = get_view_direction();

		return GPlatesOpenGL::GLIntersect::Ray(camera_eye, ray_direction);
	}
	else  // perspective...
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				get_view_projection_type() == GPlatesGui::GlobeProjection::PERSPECTIVE,
				GPLATES_ASSERTION_SOURCE);

		// Get the field-of-view.
		const double tan_half_fovy = get_perspective_tan_half_fovy(aspect_ratio);
		const double tan_half_fovx = aspect_ratio * tan_half_fovy;

		const GPlatesMaths::real_t view_x_component = (2 * (window_x / window_width) - 1) * tan_half_fovx;
		const GPlatesMaths::real_t view_y_component = (2 * (window_y / window_height) - 1) * tan_half_fovy;

		// Ray direction.
		const GPlatesMaths::UnitVector3D ray_direction = (
				view_z_axis +
				view_x_component * view_x_axis +
				view_y_component * view_y_axis
			).get_normalisation();

		const GPlatesMaths::Vector3D camera_eye = get_perspective_eye_position();

		return GPlatesOpenGL::GLIntersect::Ray(camera_eye, ray_direction);
	}
}


GPlatesOpenGL::GLIntersect::Ray
GPlatesGui::MapCamera::get_camera_ray_at_position(
		const GPlatesMaths::Vector3D &position) const
{
	if (get_view_projection_type() == GlobeProjection::ORTHOGRAPHIC)
	{
		// In orthographic projection all view rays are parallel and so there's no real eye position.
		// Instead place the ray origin an arbitrary distance (currently 1.0) back along the view direction.
		const GPlatesMaths::Vector3D camera_eye =
				position - GPlatesMaths::Vector3D(get_view_direction());

		const GPlatesMaths::UnitVector3D &ray_direction = get_view_direction();

		return GPlatesOpenGL::GLIntersect::Ray(camera_eye, ray_direction);
	}
	else  // perspective...
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				get_view_projection_type() == GPlatesGui::GlobeProjection::PERSPECTIVE,
				GPLATES_ASSERTION_SOURCE);

		const GPlatesMaths::Vector3D camera_eye = get_perspective_eye_position();

		// Ray direction from camera eye to position.
		//
		// A precondition is the camera eye position should not coincide with the specified position.
		const GPlatesMaths::Vector3D ray_direction_unnormalised = position - camera_eye;
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				!ray_direction_unnormalised.is_zero_magnitude(),
				GPLATES_ASSERTION_SOURCE);
		const GPlatesMaths::UnitVector3D ray_direction = ray_direction_unnormalised.get_normalisation();

		return GPlatesOpenGL::GLIntersect::Ray(camera_eye, ray_direction);
	}
}


boost::optional<QPointF>
GPlatesGui::MapCamera::get_position_on_map_at_camera_ray(
		const GPlatesOpenGL::GLIntersect::Ray &camera_ray)
{
	// Create a plane representing the map plane (z=0).
	//
	// For the z=0 plane (passing through origin) this is:
	//
	//   a*x + b*y + c*z + d = 0
	//   z = 0
	//
	// ...which is...
	//
	//   a = b = d = 0.0
	//   c = 1.0
	//
	const GPlatesOpenGL::GLIntersect::Plane map_plane(0, 0, 1, 0);

	// Intersect the ray with the map plane.
	const boost::optional<GPlatesMaths::real_t> ray_distance_to_map_plane = intersect_ray_plane(camera_ray, map_plane);

	// Did the ray intersect the map plane ?
	if (!ray_distance_to_map_plane)
	{
		return boost::none;
	}

	// Point on the map plane where the ray intersects.
	const GPlatesMaths::Vector3D ray_intersection_on_map_plane =
			camera_ray.get_point_on_ray(ray_distance_to_map_plane.get());

	// We know that the intersection point must have z=0 so we can just return its x and y.
	return QPointF(ray_intersection_on_map_plane.x().dval(), ray_intersection_on_map_plane.y().dval());
}


void
GPlatesGui::MapCamera::handle_zoom_changed()
{
	// View zoom changes affect our camera (both orthographic and perspective modes).
	Q_EMIT camera_changed();
}
