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
// Fragment shader source code to render volume fill spherical caps.
//

// The surface fill mask texture.
uniform sampler2DArray surface_fill_mask_sampler;

// The world-space coordinates are interpolated across the spherical cap geometry.
varying vec3 world_position;

void main (void)
{
	// Project the world position onto the cube face and determine the cube face that it projects onto.
	int cube_face_index;
	vec3 projected_world_position = project_world_position_onto_cube_face(world_position, cube_face_index);

	// Transform the world position (projected onto a cube face) into that cube face's local coordinate frame.
	vec2 cube_face_coordinate_xy = convert_projected_world_position_to_local_cube_face_coordinate_frame(
			projected_world_position, cube_face_index);

	// Convert range [-1,1] to [0,1].
	vec2 cube_face_coordinate_uv = 0.5 * cube_face_coordinate_xy + 0.5;

	// Sample the surface fill mask texture array.
	float surface_fill_mask = sample_surface_fill_mask_texture_array(
			surface_fill_mask_sampler,
			cube_face_index,
			cube_face_coordinate_uv);

	// If the current world position does not project into the surface mask then discard the current fragment
	// because it means we're outside the surface fill mask (which is outside the fill volume).
	// NOTE: We test against 0.0 instead of 0.5 (like 'projects_into_surface_fill_mask()' does) because this
	// expands the surface fill mask slightly (up to a half a texel) and we don't want any gaps to form between
	// the spherical caps and the walls (the vertically extruded boundary of surface fill mask).
	if (surface_fill_mask == 0.0)
	{
		discard;
	}

#ifdef DEPTH_RANGE
	// Write *screen-space* depth (ie, depth range [-1,1] and not [n,f]).
	// This is what's used in the ray-tracing shader since it uses inverse model-view-proj matrix on the depth
	// to get world space position and that requires normalised device coordinates not window coordinates).
	//
	// Convert window coordinates (z_w) to normalised device coordinates (z_ndc) which, for depth, is:
	//   [n,f] -> [-1,1]
	// ...where [n,f] is set by glDepthRange (default is [0,1]). The conversion is:
	//   z_w = z_ndc * (f-n)/2 + (n+f)/2
	// ...which is equivalent to:
	//   z_ndc = [2 * z_w - n - f)] / (f-n)
	//
	gl_FragColor = vec4((2 * gl_FragCoord.z - gl_DepthRange.near - gl_DepthRange.far) / gl_DepthRange.diff);
#endif
}
