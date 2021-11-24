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
// Fragment shader source code to render volume fill wall surface normals (and depth) with vertically extruded quads.
//

// The surface fill mask texture.
uniform sampler2DArray surface_fill_mask_sampler;

layout(std140) uniform SurfaceFill
{
	// If using a surface mask to limit regions of scalar field to render
	// (e.g. Africa, see Static Polygons connected with Scalar Field).
	bool using_surface_fill_mask;

	// If this is *true* (and 'using_surface_fill_mask' is true) then we're rendering the walls of the extruded fill volume.
	// The volume fill (screen-size) texture contains the surface normal and depth of the walls.
	//
	// If this is *false* (and 'using_surface_fill_mask' is true) then we're using the min/max depth range of walls
	// of extruded fill volume to limit raytracing traversal (but we're not rendering the walls as visible).
	// The volume fill (screen-size) texture will contain the min/max depth range of the walls.
	// This enables us to reduce the length along each ray that is sampled/traversed.
	// We don't need this if the walls are going to be drawn because there are already good
	// optimisations in place to limit ray sampling based on the fact that the walls are opaque.
	bool show_volume_fill_walls;

	// Is true if we should only render walls extruded from the *boundary* of the 2D surface fill mask.
	bool only_show_boundary_volume_fill_walls;
} surface_fill_block;

in VertexData
{
	// The world-space coordinates are interpolated across the wall geometry.
	vec3 world_position;

	// The surface normal for the front-face of the wall.
	vec3 front_surface_normal;
} fs_in;

layout (location = 0) out vec4 surface_normal_and_depth;

void main (void)
{
	if (surface_fill_block.only_show_boundary_volume_fill_walls)
	{
		// Project the world position onto the cube face and determine the cube face that it projects onto.
		int cube_face_index;
		vec3 projected_world_position = project_world_position_onto_cube_face(fs_in.world_position, cube_face_index);

		// Transform the world position (projected onto a cube face) into that cube face's local coordinate frame.
		vec2 cube_face_coordinate_xy = convert_projected_world_position_to_local_cube_face_coordinate_frame(
				projected_world_position, cube_face_index);

		// Convert range [-1,1] to [0,1].
		vec2 cube_face_coordinate_uv = 0.5 * cube_face_coordinate_xy + 0.5;

		// Sample the surface fill mask texture array.
		// NOTE: We use the dilated version of the sampling function which averages 3x3 (bilinearly filtered) samples.
		// This emulates a pre-processing dilation of the surface fill mask.
		float surface_fill_mask = sample_dilated_surface_fill_mask_texture_array(
				surface_fill_mask_sampler,
				cube_face_index,
				cube_face_coordinate_uv);

		// If the current world position projects *fully* into the surface mask then discard the current fragment
		// because it means we're *completely* inside the surface fill mask (which is inside the fill volume).
		// And this means we belong to a wall that is not on the boundary of a surface fill mask region.
		// We only want to render the boundary walls (the interior walls provide too much viewing occlusion).
		// NOTE: We test against 1.0 instead of 0.5 (like 'projects_into_surface_fill_mask()' does) because this
		// contracts the surface fill mask slightly (up to a half a texel) and we don't want to discard any part
		// of walls that form part of the surface fill mask boundary.
		// Also note that we get a further 1.5 texel reach by using the 'dilation' version of the mask sampling above.
		// The upside is we don't mask out parts of the boundary walls (this artifact looks obvious in some places).
		// The downside to this is we don't fully trim the interior walls (but it's not that noticeable).
		if (surface_fill_mask == 1.0)
		{
			discard;
		}
	}

	// Choose between the front and back facing surface normal.
	vec3 normal = mix(-fs_in.front_surface_normal/*back*/, fs_in.front_surface_normal/*front*/, float(gl_FrontFacing));

	// The normal gets stored in RGB channels of fragment colour.
	// The normal will get stored to a floating-point texture so no need for convert to unsigned.
	surface_normal_and_depth.xyz = normal;
	
	// Write the screen-space depth to the alpha channel.
	// The *screen-space* depth (ie, depth range [-1,1] and not [n,f]).
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
	surface_normal_and_depth.w = (2 * gl_FragCoord.z - gl_DepthRange.near - gl_DepthRange.far) / gl_DepthRange.diff;
}
