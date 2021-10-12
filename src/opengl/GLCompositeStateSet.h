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
 
#ifndef GPLATES_OPENGL_GLCOMPOSITESTATESET_H
#define GPLATES_OPENGL_GLCOMPOSITESTATESET_H

#include <vector>
#include <opengl/OpenGL.h>

#include "GLStateSet.h"


namespace GPlatesOpenGL
{
	/**
	 * A convenience wrapper around one or more child @a GLStateSet objects.
	 */
	class GLCompositeStateSet :
			public GLStateSet
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLCompositeStateSet> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLCompositeStateSet> non_null_ptr_to_const_type;


		//! Creates a @a GLCompositeStateSet object.
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLCompositeStateSet());
		}


		/**
		 * State sets added will have @a enter_state_set called in the order they are added and
		 * will have @a leave_state_set called in the reverse order.
		 */
		void
		add_state_set(
				const GLStateSet::non_null_ptr_to_const_type &state_set)
		{
			d_state_sets.push_back(state_set);
		}


		virtual
		void
		enter_state_set() const;


		virtual
		void
		leave_state_set() const;

	private:
		//! Typedef for a sequence of @a GLStateSet objects.
		typedef std::vector<GLStateSet::non_null_ptr_to_const_type> state_set_seq_type;

		/**
		 * The state sets we are entering and leaving.
		 */
		state_set_seq_type d_state_sets;


		//! Constructor.
		GLCompositeStateSet()
		{  }
	};
}

#endif // GPLATES_OPENGL_GLCOMPOSITESTATESET_H
