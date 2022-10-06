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
// Sorts and blends a list of fragments (per pixel) in depth order in the final scene.
//

layout (binding = 0, r32ui) uniform uimage2D fragment_list_head_pointer_image;

layout (std430, binding = 0) buffer FragmentList
{
    uvec4 fragments[];
} fragment_list_storage;

layout(location = 0) out vec4 colour;

// Space to sort fragments for the current pixel.
#define MAX_FRAGMENTS 10
uvec4 fragment_list[MAX_FRAGMENTS];

void main()
{
    // Extract the fragments from the fragment list associated with the current pixel.
    uint fragment_count = 0;
    uint current_fragment_pointer = imageLoad(fragment_list_head_pointer_image, ivec2(gl_FragCoord).xy).x;
    while (current_fragment_pointer != 0xffffffff && fragment_count < MAX_FRAGMENTS)
    {
        uvec4 fragment = fragment_list_storage.fragments[current_fragment_pointer];

        fragment_list[fragment_count++] = fragment;
        current_fragment_pointer = fragment.x;
    }

    // Discard if there are no fragments.
    if (fragment_count == 0)
    {
        discard;
    }

    // Sort the fragments in order of decreasing depth (back to front).
    for (uint i = 0; i < fragment_count - 1; i++)
    {
        uvec4 fragment_i = fragment_list[i];
        for (uint j = i + 1; j < fragment_count; j++)
        {
            uvec4 fragment_j = fragment_list[j];

            float depth_i = uintBitsToFloat(fragment_i.z);
            float depth_j = uintBitsToFloat(fragment_j.z);

            if (depth_i < depth_j)
            {
                fragment_list[i] = fragment_j;
                fragment_list[j] = fragment_i;
            }
        }
    }

    // Traverse the back-to-front-sorted fragments and blend them.
    colour = vec4(0);
    for (uint i = 0; i < fragment_count; i++)
    {
        vec4 fragment_colour = unpackUnorm4x8(fragment_list[i].y);
        colour =  mix(colour, fragment_colour, fragment_colour.a);
    }
}
