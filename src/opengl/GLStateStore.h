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

#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "GLState.h"
#include "GLStateSetKeys.h"
#include "GLStateSetStore.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ObjectCache.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Manages allocation of derived @a GLState classes using an object cache.
	 */
	class GLStateStore :
			public boost::enable_shared_from_this<GLStateStore>,
			private boost::noncopyable
	{
	public:
		//! A convenience typedef for a shared pointer to a @a GLStateStore.
		typedef boost::shared_ptr<GLStateStore> shared_ptr_type;
		typedef boost::shared_ptr<const GLStateStore> shared_ptr_to_const_type;

		//! A convenience typedef for a weak pointer to a @a GLStateStore.
		typedef boost::weak_ptr<GLStateStore> weak_ptr_type;
		typedef boost::weak_ptr<const GLStateStore> weak_ptr_to_const_type;


		/**
		 * Creates a @a GLStateStore object.
		 */
		static
		shared_ptr_type
		create(
				const GLStateSetStore::non_null_ptr_type &state_set_store,
				const GLStateSetKeys::non_null_ptr_to_const_type &state_set_keys)
		{
			return shared_ptr_type(new GLStateStore(state_set_store, state_set_keys));
		}


		/**
		 * Allocates a @a GLState object (that contains no state sets).
		 *
		 * Attempts to reuse an recycled @a GLState object, otherwise creates a new one.
		 *
		 * Note that the returned @a GLState object is recycled when all shared pointers to
		 * it are destroyed.
		 * And when it's recycled 'GLState::clear()' will be called on it - ie, when the last
		 * (returned) shared_ptr to it is destroyed.
		 */
		GLState::shared_ptr_type
		allocate_state();

	private:
		//! Typedef for an object cache of @a GLState objects.
		typedef GPlatesUtils::ObjectCache<GLState> state_cache_type;


		/**
		 * Used by @a GLState objects to efficiently allocate its state-set objects.
		 */
		GLStateSetStore::non_null_ptr_type d_state_set_store;

		/**
		 * Used by @a GLState objects to determine state-set slots.
		 */
		GLStateSetKeys::non_null_ptr_to_const_type d_state_set_keys;

		/**
		 * Constant data shared by instances of @a GLState allocated by us.
		 *
		 * The sharing is a memory optimisation to reduce memory usage of potentially
		 * tens of thousands of @a GLState objects.
		 */
		GLState::SharedData::shared_ptr_type d_state_shared_data;

		//! Cache of @a GLState objects.
		state_cache_type::shared_ptr_type d_state_cache;


		//! Constructor.
		GLStateStore(
				const GLStateSetStore::non_null_ptr_type &state_set_store,
				const GLStateSetKeys::non_null_ptr_to_const_type &state_set_keys);
	};
}

#endif // GPLATES_OPENGL_GLSTATESTORE_H
