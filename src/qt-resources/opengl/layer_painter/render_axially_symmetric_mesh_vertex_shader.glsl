/* $Id: render_axially_symmetric_mesh_lighting_vertex_shader.glsl 14199 2013-05-27 09:18:05Z jcannon $ */

/**
 * \file 
 * $Revision: 14199 $
 * $Date: 2013-05-27 19:18:05 +1000 (Mon, 27 May 2013) $
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
// Vertex shader source code for rendering axially symmetric meshes.
//

uniform mat4 view_projection;

uniform bool lighting_enabled;

layout(location = 0) in vec3 world_space_position;
layout(location = 1) in vec4 colour;
layout(location = 2) in vec3 world_space_x_axis;
layout(location = 3) in vec3 world_space_y_axis;
layout(location = 4) in vec3 world_space_z_axis;
layout(location = 5) in vec2 model_space_radial_position;
layout(location = 6) in vec2 radial_and_axial_normal_weights;

out VertexData
{
	vec3 world_space_sphere_normal;
	vec4 colour;
	vec3 world_space_x_axis;
	vec3 world_space_y_axis;
	vec3 world_space_z_axis;
	// The model-space coordinates are interpolated across the geometry.
	vec2 model_space_radial_position;
	vec2 radial_and_axial_normal_weights;
} vs_out;

void main (void)
{
	gl_Position = view_projection * vec4(world_space_position, 1);

	// Output the vertex colour.
	// We render both sides (front and back) of triangles (ie, there's no back-face culling).
    vs_out.colour = colour;

	if (lighting_enabled)
	{
		// Pass to fragment shader.
		vs_out.world_space_sphere_normal = normalize(world_space_position);
		vs_out.world_space_x_axis = world_space_x_axis;
		vs_out.world_space_y_axis = world_space_y_axis;
		vs_out.world_space_z_axis = world_space_z_axis;
		vs_out.model_space_radial_position = model_space_radial_position;
		vs_out.radial_and_axial_normal_weights = radial_and_axial_normal_weights;
	}
}
