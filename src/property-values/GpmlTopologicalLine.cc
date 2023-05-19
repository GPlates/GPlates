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

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/ModelTransaction.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GpmlTopologicalLine::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gpml("TopologicalLine");


std::ostream &
GPlatesPropertyValues::GpmlTopologicalLine::print_to(
		std::ostream &os) const
{
	const GPlatesModel::RevisionedVector<GpmlTopologicalSection> &sections_ = sections();

	os << "[ ";

	GPlatesModel::RevisionedVector<GpmlTopologicalSection>::const_iterator sections_iter = sections_.begin();
	GPlatesModel::RevisionedVector<GpmlTopologicalSection>::const_iterator sections_end = sections_.end();
	for ( ; sections_iter != sections_end; ++sections_iter)
	{
		os << **sections_iter;
	}

	return os << " ]";
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GpmlTopologicalLine::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// In this method we are operating on a (bubble up) cloned version of the current revision.
	if (child_revisionable == revision.sections.get_revisionable())
	{
		return revision.sections.clone_revision(transaction);
	}

	// The child property value that bubbled up the modification should be one of our children.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

	// To keep compiler happy - won't be able to get past 'Abort()'.
	return GPlatesModel::Revision::non_null_ptr_type(NULL);
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlTopologicalLine::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GpmlTopologicalLine> &gpml_topological_line)
{
	if (scribe.is_saving())
	{
		GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type sections_ = &gpml_topological_line->sections();
		scribe.save(TRANSCRIBE_SOURCE, sections_, "sections");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type> sections_ =
				scribe.load<GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type>(TRANSCRIBE_SOURCE, "sections");
		if (!sections_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		GPlatesModel::ModelTransaction transaction;
		gpml_topological_line.construct_object(
				boost::ref(transaction),  // non-const ref
				sections_);
		transaction.commit();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlTopologicalLine::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type sections_ = &sections();
			scribe.save(TRANSCRIBE_SOURCE, sections_, "sections");
		}
		else // loading
		{
			GPlatesScribe::LoadRef<GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type> sections_ =
					scribe.load<GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type>(TRANSCRIBE_SOURCE, "sections");
			if (!sections_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			// Set the property value.
			GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
			Revision &revision = revision_handler.get_revision<Revision>();
			revision.sections.change(revision_handler.get_model_transaction(), sections_);
			revision_handler.commit();
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GpmlTopologicalLine>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
