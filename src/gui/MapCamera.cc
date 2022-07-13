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
#include <QTransform>

#include "MapCamera.h"

#include "ViewportZoom.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/Rotation.h"
#include "maths/types.h"

#include "opengl/GLIntersect.h"


namespace
{
	/**
	 * Return the length of the specified QPointF.
	 */
	double
	get_length(
			const QPointF &point)
	{
		return std::sqrt(QPointF::dotProduct(point, point));
	}
}


constexpr double GPlatesGui::MapCamera::MAP_LATITUDE_EXTENT_IN_MAP_SPACE;

const double GPlatesGui::MapCamera::FRAMING_RATIO_OF_MAP_IN_VIEWPORT = 1.07;

// Our universe coordinate system is:
//
//   Z points out of the map plane (z=0)
//   Y increases from South to North
//   X increases from West to East
//
// We set up our initial camera look-at position to latitude and longitude (0,0).
// We set up our initial camera view direction to look down the negative z-axis.
// We set up our initial camera 'up' direction along the y-axis.
//
const GPlatesMaths::PointOnSphere GPlatesGui::MapCamera::INITIAL_LOOK_AT_POSITION_ON_GLOBE =
		make_point_on_sphere(GPlatesMaths::LatLonPoint(0, 0));
const GPlatesMaths::UnitVector3D GPlatesGui::MapCamera::INITIAL_VIEW_DIRECTION(0, 0, -1);
const GPlatesMaths::UnitVector3D GPlatesGui::MapCamera::INITIAL_UP_DIRECTION(0, 1, 0);


GPlatesGui::MapCamera::MapCamera(
		MapProjection &map_projection,
		ViewportProjection::Type viewport_projection,
		ViewportZoom &viewport_zoom) :
	Camera(viewport_projection, viewport_zoom),
	d_map_projection(map_projection),
	d_look_at_position_on_globe(INITIAL_LOOK_AT_POSITION_ON_GLOBE),
	d_rotation_angle(0),
	d_tilt_angle(0)
{
}


const QPointF &
GPlatesGui::MapCamera::get_look_at_position_on_map() const
{
	// If needed, update the look-at position on *map* (eg, if map projection changed).
	if (!d_look_at_position_on_map.is_valid(d_map_projection.get_projection_settings()))
	{
		// Update using the look-at position on *globe* (which is independent of map projection).
		d_look_at_position_on_map.set(
				convert_position_on_globe_to_map(d_look_at_position_on_globe),
				d_map_projection.get_projection_settings());
	}

	return d_look_at_position_on_map.get();
}


void
GPlatesGui::MapCamera::move_look_at_position_on_map(
		QPointF look_at_position_on_map,
		bool only_emit_if_changed)
{
	// Before we access the current look-at position on *map* check that it's valid (in case map projection changed).
	// This is because calling "get_look_at_position_on_map()" will update it with the new map projection.
	const bool is_look_at_position_on_map_valid =
			d_look_at_position_on_map.is_valid(d_map_projection.get_projection_settings());

	// The look-at position on *globe* corresponding to specified look-at position on *map*.
	// This will use the current map projection.
	boost::optional<GPlatesMaths::PointOnSphere> look_at_position_on_globe =
			convert_position_on_map_to_globe(look_at_position_on_map);
	if (!look_at_position_on_globe)
	{
		// Specified look-at position on *map* is outside the map projection boundary (so doesn't project onto globe).
		// So change it to be on the boundary (just inside it actually within a tolerance) where the line segment
		// (between current and specified look-at positions) intersects the map projection boundary.
		look_at_position_on_map = d_map_projection.get_map_boundary_position(
				get_look_at_position_on_map()/*map_position_inside_boundary*/,
				look_at_position_on_map/*map_position_outside_boundary*/);

		// Convert look-at position on *map* to look-at position on *globe*.
		look_at_position_on_globe = convert_position_on_map_to_globe(look_at_position_on_map);

		// Look-at map position should correspond to a valid position on the globe.
		//
		// This is guaranteed by 'd_map_projection.get_map_boundary_position()' if 'get_look_at_position_on_map()'
		// is always *inside* the map boundary (which it should be).
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				look_at_position_on_globe,
				GPLATES_ASSERTION_SOURCE);
	}

	if (only_emit_if_changed &&
		look_at_position_on_globe.get() == d_look_at_position_on_globe &&
		// It's possible the look-at position on *globe* has not changed but the map projection has.
		// In which case the look-at position on *map* will have changed.
		//
		// Can only compare current and new look-at positions on *map* if they're in the same map projection...
		(is_look_at_position_on_map_valid && (look_at_position_on_map == get_look_at_position_on_map())))
	{
		return;
	}

	// Update the position on globe.
	d_look_at_position_on_globe = look_at_position_on_globe.get();

	// Update the position on map.
	d_look_at_position_on_map.set(
			look_at_position_on_map,
			d_map_projection.get_projection_settings());

	Q_EMIT camera_changed();
}


GPlatesMaths::PointOnSphere
GPlatesGui::MapCamera::get_look_at_position_on_globe() const
{
	return d_look_at_position_on_globe;
}


void
GPlatesGui::MapCamera::move_look_at_position_on_globe(
		const GPlatesMaths::PointOnSphere &look_at_position_on_globe,
		bool only_emit_if_changed)
{
	// Before we access the current look-at position on *map* check that it's valid (in case map projection changed).
	// This is because calling "get_look_at_position_on_map()" will update it with the new map projection.
	const bool is_look_at_position_on_map_valid =
			d_look_at_position_on_map.is_valid(d_map_projection.get_projection_settings());

	// The look-at position on *map* corresponding to specified look-at position on *globe*.
	// This will use the current map projection.
	const QPointF look_at_position_on_map = convert_position_on_globe_to_map(look_at_position_on_globe);

	if (only_emit_if_changed &&
		look_at_position_on_globe == d_look_at_position_on_globe &&
		// It's possible the look-at position on *globe* has not changed but the map projection has.
		// In which case the look-at position on *map* will have changed.
		//
		// Can only compare current and new look-at positions on *map* if they're in the same map projection...
		(is_look_at_position_on_map_valid && (look_at_position_on_map == get_look_at_position_on_map())))
	{
		return;
	}

	// Update the position on globe.
	d_look_at_position_on_globe = look_at_position_on_globe;

	// Update the position on map.
	d_look_at_position_on_map.set(
			look_at_position_on_map,
			d_map_projection.get_projection_settings());

	Q_EMIT camera_changed();
}


GPlatesMaths::Vector3D
GPlatesGui::MapCamera::get_look_at_position() const
{
	const QPointF &position_on_map = get_look_at_position_on_map();
	return GPlatesMaths::Vector3D(position_on_map.x(), position_on_map.y(), 0);
}


GPlatesMaths::UnitVector3D
GPlatesGui::MapCamera::get_view_direction() const
{
	if (!d_cached_view_frame)
	{
		cache_view_frame();
	}

	return d_cached_view_frame->view_direction;
}


GPlatesMaths::UnitVector3D
GPlatesGui::MapCamera::get_up_direction() const
{
	if (!d_cached_view_frame)
	{
		cache_view_frame();
	}

	return d_cached_view_frame->up_direction;
}


void
GPlatesGui::MapCamera::set_rotation_angle(
		const GPlatesMaths::real_t &rotation_angle,
		bool only_emit_if_changed)
{
	if (only_emit_if_changed &&
		rotation_angle == d_rotation_angle)
	{
		return;
	}

	d_rotation_angle = rotation_angle;

	// Invalidate view frame - it now needs updating.
	invalidate_view_frame();

	Q_EMIT camera_changed();
}


void
GPlatesGui::MapCamera::set_tilt_angle(
		GPlatesMaths::real_t tilt_angle,
		bool only_emit_if_changed)
{
	if (tilt_angle.dval() > GPlatesMaths::HALF_PI)
	{
		tilt_angle = GPlatesMaths::HALF_PI;
	}
	else if (tilt_angle.dval() < 0)
	{
		tilt_angle = 0;
	}

	if (only_emit_if_changed &&
		tilt_angle == d_tilt_angle)
	{
		return;
	}

	d_tilt_angle = tilt_angle;

	// Invalidate view frame - it now needs updating.
	invalidate_view_frame();

	Q_EMIT camera_changed();
}


void
GPlatesGui::MapCamera::reorient_up_direction(
		const GPlatesMaths::real_t &reorientation_angle,
		bool only_emit_if_changed)
{
	set_rotation_angle(reorientation_angle, only_emit_if_changed);
}


void
GPlatesGui::MapCamera::pan_up(
		const GPlatesMaths::real_t &angle,
		bool only_emit_if_changed)
{
	const QPointF delta_pan_in_view_frame(
			0,
			GPlatesMaths::convert_rad_to_deg(angle.dval() / get_viewport_zoom().zoom_factor()));

	// Convert the pan in the view frame to a pan in the map frame.
	const QPointF delta_pan_in_map_frame = convert_pan_from_view_to_map_frame(delta_pan_in_view_frame);

	move_look_at_position_on_map(
			get_look_at_position_on_map() + delta_pan_in_map_frame,
			only_emit_if_changed);
}


void
GPlatesGui::MapCamera::pan_right(
		const GPlatesMaths::real_t &angle,
		bool only_emit_if_changed)
{
	const QPointF delta_pan_in_view_frame(
			GPlatesMaths::convert_rad_to_deg(angle.dval() / get_viewport_zoom().zoom_factor()),
			0);

	// Convert the pan in the view frame to a pan in the map frame.
	const QPointF delta_pan_in_map_frame = convert_pan_from_view_to_map_frame(delta_pan_in_view_frame);

	move_look_at_position_on_map(
			get_look_at_position_on_map() + delta_pan_in_map_frame,
			only_emit_if_changed);
}


void
GPlatesGui::MapCamera::rotate_anticlockwise(
		const GPlatesMaths::real_t &angle,
		bool only_emit_if_changed)
{
	set_rotation_angle(
			get_rotation_angle() + angle,
			only_emit_if_changed);
}


boost::optional<QPointF>
GPlatesGui::MapCamera::get_position_on_map_plane_at_camera_ray(
		const GPlatesOpenGL::GLIntersect::Ray &camera_ray) const
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
	const boost::optional<GPlatesMaths::real_t> ray_distance_to_map_plane =
			(get_viewport_projection() == ViewportProjection::ORTHOGRAPHIC)
					// For *orthographic* viewing the negative or positive side of the ray can intersect the plane
					// (since the view rays are parallel and so if we ignore the near/far clip planes then
					// everything in the infinitely long rectangular prism is visible)...
					? intersect_line_plane(camera_ray, map_plane)
					// Whereas for *perspective* viewing only the positive side of the ray can intersect the plane
					// (since the view rays emanate/diverge from a single eye location and so, ignoring the
					// near/far clip planes, only the front infinitely long pyramid with apex at eye is visible)...
					: intersect_ray_plane(camera_ray, map_plane);

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


boost::optional<QPointF>
GPlatesGui::MapCamera::get_position_on_map_boundary_intersected_by_2d_camera_ray(
		const QPointF &ray_direction,
		const QPointF &ray_origin) const
{
	const GPlatesMaths::real_t length_ray_direction = get_length(ray_direction);
	if (length_ray_direction == 0) // epsilon comparison
	{
		// Avoid divide-by-zero.
		return boost::none;
	}

	// Convert 2D direction to a 2D unit vector.
	const QPointF unit_vector_direction = (1 / length_ray_direction.dval()) * ray_direction;

	// The lower bound is inside, and upper bound outside, the map boundary.
	//
	// Inside point is the ray origin, which should be inside the map boundary.
	const QPointF &map_position_inside_boundary = ray_origin;
	// Start at the inside point and move in the unit vector direction to get the outside point.
	// We move a distance of twice the bounding radius since that ensures the outside point is
	// outside the bounding circle regardless of the location of the inside point.
	const QPointF map_position_outside_boundary = map_position_inside_boundary + 2 * d_map_projection.get_map_bounding_radius() * unit_vector_direction;

	return d_map_projection.get_map_boundary_position(map_position_inside_boundary, map_position_outside_boundary);
}


double
GPlatesGui::MapCamera::get_perspective_viewing_distance_from_eye_to_look_at_for_at_default_zoom() const
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


double
GPlatesGui::MapCamera::get_bounding_radius() const
{
	// Update the bounding radius if needed (eg, if map projection changed).
	if (!d_map_bounding_radius.is_valid(d_map_projection.get_projection_settings()))
	{
		// The radius from the map origin (at central meridian) of a sphere that not only bounds the map
		// but adds padding to bound objects *off* the map (such as rendered velocity arrows) so they don't
		// get clipped by the near and far planes of the view frustum.
		d_map_bounding_radius.set(
				// For now we'll just multiple the maximum map extent by a constant factor...
				1.5 * d_map_projection.get_map_bounding_radius(),
				d_map_projection.get_projection_settings());
	}

	return d_map_bounding_radius.get();
}


QPointF
GPlatesGui::MapCamera::convert_position_on_globe_to_map(
		const GPlatesMaths::PointOnSphere &position_on_globe) const
{
	return d_map_projection.forward_transform(make_lat_lon_point(position_on_globe));
}


boost::optional<GPlatesMaths::PointOnSphere>
GPlatesGui::MapCamera::convert_position_on_map_to_globe(
		const QPointF &position_on_map) const
{
	// See if position on map is actually inside the map projection (of the globe)
	// by attempting to inverse map project from map back onto globe.
	boost::optional<GPlatesMaths::LatLonPoint> lat_lon_position_on_globe =
			d_map_projection.inverse_transform(position_on_map);
	if (!lat_lon_position_on_globe)
	{
		// Position is outside the map projection (of the globe).
		return boost::none;
	}

	return make_point_on_sphere(lat_lon_position_on_globe.get());
}


QPointF
GPlatesGui::MapCamera::convert_pan_from_view_to_map_frame(
		const QPointF &pan_in_view_frame) const
{
	// Convert the pan in the view frame to a pan in the map frame.
	// Rotate the view frame pan by the view rotation (about the map plane normal).
	// Because we want, for example, a pan in the 'up' direction to be with respect to the current view.
	return pan_in_view_frame * QTransform().rotateRadians(d_rotation_angle.dval());
}


void
GPlatesGui::MapCamera::cache_view_frame() const
{
	const GPlatesMaths::Rotation rotation_about_map_plane_normal =
			GPlatesMaths::Rotation::create(GPlatesMaths::UnitVector3D::zBasis(), d_rotation_angle);

	// Rotate initial view frame, excluding tilt.
	//
	// Note that we only rotate the view and up directions to determine the tilt axis in the
	// globe orientation (we're not actually tilting the view yet here).
	const GPlatesMaths::UnitVector3D un_tilted_view_direction = rotation_about_map_plane_normal * INITIAL_VIEW_DIRECTION;
	const GPlatesMaths::UnitVector3D un_tilted_up_direction = rotation_about_map_plane_normal * INITIAL_UP_DIRECTION;

	// The tilt axis that the un-tilted view direction (and up direction) will tilt around.
	// However note that the axis effectively passes through the look-at position on globe (not globe centre).
	// The view direction always tilts away from the up direction (hence the order in the cross product).
	const GPlatesMaths::UnitVector3D tilt_axis =
			cross(un_tilted_view_direction, un_tilted_up_direction).get_normalisation();
	const GPlatesMaths::Rotation tilt_rotation = GPlatesMaths::Rotation::create(tilt_axis, d_tilt_angle);

	// Tilt the view and up directions using same tilt rotation.
	const GPlatesMaths::UnitVector3D tilted_view_direction = tilt_rotation * un_tilted_view_direction;
	const GPlatesMaths::UnitVector3D tilted_up_direction = tilt_rotation * un_tilted_up_direction;

	d_cached_view_frame = ViewFrame(tilted_view_direction, tilted_up_direction);
}
