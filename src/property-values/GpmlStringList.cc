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

#include "GpmlStringList.h"


void
GPlatesPropertyValues::GpmlStringList::set_string_list(
		const string_list_type &strings_)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().strings = strings_;
	revision_handler.handle_revision_modification();
}


void
GPlatesPropertyValues::GpmlStringList::swap(
		string_list_type &strings_)
{
	MutableRevisionHandler revision_handler(this);
	revision_handler.get_mutable_revision<Revision>().strings.swap(strings_);
	revision_handler.handle_revision_modification();
}


std::ostream &
GPlatesPropertyValues::GpmlStringList::print_to(
		std::ostream &os) const
{
	os << "GpmlStringList{";

		const string_list_type &strings = get_string_list();
		string_list_type::const_iterator iter = strings.begin();
		string_list_type::const_iterator end_ = strings.end();
		for ( ; iter != end_; ++iter)
		{
			os << "\"" << (*iter).get() << "\",";
		}

	os << "}";

	return os;
}

