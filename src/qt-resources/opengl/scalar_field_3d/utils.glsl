//
// Various utility structures/functions used for ray-tracing 3D scalar fields.
//

// "#extension" needs to be specified in the shader source *string* where it is used (this is not
// documented in the GLSL spec but is mentioned at http://www.opengl.org/wiki/GLSL_Core_Language).
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
Ray
get_ray(
        const vec2 screen_coord,
        const mat4 model_view_proj_inverse,
        const vec3 eye_position)
{
     return Ray(
        eye_position, // eye_position is the camera position in world coordinates
        normalize(screen_to_world(vec3(screen_coord, 0.0), model_view_proj_inverse) - eye_position));
}

// Convert screen-space depth (in range [-1,1]) to ray distance/lambda.
float
convert_screen_space_depth_to_ray_lambda(
        const float screen_space_depth,
        const vec2 screen_coord,
        const mat4 model_view_proj_inverse,
        const vec3 eye_position)
{
    vec3 world_position = screen_to_world(vec3(screen_coord, screen_space_depth), model_view_proj_inverse);
    return length(world_position - eye_position);
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
    vec2 cube_face_coordinate_xy = vec2(
            dot(cube_face_coordinate_frame.x_axis, projected_world_position),
            dot(cube_face_coordinate_frame.y_axis, projected_world_position));

    return cube_face_coordinate_xy;
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

// Samples the field data texture array.
// Performs bilinear filtering between two nearest depth layers.
vec4
sample_field_data_texture_array(
        sampler2DArray field_data_sampler,
        vec2 tile_coordinate_uv,
        int num_depth_layers,
        float tile_ID,
        float world_position_depth_layer)
{
    // There are 'num_depth_layers' layers per active tile.
    float field_data_layer = tile_ID * num_depth_layers + world_position_depth_layer;

    // Sample the field data texture array.
    // Get the two nearest depth layers so we can interpolate them.
    float lower_field_data_layer = floor(field_data_layer);
    vec4 field_data_lower_layer = texture2DArray(field_data_sampler, vec3(tile_coordinate_uv, lower_field_data_layer));
    vec4 field_data_upper_layer = texture2DArray(field_data_sampler, vec3(tile_coordinate_uv, lower_field_data_layer + 1));

    // Bilinearly interpolate between the two nearest depth layers.
    // According to the GLSL spec the layer selected is:
    //   max(0, min(texture_array_depth-1, floor(layer + 0.5)))
    // ...so accessing one layer past end of texture array is OK (it gets clamped to the last layer).
    // So if we happen to access right at the last layer then it's OK because it'll have a lower bilinear weight of one
    // and the (clamped) upper layer will have a bilinear weight of zero (so won't contribute anyway).
    vec4 field_data = mix(field_data_lower_layer, field_data_upper_layer, field_data_layer - lower_field_data_layer);

    return field_data;
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

    // Adjust for half pixel around surface fill mask boundary to avoid texture seams.
    vec2 surface_fill_mask_coordinate_uv =
        ((surface_fill_mask_resolution - 1) * cube_face_coordinate_uv + 0.5) / surface_fill_mask_resolution;

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
    return surface_fill_mask > 0.5;
}
