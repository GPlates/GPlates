/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#include <iostream>
#include <typeinfo>

#include "GpmlKeyValueDictionary.h"


const GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type
GPlatesPropertyValues::GpmlKeyValueDictionary::deep_clone() const
{
	GpmlKeyValueDictionary::non_null_ptr_type dup = clone();

	// Now we need to clear the dictionary-element vector in the duplicate, before we push-back
	// the cloned elements.
	dup->d_elements.clear();
	std::vector<GpmlKeyValueDictionaryElement>::const_iterator iter, end = d_elements.end();
	for (iter = d_elements.begin(); iter != end; ++iter) {
		dup->d_elements.push_back((*iter).deep_clone());
	}

	return dup;
}


std::ostream &
GPlatesPropertyValues::GpmlKeyValueDictionary::print_to(
		std::ostream &os) const
{
	os << "[ ";

	typedef std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator iterator_type;
	for (iterator_type iter = d_elements.begin(); iter != d_elements.end(); ++iter)
	{
		os << *iter;
	}

	return os << " ]";
}


bool
GPlatesPropertyValues::GpmlKeyValueDictionary::directly_modifiable_fields_equal(
		const GPlatesModel::PropertyValue &other) const
{
	try
	{
		const GpmlKeyValueDictionary &other_casted =
			dynamic_cast<const GpmlKeyValueDictionary &>(other);
		return d_elements == other_casted.d_elements;
	}
	catch (const std::bad_cast &)
	{
		// Should never get here, but doesn't hurt to check.
		return false;
	}
}

