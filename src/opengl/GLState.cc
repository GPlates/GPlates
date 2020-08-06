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

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"


GPlatesOpenGL::GLState::GLState(
		const GLCapabilities &capabilities,
		const GLStateStore::non_null_ptr_type &state_store) :
	d_capabilities(capabilities),
	d_state_set_store(state_store->get_state_set_store()),
	d_state_set_keys(state_store->get_state_set_keys()),
	// Note that default state is empty (and remains so)...
	d_default_state(Snapshot::create(*state_store->get_state_set_keys())),
	d_current_state(Snapshot::create(*state_store->get_state_set_keys()))
{
}


void
GPlatesOpenGL::GLState::reset_to_default()
{
	// Apply the default state so that it becomes the current state.
	apply_state(
			*d_default_state,
			// The state slots that change between the current state and default state are actually all
			// the non-null state set slots of the current state because the default state is all nulls...
			d_current_state->state_set_slots);
}


void
GPlatesOpenGL::GLState::save() const
{
	PROFILE_FUNC();

	// Allocate an empty snapshot.
	Snapshot::shared_ptr_type saved_snapshot = Snapshot::create(*d_state_set_keys);

	//
	// Next copy the current state to the snapshot.
	//
	// This includes the GLStateSet object pointers and the flags indicating non-null slots.
	//
	// NOTE: This code is written for optimisation, not simplicity, since it registers high on the CPU profile.
	//

	// Saved state.
	state_set_ptr_type *const saved_state_sets = &saved_snapshot->state_sets[0];
	state_set_slot_flag32_type *const saved_state_set_slots = &saved_snapshot->state_set_slots[0];

	// Current state.
	const state_set_ptr_type *const current_state_sets = &d_current_state->state_sets[0];
	const state_set_slot_flag32_type *const current_state_set_slots = &d_current_state->state_set_slots[0];

	// Iterate over groups of 32 slots.
	const unsigned int num_state_set_slot_flag32s = get_num_state_set_slot_flag32s(*d_state_set_keys);
	for (unsigned int state_set_slot_flag32_index = 0;
		state_set_slot_flag32_index < num_state_set_slot_flag32s;
		++state_set_slot_flag32_index)
	{
		const state_set_slot_flag32_type state_set_slot_flag32 =
				current_state_set_slots[state_set_slot_flag32_index];

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
							saved_state_sets[state_set_slot] = current_state_sets[state_set_slot];
						}
					}
				}
			}

			// Copy the 32 slot flags.
			saved_state_set_slots[state_set_slot_flag32_index] = current_state_set_slots[state_set_slot_flag32_index];
		}
	}

	// We also need to copy the flags indicating which state set slots have been changed between now
	// and the *previous* save (if there wasn't a previous save then it means the change since the
	// default startup state).
	//
	// Similarly we need to reset all the *changed* flags in the current state since it now
	// represents changes since the *current* save (and there are no changes yet).
	//
	// Both these objectives can be achieved with a swap (noting that the *changed* flags in
	// 'saved_snapshot' are currently all disabled).
	saved_snapshot->state_set_slots_changed_since_last_snapshot.swap(
			d_current_state->state_set_slots_changed_since_last_snapshot);

	// Push the saved state onto the save/restore stack.
	d_save_restore_state.push(saved_snapshot);
}


void
GPlatesOpenGL::GLState::restore()
{
	// There should be at least one saved snapshot one the stack.
	// If not then save/restore has been used incorrectly by the caller.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!d_save_restore_state.empty(),
			GPLATES_ASSERTION_SOURCE);

	// Get the most recently saved snapshot (and pop it off the stack).
	Snapshot::shared_ptr_type saved_snapshot = d_save_restore_state.top();
	d_save_restore_state.pop();

	// Apply the state of the saved snapshot so that it becomes the current state.
	apply_state(
			*saved_snapshot,
			// Only those state slots that changed (between saving the snapshot and now) need to be applied...
			d_current_state->state_set_slots_changed_since_last_snapshot);

	// We also need to restore the flags identifying which state set slots have been changed
	// between the most recent save (that we just restored) and the save before that
	// (if there wasn't a save before that then it means the change since the default startup state).
	//
	// Use a swap to copy from saved snapshot to current snapshot.
	// The swap copies current to saved as well (but we're about to discard 'saved_snapshot' anyway).
	d_current_state->state_set_slots_changed_since_last_snapshot.swap(
			saved_snapshot->state_set_slots_changed_since_last_snapshot);
}


void
GPlatesOpenGL::GLState::apply_state(
		const Snapshot &new_state,
		const state_set_slot_flags_type &state_set_slots_changed)
{
	PROFILE_FUNC();

	//
	// NOTE: This code is written for optimisation, not simplicity, since it registers high on the CPU profile.
	//

	// New state.
	const state_set_ptr_type *const new_state_sets = &new_state.state_sets[0];
	const state_set_slot_flag32_type *const new_state_set_slots = &new_state.state_set_slots[0];

	// Current state.
	state_set_ptr_type *const current_state_sets = &d_current_state->state_sets[0];
	state_set_slot_flag32_type *const current_state_set_slots = &d_current_state->state_set_slots[0];

	// State slots changed.
	const state_set_slot_flag32_type *const state_set_slots_changed_array = &state_set_slots_changed[0];

	// Iterate over the state set slots that have changed (between current and new states).
	//
	// Iterate over groups of 32 slots.
	const unsigned int num_state_set_slot_flag32s = get_num_state_set_slot_flag32s(*d_state_set_keys);
	for (unsigned int state_set_slot_flag32_index = 0;
		state_set_slot_flag32_index < num_state_set_slot_flag32s;
		++state_set_slot_flag32_index)
	{
		const state_set_slot_flag32_type state_set_slots_changed_flag32 =
				state_set_slots_changed_array[state_set_slot_flag32_index];

		// Have any of the 32 slots changed state?
		if (state_set_slots_changed_flag32 != 0)
		{
			state_set_slot_flag32_type &current_state_set_slot_flag32 =
					current_state_set_slots[state_set_slot_flag32_index];

			const state_set_key_type state_set_slot32 = (state_set_slot_flag32_index << 5);

			state_set_slot_flag32_type byte_mask = 0xff;
			// Iterate over the 4 groups of 8 slots each.
			for (int i = 0; i < 4; ++i, byte_mask <<= 8)
			{
				// Have any of these 8 slots changed state?
				if ((state_set_slots_changed_flag32 & byte_mask) != 0)
				{
					unsigned int bit32 = (i << 3);
					state_set_slot_flag32_type flag32 = (1 << bit32);

					// Iterate over 8 slots.
					for (int j = 8; --j >= 0; ++bit32, flag32 <<= 1)
					{
						// Has this slot changed state?
						if ((state_set_slots_changed_flag32 & flag32) != 0)
						{
							const state_set_key_type state_set_slot = state_set_slot32 + bit32;

							// Note that either of these could be NULL.
							const state_set_ptr_type &new_state_set = new_state_sets[state_set_slot];
							state_set_ptr_type &current_state_set = current_state_sets[state_set_slot];

							// Also including a cheap test of pointers since GLState objects can share the
							// same immutable GLStateSet objects - if they are the same object (or both NULL)
							// then there can be no difference in state and hence nothing to apply.
							//
							// Note that both state set slots should not be null otherwise that would have
							// been recorded as a change in state.
							if (new_state_set != current_state_set)
							{
								if (current_state_set)
								{
									if (new_state_set)
									{
										// Both state sets exist.

										// This is a transition from an existing state to another (possibly different)
										// existing state - if the two states compare equal then it's possible for this
										// to do nothing.
										new_state_set->apply_state(d_capabilities, *current_state_set, *this/*current_state*/);

										// Update the current state so subsequent state-sets can see it.
										current_state_set = new_state_set;
									}
									else
									{
										// Only the current state set exists - get it to set the default state.
										// This is a transition from an existing state to the default state.
										current_state_set->apply_to_default_state(d_capabilities, *this/*current_state*/);

										// Update the current state so subsequent state-sets can see it since
										// they may wish to query the current state.
										current_state_set.reset();
										current_state_set_slot_flag32 &= ~flag32;  // Clear the bit flag.
									}
								}
								else
								{
									// Since both state-sets cannot be null then 'new_state_set' must be non-null.

									// Only the new state set exists - get it to apply its internal state.
									// This is a transition from the default state to a new state.
									new_state_set->apply_from_default_state(d_capabilities, *this/*current_state*/);

									// Update the current state so subsequent state-sets can see it since
									// they may wish to query the current state.
									current_state_set = new_state_set;
									current_state_set_slot_flag32 |= flag32;  // Set the bit flag.
								}
							}
						}
					}
				}
			}
		}
	}
}


GPlatesOpenGL::GLState::Snapshot::Snapshot(
		const GLStateSetKeys &state_set_keys) :
	state_sets(state_set_keys.get_num_state_set_keys()),
	state_set_slots(GLState::get_num_state_set_slot_flag32s(state_set_keys)),
	state_set_slots_changed_since_last_snapshot(GLState::get_num_state_set_slot_flag32s(state_set_keys))
{
}
