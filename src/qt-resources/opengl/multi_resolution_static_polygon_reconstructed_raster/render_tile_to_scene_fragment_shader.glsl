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

uniform sampler2D source_texture_sampler;
uniform sampler2D clip_texture_sampler;
varying vec4 source_raster_texture_coordinate;
varying vec4 clip_texture_coordinate;

#ifdef USING_AGE_GRID
	uniform sampler2D age_grid_texture_sampler;
	uniform vec4 age_grid_texture_dimensions;
	#ifdef GENERATE_AGE_MASK
		uniform float reconstruction_time;
	#endif
	varying vec4 age_grid_texture_coordinate;
#endif

#ifdef SOURCE_RASTER_IS_FLOATING_POINT
	uniform vec4 source_texture_dimensions;
#endif

#ifdef SURFACE_LIGHTING
    #if defined(MAP_VIEW) && !defined(USING_NORMAL_MAP)
        // In the map view the lighting is constant across the map because
        // the surface normal is constant across the map (ie, it's flat unlike the 3D globe).
        uniform float ambient_and_diffuse_lighting;
    #else // all other cases we must calculate lighting...
        uniform float light_ambient_contribution;
        varying vec3 world_space_sphere_normal;
        #ifdef USING_NORMAL_MAP
            uniform sampler2D normal_map_texture_sampler;
            varying vec4 normal_map_texture_coordinate;
            #ifdef MAP_VIEW
                uniform vec4 plate_rotation_quaternion;
				#ifndef NO_DIRECTIONAL_LIGHT_FOR_NORMAL_MAPS
					uniform samplerCube light_direction_cube_texture_sampler;
				#endif
            #else
                varying vec3 model_space_light_direction;
				#ifndef NO_DIRECTIONAL_LIGHT_FOR_NORMAL_MAPS
					uniform vec3 world_space_light_direction;
				#endif
            #endif
        #else // globe view with no normal map...
            uniform vec3 world_space_light_direction;
        #endif
    #endif
#endif

void main (void)
{
	// Discard the pixel if it has been clipped by the clip texture as an
	// early rejection test since a lot of pixels will be outside the tile region.
	float clip_mask = texture2DProj(clip_texture_sampler, clip_texture_coordinate).a;
	if (clip_mask == 0)
		discard;

#ifdef USING_AGE_GRID
	#ifdef GENERATE_AGE_MASK
		// Do the texture transform projective divide.
		vec2 age_grid_texture_coords = age_grid_texture_coordinate.st / age_grid_texture_coordinate.q;
		vec4 age11, age21, age12, age22;
		vec2 age_interp;
		// Retrieve the 2x2 samples in the age grid texture and the bilinear interpolation coefficient.
		// The texture access in 'bilinearly_interpolate' starts a new indirection phase.
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
	#else
		// Get the pre-generated age mask from the age grid texture.
		vec4 age_mask_and_coverage = 
			texture2DProj(age_grid_texture_sampler, age_grid_texture_coordinate);
		float age_coverage = age_mask_and_coverage.a;
		float age_coverage_mask = age_mask_and_coverage.r * age_coverage;
	#endif

	#ifdef ACTIVE_POLYGONS
		// Choose between active polygon and age mask based on age coverage.
		float age_factor = (1 - age_coverage) + age_coverage_mask;
	#else
		// Age factor is zero outside valid (oceanic) regions of the age grid.
		float age_factor = age_coverage_mask;
	#endif

	// Early reject if the age factor is zero.
	if (age_factor == 0)
		discard;
#endif

#ifdef SOURCE_RASTER_IS_FLOATING_POINT
	// Do the texture transform projective divide.
	vec2 source_texture_coords = source_raster_texture_coordinate.st / source_raster_texture_coordinate.q;
	// Bilinearly filter the tile texture (data/coverage is in red/green channel).
	// The texture access in 'bilinearly_interpolate' starts a new indirection phase.
	vec4 tile_colour = bilinearly_interpolate_data_coverge_RG(
		 source_texture_sampler, source_texture_coords, source_texture_dimensions);
#else
	// Use hardware bilinear interpolation of fixed-point texture.
	vec4 tile_colour = texture2DProj(source_texture_sampler, source_raster_texture_coordinate);
#endif

#ifdef USING_AGE_GRID
	// Modulate the source texture's coverage with the age test result.
	#ifdef SOURCE_RASTER_IS_FLOATING_POINT
		// NOTE: For floating-point textures we've placed the coverage/alpha in the green channel.
		// And they don't have pre-multiplied alpha like RGBA fixed-point textures because
		// alpha-blending not supported for floating-point render targets.
		tile_colour.g *= age_factor;
	#else
		// NOTE: All RGBA raster data has pre-multiplied alpha (for alpha-blending to render targets)
		// The source raster already has pre-multiplied alpha (see GLVisualRasterSource).
		// However we still need to pre-multiply the age factor (alpha).
		// (RGB * A, A) -> (RGB * A * age_factor, A * age_factor).
		tile_colour *= age_factor;
	#endif
#endif

	// Reject the pixel if its coverage/alpha is zero - this is because
	// it might be an age masked tile - zero coverage, in this case, means don't draw
	// because the age test failed - if we draw then we'd overwrite valid data.
#ifdef SOURCE_RASTER_IS_FLOATING_POINT
	// NOTE: For floating-point textures we've placed the coverage/alpha in the green channel.
	if (tile_colour.g == 0)
		discard;
#else
	if (tile_colour.a == 0)
		discard;
#endif

	// Surface lighting only needs to be applied to visualised raster sources.
	// This excludes floating-point sources (GLDataRasterSource) which are used for analysis
	// only and hence do not need lighting (and should not have lighting applied).
#ifdef SURFACE_LIGHTING
    #if defined(MAP_VIEW) && !defined(USING_NORMAL_MAP)
        // The ambient+diffuse lighting is pre-calculated (constant) for the map view with no normal maps.
        float lighting = ambient_and_diffuse_lighting;
    #else // all other cases we must calculate lighting...
        #ifdef USING_NORMAL_MAP
            // Sample the model-space normal in the normal map raster.
            vec3 model_space_normal =
                texture2DProj(normal_map_texture_sampler, normal_map_texture_coordinate).xyz;
            // Need to convert the x/y/z components from unsigned to signed ([0,1] -> [-1,1]).
            model_space_normal = 2 * model_space_normal - 1;
            #ifdef MAP_VIEW
				#ifdef NO_DIRECTIONAL_LIGHT_FOR_NORMAL_MAPS
					vec3 world_space_light_direction = world_space_sphere_normal;
				#else
					// Get the world-space light direction from the light direction cube texture.
					vec3 world_space_light_direction =
							textureCube(light_direction_cube_texture_sampler, world_space_sphere_normal).xyz;
					// Need to convert the x/y/z components from unsigned to signed ([0,1] -> [-1,1]).
					world_space_light_direction = 2 * world_space_light_direction - 1;
				#endif
                // Do the lambert dot product in world-space.
                // Convert to world-space normal where light direction is.
                vec3 world_space_normal = rotate_vector_by_quaternion(plate_rotation_quaternion, model_space_normal);
                vec3 normal = world_space_normal;
                vec3 light_direction = world_space_light_direction;
            #else
                // Do the lambert dot product in *model-space* (not world-space) since more efficient.
                vec3 normal = model_space_normal;
                vec3 light_direction = model_space_light_direction;
            #endif
        #else // globe view with no normal map...
            // Do the lambert dot product in world-space.
            vec3 normal = world_space_sphere_normal;
			// The light direction is already in world-space.
			vec3 light_direction = world_space_light_direction;
        #endif

        // Apply the Lambert diffuse lighting to the tile colour.
        // Note that neither the light direction nor the surface normal need be normalised.
        float lambert = lambert_diffuse_lighting(light_direction, normal);

        #if defined(USING_NORMAL_MAP) && !defined(MAP_VIEW)
            // Need to clamp the lambert term to zero when the (unperturbed) sphere normal
            // faces away from the light direction otherwise the perturbed surface will appear
            // to be lit *through* the globe (ie, not shadowed by the globe).
            // NOTE: This is not necessary for the 2D map views.
            // The factor of 8 gives a linear falloff from 1.0 to 0.0 when dot product is below 1/8.
            // NOTE: The normal and light direction are not normalized since accuracy is less important here.
            // NOTE: Using float instead of integer parameters to 'clamp' otherwise driver compiler
            // crashes on some systems complaining cannot find (integer overload of) function in 'stdlib'.
			#ifndef NO_DIRECTIONAL_LIGHT_FOR_NORMAL_MAPS
				lambert *= clamp(8 * dot(world_space_sphere_normal, world_space_light_direction), 0.0, 1.0);
			#endif
        #endif

        // Blend between ambient and diffuse lighting.
        float lighting = mix_ambient_with_diffuse_lighting(lambert, light_ambient_contribution);
    #endif

	tile_colour.rgb *= lighting;
#endif

	// The final fragment colour.
	gl_FragColor = tile_colour;
}
