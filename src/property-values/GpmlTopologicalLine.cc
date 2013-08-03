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

#include <algorithm>
#include <iostream>
#include <boost/foreach.hpp>

#include "GpmlTopologicalLine.h"


std::ostream &
GPlatesPropertyValues::GpmlTopologicalLine::print_to(
		std::ostream &os) const
{
	os << "[ ";

	const sections_seq_type &sections = get_sections();
	sections_seq_type::const_iterator sections_iter = sections.begin();
	sections_seq_type::const_iterator sections_end = sections.end();
	for ( ; sections_iter != sections_end; ++sections_iter)
	{
		os << **sections_iter;
	}

	return os << " ]";
}


GPlatesPropertyValues::GpmlTopologicalLine::Revision::Revision(
		const Revision &other)
{
	// Clone the sections.
	BOOST_FOREACH(const GpmlTopologicalSection::non_null_ptr_to_const_type &other_section, other.sections)
	{
		sections.push_back(other_section->clone());
	}
}


bool
GPlatesPropertyValues::GpmlTopologicalLine::Revision::equality(
		const GPlatesModel::PropertyValue::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	if (sections.size() != other_revision.sections.size())
	{
		return false;
	}

	for (unsigned int n = 0; n < sections.size(); ++n)
	{
		// Compare PropertyValues, not pointers to PropertyValues...
		if (*sections[n] != *other_revision.sections[n])
		{
			return false;
		}
	}

	return GPlatesModel::PropertyValue::Revision::equality(other);
}
