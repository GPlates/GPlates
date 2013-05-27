/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

/*
 * Fragment shader source to render region-of-interest geometries.
 *
 * 'ENABLE_SEED_FRUSTUM_CLIPPING' is used for clipping to seed frustums.
 * It is used when the seed frustum is smaller than the view frustum otherwise
 * the regular primitive clipping of the GPU (in NDC space) is all that's needed.
 */

#ifdef POINT_REGION_OF_INTEREST
#ifdef SMALL_ROI_ANGLE
	uniform float tan_squared_region_of_interest_angle;
#endif
#ifdef LARGE_ROI_ANGLE
	uniform float cos_region_of_interest_angle;
#endif
#endif

#ifdef LINE_REGION_OF_INTEREST
#ifdef SMALL_ROI_ANGLE
	uniform float sin_region_of_interest_angle;
#endif
#ifdef LARGE_ROI_ANGLE
	uniform float tan_squared_region_of_interest_complementary_angle;
#endif
#endif

#if defined(POINT_REGION_OF_INTEREST) || defined(LINE_REGION_OF_INTEREST)
varying vec3 present_day_position;
#endif

#ifdef POINT_REGION_OF_INTEREST
	varying vec3 present_day_point_centre;
#endif

#ifdef LINE_REGION_OF_INTEREST
	varying vec3 present_day_line_arc_normal;
#endif

#ifdef ENABLE_SEED_FRUSTUM_CLIPPING
	varying vec4 clip_position_params;
#endif

void main (void)
{
#ifdef POINT_REGION_OF_INTEREST
	// Discard current pixel if outside region-of-interest radius about point centre.
#ifdef SMALL_ROI_ANGLE
	// Since acos (region-of-interest angle) is very inaccurate for very small angles we instead use:
	//   tan(angle) = sin(angle) / cos(angle) = |cross(x1,x2)| / dot(x1,x2)
	// 'present_day_point_centre' is constant (and unit length) across the primitive but
	// 'present_day_position' varies and is not unit length (so must be normalised).
	vec3 present_day_position_normalised = normalize(present_day_position);
	vec3 cross_position_and_point_centre = cross(present_day_position_normalised, present_day_point_centre);
	float sin_squared_angle = dot(cross_position_and_point_centre, cross_position_and_point_centre);
	float cos_angle = dot(present_day_position_normalised, present_day_point_centre);
	float cos_squared_angle = cos_angle * cos_angle;
	if (sin_squared_angle > cos_squared_angle * tan_squared_region_of_interest_angle)
		discard;
#endif
#ifdef LARGE_ROI_ANGLE
	// However acos (region-of-interest angle) is fine for larger angles.
	// Also the 'tan' (used for small angles) is not valid at 90 degrees.
	// 'present_day_point_centre' is constant (and unit length) across the primitive but
	// 'present_day_position' varies and is not unit length (so must be normalised).
	if (dot(normalize(present_day_position), present_day_point_centre) < cos_region_of_interest_angle)
		discard;
#endif
#endif

#ifdef LINE_REGION_OF_INTEREST
	// Discard current pixel if outside region-of-interest of line (great circle arc).
#ifdef SMALL_ROI_ANGLE
	// For very small region-of-interest angles sin(angle) is fine.
	// 'present_day_line_arc_normal' is constant (and unit length) across the primitive but
	// 'present_day_position' varies and is not unit length (so must be normalised).
	if (abs(dot(normalize(present_day_position), present_day_line_arc_normal)) > sin_region_of_interest_angle)
		discard;
#endif
#ifdef LARGE_ROI_ANGLE
	// Since asin (region-of-interest angle) is very inaccurate for angles near 90 degrees we instead use:
	//   tan(90-angle) = sin(90-angle) / cos(90-angle) = |cross(x,N)| / dot(x,N)
	// where 'N' is the arc normal and 'x' is the position vector.
	// 'present_day_point_centre' is constant (and unit length) across the primitive but
	// 'present_day_position' varies and is not unit length (so must be normalised).
	vec3 present_day_position_normalised = normalize(present_day_position);
	vec3 cross_position_and_arc_normal = cross(present_day_position_normalised, present_day_line_arc_normal);
	float sin_squared_complementary_angle = dot(cross_position_and_arc_normal, cross_position_and_arc_normal);
	float cos_complementary_angle = dot(present_day_position_normalised, present_day_line_arc_normal);
	float cos_squared_complementary_angle = cos_complementary_angle * cos_complementary_angle;
	if (sin_squared_complementary_angle < 
			cos_squared_complementary_angle * tan_squared_region_of_interest_complementary_angle)
		discard;
#endif
#endif

#ifdef FILL_REGION_OF_INTEREST
	// Nothing required for *fill* regions-of-interest - they are just normal geometry.
#endif

#ifdef ENABLE_SEED_FRUSTUM_CLIPPING
	// Discard current pixel if outside the seed frustum side planes.
	// Inside clip frustum means -1 < x/w < 1 and -1 < y/w < 1 which is same as
	// -w < x < w and -w < y < w.
	// 'clip_position_params' is (x, y, w, -w).
	if (!all(lessThan(clip_position_params.wxwy, clip_position_params.xzyz)))
		discard;
#endif

	// Output all channels as 1.0 to indicate inside region-of-interest.
	// TODO: Output grayscale to account for partial pixel coverage or
	// smoothing near boundary of region-of-interest (will require max blending).
	gl_FragColor = vec4(1.0);
}
