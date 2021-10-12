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
 
#ifndef GPLATES_OPENGL_GLENTERORLEAVESTATESET_H
#define GPLATES_OPENGL_GLENTERORLEAVESTATESET_H

#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLStateSet.h"


namespace GPlatesOpenGL
{
	/**
	 * A wrapper around a @a GLStateSet that only enters or leaves the wrapped state set
	 * but not both as a normal state set would do.
	 */
	class GLEnterOrLeaveStateSet :
			public GLStateSet
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLEnterOrLeaveStateSet> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLEnterOrLeaveStateSet> non_null_ptr_to_const_type;


		//! Creates a @a GLEnterOrLeaveStateSet object.
		static
		non_null_ptr_type
		create(
				const GLStateSet::non_null_ptr_to_const_type &state_set,
				bool enter_state_set,
				bool leave_state_set)
		{
			return non_null_ptr_type(new GLEnterOrLeaveStateSet(
					state_set, enter_state_set, leave_state_set));
		}


		virtual
		void
		enter_state_set() const
		{
			if (d_enter_state_set)
			{
				d_state_set->enter_state_set();
			}
		}


		virtual
		void
		leave_state_set() const
		{
			if (d_leave_state_set)
			{
				d_state_set->leave_state_set();
			}
		}

	private:
		/**
		 * The wrapped state set we are entering or leaving.
		 */
		GLStateSet::non_null_ptr_to_const_type d_state_set;

		bool d_enter_state_set;
		bool d_leave_state_set;


		//! Constructor.
		GLEnterOrLeaveStateSet(
				const GLStateSet::non_null_ptr_to_const_type &state_set,
				bool enter_state_set_,
				bool leave_state_set_) :
			d_state_set(state_set),
			d_enter_state_set(enter_state_set_),
			d_leave_state_set(leave_state_set_)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLENTERORLEAVESTATESET_H
