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
// Geometry shader source code to render fill walls (vertically extruded quads).
//
// This shader supports two paths by defining either SURFACE_NORMAL_AND_DEPTH or DEPTH_RANGE:
//
//   SURFACE_NORMAL_AND_DEPTH - Render the walls of the extruded fill volume as surface normal and depth.
//                              The isosurface program will render opaque (visible) walls.
//
//   DEPTH_RANGE              - Render the min/max depth range of the walls.
//                              The isosurface program will reduce the length along each ray that is sampled/traversed.
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

layout (lines) in;
layout (triangle_strip, max_vertices = 4) out;

in VertexData
{
	vec3 surface_point;
} gs_in[];

#ifdef SURFACE_NORMAL_AND_DEPTH
	out VertexData
	{
		// The world-space coordinates are interpolated across the wall geometry
		// so that the fragment shader can use them to lookup the surface fill mask texture.
		out vec3 world_position;

		// The surface normal for the front-face of the wall.
		out vec3 front_surface_normal;
	} gs_out;
#endif

void main (void)
{
	// The input primitive is a single segment (two vertices) of a GL_LINES.

#ifdef SURFACE_NORMAL_AND_DEPTH
	// Calculate the front face normal.
	//
	// The direction of the front face is determined by counter-clockwise ordering of vertices in the quad (triangle strip)
	// since we're using the default glFrontFace of GL_CCW.
	//
	// Note that it might be possible to get a zero (or undefined due to 'normalize') normal vector if the
	// line segment points are too close together.
	// If this happens then the wall quad (two tris) will likely be close to degenerate and not generate any fragments anyway.
	// TODO: It might be better to detect this and not emit primitives.
	vec3 surface_normal = normalize(cross(gs_in[1].surface_point, gs_in[0].surface_point));
#endif

	//
	// Vertically extrude both surface points (of input line) to both the minimum or maximum depth restricted radius of the scalar field.
	//
	// And emit a single triangle strip of 4 vertices.
	//
	for (int vertex = 0; vertex < 2; ++vertex)
	{
		for (int depth = 0; depth < 2; ++depth)
		{
			vec3 depth_point = depth_block.min_max_depth_radius_restriction[depth] * gs_in[vertex].surface_point;
#ifdef DEPTH_RANGE
			// Artificially reduce the *min* depth to avoid artifacts due to discarded iso-surface rays
			// when a wall is perpendicular to the ray - in this case the finite tessellation of the
			// inner sphere leaves thin cracks of pixels adjacent to the wall where no depth range is
			// recorded - and at these pixels the ray's min and max depth can become equal.
			// By setting reducing the min depth we extrude the wall further in order to cover these cracks.
			if (depth == 0)
			{
				depth_point = 0.9 * depth_point;
			}
#endif

			gl_Position = view_projection_block.view_projection * vec4(depth_point, 1);

#ifdef SURFACE_NORMAL_AND_DEPTH
			gs_out.world_position = depth_point;
			gs_out.front_surface_normal = surface_normal;
#endif
			
			EmitVertex();
		}
	}

	EndPrimitive();
}
