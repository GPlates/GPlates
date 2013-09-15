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

#include "GpmlTimeSample.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/ModelTransaction.h"


namespace
{
	template<class T>
	bool
	opt_eq(
			const boost::optional<GPlatesModel::RevisionedReference<T> > &opt1,
			const boost::optional<GPlatesModel::RevisionedReference<T> > &opt2)
	{
		if (opt1)
		{
			if (!opt2)
			{
				return false;
			}
			return *opt1.get().get_revisionable() == *opt2.get().get_revisionable();
		}
		else
		{
			return !opt2;
		}
	}
}


GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type
GPlatesPropertyValues::GpmlTimeSample::create(
		GPlatesModel::PropertyValue::non_null_ptr_type value_,
		GmlTimeInstant::non_null_ptr_type valid_time_,
		boost::optional<XsString::non_null_ptr_type> description_,
		const StructuralType &value_type_,
		bool is_disabled_)
{
	GPlatesModel::ModelTransaction transaction;
	non_null_ptr_type ptr(
			new GpmlTimeSample(
					transaction, value_, valid_time_, description_, value_type_, is_disabled_));
	transaction.commit();

	return ptr;
}


void
GPlatesPropertyValues::GpmlTimeSample::set_value(
		GPlatesModel::PropertyValue::non_null_ptr_type v)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().value.change(
			revision_handler.get_model_transaction(), v);
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlTimeSample::set_valid_time(
		GmlTimeInstant::non_null_ptr_type vt)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().valid_time.change(
			revision_handler.get_model_transaction(), vt);
	revision_handler.commit();
}


const boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type>
GPlatesPropertyValues::GpmlTimeSample::description() const
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.description)
	{
		return boost::none;
	}

	return GPlatesUtils::static_pointer_cast<const XsString>(
			revision.description->get_revisionable());
}


const boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_type>
GPlatesPropertyValues::GpmlTimeSample::description()
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.description)
	{
		return boost::none;
	}

	return revision.description->get_revisionable();
}


void
GPlatesPropertyValues::GpmlTimeSample::set_description(
		boost::optional<XsString::non_null_ptr_type> description_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	Revision &revision = revision_handler.get_revision<Revision>();

	if (revision.description)
	{
		if (description_)
		{
			revision.description->change(revision_handler.get_model_transaction(), description_.get());
		}
		else
		{
			revision.description->detach(revision_handler.get_model_transaction());
			revision.description = boost::none;
		}
	}
	else if (description_)
	{
		revision.description = GPlatesModel::RevisionedReference<XsString>::attach(
				revision_handler.get_model_transaction(), *this, description_.get());
	}
	// ...else nothing to do.

	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlTimeSample::set_disabled(
		bool is_disabled_)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().is_disabled = is_disabled_;
	revision_handler.commit();
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GpmlTimeSample::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const GPlatesModel::Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// In this method we are operating on a (bubble up) cloned version of the current revision.

	if (child_revisionable == revision.value.get_revisionable())
	{
		return revision.value.clone_revision(transaction);
	}
	if (child_revisionable == revision.valid_time.get_revisionable())
	{
		return revision.valid_time.clone_revision(transaction);
	}
	if (revision.description &&
		child_revisionable == revision.description->get_revisionable())
	{
		return revision.description->clone_revision(transaction);
	}

	// The child property value that bubbled up the modification should be one of our children.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

	// To keep compiler happy - won't be able to get past 'Abort()'.
	return GPlatesModel::Revision::non_null_ptr_type(NULL);
}


GPlatesPropertyValues::GpmlTimeSample::Revision::Revision(
		GPlatesModel::ModelTransaction &transaction_,
		GPlatesModel::RevisionContext &child_context_,
		GPlatesModel::PropertyValue::non_null_ptr_type value_,
		GmlTimeInstant::non_null_ptr_type valid_time_,
		boost::optional<XsString::non_null_ptr_type> description_,
		bool is_disabled_) :
	value(
			GPlatesModel::RevisionedReference<GPlatesModel::PropertyValue>::attach(
					transaction_, child_context_, value_)),
	valid_time(
			GPlatesModel::RevisionedReference<GmlTimeInstant>::attach(
					transaction_, child_context_, valid_time_)),
	is_disabled(is_disabled_)
{
	if (description_)
	{
		description = GPlatesModel::RevisionedReference<XsString>::attach(
				transaction_, child_context_, description_.get());
	}
}


GPlatesPropertyValues::GpmlTimeSample::Revision::Revision(
		const Revision &other_,
		boost::optional<RevisionContext &> context_,
		GPlatesModel::RevisionContext &child_context_) :
	GPlatesModel::Revision(context_),
	value(other_.value),
	valid_time(other_.valid_time),
	description(other_.description),
	is_disabled(other_.is_disabled)
{
	// Clone data members that were not deep copied.
	value.clone(child_context_);
	valid_time.clone(child_context_);
	if (description)
	{
		description->clone(child_context_);
	}
}


GPlatesPropertyValues::GpmlTimeSample::Revision::Revision(
		const Revision &other_,
		boost::optional<GPlatesModel::RevisionContext &> context_) :
	GPlatesModel::Revision(context_),
	value(other_.value),
	valid_time(other_.valid_time),
	description(other_.description),
	is_disabled(other_.is_disabled)
{
}


bool
GPlatesPropertyValues::GpmlTimeSample::Revision::equality(
		const GPlatesModel::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	// Note that we compare the property value contents (and not pointers).
	return *value.get_revisionable() == *other_revision.value.get_revisionable() &&
			*valid_time.get_revisionable() == *other_revision.valid_time.get_revisionable() &&
			opt_eq(description, other_revision.description) &&
			is_disabled == other_revision.is_disabled &&
			GPlatesModel::Revision::equality(other);
}


std::ostream &
GPlatesPropertyValues::operator<<(
		std::ostream &os,
		const GpmlTimeSample &time_sample)
{
	return os << *time_sample.value();
}
