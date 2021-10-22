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
// Fragment shader source code to render reconstructed raster tiles to the scene.
//
// NOTE: If the source raster is floating-point it will have its filter set to 'nearest' filtering
// (due to earlier hardware lack of support) so we need to emulate bilinear filtering in the fragment shader.
//

uniform bool using_data_raster_for_source;
uniform bool using_age_grid;
uniform bool using_age_grid_active_polygons;
uniform bool using_surface_lighting;
uniform bool using_surface_lighting_in_map_view;
uniform bool using_surface_lighting_normal_map;
uniform bool using_surface_lighting_normal_map_with_no_directional_light;

uniform sampler2D source_texture_sampler;
uniform sampler2D clip_texture_sampler;
uniform sampler2D normal_map_texture_sampler;
uniform sampler2D age_grid_texture_sampler;
uniform samplerCube light_direction_cube_texture_sampler_in_map_view_with_normal_map;

uniform vec4 age_grid_texture_dimensions;
uniform vec4 source_texture_dimensions;

uniform float reconstruction_time;
uniform vec4 plate_rotation_quaternion;

uniform float ambient_lighting;
uniform float ambient_and_diffuse_lighting_in_map_view_with_no_normal_map;
uniform vec3 world_space_light_direction_in_globe_view;

in vec4 source_texture_coordinate;
in vec4 clip_texture_coordinate;
in vec4 age_grid_texture_coordinate;
in vec4 normal_map_texture_coordinate;

in vec3 world_space_sphere_normal;
in vec3 model_space_light_direction;

layout(location = 0) out vec4 colour;

void main (void)
{
	// Discard the pixel if it has been clipped by the clip texture as an
	// early rejection test since a lot of pixels will be outside the tile region.
	float clip_mask = textureProj(clip_texture_sampler, clip_texture_coordinate).a;
	if (clip_mask == 0)
	{
		discard;
	}

	// Result of age grid test (defaults to 1.0 if not using age grid).
	float age_factor = 1.0;

	if (using_age_grid)
	{
		// Do the texture transform projective divide.
		vec2 age_grid_texture_coords = age_grid_texture_coordinate.st / age_grid_texture_coordinate.q;
		vec4 age11, age21, age12, age22;
		vec2 age_interp;
		// Retrieve the 2x2 samples in the age grid texture and the bilinear interpolation coefficient.
		bilinearly_interpolate(
			age_grid_texture_sampler,
			age_grid_texture_coords,
			age_grid_texture_dimensions,
			age11, age21, age12, age22, age_interp);
		// The red channel contains the age value and the green channel contains the coverage.
		vec4 age_vec = vec4(age11.r, age21.r, age12.r, age22.r);
		vec4 age_coverage_vec = vec4(age11.g, age21.g, age12.g, age22.g);
		// Do the age comparison test with the current reconstruction time.
		vec4 age_test = step(reconstruction_time, age_vec);
		// Multiply the coverage values into the age tests.
		vec4 cov_age_test = age_test * age_coverage_vec;
		// Bilinearly interpolate the four age test results and coverage.
		float age_coverage_mask = mix(
			mix(cov_age_test.x, cov_age_test.y, age_interp.x),
			mix(cov_age_test.z, cov_age_test.w, age_interp.x), age_interp.y);
		float age_coverage = mix(
			mix(age_coverage_vec.x, age_coverage_vec.y, age_interp.x),
			mix(age_coverage_vec.z, age_coverage_vec.w, age_interp.x), age_interp.y);

		// For continental crust (where there's no age grid coverage) the age is determined by the active polygon time of appearance.
		// For oceanic crust (where there is age grid coverage) the age is determined by the age grid
		if (using_age_grid_active_polygons)
		{
			// Choose between active polygon and age mask based on age coverage.
			//
			// This is essentially '(1 - age_coverage) + age_coverage * (age >= reconstruction_time ? 1 : 0)'.
			// Which is '1' when 'age >= reconstruction_time' (for both oceanic and continental crust) and
			// '1 - age_coverage' when 'age < reconstruction_time' (which is '0' for oceanic crust and '1' for continental crust).
			//
			// So this is always '1' for continental crust.
			// For oceanic crust, it is '1' when 'age >= reconstruction_time' and '0' when 'age < reconstruction_time'.
			//
			// Note that continental crust is always '1' here, however we are rendering with *active* polygons and so they
			// will disappear when the reconstruction time is earlier than the continental polygon's time of appearance
			// (not that continental polygons should ever disappear, but they do have finite begin times, such as 600Ma).
			age_factor = (1 - age_coverage) + age_coverage_mask;
		}
		else // inactive polygons (where polygon time of appearance is after/younger than reconstruction time)...
		{
			// Age factor is zero outside valid (oceanic) regions of the age grid - we only need inactive polygons for oceanic crust (where age grid is).
			//
			// This is essentially 'age_coverage * (age >= reconstruction_time ? 1 : 0)'.
			//
			// Which is always '0' for continental crust.
			// For oceanic crust, it is '1' when 'age >= reconstruction_time' and '0' when 'age < reconstruction_time'.
			age_factor = age_coverage_mask;
		}

		// Early reject if the age factor is zero (because the age test failed and so we don't want to overwrite valid data).
		if (age_factor == 0)
		{
			discard;
		}
	}

	vec4 tile_colour;
	
	if (using_data_raster_for_source)
	{
		// Do the texture transform projective divide.
		vec2 source_texture_coords = source_texture_coordinate.st / source_texture_coordinate.q;
		// Bilinearly filter the tile texture (data/coverage is in red/green channel).
		// The texture access in 'bilinearly_interpolate' starts a new indirection phase.
		tile_colour = bilinearly_interpolate_data_coverage_RG(
			source_texture_sampler, source_texture_coords, source_texture_dimensions);

		// Modulate the source texture's coverage with the age test result (whic is 1.0 if not using age grid).
		//
		// NOTE: For data textures we've placed the coverage/alpha in the green channel.
		// And they don't have pre-multiplied alpha like RGBA fixed-point textures because
		// alpha-blending not supported for floating-point render targets.
		tile_colour.g *= age_factor;

		// Reject the pixel if its coverage/alpha is zero.
		//
		// NOTE: For data textures we've placed the coverage/alpha in the green channel.
		if (tile_colour.g == 0)
		{
			discard;
		}
	}
	else
	{
		// Use hardware bilinear interpolation of fixed-point texture.
		tile_colour = textureProj(source_texture_sampler, source_texture_coordinate);

		// Modulate the source texture's coverage with the age test result (whic is 1.0 if not using age grid).
		//
		// NOTE: All RGBA raster data has pre-multiplied alpha (for alpha-blending to render targets)
		// The source raster already has pre-multiplied alpha (see GLVisualRasterSource).
		// However we still need to pre-multiply the age factor (alpha).
		// (RGB * A, A) -> (RGB * A * age_factor, A * age_factor).
		tile_colour *= age_factor;

		// Reject the pixel if its coverage/alpha is zero.
		if (tile_colour.a == 0)
		{
			discard;
		}
	}

	// Surface lighting only needs to be applied to visualised raster sources.
	// This excludes floating-point sources (GLDataRasterSource) which are used for analysis
	// only and hence do not need lighting (and should not have lighting applied).
	if (using_surface_lighting)
	{
		float lighting;

		if (using_surface_lighting_normal_map)
		{
			if (using_surface_lighting_in_map_view)  // map view...
			{
				// Sample the model-space normal in the normal map raster.
				vec3 model_space_normal = textureProj(normal_map_texture_sampler, normal_map_texture_coordinate).xyz;
				// Need to convert the x/y/z components from unsigned to signed ([0,1] -> [-1,1]).
				model_space_normal = 2 * model_space_normal - 1;

				vec3 world_space_light_direction_in_map_view;
				if (using_surface_lighting_normal_map_with_no_directional_light)
				{
					// Use the radial sphere normal as the pseudo light direction.
					world_space_light_direction_in_map_view = world_space_sphere_normal;
				}
				else
				{
					// Get the world-space light direction from the light direction cube texture.
					world_space_light_direction_in_map_view = textureCube(
							light_direction_cube_texture_sampler_in_map_view_with_normal_map,
							world_space_sphere_normal).xyz;
					// Need to convert the x/y/z components from unsigned to signed ([0,1] -> [-1,1]).
					world_space_light_direction_in_map_view = 2 * world_space_light_direction_in_map_view - 1;
				}

				// Do the lambert dot product in world-space.
				// Convert to world-space normal where light direction is.
				vec3 world_space_normal = rotate_vector_by_quaternion(plate_rotation_quaternion, model_space_normal);
				vec3 normal = world_space_normal;
				vec3 light_direction = world_space_light_direction_in_map_view;

				// The Lambert term in the diffuse lighting equation.
				// Note that neither the light direction nor the surface normal need be normalised.
				float diffuse_lighting = lambert_diffuse_lighting(light_direction, normal);
				// Blend between ambient and diffuse lighting.
				lighting = mix_ambient_with_diffuse_lighting(diffuse_lighting, ambient_lighting);
			}
			else // globe view...
			{
				// Sample the model-space normal in the normal map raster.
				vec3 model_space_normal = textureProj(normal_map_texture_sampler, normal_map_texture_coordinate).xyz;
				// Need to convert the x/y/z components from unsigned to signed ([0,1] -> [-1,1]).
				model_space_normal = 2 * model_space_normal - 1;

				// Do the lambert dot product in *model-space* (not world-space) since more efficient.
				vec3 normal = model_space_normal;
				vec3 light_direction = model_space_light_direction;

				// The Lambert term in the diffuse lighting equation.
				// Note that neither the light direction nor the surface normal need be normalised.
				float diffuse_lighting = lambert_diffuse_lighting(light_direction, normal);

				if (!using_surface_lighting_normal_map_with_no_directional_light)
				{
					// We're using a directional light (rather than the radial sphere normal as the pseudo light direction).
					//
					// So we need to clamp the lambert diffuse lighting to zero when the (unperturbed) sphere normal
					// faces away from the light direction otherwise the perturbed surface will appear
					// to be lit *through* the globe (ie, not shadowed by the globe).
					// NOTE: This is not necessary for the 2D map views.
					// The factor of 8 gives a linear falloff from 1.0 to 0.0 when dot product is below 1/8.
					// NOTE: The normal and light direction are not normalized since accuracy is less important here.
					// NOTE: Using float instead of integer parameters to 'clamp' otherwise driver compiler
					// crashes on some systems complaining cannot find (integer overload of) function in 'stdlib'.
					diffuse_lighting *= clamp(8 * dot(world_space_sphere_normal, world_space_light_direction_in_globe_view), 0.0, 1.0);
				}

				// Blend between ambient and diffuse lighting.
				lighting = mix_ambient_with_diffuse_lighting(diffuse_lighting, ambient_lighting);
			}
		}
		else // not using normal map...
		{
			if (using_surface_lighting_in_map_view)  // map view...
			{
				// The ambient+diffuse lighting is pre-calculated (constant) for the map view with no normal maps.
				//
				// In the map view the lighting is constant across the map because
				// the surface normal is constant across the map (ie, it's flat unlike the 3D globe).
				lighting = ambient_and_diffuse_lighting_in_map_view_with_no_normal_map;
			}
			else // globe view...
			{
				// Do the lambert dot product in world-space.
				vec3 normal = world_space_sphere_normal;
				// The light direction is already in world-space.
				vec3 light_direction = world_space_light_direction_in_globe_view;

				// The Lambert term in the diffuse lighting equation.
				// Note that neither the light direction nor the surface normal need be normalised.
				float diffuse_lighting = lambert_diffuse_lighting(light_direction, normal);
				// Blend between ambient and diffuse lighting.
				lighting = mix_ambient_with_diffuse_lighting(diffuse_lighting, ambient_lighting);
			}
		}

		tile_colour.rgb *= lighting;
	}

	// The final fragment colour.
	colour = tile_colour;
}
