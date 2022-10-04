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
// Renders stars (points) in the background of the scene.
//

uniform vec4 star_colour;

in VertexData
{
    float point_size_scale;
    float inv_point_size;
} fs_in;

layout(location = 0) out vec4 colour;

void main (void)
{
    colour = star_colour;

    // Calculate distance from current fragment centre to the centre of the circular point.
    //
    // Note: 'gl_PointCoord' range is [0,1] which we convert to [-1,1] and then
    //       convert that to [-point_size_scale, point_size_scale] which is a larger range
    //       than [-1,1]. And now a distance of 1 from point centre is the point circumference.
    vec2 fragment_to_point_centre = fs_in.point_size_scale * (2 * gl_PointCoord - 1);
    float distance_to_point_centre = length(fragment_to_point_centre);

    // How much does the distance change compared to its neighbouring pixels?
    //
    // Note: We could set 'distance_delta_over_a_pixel = fwidth(distance_to_point_centre)' but that
    //       it not as stable as hard-coding it.
    //
    // We know there are 'point_size' pixels in the distance range [-1, 1]
    // (so the distance over a pixel is '2 / point_size').
    float distance_delta_over_a_pixel = 2 * fs_in.inv_point_size;

    // Alpha is 1 when 'distance < 1 - distance_delta_over_a_pixel' and 0 when
    // 'distance > 1 + distance_delta_over_a_pixel' and smoothly interpolated inbetween.
    // This corresponds to inside, outside and on circumference (within a 2 pixel margin
    // which is anti-aliased).
    colour.a = 1 - smoothstep(
                    1 - distance_delta_over_a_pixel,
                    1 + distance_delta_over_a_pixel,
                    distance_to_point_centre);
}
