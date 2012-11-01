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
#include <algorithm>

#include "GpmlTopologicalLine.h"


namespace
{
	bool
	section_eq(
			const GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type &p1,
			const GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type &p2)
	{
		return *p1 == *p2;
	}
}


const GPlatesPropertyValues::GpmlTopologicalLine::non_null_ptr_type
GPlatesPropertyValues::GpmlTopologicalLine::deep_clone() const
{
	GpmlTopologicalLine::non_null_ptr_type dup = clone();

	// Now we need to clear the topological-section vector in the duplicate, before we
	// push-back the cloned sections.
	dup->d_sections.clear();
	sections_const_iterator iter = d_sections.begin();
	sections_const_iterator end = d_sections.end();
	for ( ; iter != end; ++iter)
	{
		GpmlTopologicalSection::non_null_ptr_type cloned_section =
				(*iter)->deep_clone_as_topo_section();
		dup->d_sections.push_back(cloned_section);
	}

	return dup;
}


std::ostream &
GPlatesPropertyValues::GpmlTopologicalLine::print_to(
		std::ostream &os) const
{
	os << "[ ";

	for (sections_const_iterator iter = d_sections.begin(); iter != d_sections.end(); ++iter)
	{
		os << **iter;
	}

	return os << " ]";
}


bool
GPlatesPropertyValues::GpmlTopologicalLine::directly_modifiable_fields_equal(
		const GPlatesModel::PropertyValue &other) const
{
	try
	{
		const GpmlTopologicalLine &other_casted =
				dynamic_cast<const GpmlTopologicalLine &>(other);
		if (d_sections.size() == other_casted.d_sections.size())
		{
			return std::equal(
					d_sections.begin(),
					d_sections.end(),
					other_casted.d_sections.begin(),
					&section_eq);
		}
		else
		{
			return false;
		}
	}
	catch (const std::bad_cast &)
	{
		// Should never get here, but doesn't hurt to check.
		return false;
	}
}
