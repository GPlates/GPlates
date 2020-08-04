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

#include <boost/bind.hpp>

#include "GLStateStore.h"

#include "GLState.h"


GPlatesOpenGL::GLStateStore::GLStateStore(
		const GLCapabilities &capabilities,
		const GLStateSetStore::non_null_ptr_type &state_set_store,
		const GLStateSetKeys::non_null_ptr_to_const_type &state_set_keys) :
	d_state_set_store(state_set_store),
	d_state_set_keys(state_set_keys),
	d_state_cache(state_cache_type::create())
{
}


GPlatesOpenGL::GLState::shared_ptr_type
GPlatesOpenGL::GLStateStore::allocate_state()
{
	boost::optional<GLState::shared_ptr_type> state = d_state_cache->allocate_object();
	if (!state)
	{
		// No unused 'GLstate' object so create a new one...
		state = d_state_cache->allocate_object(
				GLState::create_unique(
						d_state_set_store,
						d_state_set_keys,
						// This is so 'GLState' objects can allocate through us when cloning themselves...
						shared_from_this()),
				// Calls 'GLState::clear()' every time a GLState object is returned to the cache...
				boost::bind(&GLState::clear, _1));
	}

	return state.get();
}
