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

#include "GmlDataBlock.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/BubbleUpRevisionHandler.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GmlDataBlock::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gml("DataBlock");


std::ostream &
GPlatesPropertyValues::GmlDataBlock::print_to(
		std::ostream &os) const
{
	const GPlatesModel::RevisionedVector<GmlDataBlockCoordinateList> &tuple_list_ = tuple_list();

	os << "[ ";

	GPlatesModel::RevisionedVector<GmlDataBlockCoordinateList>::const_iterator tuple_list_iter = tuple_list_.begin();
	GPlatesModel::RevisionedVector<GmlDataBlockCoordinateList>::const_iterator tuple_list_end = tuple_list_.end();
	for ( ; tuple_list_iter != tuple_list_end; ++tuple_list_iter)
	{
		os << *tuple_list_iter->get();
	}

	return os << " ]";
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GmlDataBlock::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// In this method we are operating on a (bubble up) cloned version of the current revision.
	if (child_revisionable == revision.tuple_list.get_revisionable())
	{
		return revision.tuple_list.clone_revision(transaction);
	}

	// The child property value that bubbled up the modification should be one of our children.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

	// To keep compiler happy - won't be able to get past 'Abort()'.
	return GPlatesModel::Revision::non_null_ptr_type(NULL);
}
