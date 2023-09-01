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
// Clear the scene tile.
//

const uint SCENE_TILE_DESCRIPTOR_SET = 0;
const uint SCENE_TILE_DESCRIPTOR_BINDING = 0;
const uint SCENE_TILE_DIMENSION_CONSTANT_ID = 0;
const uint SCENE_TILE_NUM_FRAGMENTS_IN_STORAGE_CONSTANT_ID = 1;
const uint SCENE_TILE_MAX_FRAGMENTS_PER_SAMPLE_CONSTANT_ID = 2;
const uint SCENE_TILE_SAMPLE_COUNT_CONSTANT_ID = 3;
#include "utils/scene_tile.glsl"

void main()
{
    // Zero the fragment list head image (at the current fragment coordinate) and reset the fragment allocator.
	//
	// Note: We don't have the usual "layout (location = 0) out vec4 colour;" declaration because
	//       we're not writing to the framebuffer and so our colour attachment output will be undefined
	//       which is why we also mask colour attachment writes (using the Vulkan API).
    scene_tile_clear_fragments();
}
