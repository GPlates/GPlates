// Vertex shader source to render region-of-interest geometries.

#ifdef POINT_REGION_OF_INTEREST
	const vec3 north_pole = vec3(0, 0, 1);
#endif

attribute vec4 world_space_quaternion;

#ifdef POINT_REGION_OF_INTEREST
	attribute vec3 point_centre;
	// The 'xyz' values are (weight_tangent_x, weight_tangent_y, weight_point_centre)
	attribute vec3 tangent_frame_weights;
#endif

#ifdef LINE_REGION_OF_INTEREST
	attribute vec3 line_arc_start_point;
	attribute vec3 line_arc_normal;
	// The 'xy' values are (weight_arc_normal, weight_start_point)
	attribute vec2 tangent_frame_weights;
#endif

#ifdef FILL_REGION_OF_INTEREST
	attribute vec3 fill_position;
#endif

#ifdef ENABLE_SEED_FRUSTUM_CLIPPING
	// The 'xyz' values are (translate_x, translate_y, scale)
	attribute vec3 raster_frustum_to_seed_frustum_clip_space_transform;
	// The 'xyz' values are (translate_x, translate_y, scale)
	attribute vec3 seed_frustum_to_render_target_clip_space_transform;
#endif

#if defined(POINT_REGION_OF_INTEREST) || defined(LINE_REGION_OF_INTEREST)
varying vec3 present_day_position;
#endif

#ifdef POINT_REGION_OF_INTEREST
	varying vec3 present_day_point_centre;
#endif

#ifdef LINE_REGION_OF_INTEREST
	varying vec3 present_day_line_arc_normal;
#endif

#ifdef ENABLE_SEED_FRUSTUM_CLIPPING
	varying vec4 clip_position_params;
#endif

void main (void)
{
#ifdef POINT_REGION_OF_INTEREST
	// Pass present day point centre to the fragment shader.
	present_day_point_centre = point_centre;

	// Generate the tangent space frame around the point centre.
	// Since the point is symmetric it doesn't matter which tangent frame we choose
	// as long as it's orthonormal.
	vec3 present_day_tangent_x = normalize(cross(north_pole, point_centre));
	vec3 present_day_tangent_y = cross(point_centre, present_day_tangent_x);

	// The weights are what actually determine which vertex of the quad primitive this vertex is.
	// Eg, centre point has weights (0,0,1).
	present_day_position =
		tangent_frame_weights.x * present_day_tangent_x +
		tangent_frame_weights.y * present_day_tangent_y +
		tangent_frame_weights.z * present_day_point_centre;

   // Transform present-day vertex position using finite rotation quaternion.
	// It's ok that the position is not on the unit sphere (it'll still get rotated properly).
	vec3 rotated_position = rotate_vector_by_quaternion(world_space_quaternion, present_day_position);
#endif

#ifdef LINE_REGION_OF_INTEREST
	// Pass the present-day line arc normal to the fragment shader.
	present_day_line_arc_normal = line_arc_normal;

	// The weights (and order of start/end points) are what actually determine which 
	// vertex of the quad primitive this vertex is. Eg, centre point has weights (0,0,1).
	present_day_position =
		tangent_frame_weights.x * line_arc_normal +
		tangent_frame_weights.y * line_arc_start_point;

   // Transform present-day start point using finite rotation quaternion.
	vec3 rotated_position = rotate_vector_by_quaternion(world_space_quaternion, present_day_position);
#endif

#ifdef FILL_REGION_OF_INTEREST
   // Transform present-day position using finite rotation quaternion.
	vec3 rotated_position = rotate_vector_by_quaternion(world_space_quaternion, fill_position);
#endif

	// Transform rotated position by the view/projection matrix.
	// The view/projection matches the target raster tile.
	vec4 raster_frustum_position = gl_ModelViewProjectionMatrix * vec4(rotated_position, 1);

#ifdef ENABLE_SEED_FRUSTUM_CLIPPING
	// Post-projection translate/scale to position NDC space around seed frustum...
	vec4 loose_seed_frustum_position = vec4(
		// Scale and translate x component...
		dot(raster_frustum_to_seed_frustum_clip_space_transform.zx,
				raster_frustum_position.xw),
		// Scale and translate y component...
		dot(raster_frustum_to_seed_frustum_clip_space_transform.zy,
				raster_frustum_position.yw),
		// z and w components unaffected...
		raster_frustum_position.zw);

	// This is also the clip-space the fragment shader uses to cull pixels outside
	// the seed frustum - seed geometry should be bounded by frustum but just in case.
	// Convert to a more convenient form for the fragment shader:
	//   1) Only interested in clip position x, y, w and -w.
	//   2) The z component is depth and we only need to clip to side planes not near/far plane.
	clip_position_params = vec4(
		loose_seed_frustum_position.xy,
		loose_seed_frustum_position.w,
		-loose_seed_frustum_position.w);

	// Post-projection translate/scale to position NDC space around render target frustum...
	vec4 render_target_frustum_position = vec4(
		// Scale and translate x component...
		dot(seed_frustum_to_render_target_clip_space_transform.zx,
				loose_seed_frustum_position.xw),
		// Scale and translate y component...
		dot(seed_frustum_to_render_target_clip_space_transform.zy,
				loose_seed_frustum_position.yw),
		// z and w components unaffected...
		loose_seed_frustum_position.zw);

	gl_Position = render_target_frustum_position;
#else

	// When the seed frustum matches the target raster tile there is no need
	// for seed frustum clipping (happens automatically due to view frustum).
	// In this case both the raster frustum to seed frustum and seed frustum to
	// render target frustum are identity transforms and are not needed.
	// .
	gl_Position = raster_frustum_position;
#endif
}
