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
