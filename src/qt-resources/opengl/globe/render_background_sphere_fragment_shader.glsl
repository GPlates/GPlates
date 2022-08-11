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

//
// Fragment shader source code to render background sphere in the 3D globe views (perspective and orthographic).
//
// Renders the background colour as is, if its alpha is 1.0.
// Otherwise modifies background alpha (keeping RGB unchanged) so that it is unchanged when viewing the centre
// of the globe but is increasingly opaque towards the circumference.
//
// Imagine a translucent balloon, and consider parallel light rays travelling
// from behind the balloon towards the viewer. A light ray going through the
// centre of the balloon has to go through less material than a light ray going
// through the balloon further away from the centre, where the balloon's
// surface is more slanted relative to the viewer.
//
// We model this using a thin spherical shell at the globe surface and calculate the amount
// of shell material the view ray (for the current pixel/fragment) travels through.
//

uniform mat4 view_projection;
uniform mat4 view_inverse;
uniform mat4 projection_inverse;

uniform vec4 viewport;

uniform vec4 background_color;

// Should we write to the depth buffer?
uniform bool write_depth;

layout(location = 0) out vec4 color;

void main (void)
{
	// Get the current xy screen coordinate (in NDC space [-1,1]).
	vec2 screen_coord = window_to_NDC_xy(gl_FragCoord.xy, viewport);

	//
	// Initialize ray associated with the current fragment.
	//
	// Create the ray starting on the near plane and moving towards the far plane.
	Ray ray = get_ray(screen_coord, view_inverse, projection_inverse);

	// Intersect the ray with the globe (unit-radius sphere).
	Sphere globe = Sphere(1.0);
	Interval globe_interval;
	if (!intersect_line(globe, ray, globe_interval))
	{
		// Fragment does not cover the globe so discard it.
		discard;
	}

	if (globe_interval.from < 0)
	{
		// Globe is behind the ray, so discard fragment.
		// This can happen with perspective viewing when the camera is close to the globe.
		discard;
	}

	// The position of first intersection of ray with globe.
	vec3 front_globe_position = at(ray, globe_interval.from);

	// If we're masking depth buffer writes then no need to calculate the z-buffer depth.
	if (write_depth)
	{
		// Set the fragment depth to the first intersection with the globe surface.
		//
		// Note that we need to write to 'gl_FragDepth' because if we don't then the fixed-function
		// depth will get written (if depth writes enabled) using the fixed-function depth which is that
		// of our full-screen quad (not the actual sphere).
		vec4 screen_space_front_globe_position = view_projection * vec4(front_globe_position, 1.0);
		float z_ndc = screen_space_front_globe_position.z / screen_space_front_globe_position.w;
		// Convert normalised device coordinates (z_ndc) to window coordinates (z_w) which, for depth, is:
		//   [-1,1] -> [n,f]
		// ...where [n,f] is set by glDepthRange (default is [0,1]). The conversion is:
		//   z_w = z_ndc * (f-n)/2 + (n+f)/2
		gl_FragDepth = (z_ndc * gl_DepthRange.diff + gl_DepthRange.near + gl_DepthRange.far) / 2.0;
	}

	// Pass through the background RGB.
	color.rgb = background_color.rgb;

	// Modify background alpha (if it's transparent).
	if (background_color.a < 1.0)
	{
		// Globe is transparent.

		// Cosine of angle between globe surface normal and view direction.
		vec3 normal = front_globe_position;
		vec3 view_direction = -ray.direction;
		float cosine_globe_view_angle = dot(normal, view_direction);

		// cosine_globe_view_angle = 1  ->  power = 1
		// cosine_globe_view_angle = 0  ->  power = large (max 100)
		float power = 1.0 / max(0.01, cosine_globe_view_angle);

		// Modify the alpha channel of the background colour.
		//
		// When 'cosine_globe_view_angle' is 1.0 ('power' is also 1.0) it will not modify the alpha
		// (since '1 - pow(1 - alpha, 1.0) == alpha').
		// Non-central rays will increase alpha.
		color.a = 1.0 - pow(1.0 - background_color.a, power);
	}
	else
	{
		// Globe is opaque.
		color.a = 1.0;
	}
}
