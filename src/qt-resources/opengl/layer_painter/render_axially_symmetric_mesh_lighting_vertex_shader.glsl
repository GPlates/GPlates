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
// Vertex shader source code for lighting axially symmetric meshes.
//

attribute vec3 world_space_position_attribute;
attribute vec4 colour_attribute;
attribute vec3 world_space_x_axis_attribute;
attribute vec3 world_space_y_axis_attribute;
attribute vec3 world_space_z_axis_attribute;
attribute vec2 model_space_radial_position_attribute;
attribute vec2 radial_and_axial_normal_weights_attribute;

varying vec3 world_space_sphere_normal;
varying vec3 world_space_x_axis;
varying vec3 world_space_y_axis;
varying vec3 world_space_z_axis;
// The model-space coordinates are interpolated across the geometry.
varying vec2 model_space_radial_position;
varying vec2 radial_and_axial_normal_weights;

void main (void)
{
	gl_Position = gl_ModelViewProjectionMatrix * vec4(world_space_position_attribute, 1);

	// Output the vertex colour.
	// We render both sides (front and back) of triangles (ie, there's no back-face culling).
	gl_FrontColor = colour_attribute;
	gl_BackColor = colour_attribute;

	// Pass to fragment shader.
	world_space_sphere_normal = normalize(world_space_position_attribute);
	world_space_x_axis = world_space_x_axis_attribute;
	world_space_y_axis = world_space_y_axis_attribute;
	world_space_z_axis = world_space_z_axis_attribute;
	model_space_radial_position = model_space_radial_position_attribute;
	radial_and_axial_normal_weights = radial_and_axial_normal_weights_attribute;
}
