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

#include "model/BubbleUpRevisionHandler.h"
#include "model/ModelTransaction.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GpmlTopologicalNetwork::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gpml("TopologicalNetwork");


std::ostream &
GPlatesPropertyValues::GpmlTopologicalNetwork::print_to(
		std::ostream &os) const
{
	os << "[ ";

		const GPlatesModel::RevisionedVector<GpmlTopologicalSection> &boundary_sections_ = boundary_sections();

		os << "{ ";

			GPlatesModel::RevisionedVector<GpmlTopologicalSection>::const_iterator boundary_sections_iter = boundary_sections_.begin();
			GPlatesModel::RevisionedVector<GpmlTopologicalSection>::const_iterator boundary_sections_end = boundary_sections_.end();
			for ( ; boundary_sections_iter != boundary_sections_end; ++boundary_sections_iter)
			{
				os << *boundary_sections_iter->get();
			}

		os << " }, ";

		const GPlatesModel::RevisionedVector<GpmlPropertyDelegate> &interior_geometries_ = interior_geometries();

		os << "{ ";

			GPlatesModel::RevisionedVector<GpmlPropertyDelegate>::const_iterator interior_geometries_iter = interior_geometries_.begin();
			GPlatesModel::RevisionedVector<GpmlPropertyDelegate>::const_iterator interior_geometries_end = interior_geometries_.end();
			for ( ; interior_geometries_iter != interior_geometries_end; ++interior_geometries_iter)
			{
				os << *interior_geometries_iter->get();
			}

		os << " }";

	return os << " ]";
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GpmlTopologicalNetwork::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// In this method we are operating on a (bubble up) cloned version of the current revision.
	if (child_revisionable == revision.boundary_sections.get_revisionable())
	{
		return revision.boundary_sections.clone_revision(transaction);
	}
	if (child_revisionable == revision.interior_geometries.get_revisionable())
	{
		return revision.interior_geometries.clone_revision(transaction);
	}

	// The child property value that bubbled up the modification should be one of our children.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

	// To keep compiler happy - won't be able to get past 'Abort()'.
	return GPlatesModel::Revision::non_null_ptr_type(NULL);
}
