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

#include "scribe/Scribe.h"


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
				os << **boundary_sections_iter;
			}

		os << " }, ";

		const GPlatesModel::RevisionedVector<GpmlPropertyDelegate> &interior_geometries_ = interior_geometries();

		os << "{ ";

			GPlatesModel::RevisionedVector<GpmlPropertyDelegate>::const_iterator interior_geometries_iter = interior_geometries_.begin();
			GPlatesModel::RevisionedVector<GpmlPropertyDelegate>::const_iterator interior_geometries_end = interior_geometries_.end();
			for ( ; interior_geometries_iter != interior_geometries_end; ++interior_geometries_iter)
			{
				os << **interior_geometries_iter;
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


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlTopologicalNetwork::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GpmlTopologicalNetwork> &gpml_topological_network)
{
	if (scribe.is_saving())
	{
		GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type boundary_sections_ =
				&gpml_topological_network->boundary_sections();
		scribe.save(TRANSCRIBE_SOURCE, boundary_sections_, "boundary_sections");

		GPlatesModel::RevisionedVector<GpmlPropertyDelegate>::non_null_ptr_type interior_geometries_ =
				&gpml_topological_network->interior_geometries();
		scribe.save(TRANSCRIBE_SOURCE, interior_geometries_, "interior_geometries");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type> boundary_sections_ =
				scribe.load<GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type>(TRANSCRIBE_SOURCE, "boundary_sections");
		if (!boundary_sections_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		GPlatesScribe::LoadRef<GPlatesModel::RevisionedVector<GpmlPropertyDelegate>::non_null_ptr_type> interior_geometries_ =
				scribe.load<GPlatesModel::RevisionedVector<GpmlPropertyDelegate>::non_null_ptr_type>(TRANSCRIBE_SOURCE, "interior_geometries");
		if (!interior_geometries_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		GPlatesModel::ModelTransaction transaction;
		gpml_topological_network.construct_object(
				boost::ref(transaction),  // non-const ref
				boundary_sections_,
				interior_geometries_);
		transaction.commit();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlTopologicalNetwork::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type boundary_sections_ = &boundary_sections();
			scribe.save(TRANSCRIBE_SOURCE, boundary_sections_, "boundary_sections");

			GPlatesModel::RevisionedVector<GpmlPropertyDelegate>::non_null_ptr_type interior_geometries_ =
					&interior_geometries();
			scribe.save(TRANSCRIBE_SOURCE, interior_geometries_, "interior_geometries");
		}
		else // loading
		{
			GPlatesScribe::LoadRef<GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type> boundary_sections_ =
					scribe.load<GPlatesModel::RevisionedVector<GpmlTopologicalSection>::non_null_ptr_type>(TRANSCRIBE_SOURCE, "boundary_sections");
			if (!boundary_sections_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			GPlatesScribe::LoadRef<GPlatesModel::RevisionedVector<GpmlPropertyDelegate>::non_null_ptr_type> interior_geometries_ =
					scribe.load<GPlatesModel::RevisionedVector<GpmlPropertyDelegate>::non_null_ptr_type>(TRANSCRIBE_SOURCE, "interior_geometries");
			if (!interior_geometries_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			// Set the property value.
			GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
			Revision &revision = revision_handler.get_revision<Revision>();
			revision.boundary_sections.change(revision_handler.get_model_transaction(), boundary_sections_);
			revision.interior_geometries.change(revision_handler.get_model_transaction(), interior_geometries_);
			revision_handler.commit();
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GpmlTopologicalNetwork>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
