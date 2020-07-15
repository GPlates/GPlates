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

uniform vec4 background_color;

// Should we write to the depth buffer?
uniform bool write_depth;

// Screen-space coordinates are post-projection coordinates in the range [-1,1].
varying vec2 screen_coord;

void main (void)
{
	//
	// Initialize ray associated with the current fragment.
	//
	// Create the ray starting on the near plane and moving towards the far plane.
	//
	// Note: Seems gl_ModelViewProjectionMatrixInverse does not always work on Mac OS X.
	Ray ray = get_ray(screen_coord, gl_ModelViewMatrixInverse * gl_ProjectionMatrixInverse);

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
		vec4 screen_space_front_globe_position = gl_ModelViewProjectionMatrix * vec4(front_globe_position, 1.0);
		float screen_space_front_globe_depth = screen_space_front_globe_position.z / screen_space_front_globe_position.w;
		// Convert normalised device coordinates (screen-space) to window coordinates which, for depth, is [-1,1] -> [0,1].
		// Ideally this should also consider the effects of glDepthRange but we'll assume it's set to [0,1].
		gl_FragDepth = 0.5 * screen_space_front_globe_depth + 0.5;
	}

	// If the globe is opaque then just set the fragment colour and return.
	if (background_color.a == 1.0)
	{
		gl_FragColor = background_color;

		return;
	}

	float shell_thickness = 0.0033;  // Using same value that was in initial C++ non-shader version of this code.

	// Intersect the ray with the slightly expanded globe.
	Sphere expanded_globe = Sphere(1.0 + shell_thickness);
	Interval expanded_globe_interval;
	if (!intersect_line(expanded_globe, ray, expanded_globe_interval))
	{
		// Shouldn't get here since this expanded sphere is bigger than globe and
		// we already intersected the globe.
		discard;
	}

	// The position of second intersection of ray with globe.
	vec3 back_globe_position = at(ray, globe_interval.to);

	// The position of first and second intersections of ray with expanded globe.
	vec3 front_expanded_globe_position = at(ray, expanded_globe_interval.from);
	vec3 back_expanded_globe_position = at(ray, expanded_globe_interval.to);

	// The amount of spherical shell traversed by the ray.
	float ray_shell_thickness =
			distance(front_expanded_globe_position, back_expanded_globe_position) -
			distance(front_globe_position, back_globe_position);

	// Modify the alpha channel of the background colour.
	// A ray travelling through centre of globe travels through '2 * shell_thickness', and hence
	// will not modify the alpha (since '1 - pow(1 - alpha, 1.0) == 1.0').
	// Non-central rays will increase alpha.
	float alpha = 1.0 - pow(1.0 - background_color.a, ray_shell_thickness / (2 * shell_thickness));
	
	// Replace background alpha with newly calculated value.
	gl_FragColor = vec4(background_color.rgb, alpha);
}
