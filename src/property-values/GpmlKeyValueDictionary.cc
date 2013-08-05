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

#include "GpmlKeyValueDictionary.h"


void
GPlatesPropertyValues::GpmlKeyValueDictionary::set_elements(
		const std::vector<GpmlKeyValueDictionaryElement> &elements)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().elements = elements;
	revision_handler.handle_revision_modification();
}


std::ostream &
GPlatesPropertyValues::GpmlKeyValueDictionary::print_to(
		std::ostream &os) const
{
	const std::vector<GpmlKeyValueDictionaryElement> &elements = get_elements();

	os << "[ ";

	for (std::vector<GpmlKeyValueDictionaryElement>::const_iterator elements_iter = elements.begin();
		elements_iter != elements.end();
		++elements_iter)
	{
		os << *elements_iter;
	}

	return os << " ]";
}
