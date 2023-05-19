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

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/ModelTransaction.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GpmlTopologicalPolygon::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gpml("TopologicalPolygon");


std::ostream &
GPlatesPropertyValues::GpmlTopologicalPolygon::print_to(
		std::ostream &os) const
{
	const GPlatesModel::RevisionedVector<GpmlTopologicalSection> &exterior_sections_ = exterior_sections();

	os << "[ ";

	GPlatesModel::RevisionedVector<GpmlTopologicalSection>::const_iterator exterior_sections_iter = exterior_sections_.begin();
	GPlatesModel::RevisionedVector<GpmlTopologicalSection>::const_iterator exterior_sections_end = exterior_sections_.end();
	for ( ; exterior_sections_iter != exterior_sections_end; ++exterior_sections_iter)
	{
		os << **exterior_sections_iter;
	}

	return os << " ]";
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GpmlTopologicalPolygon::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// In this method we are operating on a (bubble up) cloned version of the current revision.
	if (child_revisionable == revision.exterior_sections.get_revisionable())
	{
		return revision.exterior_sections.clone_revision(transaction);
	}

	// The child property value that bubbled up the modification should be one of our children.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

	// To keep compiler happy - won't be able to get past 'Abort()'.
	return GPlatesModel::Revision::non_null_ptr_type(NULL);
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlTopologicalPolygon::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GpmlTopologicalPolygon> &gpml_topological_polygon)
{
	if (scribe.is_saving())
	{
		GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type exterior_sections_ =
				&gpml_topological_polygon->exterior_sections();
		scribe.save(TRANSCRIBE_SOURCE, exterior_sections_, "exterior_sections");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type> exterior_sections_ =
				scribe.load<GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type>(TRANSCRIBE_SOURCE, "exterior_sections");
		if (!exterior_sections_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		GPlatesModel::ModelTransaction transaction;
		gpml_topological_polygon.construct_object(
				boost::ref(transaction),  // non-const ref
				exterior_sections_);
		transaction.commit();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlTopologicalPolygon::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type exterior_sections_ = &exterior_sections();
			scribe.save(TRANSCRIBE_SOURCE, exterior_sections_, "exterior_sections");
		}
		else // loading
		{
			GPlatesScribe::LoadRef<GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type> exterior_sections_ =
					scribe.load<GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type>(TRANSCRIBE_SOURCE, "exterior_sections");
			if (!exterior_sections_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			// Set the property value.
			GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
			Revision &revision = revision_handler.get_revision<Revision>();
			revision.exterior_sections.change(revision_handler.get_model_transaction(), exterior_sections_);
			revision_handler.commit();
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GpmlTopologicalPolygon>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
