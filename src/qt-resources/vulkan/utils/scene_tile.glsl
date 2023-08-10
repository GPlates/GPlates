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

#ifndef SCENE_TILE_GLSL
#define SCENE_TILE_GLSL

//
// Objects rendered into the scene can add fragments to per-pixel fragment lists sorted by depth,
// which can then be blended (in per-pixel depth order) into the final scene framebuffer.
//
// Each singly linked fragment list is lock-free (non-blocking) and so will complete its operations even if other invocations halt.
// This is important because the Vulkan spec states that:
//   "The relative execution order of invocations of the same shader type is undefined."
// There are various spin-lock implementations out there, but there can always be issues with deadlocks due to
// one invocation spinning on a lock and thus preventing the invocation that has the lock from releasing it
// (eg, because the invocations happen to be executed in lock-step).
//
// Note: Scene rendering is limited to a fixed-size tile rather than being the size of the render target (eg, swapchain image) in order to
//       limit graphics memory usage. Vulkan guarantees support of at least 128MB for the maximum storage buffer size,
//       but we can fairly easily exceed this. For example, full-screen on a 4K monitor with an average of 8 fragments overlapping
//       each pixel (at 16 bytes per fragment) consumes about 1GB of graphics memory.
//

//
// NOTE: Callers should define SCENE_TILE_DESCRIPTOR_SET and SCENE_TILE_DESCRIPTOR_BINDING as integer constant expressions BEFORE including this header.
//

struct SceneTileFragment
{
    uint next_fragment_pointer;  // pointer to next fragment in a linked list (or zero)
    uint colour;  // colour packed with packUnorm4x8()
    float z;      // depth (eg, gl_FragCoord.z)
};

//
// Using 'coherent' storage images/buffers so that writes from invocations are visible to each other.
// For example, multiple L1 caches near the GPU cores are not used, instead the single L2 cache is used as the point of coherence.
//
// A 2D storage image containing head pointers (uint indices) for each pixel in a scene tile.
layout (set = SCENE_TILE_DESCRIPTOR_SET, binding = SCENE_TILE_DESCRIPTOR_BINDING, r32ui) uniform coherent uimage2D scene_tile_list_head_pointer_image;
//
// A storage buffer containing all the scene fragments in a scene tile.
layout (set = SCENE_TILE_DESCRIPTOR_SET, binding = SCENE_TILE_DESCRIPTOR_BINDING + 1) coherent buffer SceneTileStorage
{
    SceneTileFragment fragments[];
} scene_tile_storage;
//
// A storage buffer containing the global allocation pointer (uint index).
layout (set = SCENE_TILE_DESCRIPTOR_SET, binding = SCENE_TILE_DESCRIPTOR_BINDING + 2) coherent buffer SceneTileAllocator
{
    uint alloc_fragment_pointer;  // starts at 1 (so that 0 can be used as null pointer)
} scene_tile_allocator;


/*
 * Allocate and construct (with null next pointer) a scene fragment in the storage buffer.
 */
uint
scene_tile_allocate_fragment(
        vec4 fragment_colour,
        float fragment_z)
{
    // Allocate a new index into the fragment storage buffer.
    uint fragment_pointer = atomicAdd(scene_tile_allocator.alloc_fragment_pointer, 1);

    // Construct the new fragment (in the fragment storage buffer).
    scene_tile_storage.fragments[fragment_pointer] = SceneTileFragment(
            0,  // next_fragment_pointer
            packUnorm4x8(fragment_colour),
            fragment_z);

    return fragment_pointer;
}

/*
 * Search the fragment list (at the current fragment coordinate) in order of increasing 'z' to
 * find the previous and next fragments.
 *
 * Returns false if a fragment at 'z' would be behind an opaque fragment (and hence not visible).
 */
bool
scene_tile_search_fragment(
        out uint previous_fragment_pointer,
        out uint next_fragment_pointer,
        float fragment_z)
{
    // Start at the head of the fragment list.
    uint head_fragment_pointer = imageLoad(
            scene_tile_list_head_pointer_image,
            ivec2(gl_FragCoord).xy).x;

    previous_fragment_pointer = 0;
    next_fragment_pointer = head_fragment_pointer;

    while (next_fragment_pointer != 0)
    {
        // If the fragment 'z' value is less than the next fragment then the caller will need to
        // insert a new fragment just before the next fragment.
        if (fragment_z < scene_tile_storage.fragments[next_fragment_pointer].z)
        {
            return true;
        }

        // Our fragment will be behind the next fragment.
        // So if the next fragment is opaque then our fragment will not be visible.
        float next_fragment_alpha = unpackUnorm4x8(scene_tile_storage.fragments[next_fragment_pointer].colour).a;
        if (next_fragment_alpha == 1.0)
        {
            return  false;
        }

        previous_fragment_pointer = next_fragment_pointer;
        next_fragment_pointer = scene_tile_storage.fragments[previous_fragment_pointer].next_fragment_pointer;
    }

    // Caller will need to insert a new fragment at the tail of the list (or head if list is empty).
    return true;
}

/*
 * Insert a fragment at the head of the fragment list at the current fragment coordinate.
 *
 * Returns false if unable to update head pointer in image because another invocation just updated it.
 *
 * Note: 'head_fragment_pointer' can be zero (meaning the fragment list is currently empty).
 */
bool
scene_tile_insert_fragment_at_head(
        uint fragment_pointer,
        uint head_fragment_pointer)
{
    // New fragment's next pointer points to head fragment.
    scene_tile_storage.fragments[fragment_pointer].next_fragment_pointer = head_fragment_pointer;

    // Make sure writes to our new fragment have completed before we let other invocations know of its existence
    // (by atomically inserting it into the fragment list).
    memoryBarrier();

    // Attempt to store new *head* pointer (to new fragment) into head-pointer image, and return true if successful.
    // This will fail if another invocation has just updated the head pointer in the image.
    return imageAtomicCompSwap(
            scene_tile_list_head_pointer_image,
            ivec2(gl_FragCoord).xy,
            head_fragment_pointer,
            fragment_pointer) == head_fragment_pointer;
}

/*
 * Insert a fragment after an existing fragment of the fragment list at the current fragment coordinate.
 *
 * Returns false if unable to update next pointer of previous fragment because another invocation just updated it.
 *
 * Note: 'next_fragment_pointer' can be zero (meaning the fragment is being added to the tail of the fragment list).
 */
bool
scene_tile_insert_fragment(
        uint fragment_pointer,
        uint previous_fragment_pointer,
        uint next_fragment_pointer)
{
    // New fragment's next pointer points to next fragment.
    scene_tile_storage.fragments[fragment_pointer].next_fragment_pointer = next_fragment_pointer;

    // Make sure writes to our new fragment have completed before we let other invocations know of its existence
    // (by atomically inserting it into the fragment list).
    memoryBarrier();

    // Attempt to store new pointer (to new fragment) in *previous* fragment's next pointer, and return true if successful.
    // This will fail if another invocation has just updated the next pointer in the previous fragment.
    return atomicCompSwap(
            scene_tile_storage.fragments[previous_fragment_pointer].next_fragment_pointer,
            next_fragment_pointer,
            fragment_pointer) == next_fragment_pointer;
}

/*
 * Add a fragment (of specified colour and depth) to the list of fragments at the current fragment coordinate.
 *
 * Returns false if the fragment is behind an opaque fragment (and hence not visible),
 * or if fragment is fully transparent, or if fragment is a helper invocation.
 */
bool
scene_tile_add_fragment(
        vec4 fragment_colour,
        float fragment_z)
{
    if (gl_HelperInvocation)
    {
        // We're a helper invocation, so nothing to do.
        return false;
    }

    if (fragment_colour.a == 0)
    {
        // Fragment is completely transparent, and hence not visible.
        return false;
    }

    // We don't allocate/construct our fragment until we need to.
    // Because we may never add a fragment (if it's behind existing opaque fragments and hence not visible).
    uint fragment_pointer = 0;

    while (true)
    {
        // Search in order of increasing fragment 'z' to find the previous and next fragments of our fragment.
        uint previous_fragment_pointer;
        uint next_fragment_pointer;
        if (!scene_tile_search_fragment(previous_fragment_pointer, next_fragment_pointer, fragment_z))
        {
            // If we've already allocated/constructed a fragment then it'll remain allocated but unused.
            //
            // Note: It's possible the fragment has already been allocated because we may have tried to insert it but failed
            //       (because another invocation inserted a fragment just before it) and hence we tried again to search
            //       (where to insert) and found that the fragment is now occluded (by that other invocation's fragment).
            return false;
        }

        // If we've not allocated/constructed our fragment yet then do so now.
        if (fragment_pointer == 0)
        {
            fragment_pointer = scene_tile_allocate_fragment(fragment_colour, fragment_z);
        }

        // Insert our fragment into the fragment list at the current fragment coordinate (pixel).
        if (previous_fragment_pointer == 0)
        {
            // No previous fragment means we need to insert at the head of the list.
            // Note that the next fragment can also be null (meaning the list is currently empty).
            if (scene_tile_insert_fragment_at_head(fragment_pointer, next_fragment_pointer))
            {
                return true;
            }
        }
        else
        {
            // There's a previous fragment, so we can insert after it.
            // Note that the next fragment can be null (meaning we're inserting a new tail).
            if (scene_tile_insert_fragment(fragment_pointer, previous_fragment_pointer, next_fragment_pointer))
            {
                return true;
            }
        }

        // If we get here then another invocation inserted just where we were attempting to insert.
        // So try again from the start of the fragment list.
    }

    // Shouldn't be able to get here.
    return false;
}

#endif // SCENE_TILE_GLSL
