/**
 * Copyright (C) 2022 The University of Sydney, Australia
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
// Renders arrows.
//

#include "utils/utils.glsl"

layout (push_constant) uniform PushConstants
{
    mat4 view_projection;
    vec3 world_space_light_direction;
    bool lighting_enabled;
    float light_ambient_contribution;
};

const uint SCENE_TILE_DESCRIPTOR_SET = 0;
const uint SCENE_TILE_DESCRIPTOR_BINDING = 0;
const uint SCENE_TILE_DIMENSION_CONSTANT_ID = 0;
const uint SCENE_TILE_NUM_FRAGMENTS_IN_STORAGE_CONSTANT_ID = 1;
const uint SCENE_TILE_MAX_FRAGMENTS_PER_SAMPLE_CONSTANT_ID = 2;
const uint SCENE_TILE_SAMPLE_COUNT_CONSTANT_ID = 3;
#include "utils/scene_tile.glsl"

layout (location = 0) in VertexData
{
	vec3 world_space_sphere_normal;
	vec4 colour;
	vec3 world_space_x_axis;
	vec3 world_space_y_axis;
	vec3 world_space_z_axis;
	// The model-space coordinates are interpolated across the geometry.
	vec2 model_space_radial_position;
	vec2 model_space_radial_and_axial_normal_weights;
} fs_in;

void main (void)
{
	vec4 colour;

	if (lighting_enabled)
	{
		// Calculate the model-space normal of the arrow mesh at the current fragment location.
		// The arrow mesh, in model-space, is axially symmetric about its vector direction (z-axis) so we blend the radial (x,y) normal with the z-axis.
		vec3 model_space_mesh_normal = vec3(
				fs_in.model_space_radial_and_axial_normal_weights.x * normalize(fs_in.model_space_radial_position),
				fs_in.model_space_radial_and_axial_normal_weights.y);
		
		// Convert the model-space normal to world-space (the same space the light direction is in).
		vec3 world_space_mesh_normal = mat3(fs_in.world_space_x_axis, fs_in.world_space_y_axis, fs_in.world_space_z_axis) * model_space_mesh_normal;
		// Reverse the world-space normal if this fragment belongs to a back-facing mesh triangle.
		// A back face can be visible if the arrow is semi-transparent.
		if (!gl_FrontFacing)
		{
			world_space_mesh_normal = -world_space_mesh_normal;
		}
		
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

	// Add this fragment to the per-pixel fragment list (sorted by depth) for the current fragment coordinate.
	// It will later get blended (in per-pixel depth order) into the framebuffer.
	//
	// Note: We don't have the usual "layout (location = 0) out vec4 colour;" declaration because
	//       we're not writing to the framebuffer and so our colour attachment output will be undefined
	//       which is why we also mask colour attachment writes (using the Vulkan API).
	scene_tile_add_fragment(colour, gl_FragCoord.z);
}
