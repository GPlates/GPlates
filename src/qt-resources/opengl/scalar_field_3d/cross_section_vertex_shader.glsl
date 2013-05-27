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

//
// Vertex shader for rendering a vertical cross-section through a 3D scalar field.
//
// The cross-section is formed by extruding a surface polyline (or polygon) from the
// field's outer depth radius to its inner depth radius.
//

// The depth range rendering is restricted to.
// If depth range is not restricted then this is the same as 'min_max_depth_radius'.
// Also the following conditions hold:
//	render_min_max_depth_radius_restriction.x >= min_max_depth_radius.x
//	render_min_max_depth_radius_restriction.y <= min_max_depth_radius.y
// ...in other words the depth range for rendering is always within the actual depth range.
uniform vec2 render_min_max_depth_radius_restriction;

uniform bool lighting_enabled;
uniform vec3 world_space_light_direction;
uniform float light_ambient_contribution;

// The (x,y,z) values contain the point location on globe surface.
// The w value is 0 or 1 corresponding to min/max depth radius respectively.
attribute vec4 surface_point_xyz_depth_weight_w;
// The (x,y,z) values contain the neighbour point location on globe surface.
// The w value is 0 for 1D cross-sections.
// The w value is -1 or 1 for 2D cross-sections used to calculate the correct surface normal orientation.
attribute vec4 neighbour_surface_point_xyz_normal_weight_w;

// The world-space coordinates are interpolated across the cross-section geometry.
varying vec3 world_position;

// Screen-space coordinates are post-projection coordinates in the range [-1,1].
varying vec2 screen_coord;

// The lighting for the front (x component) and back (y component) face.
// Note that the face normal is constant across the (cross-section) face.
varying vec2 front_and_back_face_lighting;

void main()
{
	// Extract the surface location from the vertex attribute.
	vec3 surface_point = surface_point_xyz_depth_weight_w.xyz;
	// Extract the depth weight from the vertex attribute.
	float depth_weight = surface_point_xyz_depth_weight_w.w;

	// Vertically extrude the surface point to either the minimum or maximum depth restricted radius of the scalar field.
	vec3 depth_point = mix(
			render_min_max_depth_radius_restriction.x/*min*/,
			render_min_max_depth_radius_restriction.y/*max*/,
			depth_weight) * surface_point;

	gl_Position = gl_ModelViewProjectionMatrix * vec4(depth_point, 1);

	// The screen-space (x,y) coordinates.
	screen_coord = gl_Position.xy / gl_Position.w;

	// This assumes the cross-section geometry does not need a model transform (eg, reconstruction rotation).
	world_position = depth_point;

	// This branches on a uniform variable and hence is efficient since all pixels follow same path.
	if (lighting_enabled)
	{
		// Compute lighting for both the front and back face of the current cross-section quad (two coplanar triangles).
		// The fragment shader will choose between them based on what side the fragment is on.

		// Extract the neighbour surface location from the vertex attribute.
		vec3 neighbour_surface_point = neighbour_surface_point_xyz_normal_weight_w.xyz;
		// Extract the normal weight from the vertex attribute.
		float normal_weight = neighbour_surface_point_xyz_normal_weight_w.w;

		// If this vertex is part of a 1D cross-section (a vertical line)...
		if (normal_weight == 0.0)
		{
			// We can't calculate a normal for a 1D cross-section.
			// Instead we see if the vertical line (1D cross-section) is perpendicular to
			// the light direction giving it a maximum amount of diffuse lighting.
			// If the vertical line is in direction (or opposite) of light then it only gets ambient light.
			// This is the same lighting you'd get if the vertical line was a very thin cylindrical pipe
			// but you could always see the brightest side of it.

			// The front and back face lighting are the same.
			float front_and_back_face_lambert = length(cross(surface_point, world_space_light_direction));

			// Blend between ambient and diffuse lighting - when ambient is 1.0 there is no diffuse.
			front_and_back_face_lighting = vec2(mix(front_and_back_face_lambert, 1.0, light_ambient_contribution));
		}
		else // 2D cross-section...
		{
			// Calculate the front face normal.
			// Note that 'normal_weight' is -1 or 1 and ensures the surface normal faces forward.
			//
			// Note that it might be possible to get a zero normal vector - if this happens
			// then the cross-section quad (two tris) will likely be close to degenerate and
			// not generate any fragments anyway (so the incorrect lighting will not be visible).
			vec3 front_face_normal = normal_weight * cross(surface_point, neighbour_surface_point);

			// The Lambert term in diffuse lighting.
			// Note that the back face normal is negative of the front face normal.
			vec2 front_and_back_face_lambert = vec2(
				lambert_diffuse_lighting(world_space_light_direction, front_face_normal),
				lambert_diffuse_lighting(world_space_light_direction, -front_face_normal));

			// Blend between ambient and diffuse lighting - when ambient is 1.0 there is no diffuse.
			front_and_back_face_lighting = mix(front_and_back_face_lambert, vec2(1.0), vec2(light_ambient_contribution));
		}
	}
}
