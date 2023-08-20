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

//
// Blend the scene tile.
//

const uint SCENE_TILE_DESCRIPTOR_SET = 0;
const uint SCENE_TILE_DESCRIPTOR_BINDING = 0;
const uint SCENE_TILE_DIMENSION_CONSTANT_ID = 0;
const uint SCENE_TILE_SAMPLE_COUNT_CONSTANT_ID = 1;
#include "utils/scene_tile.glsl"

layout (location = 0) out vec4 colour;

void main()
{
    // Traverse the fragment list (at the current fragment coordinate) and blend the fragments in depth order.
    colour = scene_tile_blend_fragments();

    // Discard if fully transparent (eg, no fragments in list).
    if (colour.a == 0)
    {
        discard;
    }
}
