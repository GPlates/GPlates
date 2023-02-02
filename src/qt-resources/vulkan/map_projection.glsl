/**
 * Copyright (C) 2023 The University of Sydney, Australia
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

#ifndef MAP_PROJECTION_GLSL
#define MAP_PROJECTION_GLSL

//
// Map project a position on the globe (unit sphere) and optionally a vector direction.
//
// Note: The C++ source code "MapProjectionImage.cc" initialises the 3 map projection images used here.
//

#include "utils.glsl"

/*
 * Get the longitude and latitude (in radians) from the specified position on the unit sphere.
 *
 * Returns false if the specified position is too close to either pole (North or South) to determine its longitude.
 *
 * Note: Returned longitude is in the range [-PI, PI] and is centred on the central meridian (such that it will return a longitude of zero).
 *       Returned latitude is in the range [-PI/2, PI/2].
 */
bool
get_map_projection_longitude_and_latitude(
        out float longitude,
        out float latitude,
        out float cos_latitude,
        vec3 position,
        float map_projection_central_meridian,
        const float PI,
        const float HALF_PI)
{
    // If the 'position' is too close to either pole (North or South) then we cannot determine its longitude (atan(0,0) is undefined).
    cos_latitude = length(position.xy);  // 'position' is a unit vector
    if (cos_latitude < 2e-7)
    {
        return false;
    }

    // Get latitude from position on sphere.
    latitude = asin(position.z);  // in range [-PI/2, PI/2]

    // Get longitude from position on sphere.
    longitude = atan(position.y, position.x);  // in range [-PI, PI]
    // Centre about the central meridian (so it has a longitude of zero).
    longitude -= map_projection_central_meridian;    // in range [-PI - central_meridian, PI - central_meridian]

    // Ensure longitude is in the range [-PI, PI] by:
    // wrapping [-PI - |central_meridian|, -PI] to [PI - |central_meridian|, PI] when central_meridian > 0, and
    // wrapping [PI, PI + |central_meridian|] to [-PI, -PI + |central_meridian|] when central_meridian < 0.
    if (longitude < -PI)  // central_meridian > 0
    {
        longitude += 2 * PI;
    }
    else if (longitude > PI)  // central_meridian < 0
    {
        longitude -= 2 * PI;
    }

    return true;
}

/*
 * Get the map projection texture gather parameters such as texture coordinates for 'textureGather()' and its bilinear weights.
 *
 * Note: Longitude should be in the range [-PI, PI] and latitude in the range [-PI/2, PI/2].
 */
void
get_map_projection_texture_gather_params(
        out vec2 map_projection_gather_texture_coord,
        out vec2 map_projection_bilinear_weight,
        ivec2 map_projection_image_size,
        float longitude,
        float latitude,
        const float PI,
        const float HALF_PI)
{
    // Map projection texture coordinate uses longitude and latitude.
    //
    // Convert longitude from [-PI, PI] to [0, 1].
    float longitude_normalized = (longitude + PI) / (2 * PI);
    // Convert latitude from [-PI/2, PI/2] to [0, 1].
    float latitude_normalized = (latitude + HALF_PI) / PI;

    // Adjust for half texel since boundary of [0, 1] range maps to the texel centres of the boundary texels.
    // Everything inside the interior is linearly interpolated.
    vec2 map_projection_uv = (map_projection_image_size - 1) * vec2(longitude_normalized, latitude_normalized) + 0.5;

    // We'll be using 'textureGather()' and doing our own bilinear filtering to get better accuracy than builtin texture bilinear filtering.
    // This is because builtin texture bilinear filtering typically has only 8 bits of accuracy to its bilinear weights.
    get_texture_gather_unnormalized_bilinear_params(
            map_projection_image_size,
            map_projection_uv,  // unnormalized texture coordinates
            map_projection_gather_texture_coord,
            map_projection_bilinear_weight);
}

/*
 * Implementation function for projecting a 3D position (on the unit sphere) onto the 2D map projection.
 */
void
map_project_position_impl(
        out vec2 map_position,
        ivec2 map_projection_image_size,
        vec2 map_projection_bilinear_weight,

        // Sampled from the first map projection texture...
        vec4             x_gather,
        vec4             y_gather,
        vec4 ddx_dlon_dlat_gather,
        vec4 ddy_dlon_dlat_gather,

        // Sampled from the second map projection texture...
        vec4 dx_dlon_gather,
        vec4 dx_dlat_gather,
        vec4 dy_dlon_gather,
        vec4 dy_dlat_gather,

        // Sampled from the third map projection texture...
        vec4 ddx_dlon_dlon_gather,
        vec4 ddx_dlat_dlat_gather,
        vec4 ddy_dlon_dlon_gather,
        vec4 ddy_dlat_dlat_gather,

        const float PI)
{
    // Size of a texel in (longitude, latitude) space.
    // The texel is square, so its width and height should be the same, but we'll calculate them both anyway (just in case different).
    vec2 lon_lat_texel_size = vec2(2 * PI, PI) / (map_projection_image_size - 1);
    // Get the delta (longitude, latitude) of the actual (longitude, latitude) from each of the texel centres.
    // Note that some deltas will be negative.
    vec2 dlon_dlat_gather[4] =
    {
        vec2(map_projection_bilinear_weight.x, (map_projection_bilinear_weight.y - 1)) * lon_lat_texel_size,
        vec2((map_projection_bilinear_weight.x - 1), (map_projection_bilinear_weight.y - 1)) * lon_lat_texel_size,
        vec2((map_projection_bilinear_weight.x - 1), map_projection_bilinear_weight.y) * lon_lat_texel_size,
        vec2(map_projection_bilinear_weight.x, map_projection_bilinear_weight.y) * lon_lat_texel_size
    };

    // Gather the map positions and Jacobian/Hessian matrices of the 4 texels of the bilinear filter and use them to
    // calculate 4 extrapolated map positions at the actual (longitude, latitude) position from each of the 4 texels.
    vec2 xy_interpolate_gather[4];
    for (int n = 0; n < 4; ++n)
    {
        // The gradients of the map projection 'x' and 'y' coordinates.
        // Note: These two gradients together are equivalent to the 2x2 Jacobian matrix.
        vec2 x_gradient_gather = vec2(dx_dlon_gather[n], dx_dlat_gather[n]);
        vec2 y_gradient_gather = vec2(dy_dlon_gather[n], dy_dlat_gather[n]);
        
        // The Hessian matrix (2nd order derivatives) of the map projection 'x' coordinate.
        mat2 x_hessian_gather = mat2(
                ddx_dlon_dlon_gather[n], ddx_dlon_dlat_gather[n],   // first column
                ddx_dlon_dlat_gather[n], ddx_dlat_dlat_gather[n]);  // second column
        
        // The Hessian matrix (2nd order derivatives) of the map projection 'y' coordinate.
        mat2 y_hessian_gather = mat2(
                ddy_dlon_dlon_gather[n], ddy_dlon_dlat_gather[n],   // first column
                ddy_dlon_dlat_gather[n], ddy_dlat_dlat_gather[n]);  // second column
        
        // Extrapolate from the current texel centre to the actual position (between the 4 texel centres).
        //
        // Each separate 'x' and 'y' extrapolation is equivalent to the 2nd-order Taylor series expansion:
        //
        //   f(x) = f(a) + (x - a) * Df(a) + 0.5 * (x - a) * D2f(a) * (x - a)
        //
        // ...where 'a' represents the texel centre and 'x' represents the actual position.
        //
        xy_interpolate_gather[n].x = x_gather[n] +
                dot(x_gradient_gather, dlon_dlat_gather[n]) +
                0.5 * dot(dlon_dlat_gather[n], x_hessian_gather * dlon_dlat_gather[n]);
        xy_interpolate_gather[n].y = y_gather[n] +
                dot(y_gradient_gather, dlon_dlat_gather[n]) +
                0.5 * dot(dlon_dlat_gather[n], y_hessian_gather * dlon_dlat_gather[n]);
    }

    // Bilinearly interpolate the 4 extrapolated map positions.
    // The bilinear weights will naturally favour texels whose centre (longitude, latitude) is closer to the actual (longitude, latitude).
    map_position = bilinearly_interpolate_gather(
            xy_interpolate_gather,
            map_projection_bilinear_weight);
}

/*
 * Project a 3D position (on the unit sphere) onto the 2D map projection (defined by the 3 specified textures).
 */
void
map_project_position(
        out vec2 map_position,
        vec3 position,
        float map_projection_central_meridian,
        sampler2D map_projection_samplers[3])
{
    const float PI = radians(180);
    const float HALF_PI = PI / 2.0;

    // Get longitude and latitude from 'position'.
    float longitude;
    float latitude;
    float cos_latitude;
    if (!get_map_projection_longitude_and_latitude(
            longitude, latitude, cos_latitude,
            position, map_projection_central_meridian,
            PI, HALF_PI))
    {
        // The position is too close to either pole (North or South) to determine its longitude.
        // In this case we will just use a longitude of zero (and snap the latitude to the pole).
        longitude = 0.0;
        latitude = (position.z > 0) ? HALF_PI : -HALF_PI;
        cos_latitude = 0.0;
    }

    // All map projection images are the same size (so we can choose any of them to determine dimensions).
    ivec2 map_projection_image_size = textureSize(map_projection_samplers[0], 0);

    // Get the map projection texture gather coordinates and bilinear weights.
    vec2 map_projection_gather_texture_coord;
    vec2 map_projection_bilinear_weight;
    get_map_projection_texture_gather_params(
            map_projection_gather_texture_coord, map_projection_bilinear_weight, map_projection_image_size,
            longitude, latitude,
            PI, HALF_PI);

    // Sample the first map projection texture.
    vec4             x_gather = textureGather(map_projection_samplers[0], map_projection_gather_texture_coord, 0);
    vec4             y_gather = textureGather(map_projection_samplers[0], map_projection_gather_texture_coord, 1);
    vec4 ddx_dlon_dlat_gather = textureGather(map_projection_samplers[0], map_projection_gather_texture_coord, 2);
    vec4 ddy_dlon_dlat_gather = textureGather(map_projection_samplers[0], map_projection_gather_texture_coord, 3);

    // Sample the second map projection texture.
    vec4 dx_dlon_gather = textureGather(map_projection_samplers[1], map_projection_gather_texture_coord, 0);
    vec4 dx_dlat_gather = textureGather(map_projection_samplers[1], map_projection_gather_texture_coord, 1);
    vec4 dy_dlon_gather = textureGather(map_projection_samplers[1], map_projection_gather_texture_coord, 2);
    vec4 dy_dlat_gather = textureGather(map_projection_samplers[1], map_projection_gather_texture_coord, 3);

    // Sample the third map projection texture.
    vec4 ddx_dlon_dlon_gather = textureGather(map_projection_samplers[2], map_projection_gather_texture_coord, 0);
    vec4 ddx_dlat_dlat_gather = textureGather(map_projection_samplers[2], map_projection_gather_texture_coord, 1);
    vec4 ddy_dlon_dlon_gather = textureGather(map_projection_samplers[2], map_projection_gather_texture_coord, 2);
    vec4 ddy_dlat_dlat_gather = textureGather(map_projection_samplers[2], map_projection_gather_texture_coord, 3);

    // Map project the position.
    map_project_position_impl(
            map_position, map_projection_image_size, map_projection_bilinear_weight,

            // Sampled from the first map projection texture...
            x_gather, y_gather,
            ddx_dlon_dlat_gather, ddy_dlon_dlat_gather,

            // Sampled from the second map projection texture...
            dx_dlon_gather, dx_dlat_gather,
            dy_dlon_gather, dy_dlat_gather,

            // Sampled from the third map projection texture...
            ddx_dlon_dlon_gather, ddx_dlat_dlat_gather,
            ddy_dlon_dlon_gather, ddy_dlat_dlat_gather,

            PI);
}

/*
 * Implementation function for projecting a 3D vector onto the 2D map projection.
 */
void
map_project_vector_impl(
        out vec3 map_vector,
        vec3 position,
        float cos_latitude,
        vec3 vector,
        vec2 map_projection_bilinear_weight,
        // Sampled from the second map projection texture...
        vec4 dx_dlon_gather,
        vec4 dx_dlat_gather,
        vec4 dy_dlon_gather,
        vec4 dy_dlat_gather)
{

    // Find the local East, North and down directions at 'position' on the unit sphere.
    vec3 local_east = normalize(cross(vec3(0,0,1)/*global North*/, position));  // should not get divide-by-zero (since not at either pole)
    vec3 local_north = cross(position, local_east);  // no need to normalize (since cross of two perpendicular unit vectors)
    vec3 local_down = -position;

    // Convert vector into local (East, North, down) frame of reference.
    vec3 local_vector = vec3(
            dot(vector, local_east),
            dot(vector, local_north),
            dot(vector, local_down));

    // Bilinearly interpolate the Jacobian matrix between the 4 texel centres.
    //
    // We could have used the Hessian matrices as 1st order derivatives (Hessian is derivative of the Jacobian) in order to
    // extrapolate the Jacobian matrix from each of the 4 texel centres, similar to how the map position is extrapolated.
    // But unlike the map position extrapolation we don't have a 2nd order derivative (ie, derivative of Hessian) to handle curvature,
    // and the curvature is what gets us better accuracy than no extrapolation at all. So the accuracy of no extrapolation
    // (ie, just simply using the Jacobian) is actually a bit better than using extrapolated results (ie, Jacobian linearly extrapolated using Hessian).
    // In either case (with or without extrapolation) we still bilinearly interpolate the results of the 4 texels.
    // And so, simply bilinearly interpolating the Jacobian (gradients) can be thought of as roughly following the gradient curvature
    // (eg, a direction vector will change direction as its start position moves from one texel centre towards an adjacent one)
    // which might explain why the results are better than linear extrapolation.
    //
    // For example, here are some error measurements between the actual and computed (in compute shader) map projected unit-vectors (directions) of 50,000 arrows.
    // There are two sets, the first is extrapolated (Hessian matrices used to extrapolate Jacobian) and the second is simply the Jacobian (ie, no extrapolation).
    // Both sets are bilinearly interpolated. And in each set, the order of map projections is Rectangular, Mercator, Mollweide and Robinson.
    //
    // Extrapolation (only first order):
    //   RMS vector error:  3.02999e-06 , Max vector error: 8.57958e-06
    //   RMS vector error:  0.000266945 , Max vector error: 0.00422571
    //   RMS vector error:  0.000178484 , Max vector error: 0.00555581
    //   RMS vector error:  0.0049463   , Max vector error: 0.0326202
    //
    // No extrapolation:
    //   RMS vector error:  3.00003e-06 , Max vector error: 4.97866e-06
    //   RMS vector error:  0.000262877 , Max vector error: 0.00415843
    //   RMS vector error:  0.000144097 , Max vector error: 0.00478129
    //   RMS vector error:  0.000170958 , Max vector error: 0.01313
    //
    float dx_dlon = bilinearly_interpolate_gather(dx_dlon_gather, map_projection_bilinear_weight);
    float dx_dlat = bilinearly_interpolate_gather(dx_dlat_gather, map_projection_bilinear_weight);
    float dy_dlon = bilinearly_interpolate_gather(dy_dlon_gather, map_projection_bilinear_weight);
    float dy_dlat = bilinearly_interpolate_gather(dy_dlat_gather, map_projection_bilinear_weight);
    mat2 jacobian_matrix = mat2(
            dx_dlon, dy_dlon,   // first column
            dx_dlat, dy_dlat);  // second column

    // Convert 2D local (East, North) vector to (longitude, latitude) since the Jacobian matrix contains
    // partial derivatives relative to (longitude, latitude).
    //
    // North requires no conversion since distance from South to North is PI radians which equals the distance of latitude (having range [-PI/2, PI/2]).
    // But East requires conversion (depending on the latitude) since the small circle distance of constant latitude has distance 2*PI*cos(latitude) radians
    // (whereas the longitude distance is always 2*PI, regardless of latitude, since longitude range is always [-PI, PI]).
    vec2 lon_lat_vector = vec2(local_vector.x / cos_latitude, local_vector.y);  // should not get divide-by-zero (since 'cos_latitude' is non-zero; position not at either pole)

	// Transform 2D vector direction according to map projection warping.
    vec2 map_vector_2D = jacobian_matrix * lon_lat_vector;

	// Make 2D vector the same length as before - we're effectively just rotating it.
    map_vector_2D = length(local_vector.xy) * normalize(map_vector_2D);

	// The final vector is in 3D map projection space (2D map projection space and vertical dimension).
    map_vector = vec3(map_vector_2D, local_vector.z);
}

/*
 * Project a 3D vector onto the 2D map projection (defined by the 3 specified textures).
 *
 * Returns false if the specified position is too close to either pole (North or South) to determine
 * the direction of the vector relative to the position's local East and North (since they are indeterminate).
 *
 * The final map projected vector is actually a 3D vector where the 2D (x,y) component is in the 2D map plane and
 * the z component is along the z-axis of 3D map projection space.
 */
bool
map_project_vector(
        out vec3 map_vector,
        vec3 position,
        vec3 vector,
        float map_projection_central_meridian,
        sampler2D map_projection_samplers[3])
{
    const float PI = radians(180);
    const float HALF_PI = PI / 2.0;

    // Get longitude and latitude from 'position'.
    float longitude;
    float latitude;
    float cos_latitude;
    if (!get_map_projection_longitude_and_latitude(
            longitude, latitude, cos_latitude,
            position, map_projection_central_meridian,
            PI, HALF_PI))
    {
        // The position is too close to either pole (North or South) to determine the direction of the vector relative to
        // the position's local East and North (since they are indeterminate).
        return false;
    }

    // All map projection images are the same size (so we can choose any of them to determine dimensions).
    ivec2 map_projection_image_size = textureSize(map_projection_samplers[0], 0);

    // Get the map projection texture gather coordinates and bilinear weights.
    vec2 map_projection_gather_texture_coord;
    vec2 map_projection_bilinear_weight;
    get_map_projection_texture_gather_params(
            map_projection_gather_texture_coord, map_projection_bilinear_weight, map_projection_image_size,
            longitude, latitude,
            PI, HALF_PI);

    // Sample the second map projection texture.
    vec4 dx_dlon_gather = textureGather(map_projection_samplers[1], map_projection_gather_texture_coord, 0);
    vec4 dx_dlat_gather = textureGather(map_projection_samplers[1], map_projection_gather_texture_coord, 1);
    vec4 dy_dlon_gather = textureGather(map_projection_samplers[1], map_projection_gather_texture_coord, 2);
    vec4 dy_dlat_gather = textureGather(map_projection_samplers[1], map_projection_gather_texture_coord, 3);

    // Map project the vector.
    map_project_vector_impl(
            map_vector, position, cos_latitude, vector, map_projection_bilinear_weight,
            // Sampled from the second map projection texture...
            dx_dlon_gather, dx_dlat_gather, dy_dlon_gather, dy_dlat_gather);
    
    return true;
}

/*
 * Project a 3D position (on the unit sphere) and a 3D vector onto the 2D map projection (defined by the 3 specified textures).
 *
 * Returns false if the specified position is too close to either pole (North or South) to determine
 * the direction of the vector relative to the position's local East and North (since they are indeterminate).
 *
 * The final map projected vector is actually a 3D vector where the 2D (x,y) component is in the 2D map plane and
 * the z component is along the z-axis of 3D map projection space.
 */
bool
map_project_position_and_vector(
        out vec2 map_position,
        out vec3 map_vector,
        vec3 position,
        vec3 vector,
        float map_projection_central_meridian,
        sampler2D map_projection_samplers[3])
{
    const float PI = radians(180);
    const float HALF_PI = PI / 2.0;

    // Get longitude and latitude from 'position'.
    float longitude;
    float latitude;
    float cos_latitude;
    if (!get_map_projection_longitude_and_latitude(
            longitude, latitude, cos_latitude,
            position, map_projection_central_meridian,
            PI, HALF_PI))
    {
        // The position is too close to either pole (North or South) to determine the direction of the vector relative to
        // the position's local East and North (since they are indeterminate).
        return false;
    }

    // All map projection images are the same size (so we can choose any of them to determine dimensions).
    ivec2 map_projection_image_size = textureSize(map_projection_samplers[0], 0);

    // Get the map projection texture gather coordinates and bilinear weights.
    vec2 map_projection_gather_texture_coord;
    vec2 map_projection_bilinear_weight;
    get_map_projection_texture_gather_params(
            map_projection_gather_texture_coord, map_projection_bilinear_weight, map_projection_image_size,
            longitude, latitude,
            PI, HALF_PI);

    // Sample the first map projection texture.
    vec4             x_gather = textureGather(map_projection_samplers[0], map_projection_gather_texture_coord, 0);
    vec4             y_gather = textureGather(map_projection_samplers[0], map_projection_gather_texture_coord, 1);
    vec4 ddx_dlon_dlat_gather = textureGather(map_projection_samplers[0], map_projection_gather_texture_coord, 2);
    vec4 ddy_dlon_dlat_gather = textureGather(map_projection_samplers[0], map_projection_gather_texture_coord, 3);

    // Sample the second map projection texture.
    vec4 dx_dlon_gather = textureGather(map_projection_samplers[1], map_projection_gather_texture_coord, 0);
    vec4 dx_dlat_gather = textureGather(map_projection_samplers[1], map_projection_gather_texture_coord, 1);
    vec4 dy_dlon_gather = textureGather(map_projection_samplers[1], map_projection_gather_texture_coord, 2);
    vec4 dy_dlat_gather = textureGather(map_projection_samplers[1], map_projection_gather_texture_coord, 3);

    // Sample the third map projection texture.
    vec4 ddx_dlon_dlon_gather = textureGather(map_projection_samplers[2], map_projection_gather_texture_coord, 0);
    vec4 ddx_dlat_dlat_gather = textureGather(map_projection_samplers[2], map_projection_gather_texture_coord, 1);
    vec4 ddy_dlon_dlon_gather = textureGather(map_projection_samplers[2], map_projection_gather_texture_coord, 2);
    vec4 ddy_dlat_dlat_gather = textureGather(map_projection_samplers[2], map_projection_gather_texture_coord, 3);

    // Map project the position.
    map_project_position_impl(
            map_position, map_projection_image_size, map_projection_bilinear_weight,

            // Sampled from the first map projection texture...
            x_gather, y_gather,
            ddx_dlon_dlat_gather, ddy_dlon_dlat_gather,

            // Sampled from the second map projection texture...
            dx_dlon_gather, dx_dlat_gather,
            dy_dlon_gather, dy_dlat_gather,

            // Sampled from the third map projection texture...
            ddx_dlon_dlon_gather, ddx_dlat_dlat_gather,
            ddy_dlon_dlon_gather, ddy_dlat_dlat_gather,

            PI);

    // Map project the vector.
    map_project_vector_impl(
            map_vector, position, cos_latitude, vector, map_projection_bilinear_weight,
            // Sampled from the second map projection texture...
            dx_dlon_gather, dx_dlat_gather, dy_dlon_gather, dy_dlat_gather);
    
    return true;
}

#endif // MAP_PROJECTION_GLSL
