/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_GLSTATESTORE_H
#define GPLATES_OPENGL_GLSTATESTORE_H

#include <boost/noncopyable.hpp>

#include "GLStateSetKeys.h"
#include "GLStateSetStore.h"

#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Contains @a GLStateStore and @a GLStateSetKeys.
	 */
	class GLStateStore :
			public GPlatesUtils::ReferenceCount<GLStateStore>
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLStateStore> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLStateStore> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLStateStore object.
		 */
		static
		non_null_ptr_type
		create(
				const GLStateSetStore::non_null_ptr_type &state_set_store,
				const GLStateSetKeys::non_null_ptr_to_const_type &state_set_keys)
		{
			return non_null_ptr_type(new GLStateStore(state_set_store, state_set_keys));
		}


		GLStateSetStore::non_null_ptr_type
		get_state_set_store()
		{
			return d_state_set_store;
		}

		GLStateSetKeys::non_null_ptr_to_const_type
		get_state_set_keys() const
		{
			return d_state_set_keys;
		}

	private:

		/**
		 * Used by @a GLState objects to efficiently allocate its state-set objects.
		 */
		GLStateSetStore::non_null_ptr_type d_state_set_store;

		/**
		 * Used by @a GLState objects to determine state-set slots.
		 */
		GLStateSetKeys::non_null_ptr_to_const_type d_state_set_keys;


		//! Constructor.
		GLStateStore(
				const GLStateSetStore::non_null_ptr_type &state_set_store,
				const GLStateSetKeys::non_null_ptr_to_const_type &state_set_keys) :
			d_state_set_store(state_set_store),
			d_state_set_keys(state_set_keys)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLSTATESTORE_H
