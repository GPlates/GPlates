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
// Fragment shader for rendering a single iso-surface.
//

// GPlates currently moves this to start of *first* source code string passed into glShaderSource.
// This is because extension lines must not occur *after* any non-preprocessor source code.
//
// NOTE: Do not comment this out with a /**/ comment spanning multiple lines since GPlates does not detect that.
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
uniform float test_variable_10;
uniform float test_variable_11;
uniform float test_variable_12;
uniform float test_variable_13;
uniform float test_variable_14;
uniform float test_variable_15;

// data contains tile id from given face number and face uv coordinate
uniform sampler2DArray tile_meta_data_sampler;
// whole scalar field data containing scalar and gradient for every voxel
uniform sampler2DArray field_data_sampler;
// 2D array containing information about existance of data at given point
uniform sampler2DArray mask_data_sampler;
// array containing layer index from depth radius
uniform sampler1D depth_radius_to_layer_sampler;
// colour palette from isovalue
uniform sampler1D colour_palette_sampler;

// resolutions of sampler states
uniform int tile_meta_data_resolution;
uniform int tile_resolution;
uniform int depth_radius_to_layer_resolution;
uniform int num_depth_layers;
uniform int colour_palette_resolution;

// The min/max range of values used to map to colour whether that mapping is a look up
// of the colour palette (eg, colouring by scalar value or gradient magnitude) or by using
// a hard-wired mapping in the shader code.
uniform vec2 min_max_colour_mapping_range;

// constriction of scalar value
uniform vec2 min_max_scalar_value;
// constriction of gradient value
uniform vec2 min_max_gradient_magnitude;


//
// Lighting options.
//
uniform bool lighting_enabled;
uniform vec3 world_space_light_direction;
uniform float light_ambient_contribution;

// For testing purposes
// float light_ambient_contribution = test_variable_0;


//
// Depth occlusion options.
//

// If using the surface occlusion texture for early ray termination.
uniform bool read_from_surface_occlusion_texture;
// The surface occlusion texture used for early ray termination.
// This contains the RGBA contents of rendering the front surface of the globe.
uniform sampler2D surface_occlusion_texture_sampler;

// If using the depth texture for early ray termination.
uniform bool read_from_depth_texture;
// The depth texture used for early ray termination.
uniform sampler2D depth_texture_sampler;


//
// "Colour mode" options.
//
// NOTE: Only one of these is true.
//
uniform bool colour_mode_depth;
uniform bool colour_mode_isovalue;
uniform bool colour_mode_gradient;


//
// Isovalue options.
//
// First isovalue.
// (x,y,z) components are (isovalue, isovalue - lower deviation, isovalue + upper deviation).
// If deviation parameters are symmetric then lower and upper will have same values.
uniform vec3 isovalue1;
// Second isovalue.
// (x,y,z) components are (isovalue, isovalue - lower deviation, isovalue + upper deviation).
// If deviation parameters are symmetric then lower and upper will have same values.
uniform vec3 isovalue2;


//
// Render options.
//

// Opacity of deviation surfaces (including deviation window at globe surface).
uniform float opacity_deviation_surfaces;
// Is true if volume rendering in deviation window (between deviation surfaces).
uniform bool deviation_window_volume_rendering;
// Opacity of deviation window volume rendering.
// Only valid for 'deviation_window_volume_rendering'.
uniform float opacity_deviation_window_volume_rendering;
// Is true if rendering deviation window at globe surface.
uniform bool surface_deviation_window;
// The isoline frequency when rendering deviation window at globe surface and at volume fill walls.
uniform float surface_deviation_isoline_frequency;


//
// Surface fill mask options.
//

// If using a surface mask to limit regions of scalar field to render. (e.g. Africa, see Static Polygons connected with Scalar Field)
uniform bool using_surface_fill_mask;
uniform sampler2DArray surface_fill_mask_sampler;
uniform int surface_fill_mask_resolution;

// If using surface mask *and* rendering the walls of the extruded fill volume.
uniform bool show_volume_fill_walls;
uniform sampler2D volume_fill_wall_surface_normal_and_depth_sampler;

// If using surface mask *and* using min/max depth range of walls of extruded fill volume
// (but not rendering walls).
//
// If we have a surface fill mask, but we are not drawing the volume fill walls (surface normal and depth),
// then the min/max depth range of the volume fill walls has been generated (stored in screen-size texture).
// This enables us to reduce the length along each ray that is sampled/traversed.
// We don't need this if the walls are going to be drawn because there are already good
// optimisations in place to limit ray sampling based on the fact that the walls are opaque.
uniform bool using_volume_fill_wall_depth_range;
uniform sampler2D volume_fill_wall_depth_range_sampler;

//
// Depth range options.
//

// The actual minimum and maximum depth radii of the scalar field.
uniform vec2 min_max_depth_radius;
// The depth range rendering is restricted to.
// If depth range is not restricted then this is the same as 'min_max_depth_radius'.
// Also the following conditions hold:
//	render_min_max_depth_radius_restriction.x >= min_max_depth_radius.x
//	render_min_max_depth_radius_restriction.y <= min_max_depth_radius.y
// ...in other words the depth range for rendering is always within the actual depth range.
uniform vec2 render_min_max_depth_radius_restriction;


// Quality/performance options.
//
// Sampling rate along ray.
// (x,y) components are (distance between samples, inverse distance between samples).
uniform vec2 sampling_rate;
// Number of convergence iterations for accurate isosurface location.
uniform int bisection_iterations;


// Screen-space coordinates are post-projection coordinates in the range [-1,1].
varying vec2 screen_coord;


// Determine the crossing level ID for specified field scalar.
//
// Returns an integer in the set (0,1,2,3,4,5,6) depending on where the field scalar is
// relative to the two isovalues and their deviation isovalues.
int
get_crossing_level(
		float field_scalar)
{
	// This is equivalent to:
	//   int(field_scalar >= isovalue1.x) + int(field_scalar >= isovalue1.y) + int(field_scalar >= isovalue1.z) +
	//   int(field_scalar >= isovalue2.x) + int(field_scalar >= isovalue2.y) + int(field_scalar >= isovalue2.z)
	return int(
			dot(vec3(1), vec3(greaterThanEqual(vec3(field_scalar), isovalue1))) +
			dot(vec3(1), vec3(greaterThanEqual(vec3(field_scalar), isovalue2))));
}

// Function to contract a ray interval that spans a valid/invalid field data region boundary such
// that the contracted interval is maximally within the valid region.
// This contraction avoids noise artifacts where the isosurface meets the valid region boundary.
//
// Precondition:
//  One, and exactly one, of 'valid_back' and 'valid_front' must be true.
//
// We're narrowing in on the boundary between valid and invalid data.
// For example the georeference boundary of a regional (non-global) field.
void
contract_interval_to_valid_region(
		Ray ray,
		inout vec3 ray_sample_position,
		inout float lambda,
		inout float interval_length,
		inout int crossing_level_back,
		inout int crossing_level_front,
		inout bool active_back,
		inout bool active_front,
		inout bool valid_back,
		inout bool valid_front,
		inout float field_scalar,
		inout vec3 field_gradient)
{
	//
	// Precondition: 'valid_front ^^ valid_back' is true.
	//

	vec3 ray_sample_position_center = ray_sample_position;
	float lambda_center = lambda;
	float lambda_increment = interval_length;
	bool interval_contracted = false;

	vec3 ray_sample_position_valid = ray_sample_position;
	float lambda_valid = lambda;
	
	// current sampling interval [a,b] is divided by 2 into intervals [a, 0.5 * (a + b)] and [0.5 * (a + b), b]
	for (int i = 0; i < bisection_iterations; i++)
	{
		lambda_increment *= 0.5;
		ray_sample_position_center -= lambda_increment * ray.direction;
		lambda_center -= lambda_increment;

		bool valid_center = is_valid_field_data_sample(
				ray_sample_position_center, tile_meta_data_sampler, tile_meta_data_resolution,
				tile_resolution, mask_data_sampler);
		if (valid_center)
		{
			ray_sample_position_valid = ray_sample_position_center;
			lambda_valid = lambda_center;
			interval_contracted = true;
			
			// Move forwards along the ray in the valid region.
			if (valid_back)
			{
				ray_sample_position_center += lambda_increment * ray.direction;
				lambda_center += lambda_increment;
			}
			// else move backwards along the ray in the valid region.
		}
		else
		{
			// Move forwards along the ray to try and get out of the invalid region.
			if (valid_front)
			{
				ray_sample_position_center += lambda_increment * ray.direction;
				lambda_center += lambda_increment;
			}
			// else move backwards along the ray to try and get out of the invalid region.
		}
	}
	
	if (interval_contracted)
	{
		// Sample the field at the latest valid position.
		int cube_face_index_valid;
		vec2 cube_face_coordinate_uv_valid;
		float field_scalar_valid;
		vec3 field_gradient_valid;
		get_scalar_field_data_from_position(
				ray_sample_position_valid,
				tile_meta_data_sampler, tile_meta_data_resolution, mask_data_sampler,
				tile_resolution, field_data_sampler, depth_radius_to_layer_sampler,
				depth_radius_to_layer_resolution, num_depth_layers, min_max_depth_radius,
				cube_face_index_valid, cube_face_coordinate_uv_valid,
				field_scalar_valid, field_gradient_valid);
		
		if (valid_front)
		{
			// Calculate contracted interval.
			// The front position (lambda) is unchanged but the back position (lambda_valid) has moved towards the front.
			interval_length = lambda - lambda_valid;
		
			// Change the 'back' of the interval - it now has a valid position.
			valid_back = true;
			active_back = true;
			if (using_surface_fill_mask)
			{
				active_back = projects_into_surface_fill_mask(
						surface_fill_mask_sampler, surface_fill_mask_resolution,
						cube_face_index_valid, cube_face_coordinate_uv_valid);
			}
			crossing_level_back = get_crossing_level(field_scalar_valid);
		}
		else // valid_back ...
		{
			// Calculate contracted interval.
			// The back position (lambda_back) is unchanged but the front position (lambda_valid) has moved towards the back.
			float lambda_back = lambda - interval_length;
			interval_length = lambda_valid - lambda_back;
		
			// Change the 'front' of the interval - it now has a valid position.
			valid_front = true;
			active_front = true;
			if (using_surface_fill_mask)
			{
				active_front = projects_into_surface_fill_mask(
						surface_fill_mask_sampler, surface_fill_mask_resolution,
						cube_face_index_valid, cube_face_coordinate_uv_valid);
			}
			crossing_level_front = get_crossing_level(field_scalar_valid);
			
			// Set some values that only get set for the 'front' (most recent) ray sample.
			lambda = lambda_valid;
			ray_sample_position = ray_sample_position_valid;
			field_scalar = field_scalar_valid;
			field_gradient = field_gradient_valid;
		}
	}
}

// Function for isosurface/deviation window bisection position correction
//
// Precondition:
//  Both 'back' and 'front' should be valid.
void
isosurface_bisection_correction(
		Ray ray,
		inout vec3 ray_sample_position,
		inout float lambda,
		float lambda_increment,
		inout int crossing_level_back,
		inout int crossing_level_front,
		inout bool active_back,
		inout bool active_front,
		inout float field_scalar,
		inout vec3 field_gradient)
{
	// current sampling interval [a,b] is divided by 2 and 
	// isosurface crossing is checked for the two new intervals [a, 0.5 * (a + b)] and [0.5 * (a + b), b]
	for (int i = 0; i < bisection_iterations; i++)
	{
		lambda_increment *= 0.5;
		ray_sample_position -= lambda_increment * ray.direction;
		lambda -= lambda_increment;

		// Sample the field at the current ray position.
		int cube_face_index;
		vec2 cube_face_coordinate_uv;
		// If the field sample is not valid then discontinue bisection iteration -
		// the current iteration position is as close as we can get with bisection.
		// This is unlikely since the original interval had a valid 'back' and 'front' and
		// the field valid regions (essentially georeferencing) are unlikely to have concavities.
		if (!get_scalar_field_data_from_position(
				ray_sample_position, 
				tile_meta_data_sampler, tile_meta_data_resolution,
				mask_data_sampler, tile_resolution, field_data_sampler,
				depth_radius_to_layer_sampler, depth_radius_to_layer_resolution,
				num_depth_layers, min_max_depth_radius,
				cube_face_index, cube_face_coordinate_uv,
				field_scalar, field_gradient))
		{
			return;
		}
		
		bool active_center = true;
		if (using_surface_fill_mask)
		{
			active_center = projects_into_surface_fill_mask(
					surface_fill_mask_sampler, surface_fill_mask_resolution,
					cube_face_index, cube_face_coordinate_uv);
		}
		
		int crossing_level_center = get_crossing_level(field_scalar);

		// if current crossing level is not equal to our 
		if (crossing_level_center != crossing_level_back)
		{
			crossing_level_front = crossing_level_center;
			active_front = active_center;
		} 
		else 
		{
			crossing_level_back = crossing_level_center;
			ray_sample_position += lambda_increment * ray.direction;
			lambda += lambda_increment;
			active_back = active_center;
		}
	}
}

// Function for returning the colour when sampling intervals contain transparent window enclosing surface,
// isosurface or several surfaces combined in one interval.
vec4
get_blended_crossing_colour(
		float ray_sample_depth_radius,
		Ray ray,
		vec3 normal,
		int crossing_level_back,
		int crossing_level_front,
		bool positive_crossing,
		vec4 colour_matrix_iso[6],
		vec3 field_gradient)
{
	// Special view dependent opacity correction for surfaces resulting in improved 3D effects
	float alpha = 1 - exp (log (1 - opacity_deviation_surfaces) / abs( dot(normal,ray.direction)));

	vec4 colour_matrix[6];

	// Switch between different colour modes.
	
	// Gradient strength to isosurface colour-mapping
	if (colour_mode_gradient)
	{
		// The first window surface (lowest isovalue) is the one with a gradient pointing "to" the isosurface.
		// The second window has a gradient pointing "away" from the isosurface.
		// The first window maps ||gradient|| to colour.
		// The second window maps -||gradient|| to colour.
		float field_gradient_magnitude = length(field_gradient);
		
		// Colour order of surfaces in deviation window zone around isovalue1.
		colour_matrix[0] = vec4(1,1,1,alpha) * look_up_table_1D(
				colour_palette_sampler, colour_palette_resolution,
				min_max_colour_mapping_range.x, min_max_colour_mapping_range.y,
				field_gradient_magnitude);
		colour_matrix[1] = vec4(1);
		colour_matrix[2] = vec4(1,1,1,alpha) * look_up_table_1D(
				colour_palette_sampler, colour_palette_resolution,
				min_max_colour_mapping_range.x, min_max_colour_mapping_range.y,
				-field_gradient_magnitude);
		
		// Colour order of surfaces in deviation window zone around isovalue2.
		colour_matrix[3] = colour_matrix[0];
		colour_matrix[4] = colour_matrix[1];
		colour_matrix[5] = colour_matrix[2];
	}

	// Depth to colour mapping
	// Mapping: [radius=0,radius=1] -> [blue,cyan] for window surface isovalue + deviation
	//			[radius=0,radius=1] -> [red,yellow] for window surface isovalue - deviation
	//			Isosurface is white.
	if (colour_mode_depth)
	{
		// Relative depth: 0 -> core , 1 -> surface (for restricted max/min depth range)
		float depth_relative = (ray_sample_depth_radius - render_min_max_depth_radius_restriction.x) /
				(render_min_max_depth_radius_restriction.y - render_min_max_depth_radius_restriction.x);
		
		// Colour order of surfaces in deviation window zone around isovalue1.
		colour_matrix[0] = vec4(1, depth_relative, 0, alpha);
		colour_matrix[1] = vec4(1);
		colour_matrix[2] = vec4(0, depth_relative, 1, alpha); 
		
		// Colour order of surfaces in deviation window zone around isovalue2.
		colour_matrix[3] = colour_matrix[0];
		colour_matrix[4] = colour_matrix[1];
		colour_matrix[5] = colour_matrix[2];
	}

	// Isovalue colour mapping
	// Mapping according to colourmap. Colour for window surfaces is stored in colour_matrix_iso.
	if (colour_mode_isovalue)
	{
		// Note: Factor in the alpha of the isovalue colour in case it's transparent.

		// Colour order of surfaces in deviation window zone around isovalue1.
		colour_matrix[0] = vec4(colour_matrix_iso[0].rgb, alpha * colour_matrix_iso[0].a);
		colour_matrix[1] = vec4(1);
		colour_matrix[2] = vec4(colour_matrix_iso[2].rgb, alpha * colour_matrix_iso[2].a);
		// Colour order of surfaces in deviation window zone around isovalue2.
		colour_matrix[3] = vec4(colour_matrix_iso[3].rgb, alpha * colour_matrix_iso[3].a);
		colour_matrix[4] = vec4(1);
		colour_matrix[5] = vec4(colour_matrix_iso[5].rgb, alpha * colour_matrix_iso[5].a);
	}

	float diffuse = 1;

	if (lighting_enabled)
	{
		// The Lambert term in diffuse lighting.
		float lambert = lambert_diffuse_lighting(world_space_light_direction, normal);

		// Blend between ambient and diffuse lighting - when ambient is 1.0 there is no diffuse.
		diffuse = mix(lambert, 1.0, light_ambient_contribution);
	}

	vec4 colour_crossing = vec4(0,0,0,0);
	
	// Blending of colours of surfaces in the deviation window zones for isovalue1 and isovalue2
	if (positive_crossing)
	{
		// Ray crosses surfaces in gradient direction: dot(ray.direction,gradient) > 0
		for (int i = crossing_level_back; i < crossing_level_front; i++)
		{
			colour_crossing.rgb += (1-colour_crossing.a) * colour_matrix[i].rgb * colour_matrix[i].a * diffuse;
			colour_crossing.a += (1-colour_crossing.a) * colour_matrix[i].a;
		}
	}
	else
	{
		// Ray crosses surfaces in opposite gradient direction: dot(ray.direction,gradient) < 0
		for (int i = crossing_level_back - 1; i >= crossing_level_front; i--)
		{
			colour_crossing.rgb += (1-colour_crossing.a) * colour_matrix[i].rgb * colour_matrix[i].a * diffuse;
			colour_crossing.a += (1-colour_crossing.a) * colour_matrix[i].a;
		}
	}

	return colour_crossing;
}


// Function for returning the colour of transparant volume between window enclosing surfaces and isosurface if volume rendering is turned on
vec4
get_volume_colour(
		float ray_sample_depth_radius,
		int crossing_level_front_mod,
		float field_scalar,
		vec3 field_gradient)
{
	vec4 colour_volume;

	// Switch between different colour modes			
	
	// Gradient strength colour-mapping
	if (colour_mode_gradient)
	{
		int colour_sign = 2 * int( crossing_level_front_mod == 1 ) - 1;
		
		// 'colour_sign == 1': sampling point in region with gradient pointing towards isosurface
		// Map ||gradient|| to colour.
		//
		// 'colour_sign == -1': sampling point in region with gradient pointing away from isosurface
		// Map -||gradient|| to colour.
		
		colour_volume = look_up_table_1D(
				colour_palette_sampler, colour_palette_resolution,
				min_max_colour_mapping_range.x, min_max_colour_mapping_range.y,
				colour_sign * length(field_gradient));

		return colour_volume;
	}

	// Depth to colour mapping
	// Mapping: [radius=0,radius=1] -> [blue,cyan] for zone [isovalue,isovalue + deviation]
	//			[radius=0,radius=1] -> [red,yellow] for zone [isovalue - deviation,isovalue]
	if (colour_mode_depth)
	{
		colour_volume = vec4(
			int(crossing_level_front_mod==1),
			(ray_sample_depth_radius - render_min_max_depth_radius_restriction.x) /
				(render_min_max_depth_radius_restriction.y - render_min_max_depth_radius_restriction.x),
			int(crossing_level_front_mod==2),
			1);
	
		return colour_volume;
	}
	
	// Isovalue colour mapping
	// Mapping according to colourmap.
	if (colour_mode_isovalue)
	{
		colour_volume = look_up_table_1D(
				colour_palette_sampler, colour_palette_resolution,
				min_max_colour_mapping_range.x, min_max_colour_mapping_range.y,
				field_scalar);
	
		return colour_volume;
	}
	
	return colour_volume;
}

// Function for returning the colour within window zones at surface of outer sphere
vec4
get_surface_entry_colour(
		float field_scalar,
		vec3 field_gradient,
		float iso_value,
		float deviation)
{
	vec4 colour_surface;
			
	// Gradient strength colour-mapping
	if (colour_mode_gradient)
	{
		int colour_sign = 2 * int( field_scalar < iso_value ) - 1;
		
		// 'colour_sign == 1': sampling point in region with gradient pointing towards isosurface
		// Map ||gradient|| to colour.
		//
		// 'colour_sign == -1': sampling point in region with gradient pointing away from isosurface
		// Map -||gradient|| to colour.
		
		colour_surface = look_up_table_1D(
				colour_palette_sampler, colour_palette_resolution,
				min_max_colour_mapping_range.x, min_max_colour_mapping_range.y,
				colour_sign * length(field_gradient));

		return colour_surface;
	}
	
	// Depth to colour mapping
	// Mapping: (field_scalar-iso_value)/deviation -> [-1,1] -> [yellow -> green -> cyan]
	if (colour_mode_depth)
	{
		float deviation_relative = (field_scalar-iso_value)/deviation;

		if (deviation_relative < 0)
		{
			colour_surface = vec4(-deviation_relative,1,0,1);
		}
		else
		{
			colour_surface = vec4(0,1,deviation_relative,1);
		}
		
		return colour_surface;
	}

	// Isovalue colour mapping
	// Mapping according to colourmap.
	if (colour_mode_isovalue)
	{
		colour_surface = look_up_table_1D(
				colour_palette_sampler, colour_palette_resolution,
				min_max_colour_mapping_range.x, min_max_colour_mapping_range.y,
				field_scalar);

		return colour_surface;
	}

	return colour_surface;
}

// Function for returning the colour within window zones at extruded polygon walls
vec4
get_wall_entry_colour(
		float field_scalar,
		vec3 field_gradient,
		float iso_value,
		float deviation,
		float depth_relative)
{
	vec4 colour_wall;
			
	// Gradient strength colour-mapping
	if (colour_mode_gradient)
	{
		int colour_sign = 2 * int( field_scalar < iso_value ) - 1;
		
		// 'colour_sign == 1': sampling point in region with gradient pointing towards isosurface
		// Map ||gradient|| to colour.
		//
		// 'colour_sign == -1': sampling point in region with gradient pointing away from isosurface
		// Map -||gradient|| to colour.
		
		colour_wall = look_up_table_1D(
				colour_palette_sampler, colour_palette_resolution,
				min_max_colour_mapping_range.x, min_max_colour_mapping_range.y,
				colour_sign * length(field_gradient));

		return colour_wall;
	}
	
	// Depth to colour mapping
	// Mapping: (field_scalar-iso_value)/deviation -> [-1,1] -> COLOUR_RANGE
	// Mapping COLOUR_RANGE: depth_relative -> [0,1] -> [[red -> green -> blue],[yellow -> green -> cyan]]
	if (colour_mode_depth)
	{
		float deviation_relative = (field_scalar-iso_value)/deviation;

		float colour_index = 0.5*(deviation_relative+1)*(1-depth_relative*0.5)+depth_relative*0.25;

		if (colour_index <= 0.5)
		{
			if (colour_index <= 0.25)
			{
				colour_wall = vec4(1,colour_index*4,0,1);
			}
			else
			{
				colour_wall = vec4(2-colour_index*4,1,0,1);
			}
		}
		else
		{
			if (colour_index <= 0.75)
			{
				colour_wall = vec4(0,1,colour_index*4-2,1);
			}
			else
			{
				colour_wall = vec4(0,4-colour_index*4,1,1);
			}
		}

		return colour_wall;
	}

	// Isovalue colour mapping
	// Mapping according to colourmap.
	if (colour_mode_isovalue)
	{
		colour_wall = look_up_table_1D(colour_palette_sampler,
							colour_palette_resolution,
							min_max_colour_mapping_range.x, // input_lower_bound
							min_max_colour_mapping_range.y, // input_upper_bound
							field_scalar);
	
		return colour_wall;
	}
	
	return colour_wall;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHANGED: try to discard empty space, but generates visible artefacts 
// (minimal sampling rate to get good results: 32 samplings with 4 trace_steps)
// check, if there's a isosurface for given isovalue along casted ray
bool
quick_test_if_ray_intersects_isosurface(
		Ray ray,
		// number of trace steps along the ray for every ray section...
		int trace_steps,
		vec2 lambda_min_max)
{
	// |-x-x-x-x|-x-x-x-x|-x-x-x-x|-x-x-x-x|-x-x-x-x|-x-x-x-x|-x-x-x-x|-x-x-x-x|---> ray
	// | | = division section of ray, here total number of eight section
	// x = one step through division section

	int cube_face_index_current;
	vec2 cube_face_coordinate_uv_current;
	float field_scalar_current = 0;
	vec3 field_gradient_current = vec3(0);
	get_scalar_field_data_from_position(
			at(ray, lambda_min_max.x), 
			tile_meta_data_sampler, tile_meta_data_resolution,
			mask_data_sampler, tile_resolution, field_data_sampler,
			depth_radius_to_layer_sampler, depth_radius_to_layer_resolution,
			num_depth_layers, min_max_depth_radius,
			cube_face_index_current, cube_face_coordinate_uv_current,
			field_scalar_current, field_gradient_current);
	
	int crossing_level_front = get_crossing_level(field_scalar_current);

	// number of sections the ray gets divided by
	float lambda_div_num = 8;
	// magic lambda values according to ray's section they belong to
	float lambdas[8];
	// length of ray from minimum to maximum
	float lambda_length = abs(lambda_min_max.y - lambda_min_max.x);
	// length of one section of the ray
	float lambda_division_size = lambda_length / lambda_div_num;

	// calculate lambdas
	lambdas[0] = lambda_min_max.x + lambda_division_size;
	lambdas[1] = lambda_min_max.x + 7.0f * lambda_division_size;
	lambdas[2] = lambda_min_max.x + 2.0f * lambda_division_size;
	lambdas[3] = lambda_min_max.x + 6.0f * lambda_division_size;
	lambdas[4] = lambda_min_max.x + 3.0f * lambda_division_size;
	lambdas[5] = lambda_min_max.x + 5.0f * lambda_division_size;
	lambdas[6] = lambda_min_max.x + 4.0f * lambda_division_size;
	lambdas[7] = lambda_min_max.y;

	// size of one step through one divided part of the ray
	float div_space = lambda_division_size / float(trace_steps);

	for (int i = 0; i < trace_steps + 1; ++i)
	{
		for (int j = 0; j < int(lambda_div_num); ++j)
		{
			// get sampled ray position from magic lambda
			vec3 ray_sample_position_current = at(ray,lambdas[j]);

			// Sample the field at the current ray position.
			// If the field sample is valid then look for a level crossing.
			if (get_scalar_field_data_from_position(
					ray_sample_position_current, 
					tile_meta_data_sampler, tile_meta_data_resolution,
					mask_data_sampler, tile_resolution, field_data_sampler,
					depth_radius_to_layer_sampler, depth_radius_to_layer_resolution,
					num_depth_layers, min_max_depth_radius,
					cube_face_index_current, cube_face_coordinate_uv_current,
					field_scalar_current, field_gradient_current))
			{
				if (crossing_level_front != get_crossing_level(field_scalar_current))
				{
					// an isosurface was found
					return true;
				}
			}

			// decrement the current magic lambda
			lambdas[j] -= div_space;
		}
	}
	
	// no isosurface found
	return false;
}

// Renders the volume fill wall if ray hits wall.
//
// Returns false if ray does not need tracing which happens when the ray starts outside
// the active surface fill region (and hence either misses a wall or hits an opaque wall).
bool
render_volume_fill_walls(
		Ray ray,
		float lambda_outer_sphere_entry_point,
		inout vec4 colour_background,
		inout vec3 iso_surface_position,
		inout vec2 lambda_min_max)
{
	// The position along the ray where it enters the outer sphere.
	vec3 ray_outer_sphere_entry_point = at(ray, lambda_outer_sphere_entry_point);
	
	int cube_face_index_ray_entry;
	vec2 cube_face_coordinate_uv_ray_entry;
	calculate_cube_face_coordinates(
			ray_outer_sphere_entry_point, cube_face_index_ray_entry, cube_face_coordinate_uv_ray_entry);
	
	// Is ray entry point (in outer sphere) in an active *surface fill* mask zone.
	//
	// NOTE: We use the dilated version of the sampling function which averages 3x3 (bilinearly filtered) samples.
	// This emulates a pre-processing dilation of the surface fill mask giving it an effective 1.5 texel dilation.
	// This avoids artifacts where the surface fill mask (at surface) meets the top of the volume fill walls.
	// These artifacts are due to discarded rays by the wall depth range code because the ray slips between the cracks
	// and hits neither the wall nor the surface fill mask.
	bool active_surface_fill_mask_at_ray_entry_point = projects_into_dilated_surface_fill_mask(
			surface_fill_mask_sampler, surface_fill_mask_resolution,
			cube_face_index_ray_entry, cube_face_coordinate_uv_ray_entry);
	
	// Access the wall normal/depth (if present for the current screen pixel).
	vec4 wall_sample = texture2D(volume_fill_wall_surface_normal_and_depth_sampler, 0.5 * screen_coord + 0.5);
	float screen_space_wall_depth = wall_sample.a;
	vec3 wall_normal = wall_sample.rgb;

	// The screen-space depth range is [-1, 1] and a value of 1 (far plane) indicates there's no wall.
	bool ray_intersects_wall = (screen_space_wall_depth < 1.0);
	if (!ray_intersects_wall)
	{
		// No intersection with wall, but if ray starts outside an active masking (polygon) zone then
		// it cannot enter an active zone as all walls are fully opaque - in this case we'll return false
		// and ray casting will not be performed; background colour will be used as the final raycasting colour.
		return active_surface_fill_mask_at_ray_entry_point;
	}
	
	vec4 colour_wall = vec4(1,1,1,1);
	// Note: Seems gl_ModelViewProjectionMatrixInverse does not always work on Mac OS X.
	float lambda_wall = convert_screen_space_depth_to_ray_lambda(
			screen_space_wall_depth, screen_coord, gl_ModelViewMatrixInverse * gl_ProjectionMatrixInverse, ray.origin);
	vec3 ray_sample_position_wall = at(ray, lambda_wall);

	int cube_face_index_wall;
	vec2 cube_face_coordinate_uv_wall;
	calculate_cube_face_coordinates(ray_sample_position_wall, cube_face_index_wall, cube_face_coordinate_uv_wall);
	
	// tile offset indices
	vec2 tile_offset_uv_wall;
	// tile uv coordinates
	vec2 tile_coordinate_uv_wall;
	// tile id number per cube face
	float tile_ID_wall;
	// min and max scalar value boundaries
	vec2 tile_meta_data_min_max_scalar_wall;
	get_tile_meta_data_from_cube_face_coordinates(
			cube_face_index_wall, cube_face_coordinate_uv_wall,
			tile_meta_data_sampler, tile_meta_data_resolution, tile_resolution,
			tile_offset_uv_wall, tile_coordinate_uv_wall,
			tile_ID_wall, tile_meta_data_min_max_scalar_wall);

	// If the wall position has valid field data then
	// colour the wall in the deviation window regions.
	if (is_valid_field_data_sample(mask_data_sampler, tile_coordinate_uv_wall, tile_ID_wall))
	{
		// Extraction of wall colour at ray intersection point

		// Sample the field data.
		float field_scalar_wall;
		vec3 field_gradient_wall;
		float ray_sample_depth_radius_wall = length(ray_sample_position_wall);
		get_scalar_field_data_from_cube_face_coordinates(
				ray_sample_depth_radius_wall, cube_face_index_wall, cube_face_coordinate_uv_wall,
				tile_offset_uv_wall, tile_coordinate_uv_wall, tile_ID_wall,
				field_data_sampler, depth_radius_to_layer_sampler,
				depth_radius_to_layer_resolution, num_depth_layers, min_max_depth_radius,
				field_scalar_wall, field_gradient_wall);
		
		int crossing_level_wall = get_crossing_level(field_scalar_wall);

		// Relative depth: 0 -> core, 1 -> surface (for restrictd max/min depth range)
		float depth_relative_wall = (ray_sample_depth_radius_wall - render_min_max_depth_radius_restriction.x) /
				(render_min_max_depth_radius_restriction.y - render_min_max_depth_radius_restriction.x);
		// isoline frequency pattern
		float isoline_frequency = 0;

		// condition, what isovalue (index) has to be used 
		bool is_isovalue_one = (crossing_level_wall < 3);

		// indicates if ray sample position is behind or in front of isosurfaces
		bool not_in_crossing_zone = (int(mod(crossing_level_wall,3)) == 0);

		// isovalues needed for isoline frequency calculation
		// x = corresponding isovalue x
		// y = corresponding isovalue y/z
		vec2 isovalue_wall;

		if (is_isovalue_one)
		{
			isovalue_wall.x = isovalue1.x;
			isovalue_wall.y = mix(isovalue1.x - isovalue1.y, isovalue1.z - isovalue1.x, float(crossing_level_wall - 1) );
		}
		else
		{
			isovalue_wall.x = isovalue2.x;
			isovalue_wall.y = mix(isovalue2.x - isovalue2.y, isovalue2.z - isovalue2.x, float(crossing_level_wall - 4) );
		}

		isoline_frequency = 
			(surface_deviation_isoline_frequency * 4 + 1) *
				abs(field_scalar_wall - isovalue_wall.x + isovalue_wall.y) / isovalue_wall.y;
		isoline_frequency = floor(isoline_frequency * 0.5f);

		// Colour for window zone
		if (int(mod(isoline_frequency,2)) == 0 && !not_in_crossing_zone)
		{
			vec4 wall_entry_colour = get_wall_entry_colour(
					field_scalar_wall, field_gradient_wall,
					isovalue_wall.x, isovalue_wall.y, depth_relative_wall);
			
			// Reject transparent colours - we want the wall to be opaque since
			// it allows for performance optimisations - also if the wall was transparent
			// in places then we'd need to draw the wall behind it but we don't have its
			// depth position recorded in the wall (normal,depth) screen texture.
			if (wall_entry_colour.a == 1.0)
			{
				// Note that when alpha is 1.0 the RBG channels do not need to be pre-multiplied with alpha.
				colour_wall = wall_entry_colour;
			}
		}
	}

	// diffuse shading
	if (lighting_enabled)
	{
		float lambert = lambert_diffuse_lighting(world_space_light_direction, wall_normal);

		float diffuse = mix(lambert, 1.0, light_ambient_contribution);

		colour_wall.rgb *= diffuse;
	}

	if (lambda_wall < lambda_min_max.y)
	{
		// Wall crossing happens before inner sphere crossing -> wall colour is returned
		colour_background = colour_wall;
		iso_surface_position = at(ray, lambda_wall);
		
		// Ray will be terminated at wall intersection point.
		lambda_min_max.y = lambda_wall;
	}
	
	// Perform ray-casting if ray enters from an active surface fill masking zone (covered by polygon).
	// In this case the background colour will be blended after the raycasting colour.
	//
	// Else if ray starts outside an active surface fill masking (polygon) zone so it cannot enter an active zone
	// as all walls are fully opaque - in this case we'll return false and ray casting will not be performed;
	// background colour will be used as the final raycasting colour.
	return active_surface_fill_mask_at_ray_entry_point;
}


// Reduces the ray traversal (lambda min/max) range due to the surface fill mask (polygons).
//
// Returns false if ray does not need tracing which happens when the ray starts outside
// the active surface fill region and misses a vertically extruded surface fill wall
// (note that the walls are not visible and hence the isosurface is visible through them.
//
// This is an optimisation that enables us to reduce the length along each ray that is sampled/traversed.
// We don't need this if the walls are going to be drawn because there are already good
// optimisations in place to limit ray sampling based on the fact that the walls are opaque.
bool
reduce_depth_range_to_volume_fill_walls(
		Ray ray,
		float lambda_outer_sphere_entry_point,
		float lambda_outer_sphere_exit_point,
		inout vec2 lambda_min_max)
{
	// The position along the ray where it enters/exits the outer sphere.
	vec3 ray_outer_sphere_entry_point = at(ray, lambda_outer_sphere_entry_point);
	vec3 ray_outer_sphere_exit_point = at(ray, lambda_outer_sphere_exit_point);
	
	int cube_face_index_ray_entry;
	int cube_face_index_ray_exit;
	vec2 cube_face_coordinate_uv_ray_entry;
	vec2 cube_face_coordinate_uv_ray_exit;
	calculate_cube_face_coordinates(
			ray_outer_sphere_entry_point, cube_face_index_ray_entry, cube_face_coordinate_uv_ray_entry);
	calculate_cube_face_coordinates(
			ray_outer_sphere_exit_point, cube_face_index_ray_exit, cube_face_coordinate_uv_ray_exit);
	
	// Is ray entry/exit point (in outer sphere) in an active *surface fill* mask zone.
	//
	// NOTE: We use the dilated version of the sampling function which averages 3x3 (bilinearly filtered) samples.
	// This emulates a pre-processing dilation of the surface fill mask giving it an effective 1.5 texel dilation.
	// This avoids artifacts where the surface fill mask (at surface) meets the top of the volume fill walls.
	// These artifacts are due to discarded rays by the wall depth range code because the ray slips between the cracks
	// and hits neither the wall nor the surface fill mask.
	bool active_surface_fill_mask_at_ray_entry_point = projects_into_dilated_surface_fill_mask(
			surface_fill_mask_sampler, surface_fill_mask_resolution,
			cube_face_index_ray_entry, cube_face_coordinate_uv_ray_entry);
	bool active_surface_fill_mask_at_ray_exit_point = projects_into_dilated_surface_fill_mask(
			surface_fill_mask_sampler, surface_fill_mask_resolution,
			cube_face_index_ray_exit, cube_face_coordinate_uv_ray_exit);
	
	// If the ray both enters and exits an active zone then we should not limit its depth range.
	// Otherwise we can limit its depth range and even discard the current ray.
	if (active_surface_fill_mask_at_ray_entry_point && active_surface_fill_mask_at_ray_exit_point)
	{
		// Need to perform ray-casting.
		return true;
	}
	
	// Access the wall min/max depth range (if present for the current screen pixel).
	vec4 wall_or_inner_sphere_sample = texture2D(volume_fill_wall_depth_range_sampler, 0.5 * screen_coord + 0.5);
	
	// If the red (or green) channel is 2.0 then it means a wall (or inner sphere) was not rendered at that pixel.
	// So anything less than 2.0 (more specifically the screen-space depth range is [-1, 1]) will have a depth range.
	bool ray_has_depth_range = (wall_or_inner_sphere_sample.r < 1.5);
	if (ray_has_depth_range)
	{
		// The blue and alpha channels contain the min and max depths respectively.
		float min_depth = wall_or_inner_sphere_sample.b;
		float max_depth = wall_or_inner_sphere_sample.a;
		
		// If ray does not enter an active zone then we can skip the first part of the ray.
		if (!active_surface_fill_mask_at_ray_entry_point)
		{
			// Note: Seems gl_ModelViewProjectionMatrixInverse does not always work on Mac OS X.
			float lambda_min_depth = convert_screen_space_depth_to_ray_lambda(
					min_depth, screen_coord, gl_ModelViewMatrixInverse * gl_ProjectionMatrixInverse, ray.origin);
			lambda_min_max.x = max(lambda_min_max.x, lambda_min_depth);
		}
		
		// If ray does not exit an active zone then we can skip the last part of the ray.
		if (!active_surface_fill_mask_at_ray_exit_point)
		{
			// Note: Seems gl_ModelViewProjectionMatrixInverse does not always work on Mac OS X.
			float lambda_max_depth = convert_screen_space_depth_to_ray_lambda(
					max_depth, screen_coord, gl_ModelViewMatrixInverse * gl_ProjectionMatrixInverse, ray.origin);
			lambda_min_max.y = min(lambda_min_max.y, lambda_max_depth);
		}
	}
	else // ray does not intersect walls or inner sphere ...
	{
		// The ray misses walls and inner sphere,
		// so if it also does not enter or exit any active zones then we can simply discard the ray.
		if (!active_surface_fill_mask_at_ray_entry_point &&
			!active_surface_fill_mask_at_ray_exit_point)
		{
			return false;
		}
	}
	
	// Need to perform ray-casting.
	return true;
}

// Renders the surface deviation window (at the outer sphere surface).
void
render_surface_deviation_window(
		Ray ray,
		float lambda_outer_sphere_entry_point,
		inout vec4 colour)
{
	// The position along the ray where it enters the outer sphere.
	vec3 ray_outer_sphere_entry_point = at(ray, lambda_outer_sphere_entry_point);
	
	int cube_face_index_ray_entry;
	vec2 cube_face_coordinate_uv_ray_entry;
	calculate_cube_face_coordinates(
			ray_outer_sphere_entry_point, cube_face_index_ray_entry, cube_face_coordinate_uv_ray_entry);
	
	vec2 tile_offset_uv_ray_entry;
	vec2 tile_coordinate_uv_ray_entry;
	float tile_ID_ray_entry;
	vec2 tile_meta_data_min_max_scalar_ray_entry;
	get_tile_meta_data_from_cube_face_coordinates(
			cube_face_index_ray_entry, cube_face_coordinate_uv_ray_entry,
			tile_meta_data_sampler, tile_meta_data_resolution, tile_resolution,
			tile_offset_uv_ray_entry, tile_coordinate_uv_ray_entry,
			tile_ID_ray_entry, tile_meta_data_min_max_scalar_ray_entry);

	// If the ray enters a region of the sphere that contains no valid field data
	// then we don't draw the surface deviation window.
	if (!is_valid_field_data_sample(mask_data_sampler, tile_coordinate_uv_ray_entry, tile_ID_ray_entry))
	{
		return;
	}
	
	// If the ray entry point is outside an active *surface fill* mask zone
	// then we don't draw the surface deviation window.
	if (using_surface_fill_mask)
	{
		if (!projects_into_surface_fill_mask(
				surface_fill_mask_sampler, surface_fill_mask_resolution,
				cube_face_index_ray_entry, cube_face_coordinate_uv_ray_entry))
		{
			return;
		}
	}
	
	float field_scalar_ray_entry;
	vec3 field_gradient_ray_entry;
	get_scalar_field_data_from_cube_face_coordinates(
			length(ray_outer_sphere_entry_point), cube_face_index_ray_entry, cube_face_coordinate_uv_ray_entry,
			tile_offset_uv_ray_entry, tile_coordinate_uv_ray_entry, tile_ID_ray_entry, 
			field_data_sampler, depth_radius_to_layer_sampler, depth_radius_to_layer_resolution,
			num_depth_layers, min_max_depth_radius,
			field_scalar_ray_entry, field_gradient_ray_entry);
	
	int crossing_level_ray_entry = get_crossing_level(field_scalar_ray_entry);

	// Special view dependent opacity correction.
	vec3 normal_ray_entry = normalize(ray_outer_sphere_entry_point);
	float alpha = 1 - exp(log(1 - opacity_deviation_surfaces) / abs(dot(normal_ray_entry, ray.direction)));
	float isoline = 0;

	bool is_isovalue_one = (crossing_level_ray_entry < 3);
	bool not_in_crossing_zone = (int(mod(crossing_level_ray_entry,3)) == 0);

	// isovalues needed for isoline frequency calculation
	// x = corresponding isovalue x
	// y = corresponding isovalue y/z
	vec2 isovalue_ray_entry;

	if (is_isovalue_one)
	{
		isovalue_ray_entry.x = isovalue1.x;
		isovalue_ray_entry.y = mix(isovalue1.x - isovalue1.y, isovalue1.z - isovalue1.x, float(crossing_level_ray_entry - 1) );
	}
	else
	{
		isovalue_ray_entry.x = isovalue2.x;
		isovalue_ray_entry.y = mix( isovalue2.x - isovalue2.y, isovalue2.z - isovalue2.x, float(crossing_level_ray_entry - 4) );
	}

	isoline = (surface_deviation_isoline_frequency*4.0f+1.0f) *
			abs(field_scalar_ray_entry - isovalue_ray_entry.x + isovalue_ray_entry.y) / isovalue_ray_entry.y;
	isoline = floor(0.5f * isoline);

	if (not_in_crossing_zone)
	{
		return;
	}
	
	vec4 colour_surface = get_surface_entry_colour(
			field_scalar_ray_entry, field_gradient_ray_entry,
			isovalue_ray_entry.x, isovalue_ray_entry.y);
	
	// Differentiation between coloured and white stripes (stripe containing isocontour is always coloured!)
	if (int(mod(isoline,2)) == 0)
	{
		// Factor in 'colour_surface.a' in case colour is transparent.
		colour = vec4(colour_surface.rgb, colour_surface.a * alpha);
	}
	else
	{
		// Factor in 'colour_surface.a' in case colour is transparent.
		colour = vec4(1, 1, 1, colour_surface.a * alpha);
	}
	
	// Keep RGB pre-multiplied by alpha.
	colour.rgb *= colour.a;
}

// Move the current sample location (lambda, ray_sample_position) to the point at which
// the ray exits the tile.
//
// If the ray does not exit the tile (eg, intersects inner sphere or outer sphere or
// opaque wall) then the current sample location is set to the ray exit point.
void
sample_ray_at_tile_exit(
		Ray ray,
		int cube_face_index,
		vec2 tile_offset_uv,
		mat3 cube_face_matrix_edge[6],
		vec2 lambda_min_max,
		inout float lambda,
		inout vec3 ray_sample_position,
		inout vec4 colour)
{
	// the four plane normals of the tile
	vec3 plane_normals[4];
	// lambda_found
	bool lambda_found = false;
	// the current maxmimal calculated lambda
	float lambda_found_max = lambda_min_max.y;
	
	// create the four tile planes for given face and tile offsets
	create_plane_normals_from_vectors(
			cube_face_matrix_edge[cube_face_index],
			tile_meta_data_resolution, tile_offset_uv, plane_normals);

	// calculate lambda for every four planes
	for (int i = 0; i < 4; i++)
	{
		// calculated intersection lambda
		float lambda_intersect = get_lambda_from_plane(ray,plane_normals[i]);

		// if lambda is greater than or equal the earlier founded maximal lambda
		if (lambda_intersect >= lambda && lambda_intersect <= lambda_found_max)
		{
			// calculated intersection point
			vec3 pos_intersect = at(ray,lambda_intersect);
			
			float tolerance = 0.00001f;
			// test, if calculated intersection point is within planes
			if (is_point_within_tile_planes(pos_intersect, plane_normals, tolerance))
			{
				lambda_found_max = lambda = lambda_intersect;
				ray_sample_position = at(ray,lambda);

				lambda_found = true;
			}
		}
	}
	

	// if no lambda was found, set lambda to maximum
	if (!lambda_found)
	{
		lambda = lambda_min_max.y;
		ray_sample_position = at(ray,lambda);
		// for debugging
		//if (test_variable_1 > 0.02) colour = vec4(0,length(ray_sample_position),0,1);
	}

	// for visualisation of tile intersection points
	if (test_variable_3 > 0) 
	{
		colour.rgb += vec3(length(ray_sample_position),0,length(ray_sample_position));
		colour.a += (1 - colour.a) * 1;
	}
}

// Blends the ray sample using volume rendering if it's within a deviation window.
void
blend_deviation_window_volume_sample(
		float ray_sample_depth_radius,
		int crossing_level_front,
		float field_scalar,
		vec3 field_gradient,
		inout vec4 colour)
{
	int crossing_level_front_mod = int(mod(crossing_level_front,3));

	// Only draw in deviation window around isovalue1 (level 1 and 2) and isovalue2 (level 4 and 5).
	if ((crossing_level_front_mod >= 1) && (crossing_level_front_mod <= 2))
	{
		vec4 colour_volume = get_volume_colour(
				ray_sample_depth_radius, crossing_level_front_mod, field_scalar, field_gradient);

		// Factor in 'colour_volume.a' in case colour is transparent.
		colour.rgb += (1-colour.a) * colour_volume.rgb * colour_volume.a * opacity_deviation_window_volume_rendering;
		colour.a += (1-colour.a) * colour_volume.a * opacity_deviation_window_volume_rendering;	
	}
}

// Blend the isosurface ray sample if the isosurface is in an active region.
void
blend_isosurface_sample(
		Ray ray,
		inout vec3 ray_sample_position,
		inout int crossing_level_back,
		inout int crossing_level_front,
		inout vec3 field_gradient,
		vec4 colour_matrix_iso[6],
		inout vec3 iso_surface_position,
		inout vec4 colour)
{
	// update position of detected isosurface
	iso_surface_position = ray_sample_position;

	vec3 normal = (1 - 2 * int(dot(field_gradient, ray.direction) >= 0)) * normalize(field_gradient);

	bool positive_crossing = (crossing_level_front >= crossing_level_back);

	// get final colour of one or final blended colour of multiple surfaces
	vec4 colour_crossing = get_blended_crossing_colour(
			length(ray_sample_position), ray, normal, crossing_level_back, crossing_level_front,
			positive_crossing, colour_matrix_iso, field_gradient);

	// Note: We don't multiply 'colour_crossing.rgb' with 'colour_crossing.a' since
	// colour_crossing has already been pre-multiplied with alpha.
	colour.rgb += (1 - colour.a) * colour_crossing.rgb;
	colour.a += (1 - colour.a) * colour_crossing.a;
}

// Raycasting process
bool
raycasting(
		out vec4 colour,
		out vec3 iso_surface_position)
{
	// background colour
	vec4 background_colour = vec4(0,0,0,0);
	// lambda min max limits
	vec2 lambda_min_max = vec2(0,0);

	// final color of isosurface
	colour = vec4(0,0,0,0);
	// final position of our isosurface
	iso_surface_position = vec3(0,0,0);

	// Colour matrix contains colour vectors for iso surfaces (including deviation window surfaces)
	// using isovalue colour mapping.
	vec4 colour_matrix_iso[6];

	// colour of front deviation window (-) in relation to isovalue1
	colour_matrix_iso[0] = look_up_table_1D(
								colour_palette_sampler, colour_palette_resolution,
								min_max_colour_mapping_range.x, min_max_colour_mapping_range.y,
								isovalue1.y); 

	// colour of isosurface isovalue1
	colour_matrix_iso[1] = look_up_table_1D(
								colour_palette_sampler, colour_palette_resolution,
								min_max_colour_mapping_range.x, min_max_colour_mapping_range.y,
								isovalue1.x); 

	// colour of back deviation window (+) in relation to isovalue1
	colour_matrix_iso[2] = look_up_table_1D(
								colour_palette_sampler, colour_palette_resolution,
								min_max_colour_mapping_range.x, min_max_colour_mapping_range.y,
								isovalue1.z);

	// colour of front deviation window (-) in relation to isovalue2
	colour_matrix_iso[3] = look_up_table_1D(
								colour_palette_sampler, colour_palette_resolution,
								min_max_colour_mapping_range.x, min_max_colour_mapping_range.y,
								isovalue2.y); 

	// colour of isosurface isovalue2
	colour_matrix_iso[4] = look_up_table_1D(
								colour_palette_sampler, colour_palette_resolution,
								min_max_colour_mapping_range.x, min_max_colour_mapping_range.y,
								isovalue2.x); 

	// colour of back deviation window (+) in relation to isovalue2
	colour_matrix_iso[5] = look_up_table_1D(
								colour_palette_sampler, colour_palette_resolution,
								min_max_colour_mapping_range.x, min_max_colour_mapping_range.y,
								isovalue2.z);
	
	// vector position matrix containing 1 edge (0) and two directions (x,y) of a certain cube face
	// [cube face index][edge number]
	// edge number:
	// 0 = upper left edge position with tile uv coordinate (0,0)
	// 1 = vector pointing at tile x direction (to 1,0)
	// 2 = vector pointing at tile y direction (to 0,1)
	mat3 cube_face_matrix_edge[6];

	// contains all eight cube edges
	vec3 cube_vertices[8];
	cube_vertices[0] = vec3( 1, 1, 1);
	cube_vertices[1] = vec3( 1, 1,-1);
	cube_vertices[2] = vec3( 1,-1, 1);
	cube_vertices[3] = vec3( 1,-1,-1);
	cube_vertices[4] = vec3(-1, 1, 1);
	cube_vertices[5] = vec3(-1, 1,-1);
	cube_vertices[6] = vec3(-1,-1, 1);
	cube_vertices[7] = vec3(-1,-1,-1);

	// contains all indices for initializing cube face matrix
	vec3 cube_face_indices[6];
	cube_face_indices[0] = vec3(0,1,2);
	cube_face_indices[1] = vec3(5,4,7);
	cube_face_indices[2] = vec3(5,1,4);
	cube_face_indices[3] = vec3(6,2,7);
	cube_face_indices[4] = vec3(4,0,6);
	cube_face_indices[5] = vec3(1,5,3);

	for (int i = 0; i < 6; ++i)
	{
		// left upper vertex of cube face
		cube_face_matrix_edge[i][0] = cube_vertices[int(cube_face_indices[i].x)];
		// x direction vector of face
		cube_face_matrix_edge[i][1] = cube_vertices[int(cube_face_indices[i].y)] - cube_face_matrix_edge[i][0];
		// y direction vector of face
		cube_face_matrix_edge[i][2] = cube_vertices[int(cube_face_indices[i].z)] - cube_face_matrix_edge[i][0];
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Reject ray if it's occluded by surface geometries.

	if (read_from_surface_occlusion_texture)
	{
		// If a surface object on the front of the globe has an alpha value of 1.0 (and hence occludes
		// the current ray) then discard the current pixel.
		// Convert [-1,1] range to [0,1] for texture coordinates.
		float surface_occlusion = texture2D(surface_occlusion_texture_sampler, 0.5 * screen_coord + 0.5).a;
		
		if (surface_occlusion == 1.0)
		{
			return false;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Initialize ray

	// Create the ray starting at the ray origin and moving into direction through near plane.
	//
	// Normally the ray origin would be the eye/camera position, but for orthographic viewing the real eye position is at
	// infinity and generates parallel rays for all pixels. Note that we also support perspective viewing, so it would be
	// nice to calculate a ray origin that works for both orthographic and perspective viewing. Turns out it doesn't really
	// matter where, along the ray, the ray origin is. As long as the ray line is correct. So the ray origin doesn't need to
	// be the eye/camera position. The viewing transform is encoded in the model-view-projection transform so using that
	// provides a way to support both orthographic and perspective viewing with the same code.
	//
	// The inverse model-view-projection transform is used to convert the current screen coordinate (x,y,-2) to
	// world space for the ray origin and (x,y,0) to world space to calculate (normalised) ray direction.
	// The screen-space depth of -2 is outside the view frustum [-1,1] and in front of the near clip plane.
	// The screen-space depth of 0 is inside the view frustum [-1,1] between near and far clip planes.
	// Both values are somewhat arbitrary actually (since we don't really care where the ray origin is along the ray line).
	//
	// Note: Seems gl_ModelViewProjectionMatrixInverse does not always work on Mac OS X.
	Ray ray = get_ray(screen_coord, gl_ModelViewMatrixInverse * gl_ProjectionMatrixInverse);
	
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Determine lambda min and max interval using inner and outer spheres

	// declare outer_sphere radius, maximum given in "render_min_max_depth_radius_restriction".y component
	Sphere outer_sphere = Sphere(render_min_max_depth_radius_restriction.y * render_min_max_depth_radius_restriction.y);
	Interval outer_sphere_interval;

	// check for intersection points (at which point does the ray enter the sphere and at which does it leave the sphere)
	if (!intersect(outer_sphere,ray,outer_sphere_interval))
	{
		// if there's no intersection point, discard the pixel
		return false;
	}
	
	lambda_min_max.x = outer_sphere_interval.from;
	lambda_min_max.y = outer_sphere_interval.to;

	Sphere inner_sphere = Sphere(render_min_max_depth_radius_restriction.x * render_min_max_depth_radius_restriction.x);
	Interval inner_sphere_interval;

	if (intersect(inner_sphere,ray,inner_sphere_interval))
	{
		background_colour = vec4(1,1,1,1);

		// set maximum to inner sphere entry point
		lambda_min_max.y = min(inner_sphere_interval.from,lambda_min_max.y);
		iso_surface_position = at(ray,lambda_min_max.y);

		if (lighting_enabled)
		{
			// lambert diffuse shading
			vec3 normal = normalize(at(ray,lambda_min_max.y));
			float lambert = lambert_diffuse_lighting(world_space_light_direction,normal);
			float diffuse = mix(lambert,1.0,light_ambient_contribution);

			background_colour.rgb *= diffuse;
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Reject ray if it does not intersect isosurface(s).
	
	if (test_variable_4 > 0)
	{
		// number of trace steps along the ray for every ray section
		int trace_steps = int(test_variable_4 * 100.0f / 2.0f); // 0.02 -> 8 Samplings | 0.04 -> 16 Samplings ...
		
		// if no iso surface was found during algorithm, no further sampling has to be done
		if (!quick_test_if_ray_intersects_isosurface(ray, trace_steps, lambda_min_max))
		{
			if (test_variable_5 > 0)
				colour = vec4(1,0,0.5,1);
			
			return colour.a > 0;
		}
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Determine lambda increment
	
	// lambda incrementation factor
	float lambda_increment = sampling_rate.x;
	
	// In case min lambda somehow exceeded max lambda.
	lambda_min_max.y = max(lambda_min_max.x, lambda_min_max.y);
	if (lambda_min_max.y > lambda_min_max.x)
	{
		lambda_increment = (lambda_min_max.y - lambda_min_max.x) / ceil((lambda_min_max.y - lambda_min_max.x) / lambda_increment);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Reduce max lambda if have a depth texture
	//
	// NOTE: This is done after the lambda increment has been calculated.
	// Should the interval be reduced before calculating lambda increment?

	// Reduce max lambda if we have a depth texture.
	if (read_from_depth_texture)
	{
		// The depth texture might reduce the maximum ray length enabling early ray termination.
		// Convert [-1,1] range to [0,1] for texture coordinates.
		// Depth texture is a single-channel floating-point texture (GL_R32F).
		float depth_texture_screen_space_depth = texture2D(depth_texture_sampler, 0.5 * screen_coord + 0.5).r;
		// Note: Seems gl_ModelViewProjectionMatrixInverse does not always work on Mac OS X.
		float depth_texture_lambda = convert_screen_space_depth_to_ray_lambda(
				depth_texture_screen_space_depth, screen_coord, gl_ModelViewMatrixInverse * gl_ProjectionMatrixInverse, ray.origin);
		lambda_min_max.y = min(lambda_min_max.y, depth_texture_lambda);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Reduce min and max lambda if have the depth range of the volume fill walls
	//
	// NOTE: This is done after the lambda increment has been calculated.
	// Should the interval be reduced before calculating lambda increment?
	
	if (using_surface_fill_mask && using_volume_fill_wall_depth_range)
	{
		if (reduce_depth_range_to_volume_fill_walls(
				ray, outer_sphere_interval.from, outer_sphere_interval.to, lambda_min_max))
		{
			// Force separate else clause to avoid Nvidia GLSL compiler bug (tested driver version 320.18)
			// where 'lambda_min_max' is not copied back from function call.
		}
		else
		{
			// There is no need to ray-trace, so set the colour and return.
			colour = background_colour;
			
			// Note: 'iso_surface_position' should already have been set to correct position at inner sphere,
			// or the ray does not hit inner sphere in which case discarded (since colour.a == 0).
			return colour.a > 0;
		}
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Show extruded polygon walls.
	//
	// NOTE: This is done after the lambda increment has been calculated since max lambda can be reduced here.
	// Should the interval be reduced before calculating lambda increment?
	
	if (using_surface_fill_mask && show_volume_fill_walls)
	{
		if (!render_volume_fill_walls(
				ray, outer_sphere_interval.from, background_colour, iso_surface_position, lambda_min_max))
		{
			// There is no need to ray-trace, so set the colour and return.
			colour = background_colour;
			
			// Note: 'iso_surface_position' should already have been set to correct position at inner sphere,
			// or the ray does not hit inner sphere in which case discarded (since colour.a == 0).
			return colour.a > 0;
		}
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Render surface deviation window (isolines on outer sphere surface).
	
	if (surface_deviation_window)
	{
		render_surface_deviation_window(ray, outer_sphere_interval.from, colour);
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Initialize ray casting variables

	// current lambda value
	float lambda = lambda_min_max.x;
	// current ray position, set to first interesection point with outer sphere
	vec3 ray_sample_position = at(ray,lambda);
	// ray incrementation vector
	vec3 pos_vec_increment = ray.direction * lambda_increment;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Start ray casting
	
	// scalar value of sample point
	float field_scalar;
	// field gradient at given sample point
	vec3 field_gradient;

	// cube face index to which ray position points to
	int cube_face_index;
	// local uv cube face coordinates in range [0,1]
	vec2 cube_face_coordinate_uv;
	vec2 tile_offset_uv;
	vec2 tile_coordinate_uv;
	// CHANGED: contains min/max scalar value boundaries of given tile
	vec2 tile_meta_data_min_max_scalar;
	// tile id number per cube face
	float tile_ID;
	
	// previous tile that was skipped
	float prev_skipped_tile_ID = -1;
	// previous tile offset uv to test out of tile boundaries
	vec2 prev_tile_offset_uv = vec2(-1,-1);
	// previous cube face index
	int prev_cube_face_index = -1;

	// Whether the ray is currently sampling valid data.
	// For example, outside the field's lat/lon georeferenced region is invalid (no data).
	bool valid_front = false;
	// Whether the ray is currently sampling valid data *and* sample is within the surface fill mask.
	bool active_front = false;
	// level of ray crossing the isosurface(s) forwards  0 [ 1 | 2 ] - 3 - [ 4 | 5 ] - 6 - 
	int crossing_level_front = 0;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Start the ray casting by sampling the first point at min lambda

	// Sample the field at the initial ray sample position.
	valid_front = get_scalar_field_data_from_position(
			ray_sample_position, 
			tile_meta_data_sampler, tile_meta_data_resolution,
			mask_data_sampler, tile_resolution, field_data_sampler,
			depth_radius_to_layer_sampler, depth_radius_to_layer_resolution,
			num_depth_layers, min_max_depth_radius,
			cube_face_index, cube_face_coordinate_uv,
			field_scalar, field_gradient);
	active_front = valid_front;
	if (using_surface_fill_mask)
	{
		active_front = active_front && projects_into_surface_fill_mask(
				surface_fill_mask_sampler, surface_fill_mask_resolution,
				cube_face_index, cube_face_coordinate_uv);
	}
	if (valid_front)
	{
		crossing_level_front = get_crossing_level(field_scalar);
	}
	
	// Blending step during volume rendering for deviation window zone.
	if (deviation_window_volume_rendering && active_front)
	{
		blend_deviation_window_volume_sample(
				length(ray_sample_position), crossing_level_front,
				field_scalar, field_gradient, colour);
	}

	// go to the next ray sample point and increment current lambda
	ray_sample_position += pos_vec_increment;
	lambda += lambda_increment;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Ray casting loop
	
	// defines, if raycasting has reached last step, then active last crossing step
	bool last_crossing_active = false;
	// defines, if last crossing has to be done and avoids heavy artefacts at last ray position due to high precision errors
	bool last_crossing = false;
	
	float opaque_colour_threshold = 0.95;

	// break condition: if opacity is greater than a threshold or lambda exceeded max lambda restriction
	while (colour.a <= opaque_colour_threshold)
	{
		// This last crossing code avoids having two separate while loops which duplicate some common functionality:
		// One loop for all ray intervals except the last and one for the last interval (to avoid noise artifacts).
		if (!last_crossing_active)
		{
			// If raycasting exceeds maximum ray length, activate last crossing.
			//
			// Artefact correction. The last ray interval does not necessarily end at max lambda.
			// So to avoid pixel noise artefacts (such as where isosurface intersects inner sphere) this correction
			// re-computes the last interval with the exact max lambda value to overcome this problem.
			if (lambda >= lambda_min_max.y)
			{
				last_crossing_active = true;
				// Set to false in case we encounter a 'continue' statement before detecting a crossing.
				last_crossing = false;
				// Set ray sample position to the last ray position.
				lambda_increment = lambda_min_max.y - (lambda - lambda_increment);
				lambda = lambda_min_max.y;
				ray_sample_position = at(ray, lambda);
			}
		}
		else if (!last_crossing)
		{
			// No crossing was detected in the last ray interval - so nothing left to do.
			break;
		}
		else
		{
			// A crossing was detected in the last ray interval and now we are looking for another crossing.
			// Set ray sample position to the last ray position.
			lambda_increment = lambda_min_max.y - lambda;
			lambda = lambda_min_max.y;
			ray_sample_position = at(ray, lambda);
			// Set to false in case we encounter a 'continue' statement before detecting a crossing.
			last_crossing = false;
		} 
		
		// Update data for previous sampling point
		int crossing_level_back = crossing_level_front;
		bool active_back = active_front;
		bool valid_back = valid_front;
		
		// Locate ray sample within a cube face.
		calculate_cube_face_coordinates(ray_sample_position, cube_face_index, cube_face_coordinate_uv);

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// If ray position is within the same tile then no tile meta data lookups are needed.
		
		if (test_variable_6 > 0)
		{
			// always calculate current uv coordinates of the tile
			vec2 tile_meta_data_coordinate_uv;
			get_tile_uv_coordinates(
					cube_face_coordinate_uv, tile_meta_data_resolution, tile_resolution,
					tile_offset_uv, tile_meta_data_coordinate_uv, tile_coordinate_uv);

			// if we sample into a new cube face or a different tile, then get new tile id
			if (prev_cube_face_index != cube_face_index ||
				prev_tile_offset_uv.x != tile_offset_uv.x ||
				prev_tile_offset_uv.y != tile_offset_uv.y)
			{
				get_tile_meta_data_from_tile_uv_coordinates(
						cube_face_index, tile_meta_data_coordinate_uv,
						tile_meta_data_sampler, tile_ID, tile_meta_data_min_max_scalar);
				prev_cube_face_index = cube_face_index;
				prev_tile_offset_uv = tile_offset_uv;
			}
		}
		else
		{
			get_tile_meta_data_from_cube_face_coordinates(
					cube_face_index, cube_face_coordinate_uv,
					tile_meta_data_sampler, tile_meta_data_resolution, tile_resolution,
					tile_offset_uv, tile_coordinate_uv, tile_ID, tile_meta_data_min_max_scalar);
		}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Cull certain tiles, whose scalar value boundaries do not contain the given isovalues
		
		// check, if isovalue is within minimum/maximum scalar value range of sampled tile
		//
		//	((isovalue1.x >=  tile_meta_data_min_max_scalar.x) && (isovalue1.x <= tile_meta_data_min_max_scalar.y )) ||
		//	((isovalue2.x >=  tile_meta_data_min_max_scalar.x) && (isovalue2.x <= tile_meta_data_min_max_scalar.y )) ||
		//	((isovalue1.y >=  tile_meta_data_min_max_scalar.x) && (isovalue1.y <= tile_meta_data_min_max_scalar.y )) ||
		//	((isovalue1.z >=  tile_meta_data_min_max_scalar.x) && (isovalue1.z <= tile_meta_data_min_max_scalar.y )) ||
		//	((isovalue2.y >=  tile_meta_data_min_max_scalar.x) && (isovalue2.y <= tile_meta_data_min_max_scalar.y )) ||
		//	((isovalue2.z >=  tile_meta_data_min_max_scalar.x) && (isovalue2.z <= tile_meta_data_min_max_scalar.y ))
		bool isovalue_in_tile_bounds = bool(
				dot(vec3(greaterThanEqual(isovalue1, vec3(tile_meta_data_min_max_scalar.x))),
					vec3(lessThanEqual(isovalue1, vec3(tile_meta_data_min_max_scalar.y)))) +
				dot(vec3(greaterThanEqual(isovalue2, vec3(tile_meta_data_min_max_scalar.x))),
					vec3(lessThanEqual(isovalue2, vec3(tile_meta_data_min_max_scalar.y)))));
		
		// DEBUGGING 
		//if (test_variable_9 > 0) isovalue_in_tile_bounds = true;

		if (test_variable_2 > 0)
		{
			// if isovalue is not in tile scalar value range, calculate plane intersection points and jump to the next tile
			if (!isovalue_in_tile_bounds &&
				prev_skipped_tile_ID != tile_ID)
			{
				// do not calculate planes twice for one tile
				prev_skipped_tile_ID = tile_ID;
				
				sample_ray_at_tile_exit(
						ray, cube_face_index, tile_offset_uv,
						cube_face_matrix_edge, lambda_min_max,
						lambda, ray_sample_position, colour);
				
				continue; // no lookups have to be done, so sample the next point
			}
		}
		
		// Determine if current ray position samples valid field data.
		valid_front = is_valid_field_data_sample(mask_data_sampler, tile_coordinate_uv, tile_ID);
		active_front = valid_front;
		
		if (!valid_front && !valid_back)
		{
			// go to the next ray sample point and increment current lambda
			ray_sample_position += pos_vec_increment;
			lambda += lambda_increment;
			
			continue;
		}
		
		if (using_surface_fill_mask)
		{
			active_front = active_front && projects_into_surface_fill_mask(
					surface_fill_mask_sampler, surface_fill_mask_resolution,
					cube_face_index, cube_face_coordinate_uv);
		}

		if (test_variable_2 > 0)
		{
			if (!isovalue_in_tile_bounds)
			{
				// go to the next ray sample point and increment current lambda
				ray_sample_position += pos_vec_increment;
				lambda += lambda_increment;
				
				continue;
			}
		}
		
		if (valid_front)
		{
			get_scalar_field_data_from_cube_face_coordinates(
					length(ray_sample_position), cube_face_index, cube_face_coordinate_uv,
					tile_offset_uv, tile_coordinate_uv, tile_ID, 
					field_data_sampler, depth_radius_to_layer_sampler, depth_radius_to_layer_resolution,
					num_depth_layers, min_max_depth_radius,
					field_scalar, field_gradient);
			
			crossing_level_front = get_crossing_level(field_scalar);
		}
		
		float interval_length = lambda_increment;

		// If crossing from a region of valid data to a region of invalid data, or vice versa,
		// then narrow in on the boundary in case there's an isosurface close to the boundary.
		if (valid_front ^^ valid_back)
		{
			// Attempt to contract the [back,front] ray interval such that it is maximally within
			// the valid field data region and hence we can then look for a crossing.
			// This avoids noise artifacts where the isosurface meets the valid region boundary.
			contract_interval_to_valid_region(
					ray, ray_sample_position, lambda, interval_length,
					crossing_level_back, crossing_level_front,
					active_back, active_front, valid_back, valid_front,
					field_scalar, field_gradient);
		}

		// Crossing of one or multiple surfaces is detected.
		if (valid_front && valid_back && (crossing_level_front != crossing_level_back))
		{
			// Bisection correction to obtain more accurate values for crossing location and field gradient
			isosurface_bisection_correction(
					ray, ray_sample_position, lambda, interval_length,
					crossing_level_back, crossing_level_front, active_back, active_front,
					field_scalar, field_gradient);

			// Blend the isosurface ray sample if the isosurface is in an active region.
			if (active_back && active_front)
			{
				blend_isosurface_sample(
						ray, ray_sample_position, crossing_level_back, crossing_level_front,
						field_gradient, colour_matrix_iso, iso_surface_position, colour);
			}
			
			// Flag detected crossing, if we're looking for a crossing in the last ray interval.
			if (last_crossing_active)
			{
				last_crossing = true;
			}
		}

		// Blending step during volume rendering for deviation window zone.
		if (deviation_window_volume_rendering && active_front)
		{
			blend_deviation_window_volume_sample(
					length(ray_sample_position), crossing_level_front,
					field_scalar, field_gradient, colour);
		}
		
		// go to the next ray sample point and increment current lambda
		ray_sample_position += pos_vec_increment;
		lambda += lambda_increment;
	}

	// Blending of earth core sphere or wall colour.
	// Note: The background colour has pre-multiplied alpha in its RGB channels.
	colour += (1 - colour.a) * background_colour;
	
	return (colour.a > 0);
}
	

void
main()
{
	// Return values after ray casting stopped
	vec4 colour;
	vec3 iso_surface_position;

	// Start raycasting process.
	// If raycasting does not return a colour or was not sucessful, discard pixel.
	// Note: The output colour has pre-multiplied alpha in its RGB channels.
	if (!raycasting(colour, iso_surface_position))
	{
		discard;
	}
	
	// Note that we need to write to 'gl_FragDepth' because if we don't then the fixed-function
	// depth will get written (if depth writes enabled) using the fixed-function depth which is that
	// of our full-screen quad (not the actual ray-traced depth).
	vec4 screen_space_iso_surface_position = gl_ModelViewProjectionMatrix * vec4(iso_surface_position, 1.0);
	float screen_space_iso_surface_depth = screen_space_iso_surface_position.z / screen_space_iso_surface_position.w;

	// Convert normalised device coordinates (screen-space) to window coordinates which, for depth, is [-1,1] -> [0,1].
	// Ideally this should also consider the effects of glDepthRange but we'll assume it's set to [0,1].
	gl_FragDepth = 0.5 * screen_space_iso_surface_depth + 0.5;

	//
	// Using multiple render targets here.
	//
	// This is in case another scalar field is rendered as an iso-surface in which case
	// it cannot use the hardware depth buffer because it's not rendering traditional geometry.
	// Well it can, but it's not efficient because it cannot early cull rays (it needs to do the full
	// ray trace before it can output fragment depth to make use of the depth buffer).
	// So instead it must query a depth texture in order to early-cull rays.
	// The hardware depth buffer is still used for correct depth sorting of the cross-sections and iso-surfaces.
	// Note that this doesn't work with semi-transparency (non-zero deviation windows) but that's a much harder
	// problem to solve - depth peeling.
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
	// Note that this does not need to consider the effects of glDepthRange.
	//
	// NOTE: If this output is connected to a depth texture and 'read_from_depth_texture' is true then
	// they cannot both be the same texture because a shader cannot write to the same texture it reads from.
	gl_FragData[1] = vec4(screen_space_iso_surface_depth);
}
