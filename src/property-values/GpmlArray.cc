/**
 * @file
 *
 * Most recent change:
 *   $Date: 2010-11-12 03:12:10 +1100 (Fri, 12 Nov 2010) $
 *
 * Copyright (C) 2010 Geological Survey of Norway
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

#include "GpmlArray.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/TranscribeQualifiedXmlName.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GpmlArray::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gpml("Array");


std::ostream &
GPlatesPropertyValues::GpmlArray::print_to(
                std::ostream &os) const
{
	os << "[ ";

	bool first = true;
	for (GPlatesModel::PropertyValue::non_null_ptr_to_const_type property_value : members())
	{
		if (first)
		{
			first = false;
		}
		else
		{
			os << " , ";
		}
		os << *property_value;
	}

	return os << " ]";
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GpmlArray::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// In this method we are operating on a (bubble up) cloned version of the current revision.
	if (child_revisionable == revision.members.get_revisionable())
	{
		return revision.members.clone_revision(transaction);
	}

	// The child property value that bubbled up the modification should be one of our children.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

	// To keep compiler happy - won't be able to get past 'Abort()'.
	return GPlatesModel::Revision::non_null_ptr_type(NULL);
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlArray::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GpmlArray> &gpml_array)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, gpml_array->get_value_type(), "value_type");

		// Save the members.
		GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue>::non_null_ptr_type members_ = &gpml_array->members();
		scribe.save(TRANSCRIBE_SOURCE, members_, "members");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<StructuralType> value_type_ = scribe.load<StructuralType>(TRANSCRIBE_SOURCE, "value_type");
		if (!value_type_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Load the members.
		GPlatesScribe::LoadRef<GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue>::non_null_ptr_type> members_ =
				scribe.load<GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue>::non_null_ptr_type>(TRANSCRIBE_SOURCE, "members");
		if (!members_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		GPlatesModel::ModelTransaction transaction;
		gpml_array.construct_object(
				boost::ref(transaction),  // non-const ref
				members_,
				value_type_);
		transaction.commit();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlArray::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, get_value_type(), "value_type");

			// Save the members.
			GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue>::non_null_ptr_type members_ = &members();
			scribe.save(TRANSCRIBE_SOURCE, members_, "members");
		}
		else // loading
		{
			GPlatesScribe::LoadRef<StructuralType> value_type_ = scribe.load<StructuralType>(TRANSCRIBE_SOURCE, "value_type");
			if (!value_type_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			// Load the members.
			GPlatesScribe::LoadRef<GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue>::non_null_ptr_type> members_ =
					scribe.load<GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue>::non_null_ptr_type>(TRANSCRIBE_SOURCE, "members");
			if (!members_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			// Set the property value.
			d_value_type = value_type_;
			{
				GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
				revision_handler.get_revision<Revision>().members.change(
						revision_handler.get_model_transaction(),
						members_);
				revision_handler.commit();
			}
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GpmlArray>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
