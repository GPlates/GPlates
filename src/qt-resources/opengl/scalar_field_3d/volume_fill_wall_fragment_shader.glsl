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
// Fragment shader source code to render volume fill walls (vertically extruded quads).
//

#ifdef SURFACE_NORMALS
	// The surface fill mask texture.
	uniform sampler2DArray surface_fill_mask_sampler;
	uniform int surface_fill_mask_resolution;

	// Is true if we should only should walls extruded from the boundary of the 2D surface fill mask.
	uniform bool only_show_boundary_walls;

	// The world-space coordinates are interpolated across the wall geometry.
	varying vec3 world_position;

	// The surface normal for the front-face of the wall.
	varying vec3 front_surface_normal;
#endif

void main (void)
{
#ifdef SURFACE_NORMALS
	// This branches on a uniform variable and hence is efficient since all pixels follow same path.
	if (only_show_boundary_walls)
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
				surface_fill_mask_resolution,
				cube_face_index,
				cube_face_coordinate_uv);

		// If the current world position projects *fully* into the surface mask then discard the current fragment
		// because it means we're *completely* inside the surface fill mask (which is inside the fill volume).
		// And this means we belong to a wall that is not on the boundary of a surface fill mask region.
		// We only want to render the boundary walls (the interior walls provide to much viewing occlusion).
		// NOTE: We test against 1.0 instead of 0.5 (like 'projects_into_surface_fill_mask()' does) because this
		// contracts the surface fill mask slightly (up to a half a texel) and we don't want to discard any part
		// of walls that form part of the surface fill mask boundary.
		if (surface_fill_mask == 1.0)
		{
			discard;
		}
	}

	// Choose between the front and back facing surface normal.
	vec3 normal = mix(-front_surface_normal/*back*/, front_surface_normal/*front*/, float(gl_FrontFacing));

	// The normal gets stored in RGB channels of fragment colour.
	// The normal will get stored to a floating-point texture so no need for convert to unsigned.
	gl_FragColor.xyz = normal;
	
	// Write the screen-space depth to the alpha channel.
	// The  *screen-space* depth (ie, depth range [-1,1] and not [0,1]).
	// This is what's used in the ray-tracing shader since it uses inverse model-view-proj matrix on the depth
	// to get world space position and that requires normalised device coordinates not window coordinates).
	// Ideally this should also consider the effects of glDepthRange but we'll assume it's set to [0,1].
	gl_FragColor.w = 2 * gl_FragCoord.z - 1;
#endif

#ifdef DEPTH_RANGE
	// Write *screen-space* depth (ie, depth range [-1,1] and not [0,1]).
	// This is what's used in the ray-tracing shader since it uses inverse model-view-proj matrix on the depth
	// to get world space position and that requires normalised device coordinates not window coordinates).
	// Ideally this should also consider the effects of glDepthRange but we'll assume it's set to [0,1].
	gl_FragColor = vec4(2 * gl_FragCoord.z - 1);
#endif
}
