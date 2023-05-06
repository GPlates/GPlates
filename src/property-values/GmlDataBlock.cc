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
#include <boost/ref.hpp>

#include "GmlDataBlock.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/BubbleUpRevisionHandler.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GmlDataBlock::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gml("DataBlock");


std::ostream &
GPlatesPropertyValues::GmlDataBlock::print_to(
		std::ostream &os) const
{
	const GPlatesModel::RevisionedVector<GmlDataBlockCoordinateList> &tuple_list_ = tuple_list();

	os << "[ ";

	bool first = true;
	GPlatesModel::RevisionedVector<GmlDataBlockCoordinateList>::const_iterator tuple_list_iter = tuple_list_.begin();
	GPlatesModel::RevisionedVector<GmlDataBlockCoordinateList>::const_iterator tuple_list_end = tuple_list_.end();
	for ( ; tuple_list_iter != tuple_list_end; ++tuple_list_iter)
	{
		if (first)
		{
			first = false;
		}
		else
		{
			os << " , ";
		}
		os << **tuple_list_iter;
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


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GmlDataBlock::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GmlDataBlock> &gml_data_block)
{
	if (scribe.is_saving())
	{
		// Get the current tuple list.
		std::vector<GmlDataBlockCoordinateList::non_null_ptr_type> tuple_list_;
		for (GmlDataBlockCoordinateList::non_null_ptr_type list_ : gml_data_block->tuple_list())
		{
			tuple_list_.push_back(list_);
		}

		// Save the tuple list.
		scribe.save(TRANSCRIBE_SOURCE, tuple_list_, "tuple_list");
	}
	else // loading
	{
		// Load the tuple list.
		std::vector<GmlDataBlockCoordinateList::non_null_ptr_type> tuple_list_;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, tuple_list_, "tuple_list"))
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		GPlatesModel::ModelTransaction transaction;
		gml_data_block.construct_object(
				boost::ref(transaction),  // non-const ref
				GPlatesModel::RevisionedVector<GmlDataBlockCoordinateList>::create(
						tuple_list_.begin(),
						tuple_list_.end()));
		transaction.commit();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GmlDataBlock::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			// Get the current tuple list.
			std::vector<GmlDataBlockCoordinateList::non_null_ptr_type> tuple_list_;
			for (GmlDataBlockCoordinateList::non_null_ptr_type list_ : tuple_list())
			{
				tuple_list_.push_back(list_);
			}

			// Save the tuple list.
			scribe.save(TRANSCRIBE_SOURCE, tuple_list_, "tuple_list");
		}
		else // loading
		{
			// Load the tuple list.
			std::vector<GmlDataBlockCoordinateList::non_null_ptr_type> tuple_list_;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, tuple_list_, "tuple_list"))
			{
				return scribe.get_transcribe_result();
			}

			// Set the property value.
			tuple_list().assign(tuple_list_.begin(), tuple_list_.end());
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GmlDataBlock>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
