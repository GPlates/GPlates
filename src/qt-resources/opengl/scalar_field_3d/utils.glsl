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
// Various utility structures/functions used for ray-tracing, or rendering cross-sections of, 3D scalar fields.
//

// GPlates currently moves this to start of *first* source code string passed into glShaderSource.
// This is because extension lines must not occur *after* any non-preprocessor source code.
//
// NOTE: Do not comment this out with a /**/ comment spanning multiple lines since GPlates does not detect that.
#extension GL_EXT_texture_array : enable

// A local orthonormal coordinate frame for a particular cube face.
struct CubeFaceCoordinateFrame
{
	vec3 x_axis;
	vec3 y_axis;
	vec3 z_axis;
};

// The local coordinate frames for each cube face.
//
// NOTE: This used to be a constant array (not using uniform variables)
// but MacOS (Snow Leopard) has not fully implemented the GLSL 1.2 specification
// so we use a uniform array instead (and set the values using C++ code).
uniform CubeFaceCoordinateFrame cube_face_coordinate_frames[6];


struct Ray
{
  vec3 origin;
  vec3 direction;
};

// Position at given ray parameter
vec3
at(
		const Ray ray,
		float lambda)
{
  return ray.origin + lambda * ray.direction;
}

struct Interval
{
  float from;
  float to;
};

bool
isValid(
		const Interval interval)
{
  return interval.from <= interval.to;
}

void
invalidate(
		Interval interval)
{
  interval.from = 1;
  interval.to = 0;
}

struct Sphere
{
  float radius_squared;
};

bool
intersect(
		const Sphere sphere,
		const Ray ray,
		out Interval interval)
{
  float B = dot(ray.origin, ray.direction);
  float C = dot(ray.origin, ray.origin) - sphere.radius_squared;
  float D = B * B - C;
  if (D < 0)
  {
	return false;
  }
  float sqrtD = sqrt(D);
  interval = Interval(-B - sqrtD /*from*/, -B + sqrtD /*to*/);
  return true;
}

bool
contains(
		const Sphere sphere,
		const vec3 point)
{
  float distSq = dot(point, point);
  return distSq <= sphere.radius_squared;
}

// Transformation of 3D screen-space coordinates (includes depth) to world coordinates.
// These screen-space coordinates are post-projection coordinates in the range [-1,1] for x, y and z components.
vec3
screen_to_world(
		const vec3 screen_coord,
		const mat4 model_view_proj_inverse)
{
	 vec4 screen_position = vec4(screen_coord, 1.0);
	 vec4 world_position = model_view_proj_inverse * screen_position;
	 world_position /= world_position.w;

	 return world_position.xyz;
}

// Ray initialization from screen-space coordinate.
//
// The inverse model-view-projection transform is used to convert the current screen coordinate (x,y,-2) to
// world space for the ray origin and (x,y,0) to world space to calculate (normalised) ray direction.
// The screen-space depth of -2 is outside the view frustum [-1,1] and in front of the near clip plane.
// The screen-space depth of 0 is inside the view frustum [-1,1] between near and far clip planes.
// Both values are somewhat arbitrary actually (since we don't really care where the ray origin is along the ray line).
Ray
get_ray(
		const vec2 screen_coord,
		const mat4 model_view_proj_inverse)
{
	 vec3 ray_origin = screen_to_world(vec3(screen_coord, -2.0), model_view_proj_inverse);

	 return Ray(
		ray_origin,
		normalize(screen_to_world(vec3(screen_coord, 0.0), model_view_proj_inverse) - ray_origin));
}

// Convert screen-space depth (in range [-1,1]) to ray distance/lambda.
float
convert_screen_space_depth_to_ray_lambda(
		const float screen_space_depth,
		const vec2 screen_coord,
		const mat4 model_view_proj_inverse,
		const vec3 ray_origin)
{
	vec3 world_position = screen_to_world(vec3(screen_coord, screen_space_depth), model_view_proj_inverse);
	return length(world_position - ray_origin);
}

// Look up a 1D texture using 'input_value'.
// 'input_lower_bound' and 'input_upper_bound' map to the first and last texel.
vec4
look_up_table_1D(
		sampler1D table_sampler,
		float table_resolution,
		float input_lower_bound,
		float input_upper_bound,
		float input_value)
{
	// Map the range [input_lower_bound, input_upper_bound] to [0,1].
	float table_coordinate_u = (input_value - input_lower_bound) / (input_upper_bound - input_lower_bound);

	// Adjust for half pixel since table lower and upper bounds are mapped to the first and last texel centres.
	// Everything in between is linearly interpolated.
	table_coordinate_u = ((table_resolution - 1) * table_coordinate_u + 0.5) / table_resolution;

	return texture1D(table_sampler, table_coordinate_u);
}

// Project the world position onto the cube face and determine the cube face that it projects onto.
vec3
project_world_position_onto_cube_face(
		vec3 world_position,
		out int cube_face_index)
{
	// Determine the cube face that the current world position projects onto.
	float cube_face_projection_scale;

	vec3 abs_world_position = abs(world_position);

	if (abs_world_position.x > abs_world_position.y)
	{
		if (abs_world_position.x > abs_world_position.z)
		{
			cube_face_projection_scale = abs_world_position.x;
			cube_face_index = 0 + int(world_position.x <= 0);
		}
		else
		{
			cube_face_projection_scale = abs_world_position.z;
			cube_face_index = 4 + int(world_position.z <= 0);
		}
	}
	else 
	{
		if (abs_world_position.y > abs_world_position.z)
		{
			cube_face_projection_scale = abs_world_position.y;
			cube_face_index = 2 + int(world_position.y <= 0); 
		}
		else
		{
			cube_face_projection_scale = abs_world_position.z;
			cube_face_index = 4 + int(world_position.z <= 0);
		}
	}

	// Project the world position onto the cube face.
	vec3 projected_world_position = world_position / cube_face_projection_scale;

	return projected_world_position;
}

// Transforms a world position (projected onto a cube face) into that cube face's local coordinate frame.
// Only generates local (x,y) components since z component is assumed to be *on* the cube face (ie, local z = 1).
vec2
convert_projected_world_position_to_local_cube_face_coordinate_frame(
		vec3 projected_world_position,
		int cube_face_index)
{
	// Determine the local (x,y) coordinates in the cube face coordinate frame.
	CubeFaceCoordinateFrame cube_face_coordinate_frame = cube_face_coordinate_frames[cube_face_index];
	// local cube face coordinates in range [-1,1]
	vec2 cube_face_coordinate_xy = vec2(
			dot(cube_face_coordinate_frame.x_axis, projected_world_position),
			dot(cube_face_coordinate_frame.y_axis, projected_world_position));

	return cube_face_coordinate_xy;
}

// get cube face index and uv coordinates from position
void
calculate_cube_face_coordinates(
		in vec3 position,
		out int cube_face_index,
		out vec2 cube_face_coordinate_uv)
{
	// first project position onto cube face and determine cube face index
	vec3 projected_position = project_world_position_onto_cube_face(position, cube_face_index);
	
	// calculate cube face coordinates
	vec2 cube_face_coordinate_xy = convert_projected_world_position_to_local_cube_face_coordinate_frame(
			projected_position, cube_face_index);
			
	// transform coordinates from [-1,1] to [0,1]
	cube_face_coordinate_uv = 0.5 * cube_face_coordinate_xy + 0.5;
}

// Converts the local cube face (u,v) coordinates to (u,v) coordinates into metadata and field data textures.
void
get_tile_uv_coordinates(
		vec2 cube_face_coordinate_uv,
		int tile_meta_data_resolution,
		int tile_resolution,
		out vec2 tile_offset_uv,
		out vec2 tile_meta_data_coordinate_uv,
		out vec2 tile_coordinate_uv)
{
	// Determine the tile (u,v) offsets - this will be used later to determine which field tile layer to access.
	// Need to clamp to ensure we don't go out-of-bounds (due to numerical tolerances).
	// Normally texture edge clamping, in 'tile_meta_data_sampler', would take care of that for us but 'tile_offset_uv'
	// is also used to determine the field tile layer for 'field_data_sampler' and it can get out-of-sync otherwise.
	// This was noticeable as a sliver of rendered pixels appearing in the middle of nowhere at a cube face boundary.
	tile_offset_uv = clamp(floor(tile_meta_data_resolution * cube_face_coordinate_uv), 0.0, tile_meta_data_resolution - 1);

	// Determine the 2D texture coordinates within the tile metadata texture layer.
	// The tile metadata texture array uses nearest neighbour filtering so just place coordinates at texel centres.
	// We use 'tile_offset_uv' since we don't want the metadata and field texture accesses to get out-of-sync near tile boundaries.
	tile_meta_data_coordinate_uv = (tile_offset_uv + 0.5) / tile_meta_data_resolution;

	// Determine the 2D texture coordinates within the tile texture layer.
	tile_coordinate_uv = tile_meta_data_resolution * cube_face_coordinate_uv - tile_offset_uv;

	// Adjust for half pixel around tile boundary to avoid texture seams.
	tile_coordinate_uv = ((tile_resolution - 1) * tile_coordinate_uv + 0.5) / tile_resolution;
}

// Get tile metadata from cube face index and tile uv coordinates.
void
get_tile_meta_data_from_tile_uv_coordinates(
		in int cube_face_index,
		in vec2 tile_meta_data_coordinate_uv,
		in sampler2DArray tile_meta_data_sampler,
		out float tile_ID,
		out vec2 tile_meta_data_min_max_scalar)
{
	// access tile meta data and get tile ID (contained in r - component)
	vec3 tile_meta_data = texture2DArray(tile_meta_data_sampler, vec3(tile_meta_data_coordinate_uv,cube_face_index)).rgb;
	
	// get tile ID
	tile_ID = tile_meta_data.r;
	// get min and max scalar value boundaries
	tile_meta_data_min_max_scalar = tile_meta_data.bg;
}

// Get tile metadata and tile uv coordinates from cube face index and cube face uv coordinates.
void
get_tile_meta_data_from_cube_face_coordinates(
		in int cube_face_index,
		in vec2 cube_face_coordinate_uv,
		in sampler2DArray tile_meta_data_sampler,
		in int tile_meta_data_resolution,
		in int tile_resolution,
		out vec2 tile_offset_uv,
		out vec2 tile_coordinate_uv,
		out float tile_ID,
		out vec2 tile_meta_data_min_max_scalar)
{
	// get tile coordinates
	vec2 tile_meta_data_coordinate_uv;
	get_tile_uv_coordinates(
			cube_face_coordinate_uv, tile_meta_data_resolution, tile_resolution,
			tile_offset_uv, tile_meta_data_coordinate_uv, tile_coordinate_uv);

	get_tile_meta_data_from_tile_uv_coordinates(
			cube_face_index, tile_meta_data_coordinate_uv, tile_meta_data_sampler,
			tile_ID, tile_meta_data_min_max_scalar);
}

// Get tile metadata and tile uv coordinates from cube face index and cube face uv coordinates.
void
get_tile_meta_data_from_position(
		in vec3 position,
		in sampler2DArray tile_meta_data_sampler,
		in int tile_meta_data_resolution,
		in int tile_resolution,
		out vec2 tile_offset_uv,
		out vec2 tile_coordinate_uv,
		out float tile_ID,
		out vec2 tile_meta_data_min_max_scalar)
{
	int cube_face_index;
	vec2 cube_face_coordinate_uv;
	calculate_cube_face_coordinates(position, cube_face_index, cube_face_coordinate_uv);
	
	get_tile_meta_data_from_cube_face_coordinates(
			cube_face_index, cube_face_coordinate_uv,
			tile_meta_data_sampler, tile_meta_data_resolution, tile_resolution,
			tile_offset_uv, tile_coordinate_uv, tile_ID, tile_meta_data_min_max_scalar);
}

// Determines if the field position contains valid field data.
//
// Returns true if the field position's tile is valid and all field samples in bilinear filter
// (of the field position) contain valid field data.
// This ensures that sampling the field data itself will only use valid field samples in its bilinear filter.
bool
is_valid_field_data_sample(
		sampler2DArray mask_data_sampler,
		vec2 tile_coordinate_uv,
		float tile_ID)
		//int mask_data_resolution,
		//int cube_face_index,
		//vec2 cube_face_coordinate_uv)
{
	if (tile_ID < 0)
	{
		return false;
	}
		
	// Sample the mask data texture array.
	float mask_data = texture2DArray(mask_data_sampler, vec3(tile_coordinate_uv, tile_ID)).r;
	
	// If any sample in the bilinear filter is zero then return false.
	// If all bilinear samples are valid then the bilinear filter will return 1.0.
	return mask_data == 1.0;
}

// Determines if the field position contains valid field data.
//
// Same as the other overload but accepts position instead of tile ID/coordinates.
bool
is_valid_field_data_sample(
		vec3 position,
		sampler2DArray tile_meta_data_sampler,
		int tile_meta_data_resolution,
		int tile_resolution,
		sampler2DArray mask_data_sampler)
{
	vec2 tile_offset_uv;
	vec2 tile_coordinate_uv;
	float tile_ID;
	vec2 tile_meta_data_min_max_scalar;
	get_tile_meta_data_from_position(
			position, tile_meta_data_sampler, tile_meta_data_resolution,
			tile_resolution, tile_offset_uv, tile_coordinate_uv,
			tile_ID, tile_meta_data_min_max_scalar);
			
	return is_valid_field_data_sample(mask_data_sampler, tile_coordinate_uv, tile_ID);
}

// Samples the depth-radius-to-depth-layer texture.
// Returns the integer bounding depth layers and the fractional layer.
void
get_depth_layer(
		in float depth_radius,
		in sampler1D depth_radius_to_layer_sampler,
		in int depth_radius_to_layer_resolution,
		in vec2 min_max_depth_radius,
		out int lower_depth_layer,
		out int upper_depth_layer,
		out float frac_depth_layer)
{
	// Convert depth radius to depth layer.
	//
	// There will be small mapping errors within a texel distance to each depth radius (for non-equidistant depth mappings)
	// but that can only be reduced by increasing the resolution of the 1D texture.
	// Note that 'depth_layer' is not an integer layer index.
	float depth_layer = look_up_table_1D(
			depth_radius_to_layer_sampler, depth_radius_to_layer_resolution,
			min_max_depth_radius.x, min_max_depth_radius.y, depth_radius).r;
	
	// Get the two nearest depth layers so we can interpolate them.
	lower_depth_layer = int(floor(depth_layer));
	upper_depth_layer = lower_depth_layer + 1;
	frac_depth_layer = depth_layer - float(lower_depth_layer);
}

// Convert the cube face index, tile offset/coordinate and depth layer to
// UVW coordinates for sampling the field data texture array.
void
get_field_data_tile_uvw(
		int cube_face_index,
		vec2 tile_offset_uv,
		int tile_ID,
		int num_depth_layers,
		int lower_depth_layer,
		int upper_depth_layer,
		out vec3 lower_field_data_tile_uvw,
		out vec3 upper_field_data_tile_uvw)
{
	// There are 'num_depth_layers' layers per active tile.
	int lower_field_data_layer = tile_ID * num_depth_layers + lower_depth_layer;
	int upper_field_data_layer = tile_ID * num_depth_layers + upper_depth_layer;

	lower_field_data_tile_uvw = vec3(vec2(0), lower_field_data_layer);
	upper_field_data_tile_uvw = vec3(vec2(0), upper_field_data_layer);
}

// Get the field data UVW coordinates from the tile location and the intra-tile coordinates.
void
get_field_data_uv(
		vec2 tile_coordinate_uv,
		vec3 lower_field_data_tile_uvw,
		vec3 upper_field_data_tile_uvw,
		out vec3 lower_field_data_uvw,
		out vec3 upper_field_data_uvw)
{
	lower_field_data_uvw = lower_field_data_tile_uvw;
	upper_field_data_uvw = upper_field_data_tile_uvw;
	
	lower_field_data_uvw.xy += tile_coordinate_uv;
	upper_field_data_uvw.xy += tile_coordinate_uv;
}

// Samples the field data texture array.
// Performs bilinear filtering between two specified depth layers.
vec4
sample_field_data_texture_array(
		vec3 lower_field_data_uvw,
		vec3 upper_field_data_uvw,
		float frac_upper_field,
		sampler2DArray field_data_sampler)
{
	// Sample the field data texture array at the two nearest depth layers and interpolate them.
	vec4 lower_field_data = texture2DArray(field_data_sampler, lower_field_data_uvw);
	vec4 upper_field_data = texture2DArray(field_data_sampler, upper_field_data_uvw);

	// Bilinearly interpolate between the two depth layers.
	// According to the GLSL spec the layer selected is:
	//   max(0, min(texture_array_depth-1, floor(layer + 0.5)))
	// ...so accessing one layer past end of texture array is OK (it gets clamped to the last layer).
	// So if we happen to access right at the last layer then it's OK because it'll have a lower bilinear weight of one
	// and the (clamped) upper layer will have a bilinear weight of zero (so won't contribute anyway).
	return mix(lower_field_data, upper_field_data, frac_upper_field);
}

// get scalar field data from texture 2D array
//
// NOTE: It is assumed the sampled field position contains valid data - see 'is_valid_field_data_sample()'.
void
get_scalar_field_data_from_cube_face_coordinates(
		in float depth_radius,
		in int cube_face_index,
		in vec2 cube_face_coordinate_uv,
		in vec2 tile_offset_uv,
		in vec2 tile_coordinate_uv,
		in float tile_ID,
		in sampler2DArray field_data_sampler,
		in sampler1D depth_radius_to_layer_sampler,
		in int depth_radius_to_layer_resolution,
		in int num_depth_layers,
		in vec2 min_max_depth_radius,
		inout float field_scalar,
		inout vec3 field_gradient)
{
	// Convert depth radius to depth layer.
	int lower_depth_layer;
	int upper_depth_layer;
	float frac_depth_layer;
	get_depth_layer(
			depth_radius,
			depth_radius_to_layer_sampler, depth_radius_to_layer_resolution, min_max_depth_radius,
			lower_depth_layer, upper_depth_layer, frac_depth_layer);

	// Find the location of the tile within the field data texture array.
	vec3 lower_field_data_tile_uvw;
	vec3 upper_field_data_tile_uvw;
	get_field_data_tile_uvw(
			cube_face_index, tile_offset_uv, int(tile_ID),
			num_depth_layers, lower_depth_layer, upper_depth_layer,
			lower_field_data_tile_uvw, upper_field_data_tile_uvw);

	// Get the field data UVW coordinates from the tile location and the intra-tile coordinates.
	vec3 lower_field_data_uvw;
	vec3 upper_field_data_uvw;
	get_field_data_uv(
			tile_coordinate_uv,
			lower_field_data_tile_uvw, upper_field_data_tile_uvw,
			lower_field_data_uvw, upper_field_data_uvw);

	// Sample the field data.
	vec4 field_data = sample_field_data_texture_array(
			lower_field_data_uvw, upper_field_data_uvw, frac_depth_layer, field_data_sampler);

	// grab scalar value
	field_scalar = field_data.r;
	// grab gradient vector
	field_gradient = field_data.gba;
}

// Get scalar field data from texture 2D array according to position.
//
// Returns true if the field contains valid data at the field position -
// field data can be invalid if the field position is within an inactive tile or
// if the tile is active but only part of the tile is valid (due to mask).
bool
get_scalar_field_data_from_position(
		in vec3 position,
		in sampler2DArray tile_meta_data_sampler,
		in int tile_meta_data_resolution,
		in sampler2DArray mask_data_sampler,
		in int tile_resolution,
		in sampler2DArray field_data_sampler,
		in sampler1D depth_radius_to_layer_sampler,
		in int depth_radius_to_layer_resolution,
		in int num_depth_layers,
		in vec2 min_max_depth_radius,
		inout int cube_face_index,
		inout vec2 cube_face_coordinate_uv,
		inout float field_scalar, 
		inout vec3 field_gradient)
{
	calculate_cube_face_coordinates(position, cube_face_index, cube_face_coordinate_uv);
	
	// tile offset indices
	vec2 tile_offset_uv;
	// tile uv coordinates
	vec2 tile_coordinate_uv;
	// tile id number per cube face
	float tile_ID;
	// min and max scalar value boundaries
	vec2 tile_meta_data_min_max_scalar;
	get_tile_meta_data_from_cube_face_coordinates(
			cube_face_index, cube_face_coordinate_uv,
			tile_meta_data_sampler, tile_meta_data_resolution, tile_resolution,
			tile_offset_uv, tile_coordinate_uv, tile_ID, tile_meta_data_min_max_scalar);

	// Return false if the current tile (or field position) has no valid data.
	if (!is_valid_field_data_sample(mask_data_sampler, tile_coordinate_uv, tile_ID))
	{
		return false;
	}
	
	// Sample the field data.
	float depth_radius = length(position);
	get_scalar_field_data_from_cube_face_coordinates(
			depth_radius, cube_face_index, cube_face_coordinate_uv,
			tile_offset_uv, tile_coordinate_uv, tile_ID,
			field_data_sampler, depth_radius_to_layer_sampler,
			depth_radius_to_layer_resolution, num_depth_layers, min_max_depth_radius,
			field_scalar, field_gradient);
	
	// Field position contains valid field data.
	return true;
}

// Samples the surface fill mask texture array to return a value in the range [0,1].
float
sample_surface_fill_mask_texture_array(
		sampler2DArray surface_fill_mask_sampler,
		int surface_fill_mask_resolution,
		int cube_face_index,
		vec2 cube_face_coordinate_uv)
{
	// The surface fill mask has only one texture layer per cube face.

	// Adjust for (one-and-a)-half pixel around surface fill mask boundary to avoid texture seams.
	//
	// NOTE: Each cube face of surface fill mask has been expanded by 1.5 texels instead of the normal 0.5 texels.
	// This is because the centre of the next-to-border texels has been mapped to the edge
	// of a cube face frustum (instead of the centre of border texels).
	// This enables dilation versions of sampling functions to sample a 3x3 texel pattern and not
	// have any sample texture coordinates get clamped (which could cause issues at cube face
	// edges). The 3x3 sample pattern is used to emulate a pre-processing dilation of the texture.
	vec2 surface_fill_mask_coordinate_uv =
		((surface_fill_mask_resolution - 3) * cube_face_coordinate_uv + 1.5) / surface_fill_mask_resolution;

	// Sample the surface fill mask texture array (an 8-bit RGBA with the mask value in RGB).
	float surface_fill_mask = texture2DArray(
			surface_fill_mask_sampler,
			vec3(surface_fill_mask_coordinate_uv, cube_face_index)).r;

	// Zero means no fill, one means fill and inbetween is bilinear filtering.
	return surface_fill_mask;
}

// Returns true if the specified cube face location projects into a surface fill mask region, otherwise returns false.
bool
projects_into_surface_fill_mask(
		sampler2DArray surface_fill_mask_sampler,
		int surface_fill_mask_resolution,
		int cube_face_index,
		vec2 cube_face_coordinate_uv)
{
	// Sample the surface fill mask texture array.
	float surface_fill_mask = sample_surface_fill_mask_texture_array(
			surface_fill_mask_sampler,
			surface_fill_mask_resolution,
			cube_face_index,
			cube_face_coordinate_uv);

	// Zero means no fill, one means fill and inbetween is bilinear filtering - so choose 0.5 as the cutoff.
	// Also using 0.5 makes a smoother mask boundary (less jagged than 0 or 1 which are stair-step in appearance).
	// Also using 0.5 means the (smoothed) jagged mask boundary line averages (oscillates about) the
	// infinite resolution (non-jagged) boundary line.
	return surface_fill_mask > 0.5;
}

// Same as 'sample_surface_fill_mask_texture_array()' except emulates a dilated surface fill mask by also sampling
// the 3x3 (bilinearly filtered) sample locations around the UV coordinate location.
float
sample_dilated_surface_fill_mask_texture_array(
		sampler2DArray surface_fill_mask_sampler,
		int surface_fill_mask_resolution,
		int cube_face_index,
		vec2 cube_face_coordinate_uv)
{
	// The surface fill mask has only one texture layer per cube face.
	
	float delta_uv_texel = 1.0 / surface_fill_mask_resolution;

	// Get the UV coordinate location (this is the centre sample location of the 3x3 sample pattern).
	//
	// NOTE: Each cube face of surface fill mask has been expanded by 1.5 texels instead of the normal 0.5 texels.
	// This is because the centre of the next-to-border texels has been mapped to the edge
	// of a cube face frustum (instead of the centre of border texels).
	// This enables dilation versions of sampling functions to sample a 3x3 texel pattern and not
	// have any sample texture coordinates get clamped (which could cause issues at cube face
	// edges). The 3x3 sample pattern is used to emulate a pre-processing dilation of the texture.
	vec2 uv_centre = delta_uv_texel * ((surface_fill_mask_resolution - 3) * cube_face_coordinate_uv + 1.5);

	vec3 uvw_00 = vec3(uv_centre + vec2(-delta_uv_texel, -delta_uv_texel), cube_face_index);
	vec3 uvw_01 = vec3(uv_centre + vec2(-delta_uv_texel, 0), cube_face_index);
	vec3 uvw_02 = vec3(uv_centre + vec2(-delta_uv_texel, delta_uv_texel), cube_face_index);
			
	vec3 uvw_10 = vec3(uv_centre + vec2(0, -delta_uv_texel), cube_face_index);
	vec3 uvw_11 = vec3(uv_centre + vec2(0, 0), cube_face_index);
	vec3 uvw_12 = vec3(uv_centre + vec2(0, delta_uv_texel), cube_face_index);
			
	vec3 uvw_20 = vec3(uv_centre + vec2(delta_uv_texel, -delta_uv_texel), cube_face_index);
	vec3 uvw_21 = vec3(uv_centre + vec2(delta_uv_texel, 0), cube_face_index);
	vec3 uvw_22 = vec3(uv_centre + vec2(delta_uv_texel, delta_uv_texel), cube_face_index);
	
	// Sample the surface fill mask texture array (an 8-bit RGBA with the mask value in RGB).
	// Note that each of these texture samples is a bilinear filtering of nearest 2x2 texels.
	// This gives a radial reach of about 1.5 texels (instead of 1 texel).
	float surface_fill_mask = 0;
	surface_fill_mask += texture2DArray(surface_fill_mask_sampler, uvw_00).r;
	surface_fill_mask += texture2DArray(surface_fill_mask_sampler, uvw_01).r;
	surface_fill_mask += texture2DArray(surface_fill_mask_sampler, uvw_02).r;
	surface_fill_mask += texture2DArray(surface_fill_mask_sampler, uvw_10).r;
	surface_fill_mask += texture2DArray(surface_fill_mask_sampler, uvw_11).r;
	surface_fill_mask += texture2DArray(surface_fill_mask_sampler, uvw_12).r;
	surface_fill_mask += texture2DArray(surface_fill_mask_sampler, uvw_20).r;
	surface_fill_mask += texture2DArray(surface_fill_mask_sampler, uvw_21).r;
	surface_fill_mask += texture2DArray(surface_fill_mask_sampler, uvw_22).r;
	
	// Return the average of the 3x3 samples.
	return (1.0 / 9.0) * surface_fill_mask;
}

// Same as 'projects_into_surface_fill_mask()' except emulates a dilated surface fill mask by also sampling
// the 3x3 (bilinearly filtered) sample locations around the UV coordinate location.
bool
projects_into_dilated_surface_fill_mask(
		sampler2DArray surface_fill_mask_sampler,
		int surface_fill_mask_resolution,
		int cube_face_index,
		vec2 cube_face_coordinate_uv)
{
	// Sample the surface fill mask texture array.
	float surface_fill_mask = sample_dilated_surface_fill_mask_texture_array(
			surface_fill_mask_sampler,
			surface_fill_mask_resolution,
			cube_face_index,
			cube_face_coordinate_uv);

	// Zero means no fill across dilation pattern and one means full fill across dilation pattern.
	// In order to extend the dilation pattern further we test against zero.
	return surface_fill_mask > 0;
}

// creates normals of all four planes for a given cube face and tile
void
create_plane_normals_from_vectors(
		in mat3 cube_face_matrix,
		in int tile_meta_data_resolution,
		in vec2 tile_offset_uv,
		out vec3 plane_normals[4])
{
	// size of one tile in a cube face (defraction of cube edge size = 1)
	float tile_size = 1.0f / tile_meta_data_resolution;
	// fraction of offsets depending on tile resolution
	vec2 tile_offset_frac = tile_offset_uv / tile_meta_data_resolution;
	// x direction vector of cube face
	vec3 cube_face_dir_x = tile_size * cube_face_matrix[1];
	// y direction vector of cube face
	vec3 cube_face_dir_y = tile_size * cube_face_matrix[2];

	// create all cube vertices
	vec3 cube_face_0_0 = cube_face_matrix[0]	+ tile_offset_frac.x * cube_face_matrix[1] 
												+ tile_offset_frac.y * cube_face_matrix[2];
	vec3 cube_face_1_0 = cube_face_0_0 + cube_face_dir_x;
	vec3 cube_face_0_1 = cube_face_0_0 + cube_face_dir_y;
	vec3 cube_face_1_1 = cube_face_0_0 + cube_face_dir_x + cube_face_dir_y;

	// create normals from given vertices
	// normals pointing indoors
	plane_normals[0] = normalize(cross(cube_face_0_0,cube_face_0_1));
	plane_normals[1] = normalize(cross(cube_face_1_0,cube_face_0_0));
	plane_normals[2] = normalize(cross(cube_face_0_1,cube_face_1_1));
	plane_normals[3] = normalize(cross(cube_face_1_1,cube_face_1_0));
}

// tests, if point is within the four tile planes
// tolerance is used due to imprecision of floats
bool
is_point_within_tile_planes(
		vec3 sample_pos,
		vec3 plane_normals[4], 
		float tolerance)
{
	// Grouping 4 floats into 1 vec4 enables the use of inbuilt vector comparison operators.

	vec4 distance_to_planes = vec4(
			dot(sample_pos, plane_normals[0]),
			dot(sample_pos, plane_normals[1]),
			dot(sample_pos, plane_normals[2]),
			dot(sample_pos, plane_normals[3]));

	vec4 maximum_plane_distance = vec4(0 - tolerance);

	// distance_to_plane_1 >= maximum_plane_distance && 
	// distance_to_plane_2 >= maximum_plane_distance && 
	// distance_to_plane_3 >= maximum_plane_distance && 
	// distance_to_plane_4 >= maximum_plane_distance
	return all(greaterThanEqual(distance_to_planes, maximum_plane_distance));
}

// calculates lambda value of intersection point of ray line with plane
float
get_lambda_from_plane(
		Ray ray,
		vec3 plane_normal)
{
	// ray = o + lambda * d;
	// lambda = -(o * n) / (d * n)
	float dot_denum = dot(ray.direction,plane_normal);
	// if ray is parallel to plane, then dot product converges to 0
	if (dot_denum == 0) // if ray is exactly parallel
		return 0;

	float dot_num = -dot(ray.origin,plane_normal);

	return (dot_num / dot_denum);
}
