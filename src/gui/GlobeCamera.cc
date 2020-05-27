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

#include "maths/MathsUtils.h"


const double GPlatesGui::GlobeCamera::FRAMING_RATIO_OF_GLOBE_IN_VIEWPORT = 1.07;
// Use a standard field-of-view of 90 degrees for the smaller viewport dimension.
const double GPlatesGui::GlobeCamera::PERSPECTIVE_FIELD_OF_VIEW_DEGREES = 90.0;


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
	d_view_direction(-1, 0, 0),
	d_look_at_position(1, 0, 0),
	d_up_direction(0, 0, 1),
	d_distance_eye_to_look_at_for_perspective_viewing_at_default_zoom(
			calc_distance_eye_to_look_at_for_perspective_viewing_at_default_zoom())
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


GPlatesMaths::Vector3D
GPlatesGui::GlobeCamera::get_perspective_eye_position() const
{
	// Zooming brings us closer to the globe surface but never quite reaches it.
	// Move 1/zoom_factor times the default zoom distance between the look-at location and the eye location.
	const double distance_eye_to_look_at =
			d_distance_eye_to_look_at_for_perspective_viewing_at_default_zoom / d_viewport_zoom.zoom_factor();

	return GPlatesMaths::Vector3D(d_look_at_position) - distance_eye_to_look_at * d_view_direction;
}


void
GPlatesGui::GlobeCamera::get_orthographic_left_right_bottom_top(
		unsigned int viewport_width,
		unsigned int viewport_height,
		double &ortho_left,
		double &ortho_right,
		double &ortho_bottom,
		double &ortho_top) const
{
	// The smaller/larger of the viewport dimensions.
	double smaller_dim;
	double larger_dim;
	if (viewport_width <= viewport_height)
	{
		smaller_dim = viewport_width;
		larger_dim = viewport_height;
	}
	else
	{
		smaller_dim = viewport_height;
		larger_dim = viewport_width;
	}
		
	// This is used for the coordinates of the symmetrical clipping planes which bound the smaller dimension.
	const double smaller_dim_clipping = FRAMING_RATIO_OF_GLOBE_IN_VIEWPORT / d_viewport_zoom.zoom_factor();

	// This is used for the coordinates of the symmetrical clipping planes which bound the larger dimension.
	const double dim_ratio = larger_dim / smaller_dim;
	const double larger_dim_clipping = smaller_dim_clipping * dim_ratio;

	if (viewport_width <= viewport_height)
	{
		ortho_left = -smaller_dim_clipping;
		ortho_right = smaller_dim_clipping;
		ortho_bottom = -larger_dim_clipping;
		ortho_top = larger_dim_clipping;
	}
	else
	{
		ortho_left = -larger_dim_clipping;
		ortho_right = larger_dim_clipping;
		ortho_bottom = -smaller_dim_clipping;
		ortho_top = smaller_dim_clipping;
	}
}


void
GPlatesGui::GlobeCamera::get_perspective_aspect_ratio_and_fovy(
		unsigned int viewport_width,
		unsigned int viewport_height,
		double &aspect_ratio,
		double &fovy_degrees) const
{
	// The aspect ratio (width/height) of the screen.
	aspect_ratio = double(viewport_width) / viewport_height;

	// Since 'glu_perspective()' accepts a 'y' field-of-view (along height dimension),
	// if the height is the smaller dimension we don't need to do anything.
	fovy_degrees = PERSPECTIVE_FIELD_OF_VIEW_DEGREES;

	// If the width is the smaller dimension then our field-of-view applies to the width,
	// so we need to calculate the field-of-view that applies to the height.
	if (aspect_ratio < 1.0)
	{
		// Convert field-of-view in x-axis to field-of-view in y-axis by adjusting for the aspect ratio.
		const double fovy_degrees = GPlatesMaths::convert_rad_to_deg(
				2.0 * std::atan(
						std::tan(GPlatesMaths::convert_deg_to_rad(PERSPECTIVE_FIELD_OF_VIEW_DEGREES) / 2.0) /
						aspect_ratio));
	}
}

void
GPlatesGui::GlobeCamera::handle_zoom_changed()
{
	// View zoom changes affect our camera (both orthographic and perspective modes).
	Q_EMIT camera_changed();
}
