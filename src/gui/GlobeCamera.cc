/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#include "GlobeCamera.h"

#include "ViewportZoom.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/Rotation.h"
#include "maths/MathsUtils.h"

#include "opengl/GLIntersect.h"


const double GPlatesGui::GlobeCamera::FRAMING_RATIO_OF_GLOBE_IN_ORTHOGRAPHIC_VIEWPORT = 1.07;

// Our universe coordinate system is:
//
//   Z points up
//   Y points right
//   X points out of the screen
//
// We set up our initial camera look-at position where the globe (unit sphere) intersects the positive x-axis.
// We set up our initial camera view direction to look down the negative x-axis.
// We set up our initial camera 'up' direction along the z-axis.
//
const GPlatesMaths::PointOnSphere GPlatesGui::GlobeCamera::INITIAL_LOOK_AT_POSITION(GPlatesMaths::UnitVector3D(1, 0, 0));
const GPlatesMaths::UnitVector3D GPlatesGui::GlobeCamera::INITIAL_VIEW_DIRECTION(-1, 0, 0);
const GPlatesMaths::UnitVector3D GPlatesGui::GlobeCamera::INITIAL_UP_DIRECTION(0, 0, 1);


GPlatesGui::GlobeCamera::GlobeCamera(
		ViewportProjection::Type viewport_projection,
		ViewportZoom &viewport_zoom) :
	Camera(viewport_projection, viewport_zoom),
	d_view_orientation(GPlatesMaths::Rotation::create_identity_rotation()),
	d_tilt_angle(0)
{
}


GPlatesMaths::PointOnSphere
GPlatesGui::GlobeCamera::get_look_at_position_on_globe() const
{
	if (!d_cached_view_frame)
	{
		cache_view_frame();
	}

	return d_cached_view_frame->look_at_position;
}


void
GPlatesGui::GlobeCamera::move_look_at_position_on_globe(
		const GPlatesMaths::PointOnSphere &look_at_position_on_globe,
		bool only_emit_if_changed)
{
	// Rotation from current look-at position to specified look-at position.
	const GPlatesMaths::Rotation view_rotation = GPlatesMaths::Rotation::create(
			get_look_at_position_on_globe(),
			look_at_position_on_globe);

	// Accumulate view rotation into current view orientation.
	set_view_orientation(view_rotation * get_view_orientation(), only_emit_if_changed);
}


GPlatesMaths::Vector3D
GPlatesGui::GlobeCamera::get_look_at_position() const
{
	return GPlatesMaths::Vector3D(get_look_at_position_on_globe().position_vector());
}


GPlatesMaths::UnitVector3D
GPlatesGui::GlobeCamera::get_view_direction() const
{
	if (!d_cached_view_frame)
	{
		cache_view_frame();
	}

	return d_cached_view_frame->view_direction;
}


GPlatesMaths::UnitVector3D
GPlatesGui::GlobeCamera::get_up_direction() const
{
	if (!d_cached_view_frame)
	{
		cache_view_frame();
	}

	return d_cached_view_frame->up_direction;
}


void
GPlatesGui::GlobeCamera::set_view_orientation(
		const GPlatesMaths::Rotation &view_orientation,
		bool only_emit_if_changed)
{
	if (only_emit_if_changed &&
		view_orientation.quat() == d_view_orientation.quat())
	{
		return;
	}

	d_view_orientation = view_orientation;

	// Invalidate view frame - it now needs updating.
	invalidate_view_frame();

	Q_EMIT camera_changed();
}


void
GPlatesGui::GlobeCamera::set_tilt_angle(
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
GPlatesGui::GlobeCamera::reorient_up_direction(
		const GPlatesMaths::real_t &reorientation_angle,
		bool only_emit_if_changed)
{
	// Rotate the view around the view direction.
	// It's negated so that it points towards the camera (from globe centre).
	// This works regardless of whether the view is tilted or not.
	const GPlatesMaths::UnitVector3D &rotation_axis = -get_view_direction();

	const GPlatesMaths::Vector3D vertical_orientation_unnormalised =
			cross(GPlatesMaths::UnitVector3D::zBasis()/*North pole*/, rotation_axis);
	if (vertical_orientation_unnormalised.is_zero_magnitude())
	{
		// The position on the globe from origin to globe surface along negative view direction
		// happens to be the North pole. So we cannot reorient, hence do nothing and return.
		
		if (!only_emit_if_changed)
		{
			// Client wants this signal emitted regardless.
			Q_EMIT camera_changed();
		}

		return;
	}
	const GPlatesMaths::UnitVector3D vertical_orientation = vertical_orientation_unnormalised.get_normalisation();

	const GPlatesMaths::UnitVector3D current_orientation =
			cross(get_view_direction(), get_up_direction()).get_normalisation();

	// Current orientation angle.
	GPlatesMaths::real_t current_orientation_angle = acos(dot(current_orientation, vertical_orientation));

	// Rotation angles go clockwise around rotation axis, so negate when going anti-clockwise.
	if (dot(current_orientation, cross(rotation_axis, vertical_orientation)).dval() < 0)
	{
		current_orientation_angle = -current_orientation_angle;
	}

	// If there's a difference between the desired angle and the current angle then we'll need to rotate.
	const GPlatesMaths::real_t reorient_rotation_angle = reorientation_angle - current_orientation_angle;

	const GPlatesMaths::Rotation reorient_rotation =
			GPlatesMaths::Rotation::create(rotation_axis, reorient_rotation_angle);

	set_view_orientation(reorient_rotation * get_view_orientation(), only_emit_if_changed);
}


void
GPlatesGui::GlobeCamera::pan_up(
		const GPlatesMaths::real_t &angle,
		bool only_emit_if_changed)
{
	// Rotate the view around the axis perpendicular to the view and up directions.
	const GPlatesMaths::UnitVector3D rotation_axis =
			cross(get_up_direction(), get_view_direction()).get_normalisation();

	// Rotate by positive angle to rotate the view "down".
	const GPlatesMaths::Rotation rotation =
			GPlatesMaths::Rotation::create(rotation_axis,  angle / get_viewport_zoom().zoom_factor());

	set_view_orientation(rotation * get_view_orientation(), only_emit_if_changed);
}


void
GPlatesGui::GlobeCamera::pan_right(
		const GPlatesMaths::real_t &angle,
		bool only_emit_if_changed)
{
	// Rotate the view around the "up" direction axis.
	const GPlatesMaths::UnitVector3D &rotation_axis = get_up_direction();

	// Rotate by positive angle to rotate the view "right".
	const GPlatesMaths::Rotation rotation =
			GPlatesMaths::Rotation::create(rotation_axis,  angle / get_viewport_zoom().zoom_factor());

	set_view_orientation(rotation * get_view_orientation(), only_emit_if_changed);
}


void
GPlatesGui::GlobeCamera::rotate_anticlockwise(
		const GPlatesMaths::real_t &angle,
		bool only_emit_if_changed)
{
	// Rotate the view around the look-at position.
	const GPlatesMaths::UnitVector3D &rotation_axis = get_look_at_position_on_globe().position_vector();

	// Rotate by positive angle to rotate the view "anti-clockwise".
	const GPlatesMaths::Rotation rotation =
			GPlatesMaths::Rotation::create(rotation_axis,  angle);

	set_view_orientation(rotation * get_view_orientation(), only_emit_if_changed);
}


boost::optional<GPlatesMaths::PointOnSphere>
GPlatesGui::GlobeCamera::get_position_on_globe_at_camera_ray(
		const GPlatesOpenGL::GLIntersect::Ray &camera_ray) const
{
	// Create a unit sphere representing the globe.
	const GPlatesOpenGL::GLIntersect::Sphere globe(GPlatesMaths::Vector3D()/*origin*/, 1.0);

	// Intersect the ray with the globe.
	boost::optional<GPlatesMaths::real_t> ray_distance_to_globe;
	if (get_viewport_projection() == ViewportProjection::ORTHOGRAPHIC)
	{
		// For *orthographic* viewing the negative or positive side of the ray can intersect the globe
		// (since the view rays are parallel and so if we ignore the near/far clip planes then
		// everything in the infinitely long rectangular prism is visible).
		boost::optional<std::pair<GPlatesMaths::real_t/*first*/, GPlatesMaths::real_t/*second*/>>
				intersections = intersect_line_sphere(camera_ray, globe);
		if (intersections)
		{
			ray_distance_to_globe = intersections->first;  // Closest/smallest distance.
		}
	}
	else
	{
		// Whereas for *perspective* viewing only the positive side of the ray can intersect the globe
		// (since the view rays emanate/diverge from a single eye location and so, ignoring the
		// near/far clip planes, only the front infinitely long pyramid with apex at eye is visible).
		ray_distance_to_globe = intersect_ray_sphere(camera_ray, globe);
	}

	// Did the ray intersect the globe ?
	if (!ray_distance_to_globe)
	{
		return boost::none;
	}

	// Return the point on the globe where the ray first intersects.
	// Due to numerical precision the ray may be slightly off the globe so we'll
	// normalise it (otherwise can provide out-of-range for 'acos' later on).
	// Also note the normalisation shouldn't fail since ray-globe intersection cannot be at the origin.
	const GPlatesMaths::UnitVector3D projected_point_on_globe =
			camera_ray.get_point_on_ray(ray_distance_to_globe.get()).get_normalisation();

	// Return true to indicate ray intersected the globe.
	return GPlatesMaths::PointOnSphere(projected_point_on_globe);
}


GPlatesMaths::PointOnSphere
GPlatesGui::GlobeCamera::get_nearest_globe_horizon_position_at_camera_ray(
		const GPlatesOpenGL::GLIntersect::Ray &camera_ray) const
{
	// Create a unit sphere representing the globe.
	const GPlatesOpenGL::GLIntersect::Sphere globe(GPlatesMaths::Vector3D()/*origin*/, 1.0);

	const GPlatesMaths::Vector3D horizon_point =
			get_nearest_sphere_horizon_position_at_camera_ray(camera_ray, globe);

	// Getting a non-unit length assertion error in UnitVector3D somewhere (maybe not here though)...
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			horizon_point.magSqrd() == 1.0,
			GPLATES_ASSERTION_SOURCE);
	return GPlatesMaths::PointOnSphere(
			// Since globe centre is at origin and has radius 1.0, the horizon point should already be unit length...
			GPlatesMaths::UnitVector3D(horizon_point.x(), horizon_point.y(), horizon_point.z()));
}


GPlatesMaths::Vector3D
GPlatesGui::GlobeCamera::get_nearest_sphere_horizon_position_at_camera_ray(
		const GPlatesOpenGL::GLIntersect::Ray &camera_ray,
		const GPlatesOpenGL::GLIntersect::Sphere &sphere) const
{
	const GPlatesMaths::Vector3D ray_origin_to_sphere_centre = sphere.get_centre() - camera_ray.get_origin();

	// A precondition is the camera ray origin should be outside the sphere.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			ray_origin_to_sphere_centre.magnitude() > sphere.get_radius(),
			GPLATES_ASSERTION_SOURCE);

	if (get_viewport_projection() == ViewportProjection::ORTHOGRAPHIC)
	{
		// Find the position, along camera ray, closest to the sphere.
		// We'll project this towards the origin onto the sphere, and consider that the horizon of the sphere.
		// Note that this works well for orthographic viewing since all camera rays are parallel and
		// hence all perpendicular to the horizon (circumference around sphere).

		// Project line segment from ray origin to sphere origin onto the ray direction.
		// This gives us the distance along ray to that point on the ray that is closest to the sphere.
		const GPlatesMaths::real_t ray_distance_to_closest_point =
				dot(ray_origin_to_sphere_centre, camera_ray.get_direction());
		const GPlatesMaths::Vector3D closest_point = camera_ray.get_point_on_ray(ray_distance_to_closest_point);

		// Project closest point on ray onto the sphere.
		//
		// A precondition is the camera ray's line does not intersect the sphere and hence closest point
		// cannot coincide with the sphere centre.
		const GPlatesMaths::Vector3D sphere_direction_to_closest_point_unnormalised =
				closest_point - sphere.get_centre();
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				!sphere_direction_to_closest_point_unnormalised.is_zero_magnitude(),
				GPLATES_ASSERTION_SOURCE);
		const GPlatesMaths::UnitVector3D sphere_direction_to_closest_point =
				sphere_direction_to_closest_point_unnormalised.get_normalisation();
		const GPlatesMaths::Vector3D horizon_point = sphere.get_centre() +
				sphere.get_radius() * sphere_direction_to_closest_point;

		return horizon_point;
	}
	else // perspective...
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				get_viewport_projection() == ViewportProjection::PERSPECTIVE,
				GPLATES_ASSERTION_SOURCE);

		// For perspective viewing we want the equivalent of a ray, emanating from the eye location,
		// that touches the sphere surface tangentially - call this the horizon ray.
		//
		// First we find the normal to the plane that contains our current ray and the sphere centre.
		// The plane intersects the sphere surface to form a circle. We find the point on this circle
		// such that its vector relative to sphere centre is perpendicular to the eye location
		// (also as a vector relative to sphere centre). We then find the angle between this point
		// and the sphere centre (relative to the eye location). It turns out that this angle is also
		// the angle between this point and the horizon point (relative to sphere centre). So we then
		// rotate this point by that angle around the sphere centre (along the plane). The rotated
		// point is the horizon point where the horizon ray touches the sphere surface tangentially.

		// Note that normalisation should never fail here since the current ray is not pointing
		// at the sphere centre because, as a precondition, we already know it missed the sphere.
		const GPlatesMaths::Vector3D horizon_rotation_axis_unnormalised =
				cross(ray_origin_to_sphere_centre, camera_ray.get_direction());
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				!horizon_rotation_axis_unnormalised.is_zero_magnitude(),
				GPLATES_ASSERTION_SOURCE);
		const GPlatesMaths::UnitVector3D horizon_rotation_axis =
				horizon_rotation_axis_unnormalised.get_normalisation();

		// Normalisation should not fail since horizon axis is perpendicular to ray-origin-to-sphere-centre
		// and both vectors are non-zero length.
		const GPlatesMaths::UnitVector3D ray_origin_to_sphere_centre_perpendicular =
				cross(horizon_rotation_axis, ray_origin_to_sphere_centre).get_normalisation();

		// And distance from sphere centre to ray origin (eye) should be greater than the sphere radius
		// since the eye is outside sphere.
		const GPlatesMaths::real_t horizon_rotation_angle = asin(
				sphere.get_radius() / ray_origin_to_sphere_centre.magnitude());
		const GPlatesMaths::Rotation horizon_rotation =
				GPlatesMaths::Rotation::create(horizon_rotation_axis, horizon_rotation_angle);
		const GPlatesMaths::UnitVector3D horizon_direction = horizon_rotation * ray_origin_to_sphere_centre_perpendicular;

		// Project closest point on ray onto the sphere.
		const GPlatesMaths::Vector3D horizon_point = sphere.get_centre() +
				sphere.get_radius() * horizon_direction;

		return horizon_point;
	}
}


GPlatesOpenGL::GLIntersect::Plane
GPlatesGui::GlobeCamera::get_front_globe_horizon_plane() const
{
	if (get_viewport_projection() == ViewportProjection::ORTHOGRAPHIC)
	{
		// For orthographic viewing all camera rays are parallel and hence all are perpendicular
		// to the horizon (visible circumference around sphere). This means the visible circumference
		// is a great circle (not a small circle as with perspective projections), which means the
		// horizon plane passes through the globe centre. The plane simply divides the globe into
		// two equal sized halves.
		return GPlatesOpenGL::GLIntersect::Plane(
				-get_view_direction(), /*plane normal*/  // Front of globe is in positive half space.
				GPlatesMaths::Vector3D()/*globe origin*/);
	}
	else // perspective...
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				get_viewport_projection() == ViewportProjection::PERSPECTIVE,
				GPLATES_ASSERTION_SOURCE);

		const GPlatesMaths::Vector3D camera_eye = get_perspective_eye_position();
		const GPlatesMaths::Real distance_between_camera_eye_and_globe_centre = camera_eye.magnitude();

		// The camera eye location is not expected to be inside the globe.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				distance_between_camera_eye_and_globe_centre >= 1,
				GPLATES_ASSERTION_SOURCE);

		// Horizon plane normal points in direction from globe centre to camera eye.
		// This defines the positive half space as the front of the globe (the visible part).
		const GPlatesMaths::UnitVector3D horizon_plane_normal = camera_eye.get_normalisation();

		// The distance from the plane to the globe centre (along plane normal) turns out to be
		// inversely proportional to the distance from camera eye to globe centre, as follows:
		//
		// A horizon point (on globe) subtends an angle (at camera eye) from globe centre defined by:
		//
		//   sin(angle) = 1 / E
		//
		// ...where E is distance from camera eye to globe centre. And distance from globe centre to
		// horizon point is:
		//
		//   D = sqrt(E^2 - 1)
		//
		// Now distance from camera eye to horizon plane is:
		//
		//   D * cos(angle) = sqrt(E^2 - 1) * sqrt(1 - (1/E)^2)   # since cos^2(angle) + sin^2(angle) = 1
		//                  = sqrt(E^2 - 1) * (1/E) * sqrt(E^2 - 1)
		//                  = (E^2 - 1) * (1/E)
		//                  = E - (1/E)
		//
		// Hence the distance from globe centre to horizon plane is:
		//
		//   E - (E - (1/E)) = 1/E
		//
		// Note that since "E >= 1" therefore "1/E <= 1" and so horizon plane will intersect (unit radius) globe.
		//
		const GPlatesMaths::Vector3D point_on_horizon_plane = GPlatesMaths::Vector3D()/*globe centre*/ +
				(1 / distance_between_camera_eye_and_globe_centre) * horizon_plane_normal;

		return GPlatesOpenGL::GLIntersect::Plane(horizon_plane_normal, point_on_horizon_plane);
	}
}


double
GPlatesGui::GlobeCamera::get_perspective_viewing_distance_from_eye_to_look_at_for_at_default_zoom() const
{
//
// Defining this adjusts the the initial eye distance such that objects at the look-at position project
// onto the viewport with the same projected size as they do in the orthographic projection.
// This has the advantage that an object at the look-at position appears roughly the same size in both
// projections regardless of the zoom level. The disadvantage is, in the initial view, the globe circumference
// appears smaller in the perspective projection than in the orthographic projection.
//
// If not defined, then the initial eye distance is adjusted such that the globe, (perspectively) projected
// onto the viewport, is just encompassed by the viewport (and a little extra due to the framing ratio).
// This means the perspective projection of the globe onto the viewport almost fills it.
// The advantage is, in the initial view, the globe circumference appears the same size in the perspective
// projection as in the orthographic projection. The disadvantage is an object at the look-at position
// appears bigger in the perspective projection than in the orthographic projection (for the same zoom level)
// and this happens at all zoom levels.
//
#define PROJECTED_SCALE_AT_LOOKAT_POSITION_MATCHES_ORTHOGRAPHIC

#if defined(PROJECTED_SCALE_AT_LOOKAT_POSITION_MATCHES_ORTHOGRAPHIC)
	// Since the orthographic projection has parallel rays, it will project the globe onto a unit-radius
	// circle in the view plane (where view plane normal is parallel to view direction), and there
	// will be a little bit of space around the globe in the viewport according to
	// FRAMING_RATIO_OF_GLOBE_IN_ORTHOGRAPHIC_VIEWPORT. In the perspective view, if we put that circle
	// at the look-at position then we want it to fill the viewport in the same way. This doesn't mean
	// the projection of the globe itself will fill the viewport in the same way as for the orthographic
	// projection, it will actually appear smaller. But it does mean an object at the look-at position
	// will scale to a similar size in the viewport as it does in the orthographic projection.
	// This way, when we zoom into the view, an object at the look-at position will appear to be the
	// same size in both the perspective and orthographic projections.
	//
	// This means 'tan(FOV/2) = FRAMING_RATIO_OF_GLOBE_IN_ORTHOGRAPHIC_VIEWPORT / distance', which means
	// distance = FRAMING_RATIO_OF_GLOBE_IN_ORTHOGRAPHIC_VIEWPORT / tan(FOV/2).
	return FRAMING_RATIO_OF_GLOBE_IN_ORTHOGRAPHIC_VIEWPORT / TAN_HALF_PERSPECTIVE_FIELD_OF_VIEW;
#else
	// Field-of-view for the smaller viewport dimension.
	const double perspective_field_of_view_smaller_dim_radians =
			GPlatesMaths::convert_deg_to_rad(PERSPECTIVE_FIELD_OF_VIEW_DEGREES);

	//
	// Our universe coordinate system is:
	//
	//   Z points up
	//   Y points right
	//   X points out of the screen
	//
	// So we set up our initial camera view direction to look down the negative x-axis at the
	// position (1,0,0) on the globe (unit sphere), and set our 'up' direction along the y-axis.
	//
	// For perspective viewing at the default zoom (ie, zoom factor 1.0)...
	//
	// Adjust the eye distance such that the globe is just encompassed by the view
	// (and a little extra due to the framing ratio).
	//
	// First a ray from the eye position touches the globe surface tangentially.
	// That intersection point has a positive x value (since closer to eye than the global
	// y-z plane at x=0), and a radial value that is distance from x-axis (less than 1.0).
	// This is what we'd get if the field-of-view exactly encompassed the globe.
	// Note that the angle between this tangential ray and the x-axis is half the field of view.
	// That positive x value is 'sin(fov/2)' and the radial r value is 'cos(fov/2)'.
	// Picture a y-z plane (at x='sin(fov/2)') parallel to the global y-z plane (at x=0) but moved closer.
	//
	// But now we want the field-of-view to encompass slightly more than the globe.
	// The framing ratio (slightly larger than 1.0) lifts the tangential line slightly off the
	// globe surface to create a little space around the globe in the viewport.
	// So we extend that radial value by the framing ratio to get 'r = framing_ratio * cos(fov/2)'.
	// The reason we extend along the radial direction (y-z plane) is the non-tilted
	// (ie, eye direction along x-axis) perspective frustum projects 3D points that are in the
	// same plane onto the viewport in a proportional manner, and so the framing ratio should
	// provide the desired spacing around the globe (when the globe is projected into the viewport).
	// The final eye position is such that the same field-of-view now applies to this extended 'r'
	// (in other words that ray is no longer touching the globe surface, it misses it slightly by the framing ratio):
	//
	//   tan(fov/2) = r/(d-x)
	// 
	// ...where 'd' is the distance from eye to globe centre (global origin) and 'd-x' is distance
	// from eye to that y-z plane (where x='sin(fov/2)'), and r='framing_ratio*cos(fov/2)'.
	//
	// Therefore:
	//
	//   d = x + r/tan(fov/2)
	//     = sin(fov/2) + framing_ratio * cos(fov/2) / tan(fov/2)
	//     = sin(fov/2) + framing_ratio * sin(fov/2)
	//     = (1 + framing_ratio) * sin(fov/2)
	//
	const double eye_to_globe_centre_distance = (1.0 + FRAMING_RATIO_OF_GLOBE_IN_ORTHOGRAPHIC_VIEWPORT) *
			std::sin(perspective_field_of_view_smaller_dim_radians / 2.0);

	// Subtract 1.0 since we want distance to look-at point on globe surface (not globe origin).
	return eye_to_globe_centre_distance - 1.0;
#endif
}


void
GPlatesGui::GlobeCamera::cache_view_frame() const
{
	// Rotate initial view frame, excluding tilt.
	//
	// Note that we only rotate the view and up directions to determine the tilt axis in the
	// globe orientation (we're not actually tilting the view yet here).
	const GPlatesMaths::PointOnSphere look_at_position = d_view_orientation * INITIAL_LOOK_AT_POSITION;
	const GPlatesMaths::UnitVector3D un_tilted_view_direction = d_view_orientation * INITIAL_VIEW_DIRECTION;
	const GPlatesMaths::UnitVector3D un_tilted_up_direction = d_view_orientation * INITIAL_UP_DIRECTION;

	// The tilt axis that the un-tilted view direction (and up direction) will tilt around.
	// However note that the axis effectively passes through the look-at position on globe (not globe centre).
	// The view direction always tilts away from the up direction (hence the order in the cross product).
	const GPlatesMaths::UnitVector3D tilt_axis =
			cross(un_tilted_view_direction, un_tilted_up_direction).get_normalisation();
	const GPlatesMaths::Rotation tilt_rotation = GPlatesMaths::Rotation::create(tilt_axis, d_tilt_angle);

	// Tilt the view and up directions using same tilt rotation.
	const GPlatesMaths::UnitVector3D tilted_view_direction = tilt_rotation * un_tilted_view_direction;
	const GPlatesMaths::UnitVector3D tilted_up_direction = tilt_rotation * un_tilted_up_direction;

	d_cached_view_frame = ViewFrame(look_at_position, tilted_view_direction, tilted_up_direction);
}
