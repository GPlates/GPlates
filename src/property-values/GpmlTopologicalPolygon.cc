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

#include "GpmlTopologicalPolygon.h"


std::ostream &
GPlatesPropertyValues::GpmlTopologicalPolygon::print_to(
		std::ostream &os) const
{
	os << "[ ";

	const sections_seq_type &exterior_sections = get_exterior_sections();
	sections_seq_type::const_iterator exterior_sections_iter = exterior_sections.begin();
	sections_seq_type::const_iterator exterior_sections_end = exterior_sections.end();
	for ( ; exterior_sections_iter != exterior_sections_end; ++exterior_sections_iter)
	{
		os << **exterior_sections_iter;
	}

	return os << " ]";
}


GPlatesPropertyValues::GpmlTopologicalPolygon::Revision::Revision(
		const Revision &other)
{
	// Clone the exterior sections.
	BOOST_FOREACH(
			const GpmlTopologicalSection::non_null_ptr_to_const_type &other_exterior_section,
			other.exterior_sections)
	{
		exterior_sections.push_back(other_exterior_section->clone());
	}
}


bool
GPlatesPropertyValues::GpmlTopologicalPolygon::Revision::equality(
		const GPlatesModel::PropertyValue::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	if (exterior_sections.size() != other_revision.exterior_sections.size())
	{
		return false;
	}

	for (unsigned int n = 0; n < exterior_sections.size(); ++n)
	{
		// Compare PropertyValues, not pointers to PropertyValues...
		if (*exterior_sections[n] != *other_revision.exterior_sections[n])
		{
			return false;
		}
	}

	return GPlatesModel::PropertyValue::Revision::equality(other);
}
