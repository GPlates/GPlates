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

#include <algorithm>
#include <iostream>

#include "GpmlTopologicalNetwork.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/NotYetImplementedException.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/ModelTransaction.h"


void
GPlatesPropertyValues::GpmlTopologicalNetwork::set_boundary_sections(
		const boundary_sections_seq_type &boundary_sections)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().boundary_sections = boundary_sections;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlTopologicalNetwork::set_interior_geometries(
		const interior_geometry_seq_type &interior_geometries)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().interior_geometries = interior_geometries;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GpmlTopologicalNetwork::print_to(
		std::ostream &os) const
{
	os << "[ ";

		os << "{ ";

			const boundary_sections_seq_type &boundary_sections = get_boundary_sections();
			boundary_sections_seq_type::const_iterator boundary_sections_iter = boundary_sections.begin();
			boundary_sections_seq_type::const_iterator boundary_sections_end = boundary_sections.end();
			for ( ; boundary_sections_iter != boundary_sections_end; ++boundary_sections_iter)
			{
				os << *boundary_sections_iter;
			}

		os << " }, ";

		os << "{ ";

			const interior_geometry_seq_type &interior_geometries = get_interior_geometries();
			interior_geometry_seq_type::const_iterator interior_geometries_iter = interior_geometries.begin();
			interior_geometry_seq_type::const_iterator interior_geometries_end = interior_geometries.end();
			for ( ; interior_geometries_iter != interior_geometries_end; ++interior_geometries_iter)
			{
				os << *interior_geometries_iter;
			}

		os << " }";

	return os << " ]";
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GpmlTopologicalNetwork::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Currently this can't be reached because we don't attach to our children yet.
	throw GPlatesGlobal::NotYetImplementedException(GPLATES_EXCEPTION_SOURCE);
}


bool
GPlatesPropertyValues::GpmlTopologicalNetwork::Revision::equality(
		const GPlatesModel::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	if (boundary_sections.size() != other_revision.boundary_sections.size() ||
		interior_geometries.size() != other_revision.interior_geometries.size())
	{
		return false;
	}

	for (unsigned int n = 0; n < boundary_sections.size(); ++n)
	{
		if (boundary_sections[n] != other_revision.boundary_sections[n])
		{
			return false;
		}
	}

	for (unsigned int m = 0; m < interior_geometries.size(); ++m)
	{
		if (interior_geometries[m] != other_revision.interior_geometries[m])
		{
			return false;
		}
	}

	return PropertyValue::Revision::equality(other);
}


std::ostream &
GPlatesPropertyValues::operator<<(
		std::ostream &os,
		const GpmlTopologicalNetwork::BoundarySection &topological_network_boundary_section)
{
	return os << *topological_network_boundary_section.get_source_section();
}


std::ostream &
GPlatesPropertyValues::operator<<(
		std::ostream &os,
		const GpmlTopologicalNetwork::Interior &topological_network_interior)
{
	return os << *topological_network_interior.get_source_geometry();
}
