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
// Vertex shader source code to render reconstructed raster tiles to the scene.
//

uniform bool using_age_grid;
uniform bool using_surface_lighting;
uniform bool using_surface_lighting_in_map_view;
uniform bool using_surface_lighting_normal_map;
uniform bool using_surface_lighting_normal_map_with_no_directional_light;

uniform mat4 view_projection;

uniform mat4 source_raster_texture_transform;
uniform mat4 clip_texture_transform;
uniform mat4 age_grid_texture_transform;
uniform mat4 normal_map_texture_transform;

uniform vec4 plate_rotation_quaternion;
uniform vec3 world_space_light_direction_in_globe_view;

layout(location = 0) in vec4 position;

out vec4 source_raster_texture_coordinate;
out vec4 clip_texture_coordinate;
out vec4 age_grid_texture_coordinate;
out vec4 normal_map_texture_coordinate;

out vec3 world_space_sphere_normal;
out vec3 model_space_light_direction;

void main (void)
{
    gl_Position = view_projection * position;

	// Source raster cube map projection.
	source_raster_texture_coordinate = source_raster_texture_transform * position;

	// Clip texture cube map projection.
	clip_texture_coordinate = clip_texture_transform * position;

    if (using_age_grid)
    {
        // Age grid cube map projection.
        age_grid_texture_coordinate = age_grid_texture_transform * position;
    }

    if (using_surface_lighting)
    {
		if (using_surface_lighting_normal_map)
		{
            // Normal map raster cube map projection.
            normal_map_texture_coordinate = normal_map_texture_transform * position;

			if (using_surface_lighting_in_map_view)  // map view...
			{
                // World-space sphere normal - rotate vertex position to world-space.
                //
                // Used as the texture coordinate for the map view's light direction cube texture, or
                // used directly as a pseudo light direction.
                world_space_sphere_normal = rotate_vector_by_quaternion(plate_rotation_quaternion, position.xyz);
			}
			else // globe view...
			{
                // World-space sphere normal - rotate vertex position to world-space.
                world_space_sphere_normal = rotate_vector_by_quaternion(plate_rotation_quaternion, position.xyz);

                // It's more efficient for fragment shader to do lambert dot product in model-space
                // instead of world-space so reverse rotate light direction into model space.
                vec4 plate_reverse_rotation_quaternion =
                    vec4(-plate_rotation_quaternion.xyz, plate_rotation_quaternion.w);
                if (using_surface_lighting_normal_map_with_no_directional_light)
                {
                    // Use the radial sphere normal as the pseudo light direction.
                    model_space_light_direction = rotate_vector_by_quaternion(
                        plate_reverse_rotation_quaternion, world_space_sphere_normal);
                }
                else
                {
                    model_space_light_direction = rotate_vector_by_quaternion(
                        plate_reverse_rotation_quaternion, world_space_light_direction_in_globe_view);
                }
			}
		}
		else // not using normal map...
		{
			if (using_surface_lighting_in_map_view)  // map view...
			{
                // Nothing needed.
			}
			else // globe view...
			{
                // World-space sphere normal - rotate vertex position to world-space.
                world_space_sphere_normal = rotate_vector_by_quaternion(plate_rotation_quaternion, position.xyz);
			}
		}
    }
}
