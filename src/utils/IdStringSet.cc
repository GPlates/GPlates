/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "IdStringSet.h"
#include <boost/none.hpp>


GPlatesUtils::IdStringSet::AbstractBackRef::~AbstractBackRef()
{  }


bool
GPlatesUtils::IdStringSet::SharedIterator::operator==(
		const SharedIterator &other) const
{
	if (d_impl_ptr != other.d_impl_ptr)
	{
		// Since the two instances point at different impls, they cannot be equal.
		return false;
	}
	// else, the two instances point at the same impl (or both have NULL impl-pointers).
	
	if (d_impl_ptr.get() == NULL)
	{
		// Both impl-pointers are NULL.  This means that both instances are uninitialised.
		//
		// We'll implement this function so that all uninitialised instances compare equal
		// so that it's possible to determine whether a given instance may be dereferenced.
		return true;
	}

	return (d_iter == other.d_iter);
}


void
GPlatesUtils::IdStringSet::SharedIterator::increment_ref_count()
{
	if (d_impl_ptr.get() == NULL)
	{
		// This instance is uninitialised.
		return;
	}
	++(d_iter->d_ref_count);
}


void
GPlatesUtils::IdStringSet::SharedIterator::decrement_ref_count()
{
	if (d_impl_ptr.get() == NULL)
	{
		// This instance is uninitialised.
		return;
	}
	if (--(d_iter->d_ref_count) == 0)
	{
		// There are no more references to the element in the set.
		d_impl_ptr->collection().erase(d_iter);
	}
}


const boost::optional<GPlatesUtils::IdStringSet::SharedIterator>
GPlatesUtils::IdStringSet::contains(
		const UnicodeString &s) const
{
	UnicodeStringAndRefCountWithBackRef elem(s);
	collection_type::iterator iter = d_impl->collection().find(elem);
	if (iter != d_impl->collection().end())
	{
		// The element already exists in the set.
		SharedIterator sh_iter(iter, d_impl);
		return sh_iter;
	}
	else
	{
		return boost::none;
	}
}


GPlatesUtils::IdStringSet::SharedIterator
GPlatesUtils::IdStringSet::insert(
		const UnicodeString &s)
{
	UnicodeStringAndRefCountWithBackRef elem(s);
	collection_type::iterator iter = d_impl->collection().find(elem);
	if (iter != d_impl->collection().end())
	{
		// The element already exists in the set.
		SharedIterator sh_iter(iter, d_impl);
		return sh_iter;
	}
	else
	{
		std::pair< collection_type::iterator, bool > insertion = d_impl->collection().insert(elem);
		// Now the element exists in the set.
		SharedIterator sh_iter(insertion.first, d_impl);
		return sh_iter;
	}
}
