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
// Fragment shader for rendering a vertical cross-section through a 3D scalar field.
//
// The cross-section is formed by extruding a surface polyline (or polygon) from the
// field's outer depth radius to its inner depth radius.
//

uniform sampler2DArray tile_meta_data_sampler;
uniform sampler2DArray field_data_sampler;
uniform sampler2DArray mask_data_sampler;
uniform sampler1D depth_radius_to_layer_sampler;
uniform sampler1D colour_palette_sampler;
uniform sampler2DArray surface_fill_mask_sampler;


//
// Colour options.
//

// NOTE: Only one of these is true.
//       Also note that depth colour mode is not available for cross-section rendering (only isosurface rendering).
uniform bool colour_mode_scalar;
uniform bool colour_mode_gradient;

// The min/max range of values used to map to colour whether that mapping is a look up
// of the colour palette (eg, colouring by scalar value or gradient magnitude) or by using
// a hard-wired mapping in the shader code.
uniform vec2 min_max_colour_mapping_range;


layout (std140) uniform ScalarField
{
	// min/max scalar value across the entire scalar field
	vec2 min_max_scalar_value;

	// min/max gradient magnitude across the entire scalar field
	vec2 min_max_gradient_magnitude;
} scalar_field_block;

layout(std140) uniform Depth
{
    // The actual minimum and maximum depth radii of the scalar field.
    vec2 min_max_depth_radius;

    // The depth range rendering is restricted to.
    // If depth range is not restricted then this is the same as 'min_max_depth_radius'.
    // Also the following conditions hold:
    //	min_max_depth_radius_restriction.x >= min_max_depth_radius.x
    //	min_max_depth_radius_restriction.y <= min_max_depth_radius.y
    // ...in other words the depth range for rendering is always within the actual depth range.
    vec2 min_max_depth_radius_restriction;

	// Number of depth layers in scalar field.
	int num_depth_layers;
} depth_block;

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

layout (std140) uniform Lighting
{
	bool enabled;
	float ambient;
	vec3 world_space_light_direction;
} lighting_block;

layout (std140) uniform Test
{
	// Test variables (in range [0,1]).
	// Initial values are zero.
	float variable[16];
} test_block;


in VertexData
{
	// The world-space coordinates are interpolated across the cross-section geometry.
	vec3 world_position;

	// The lighting for the front (x component) and back (y component) face.
	// Note that the face normal is constant across the (cross-section) face.
	vec2 front_and_back_face_lighting;
} fs_in;

layout (location = 0) out vec4 colour;
layout (location = 1) out vec4 depth;

void main()
{
	// Sample the field at the current world position in the cross-section.
	// Discard if the field at the current world position contains no valid data.
	int cube_face_index;
	vec2 cube_face_coordinate_uv;
	float field_scalar;
	vec3 field_gradient;
	if (!get_scalar_field_data_from_position(
			fs_in.world_position,
			tile_meta_data_sampler, mask_data_sampler, field_data_sampler,
			depth_radius_to_layer_sampler, depth_block.num_depth_layers, depth_block.min_max_depth_radius,
			cube_face_index, cube_face_coordinate_uv,
			field_scalar, field_gradient))
	{
		discard;
	}
	
	// If using a surface fill mask and current cross section pixel is outside mask region then discard.
	if (surface_fill_block.using_surface_fill_mask)
	{
		if (!projects_into_surface_fill_mask(surface_fill_mask_sampler, cube_face_index, cube_face_coordinate_uv))
		{
			discard;
		}
	}

	colour = vec4(0);

	// Look up the colour palette using scalar value (or gradient magnitude).
	if (colour_mode_scalar)
	{
		colour = look_up_table_1D(
				colour_palette_sampler,
				min_max_colour_mapping_range.x, // input_lower_bound
				min_max_colour_mapping_range.y, // input_upper_bound
				field_scalar);
	}
	else if (colour_mode_gradient)
	{
		// NOTE: We have an asymmetric map of ||gradient|| to colour.
		// We do not have a separate mapping of -||gradient|| to colour as is the case for isosurface rendering
		// because we cannot easily determine front and back of isosurface when rendering cross-sections.
		// So we only look up *positive* gradient (magnitude) - ie, we never negate the magnitude.
		// UPDATE: Actually it would be possible to determine front and back of isosurface from the sign of
		// dot(view_direction, gradient)... we just need view_direction which, in orthographic view, is
		// constant and, in perspective view, is difference of world_position and eye position.
		// However it could be confusing to view this across a flat cross section.
		colour = look_up_table_1D(
				colour_palette_sampler,
				min_max_colour_mapping_range.x, // input_lower_bound
				min_max_colour_mapping_range.y, // input_upper_bound
				length(field_gradient));
	}

#if 1
	// If the current pixel is transparent then discard it.
	//
	// Note: We avoid final alpha values other than 0.0 or 1.0 in order
	// to avoid order-dependent alpha-blending artifacts.
	// These occur due to the fact that the cross sections are not drawn in
	// back to front order (relative to the camera). If cross-section A
	// partially occludes cross-section B, but A is drawn before B then
	// A will write semi-transparent (border) pixels into the colour buffer
	// (eg, blended with a black background) and the depth buffer will record
	// the depth of these pixels. When B is then drawn it will not overwrite the
	// A pixels (including the semi-transparent border pixels) and this leaves
	// darkened edges around A (due to the black background).
	// So instead we threshold alpha with 0.5 such that [0,0.5] -> 0.0 and [0.5,1] -> 1.0.
	// This is essentially alpha cutouts. And since our 1D colour texture has been dilated
	// one texel (for fully transparent texels neighbouring opaque texels) we don't have
	// the issue of bilinear texture filtering including RGB black values in the final colour.
	//
	//      ---
	//     | B |
	//  -----------
	// |     A     |
	//  -----------
	//     | B |
	//      ---
	//
	if (colour.a < 0.5)
	{
		discard;
	}
	colour.a = 1;
#else
	// If the current pixel is fully transparent then discard it.
	if (colour.a == 0)
	{
		discard;
	}
#endif

	// Output colour as pre-multiplied alpha.
	// This enables alpha-blending to use (src,dst) blend factors of (1, 1-SrcAlpha) instead of
	// (SrcAlpha, 1-SrcAlpha) which keeps alpha intact (avoids double-blending when rendering to
	// a render-target and then, in turn, blending that into the main framebuffer).
	// Note: We don't have semi-transparent cross-sections (yet).
	colour.rgb *= colour.a;

	// This branches on a uniform variable and hence is efficient since all pixels follow same path.
	if (lighting_block.enabled)
	{
		// Choose between the front and back face lighting (generated by vertex shader).
		float diffuse = mix(fs_in.front_and_back_face_lighting.y/*back*/, fs_in.front_and_back_face_lighting.x/*front*/, float(gl_FrontFacing));

		colour.rgb *= diffuse;
	}

	// Note that we're not writing to 'gl_FragDepth'.
	// This means the fixed-function depth will get written to the hardware depth buffer
	// which is what we want for non-ray-traced geometries.
	//
	// Also note that we don't need to read a depth texture for efficiency like the iso-surface shader does
	// because the hardware depth buffer should be relatively efficient at fragment culling
	// (since we're not outputting fragment depth in the shader which reduces hardware depth buffer efficiency).
	// 

	//
	// Using multiple render targets here.
	//
	// This is in case another scalar field is rendered as an iso-surface in which case
	// it cannot use the hardware depth buffer because it's not rendering traditional geometry.
	// Well it can, but it's not efficient because it cannot early cull rays (it needs to do the full
	// ray trace before it can output fragment depth to make use of the depth buffer).
	// So instead it must query a depth texture in order to early-cull rays.
	// The hardware depth buffer is still used for correct depth sorting of the cross-sections and iso-surfaces.
	// Note that this doesn't work with semi-transparency but that's a much harder problem to solve - depth peeling.
	//

	// We write to both 'colour' and 'depth' with the latter being ignored if glDrawBuffers is NONE for draw buffer index 1 (the default).
	//
	// Note that 'colour' is render target 0 due to:
	//
	//   layout (location = 0) out vec4 colour;
	//
	//
	// Write *screen-space* depth (ie, depth range [-1,1] and not [0,1]) to render target 1.
	// This is what's used in the ray-tracing shader since it uses inverse model-view-proj matrix on the depth
	// to get world space position and that requires normalised device coordinates not window coordinates).
	// Ideally this should also consider the effects of glDepthRange but we'll assume it's set to [0,1].
	//
	// Note the 'depth' is render target 1 due to:
	//
	//   layout (location = 1) out vec4 depth;
	//
	depth = vec4(2 * gl_FragCoord.z - 1);
}
