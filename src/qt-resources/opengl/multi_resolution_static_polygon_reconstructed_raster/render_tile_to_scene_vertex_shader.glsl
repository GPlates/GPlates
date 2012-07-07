//
// Vertex shader source code to render reconstructed raster tiles to the scene.
//

uniform mat4 source_raster_texture_transform;
uniform mat4 clip_texture_transform;
varying vec4 source_raster_texture_coordinate;
varying vec4 clip_texture_coordinate;

#ifdef USING_AGE_GRID
	uniform mat4 age_grid_texture_transform;
	varying vec4 age_grid_texture_coordinate;
#endif

#ifdef SURFACE_LIGHTING
	uniform vec4 plate_rotation_quaternion;
	varying vec3 world_space_sphere_normal;
	#ifdef USING_NORMAL_MAP
		uniform mat4 normal_map_texture_transform;
		varying vec4 normal_map_texture_coordinate;
	#endif
	#ifndef MAP_VIEW
		uniform vec3 world_space_light_direction;
		varying vec3 model_space_light_direction;
	#endif
#endif

void main (void)
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

	// Clip texture cube map projection.
	clip_texture_coordinate = clip_texture_transform * gl_Vertex;

	// Source raster cube map projection.
	source_raster_texture_coordinate = source_raster_texture_transform * gl_Vertex;

#ifdef USING_AGE_GRID
	// Age grid cube map projection.
	age_grid_texture_coordinate = age_grid_texture_transform * gl_Vertex;
#endif

#ifdef SURFACE_LIGHTING
	// Rotate vertex position to world-space - it's also the sphere normal.
	// This will also be the texture coordinate for the map view's light direction cube texture.
	world_space_sphere_normal = rotate_vector_by_quaternion(plate_rotation_quaternion, gl_Vertex.xyz);
	#ifdef USING_NORMAL_MAP
		// Normal map raster cube map projection.
		normal_map_texture_coordinate = normal_map_texture_transform * gl_Vertex;
	#endif
	#ifndef MAP_VIEW
		// It's more efficient for fragment shader to do lambert dot product in model-space
		// instead of world-space so reverse rotate light direction into model space.
		vec4 plate_reverse_rotation_quaternion =
			vec4(-plate_rotation_quaternion.xyz, plate_rotation_quaternion.w);
		model_space_light_direction = rotate_vector_by_quaternion(
			plate_reverse_rotation_quaternion, world_space_light_direction);
	#endif
#endif
}
