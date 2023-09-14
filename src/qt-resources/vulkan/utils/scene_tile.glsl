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
// Callers should only need to call one function in order to add a fragment:
//
//    /*
//     * Add a fragment (of specified colour and depth) to the list of fragments at the current pixel coordinate.
//     *
//     * Returns false if the fragment is behind an opaque fragment (and hence not visible),
//     * or if fragment is fully transparent, or if fragment is a helper invocation.
//     */
//    bool
//    scene_tile_add_fragment(
//            vec4 fragment_colour,
//            float fragment_z);  // eg, pass in gl_FragCoord.z
//
// NOTE: Callers should also define the following as integer constant expressions BEFORE including this header:
//       * SCENE_TILE_DESCRIPTOR_SET - the descriptor set to be used for the scene tile.
//       * SCENE_TILE_DESCRIPTOR_BINDING - the first descriptor binding in the set (subsequent bindings in the scene tile will increment this).
//       * SCENE_TILE_DIMENSION_CONSTANT_ID - the 'constant_id' of the specialization constant for the scene tile dimension.
//       * SCENE_TILE_NUM_FRAGMENTS_IN_STORAGE_CONSTANT_ID - the 'constant_id' of the specialization constant for the total number of fragments in storage.
//       * SCENE_TILE_MAX_FRAGMENTS_PER_SAMPLE_CONSTANT_ID - the 'constant_id' of the specialization constant for the maximum number of fragments per sample.
//       * SCENE_TILE_SAMPLE_COUNT_CONSTANT_ID - the 'constant_id' of the specialization constant for the (multi)sample count.
//
// Each singly linked fragment list is lock-free (non-blocking) and so will complete its operations even if other invocations halt.
// This is important because the Vulkan spec states that:
//   "The relative execution order of invocations of the same shader type is undefined."
// There are various spin-lock implementations out there, but there can always be issues with deadlocks due to
// one invocation spinning on a lock and thus preventing the invocation that has the lock from releasing it
// (eg, because the invocations happen to be executed in lock-step).
//
// The memory management of fragments follows that of:
//   "Lock-Free Linked Lists Using Compare-and-Swap"  Valois 1995, with corrections covered in:
//   "Correction of a Memory Management Method for Lock-Free Data Structures" Scott 1996.
// ...which is a clever reference-counting scheme that tracks all shared and private pointers to fragments, and reclaims a fragment (for re-use)
// when there are no longer any references to it, and also solves the ABA problem (https://en.wikipedia.org/wiki/ABA_problem).
//
// Note: Scene rendering is limited to a fixed-size tile rather than being the size of the render target (eg, swapchain image) in order to
//       limit graphics memory usage. Vulkan guarantees support of at least 128MB for the maximum storage buffer size,
//       but we can fairly easily exceed this. For example, full-screen on a 4K monitor with an average of 8 fragments overlapping
//       each pixel (at 16 bytes per fragment) consumes about 1GB of graphics memory.
//


/*
 * Information for a fragment generated at a pixel coordinate.
 *
 * Multiple fragments can get generated at the same pixel coordinate.
 */
struct SceneTileFragment
{
    uint next_fragment_pointer;  // pointer to next fragment in a linked list (or zero)
    uint colour;   // colour packed with packUnorm4x8()
    float z;       // depth (eg, gl_FragCoord.z)
    // The following field contains 3 quantities:
    // 1) the sample coverage of this fragment (eg, gl_SampleMaskIn[0]) in the lowest 'scene_tile_sample_count' bits, and
    // 2) the memory reclamation boolean (1 bit) to ensure only one invocation moves a removed fragment onto the free list, and
    // 3) the reference count of fragment pointers referencing this fragment (the remaining bits).
    uint reference_count_and_reclaim_and_coverage;
};


layout (constant_id = SCENE_TILE_DIMENSION_CONSTANT_ID) const uint scene_tile_dimension = 1024;
// Vulkan guarantees support of at least 128MB for the maximum storage buffer size.
layout (constant_id = SCENE_TILE_NUM_FRAGMENTS_IN_STORAGE_CONSTANT_ID) const uint scene_tile_num_fragments_in_storage = 128 * 1024 * 1024 / 16/*fragment size*/;
layout (constant_id = SCENE_TILE_MAX_FRAGMENTS_PER_SAMPLE_CONSTANT_ID) const uint scene_tile_max_fragments_per_sample = 8;
layout (constant_id = SCENE_TILE_SAMPLE_COUNT_CONSTANT_ID) const uint scene_tile_sample_count = 1;


//
// Using 'coherent' storage images/buffers so that writes from invocations are visible to each other.
// For example, multiple L1 caches near the GPU cores are not used, instead the single L2 cache is used as the point of coherence.
//
// Using 'volatile' storage images/buffers since the same location/variable can be accessed repeatedly, and other invocations may
// modify these variables inbetween these accesses (and so variable should be refetched from memory on each access).
//
// A 2D storage image containing head pointers (uint indices) for each pixel in a scene tile.
layout (set = SCENE_TILE_DESCRIPTOR_SET, binding = SCENE_TILE_DESCRIPTOR_BINDING, r32ui) coherent volatile uniform uimage2D scene_tile_pixel_list_head_image;
//
// A storage buffer containing all the scene fragments in a scene tile.
layout (set = SCENE_TILE_DESCRIPTOR_SET, binding = SCENE_TILE_DESCRIPTOR_BINDING + 1) coherent volatile buffer SceneTileStorage
{
    SceneTileFragment fragments[];
} scene_tile_storage;
//
// A storage buffer containing the storage allocater and free list.
layout (set = SCENE_TILE_DESCRIPTOR_SET, binding = SCENE_TILE_DESCRIPTOR_BINDING + 2) coherent volatile buffer SceneTileAllocator
{
    // Allocate fragments directly from storage (using incremented indices) until storage runs out...
    uint alloc_fragment_storage_index;    // starts at 1 (so that 0 can be used as null pointer)
    // ...then allocate from the free list (which contains deallocated fragments).
    uint alloc_fragment_free_list;
} scene_tile_allocator;


// The fragment coverage mask for all samples in a pixel (eg, 0xf for 4xMSAA).
const uint scene_tile_fragment_coverage_mask = (1 << scene_tile_sample_count) - 1;

// The fragment memory reclamation mask (1 bit after the fragment coverage mask).
const uint scene_tile_fragment_reclaim_mask = 1 << scene_tile_sample_count;

// The fragment pointers reference count *increment* value (the value '1' bit-shifted after the fragment memory reclamation mask bit).
const uint scene_tile_fragment_reference_count_increment = 1 << (scene_tile_sample_count + 1);
// The fragment pointers reference count *decrement* value (stored as an *unsigned* integer representing the equivalent negative signed value).
// Note: The constructors int(uint) and uint(int) preserve the bit pattern on conversion.
//       And integer arithmetic in Vulkan uses two's complement, so we can emulate subtraction using unsigned integers.
const uint scene_tile_fragment_reference_count_decrement = uint(-int(scene_tile_fragment_reference_count_increment));

// The fragment pointers reference count mask (remaining bits after the fragment memory reclamation mask).
const uint scene_tile_fragment_reference_count_mask = ~(scene_tile_fragment_reference_count_increment - 1);

// Mask for the mark bit in a fragment pointer.
// This is just a single bit (the highest-order bit) for marking a fragment for removal from a pixel list.
const uint scene_tile_marked_fragment_pointer_mask = 0x80000000;

// Returns true if fragment pointer is marked (for removal from a pixel list).
#define SCENE_TILE_FRAGMENT_POINTER_IS_MARKED(fragment_pointer) \
    ((fragment_pointer & scene_tile_marked_fragment_pointer_mask) != 0)

// Returns the unmarked fragment pointer (with mark bit zeroed but not the tag bits).
#define SCENE_TILE_GET_UNMARKED_FRAGMENT_POINTER(fragment_pointer) \
    (fragment_pointer & ~scene_tile_marked_fragment_pointer_mask)

// Convert a fragment pointer to a fragment index (into storage buffer).
// This returns the unmarked fragment pointer (with marked bit set to zero).
#define SCENE_TILE_GET_FRAGMENT_INDEX(fragment_pointer) \
    SCENE_TILE_GET_UNMARKED_FRAGMENT_POINTER(fragment_pointer)

// Return the fragment in storage pointed to by the specified fragment pointer.
#define SCENE_TILE_GET_FRAGMENT(fragment_pointer) \
    scene_tile_storage.fragments[SCENE_TILE_GET_FRAGMENT_INDEX(fragment_pointer)]

// Returns the fragment coverage of the specified fragment.
#define SCENE_TILE_FRAGMENT_GET_COVERAGE(fragment) \
    (fragment.reference_count_and_reclaim_and_coverage & scene_tile_fragment_coverage_mask)


/*
 * Track the occlusion caused by the closest fragments at a pixel coordinate.
 *
 * Keeps track of how many fragments (in a pixel list) cover each sample of the pixel and
 * their accumulated occlusion (alpha).
 */
struct SceneTilePixelOcclusion
{
    uint num_fragments_per_sample[scene_tile_sample_count];
    float alpha_per_sample[scene_tile_sample_count];
};

/*
 * Clear the pixel occlusion.
 */
void
scene_tile_pixel_occlusion_clear(
        out SceneTilePixelOcclusion pixel_occlusion)
{
    for (uint sample_id = 0; sample_id < scene_tile_sample_count; ++sample_id)
    {
        pixel_occlusion.num_fragments_per_sample[sample_id] = 0;
        pixel_occlusion.alpha_per_sample[sample_id] = 0.0;
    }
}

/*
 * Update the occlusion (alpha), and length of fragment list, for samples covered by the next closest (in z) fragment.
 *
 * Note: This function should be called in order of increasing fragment 'z' (depth).
 *
 * One reason we do this on a per-sample basis is because it's possible that two adjacent triangles both
 * cover a single pixel (eg, 3 samples from first triangle and 1 from second, for 4xMSAA) and each
 * triangle will contribute its own fragment and hence the two fragments cannot obscure each other
 * (because they're adjacent in screen space). Also, the triangles don't have to be adjacent, they
 * can belong to separate primitives that do have overlapping samples (in z).
 */
void
scene_tile_pixel_occlusion_update(
        inout SceneTilePixelOcclusion pixel_occlusion,
        SceneTileFragment fragment)
{
    float fragment_alpha = unpackUnorm4x8(fragment.colour).a;

    for (uint sample_id = 0; sample_id < scene_tile_sample_count; ++sample_id)
    {
        // If the incoming fragment covers the current sample.
        if ((SCENE_TILE_FRAGMENT_GET_COVERAGE(fragment) & (1 << sample_id)) != 0)
        {
            // Increment the number of fragments covering the current sample (to include the incoming fragment).
            pixel_occlusion.num_fragments_per_sample[sample_id]++;

            // Accumulate alpha at the current sample (to include the incoming fragment).
            pixel_occlusion.alpha_per_sample[sample_id] += (1 - pixel_occlusion.alpha_per_sample[sample_id]) * fragment_alpha;
        }
    }
}

/*
 * Return whether the next closest (in z) fragment is occluded by closer fragments (added with calls to 'scene_tile_pixel_occlusion_update()').
 *
 * If all samples covered by the incoming fragment have been occluded then fragment itself is occluded.
 */
bool
scene_tile_pixel_occlusion_is_fragment_occluded(
        SceneTilePixelOcclusion pixel_occlusion,
        int fragment_coverage)
{
    // See if any samples (covered by the incoming fragment) are occluded by closer (z) fragments or
    // if the maximum number of fragments are already closer (z).
    for (uint sample_id = 0; sample_id < scene_tile_sample_count; ++sample_id)
    {
        // If the incoming fragment covers the current sample.
        if ((fragment_coverage & (1 << sample_id)) != 0)
        {
            // See if the number of (closer) fragments covering the current sample has reached the maximum.
            if (pixel_occlusion.num_fragments_per_sample[sample_id] >= scene_tile_max_fragments_per_sample)
            {
                continue;  // sample occluded
            }

            // See if the accumulated alpha at the current sample is enough to occlude.
            if (pixel_occlusion.alpha_per_sample[sample_id] > 0.99)
            {
                continue;  // sample occluded
            }

            // If we get here then the current sample is not yet covered by the maximum number of
            // fragments (per sample) and is not occluded by them (so far).
            // Therefore the incoming fragment is not fully occluded.
            return false;
        }
    }

    // All samples covered by the incoming fragment have been occluded, so the fragment itself is occluded.
    return  true;
}


/*
 * Set the fragment coverage and clear the fragment reclaim bit in the specified fragment (leaving reference count intact).
 *
 * This is all done in one atomic operation.
 */
void
scene_tile_fragment_set_coverage_and_clear_reclaim(
        uint fragment_pointer,
        int fragment_coverage)
{
    while (true)
    {
        // Current value.
        uint old_reference_count_and_reclaim_and_coverage = SCENE_TILE_GET_FRAGMENT(fragment_pointer).reference_count_and_reclaim_and_coverage;

        // New value.
        uint new_reference_count_and_reclaim_and_coverage = old_reference_count_and_reclaim_and_coverage;
        new_reference_count_and_reclaim_and_coverage & ~scene_tile_fragment_reclaim_mask;    // clear the reclaim bit
        new_reference_count_and_reclaim_and_coverage &= ~scene_tile_fragment_coverage_mask;  // clear the coverage bits
        new_reference_count_and_reclaim_and_coverage |= fragment_coverage;                   // set the coverage bits

        // Attempt to change the old value to the new value.
        if (atomicCompSwap(
                SCENE_TILE_GET_FRAGMENT(fragment_pointer).reference_count_and_reclaim_and_coverage,
                old_reference_count_and_reclaim_and_coverage,
                new_reference_count_and_reclaim_and_coverage) == old_reference_count_and_reclaim_and_coverage)
        {
            return;  // succeeded
        }

        // If we get here then another invocation changed the reference count between our read and our atomic operation.
        // So try again with the updated value.
    }
}

/*
 * Decrement the reference count of the specified fragment, and if it's zero then also set the fragment reclaim bit and return true (otherwise false).
 *
 * This is all done in one atomic operation.
 */
bool
scene_tile_fragment_decrement_reference_count_and_reclaim(
        uint fragment_pointer)
{
    while (true)
    {
        // Current value.
        uint old_reference_count_and_reclaim_and_coverage = SCENE_TILE_GET_FRAGMENT(fragment_pointer).reference_count_and_reclaim_and_coverage;

        // New value.
        uint new_reference_count_and_reclaim_and_coverage = old_reference_count_and_reclaim_and_coverage;
        // Decrement the reference count.
        new_reference_count_and_reclaim_and_coverage += scene_tile_fragment_reference_count_decrement;
        // If the reference count is zero then also set the reclaim bit.
        if ((new_reference_count_and_reclaim_and_coverage & scene_tile_fragment_reference_count_mask) == 0)
        {
            new_reference_count_and_reclaim_and_coverage |= scene_tile_fragment_reclaim_mask;
        }

        // Attempt to change the old value to the new value.
        if (atomicCompSwap(
                SCENE_TILE_GET_FRAGMENT(fragment_pointer).reference_count_and_reclaim_and_coverage,
                old_reference_count_and_reclaim_and_coverage,
                new_reference_count_and_reclaim_and_coverage) == old_reference_count_and_reclaim_and_coverage)
        {
            // Swap succeeded.
            // Return true if we've just set the reclaim bit (ie, it was clear before but is now set).
            return ((old_reference_count_and_reclaim_and_coverage ^ new_reference_count_and_reclaim_and_coverage) & scene_tile_fragment_reclaim_mask) != 0;
        }

        // If we get here then another invocation changed the reference count (and possibly also set the reclaim bit) between our read and our atomic operation.
        // So try again with the updated value.
    }
}

/*
 * Deallocate a fragment (by adding to the free list).
 */
void
scene_tile_fragment_deallocate(
        uint fragment_pointer)
{
    while (true)
    {
        // The deallocated pointer will become the head of the free list.
        // So the current head of the free list will then become the next fragment in the free list.
        uint next_fragment_pointer = scene_tile_allocator.alloc_fragment_free_list;
        SCENE_TILE_GET_FRAGMENT(fragment_pointer).next_fragment_pointer = next_fragment_pointer;

        // Make sure the write to the next pointer (in our deallocated fragment) has completed before we let other invocations
        // know of its existence (by atomically inserting it into the free list).
        memoryBarrier();

        // Attempt to store new head pointer (the deallocated fragment) into free list, and return true if successful.
        // This will fail if another invocation has just updated the head pointer.
        if (atomicCompSwap(
                scene_tile_allocator.alloc_fragment_free_list,
                next_fragment_pointer,
                fragment_pointer) == next_fragment_pointer)
        {
            return;  // succeeded
        }

        // If we get here then another invocation pushed or popped the head of the free list.
        // So try adding to the head again.
    }
}

/*
 * Release a fragment (decrements reference count of pointed-to fragment, and reclaims if zero and not yet reclaimed).
 */
void
scene_tile_fragment_release(
        uint fragment_pointer)
{
    if (fragment_pointer == 0)
    {
        return;
    }

    // Decrement the reference count.
    // If it's zero and we reclaimed it (are the first to set its reclaim bit) then
    // deallocate the fragment (return it to the free list to be available for re-use).
    if (scene_tile_fragment_decrement_reference_count_and_reclaim(fragment_pointer))
    {
        scene_tile_fragment_deallocate(fragment_pointer);
    }
}

/*
 * Macro to acquire a fragment.
 *
 * The fragment pointer is obtained using the expression 'access_fragment_pointer'.
 * This needs to be a macro (instead of a function) because the expression could reference
 * a storage buffer or image, and we cannot perform atomic operations on them when they are
 * passed into a function (because function passes values by copying rather than by reference).
 */
#define SCENE_TILE_FRAGMENT_ACQUIRE(access_fragment_pointer) \
    while (true) \
    { \
        /* Access the fragment pointer. */ \
        uint fragment_pointer = access_fragment_pointer; \
        if (fragment_pointer == 0) \
        { \
            return 0; \
        } \
        \
        /* Acquire the fragment by incrementing its reference count. */ \
        atomicAdd( \
                SCENE_TILE_GET_FRAGMENT(fragment_pointer).reference_count_and_reclaim_and_coverage, \
                scene_tile_fragment_reference_count_increment); \
        \
        /* See if fragment pointer has been modified (by another invocation). */ \
        /* Access it again to make sure it hasn't. */ \
        if (fragment_pointer == access_fragment_pointer) \
        { \
            return fragment_pointer; \
        } \
        \
        /* If we get here then another invocation modified the fragment pointer. */ \
        /* This means the fragment's reference count could have gone to zero *before* we atomically incremented it */ \
        /* and hence the fragment could have been reclaimed already. So we can't acquire it */ \
        /* (because it could now be being re-used for another newly allocated fragment). */ \
        /* So we decrement the reference count (that we just incremented), and then */ \
        /* try removing/popping the (new) head of the free list again. */ \
        /* */ \
        /* Note: We can actually decrement the reference count while the fragment is being re-used as a new fragment, */ \
        /*       and this won't (incorrectly) reclaim the fragment, because if the fragment is being re-used then it */ \
        /*       will have incremented the reference count (meaning our decrement cannot set it to zero and hence will not reclaim it). */ \
        /*       Alternatively if it hasn't been re-used yet (but is available for re-use on the free list) then the reclaim bit will */ \
        /*       be set (also preventing us from reclaiming it). */ \
        scene_tile_fragment_release(fragment_pointer); \
    }

/*
 * Acquire the head fragment in the free list (increments reference count of head fragment).
 */
uint
scene_tile_fragment_acquire_free_list_head()
{
    SCENE_TILE_FRAGMENT_ACQUIRE
    (
            scene_tile_allocator.alloc_fragment_free_list
    )
}

/*
 * Acquire the head fragment of the current pixel's fragment list (increments reference count of head fragment).
 */
uint
scene_tile_fragment_acquire_pixel_list_head()
{
    SCENE_TILE_FRAGMENT_ACQUIRE
    (
            imageLoad(scene_tile_pixel_list_head_image, ivec2(gl_FragCoord.xy)).r
    )
}

/*
 * Acquire the next fragment of the specified fragment (increments reference count of next fragment).
 */
uint
scene_tile_fragment_acquire_pixel_list_next(
        uint pixel_list_fragment_pointer)
{
    SCENE_TILE_FRAGMENT_ACQUIRE
    (
            SCENE_TILE_GET_FRAGMENT(pixel_list_fragment_pointer).next_fragment_pointer
    )
}

/*
 * Allocate and construct (with null next pointer) a scene fragment in the storage buffer.
 */
uint
scene_tile_fragment_allocate(
        vec4 fragment_colour,
        float fragment_z,
        int fragment_coverage)
{
    // Pointer to the fragment to be allocated.
    uint fragment_pointer;

    // Attempt to allocate directly from the storage buffer.
    //
    // This allocates all fragments (in the storage buffer) for the first time.
    // Some of them will subsequently get freed/reclaimed and end up in the free list.
    // But the free list is only used once all fragments in the storage buffer have been allocated first.
    if (
        // Avoid incrementing alloc fragment pointer if we've exceeded total number of fragments in storage.
        // This helps prevent the potential for it to wraparound back to zero (and incorrectly start allocating again)...
        scene_tile_allocator.alloc_fragment_storage_index < scene_tile_num_fragments_in_storage &&
        // Allocate a new index into the fragment storage buffer and check it hasn't exceeded total number of fragments in storage.
        // This second check is necessary because another invocation might have allocated the last fragment in storage between
        // when we checked (just above) and when we allocated just now (with atomic increment).
        (fragment_pointer = atomicAdd(scene_tile_allocator.alloc_fragment_storage_index, 1)) < scene_tile_num_fragments_in_storage)
    {
        // Set the fragment coverage and increment the reference count (to the equivalent of '1') and leave the reclaim bit clear.
        //
        // Note: This is the first time this fragment memory is being allocated/referenced (ie, it's not being reclaimed)
        //       so we don't need to increment the reference count using 'scene_tile_fragment_acquire*()'.
        uint reference_count_and_reclaim_and_coverage = fragment_coverage + scene_tile_fragment_reference_count_increment;

        // Construct the new fragment (in the fragment storage buffer).
        SCENE_TILE_GET_FRAGMENT(fragment_pointer) = SceneTileFragment(
                0,  // next_fragment_pointer
                packUnorm4x8(fragment_colour),
                fragment_z,
                reference_count_and_reclaim_and_coverage);
    }
    else  // we've run out of storage...
    {
        // Allocate from the free list.
        while (true)
        {
            // Acquire the head fragment on the free list.
            //
            // Note: It's important to acquire the fragment (increment reference count) before we clear the reclaim bit
            //       because once the reclaim bit is cleared then a pointer to this fragment (in another invocation) might
            //       release the fragment which might otherwise decrement its reference count to zero and find the reclaim bit
            //       is clear and (incorrectly) deallocate the fragment (add to free list, even though it's already on the free list).
            //       By incrementing the reference count here we avoid this problem.
            fragment_pointer = scene_tile_fragment_acquire_free_list_head();
            if (fragment_pointer == 0)
            {
                // The free list is empty, so we're unable to allocate a new fragment.
                return 0;
            }

            // The next fragment in the free list.
            uint next_fragment_pointer = SCENE_TILE_GET_FRAGMENT(fragment_pointer).next_fragment_pointer;

            // Attempt to store new head pointer (the next fragment in free list) into free list.
            // This will fail if another invocation has just updated the head pointer.
            if (atomicCompSwap(
                    scene_tile_allocator.alloc_fragment_free_list,
                    fragment_pointer,
                    next_fragment_pointer) == fragment_pointer)
            {
                break;  // succeeded
            }

            // If we get here then another invocation pushed or popped the head of the free list.
            // So first release the acquired fragment, and then try removing/popping the (new) head again.
            scene_tile_fragment_release(fragment_pointer);
        }

        // Initialise the new fragment.
        SCENE_TILE_GET_FRAGMENT(fragment_pointer).next_fragment_pointer = 0;
        SCENE_TILE_GET_FRAGMENT(fragment_pointer).colour = packUnorm4x8(fragment_colour);
        SCENE_TILE_GET_FRAGMENT(fragment_pointer).z = fragment_z;
        // Set the fragment coverage and clear the fragment reclaim bit (leaving reference count intact).
        // Note: The reference count can still be updated by other invocations even though the fragment is on the free list.
        scene_tile_fragment_set_coverage_and_clear_reclaim(fragment_pointer, fragment_coverage);
    }

    return fragment_pointer;
}


/*
 * Search the fragment list (at the current fragment coordinate) in order of increasing 'z' to
 * find the previous and next fragments.
 *
 * The starting location of the search is specified with 'previous_fragment_pointer', which is both input and output (due to 'inout').
 * Since it's input the caller should own (have acquired) 'previous_fragment_pointer' (if non-null) to ensure not reclaimed by another invocation.
 * On output, 'previous_fragment_pointer' can change. If it has changed then its pointer value (from input) is released and a reference to
 * the new pointer is acquired (and transferred to the caller on returning). If it hasn't changed then its pointer (from input) is not released.
 * In either case it becomes the responsibility of the caller to release the returned (on output) 'previous_fragment_pointer'
* (without having to worry about releasing the 'previous_fragment_pointer' value from input).
 * The location 'next_fragment_pointer' is output only (due to 'out').
 * On output, 'next_fragment_pointer' will be acquired (fragment reference count incremented) and that new reference will be transferred to the caller on returning.
 * So, in summary, both 'previous_fragment_pointer' and 'next_fragment_pointer' references are owned by the caller on return.
 *
 * The accumulated pixel occlusion is specified with 'pixel_occlusion' (it's input and output due to 'inout') and
 * will be updated using all fragments closer (in 'z') to the incoming fragment.
 *
 * Returns false if a fragment at 'z' would be behind an opaque fragment (and hence not visible) or
 * there are already enough (max allowed) closer fragments (in z order).
 */
bool
scene_tile_search_fragment(
        inout SceneTilePixelOcclusion pixel_occlusion,
        inout uint previous_fragment_pointer,
        out uint next_fragment_pointer,
        float fragment_z,
        int fragment_coverage)
{
    if (previous_fragment_pointer == 0)
    {
        // Start at the head of the current pixel's fragment list.
        //
        // Note: We need to acquire the next fragment (because the next fragment pointer is not passed in).
        next_fragment_pointer = scene_tile_fragment_acquire_pixel_list_head();
    }
    else
    {
        // Start at the previous fragment.
        //
        // Note: The caller must have already acquired the previous fragment because
        //       they passed us a non-null pointer to the previous fragment.
        //       However we need to acquire the next fragment (because it's not passed in).
        next_fragment_pointer = scene_tile_fragment_acquire_pixel_list_next(previous_fragment_pointer);
    }

    while (next_fragment_pointer != 0)
    {
        SceneTileFragment next_fragment = SCENE_TILE_GET_FRAGMENT(next_fragment_pointer);

        // If the fragment 'z' value is less than the next fragment then the caller will need to
        // insert a new fragment just before the next fragment.
        if (fragment_z < next_fragment.z)
        {
            return true;
        }

        // Update the occlusion (alpha), and length of fragment list, for samples covered by the next fragment.
        //
        // Later the sample-rate shading (ie, fragment shader invoked per-sample) will blend each sample separately.
        scene_tile_pixel_occlusion_update(pixel_occlusion, next_fragment);
        
        // If the incoming fragment samples are occluded by closer fragments then reject the fragment.
        if (scene_tile_pixel_occlusion_is_fragment_occluded(pixel_occlusion, fragment_coverage))
        {
            return false;
        }

        // Release the previous fragment and move the previous pointer to the current 'next' fragment.
        // Note that we've already acquired the 'next' fragment, so we don't have to do that here.
        scene_tile_fragment_release(previous_fragment_pointer);
        previous_fragment_pointer = next_fragment_pointer;

        // Acquire the fragment after the current 'next' fragment and move the current 'next' pointer to it.
        next_fragment_pointer = scene_tile_fragment_acquire_pixel_list_next(next_fragment_pointer);
    }

    // Caller will need to insert a new fragment at the tail of the list (or head if list is empty).
    return true;
}

/*
 * Insert a fragment at the head of the fragment list at the current fragment coordinate.
 *
 * Returns false if unable to update head pointer in image because another invocation just updated it
 * (ie, if the list head pointer is not 'head_fragment_pointer').
 *
 * If successful then 'fragment_pointer' reference ownership is transferred into the list (otherwise caller retains it).
 * The caller should also own (have acquired) 'head_fragment_pointer' (but its ownership is not transferred),
 * it just ensures the head fragment is not reclaimed by another invocation (while we're still referencing it).
 *
 * Note: 'head_fragment_pointer' can be zero (meaning the fragment list is currently empty).
 */
bool
scene_tile_insert_fragment_at_head(
        uint fragment_pointer,
        uint head_fragment_pointer)
{
    // New fragment's next pointer points to head fragment.
    SCENE_TILE_GET_FRAGMENT(fragment_pointer).next_fragment_pointer = head_fragment_pointer;

    // Make sure writes to our new fragment have completed before we let other invocations know of its existence
    // (by atomically inserting it into the fragment list).
    memoryBarrier();

    // Attempt to store new *head* pointer (to new fragment) into head-pointer image, and return true if successful.
    // This will fail if another invocation has just updated the head pointer in the image.
    return imageAtomicCompSwap(
            scene_tile_pixel_list_head_image,
            ivec2(gl_FragCoord.xy),
            head_fragment_pointer,
            fragment_pointer) == head_fragment_pointer;

    // Note: If successfully inserted then ownership of fragment pointer is transferred to the list head pointer
    //       (otherwise the caller retains ownership, and can try inserting again).
    //       If successfully inserted then ownership of the original head pointer is transferred to the inserted fragment's next pointer
    //       (otherwise ownership stays with the head of the list, noting that the list head hasn't changed).
    //       This doesn't affect the caller's ownership of its head fragment pointer
    //       (the caller also owns a reference, in addition to the list owning its own reference).
}

/*
 * Insert a fragment after an existing fragment of the fragment list at the current fragment coordinate.
 *
 * Returns false if unable to update next pointer of previous fragment because another invocation just updated it
 * (ie, if the next pointer of 'previous_fragment_pointer' is not 'next_fragment_pointer').
 *
 * If successful then 'fragment_pointer' reference ownership is transferred into the list.
 * The caller should also own (have acquired) 'previous_fragment_pointer' and 'next_fragment_pointer' (but their ownership is not transferred),
 * that just ensures those fragments are not reclaimed by other invocations (while we're still referencing them).
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
    SCENE_TILE_GET_FRAGMENT(fragment_pointer).next_fragment_pointer = next_fragment_pointer;

    // Make sure writes to our new fragment have completed before we let other invocations know of its existence
    // (by atomically inserting it into the fragment list).
    memoryBarrier();

    // Attempt to store new pointer (to new fragment) in *previous* fragment's next pointer, and return true if successful.
    // This will fail if another invocation has just updated the next pointer in the previous fragment.
    return atomicCompSwap(
            SCENE_TILE_GET_FRAGMENT(previous_fragment_pointer).next_fragment_pointer,
            next_fragment_pointer,
            fragment_pointer) == next_fragment_pointer;

    // Note: If successfully inserted then ownership of the fragment pointer is transferred to the next pointer of previous fragment
    //       (otherwise the caller retains ownership, and can try inserting again).
    //       If successfully inserted then ownership of the next fragment pointer is transferred to the next pointer of inserted fragment
    //       (otherwise ownership stays with the next pointer of previous fragment, noting that the previous fragment's next pointer hasn't changed).
    //       This doesn't affect the caller's ownership of their previous and next fragment pointers
    //       (the caller also owns those references, in addition to the list owning its own references).
}

/*
 * Add a fragment (of specified colour and depth) to the list of fragments at the current pixel coordinate.
 *
 * Returns false if the fragment is behind an opaque fragment (and hence not visible),
 * or if fragment is fully transparent, or if fragment is a helper invocation.
 */
bool
scene_tile_add_fragment(
        vec4 fragment_colour,
        float fragment_z)  // eg, pass in gl_FragCoord.z
{
    if (gl_HelperInvocation)
    {
        // We're a helper invocation, so nothing to do.
        return false;
    }

    fragment_colour.a = 0.5;

    if (fragment_colour.a == 0)
    {
        // Fragment is completely transparent, and hence not visible.
        return false;
    }

    // Fragment coverage (which samples of the current pixel are covered by the rendered primitive).
    //
    // Only supports a sample count <= 32.
    // However, 32 bits of coverage is enough (we currently only use 4 samples per fragment).
    int fragment_coverage = gl_SampleMaskIn[0];

    // We don't allocate/construct our fragment until we need to.
    // Because we may never add a fragment (if it's behind existing opaque fragments and hence not visible).
    uint fragment_pointer = 0;

    // Track the occlusion caused by the closest fragments at the current pixel coordinate.
    SceneTilePixelOcclusion pixel_occlusion;
    scene_tile_pixel_occlusion_clear(pixel_occlusion);

    // The position in the fragment list.
    uint previous_fragment_pointer = 0;  // start at the head of the list
    uint next_fragment_pointer;

    while (true)
    {
        // Search in order of increasing fragment 'z' to find the previous and next fragments of our fragment.
        if (!scene_tile_search_fragment(pixel_occlusion, previous_fragment_pointer, next_fragment_pointer, fragment_z, fragment_coverage))
        {
            // All samples covered by the current fragment are either behind existing opaque fragments or
            // the list of fragments has exceeded a maximum length.
            //
            // So we won't add the current fragment.

            // Release the fragment (if we've already allocated one).
            // This should deallocate the fragment since we should have the only reference to it.
            //
            // Note: It's possible the fragment has already been allocated (initially its unallocated) because we may have tried to insert it
            //       but failed (because another invocation inserted a fragment just before it) and hence we tried again to search
            //       (where to insert) and found that the fragment is now occluded (by that other invocation's fragment).
            scene_tile_fragment_release(fragment_pointer);

            // Before returning, release our previous and next fragment pointers (they will no longer reference fragments).
            scene_tile_fragment_release(previous_fragment_pointer);
            scene_tile_fragment_release(next_fragment_pointer);

            return false;
        }

        // If we've not allocated/constructed our fragment yet then attempt to do so now.
        // Note: The returned fragment pointer now owns a reference to the new fragment.
        if (fragment_pointer == 0)
        {
            fragment_pointer = scene_tile_fragment_allocate(fragment_colour, fragment_z, fragment_coverage);
        }

        if (fragment_pointer == 0)
        {
            // Before returning, release our previous and next fragment pointers (they will no longer reference fragments).
            scene_tile_fragment_release(previous_fragment_pointer);
            scene_tile_fragment_release(next_fragment_pointer);

            // We were unable to allocate a fragment (ran out of memory), so just return without adding the fragment.
            return false;
        }
        else
        {
            //
            // We've allocated a fragment, so insert it into the fragment list at the current fragment coordinate (pixel)...
            //
            if (previous_fragment_pointer == 0)
            {
                // No previous fragment means we need to insert at the head of the list.
                // Note that the next fragment can also be null (meaning the list is currently empty).
                if (scene_tile_insert_fragment_at_head(fragment_pointer, next_fragment_pointer))
                {
                    // Before returning, release our previous and next fragment pointers (they will no longer reference fragments).
                    // Note: 'fragment_pointer' reference ownership is transferred into the list (so we don't have to release it).
                    scene_tile_fragment_release(previous_fragment_pointer);
                    scene_tile_fragment_release(next_fragment_pointer);

                    return true;
                }
            }
            else
            {
                // There's a previous fragment, so we can insert after it.
                // Note that the next fragment can be null (meaning we're inserting a new tail).
                if (scene_tile_insert_fragment(fragment_pointer, previous_fragment_pointer, next_fragment_pointer))
                {
                    // Before returning, release our previous and next fragment pointers (they will no longer reference fragments).
                    // Note: 'fragment_pointer' reference ownership is transferred into the list (so we don't have to release it).
                    scene_tile_fragment_release(previous_fragment_pointer);
                    scene_tile_fragment_release(next_fragment_pointer);

                    return true;
                }
            }
        }

        // If we get here then another invocation inserted just where we were attempting to insert.
        // So try again from the current position in the fragment list.
        //
        // Note: We don't need to start at the head of the list because we know the incoming fragment 'z' is greater than
        //       the previous fragment 'z', so we can just continue from there. The fragment inserted by the other
        //       invocation is the previous fragment's new *next* fragment (which will get tested in the next search).

        // But before we try again, release our 'next' fragment pointer since the 'previous' fragment pointer is the only
        // fragment pointer that should remain acquired (if non-null) prior to continuing a search (in next loop iteration).
        scene_tile_fragment_release(next_fragment_pointer);
    }
}


//
// The following clear and blend functions are each called in separate draw calls (separate from general scene draw calls that add fragments).
//
// And these draw calls are surrounded by memory barriers (called using the Vulkan API) to ensure shader writes from other draw calls have completed.
//
// The clear function just writes *once* to each memory location (eg, each pixel coordinate in image) and the blend function just *reads* data,
// so there's no need for atomic operations or 'memoryBarrier()' calls (and no need to acquiring/releasing fragments to support reclamation).
//


/*
 * Clear the scene tile in preparation for rendering fragments to it.
 *
 * This zeroes the fragment list head image (at the current fragment coordinate) and resets the fragment allocator.
 *
 * This can be used when rendering a fullscreen "quad" to clear all pixels.
 */
void
scene_tile_clear_fragments()
{
    ivec2 fragment_coord = ivec2(gl_FragCoord.xy);

    // Zero fragment list head image at the current fragment coordinate.
    imageStore(
            scene_tile_pixel_list_head_image,
            fragment_coord,
            uvec4(0));
    
    // Only one fragment (coordinate) needs to reset the fragment allocator.
    // Arbitrarily choose the fragment with zero coordinates.
    if (fragment_coord == vec2(0, 0))
    {
        scene_tile_allocator.alloc_fragment_storage_index = 1;    // starts at 1 so 0 can be used as null pointer
        scene_tile_allocator.alloc_fragment_free_list = 0;  // initially empty free list
    }
}

/*
 * Blend the scene tile.
 *
 * This traverses the fragment list (at the current fragment coordinate) and blends the fragments in depth order.
 *
 * This can be used when rendering a fullscreen "quad" to blend all pixels.
 */
vec4
scene_tile_blend_fragments()
{
    vec4 colour = vec4(0);

    // The number of fragments (in the list) that cover the current sample.
    uint num_fragments_covering_sample = 0;

    // Get fragment list head pointer.
    uint current_fragment_pointer = imageLoad(
            scene_tile_pixel_list_head_image,
            ivec2(gl_FragCoord.xy)).r;

    // Traverse the front-to-back-sorted fragments and blend them.
    while (current_fragment_pointer != 0)
    {
        SceneTileFragment fragment = SCENE_TILE_GET_FRAGMENT(current_fragment_pointer);

        // If the current sample is covered by the fragment's coverage mask then blend the fragment's colour.
        //
        // Note: Use of 'gl_SampleID' causes the fragment shader to be invoked per-sample (rather than per-pixel).
        if ((SCENE_TILE_FRAGMENT_GET_COVERAGE(fragment) & (1 << gl_SampleID)) != 0)
        {
            // Extract the fragment colour (from a 32-bit unsigned integer).
            vec4 fragment_colour = unpackUnorm4x8(fragment.colour);
            // Pre-multiply the fragment RGB by its alpha.
            fragment_colour.rgb *= fragment_colour.a;
            // Blend the fragment colour behind the accumulated front-to-back "colour".
            colour += (1 - colour.a) * fragment_colour;

            // If the current fragment is opaque then no subsequent fragments in the list will be visible.
            // This just potentially saves some global memory access/latency.
            if (fragment_colour.a > 0.99)
            {
                break;
            }

            // See if the number of fragments covering the current sample has reached the maximum.
            if (++num_fragments_covering_sample >= scene_tile_max_fragments_per_sample)
            {
                // Ignore the remaining fragments.
                break;
            }
        }

        current_fragment_pointer = fragment.next_fragment_pointer;
    }

    return colour;
}

#endif // SCENE_TILE_GLSL
