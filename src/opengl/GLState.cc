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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include "GLState.h"

#include "GLCapabilities.h"
#include "GLContext.h"
#include "GLStateStore.h"

#include "utils/Profile.h"


GPlatesOpenGL::GLState::GLState(
		const GLStateSetStore::non_null_ptr_type &state_set_store,
		const GLStateSetKeys::non_null_ptr_to_const_type &state_set_keys,
		const GLStateStore::weak_ptr_type &state_store) :
	d_state_set_store(state_set_store),
	d_state_set_keys(state_set_keys),
	d_state_store(state_store),
	d_state_sets(state_set_keys->get_num_state_set_keys()),
	d_state_set_slots(get_num_state_set_slot_flag32s(*state_set_keys))
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
		cloned_state = create(d_state_set_store, d_state_set_keys, d_state_store);
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
	const state_set_ptr_type *const state_sets = &d_state_sets[0];
	state_set_ptr_type *const cloned_state_sets = &cloned_state->d_state_sets[0];
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
	state_set_ptr_type *const state_sets = &d_state_sets[0];
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
							state_sets[state_set_slot32 + bit32] = state_set_ptr_type();
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
GPlatesOpenGL::GLState::apply_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state) const
{
	PROFILE_FUNC();

	apply_state(capabilities, last_applied_state, state_set_slot_flags_type());
}


void
GPlatesOpenGL::GLState::apply_state(
		const GLCapabilities &capabilities,
		GLState &last_applied_state,
		const state_set_slot_flags_type &state_set_slots_mask) const
{
	//
	// NOTE: This code is written in an optimised manner since it shows high on the CPU profile.
	// And there's a bit of copy'n'paste going on from the single slot version of 'apply_state'.
	//

	// Iterate over the state set slots.
	const unsigned int num_state_set_slot_flag32s = d_state_set_slots.size();
	const state_set_slot_flag32_type *const state_set_slots = &d_state_set_slots[0];
	state_set_slot_flag32_type *const last_applied_state_set_slots = &last_applied_state.d_state_set_slots[0];
	const state_set_ptr_type *const state_sets = &d_state_sets[0];
	state_set_ptr_type *const last_applied_state_sets = &last_applied_state.d_state_sets[0];
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
			state_set_slot_flag32_type state_set_slot_flag32 = state_set_slot_flag32_mask &
					(state_set_slot_flag32_to_apply | last_applied_state_set_slots[state_set_slot_flag32_index]);

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
								const state_set_ptr_type &current_state_set = state_sets[state_set_slot];
								state_set_ptr_type &last_applied_state_set = last_applied_state_sets[state_set_slot];

								// Also including a cheap test of pointers since GLState objects can share the
								// same immutable GLStateSet objects - if they are the same object (or both NULL)
								// then there can be no difference in state and hence nothing to apply.
								//
								// Note that both state set slots cannot be null since we've excluded that
								// possibility with the combined flag.
								if (current_state_set != last_applied_state_set)
								{
									if (last_applied_state_set)
									{
										if (current_state_set)
										{
											// Both the current and last applied state sets exist.

											// This is a transition from an existing state to another (possibly different)
											// existing state - if the two states are the same then it's possible for this
											// to do nothing.
											current_state_set->apply_state(capabilities, *last_applied_state_set, last_applied_state);

											// Update the last applied state so subsequent state-sets can see it.
											last_applied_state_set = current_state_set;
										}
										else
										{
											// Only the last applied state set exists - get it to set the default state.
											// This is a transition from an existing state to the default state.
											last_applied_state_set->apply_to_default_state(capabilities, last_applied_state);

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
										current_state_set->apply_from_default_state(capabilities, last_applied_state);

										// Update the last applied state so subsequent state-sets can see it.
										last_applied_state_set = current_state_set;
										// Set the bit flag.
										// Note that this is set immediately after the state is applied
										// in case the state-sets look at it.
										last_applied_state_set_slots[state_set_slot_flag32_index] |= flag32;
									}

									// It's also possible that other state-sets in 'last_applied_state'
									// were modified and they might be in the current group of 32 slots.
									state_set_slot_flag32 = state_set_slot_flag32_mask &
											(state_set_slot_flag32_to_apply | last_applied_state_set_slots[state_set_slot_flag32_index]);
								}
							}
						}
					}
				}
			}
		}
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
	const state_set_ptr_type *const state_sets_to_merge = &state_change.d_state_sets[0];
	state_set_ptr_type *const state_sets = &d_state_sets[0];
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
