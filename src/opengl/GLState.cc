/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>

#include "GLState.h"

#include "GLContext.h"
#include "GLStateStore.h"

#include "utils/Profile.h"


GPlatesOpenGL::GLState::GLState(
		const GLStateSetStore::non_null_ptr_type &state_set_store,
		const GLStateSetKeys::non_null_ptr_to_const_type &state_set_keys,
		const SharedData::shared_ptr_to_const_type &shared_data,
		const GLStateStore::weak_ptr_type &state_store) :
	d_state_set_store(state_set_store),
	d_state_set_keys(state_set_keys),
	d_state_store(state_store),
	d_state_sets(state_set_keys->get_num_state_set_keys()),
	d_state_set_slots(get_num_state_set_slot_flag32s(*state_set_keys)),
	d_shared_data(shared_data)
{
}


GPlatesOpenGL::GLState::shared_ptr_type
GPlatesOpenGL::GLState::clone() const
{
	PROFILE_FUNC();

	shared_ptr_type cloned_state;

	//
	// First allocate the cloned state.
	//
	const GLStateStore::shared_ptr_type state_store = d_state_store.lock();
	if (state_store)
	{
		// Allocate using the state store since it still exists (this is more efficient).
		cloned_state = state_store->allocate_state();
	}
	else
	{
		// Allocate on the heap since the state store does not exist anymore.
		cloned_state = create(d_state_set_store, d_state_set_keys, d_shared_data, d_state_store);
	}

	//
	// Next copy the current state to the cloned state.
	//
	// NOTE: This code has been written for optimisation, not simplicity, since it registers high
	// on the CPU profile for some rendering paths.
	//
	const unsigned int num_state_set_slot_flag32s = d_state_set_slots.size();
	const state_set_slot_flag32_type *const state_set_slots = &d_state_set_slots[0];
	state_set_slot_flag32_type *const cloned_state_set_slots = &cloned_state->d_state_set_slots[0];
	const immutable_state_set_ptr_type *const state_sets = &d_state_sets[0];
	immutable_state_set_ptr_type *const cloned_state_sets = &cloned_state->d_state_sets[0];
	// Iterate over groups of 32 slots.
	for (unsigned int state_set_slot_flag32_index = 0;
		state_set_slot_flag32_index < num_state_set_slot_flag32s;
		++state_set_slot_flag32_index)
	{
		const state_set_slot_flag32_type state_set_slot_flag32 =
				state_set_slots[state_set_slot_flag32_index];

		// Are any of the current 32 slots non-null?
		if (state_set_slot_flag32 != 0)
		{
			const state_set_key_type state_set_slot32 = (state_set_slot_flag32_index << 5);

			state_set_slot_flag32_type byte_mask = 0xff;
			// Iterate over the 4 groups of 8 slots each.
			for (int i = 0; i < 4; ++i, byte_mask <<= 8)
			{
				// Are any of the current 8 slots non-null?
				if ((state_set_slot_flag32 & byte_mask) != 0)
				{
					unsigned int bit32 = (i << 3);
					state_set_slot_flag32_type flag32 = (1 << bit32);

					// Iterate over 8 slots.
					for (int j = 8; --j >= 0; ++bit32, flag32 <<= 1)
					{
						// Is the current slot non-null?
						if ((state_set_slot_flag32 & flag32) != 0)
						{
							const state_set_key_type state_set_slot = state_set_slot32 + bit32;

							// Copy the slot's state-set pointer.
							cloned_state_sets[state_set_slot] = state_sets[state_set_slot];
						}
					}
				}
			}

			// Copy the 32 slot flags.
			cloned_state_set_slots[state_set_slot_flag32_index] = state_set_slots[state_set_slot_flag32_index];
		}
	}

	// Return the cloned state.
	return cloned_state;
}


void
GPlatesOpenGL::GLState::clear()
{
	PROFILE_FUNC();

	// Clear any state set slots that are non-null.
	//
	// NOTE: This code has been written for optimisation, not simplicity, since it registers high
	// on the CPU profile for some rendering paths.
	//
	const unsigned int num_state_set_slot_flag32s = d_state_set_slots.size();
	state_set_slot_flag32_type *const state_set_slots = &d_state_set_slots[0];
	immutable_state_set_ptr_type *const state_sets = &d_state_sets[0];
	// Iterate over groups of 32 slots.
	for (unsigned int state_set_slot_flag32_index = 0;
		state_set_slot_flag32_index < num_state_set_slot_flag32s;
		++state_set_slot_flag32_index)
	{
		const state_set_slot_flag32_type state_set_slot_flag32 =
				state_set_slots[state_set_slot_flag32_index];

		// Are any of the current 32 slots non-null?
		if (state_set_slot_flag32 != 0)
		{
			const state_set_key_type state_set_slot32 = (state_set_slot_flag32_index << 5);

			state_set_slot_flag32_type byte_mask = 0xff;
			// Iterate over the 4 groups of 8 slots each.
			for (int i = 0; i < 4; ++i, byte_mask <<= 8)
			{
				// Are any of the current 8 slots non-null?
				if ((state_set_slot_flag32 & byte_mask) != 0)
				{
					unsigned int bit32 = (i << 3);
					state_set_slot_flag32_type flag32 = (1 << bit32);

					// Iterate over 8 slots.
					for (int j = 8; --j >= 0; ++bit32, flag32 <<= 1)
					{
						// Is the current slot non-null?
						if ((state_set_slot_flag32 & flag32) != 0)
						{
							// Clear the slot's state-set pointer.
							state_sets[state_set_slot32 + bit32] = immutable_state_set_ptr_type();
						}
					}
				}
			}

			// Clear 32 slot flags.
			state_set_slots[state_set_slot_flag32_index] = 0;
		}
	}
}


void
GPlatesOpenGL::GLState::set_bind_buffer_object_and_apply(
		const GLBufferObject::shared_ptr_to_const_type &buffer_object,
		GLenum target,
		GLState &last_applied_state)
{
	const state_set_key_type state_set_key = d_state_set_keys->get_bind_buffer_object_key(target);

	set_state_set(
			d_state_set_store->bind_buffer_object_state_sets,
			state_set_key,
			boost::in_place(buffer_object, target));

	// If the buffer object is bound to the vertex element target then it will get recorded into the
	// currently bound vertex array object so we need to track this change with the vertex array object.
	// See http://www.opengl.org/wiki/Vertex_Array_Object for more details.
	if (target == GLBuffer::TARGET_ELEMENT_ARRAY_BUFFER)
	{
		begin_bind_vertex_array_object(last_applied_state);
		apply_state(last_applied_state, state_set_key);
		end_bind_vertex_array_object(last_applied_state);
	}
	else
	{
		apply_state(last_applied_state, state_set_key);
	}
}


void
GPlatesOpenGL::GLState::set_unbind_buffer_object_and_apply(
		GLenum target,
		GLState &last_applied_state)
{
	const state_set_key_type state_set_key = d_state_set_keys->get_bind_buffer_object_key(target);

	set_state_set(
			d_state_set_store->bind_buffer_object_state_sets,
			state_set_key,
			boost::in_place(target));

	// If the buffer object target is the vertex element target then it will get recorded into the
	// currently bound vertex array object so we need to track this change with the vertex array object.
	// See http://www.opengl.org/wiki/Vertex_Array_Object for more details.
	if (target == GLBuffer::TARGET_ELEMENT_ARRAY_BUFFER)
	{
		begin_bind_vertex_array_object(last_applied_state);
		apply_state(last_applied_state, state_set_key);
		end_bind_vertex_array_object(last_applied_state);
	}
	else
	{
		apply_state(last_applied_state, state_set_key);
	}
}


void
GPlatesOpenGL::GLState::apply_state(
		GLState &last_applied_state) const
{
	PROFILE_FUNC();

	// Since a vertex array object contains state such as vertex attribute array buffer bindings,
	// client enable/disable state, etc, we apply it first.
	// Then any state that it might contain gets applied afterwards so that it gets recorded
	// in the bound vertex array object.
	begin_bind_vertex_array_object(last_applied_state);

	// NOTE: This is called twice because it's possible for some state-sets to
	// modify the dependent state-sets when they are being applied. For example, changing the active
	// texture unit when binding a texture - both of which are separate state-sets and the active
	// texture unit state-set might come before the bind texture state-set - in which case the
	// bind texture state-set will effectively override the active texture unit state-set).
	//
	// The first pass excludes the dependent state-sets and the second pass only includes the
	// dependent state-sets.
	//
	// In essence, calling this twice ensures that the state of 'this' is applied properly.

	// First application is for all combined state-sets that are *not* dependent state-sets.
	// Note that we determine the state-set slots *after* applying the bind-vertex-array-object
	// state-set since it can modify the last applied state outside of its slot.
	apply_state(last_applied_state, d_shared_data->inverse_dependent_state_set_slots);

	// Note that the combined state-set slots are recalculated which is good since
	// 'last_applied_state' may have had its dependent state-set slots modified.

	// Second application is for all combined state-sets that are also dependent state-sets.
	// Note that dependent state-sets do not modify other state-sets (so we don't need a third pass).
	apply_state(last_applied_state, d_shared_data->dependent_state_set_slots);

	// Some of the above state targets the currently bound vertex array object if one is bound.
	// So we shadow the state that is currently set in the native OpenGL vertex array object.
	// This is so we know what state to apply/revert the next time it is bound - the client desires
	// it be in a certain state and we are always targeting that state (in case, for example, a
	// vertex element buffer gets bound, which gets recorded in the vertex array object, and we want
	// to remove that recording when the same vertex element buffer gets unbound).
	end_bind_vertex_array_object(last_applied_state);
}


void
GPlatesOpenGL::GLState::apply_state_used_by_gl_clear(
		GLState &last_applied_state) const
{
	// NOTE: There are no bind vertex array object state-sets or dependent state-sets to worry about here.
	// Simple application of the 'glClear' state set slots is all that is required.
	apply_state(last_applied_state, d_shared_data->gl_clear_state_set_slots);
}


void
GPlatesOpenGL::GLState::apply_state_used_by_gl_read_pixels(
		GLState &last_applied_state) const
{
	// NOTE: There are no bind vertex array object state-sets or dependent state-sets to worry about here.
	// Simple application of the 'glReadPixel' state set slots is all that is required.
	apply_state(last_applied_state, d_shared_data->gl_read_pixels_state_set_slots);
}


void
GPlatesOpenGL::GLState::apply_state(
		GLState &last_applied_state,
		const state_set_slot_flags_type &state_set_slots_mask) const
{
	// Note that we want to change the state sets in the order of their slots.
	// This is because it is typically more efficient that way.
	//
	// For example binding a texture to a specific texture unit requires changing the active texture unit.
	// If texturing needs to be enabled on the same texture unit then it's more efficient if it
	// doesn't have to change the active texture unit.
	// In other words by proceeding in linear order we get:
	//
	//   - set active texture to unit 0
	//   - bind a texture on unit 0
	//   - enable texture on unit 0
	//   - set active texture to unit 1
	//   - bind a texture on unit 1
	//   - enable texture on unit 1
	//
	// ...instead of something like...
	//
	//   - set active texture to unit 0
	//   - bind a texture on unit 0
	//   - set active texture to unit 1
	//   - bind a texture on unit 1
	//   - set active texture to unit 0
	//   - enable texture on unit 0
	//   - set active texture to unit 1
	//   - enable texture on unit 1
	//
	// ...since there's less switching of active texture units.

	//
	// NOTE: This code is written in an optimised manner since it shows high on the CPU profile.
	// And there's a bit of copy'n'paste going on from the single slot version of 'apply_state'.
	//

	// Iterate over the state set slots.
	const unsigned int num_state_set_slot_flag32s = d_state_set_slots.size();
	const state_set_slot_flag32_type *const mutable_state_set_slots =
			d_shared_data->mutable_state_set_slots
			? &d_shared_data->mutable_state_set_slots.get()[0]
			: NULL;
	const state_set_slot_flag32_type *const state_set_slots = &d_state_set_slots[0];
	state_set_slot_flag32_type *const last_applied_state_set_slots = &last_applied_state.d_state_set_slots[0];
	const immutable_state_set_ptr_type *const state_sets = &d_state_sets[0];
	immutable_state_set_ptr_type *const last_applied_state_sets = &last_applied_state.d_state_sets[0];
	const state_set_slot_flag32_type *const state_set_slots_mask_array = &state_set_slots_mask[0];

	// Iterate over groups of 32 slots.
	for (unsigned int state_set_slot_flag32_index = 0;
		state_set_slot_flag32_index < num_state_set_slot_flag32s;
		++state_set_slot_flag32_index)
	{
		const state_set_slot_flag32_type state_set_slot_flag32_mask =
				state_set_slots_mask_array[state_set_slot_flag32_index];

		// Are any of the current 32 slots non-null in the mask?
		if (state_set_slot_flag32_mask != 0)
		{
			const state_set_slot_flag32_type state_set_slot_flag32_to_apply =
					state_set_slots[state_set_slot_flag32_index];

			// Include state-set slots that exist in either state (or both).
			// Only those state-sets that don't exist in either state are excluded here (not visited/applied).
			// If state set slot is set in either in 'this' state or the last applied state then apply it.
			state_set_slot_flag32_type state_set_slot_flag32 =
					state_set_slot_flag32_mask &
						(state_set_slot_flag32_to_apply |
							last_applied_state_set_slots[state_set_slot_flag32_index]);

			// Are any of the current 32 slots non-null in the combined flag?
			if (state_set_slot_flag32 != 0)
			{
				const state_set_key_type state_set_slot32 = (state_set_slot_flag32_index << 5);

				state_set_slot_flag32_type byte_mask = 0xff;
				// Iterate over the 4 groups of 8 slots each.
				for (int i = 0; i < 4; ++i, byte_mask <<= 8)
				{
					// Are any of the current 8 slots non-null?
					if ((state_set_slot_flag32 & byte_mask) != 0)
					{
						unsigned int bit32 = (i << 3);
						state_set_slot_flag32_type flag32 = (1 << bit32);

						// Iterate over 8 slots.
						for (int j = 8; --j >= 0; ++bit32, flag32 <<= 1)
						{
							// Is the current slot non-null?
							if ((state_set_slot_flag32 & flag32) != 0)
							{
								const state_set_key_type state_set_slot = state_set_slot32 + bit32;

								// Note that either of these could be NULL.
								const immutable_state_set_ptr_type &current_state_set = state_sets[state_set_slot];
								immutable_state_set_ptr_type &last_applied_state_set = last_applied_state_sets[state_set_slot];

								// Also including a cheap test of pointers since GLState objects can share the
								// same immutable GLStateSet objects - if they are the same object (or both NULL)
								// then there can be no difference in state and hence nothing to apply.
								//
								// NOTE: Not all slots are *immutable* - there are a few exceptions.
								// Also note that both state set slots cannot be null since we've excluded that
								// possibility with the combined flag.
								if (current_state_set == last_applied_state_set)
								{
									if (
										// Continue to the next slot if there are *no* mutable state sets...
										!mutable_state_set_slots ||
										// Continue to the next slot if this slot is *not* a mutable state set slot...
										(mutable_state_set_slots[state_set_slot_flag32_index] & flag32) == 0)
									{
										continue;
									}
								}
								// Note that if we get here then both state-sets cannot be null.

								if (last_applied_state_set)
								{
									if (current_state_set)
									{
										// Both the current and last applied state sets exist.

										// This is a transition from an existing state to another (possibly different)
										// existing state - if the two states are the same then it's possible for this
										// to do nothing.
										current_state_set->apply_state(*last_applied_state_set, last_applied_state);

										// Update the last applied state so subsequent state-sets can see it.
										last_applied_state_set = current_state_set;
									}
									else
									{
										// Only the last applied state set exists - get it to set the default state.
										// This is a transition from an existing state to the default state.
										last_applied_state_set->apply_to_default_state(last_applied_state);

										// Update the last applied state so subsequent state-sets can see it.
										last_applied_state_set.reset();
										// Clear the bit flag.
										// Note that this is set immediately after the state is applied
										// in case the state-sets look at it.
										last_applied_state_set_slots[state_set_slot_flag32_index] &= ~flag32;
									}
								}
								else
								{
									// Since both state-sets cannot be null then 'current_state_set' must be non-null.

									// Only the current state set exists - get it to apply its internal state.
									// This is a transition from the default state to a new state.
									current_state_set->apply_from_default_state(last_applied_state);

									// Update the last applied state so subsequent state-sets can see it.
									last_applied_state_set = current_state_set;
									// Set the bit flag.
									// Note that this is set immediately after the state is applied
									// in case the state-sets look at it.
									last_applied_state_set_slots[state_set_slot_flag32_index] |= flag32;
								}

								// It's also possible that other state-sets in 'last_applied_state'
								// were modified and they might be in the current group of 32 slots.
								state_set_slot_flag32 =
										state_set_slot_flag32_mask &
											(state_set_slot_flag32_to_apply |
												last_applied_state_set_slots[state_set_slot_flag32_index]);
							}
						}
					}
				}
			}
		}
	}
}


void
GPlatesOpenGL::GLState::apply_state(
		GLState &last_applied_state,
		state_set_key_type state_set_key) const
{
	// Find the bit flag for the specified state set key.
	const unsigned int state_set_slot_flag32_index = (state_set_key >> 5);
	const unsigned int bit32 = (state_set_key & 31);
	const state_set_slot_flag32_type flag32 = (1 << bit32);

	// Note that either of these could be NULL.
	const immutable_state_set_ptr_type &current_state_set = d_state_sets[state_set_key];
	immutable_state_set_ptr_type &last_applied_state_set = last_applied_state.d_state_sets[state_set_key];

	// Also including a cheap test of pointers since GLState objects can share the
	// same immutable GLStateSet objects - if they are the same object (or both NULL)
	// then there can be no difference in state and hence nothing to apply.
	//
	// NOTE: Not all slots are *immutable* - there are a few exceptions.
	if (current_state_set == last_applied_state_set)
	{
		if (
			// Continue to the next slot if there are *no* mutable state sets...
			!d_shared_data->mutable_state_set_slots ||
			// Continue to the next slot if both state sets are null...
			!current_state_set ||
			// Continue to the next slot if this slot is *not* a mutable state set slot...
			(d_shared_data->mutable_state_set_slots.get()[state_set_slot_flag32_index] & flag32) == 0)
		{
			return;
		}
	}
	// Note that if we get here then both state-sets cannot be null.
	//
	// This is normally handled by the 'for' loop above (ie, it skips over those state-set
	// slots where both state-sets are null) provided the caller excluded those slots.

	if (last_applied_state_set)
	{
		if (current_state_set)
		{
			// Both the current and last applied state sets exist.

			// This is a transition from an existing state to another (possibly different)
			// existing state - if the two states are the same then it's possible for this
			// to do nothing.
			current_state_set->apply_state(*last_applied_state_set, last_applied_state);

			// Update the last applied state so subsequent state-sets can see it.
			last_applied_state_set = current_state_set;
		}
		else
		{
			// Only the last applied state set exists - get it to set the default state.
			// This is a transition from an existing state to the default state.
			last_applied_state_set->apply_to_default_state(last_applied_state);

			// Update the last applied state so subsequent state-sets can see it.
			last_applied_state_set.reset();
			// Clear the bit flag.
			// Note that this is set immediately after the state is applied
			// in case the state-sets look at it.
			last_applied_state.d_state_set_slots[state_set_slot_flag32_index] &= ~flag32;
		}
	}
	else
	{
		// Since both state-sets cannot be null then 'current_state_set' must be non-null.

		// Only the current state set exists - get it to apply its internal state.
		// This is a transition from the default state to a new state.
		current_state_set->apply_from_default_state(last_applied_state);

		// Update the last applied state so subsequent state-sets can see it.
		last_applied_state_set = current_state_set;
		// Set the bit flag.
		// Note that this is set immediately after the state is applied
		// in case the state-sets look at it.
		last_applied_state.d_state_set_slots[state_set_slot_flag32_index] |= flag32;
	}
}


void
GPlatesOpenGL::GLState::merge_state_change(
		const GLState &state_change)
{
	PROFILE_FUNC();

	// Merge all state set slots of 'state_change' that have been set.
	//
	// NOTE: This code has been written for optimisation, not simplicity, since it registers high
	// on the CPU profile for some rendering paths.
	//
	const unsigned int num_state_set_slot_flag32s = d_state_set_slots.size();
	const state_set_slot_flag32_type *const state_set_slots_to_merge = &state_change.d_state_set_slots[0];
	state_set_slot_flag32_type *const state_set_slots = &d_state_set_slots[0];
	const immutable_state_set_ptr_type *const state_sets_to_merge = &state_change.d_state_sets[0];
	immutable_state_set_ptr_type *const state_sets = &d_state_sets[0];
	// Iterate over groups of 32 slots.
	for (unsigned int state_set_slot_flag32_index = 0;
		state_set_slot_flag32_index < num_state_set_slot_flag32s;
		++state_set_slot_flag32_index)
	{
		const state_set_slot_flag32_type state_set_slot_flag32_to_merge =
				state_set_slots_to_merge[state_set_slot_flag32_index];

		// Are any of the current 32 slots non-null?
		if (state_set_slot_flag32_to_merge != 0)
		{
			const state_set_key_type state_set_slot32_to_merge = (state_set_slot_flag32_index << 5);

			state_set_slot_flag32_type byte_mask = 0xff;
			// Iterate over the 4 groups of 8 slots each.
			for (int i = 0; i < 4; ++i, byte_mask <<= 8)
			{
				// Are any of the current 8 slots non-null?
				if ((state_set_slot_flag32_to_merge & byte_mask) != 0)
				{
					unsigned int bit32 = (i << 3);
					state_set_slot_flag32_type flag32 = (1 << bit32);

					// Iterate over 8 slots.
					for (int j = 8; --j >= 0; ++bit32, flag32 <<= 1)
					{
						// Is the current slot non-null?
						if ((state_set_slot_flag32_to_merge & flag32) != 0)
						{
							const state_set_key_type state_set_slot = state_set_slot32_to_merge + bit32;

							// Copy over the state set from the state change.
							// This is always non-null.
							state_sets[state_set_slot] = state_sets_to_merge[state_set_slot];
						}
					}
				}
			}

			// Mark those slots that have been set.
			state_set_slots[state_set_slot_flag32_index] |= state_set_slot_flag32_to_merge;
		}
	}
}


void
GPlatesOpenGL::GLState::copy_vertex_array_state(
		const GLState &state)
{
	PROFILE_FUNC();

	// Copy only those slots that contain vertex array state...
	//
	// NOTE: This code has been written for optimisation, not simplicity, since it registers high
	// on the CPU profile for some rendering paths.
	//
	const unsigned int num_state_set_slot_flag32s = d_state_set_slots.size();

	const state_set_slot_flag32_type *const state_set_slots_to_copy = &state.d_state_set_slots[0];
	state_set_slot_flag32_type *const state_set_slots = &d_state_set_slots[0];
	const immutable_state_set_ptr_type *const state_sets_to_copy = &state.d_state_sets[0];
	immutable_state_set_ptr_type *const state_sets = &d_state_sets[0];

	const state_set_slot_flags_type &state_set_slots_copy_mask = d_shared_data->vertex_array_state_set_slots;
	const state_set_slot_flag32_type *const state_set_slots_copy_mask_array = &state_set_slots_copy_mask[0];

	// Iterate over groups of 32 slots.
	for (unsigned int state_set_slot_flag32_index = 0;
		state_set_slot_flag32_index < num_state_set_slot_flag32s;
		++state_set_slot_flag32_index)
	{
		const state_set_slot_flag32_type state_set_slot_flag32_copy_mask =
				state_set_slots_copy_mask_array[state_set_slot_flag32_index];

		// Are any of the current 32 slots non-null?
		if (state_set_slot_flag32_copy_mask != 0)
		{
			const state_set_key_type state_set_slot32_to_copy = (state_set_slot_flag32_index << 5);

			state_set_slot_flag32_type byte_mask = 0xff;
			// Iterate over the 4 groups of 8 slots each.
			for (int i = 0; i < 4; ++i, byte_mask <<= 8)
			{
				// Are any of the current 8 slots non-null?
				if ((state_set_slot_flag32_copy_mask & byte_mask) != 0)
				{
					unsigned int bit32 = (i << 3);
					state_set_slot_flag32_type flag32 = (1 << bit32);

					// Iterate over 8 slots.
					for (int j = 8; --j >= 0; ++bit32, flag32 <<= 1)
					{
						// Is the current slot non-null?
						if ((state_set_slot_flag32_copy_mask & flag32) != 0)
						{
							const state_set_key_type state_set_slot = state_set_slot32_to_copy + bit32;

							// Copy over the state set.
							// This is could be null or non-null.
							state_sets[state_set_slot] = state_sets_to_copy[state_set_slot];
						}
					}
				}
			}

			//
			// Mark those slots that have been set (or unset).
			//

			// First clear all flags corresponding to the copy flags.
			state_set_slots[state_set_slot_flag32_index] &= ~state_set_slot_flag32_copy_mask;
			// Next set any flags corresponding to the vertex array flags.
			state_set_slots[state_set_slot_flag32_index] |=
					(state_set_slot_flag32_copy_mask & state_set_slots_to_copy[state_set_slot_flag32_index]);
		}
	}
}


void
GPlatesOpenGL::GLState::begin_bind_vertex_array_object(
		GLState &last_applied_state) const
{
 #ifdef GL_ARB_vertex_array_object // In case old 'glew.h' header
	if (GLEW_ARB_vertex_array_object)
	{
		apply_state(last_applied_state, GLStateSetKeys::KEY_BIND_VERTEX_ARRAY_OBJECT);
	}
#endif
}


void
GPlatesOpenGL::GLState::end_bind_vertex_array_object(
		GLState &last_applied_state) const
{
#ifdef GL_ARB_vertex_array_object // In case old 'glew.h' header
	// If vertex array objects are not supported by the runtime system then nothing to do.
 	if (GLEW_ARB_vertex_array_object)
	{
		// Get the bind vertex array object state-set.
		// Note that we get this from the last applied state as that is the state that OpenGL currently sees.
		const immutable_state_set_ptr_type &bind_vertex_array_object_state_set =
				last_applied_state.d_state_sets[GLStateSetKeys::KEY_BIND_VERTEX_ARRAY_OBJECT];

		// If there's a bind-vertex-array-object state-set then use its resource state otherwise
		// use the vertex array state of the default vertex array object (resource handle zero).
		const shared_ptr_type current_vertex_array_state =
				bind_vertex_array_object_state_set
				// Downcast to the expected GLStateSet derived type...
				? dynamic_cast<const GLBindVertexArrayObjectStateSet &>(*bind_vertex_array_object_state_set)
						.d_current_resource_state
				: d_shared_data->default_vertex_array_object_current_context_state;

		// Update the shadowed vertex array object state to reflect what OpenGL currently sees.
		current_vertex_array_state->copy_vertex_array_state(last_applied_state);
	}
#endif
}


unsigned int
GPlatesOpenGL::GLState::get_num_state_set_slot_flag32s(
		const GLStateSetKeys &state_set_keys)
{
	// Slot flags go into groups of 32 (since using 32-bit integer bitmasks)...
	return (state_set_keys.get_num_state_set_keys() >> 5) +
		((state_set_keys.get_num_state_set_keys() & 31) != 0 ? 1 : 0);
}


bool
GPlatesOpenGL::GLState::is_state_set_slot_set(
		state_set_slot_flags_type &state_set_slots,
		state_set_key_type state_set_slot)
{
	// Find the bit flag for the specified state set key.
	const unsigned int state_set_slot_flag32_index = (state_set_slot >> 5);
	const unsigned int bit32 = (state_set_slot & 31);
	const state_set_slot_flag32_type flag32 = (1 << bit32);

	return (state_set_slots[state_set_slot_flag32_index] & flag32) != 0;
}


void
GPlatesOpenGL::GLState::set_state_set_slot_flag(
		state_set_slot_flags_type &state_set_slots,
		state_set_key_type state_set_slot)
{
	// Find the bit flag for the specified state set key.
	const unsigned int state_set_slot_flag32_index = (state_set_slot >> 5);
	const unsigned int bit32 = (state_set_slot & 31);
	const state_set_slot_flag32_type flag32 = (1 << bit32);

	state_set_slots[state_set_slot_flag32_index] |= flag32;
}


void
GPlatesOpenGL::GLState::clear_state_set_slot_flag(
		state_set_slot_flags_type &state_set_slots,
		state_set_key_type state_set_slot)
{
	// Find the bit flag for the specified state set key.
	const unsigned int state_set_slot_flag32_index = (state_set_slot >> 5);
	const unsigned int bit32 = (state_set_slot & 31);
	const state_set_slot_flag32_type flag32 = (1 << bit32);

	state_set_slots[state_set_slot_flag32_index] &= ~flag32;
}


GPlatesOpenGL::GLState::SharedData::SharedData(
		const GLStateSetKeys &state_set_keys,
		const GLState::shared_ptr_type &default_vertex_array_object_current_context_state_) :
	dependent_state_set_slots(get_num_state_set_slot_flag32s(state_set_keys)),
	inverse_dependent_state_set_slots(get_num_state_set_slot_flag32s(state_set_keys)),
	vertex_array_state_set_slots(get_num_state_set_slot_flag32s(state_set_keys)),
	inverse_vertex_array_state_set_slots(get_num_state_set_slot_flag32s(state_set_keys)),
	gl_clear_state_set_slots(get_num_state_set_slot_flag32s(state_set_keys)),
	gl_read_pixels_state_set_slots(get_num_state_set_slot_flag32s(state_set_keys)),
	default_vertex_array_object_current_context_state(default_vertex_array_object_current_context_state_)
{
	initialise_dependent_state_set_slots(state_set_keys);
	initialise_vertex_array_state_set_slots(state_set_keys);
	initialise_gl_clear_state_set_slots(state_set_keys);
	initialise_gl_read_pixels_state_set_slots(state_set_keys);
	initialise_mutable_state_set_slots(state_set_keys);
}


void
GPlatesOpenGL::GLState::SharedData::initialise_dependent_state_set_slots(
		const GLStateSetKeys &state_set_keys)
{
	// There are a handful of state sets that need to be treated as special cases.
	// These states are used to direct where other *global* states should be written to.
	// For example the active texture unit directs which texture unit the next 'bind texture'
	// state should apply to. Another example is the bound array buffer which is used when
	// setting the vertex attribute arrays so they know which vertex buffer to bind to.
	//
	// A second pass of state application is required for these states since other states
	// can manipulate them (and possibly after they've already been applied).
	// For example:
	//  - "active texture unit" sets active unit to 1, then
	//  - "bind texture' sets active unit to 0 and binds a texture to it.
	// ...but the active texture unit is left at unit 0 instead of unit 1, so the second
	// pass only runs the "active texture unit" state-set and sets unit 1 as the active unit.
	//
	// Global states was highlighted above to differentiate from the *local* state in OpenGL
	// objects such as buffer objects, textures, etc. There are lots of objects that need to be
	// bound first in order to modify their local state but that's the local object state and
	// not other global state.
	//
	// You can see the anonymous functions at the top of 'GLStateSets.cc' that handle the
	// setting and resetting of these specific states.
	//
	// NOTE: These are also the only state modifications that "GLStateSet::apply_state()" can make
	// through its 'last_applied_state' function argument. In other words these are the only
	// modifications to the last applied state that can be made *while* applying the state.
	std::vector<bool> dependent_slots(state_set_keys.get_num_state_set_keys());
	dependent_slots[GLStateSetKeys::KEY_MATRIX_MODE] = true;
	dependent_slots[GLStateSetKeys::KEY_ACTIVE_TEXTURE] = true;
	dependent_slots[GLStateSetKeys::KEY_BIND_ARRAY_BUFFER_OBJECT] = true;

	// Iterate over all the slots and add to the dependent or inverse-dependent sequence.
	for (state_set_key_type state_set_slot = 0;
		state_set_slot < state_set_keys.get_num_state_set_keys();
		++state_set_slot)
	{
		// Remove the bind vertex array object state-set slot from both since it gets its own apply pass.
		if (state_set_slot == GLStateSetKeys::KEY_BIND_VERTEX_ARRAY_OBJECT)
		{
			continue;
		}

		if (dependent_slots[state_set_slot])
		{
			set_state_set_slot_flag(dependent_state_set_slots, state_set_slot);
		}
		else
		{
			set_state_set_slot_flag(inverse_dependent_state_set_slots, state_set_slot);
		}
	}
}


void
GPlatesOpenGL::GLState::SharedData::initialise_vertex_array_state_set_slots(
		const GLStateSetKeys &state_set_keys)
{
	std::vector<bool> vertex_array_slots(state_set_keys.get_num_state_set_keys());

	//
	// All non-generic vertex attribute enable/disable client state.
	//
	vertex_array_slots[GLStateSetKeys::KEY_ENABLE_CLIENT_STATE_COLOR_ARRAY] = true;
	vertex_array_slots[GLStateSetKeys::KEY_ENABLE_CLIENT_STATE_NORMAL_ARRAY] = true;
	vertex_array_slots[GLStateSetKeys::KEY_ENABLE_CLIENT_STATE_VERTEX_ARRAY] = true;
	// Iterate over the enable texture coordinate client state flags.
	const unsigned int MAX_TEXTURE_COORDS = GLContext::get_parameters().texture.gl_max_texture_coords;
	for (unsigned int texture_coord_index = 0; texture_coord_index < MAX_TEXTURE_COORDS; ++texture_coord_index)
	{
		vertex_array_slots[
				state_set_keys.get_enable_client_texture_state_key(GL_TEXTURE0 + texture_coord_index)] = true;
	}

	//
	// All non-generic vertex attribute array state.
	//
	vertex_array_slots[GLStateSetKeys::KEY_VERTEX_ARRAY_COLOR_POINTER] = true;
	vertex_array_slots[GLStateSetKeys::KEY_VERTEX_ARRAY_NORMAL_POINTER] = true;
	vertex_array_slots[GLStateSetKeys::KEY_VERTEX_ARRAY_VERTEX_POINTER] = true;
	// Iterate over the texture coordinate arrays.
	for (unsigned int texture_coord_index = 0; texture_coord_index < MAX_TEXTURE_COORDS; ++texture_coord_index)
	{
		vertex_array_slots[
				state_set_keys.get_tex_coord_pointer_state_key(GL_TEXTURE0 + texture_coord_index)] = true;
	}

	//
	// All generic vertex attribute enable/disable client state.
	//
	const GLuint MAX_VERTEX_ATTRIBS = GLContext::get_parameters().shader.gl_max_vertex_attribs;
	// Iterate over the supported number of generic vertex attribute arrays.
	for (GLuint attribute_index = 0; attribute_index < MAX_VERTEX_ATTRIBS; ++attribute_index)
	{
		vertex_array_slots[
				state_set_keys.get_enable_vertex_attrib_array_key(attribute_index)] = true;
	}

	//
	// All generic vertex attribute array state.
	//
	// Iterate over the supported number of generic vertex attribute arrays.
	for (GLuint attribute_index = 0; attribute_index < MAX_VERTEX_ATTRIBS; ++attribute_index)
	{
		vertex_array_slots[state_set_keys.get_vertex_attrib_array_key(attribute_index)] = true;
	}

	//
	// The vertex element buffer, unlike the vertex buffer, *is* recorded in the vertex array.
	// See http://www.opengl.org/wiki/Vertex_Array_Object for more details.
	//
	vertex_array_slots[GLStateSetKeys::KEY_BIND_ELEMENT_ARRAY_BUFFER_OBJECT] = true;

	// Iterate over all the slots and add to the vertex-array or inverse-vertex-array sequence.
	for (state_set_key_type state_set_slot = 0;
		state_set_slot < state_set_keys.get_num_state_set_keys();
		++state_set_slot)
	{
		if (vertex_array_slots[state_set_slot])
		{
			set_state_set_slot_flag(vertex_array_state_set_slots, state_set_slot);
		}
		else
		{
			set_state_set_slot_flag(inverse_vertex_array_state_set_slots, state_set_slot);
		}
	}
}


void
GPlatesOpenGL::GLState::SharedData::initialise_gl_clear_state_set_slots(
		const GLStateSetKeys &state_set_keys)
{
	// Specify the state set keys representing states needed by 'glClear'.
	// Note that the viewport is not used by 'glClear' (but the scissor test and rectangle are).
	set_state_set_slot_flag(gl_clear_state_set_slots, GLStateSetKeys::KEY_BIND_FRAME_BUFFER);
	set_state_set_slot_flag(gl_clear_state_set_slots, GLStateSetKeys::KEY_CLEAR_COLOR);
	set_state_set_slot_flag(gl_clear_state_set_slots, GLStateSetKeys::KEY_CLEAR_DEPTH);
	set_state_set_slot_flag(gl_clear_state_set_slots, GLStateSetKeys::KEY_CLEAR_STENCIL);
	set_state_set_slot_flag(gl_clear_state_set_slots, GLStateSetKeys::KEY_COLOR_MASK);
	set_state_set_slot_flag(gl_clear_state_set_slots, GLStateSetKeys::KEY_ENABLE_SCISSOR_TEST);
	set_state_set_slot_flag(gl_clear_state_set_slots, GLStateSetKeys::KEY_SCISSOR);
}


void
GPlatesOpenGL::GLState::SharedData::initialise_gl_read_pixels_state_set_slots(
		const GLStateSetKeys &state_set_keys)
{
	// Specify the state set keys representing states needed by 'glReadPixels'.
	set_state_set_slot_flag(gl_read_pixels_state_set_slots, GLStateSetKeys::KEY_BIND_FRAME_BUFFER);
	set_state_set_slot_flag(gl_read_pixels_state_set_slots, GLStateSetKeys::KEY_BIND_PIXEL_PACK_BUFFER_OBJECT);
}


void
GPlatesOpenGL::GLState::SharedData::initialise_mutable_state_set_slots(
		const GLStateSetKeys &state_set_keys)
{
	// If we are emulating vertex buffers and vertex element buffers then it's possible that
	// the buffer data can be updated by the client resulting in a new client memory pointer
	// must then be specified directly to OpenGL.
	// This effectively makes the vertex attribute state-sets mutable because even though
	// we may have already applied the same state-set object when we are asked to apply it again
	// we cannot assume it's a redundant state change and must ask the @a GLStateSet to apply
	// its state just in case the client memory pointer has changed.
	//
	// This is something that real OpenGL buffer objects have to contend with (in the OpenGL driver).
	//
	// UPDATE:
	// This shouldn't be necessary for native buffer objects - seems to work fine without this on
	// nVidia hardware but ATI hardware seems to need it (at least the Macbook AMD HD6750 tested on)
	// - seems needs the vertex array pointers to be rebound whenever 'glBufferData' is called.
	// Maybe this isn't in the spec and nVidia do it anyway - not sure what the spec says?
	// So for now this applies to *both* client memory arrays and native OpenGL buffer objects.
	//

	// Create mutable state-set slots if necessary.
	if (!mutable_state_set_slots)
	{
		mutable_state_set_slots = state_set_slot_flags_type(get_num_state_set_slot_flag32s(state_set_keys));
	}

	// Add all the non-generic attribute array slots.
	set_state_set_slot_flag(mutable_state_set_slots.get(), GLStateSetKeys::KEY_VERTEX_ARRAY_COLOR_POINTER);
	set_state_set_slot_flag(mutable_state_set_slots.get(), GLStateSetKeys::KEY_VERTEX_ARRAY_NORMAL_POINTER);
	set_state_set_slot_flag(mutable_state_set_slots.get(), GLStateSetKeys::KEY_VERTEX_ARRAY_VERTEX_POINTER);

	// Add all texture coordinate pointer slots.
	const GLuint MAX_TEXTURE_COORDS = GLContext::get_parameters().texture.gl_max_texture_coords;
	for (GLuint texture_coord_index = 0; texture_coord_index < MAX_TEXTURE_COORDS; ++texture_coord_index)
	{
		set_state_set_slot_flag(
				mutable_state_set_slots.get(),
				state_set_keys.get_tex_coord_pointer_state_key(GL_TEXTURE0 + texture_coord_index));
	}

	// Add all the generic attribute array slots.
	const GLuint MAX_VERTEX_ATTRIBS = GLContext::get_parameters().shader.gl_max_vertex_attribs;
	for (GLuint attribute_index = 0; attribute_index < MAX_VERTEX_ATTRIBS; ++attribute_index)
	{
		set_state_set_slot_flag(
				mutable_state_set_slots.get(),
				state_set_keys.get_vertex_attrib_array_key(attribute_index));
	}

	// Remove the bind vertex array object state-set slot since it gets its own apply pass.
	if (mutable_state_set_slots)
	{
		clear_state_set_slot_flag(mutable_state_set_slots.get(), GLStateSetKeys::KEY_BIND_VERTEX_ARRAY_OBJECT);
	}
}
