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

layout (push_constant) uniform PushConstants
{
    mat4 view_projection;
    vec4 star_colour;
    float radius_multiplier;
    float point_size;
};

layout (location = 0) in vec4 position;

layout (location = 0) out VertexData
{
    float point_size_scale;
    float inv_point_size;
} vs_out;

out gl_PerVertex
{
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[1];
};

void main()
{
    // Expand the position away from the origin by the requested amount.
    vec4 radius_expanded_position = vec4(radius_multiplier * position.xyz, 1);

    gl_Position = view_projection * radius_expanded_position;

    // Expand the point size by 2 pixels (1 pixel radius expansion) to give space
    // for one pixel of anti-aliasing outside circumference (see fragment shader).
    float point_size_expansion = 2;
    float expanded_point_size = point_size + point_size_expansion;
    vs_out.point_size_scale = expanded_point_size / point_size;
    vs_out.inv_point_size = 1.0 / point_size;
    gl_PointSize = expanded_point_size;
    
    // We enabled GL_DEPTH_CLAMP to disable the far clipping plane, but it also disables
    // near plane (which we still want), so we'll handle that ourself.
    //
    // Inside near clip plane means:
    //
    //   -gl_Position.w <= gl_Position.z
    //
    // ...or:
    //
    //   gl_Position.z + gl_Position.w >= 0
    //
    gl_ClipDistance[0] = gl_Position.z + gl_Position.w;
}
