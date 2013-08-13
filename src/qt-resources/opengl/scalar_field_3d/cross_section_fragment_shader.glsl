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

// "#extension" needs to be specified in the shader source *string* where it is used (this is not
// documented in the GLSL spec but is mentioned at http://www.opengl.org/wiki/GLSL_Core_Language).
#extension GL_EXT_texture_array : enable
// Shouldn't need this since '#version 120' (in separate source string) supports 'gl_FragData', but just in case...
#extension GL_ARB_draw_buffers : enable

//
// Uniform variables default to zero according to the GLSL spec:
//
// "The link time initial value is either the value of the
// variable's initializer, if present, or 0 if no initializer is present."
//

// Test variables (in range [0,1]) set via the scalar field visual layer GUI.
// Initial values are zero.
uniform float test_variable_0;
uniform float test_variable_1;
uniform float test_variable_2;
uniform float test_variable_3;
uniform float test_variable_4;
uniform float test_variable_5;
uniform float test_variable_6;
uniform float test_variable_7;
uniform float test_variable_8;
uniform float test_variable_9;

uniform sampler2DArray tile_meta_data_sampler;
uniform sampler2DArray field_data_sampler;
uniform sampler2DArray mask_data_sampler;
uniform sampler1D depth_radius_to_layer_sampler;
uniform sampler1D colour_palette_sampler;

uniform int tile_meta_data_resolution;
uniform int tile_resolution;
uniform int depth_radius_to_layer_resolution;
uniform int colour_palette_resolution;
uniform vec2 min_max_depth_radius;
uniform int num_depth_layers;
uniform vec2 min_max_colour_mapping_range;
uniform vec2 min_max_scalar_value;
uniform vec2 min_max_gradient_magnitude;

uniform bool lighting_enabled;

//
// "Colour mode" options.
//
// NOTE: Only one of these is true.
// Also note that depth colour mode is not available for cross-section rendering (only isosurface rendering).
//
uniform bool colour_mode_scalar;
uniform bool colour_mode_gradient;

// If using the surface occlusion texture.
uniform bool read_from_surface_occlusion_texture;
// The surface occlusion texture.
// This contains the RGBA contents of rendering the front surface of the globe.
uniform sampler2D surface_occlusion_texture_sampler;

// If using a surface mask to limit regions in which cross sections are rendered.
// (e.g. Africa, see Static Polygons connected with Scalar Field).
uniform bool using_surface_fill_mask;
uniform sampler2DArray surface_fill_mask_sampler;
uniform int surface_fill_mask_resolution;

// The world-space coordinates are interpolated across the cross-section geometry.
varying vec3 world_position;

// Screen-space coordinates are post-projection coordinates in the range [-1,1].
varying vec2 screen_coord;

// The lighting for the front (x component) and back (y component) face.
varying vec2 front_and_back_face_lighting;

void main()
{
	// This branches on a uniform variable and hence is efficient since all pixels follow same path.
	if (read_from_surface_occlusion_texture)
	{
		// If a surface object on the front of the globe has an alpha value of 1.0 (and hence occludes
		// the current ray) then discard the current pixel.
		// Convert [-1,1] range to [0,1] for texture coordinates.
		//
		// TODO: This test might not improve efficiency - and so needs profiling to see if worth it.
		float surface_occlusion = texture2D(surface_occlusion_texture_sampler, 0.5 * screen_coord + 0.5).a;
		if (surface_occlusion == 1.0)
		{
			discard;
		}
	}

	// Sample the field at the current world position in the cross-section.
	// Discard if the field at the current world position contains no valid data.
	int cube_face_index;
	vec2 cube_face_coordinate_uv;
	float field_scalar;
	vec3 field_gradient;
	if (!get_scalar_field_data_from_position(
			world_position,
			tile_meta_data_sampler, tile_meta_data_resolution,
			mask_data_sampler, tile_resolution, field_data_sampler,
			depth_radius_to_layer_sampler, depth_radius_to_layer_resolution,
			num_depth_layers, min_max_depth_radius,
			cube_face_index, cube_face_coordinate_uv,
			field_scalar, field_gradient))
	{
		discard;
	}
	
	// If using a surface fill mask and current cross section pixel is outside mask region then discard.
	if (using_surface_fill_mask)
	{
		if (!projects_into_surface_fill_mask(
				surface_fill_mask_sampler, surface_fill_mask_resolution,
				cube_face_index, cube_face_coordinate_uv))
		{
			discard;
		}
	}

	vec4 colour = vec4(0,0,0,0);

	// Look up the colour palette using scalar value (or gradient magnitude).
	if (colour_mode_scalar)
	{
		colour = look_up_table_1D(
				colour_palette_sampler,
				colour_palette_resolution,
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
		colour = look_up_table_1D(
				colour_palette_sampler,
				colour_palette_resolution,
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
	if (lighting_enabled)
	{
		// Choose between the front and back face lighting (generated by vertex shader).
		float diffuse = mix(front_and_back_face_lighting.y/*back*/, front_and_back_face_lighting.x/*front*/, float(gl_FrontFacing));

		colour.rgb *= diffuse;
	}

	// Note that we're not writing to 'gl_FragDepth'.
	// This means the fixed-function depth will get written to the hardware depth buffer
	// which is what we want for non-ray-traced geometries.
	// Also note that we don't need to read a depth texture like the iso-surface shader does
	// because the hardware depth buffer should be relatively efficient at fragment culling
	// (since we're not outputing fragment depth in the shader).

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

	// According to the GLSL spec...
	//
	// "If a shader statically assigns a value to gl_FragColor, it may not assign a value
	// to any element of gl_FragData. If a shader statically writes a value to any element
	// of gl_FragData, it may not assign a value to gl_FragColor. That is, a shader may
	// assign values to either gl_FragColor or gl_FragData, but not both."
	// "A shader contains a static assignment to a variable x if, after pre-processing,
	// the shader contains a statement that would write to x, whether or not run-time flow
	// of control will cause that statement to be executed."
	//
	// ...so we can't branch on a uniform (boolean) variable here.
	// So we write to both gl_FragData[0] and gl_FragData[1] with the latter output being ignored
	// if glDrawBuffers is NONE for draw buffer index 1 (the default).

	// Write colour to render target 0.
	gl_FragData[0] = colour;

	// Write *screen-space* depth (ie, depth range [-1,1] and not [0,1]) to render target 1.
	// This is what's used in the ray-tracing shader since it uses inverse model-view-proj matrix on the depth
	// to get world space position and that requires normalised device coordinates not window coordinates).
	// Ideally this should also consider the effects of glDepthRange but we'll assume it's set to [0,1].
	gl_FragData[1] = vec4(2 * gl_FragCoord.z - 1);
}
