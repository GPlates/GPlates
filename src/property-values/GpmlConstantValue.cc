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

#include "GpmlConstantValue.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/ModelTransaction.h"
#include "model/TranscribeQualifiedXmlName.h"

#include "scribe/Scribe.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GpmlConstantValue::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gpml("ConstantValue");


const GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type
GPlatesPropertyValues::GpmlConstantValue::create(
		PropertyValue::non_null_ptr_type value_,
		const StructuralType &value_type_,
		boost::optional<GPlatesUtils::UnicodeString> description_)
{
	GPlatesModel::ModelTransaction transaction;
	non_null_ptr_type ptr(new GpmlConstantValue(transaction, value_, value_type_, description_));
	transaction.commit();

	return ptr;
}


void
GPlatesPropertyValues::GpmlConstantValue::set_value(
		PropertyValue::non_null_ptr_type value_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().value.change(
			revision_handler.get_model_transaction(), value_);
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlConstantValue::set_description(
		boost::optional<GPlatesUtils::UnicodeString> new_description)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().description = new_description;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GpmlConstantValue::print_to(
		std::ostream &os) const
{
	return os << *get_current_revision<Revision>().value.get_revisionable();
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GpmlConstantValue::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// There's only one nested property value so it must be that.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			child_revisionable == revision.value.get_revisionable(),
			GPLATES_ASSERTION_SOURCE);

	// Create a new revision for the child property value.
	return revision.value.clone_revision(transaction);
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlConstantValue::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GpmlConstantValue> &gpml_constant_value)
{
	if (scribe.is_saving())
	{
		// Save the property value.
		scribe.save(TRANSCRIBE_SOURCE, gpml_constant_value->value(), "value");

		// Save the property value type.
		scribe.save(TRANSCRIBE_SOURCE, gpml_constant_value->get_value_type(), "value_type");

		// Save the optional description.
		scribe.save(TRANSCRIBE_SOURCE, gpml_constant_value->get_description(), "description");
	}
	else // loading
	{
		// Load the property value.
		GPlatesScribe::LoadRef<PropertyValue::non_null_ptr_type> value =
				scribe.load<PropertyValue::non_null_ptr_type>(TRANSCRIBE_SOURCE, "value");
		if (!value.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Load the property value type.
		GPlatesScribe::LoadRef<StructuralType> value_type =
				scribe.load<StructuralType>(TRANSCRIBE_SOURCE, "value_type");
		if (!value_type.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Load the optional description.
		boost::optional<GPlatesUtils::UnicodeString> description;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, description, "description"))
		{
			return scribe.get_transcribe_result();
		}

		// Create the property.
		GPlatesModel::ModelTransaction transaction;
		gpml_constant_value.construct_object(
				boost::ref(transaction),  // non-const ref
				value,
				value_type,
				description);
		transaction.commit();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlConstantValue::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			// Save the property value.
			scribe.save(TRANSCRIBE_SOURCE, value(), "value");

			// Save the property value type.
			scribe.save(TRANSCRIBE_SOURCE, get_value_type(), "value_type");

			// Save the optional description.
			scribe.save(TRANSCRIBE_SOURCE, get_description(), "description");
		}
		else // loading
		{
			// Load the property value.
			GPlatesScribe::LoadRef<PropertyValue::non_null_ptr_type> value =
					scribe.load<PropertyValue::non_null_ptr_type>(TRANSCRIBE_SOURCE, "value");
			if (!value.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			// Load the property value type.
			GPlatesScribe::LoadRef<StructuralType> value_type =
					scribe.load<StructuralType>(TRANSCRIBE_SOURCE, "value_type");
			if (!value_type.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			// Load the optional description.
			boost::optional<GPlatesUtils::UnicodeString> description;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, description, "description"))
			{
				return scribe.get_transcribe_result();
			}

			// Set the property.
			set_value(value);
			d_value_type = value_type;
			set_description(description);
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GpmlConstantValue>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
