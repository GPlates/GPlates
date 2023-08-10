/* $Id: render_axially_symmetric_mesh_lighting_fragment_shader.glsl 14199 2013-05-27 09:18:05Z jcannon $ */

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
// Fragment shader source code for rendering axially symmetric meshes.
//

uniform bool lighting_enabled;
uniform float light_ambient_contribution;
uniform vec3 world_space_light_direction;

in VertexData
{
	vec3 world_space_sphere_normal;
	vec4 colour;
	vec3 world_space_x_axis;
	vec3 world_space_y_axis;
	vec3 world_space_z_axis;
	// The model-space coordinates are interpolated across the geometry.
	vec2 model_space_radial_position;
	vec2 radial_and_axial_normal_weights;
} fs_in;

layout(location = 0) out vec4 colour;

void main (void)
{
	if (lighting_enabled)
	{
		// Calculate the model-space normal of the axially symmetric mesh at the current fragment location.
		// The mesh, in model-space, is axially symmetric about its z-axis so we blend the radial (x,y) normal with the z-axis.
		vec3 model_space_mesh_normal = vec3(
				fs_in.radial_and_axial_normal_weights.x * normalize(fs_in.model_space_radial_position),
				fs_in.radial_and_axial_normal_weights.y);
		
		// Convert the model-space normal to world-space (the same space the light direction is in).
		vec3 world_space_mesh_normal = mat3(fs_in.world_space_x_axis, fs_in.world_space_y_axis, fs_in.world_space_z_axis) * model_space_mesh_normal;
		
		// Apply the Lambert diffuse lighting using the world-space normal.
		// Note that neither the light direction nor the surface normal need be normalised.
		float lambert = lambert_diffuse_lighting(world_space_light_direction, world_space_mesh_normal);
		
		// Need to clamp the lambert term to zero when the (unperturbed) sphere normal
		// faces away from the light direction otherwise the mesh surface will appear
		// to be lit *through* the globe (ie, not shadowed by the globe).
		// NOTE: This is not necessary for the 2D map views (but currently we only render axially symmetric meshes in 3D globe views.
		// The factor of 8 gives a linear falloff from 1.0 to a lower bound when dot product is below 1/8.
		// We use a non-zero lower bound so that the mesh retains some diffuse lighting in shadow to help visualise its shape better.
		// NOTE: Using float instead of integer parameters to 'clamp' otherwise driver compiler
		// crashes on some systems complaining cannot find (integer overload of) function in 'stdlib'.
		lambert *= clamp(8 * dot(world_space_light_direction, fs_in.world_space_sphere_normal), 0.3, 1.0);

		// Blend between ambient and diffuse lighting.
		float ambient_and_diffuse_lighting = mix_ambient_with_diffuse_lighting(lambert, light_ambient_contribution);
		
		// The final fragment colour.
		colour = vec4(fs_in.colour.rgb * ambient_and_diffuse_lighting, fs_in.colour.a);
	}
	else
	{
		// Simply return the vertex colour.
		colour = fs_in.colour;
	}
}