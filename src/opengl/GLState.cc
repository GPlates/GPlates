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

#include "GLState.h"

#include "global/PreconditionViolationError.h"
#include "global/GPlatesAssert.h"


void
GPlatesOpenGL::GLState::push_state_set(
		const GLStateSet::non_null_ptr_to_const_type &state_set)
{
	// Push the new state set onto the stack.
	d_state_set_stack.push_back(state_set);

	// Get the state set to modify OpenGL state.
	state_set->enter_state_set();
}


void
GPlatesOpenGL::GLState::pop_state_set()
{
	// We should always have at least one entry on the stack.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!d_state_set_stack.empty(),
			GPLATES_EXCEPTION_SOURCE);

	const GLStateSet::non_null_ptr_to_const_type &state_set = d_state_set_stack.back();

	// Get the state set to modify OpenGL state.
	state_set->leave_state_set();

	// Actually pop the state set off the stack.
	d_state_set_stack.pop_back();
}


void
GPlatesOpenGL::draw(
		const GLDrawable::non_null_ptr_to_const_type &drawable,
		const GLStateSet::non_null_ptr_to_const_type &state_set,
		GLState &state)
{
	state.push_state_set(state_set);
	drawable->bind_and_draw();
	state.pop_state_set();
}
