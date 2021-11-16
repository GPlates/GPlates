/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2021 The University of Sydney, Australia
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
// Geometry shader for rendering a vertical cross-section through a 3D scalar field.
//
// The cross-section is formed by extruding a surface polyline (or polygon) from the
// field's outer depth radius to its inner depth radius.
//

layout (std140) uniform ViewProjection
{
	// Combined view-projection transform, and inverse of view and projection transforms.
	mat4 view_projection;
	mat4 view_inverse;
	mat4 projection_inverse;

	// Viewport (used to convert window-space coordinates gl_FragCoord.xy to NDC space [-1,1]).
	vec4 viewport;
} view_projection_block;

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

layout (std140) uniform Lighting
{
	bool enabled;
	float ambient;
	vec3 world_space_light_direction;
} lighting_block;

#ifdef CROSS_SECTION_1D
	layout (points) in;
	layout (line_strip, max_vertices = 2) out;
#endif
#ifdef CROSS_SECTION_2D
	layout (lines) in;
	layout (triangle_strip, max_vertices = 4) out;
#endif

in VertexData
{
	vec3 surface_point;
} gs_in[];

out VertexData
{
	// The world-space coordinates are interpolated across the cross-section geometry.
	vec3 world_position;

	// The lighting for the front (x component) and back (y component) face.
	// Note that the face normal is constant across the (cross-section) face.
	vec2 front_and_back_face_lighting;
} gs_out;

void main()
{
	vec2 front_and_back_face_lighting;
	if (lighting_block.enabled)
	{
#ifdef CROSS_SECTION_1D
		// We can't calculate a normal for a 1D cross-section.
		// Instead we see if the vertical line (1D cross-section) is perpendicular to
		// the light direction giving it a maximum amount of diffuse lighting.
		// If the vertical line is in direction (or opposite) of light then it only gets ambient light.
		// This is the same lighting you'd get if the vertical line was a very thin cylindrical pipe
		// but you could always see the brightest side of it.

		// The front and back face lighting are the same.
		float front_and_back_face_diffuse_lighting = length(cross(gs_in[0].surface_point, lighting_block.world_space_light_direction));

		// Blend between ambient and diffuse lighting - when ambient is 1.0 there is no diffuse.
		front_and_back_face_lighting = vec2(mix(front_and_back_face_diffuse_lighting, 1.0, lighting_block.ambient));
#endif

#ifdef CROSS_SECTION_2D
		// Compute lighting for both the front and back face of the current cross-section quad (two coplanar triangles).
		// The fragment shader will choose between them based on what side the fragment is on (using 'gl_FrontFacing').

		// Calculate the front face normal.
		//
		// Note that it might be possible to get a zero normal vector - if this happens
		// then the cross-section quad (two tris) will likely be close to degenerate and
		// not generate any fragments anyway (so the incorrect lighting will not be visible).
		//
		// The direction of the front face is determined by counter-clockwise ordering of vertices in the quad (triangle strip)
		// since we're using the default glFrontFace of GL_CCW.
		vec3 front_face_normal = cross(gs_in[1].surface_point, gs_in[0].surface_point);

		// The Lambert term in diffuse lighting.
		// Note that the back face normal is negative of the front face normal.
		vec2 front_and_back_face_diffuse_lighting = vec2(
			lambert_diffuse_lighting(lighting_block.world_space_light_direction, front_face_normal),
			lambert_diffuse_lighting(lighting_block.world_space_light_direction, -front_face_normal));

		// Blend between ambient and diffuse lighting - when ambient is 1.0 there is no diffuse.
		front_and_back_face_lighting = mix(front_and_back_face_diffuse_lighting, vec2(1.0), vec2(lighting_block.ambient));
#endif
	}

#ifdef CROSS_SECTION_1D
	//
	// Vertically extrude the sole surface point to both the minimum or maximum depth restricted radius of the scalar field.
	//
	// And emit a single line strip of 2 vertices.
	//
	for (int depth = 0; depth < 2; ++depth)
	{
		// This assumes the cross-section geometry does not need a model transform (eg, reconstruction rotation).
		// This is the case for reconstructed geometries and resolved topologies.
		vec3 depth_point = depth_block.min_max_depth_radius_restriction[depth] * gs_in[0].surface_point;

		gl_Position = view_projection_block.view_projection * vec4(depth_point, 1);
		gs_out.world_position = depth_point;
		if (lighting_block.enabled)
		{
			gs_out.front_and_back_face_lighting = front_and_back_face_lighting;
		}
		
		EmitVertex();
	}

	EndPrimitive();
#endif

#ifdef CROSS_SECTION_2D
	//
	// Vertically extrude both surface points (of input line) to both the minimum or maximum depth restricted radius of the scalar field.
	//
	// And emit a single triangle strip of 4 vertices.
	//
	for (int vertex = 0; vertex < 2; ++vertex)
	{
		for (int depth = 0; depth < 2; ++depth)
		{
			// This assumes the cross-section geometry does not need a model transform (eg, reconstruction rotation).
			// This is the case for reconstructed geometries and resolved topologies.
			vec3 depth_point = depth_block.min_max_depth_radius_restriction[depth] * gs_in[vertex].surface_point;

			gl_Position = view_projection_block.view_projection * vec4(depth_point, 1);
			gs_out.world_position = depth_point;
			if (lighting_block.enabled)
			{
				gs_out.front_and_back_face_lighting = front_and_back_face_lighting;
			}
			
			EmitVertex();
		}
	}

	EndPrimitive();
#endif
}
