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

#include "GpmlKeyValueDictionary.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/BubbleUpRevisionHandler.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GpmlKeyValueDictionary::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gpml("KeyValueDictionary");


std::ostream &
GPlatesPropertyValues::GpmlKeyValueDictionary::print_to(
		std::ostream &os) const
{
	const GPlatesModel::RevisionedVector<GpmlKeyValueDictionaryElement> &elements_ = elements();

	os << "[ ";

	GPlatesModel::RevisionedVector<GpmlKeyValueDictionaryElement>::const_iterator elements_iter =
			elements_.begin();
	GPlatesModel::RevisionedVector<GpmlKeyValueDictionaryElement>::const_iterator elements_end =
			elements_.end();
	for ( ; elements_iter != elements_end; ++elements_iter)
	{
		os << **elements_iter << ", ";
	}

	return os << " ]";
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GpmlKeyValueDictionary::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// In this method we are operating on a (bubble up) cloned version of the current revision.
	if (child_revisionable == revision.elements.get_revisionable())
	{
		return revision.elements.clone_revision(transaction);
	}

	// The child property value that bubbled up the modification should be one of our children.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

	// To keep compiler happy - won't be able to get past 'Abort()'.
	return GPlatesModel::Revision::non_null_ptr_type(NULL);
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlKeyValueDictionary::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GpmlKeyValueDictionary> &gpml_key_value_dictionary)
{
	if (scribe.is_saving())
	{
		// Save the elements.
		GPlatesModel::RevisionedVector<GpmlKeyValueDictionaryElement>::non_null_ptr_type elements_ =
				&gpml_key_value_dictionary->elements();
		scribe.save(TRANSCRIBE_SOURCE, elements_, "elements");
	}
	else // loading
	{
		// Load the elements.
		GPlatesScribe::LoadRef<GPlatesModel::RevisionedVector<GpmlKeyValueDictionaryElement>::non_null_ptr_type> elements_ =
				scribe.load<GPlatesModel::RevisionedVector<GpmlKeyValueDictionaryElement>::non_null_ptr_type>(TRANSCRIBE_SOURCE, "elements");
		if (!elements_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		GPlatesModel::ModelTransaction transaction;
		gpml_key_value_dictionary.construct_object(
				boost::ref(transaction),  // non-const ref
				elements_);
		transaction.commit();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlKeyValueDictionary::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			// Save the elements.
			GPlatesModel::RevisionedVector<GpmlKeyValueDictionaryElement>::non_null_ptr_type elements_ = &elements();
			scribe.save(TRANSCRIBE_SOURCE, elements_, "elements");
		}
		else // loading
		{
			// Load the elements.
			GPlatesScribe::LoadRef<GPlatesModel::RevisionedVector<GpmlKeyValueDictionaryElement>::non_null_ptr_type> elements_ =
					scribe.load<GPlatesModel::RevisionedVector<GpmlKeyValueDictionaryElement>::non_null_ptr_type>(TRANSCRIBE_SOURCE, "elements");
			if (!elements_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			// Set the property value.
			GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
			revision_handler.get_revision<Revision>().elements.change(
					revision_handler.get_model_transaction(),
					elements_);
			revision_handler.commit();
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GpmlKeyValueDictionary>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
