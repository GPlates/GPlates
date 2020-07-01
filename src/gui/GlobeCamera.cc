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

#include "maths/Rotation.h"

#include "opengl/GLIntersect.h"


const double GPlatesGui::GlobeCamera::FRAMING_RATIO_OF_GLOBE_IN_VIEWPORT = 1.07;

// Use a standard field-of-view of 90 degrees for the smaller viewport dimension.
const double GPlatesGui::GlobeCamera::PERSPECTIVE_FIELD_OF_VIEW_DEGREES = 90.0;
const double GPlatesGui::GlobeCamera::TAN_HALF_PERSPECTIVE_FIELD_OF_VIEW_DEGREES =
		std::tan(GPlatesMaths::convert_deg_to_rad(GPlatesGui::GlobeCamera::PERSPECTIVE_FIELD_OF_VIEW_DEGREES) / 2.0);

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


double
GPlatesGui::GlobeCamera::get_distance_eye_to_look_at_for_perspective_viewing_at_default_zoom()
{
	static const double distance_eye_to_look_at_for_perspective_viewing_at_default_zoom =
			calc_distance_eye_to_look_at_for_perspective_viewing_at_default_zoom();

	return distance_eye_to_look_at_for_perspective_viewing_at_default_zoom;
}


double
GPlatesGui::GlobeCamera::calc_distance_eye_to_look_at_for_perspective_viewing_at_default_zoom()
{
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
	const double eye_to_globe_centre_distance = (1.0 + FRAMING_RATIO_OF_GLOBE_IN_VIEWPORT) *
			std::sin(perspective_field_of_view_smaller_dim_radians / 2.0);

	// Subtract 1.0 since we want distance to look-at point on globe surface (not globe origin).
	return eye_to_globe_centre_distance - 1.0;
}


GPlatesGui::GlobeCamera::GlobeCamera(
	ViewportZoom &viewport_zoom) :
	d_viewport_zoom(viewport_zoom),
	d_projection_type(GlobeProjection::ORTHOGRAPHIC),
	d_view_orientation(GPlatesMaths::Rotation::create_identity_rotation()),
	d_tilt_angle(0)
{
	// View zoom changes affect our camera.
	QObject::connect(
			&viewport_zoom, SIGNAL(zoom_changed()),
			this, SLOT(handle_zoom_changed()));
}


void
GPlatesGui::GlobeCamera::set_projection_type(
		GlobeProjection::Type projection_type)
{
	if (d_projection_type == projection_type)
	{
		return;
	}

	d_projection_type = projection_type;

	Q_EMIT camera_changed();
}


const GPlatesMaths::PointOnSphere &
GPlatesGui::GlobeCamera::get_look_at_position() const
{
	if (!d_view_frame)
	{
		cache_view_frame();
	}

	return d_view_frame->look_at_position;
}


const GPlatesMaths::UnitVector3D &
GPlatesGui::GlobeCamera::get_view_direction() const
{
	if (!d_view_frame)
	{
		cache_view_frame();
	}

	return d_view_frame->view_direction;
}


const GPlatesMaths::UnitVector3D &
GPlatesGui::GlobeCamera::get_up_direction() const
{
	if (!d_view_frame)
	{
		cache_view_frame();
	}

	return d_view_frame->up_direction;
}


void
GPlatesGui::GlobeCamera::set_view_orientation(
		const GPlatesMaths::Rotation &view_orientation)
{
	if (view_orientation.quat() == d_view_orientation.quat())
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
		const GPlatesMaths::real_t &tilt_angle)
{
	if (tilt_angle == d_tilt_angle)
	{
		return;
	}

	d_tilt_angle = tilt_angle;

	// Invalidate view frame - it now needs updating.
	invalidate_view_frame();

	Q_EMIT camera_changed();
}


void
GPlatesGui::GlobeCamera::move_look_at_position(
		const GPlatesMaths::PointOnSphere &new_look_at_position)
{
	if (new_look_at_position == get_look_at_position())
	{
		return;
	}

	// Rotation from current look-at position to specified look-at position.
	const GPlatesMaths::Rotation view_rotation = GPlatesMaths::Rotation::create(
			get_look_at_position(),
			new_look_at_position);

	// Accumulate view rotation into current view orientation.
	d_view_orientation = view_rotation * d_view_orientation;

	// Invalidate view frame - it now needs updating.
	invalidate_view_frame();

	Q_EMIT camera_changed();
}


void
GPlatesGui::GlobeCamera::reorient_up_direction(
		const GPlatesMaths::real_t &reorientation_angle)
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

	set_view_orientation(reorient_rotation * get_view_orientation());
}


void
GPlatesGui::GlobeCamera::rotate_down(
		const GPlatesMaths::real_t &angle)
{
	// Rotate the view around the axis perpendicular to the view and up directions.
	const GPlatesMaths::UnitVector3D rotation_axis =
			cross(get_view_direction(), get_up_direction()).get_normalisation();

	// Rotate by positive angle to rotate the view "down".
	const GPlatesMaths::Rotation rotation =
			GPlatesMaths::Rotation::create(rotation_axis,  angle);

	set_view_orientation(rotation * get_view_orientation());
}


void
GPlatesGui::GlobeCamera::rotate_right(
		const GPlatesMaths::real_t &angle)
{
	// Rotate the view around the "up" direction axis.
	const GPlatesMaths::UnitVector3D &rotation_axis = get_up_direction();

	// Rotate by positive angle to rotate the view "right".
	const GPlatesMaths::Rotation rotation =
			GPlatesMaths::Rotation::create(rotation_axis,  angle);

	set_view_orientation(rotation * get_view_orientation());
}


void
GPlatesGui::GlobeCamera::rotate_anticlockwise(
		const GPlatesMaths::real_t &angle)
{
	// Rotate the view around the look-at position.
	const GPlatesMaths::UnitVector3D &rotation_axis = get_look_at_position().position_vector();

	// Rotate by positive angle to rotate the view "anti-clockwise".
	const GPlatesMaths::Rotation rotation =
			GPlatesMaths::Rotation::create(rotation_axis,  angle);

	set_view_orientation(rotation * get_view_orientation());
}


void
GPlatesGui::GlobeCamera::get_orthographic_left_right_bottom_top(
		const double &aspect_ratio,
		double &ortho_left,
		double &ortho_right,
		double &ortho_bottom,
		double &ortho_top) const
{
	// This is used for the coordinates of the symmetrical clipping planes which bound the smaller dimension.
	const double smaller_dim_clipping = FRAMING_RATIO_OF_GLOBE_IN_VIEWPORT / d_viewport_zoom.zoom_factor();

	if (aspect_ratio > 1.0)
	{
		// right - left > top - bottom
		ortho_left = -smaller_dim_clipping * aspect_ratio;
		ortho_right = smaller_dim_clipping * aspect_ratio;
		ortho_bottom = -smaller_dim_clipping;
		ortho_top = smaller_dim_clipping;
	}
	else
	{
		// right - left <= top - bottom
		ortho_left = -smaller_dim_clipping;
		ortho_right = smaller_dim_clipping;
		ortho_bottom = -smaller_dim_clipping / aspect_ratio;
		ortho_top = smaller_dim_clipping / aspect_ratio;
	}
}


GPlatesMaths::Vector3D
GPlatesGui::GlobeCamera::get_perspective_eye_position() const
{
	// Zooming brings us closer to the globe surface but never quite reaches it.
	// Move 1/zoom_factor times the default zoom distance between the look-at location and the eye location.
	const double distance_eye_to_look_at =
			get_distance_eye_to_look_at_for_perspective_viewing_at_default_zoom() / d_viewport_zoom.zoom_factor();

	return GPlatesMaths::Vector3D(get_look_at_position().position_vector()) -
			distance_eye_to_look_at * get_view_direction();
}


void
GPlatesGui::GlobeCamera::get_perspective_fovy(
		const double &aspect_ratio,
		double &fovy_degrees) const
{
	// Since 'glu_perspective()' accepts a 'y' field-of-view (along height dimension),
	// if the height is the smaller dimension we don't need to do anything.
	fovy_degrees = PERSPECTIVE_FIELD_OF_VIEW_DEGREES;

	// If the width is the smaller dimension then our field-of-view applies to the width,
	// so we need to calculate the field-of-view that applies to the height.
	if (aspect_ratio < 1.0)
	{
		// Convert field-of-view in x-axis to field-of-view in y-axis by adjusting for the aspect ratio.
		fovy_degrees = GPlatesMaths::convert_rad_to_deg(
				2.0 * std::atan(TAN_HALF_PERSPECTIVE_FIELD_OF_VIEW_DEGREES / aspect_ratio));
	}
}


GPlatesOpenGL::GLIntersect::Ray
GPlatesGui::GlobeCamera::get_camera_ray_at_window_coord(
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

	if (get_projection_type() == GlobeProjection::ORTHOGRAPHIC)
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

		const GPlatesMaths::Vector3D look_at_position(get_look_at_position().position_vector());

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
				get_projection_type() == GPlatesGui::GlobeProjection::PERSPECTIVE,
				GPLATES_ASSERTION_SOURCE);

		double tan_fovx;
		double tan_fovy;
		// Field-of-view applies to smaller dimension.
		if (aspect_ratio < 1.0)
		{
			// Width is smaller dimension.
			tan_fovx = TAN_HALF_PERSPECTIVE_FIELD_OF_VIEW_DEGREES;
			tan_fovy = tan_fovx / aspect_ratio;
		}
		else
		{
			// Height is smaller dimension.
			tan_fovy = TAN_HALF_PERSPECTIVE_FIELD_OF_VIEW_DEGREES;
			tan_fovx = tan_fovy * aspect_ratio;
		}

		const GPlatesMaths::real_t view_x_component = (2 * (window_x / window_width) - 1) * tan_fovx;
		const GPlatesMaths::real_t view_y_component = (2 * (window_y / window_height) - 1) * tan_fovy;

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
GPlatesGui::GlobeCamera::get_camera_ray_at_position_on_globe(
		const GPlatesMaths::UnitVector3D &pos_on_globe) const
{
	if (get_projection_type() == GlobeProjection::ORTHOGRAPHIC)
	{
		// In orthographic projection all view rays are parallel and so there's no real eye position.
		// Instead place the ray origin an arbitrary distance (currently 1.0) back along the view direction.
		const GPlatesMaths::Vector3D camera_eye =
				GPlatesMaths::Vector3D(pos_on_globe) - GPlatesMaths::Vector3D(get_view_direction());

		const GPlatesMaths::UnitVector3D &ray_direction = get_view_direction();

		return GPlatesOpenGL::GLIntersect::Ray(camera_eye, ray_direction);
	}
	else  // perspective...
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				get_projection_type() == GPlatesGui::GlobeProjection::PERSPECTIVE,
				GPLATES_ASSERTION_SOURCE);

		const GPlatesMaths::Vector3D camera_eye = get_perspective_eye_position();

		// Ray direction from camera eye to position on globe.
		const GPlatesMaths::UnitVector3D ray_direction =
				// Note that camera eye position should be outside the globe
				// (and hence never coincide with pos_on_globe)...
				(GPlatesMaths::Vector3D(pos_on_globe) - camera_eye).get_normalisation();

		return GPlatesOpenGL::GLIntersect::Ray(camera_eye, ray_direction);
	}
}


boost::optional<GPlatesMaths::PointOnSphere>
GPlatesGui::GlobeCamera::get_position_on_globe_at_camera_ray(
		const GPlatesOpenGL::GLIntersect::Ray &camera_ray) const
{
	// Create a unit sphere representing the globe.
	const GPlatesOpenGL::GLIntersect::Sphere globe(GPlatesMaths::Vector3D()/*origin*/, 1.0);

	// Intersect the ray with the globe.
	const boost::optional<GPlatesMaths::real_t> ray_distance_to_globe = intersect_ray_sphere(camera_ray, globe);

	// Did the ray intersect the globe ?
	if (!ray_distance_to_globe)
	{
		return boost::none;
	}

	// Return the point on the globe where the ray first intersects.
	// Due to numerical precision the ray may be slightly off the globe so we'll
	// normalise it (otherwise can provide out-of-range for 'acos' later on).
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

	return GPlatesMaths::PointOnSphere(
			// Since globe centre is at origin and has radius 1.0, the horizon point should already be unit length...
			GPlatesMaths::UnitVector3D(horizon_point.x(), horizon_point.y(), horizon_point.z()));
}


GPlatesMaths::Vector3D
GPlatesGui::GlobeCamera::get_nearest_sphere_horizon_position_at_camera_ray(
		const GPlatesOpenGL::GLIntersect::Ray &camera_ray,
		const GPlatesOpenGL::GLIntersect::Sphere &sphere) const
{
	if (get_projection_type() == GPlatesGui::GlobeProjection::ORTHOGRAPHIC)
	{
		// Find the position, along camera ray, closest to the sphere.
		// We'll project this towards the origin onto the sphere, and consider that the horizon of the sphere.
		// Note that this works well for orthographic viewing since all camera rays are parallel and
		// hence all perpendicular to the horizon (circumference around sphere).

		// Project line segment from ray origin to sphere origin onto the ray direction.
		// This gives us the distance along ray to that point on the ray that is closest to the sphere.
		const GPlatesMaths::real_t ray_distance_to_closest_point =
				dot(sphere.get_centre() - camera_ray.get_origin(), camera_ray.get_direction());
		const GPlatesMaths::Vector3D closest_point = camera_ray.get_point_on_ray(ray_distance_to_closest_point);

		// Project closest point on ray onto the sphere.
		const GPlatesMaths::UnitVector3D sphere_direction_to_closest_point =
				(closest_point - sphere.get_centre()).get_normalisation();
		const GPlatesMaths::Vector3D horizon_point = sphere.get_centre() +
				sphere.get_radius() * sphere_direction_to_closest_point;

		return horizon_point;
	}
	else // perspective...
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				get_projection_type() == GPlatesGui::GlobeProjection::PERSPECTIVE,
				GPLATES_ASSERTION_SOURCE);

		// For perspective viewing we want the equivalent of a ray, emanating from the eye location,
		// that touches the sphere surface tangentially - call this the horizon ray.
		//
		// First we find the normal to the plane that contains our current ray and the sphere centre.
		// The plane intersects the sphere surface to form a circle. We find the point on this circle
		// that is perpendicular to the eye location (both with respect to sphere centre). We then find
		// the angle between this point and the sphere centre (at the eye location). We then rotate
		// this point by the angle around the sphere centre (along the plane). The rotated point is
		// the horizon point where the horizon ray touches the sphere surface tangentially.
		
		const GPlatesMaths::Vector3D ray_origin_to_sphere_centre = sphere.get_centre() - camera_ray.get_origin();
		// Note that normalisation should never fail here since the current ray is not pointing
		// at the sphere centre because we already know it missed the sphere.
		const GPlatesMaths::UnitVector3D horizon_rotation_axis =
				cross(ray_origin_to_sphere_centre, camera_ray.get_direction()).get_normalisation();
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

	d_view_frame = ViewFrame(look_at_position, tilted_view_direction, tilted_up_direction);
}


void
GPlatesGui::GlobeCamera::handle_zoom_changed()
{
	// View zoom changes affect our camera (both orthographic and perspective modes).
	Q_EMIT camera_changed();
}
