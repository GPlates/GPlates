/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include "GpmlTopologicalNetwork.h"


namespace
{
	bool
	section_eq(
			const GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_to_const_type &p1,
			const GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_to_const_type &p2)
	{
		return *p1 == *p2;
	}
}


const GPlatesPropertyValues::GpmlTopologicalNetwork::non_null_ptr_type
GPlatesPropertyValues::GpmlTopologicalNetwork::deep_clone() const
{
	GpmlTopologicalNetwork::non_null_ptr_type dup = clone();

	// Now we need to clear the boundary-sections vector in the duplicate, before we
	// push-back the cloned sections.
	dup->d_boundary_sections.clear();
	boundary_sections_const_iterator boundary_sections_iter = d_boundary_sections.begin();
	const boundary_sections_const_iterator boundary_sections_iter_end = d_boundary_sections.end();
	for ( ; boundary_sections_iter != boundary_sections_iter_end; ++boundary_sections_iter)
	{
		GpmlTopologicalSection::non_null_ptr_type cloned_section =
				(*boundary_sections_iter)->deep_clone_as_topo_section();
		dup->d_boundary_sections.push_back(cloned_section);
	}

	// Now we need to clear the interior-geometries vector in the duplicate, before we
	// push-back the cloned geometries.
	dup->d_interior_geometries.clear();
	interior_geometries_const_iterator interior_geometries_iter = d_interior_geometries.begin();
	const interior_geometries_const_iterator interior_geometries_iter_end = d_interior_geometries.end();
	for ( ; interior_geometries_iter != interior_geometries_iter_end; ++interior_geometries_iter)
	{
		const Interior &cloned_interior_geometry = interior_geometries_iter->deep_clone();
		dup->d_interior_geometries.push_back(cloned_interior_geometry);
	}

	return dup;
}


std::ostream &
GPlatesPropertyValues::GpmlTopologicalNetwork::print_to(
		std::ostream &os) const
{
	os << "[ ";

		os << "{ ";

			for (boundary_sections_const_iterator boundary_sections_iter = d_boundary_sections.begin();
				boundary_sections_iter != d_boundary_sections.end();
				++boundary_sections_iter)
			{
				os << **boundary_sections_iter;
			}

		os << " }, ";

		os << "{ ";

			for (interior_geometries_const_iterator interior_geometries_iter = d_interior_geometries.begin();
				interior_geometries_iter != d_interior_geometries.end();
				++interior_geometries_iter)
			{
				os << *interior_geometries_iter;
			}

		os << " }";

	return os << " ]";
}


bool
GPlatesPropertyValues::GpmlTopologicalNetwork::directly_modifiable_fields_equal(
		const GPlatesModel::PropertyValue &other) const
{
	try
	{
		const GpmlTopologicalNetwork &other_casted =
			dynamic_cast<const GpmlTopologicalNetwork &>(other);
		if (d_boundary_sections.size() == other_casted.d_boundary_sections.size() &&
			d_interior_geometries.size() == other_casted.d_interior_geometries.size())
		{
			return
					std::equal(
							d_boundary_sections.begin(),
							d_boundary_sections.end(),
							other_casted.d_boundary_sections.begin(),
							&section_eq) &&
					std::equal(
							d_interior_geometries.begin(),
							d_interior_geometries.end(),
							other_casted.d_interior_geometries.begin());
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


const GPlatesPropertyValues::GpmlTopologicalNetwork::Interior
GPlatesPropertyValues::GpmlTopologicalNetwork::Interior::deep_clone() const
{
	Interior dup(*this);

	const GpmlPropertyDelegate::non_null_ptr_type cloned_source_geometry =
			d_source_geometry->deep_clone();
	dup.d_source_geometry = cloned_source_geometry;

	return dup;
}


bool
GPlatesPropertyValues::GpmlTopologicalNetwork::Interior::operator==(
		const GpmlTopologicalNetwork::Interior &other) const
{
	return *d_source_geometry == *other.d_source_geometry;
}



std::ostream &
GPlatesPropertyValues::operator<<(
		std::ostream &os,
		const GpmlTopologicalNetwork::Interior &topological_network_interior)
{
	return os << *topological_network_interior.get_source_geometry();
}
