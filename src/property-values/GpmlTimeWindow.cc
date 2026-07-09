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

#include "GpmlTimeWindow.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/ModelTransaction.h"
#include "model/TranscribeQualifiedXmlName.h"

#include "scribe/Scribe.h"


GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type
GPlatesPropertyValues::GpmlTimeWindow::create(
		GPlatesModel::PropertyValue::non_null_ptr_type time_dependent_value_,
		GmlTimePeriod::non_null_ptr_type valid_time_,
		const StructuralType &value_type_)
{
	GPlatesModel::ModelTransaction transaction;
	non_null_ptr_type ptr(
			new GpmlTimeWindow(
					transaction, time_dependent_value_, valid_time_, value_type_));
	transaction.commit();

	return ptr;
}


void
GPlatesPropertyValues::GpmlTimeWindow::set_time_dependent_value(
		GPlatesModel::PropertyValue::non_null_ptr_type v)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().time_dependent_value.change(
			revision_handler.get_model_transaction(), v);
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlTimeWindow::set_valid_time(
		GmlTimePeriod::non_null_ptr_type vt)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().valid_time.change(
			revision_handler.get_model_transaction(), vt);
	revision_handler.commit();
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GpmlTimeWindow::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const GPlatesModel::Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// In this method we are operating on a (bubble up) cloned version of the current revision.

	if (child_revisionable == revision.time_dependent_value.get_revisionable())
	{
		return revision.time_dependent_value.clone_revision(transaction);
	}
	if (child_revisionable == revision.valid_time.get_revisionable())
	{
		return revision.valid_time.clone_revision(transaction);
	}

	// The child property value that bubbled up the modification should be one of our children.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

	// To keep compiler happy - won't be able to get past 'Abort()'.
	return GPlatesModel::Revision::non_null_ptr_type(NULL);
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlTimeWindow::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<GpmlTimeWindow> &gpml_time_window)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, gpml_time_window->time_dependent_value(), "value");
		scribe.save(TRANSCRIBE_SOURCE, gpml_time_window->valid_time(), "valid_time");
		scribe.save(TRANSCRIBE_SOURCE, gpml_time_window->get_value_type(), "value_type");
	}
	else // loading
	{
		GPlatesScribe::LoadRef<GPlatesModel::PropertyValue::non_null_ptr_type> value =
				scribe.load<GPlatesModel::PropertyValue::non_null_ptr_type>(TRANSCRIBE_SOURCE, "value");
		if (!value.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		GPlatesScribe::LoadRef<GmlTimePeriod::non_null_ptr_type> valid_time_ =
				scribe.load<GmlTimePeriod::non_null_ptr_type>(TRANSCRIBE_SOURCE, "valid_time");
		if (!valid_time_.is_valid())
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
		gpml_time_window.construct_object(
				boost::ref(transaction),  // non-const ref
				value,
				valid_time_,
				value_type_);
		transaction.commit();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesPropertyValues::GpmlTimeWindow::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, time_dependent_value(), "value");
			scribe.save(TRANSCRIBE_SOURCE, valid_time(), "valid_time");
			scribe.save(TRANSCRIBE_SOURCE, get_value_type(), "value_type");
		}
		else // loading
		{
			GPlatesScribe::LoadRef<GPlatesModel::PropertyValue::non_null_ptr_type> value =
					scribe.load<GPlatesModel::PropertyValue::non_null_ptr_type>(TRANSCRIBE_SOURCE, "value");
			if (!value.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			GPlatesScribe::LoadRef<GmlTimePeriod::non_null_ptr_type> valid_time_ =
					scribe.load<GmlTimePeriod::non_null_ptr_type>(TRANSCRIBE_SOURCE, "valid_time");
			if (!valid_time_.is_valid())
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
			set_time_dependent_value(value);
			set_valid_time(valid_time_);
			d_value_type = value_type_;
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GPlatesModel::PropertyValue, GpmlTimeWindow>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesPropertyValues::GpmlTimeWindow::Revision::Revision(
		GPlatesModel::ModelTransaction &transaction_,
		GPlatesModel::RevisionContext &child_context_,
		GPlatesModel::PropertyValue::non_null_ptr_type time_dependent_value_,
		GmlTimePeriod::non_null_ptr_type valid_time_) :
	time_dependent_value(
			GPlatesModel::RevisionedReference<GPlatesModel::PropertyValue>::attach(
					transaction_, child_context_, time_dependent_value_)),
	valid_time(
			GPlatesModel::RevisionedReference<GmlTimePeriod>::attach(
					transaction_, child_context_, valid_time_))
{
}


GPlatesPropertyValues::GpmlTimeWindow::Revision::Revision(
		const Revision &other_,
		boost::optional<RevisionContext &> context_,
		GPlatesModel::RevisionContext &child_context_) :
	GPlatesModel::Revision(context_),
	time_dependent_value(other_.time_dependent_value),
	valid_time(other_.valid_time)
{
	// Clone data members that were not deep copied.
	time_dependent_value.clone(child_context_);
	valid_time.clone(child_context_);
}


GPlatesPropertyValues::GpmlTimeWindow::Revision::Revision(
		const Revision &other_,
		boost::optional<GPlatesModel::RevisionContext &> context_) :
	GPlatesModel::Revision(context_),
	time_dependent_value(other_.time_dependent_value),
	valid_time(other_.valid_time)
{
}


bool
GPlatesPropertyValues::GpmlTimeWindow::Revision::equality(
		const GPlatesModel::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	// Note that we compare the property value contents (and not pointers).
	return *time_dependent_value.get_revisionable() == *other_revision.time_dependent_value.get_revisionable() &&
			*valid_time.get_revisionable() == *other_revision.valid_time.get_revisionable() &&
			GPlatesModel::Revision::equality(other);
}


std::ostream &
GPlatesPropertyValues::operator<<(
		std::ostream &os,
		const GpmlTimeWindow &time_window)
{
	os << "{ " << *time_window.valid_time() << ": " << *time_window.time_dependent_value() << " }";

	return os;
}
