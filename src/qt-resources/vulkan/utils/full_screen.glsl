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

#ifndef FULL_SCREEN_GLSL
#define FULL_SCREEN_GLSL

//
// Functions to get clip space position in the range [-1, 1] and texture space coordinates
// in the range [0, 1] for a fullscreen "quad" rendered with:
//
//   vkCmdDraw(commandBuffer, 3, 1, 0, 0);
//
// ...and without needing any vertex/index buffers.
//
// This actually draws a triangle (not a quad) that covers more than just the screen but
// the parts outside the screen are clipped to leave a quad covering just the screen
// (in reality, GPUs use a guard band to minimize actual clipping that introduces new vertices).
//
// See https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
//

/*
 * Get the clip space position in the range [-1, 1] from the vertex index (eg, 'gl_VertexIndex').
 *
 * The triangle is oriented clockwise (in Vulkan framebuffer space).
 * Vulkan is opposite to OpenGL since Vulkan framebuffer 'y' is top-down (OpenGL is bottom-up).
 *
 * The top-left has clip coordinates [-1, -1] and bottom-right has [1, 1].
 */
vec4
full_screen_clip_space_coordinates(
        int vertex_index)
{
    // In the range [0, 1].
    vec2 texture_space_coordinates = vec2(
            (vertex_index << 1) & 2,
            vertex_index & 2);
    
    // Convert to range [-1, 1] and return as vec4.
    return vec4(2.0 * texture_space_coordinates - 1.0, 0.0, 1.0);
}

/*
 * Get the texture space position in the range [0, 1] from the vertex index (eg, 'gl_VertexIndex').
 *
 * The triangle is oriented clockwise (in Vulkan framebuffer space).
 * Vulkan is opposite to OpenGL since Vulkan framebuffer 'y' is top-down (OpenGL is bottom-up).
 *
 * The top-left has texture coordinates [0, 0] and bottom-right has [1, 1].
 */
vec2
full_screen_texture_space_coordinates(
        int vertex_index)
{
    // In the range [0, 1].
    return vec2(
            (vertex_index << 1) & 2,
            vertex_index & 2);
}

#endif // FULL_SCREEN_GLSL
