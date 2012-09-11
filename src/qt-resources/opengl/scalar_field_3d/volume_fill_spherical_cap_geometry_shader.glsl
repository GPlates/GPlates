//
// Geometry shader source code to render volume fill spherical caps.
//

// "#extension" needs to be specified in the shader source *string* where it is used (this is not
// documented in the GLSL spec but is mentioned at http://www.opengl.org/wiki/GLSL_Core_Language).
#extension GL_EXT_geometry_shader4 : enable

// The depth range rendering is restricted to.
// If depth range is not restricted then this is the same as 'min_max_depth_radius'.
// Also the following conditions hold:
//	render_min_max_depth_radius_restriction.x >= min_max_depth_radius.x
//	render_min_max_depth_radius_restriction.y <= min_max_depth_radius.y
// ...in other words the depth range for rendering is always within the actual depth range.
uniform vec2 render_min_max_depth_radius_restriction;

// (x,y) is (dot product threshold, num subdivisions)
uniform vec2 tessellate_params;

// The input primitive is a single segment (two vertices) of, for example, a GL_LINES draw call.
varying in vec4 polygon_centroid_point[2];

// The world-space coordinates are interpolated across the spherical cap geometry
// so that the fragment shader can use them to lookup the surface fill mask texture.
varying out vec3 world_position;

void
tessellate_on_sphere(
		float depth_radius,
		vec3 surface_line_start_point,
		vec3 surface_line_end_point,
		vec3 surface_polygon_centroid)
{
	vec3 depth_start_point = depth_radius * surface_line_start_point;
	vec3 depth_end_point = depth_radius * surface_line_end_point;
	vec3 depth_polygon_centroid = depth_radius * surface_polygon_centroid;

	// See if we can emit a triangle without tessellating it.
	float dot_start_to_centroid = dot(surface_line_start_point, surface_polygon_centroid);
	float dot_end_to_centroid = dot(surface_line_end_point, surface_polygon_centroid);
	if (dot_start_to_centroid > tessellate_params.x/*threshold*/ &&
		dot_end_to_centroid > tessellate_params.x/*threshold*/)
	{
		world_position = depth_start_point;
		gl_Position = gl_ModelViewProjectionMatrix * vec4(world_position, 1);
		EmitVertex();

		world_position = depth_end_point;
		gl_Position = gl_ModelViewProjectionMatrix * vec4(world_position, 1);
		EmitVertex();

		// Emit the final (polygon centroid) vertex.
		world_position = depth_polygon_centroid;
		gl_Position = gl_ModelViewProjectionMatrix * vec4(world_position, 1);
		EmitVertex();

		EndPrimitive();

		return;
	}

	// We need to ensure adjacent geometry shader triangles (generated from line segments)
	// have matching tessellated vertices otherwise cracks will form.
	// For now we'll just subdivide each edge a constant number of times based on the depth radius.
	// TODO: Replace this will adaptive subdivision based on edge arc lengths.

	// Emit the two line segment points first.

	world_position = depth_start_point;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(world_position, 1);
	EmitVertex();

	world_position = depth_end_point;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(world_position, 1);
	EmitVertex();

	int numsubdivisions = int(depth_radius * tessellate_params.y);
	if (numsubdivisions > 1)
	{
		float start_to_centroid_arc_angle = acos(dot_start_to_centroid);
		float end_to_centroid_arc_angle = acos(dot_end_to_centroid);

		vec3 start_to_centroid_rotation_axis =
				normalize(cross(surface_line_start_point, surface_polygon_centroid));
		vec3 end_to_centroid_rotation_axis =
				normalize(cross(surface_line_end_point, surface_polygon_centroid));

		vec4 start_to_centroid_quaternion = vec4(
				sin(0.5 * start_to_centroid_arc_angle) * start_to_centroid_rotation_axis,
				cos(0.5 * start_to_centroid_arc_angle));
		vec4 end_to_centroid_quaternion = vec4(
				sin(0.5 * end_to_centroid_arc_angle) * end_to_centroid_rotation_axis,
				cos(0.5 * end_to_centroid_arc_angle));

		vec3 start_to_centroid_world_position = depth_start_point;
		vec3 end_to_centroid_world_position = depth_end_point;
		for (int n = 1; n < numsubdivisions; n++)
		{
			// Rotate the start edge vertex to the next position.
			start_to_centroid_world_position = rotate_vector_by_quaternion(
					start_to_centroid_quaternion, start_to_centroid_world_position);
			// Rotate the end edge vertex to the next position.
			end_to_centroid_world_position = rotate_vector_by_quaternion(
					end_to_centroid_quaternion, end_to_centroid_world_position);

			// Emit start edge vertex.
			world_position = start_to_centroid_world_position;
			gl_Position = gl_ModelViewProjectionMatrix * vec4(world_position, 1);
			EmitVertex();
			// Emit end edge vertex.
			world_position = end_to_centroid_world_position;
			gl_Position = gl_ModelViewProjectionMatrix * vec4(world_position, 1);
			EmitVertex();
		}
	}

	// Emit the final (polygon centroid) vertex.
	world_position = depth_polygon_centroid;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(world_position, 1);
	EmitVertex();

	EndPrimitive();
}

void main (void)
{
	// The input primitive is a single segment (two vertices) of, for example, a GL_LINES draw call.

	tessellate_on_sphere(
			render_min_max_depth_radius_restriction.y/*max depth*/,
			gl_PositionIn[0].xyz,
			gl_PositionIn[1].xyz,
			// Both start and end vertices carry same centroid point so doesn't matter which one is accessed...
			polygon_centroid_point[0].xyz);

	// When generating the min/max depth range we also need to render the inner depth spherical caps.
	// This closes off the volume fill region making it watertight and ensuring any ray entering a front
	// surface the same ray will exit through another back surface.
#ifdef DEPTH_RANGE
	tessellate_on_sphere(
			render_min_max_depth_radius_restriction.x/*min depth*/,
			gl_PositionIn[0].xyz,
			gl_PositionIn[1].xyz,
			// Both start and end vertices carry same centroid point so doesn't matter which one is accessed...
			polygon_centroid_point[0].xyz);
#endif
}
