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


namespace GPlatesGui
{
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
	// The look-at position on *globe* corresponding to specified look-at position on *map*.
	// This will use the current map projection.
	boost::optional<GPlatesMaths::PointOnSphere> look_at_position_on_globe =
			convert_position_on_map_to_globe(look_at_position_on_map);
	if (!look_at_position_on_globe)
	{
		look_at_position_on_map = get_map_boundary_position(
				get_look_at_position_on_map()/*map_position_inside_boundary*/,
				look_at_position_on_map/*map_position_outside_boundary*/);

		look_at_position_on_globe = convert_position_on_map_to_globe(look_at_position_on_map);

		// Look-at map position should correspond to a valid position on the globe.
		//
		// This is guaranteed by 'get_map_boundary_position()' if 'get_look_at_position_on_map()'
		// is always *inside* the map boundary (which it should be).
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				look_at_position_on_globe,
				GPLATES_ASSERTION_SOURCE);
	}

	if (only_emit_if_changed &&
		look_at_position_on_globe.get() == d_look_at_position_on_globe &&
		look_at_position_on_map == d_look_at_position_on_map.get()/*is old value if map projection changed*/)
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
	// The look-at position on *map* corresponding to specified look-at position on *globe*.
	// This will use the current map projection.
	const QPointF look_at_position_on_map = convert_position_on_globe_to_map(look_at_position_on_globe);

	if (only_emit_if_changed &&
		look_at_position_on_globe == d_look_at_position_on_globe &&
		// It's possible the look-at position on *globe* has not changed but the map projection has.
		// In which case the look-at position on *map* will have changed...
		look_at_position_on_map == d_look_at_position_on_map.get()/*is old value if map projection changed*/)
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
			GPlatesMaths::convert_rad_to_deg(angle.dval()));

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
			GPlatesMaths::convert_rad_to_deg(angle.dval()),
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
		// Query the left/right/top/bottom sides and corners of the map projection.
		// These are extremal points that will produce the maximum distance to the map centre.
		const double central_meridian = d_map_projection.central_meridian();
		const QPointF map_projected_points[] =
		{
			// Left/right sides...
			d_map_projection.forward_transform(GPlatesMaths::LatLonPoint(0, central_meridian - 180 + 1e-6)),
			d_map_projection.forward_transform(GPlatesMaths::LatLonPoint(0, central_meridian + 180 - 1e-6)),
			// Top/bottom sides...
			d_map_projection.forward_transform(GPlatesMaths::LatLonPoint(90 - 1e-6, central_meridian)),
			d_map_projection.forward_transform(GPlatesMaths::LatLonPoint(-90 + 1e-6, central_meridian)),
			// Top left/right corners...
			d_map_projection.forward_transform(GPlatesMaths::LatLonPoint(90 - 1e-6, central_meridian - 180 + 1e-6)),
			d_map_projection.forward_transform(GPlatesMaths::LatLonPoint(90 - 1e-6, central_meridian + 180 - 1e-6)),
			// Bottom left/right corners...
			d_map_projection.forward_transform(GPlatesMaths::LatLonPoint(-90 + 1e-6, central_meridian - 180 + 1e-6)),
			d_map_projection.forward_transform(GPlatesMaths::LatLonPoint(-90 + 1e-6, central_meridian + 180 - 1e-6))
		};
		const unsigned int num_map_projected_points = sizeof(map_projected_points) / sizeof(map_projected_points[0]);

		// The bounding extent is the maximum distance of any extremal point to the origin.
		// Note that the lat-lon point (0, central_meridian) maps to the origin in map projection space.
		double map_bounding_extent = 0.0;
		for (unsigned int point_index = 0; point_index < num_map_projected_points; ++point_index)
		{
			const QPointF &map_projected_point = map_projected_points[point_index];

			const double distance_from_origin = get_length(map_projected_point);
			if (map_bounding_extent < distance_from_origin)
			{
				map_bounding_extent = distance_from_origin;
			}
		}

		// The radius from the map origin (at central meridian) of a sphere that not only bounds the map
		// but adds padding to bound objects *off* the map (such as rendered velocity arrows) so they don't
		// get clipped by the near and far planes of the view frustum.
		//
		// For now we'll just multiple the maximum map extent by a constant factor.
		d_map_bounding_radius.set(
				1.5 * map_bounding_extent,
				d_map_projection.get_projection_settings());
	}

	return d_map_bounding_radius.get();
}


QPointF
GPlatesGui::MapCamera::get_map_boundary_position(
		const QPointF &map_position_inside_boundary,
		const QPointF &map_position_outside_boundary) const
{
	// One point should be inside and one outside the map boundary.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			is_inside_map_boundary(map_position_inside_boundary) &&
					!is_inside_map_boundary(map_position_outside_boundary),
			GPLATES_ASSERTION_SOURCE);

	QPointF inside_point = map_position_inside_boundary;
	QPointF outside_point = map_position_outside_boundary;

	const double bounding_radius = get_bounding_radius();

	// If the outside point is far away (from the inside point) then shrink it towards the inside point.
	//
	// This ensures the subsequent bisection iteration converges more quickly in those cases
	// where the outside point is very far away from the map boundary.
	//
	// We just need to get the outside point reasonably close to the bounding circle (not right on it).
	// So we don't need to do an exact line-circle intersection test.
	// Instead, to keep the shrunk outside point outside the bounding radius (and hence outside the map boundary)
	// we shrink it along the line segment towards the inside point such that its distance to the inside point
	// is twice the bounding radius (since that ensures the shrunk outside point remains outside the bounding circle,
	// regardless of the location of the inside point inside the map boundary and hence inside the bounding circle).
	// This is all just to get the outside point within a reasonable distance from the inside point.
	if (get_length(outside_point - inside_point) > 2 * bounding_radius)
	{
		outside_point = inside_point + (2 * bounding_radius / get_length(outside_point - inside_point)) *
				(outside_point - inside_point);

		// Ensure it's still outside the map boundary.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				!is_inside_map_boundary(outside_point),
				GPLATES_ASSERTION_SOURCE);
	}

	// Use bisection iterations to converge on the map projection boundary.
	//
	// The inside and outside points get closer to each other until they are within a
	// threshold distance that terminates bisection iteration.
	const double bisection_threshold = 1e-6 * bounding_radius;  // Roughly 1 arc second on map.
	while (get_length(outside_point - inside_point) > bisection_threshold)
	{
		const QPointF mid_point = 0.5 * (inside_point + outside_point);

		// See if mid-range point is inside map projection boundary.
		if (is_inside_map_boundary(mid_point))
		{
			// [mid_point, outside_point] range crosses the map boundary.
			inside_point = mid_point;
		}
		else
		{
			// [inside_point, mid_point] range crosses the map boundary.
			outside_point = mid_point;
		}
	}

	return inside_point;
}


bool
GPlatesGui::MapCamera::is_inside_map_boundary(
		const QPointF &point) const
{
	return static_cast<bool>(d_map_projection.inverse_transform(point));
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
