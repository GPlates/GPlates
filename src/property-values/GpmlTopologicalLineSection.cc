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

#include <boost/ref.hpp>

#include "GpmlTopologicalLineSection.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/ModelTransaction.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GpmlTopologicalLineSection::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gpml("TopologicalLineSection");


const GPlatesPropertyValues::GpmlTopologicalLineSection::non_null_ptr_type
GPlatesPropertyValues::GpmlTopologicalLineSection::create(
		GpmlPropertyDelegate::non_null_ptr_type source_geometry_,
		const bool reverse_order) 
{
	GPlatesModel::ModelTransaction transaction;
	non_null_ptr_type ptr(new GpmlTopologicalLineSection(transaction, source_geometry_, reverse_order));
	transaction.commit();

	return ptr;
}


void
GPlatesPropertyValues::GpmlTopologicalLineSection::set_source_geometry(
		GpmlPropertyDelegate::non_null_ptr_type source_geometry_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().source_geometry.change(
			revision_handler.get_model_transaction(), source_geometry_);
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlTopologicalLineSection::set_reverse_order(
		bool reverse_order)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().reverse_order = reverse_order;
	revision_handler.commit();
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GpmlTopologicalLineSection::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// In this method we are operating on a (bubble up) cloned version of the current revision.

	// The child property value that bubbled up the modification should be one of our children.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			child_revisionable == revision.source_geometry.get_revisionable(),
			GPLATES_ASSERTION_SOURCE);

	return revision.source_geometry.clone_revision(transaction);
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlTopologicalLineSection::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GpmlTopologicalLineSection> &gpml_topological_line_section)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, gpml_topological_line_section->get_source_geometry(), "source_geometry");
		scribe.save(TRANSCRIBE_SOURCE, gpml_topological_line_section->get_reverse_order(), "reverse_order");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<GpmlPropertyDelegate::non_null_ptr_type> source_geometry_ =
				scribe.load<GpmlPropertyDelegate::non_null_ptr_type>(TRANSCRIBE_SOURCE, "source_geometry");
		if (!source_geometry_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		bool reverse_order_;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, reverse_order_, "reverse_order"))
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		GPlatesModel::ModelTransaction transaction;
		gpml_topological_line_section.construct_object(
				boost::ref(transaction),  // non-const ref
				source_geometry_,
				reverse_order_);
		transaction.commit();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlTopologicalLineSection::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, get_source_geometry(), "source_geometry");
			scribe.save(TRANSCRIBE_SOURCE, get_reverse_order(), "reverse_order");
		}
		else // loading
		{
			GPlatesScribe::LoadRef<GpmlPropertyDelegate::non_null_ptr_type> source_geometry_ =
					scribe.load<GpmlPropertyDelegate::non_null_ptr_type>(TRANSCRIBE_SOURCE, "source_geometry");
			if (!source_geometry_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			bool reverse_order_;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, reverse_order_, "reverse_order"))
			{
				return scribe.get_transcribe_result();
			}

			// Set the property value.
			GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
			Revision &revision = revision_handler.get_revision<Revision>();
			revision.source_geometry.change(revision_handler.get_model_transaction(), source_geometry_);
			revision.reverse_order = reverse_order_;
			revision_handler.commit();
		}
	}

	// Transcribe base class.
	if (!scribe.transcribe_base<GpmlTopologicalSection>(TRANSCRIBE_SOURCE, *this, "GpmlTopologicalSection"))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
