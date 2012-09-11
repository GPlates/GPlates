//
// Fragment shader source code to render volume fill spherical caps.
//

// The surface fill mask texture.
uniform sampler2DArray surface_fill_mask_sampler;
uniform int surface_fill_mask_resolution;

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
			surface_fill_mask_resolution,
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
	// Write the  *screen-space* depth (ie, depth range [-1,1] and not [0,1]).
	// This is what's used in the ray-tracing shader since it uses inverse model-view-proj matrix on the depth
	// to get world space position and that requires normalised device coordinates not window coordinates).
	// Ideally this should also consider the effects of glDepthRange but we'll assume it's set to [0,1].
	gl_FragColor = vec4(2 * gl_FragCoord.z - 1);
#endif
}
