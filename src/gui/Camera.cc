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

#include "Camera.h"

#include "ViewportZoom.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/MathsUtils.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"


const double GPlatesGui::Camera::DEFAULT_PAN_ROTATE_TILT_RADIANS = GPlatesMaths::convert_deg_to_rad(5.0);

// Use a VERTICAL field-of-view of 70 degrees.
// This results in a HORIZONTAL field-of-view of:
//   * 70 degrees for a square viewport,
//   * 86 degrees for a viewport with 4:3 aspect ratio,
//   * 102 degrees for a viewport with 16:9 aspect ratio.
// ...provided the viewport aspect ratio exceeds the optimal aspect ratio (ie, 1.0 for globe, 2.0 for map),
// otherwise the VERTICAL field-of-view is increased (ie, no longer fixed at 70 degrees).
// This means if the viewport aspect ratio is less than 1.0 for globe or 2.0 for map then the VERTICAL
// tan(field-of-view/2) is increased by 1.0/aspect_ratio for globe or 2.0/aspect_ratio for map
// (to ensure the entire globe or map is visible in the viewport, at default zoom).
const double GPlatesGui::Camera::PERSPECTIVE_FIELD_OF_VIEW_DEGREES = 70.0;
const double GPlatesGui::Camera::TAN_HALF_PERSPECTIVE_FIELD_OF_VIEW =
		std::tan(GPlatesMaths::convert_deg_to_rad(GPlatesGui::Camera::PERSPECTIVE_FIELD_OF_VIEW_DEGREES) / 2.0);


GPlatesGui::Camera::Camera(
		ViewportProjection::Type viewport_projection,
		ViewportZoom &viewport_zoom) :
	d_viewport_projection(viewport_projection),
	d_viewport_zoom(viewport_zoom)
{
	// View zoom changes affect our camera.
	QObject::connect(
			&viewport_zoom, SIGNAL(zoom_changed()),
			this, SLOT(handle_zoom_changed()));
}


void
GPlatesGui::Camera::set_viewport_projection(
		ViewportProjection::Type viewport_projection)
{
	if (d_viewport_projection != viewport_projection)
	{
		d_viewport_projection = viewport_projection;
		Q_EMIT camera_changed();
	}
}


GPlatesOpenGL::GLMatrix
GPlatesGui::Camera::get_view_transform() const
{
	const GPlatesMaths::Vector3D camera_eye = get_eye_position();
	const GPlatesMaths::Vector3D camera_look_at = get_look_at_position();
	const GPlatesMaths::UnitVector3D camera_up = get_up_direction();

	// Get the view transform.
	//
	// Note: We don't use GLMatrix::glu_look_at() because that calculates a view direction
	//       from the eye position to the look-at position, and for orthographic viewing the
	//       look-at position can be behind the eye (since we're arbitrarily placing the eye at
	//       the near z=0 plane) and this essentially reverses the view direction.
	//       Although the look-at position is now limited to being in front of the eye
	//       (since the look-at position is now restricted to lie on the map, or globe, which is
	//       inside the sphere bounding the scene) - however it's possible that could change in future.
	//       Also, since we have the view direction there is no need to calculate it.
	GPlatesOpenGL::GLMatrix view_transform;
	view_transform.view(
			get_eye_position(),
			get_view_direction(),
			get_up_direction());

	return view_transform;
}


GPlatesOpenGL::GLMatrix
GPlatesGui::Camera::get_projection_transform(
		const double &viewport_aspect_ratio) const
{
	// The signed distance from origin (0,0,0) to eye along view direction.
	// This is positive/negative if the eye is in-front/behind the plane
	// passing through origin with plane normal in view direction.
	const double signed_distance_from_origin_to_eye_along_view_direction =
			dot(get_eye_position(), get_view_direction()).dval();
	// Reverse to get the signed distance from eye to origin (along view direction).
	// We want our near/far distances to be positive when near/far plane is in front of the eye.
	const double signed_distance_from_eye_to_origin_along_view_direction =
			-signed_distance_from_origin_to_eye_along_view_direction;

	// Distance from eye to far side of bounding sphere surface along the view direction.
	// This puts the far plane far enough away to include the entire scene.
	//
	// Note: This is calculated the same for both orthographic and perspective viewing.
	const GLdouble depth_to_far_side_of_scene = signed_distance_from_eye_to_origin_along_view_direction + get_bounding_radius();

	GPlatesOpenGL::GLMatrix projection_transform;
	if (get_viewport_projection() == ViewportProjection::ORTHOGRAPHIC)
	{
		// Distance along the view direction from eye to near side of surface of sphere that bounds the scene (globe or map).
		// This puts the near plane close enough to include the entire scene.
		//
		// Note: This will be zero if we place the eye position *on* the near clip plane
		//       (as we currently do - see 'get_orthographic_eye_position()').
		const GLdouble depth_to_near_side_of_scene = signed_distance_from_eye_to_origin_along_view_direction - get_bounding_radius();

		// Note that, counter-intuitively, zooming into an orthographic view is not accomplished by moving
		// the eye closer to the globe. Instead it's accomplished by reducing the width and height of
		// the orthographic viewing frustum (rectangular prism).
		double ortho_left;
		double ortho_right;
		double ortho_bottom;
		double ortho_top;
		get_orthographic_left_right_bottom_top(
				viewport_aspect_ratio,
				ortho_left, ortho_right, ortho_bottom, ortho_top);

		projection_transform.gl_ortho(
				ortho_left, ortho_right,
				ortho_bottom, ortho_top,
				depth_to_near_side_of_scene,
				depth_to_far_side_of_scene);
	}
	else // perspective...
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				get_viewport_projection() == ViewportProjection::PERSPECTIVE,
				GPLATES_ASSERTION_SOURCE);

		//
		// For perspective viewing it's generally advised to keep the near plane as far away as
		// possible in order to get better precision from the depth buffer (ie, quantised 32-bit
		// depths in hardware depth buffer, or 24 bits if using 8 bits for stencil, spread over a
		// shorter near-to-far distance).
		// Actually most of the loss of precision occurs in the far distance (since it's essentially
		// the post-projection '1/z' that's quantised into the 32-bit depth buffer), so depths
		// close to the near clip plane get mapped to more quantised values than further away.
		// So if there's any z-fighting (different objects mapped to same depth buffer value)
		// it'll happen more towards the far plane where it's not as noticeable
		// (since projected to a smaller area in viewport).
		// According to https://www.khronos.org/opengl/wiki/Depth_Buffer_Precision the z value
		// in eye coordinates (ie, eye at z=0) is related to the near 'n' and far 'f' distances
		// and the integer z-buffer value 'z_w' and the number of integer depth buffer values 's':
		//
		//   z_eye =         f * n
		//           -----------------------
		//           (z_w / s) * (f - n) - f
		//
		// ...for z_w equal to 0 and s this gives a z_eye of -n and -f.
		//
		// Since the eye position can move quite close to the scene (globe or map) in perspective view
		// (to accomplish viewport zooming) we also don't want to clip away the closest part of the globe or map,
		// and we don't want to clip away any objects sticking outside the globe or map as much as we
		// can avoid it (such as rendered arrows). So we can't keep the near plane too far away.
		//
		// Set the near distance to half the distance between the eye and the globe or map at maximum zoom (factor 1000).
		//
		// For an idea of the effect of doing this we plug in some values corresponding to the globe view (unit radius globe):
		//   The fully zoomed out distance to the globe is approximately 1.0. Fully zoomed in is then 1.0/1000.
		//   The near distance is half that n=0.5*1.0/1000=0.0005. And the maximum far distance, for fully zoomed out view,
		//   is f=(1.0 + 2.0 + 0.5)=3.5 where 2.0 is globe diameter and 0.5 is extra padding (for rendered arrows, etc).
		//   And with s=2^24 for a 24-bit depth buffer, we plug in the z_w values of 0, 1 and s, s-1
		//   (which are the two closest and two furthest integer z-buffer values respectively)
		//   then we get z_eye(0)-z_eye(1) = 3.0e-11 and z_eye(s)-z_eye(s-1) = 1.46e-3.
		//   This corresponds to ~0.19mm and 9.3km respectively.
		//   This also shows we get about 4.9e+7 times more z-buffer precision at the near plane compared to the far plane.
		//

		//
		// Near distance.
		//

		const double max_zoom_factor = ViewportZoom::s_max_zoom_percent / 100.0;
		const double min_distance_from_eye_to_look_at =
				get_perspective_viewing_distance_from_eye_to_look_at_for_at_default_zoom() / max_zoom_factor;

		// The distance from camera eye to near plane (along view direction).
		//
		// The division by 2.0 is because we are choosing the near plane distance to be half the
		// minimum distance from eye to globe or map.
		// And the sqrt(2.0) is because a standard perspective field-of-view of 90 degrees results in
		// a maximum distance from eye to visible near plane (at corners of viewport) that is sqrt(2) times
		// the minimum distance to near plane (at the centre of the viewport).
		const GLdouble depth_to_near_side_of_scene = min_distance_from_eye_to_look_at / (2.0 * std::sqrt(2.0));


		const double fovy_degrees = get_perspective_fovy(viewport_aspect_ratio);

		projection_transform.glu_perspective(
				fovy_degrees,
				viewport_aspect_ratio,
				depth_to_near_side_of_scene,
				depth_to_far_side_of_scene);
	}

	return projection_transform;
}

GPlatesMaths::Vector3D
GPlatesGui::Camera::get_eye_position() const
{
	if (get_viewport_projection() == ViewportProjection::ORTHOGRAPHIC)
	{
		return get_orthographic_eye_position(get_look_at_position());
	}
	else // perspective...
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				get_viewport_projection() == ViewportProjection::PERSPECTIVE,
				GPLATES_ASSERTION_SOURCE);

		return get_perspective_eye_position();
	}
}


GPlatesOpenGL::GLIntersect::Ray
GPlatesGui::Camera::get_camera_ray_at_window_coord(
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

	if (get_viewport_projection() == ViewportProjection::ORTHOGRAPHIC)
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

		// Choose an arbitrary position on the ray. We know the look-at position projects to the
		// centre of the viewport (where 'view_x_component = 0' and 'view_y_component = 0').
		const GPlatesMaths::Vector3D position_on_ray =
				get_look_at_position() +
				view_x_component * view_x_axis +
				view_y_component * view_y_axis;

		// Ray origin.
		const GPlatesMaths::Vector3D ray_origin = get_orthographic_eye_position(position_on_ray);

		// Ray direction.
		// This is the same all rays (they are parallel).
		const GPlatesMaths::UnitVector3D ray_direction = get_view_direction();

		return GPlatesOpenGL::GLIntersect::Ray(ray_origin, ray_direction);
	}
	else  // perspective...
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				get_viewport_projection() == ViewportProjection::PERSPECTIVE,
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

		// Ray origin.
		const GPlatesMaths::Vector3D ray_origin = get_perspective_eye_position();

		return GPlatesOpenGL::GLIntersect::Ray(ray_origin, ray_direction);
	}
}


GPlatesOpenGL::GLIntersect::Ray
GPlatesGui::Camera::get_camera_ray_at_position(
		const GPlatesMaths::Vector3D &position) const
{
	if (get_viewport_projection() == ViewportProjection::ORTHOGRAPHIC)
	{
		// Ray origin.
		const GPlatesMaths::Vector3D ray_origin = get_orthographic_eye_position(position);

		// Ray direction.
		// This is the same all rays (they are parallel).
		const GPlatesMaths::UnitVector3D ray_direction = get_view_direction();

		return GPlatesOpenGL::GLIntersect::Ray(ray_origin, ray_direction);
	}
	else  // perspective...
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				get_viewport_projection() == ViewportProjection::PERSPECTIVE,
				GPLATES_ASSERTION_SOURCE);

		const GPlatesMaths::Vector3D ray_origin = get_perspective_eye_position();

		// Ray direction from camera eye to position.
		//
		// A precondition is the camera eye position should not coincide with the specified position.
		const GPlatesMaths::Vector3D ray_direction_unnormalised = position - ray_origin;
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				!ray_direction_unnormalised.is_zero_magnitude(),
				GPLATES_ASSERTION_SOURCE);
		const GPlatesMaths::UnitVector3D ray_direction = ray_direction_unnormalised.get_normalisation();

		return GPlatesOpenGL::GLIntersect::Ray(ray_origin, ray_direction);
	}
}


boost::optional<QPointF>
GPlatesGui::Camera::get_window_coord_at_position(
		const GPlatesMaths::Vector3D &position,
		int window_width,
		int window_height) const
{
	// The aspect ratio (width/height) of the window.
	const double aspect_ratio = double(window_width) / window_height;

	// The view orientation axes.
	const GPlatesMaths::Vector3D view_z_axis(get_view_direction());
	const GPlatesMaths::Vector3D view_y_axis(get_up_direction());
	const GPlatesMaths::Vector3D view_x_axis = cross(view_z_axis, view_y_axis);

	if (get_viewport_projection() == ViewportProjection::ORTHOGRAPHIC)
	{
		double ortho_left;
		double ortho_right;
		double ortho_bottom;
		double ortho_top;
		get_orthographic_left_right_bottom_top(
				aspect_ratio,
				ortho_left, ortho_right, ortho_bottom, ortho_top);

		// We know the look-at position projects to the centre of the viewport
		// (where 'view_x_component = 0' and 'view_y_component = 0').
		const GPlatesMaths::Vector3D position_rel_look_at = position - get_look_at_position();

		// X and y components, in view frame, of position relative to look-at position.
		const GPlatesMaths::real_t view_x_component = dot(position_rel_look_at, view_x_axis);
		const GPlatesMaths::real_t view_y_component = dot(position_rel_look_at, view_y_axis);

		// Convert view frame coordinates to projected window coordinates.
		const GPlatesMaths::real_t window_x = window_width *
				((view_x_component - ortho_left) / (ortho_right - ortho_left));
		const GPlatesMaths::real_t window_y = window_height *
				((view_y_component - ortho_bottom) / (ortho_top - ortho_bottom));

		return QPointF(window_x.dval(), window_y.dval());
	}
	else  // perspective...
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				get_viewport_projection() == ViewportProjection::PERSPECTIVE,
				GPLATES_ASSERTION_SOURCE);

		// Get the field-of-view.
		const double tan_half_fovy = get_perspective_tan_half_fovy(aspect_ratio);
		const double tan_half_fovx = aspect_ratio * tan_half_fovy;

		const GPlatesMaths::Vector3D position_rel_camera_eye = position - get_perspective_eye_position();

		// Z component, in view frame, of position relative to camera eye.
		const GPlatesMaths::real_t view_z_component = dot(position_rel_camera_eye, view_z_axis);
		if (view_z_component == 0) // epsilon test (avoids divide by zero)
		{
			return boost::none;
		}

		// X and y components, in view frame, of position relative to camera eye.
		const GPlatesMaths::real_t view_x_component = dot(position_rel_camera_eye, view_x_axis);
		const GPlatesMaths::real_t view_y_component = dot(position_rel_camera_eye, view_y_axis);

		// X and y tans of view frame coordinate.
		const GPlatesMaths::real_t tan_view_x_component = view_x_component / view_z_component;
		const GPlatesMaths::real_t tan_view_y_component = view_y_component / view_z_component;

		// Convert view frame coordinates to projected window coordinates.
		const GPlatesMaths::real_t window_x = window_width * (((tan_view_x_component / tan_half_fovx) + 1.0) / 2.0);
		const GPlatesMaths::real_t window_y = window_height * (((tan_view_y_component / tan_half_fovy) + 1.0) / 2.0);

		return QPointF(window_x.dval(), window_y.dval());
	}
}


GPlatesMaths::Vector3D
GPlatesGui::Camera::get_orthographic_eye_position(
		const GPlatesMaths::Vector3D &look_at_position) const
{
	// Note that, counter-intuitively, zooming into an orthographic view is not accomplished by moving
	// the eye closer to the globe or map. Instead it's accomplished by reducing the width and height of
	// the orthographic viewing frustum (rectangular prism). Our setting of the eye position is simply
	// to ensure that the scene (globe or map) is in *front* of the eye (along the view direction).

	// The signed distance from origin (0,0,0) to specified look-at position (along view direction).
	// This is positive/negative if the specified look-at position is in-front/behind the plane
	// passing through origin with plane normal in view direction.
	const GPlatesMaths::real_t signed_distance_from_origin_to_look_at_position_along_view_direction =
			dot(look_at_position, get_view_direction());

	// The absolute distance from origin (0,0,0) to surface of sphere that bounds the scene (globe or map).
	const GPlatesMaths::real_t distance_from_eye_to_origin = get_bounding_radius();

	// Signed distance from eye position to specified look-at position (along view direction).
	const GPlatesMaths::real_t signed_distance_from_eye_to_look_at_position_along_view_direction =
			distance_from_eye_to_origin + signed_distance_from_origin_to_look_at_position_along_view_direction;
	// Reverse to get the signed distance from specified look-at position to eye position (along view direction).
	const GPlatesMaths::real_t signed_distance_from_look_at_position_to_eye_along_view_direction =
			-signed_distance_from_eye_to_look_at_position_along_view_direction;

	// Move from specified look-at position back along the negative view direction to find the eye position.
	return look_at_position + signed_distance_from_look_at_position_to_eye_along_view_direction * get_view_direction();
}


GPlatesMaths::Vector3D
GPlatesGui::Camera::get_perspective_eye_position() const
{
	// In contrast to orthographic viewing, zooming in 'perspective' viewing is accomplished by moving the eye position.
	// Alternatively zooming could also be accomplished by narrowing the field-of-view, but it's better
	// to keep the field-of-view constant since that is how we view the real world with the naked eye
	// (as opposed to a telephoto lens where the viewing rays become more parallel with greater zoom).
	//
	// Zooming brings us closer to the globe or map surface but never quite reaches it.
	// Move 1/zoom_factor times the default zoom distance between the look-at location and the eye location.
	const double distance_eye_to_look_at = get_perspective_viewing_distance_from_eye_to_look_at_for_at_default_zoom() /
			get_viewport_zoom().zoom_factor();

	return get_look_at_position() - distance_eye_to_look_at * GPlatesMaths::Vector3D(get_view_direction());
}


void
GPlatesGui::Camera::get_orthographic_left_right_bottom_top(
		const double &aspect_ratio,
		double &ortho_left,
		double &ortho_right,
		double &ortho_bottom,
		double &ortho_top) const
{
	// Note that, counter-intuitively, zooming into an orthographic view is not accomplished by moving
	// the eye closer to the globe or map. Instead it's accomplished by reducing the width and height of
	// the orthographic viewing frustum (rectangular prism).

	const double optimal_aspect_ratio = get_optimal_aspect_ratio();

	// If the aspect ratio (viewport width/height) exceeds the optimal aspect ratio of the globe or map view
	// then set the frustum to bound the height extent, otherwise set frustum to the bound width extent.
	//
	// Note: For the map view, this makes it look well contained in the viewport regardless of aspect ratio,
	//       but only when it's not rotated. However that's the most common orientation in most cases.
	if (aspect_ratio > optimal_aspect_ratio)
    {
		// This is used for the coordinates of the symmetrical clipping planes which bound the height direction.
		const double height_clipping =
				get_orthographic_half_height_extent_at_default_zoom() / get_viewport_zoom().zoom_factor();
 
		// right - left > top - bottom
		ortho_left = -height_clipping * aspect_ratio;
		ortho_right = height_clipping * aspect_ratio;
		ortho_bottom = -height_clipping;
		ortho_top = height_clipping;
    }
    else
    {
		// This is used for the coordinates of the symmetrical clipping planes which bound the width direction.
		const double width_clipping = optimal_aspect_ratio *
				get_orthographic_half_height_extent_at_default_zoom() / get_viewport_zoom().zoom_factor();

		// right - left <= top - bottom
		ortho_left = -width_clipping;
		ortho_right = width_clipping;
		ortho_bottom = -width_clipping / aspect_ratio;
		ortho_top = width_clipping / aspect_ratio;
    }
}


double
GPlatesGui::Camera::get_perspective_fovy(
		const double &aspect_ratio) const
{
	const double tan_half_fovy = get_perspective_tan_half_fovy(aspect_ratio);

	return GPlatesMaths::convert_rad_to_deg(2.0 * std::atan(tan_half_fovy));
}


double
GPlatesGui::Camera::get_perspective_tan_half_fovy(
		const double &aspect_ratio) const
{
	const double optimal_aspect_ratio = get_optimal_aspect_ratio();

	// If the aspect ratio (viewport width/height) exceeds the optimal aspect ratio of the globe or map view
	// then set the frustum to bound the height extent, otherwise set frustum to the bound width extent.
	//
	// Note: For the map view, this makes it look well contained in the viewport regardless of aspect ratio,
	//       but only when it's not rotated. However that's the most common orientation in most cases.
	if (aspect_ratio > optimal_aspect_ratio)
	{
		// Set frustum to bound the *height* extent...
		//
		//   tan(fovy/2) = TAN_HALF_PERSPECTIVE_FIELD_OF_VIEW
		//
		return TAN_HALF_PERSPECTIVE_FIELD_OF_VIEW;
	}
	else
	{
		// Set frustum to bound the *width* extent.
		//
		// Easiest is to increase the fovy from what it is when 'aspect_ratio == optimal_aspect_ratio'.
		// Noting that this factor is 1.0 when 'aspect_ratio == optimal_aspect_ratio'.
		const double fovy_increase_factor = optimal_aspect_ratio / aspect_ratio;

		return TAN_HALF_PERSPECTIVE_FIELD_OF_VIEW * fovy_increase_factor;
	}
}


void
GPlatesGui::Camera::handle_zoom_changed()
{
	// View zoom changes affect our camera (both orthographic and perspective modes).
	Q_EMIT camera_changed();
}
