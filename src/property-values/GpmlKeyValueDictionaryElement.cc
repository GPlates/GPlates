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

#include "GpmlKeyValueDictionaryElement.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/ModelTransaction.h"
#include "model/TranscribeQualifiedXmlName.h"

#include "scribe/Scribe.h"


GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type
GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
		XsString::non_null_ptr_type key_,
		GPlatesModel::PropertyValue::non_null_ptr_type value_,
		const StructuralType &value_type_)
{
	GPlatesModel::ModelTransaction transaction;
	non_null_ptr_type ptr(
			new GpmlKeyValueDictionaryElement(
					transaction, key_, value_, value_type_));
	transaction.commit();
	return ptr;
}


void
GPlatesPropertyValues::GpmlKeyValueDictionaryElement::set_key(
		XsString::non_null_ptr_type key_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().key.change(
			revision_handler.get_model_transaction(), key_);
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlKeyValueDictionaryElement::set_value(
		GPlatesModel::PropertyValue::non_null_ptr_type value_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().value.change(
			revision_handler.get_model_transaction(), value_);
	revision_handler.commit();
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GpmlKeyValueDictionaryElement::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const GPlatesModel::Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// In this method we are operating on a (bubble up) cloned version of the current revision.

	if (child_revisionable == revision.key.get_revisionable())
	{
		return revision.key.clone_revision(transaction);
	}
	if (child_revisionable == revision.value.get_revisionable())
	{
		return revision.value.clone_revision(transaction);
	}

	// The child property value that bubbled up the modification should be one of our children.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

	// To keep compiler happy - won't be able to get past 'Abort()'.
	return GPlatesModel::Revision::non_null_ptr_type(NULL);
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlKeyValueDictionaryElement::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GpmlKeyValueDictionaryElement> &gpml_key_value_dictionary_element)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, gpml_key_value_dictionary_element->key(), "key");
		scribe.save(TRANSCRIBE_SOURCE, gpml_key_value_dictionary_element->value(), "value");
		scribe.save(TRANSCRIBE_SOURCE, gpml_key_value_dictionary_element->get_value_type(), "value_type");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<XsString::non_null_ptr_type> key_ =
				scribe.load<XsString::non_null_ptr_type>(TRANSCRIBE_SOURCE, "key");
		if (!key_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		GPlatesScribe::LoadRef<GPlatesModel::PropertyValue::non_null_ptr_type> value_ =
				scribe.load<GPlatesModel::PropertyValue::non_null_ptr_type>(TRANSCRIBE_SOURCE, "value");
		if (!value_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		GPlatesScribe::LoadRef<StructuralType> value_type_ =
				scribe.load<StructuralType>(TRANSCRIBE_SOURCE, "value_type");
		if (!value_type_.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Create the property value.
		GPlatesModel::ModelTransaction transaction;
		gpml_key_value_dictionary_element.construct_object(
				boost::ref(transaction),  // non-const ref
				key_,
				value_,
				value_type_);
		transaction.commit();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlKeyValueDictionaryElement::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, key(), "key");
			scribe.save(TRANSCRIBE_SOURCE, value(), "value");
			scribe.save(TRANSCRIBE_SOURCE, get_value_type(), "value_type");
		}
		else // loading
		{
			GPlatesScribe::LoadRef<XsString::non_null_ptr_type> key_ =
					scribe.load<XsString::non_null_ptr_type>(TRANSCRIBE_SOURCE, "key");
			if (!key_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			GPlatesScribe::LoadRef<GPlatesModel::PropertyValue::non_null_ptr_type> value_ =
					scribe.load<GPlatesModel::PropertyValue::non_null_ptr_type>(TRANSCRIBE_SOURCE, "value");
			if (!value_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			GPlatesScribe::LoadRef<StructuralType> value_type_ =
					scribe.load<StructuralType>(TRANSCRIBE_SOURCE, "value_type");
			if (!value_type_.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			// Set the property value.
			GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
			Revision &revision = revision_handler.get_revision<Revision>();
			revision.key.change(revision_handler.get_model_transaction(), key_);
			revision.value.change(revision_handler.get_model_transaction(), value_);
			revision_handler.commit();
			d_value_type = value_type_;
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


std::ostream &
GPlatesPropertyValues::operator<<(
		std::ostream &os,
		const GpmlKeyValueDictionaryElement &element)
{
	return os << *element.key() << ":" << *element.value();
}
