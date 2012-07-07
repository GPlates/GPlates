// Vertex shader source code to render polygons to the polygon stencil texture.

attribute vec3 present_day_position;
attribute vec4 fill_colour;
attribute vec4 world_space_quaternion;
// The 'xyzw' values are (translate_x, translate_y, scale_x, scale_y)
attribute vec4 polygon_frustum_to_render_target_clip_space_transform;

varying vec4 clip_position_params;
varying vec4 fragment_fill_colour;

void main (void)
{
	// The polygon fill colour.
	fragment_fill_colour = fill_colour;

   // Transform present-day position using finite rotation quaternion.
	vec3 rotated_position = rotate_vector_by_quaternion(world_space_quaternion, present_day_position);

	// Transform rotated position by the view/projection matrix.
	// The view/projection matches the polygon frustum.
	vec4 polygon_frustum_position = gl_ModelViewProjectionMatrix * vec4(rotated_position, 1);

	// This is also the clip-space the fragment shader uses to cull pixels outside
	// the polygon frustum.
	// Convert to a more convenient form for the fragment shader:
	//   1) Only interested in clip position x, y, w and -w.
	//   2) The z component is depth and we only need to clip to side planes not near/far plane.
	clip_position_params = vec4(
		polygon_frustum_position.xy,
		polygon_frustum_position.w,
		-polygon_frustum_position.w);

	// Post-projection translate/scale to position NDC space around render target frustum...
	vec4 render_target_frustum_position = vec4(
		// Scale and translate x component...
		dot(polygon_frustum_to_render_target_clip_space_transform.zx,
				polygon_frustum_position.xw),
		// Scale and translate y component...
		dot(polygon_frustum_to_render_target_clip_space_transform.wy,
				polygon_frustum_position.yw),
		// z and w components unaffected...
		polygon_frustum_position.zw);

	gl_Position = render_target_frustum_position;
}
