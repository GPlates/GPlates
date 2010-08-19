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
 
#ifndef GPLATES_OPENGL_GLSTATE_H
#define GPLATES_OPENGL_GLSTATE_H

#include <vector>

#include "GLStateSet.h"


namespace GPlatesOpenGL
{
	/**
	 * Sets OpenGL state by providing a push/pop framework for state sets.
	 */
	class GLState
	{
	public:
		/**
		 * Pushes @a state_set onto a stack and applies its 'enter_state_set()' method.
		 *
		 * This method directly modifies the OpenGL state.
		 *
		 * All currently pushed state sets together determine the current OpenGL state.
		 */
		void
		push_state_set(
				const GLStateSet::non_null_ptr_to_const_type &state_set);


		/**
		 * Pops the most recently pushed state set from the stack and applies its
		 * 'leave_state_set()' method.
		 *
		 * This method directly modifies the OpenGL state.
		 *
		 * NOTE: This is effectively a partial, fine-grained replacement for glPopAttrib()
		 * because glPopAttrib() apparently has a history of bad driver support where
		 * driver writers have flushed the entire pipeline instead of reconfiguring
		 * the pipeline to effectively undo the most recent call to glPushAttrib() - which
		 * has made glPushAttrib/glPopAttrib noticeably slow on some systems in the past.
		 *
		 * @throws PreconditionViolationError if popped more times than pushed.
		 */
		void
		pop_state_set();

	private:
		//! Typedef for a stack of @a GLStateSet objects.
		typedef std::vector<GLStateSet::non_null_ptr_to_const_type> state_set_stack_type;


		/**
		 * Stack of currently pushed @a GLStateSet objects.
		 */
		state_set_stack_type d_state_set_stack;
	};
}

#endif // GPLATES_OPENGL_GLSTATE_H
