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

#include <boost/utility/compare_pointees.hpp>
#include <iostream>

#include "GpmlHotSpotTrailMark.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

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


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GpmlHotSpotTrailMark::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gpml("HotSpotTrailMark");


const GPlatesPropertyValues::GpmlHotSpotTrailMark::non_null_ptr_type
GPlatesPropertyValues::GpmlHotSpotTrailMark::create(
		const GmlPoint::non_null_ptr_type &position_,
		const boost::optional<GpmlMeasure::non_null_ptr_type> &trail_width_,
		const boost::optional<GmlTimeInstant::non_null_ptr_type> &measured_age_,
		const boost::optional<GmlTimePeriod::non_null_ptr_type> &measured_age_range_)
{
	GPlatesModel::ModelTransaction transaction;
	non_null_ptr_type ptr(
			new GpmlHotSpotTrailMark(
					transaction, position_, trail_width_, measured_age_, measured_age_range_));
	transaction.commit();

	return ptr;
}


void
GPlatesPropertyValues::GpmlHotSpotTrailMark::set_position(
		GmlPoint::non_null_ptr_type pos)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().position.change(
			revision_handler.get_model_transaction(), pos);
	revision_handler.commit();
}


const boost::optional<GPlatesPropertyValues::GpmlMeasure::non_null_ptr_to_const_type>
GPlatesPropertyValues::GpmlHotSpotTrailMark::trail_width() const
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.trail_width)
	{
		return boost::none;
	}

	return GPlatesUtils::static_pointer_cast<const GpmlMeasure>(
			revision.trail_width->get_revisionable());
}


const boost::optional<GPlatesPropertyValues::GpmlMeasure::non_null_ptr_type>
GPlatesPropertyValues::GpmlHotSpotTrailMark::trail_width()
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.trail_width)
	{
		return boost::none;
	}

	return revision.trail_width->get_revisionable();
}


void
GPlatesPropertyValues::GpmlHotSpotTrailMark::set_trail_width(
		GpmlMeasure::non_null_ptr_type tw)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	Revision &revision = revision_handler.get_revision<Revision>();

	if (revision.trail_width)
	{
		revision.trail_width->change(revision_handler.get_model_transaction(), tw);
	}
	else
	{
		revision.trail_width = GPlatesModel::RevisionedReference<GpmlMeasure>::attach(
				revision_handler.get_model_transaction(), *this, tw);
	}

	revision_handler.commit();
}


const boost::optional<GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_to_const_type>
GPlatesPropertyValues::GpmlHotSpotTrailMark::measured_age() const
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.measured_age)
	{
		return boost::none;
	}

	return GPlatesUtils::static_pointer_cast<const GmlTimeInstant>(
			revision.measured_age->get_revisionable());
}


const boost::optional<GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type>
GPlatesPropertyValues::GpmlHotSpotTrailMark::measured_age()
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.measured_age)
	{
		return boost::none;
	}

	return revision.measured_age->get_revisionable();
}


void
GPlatesPropertyValues::GpmlHotSpotTrailMark::set_measured_age(
		GmlTimeInstant::non_null_ptr_type ti)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	Revision &revision = revision_handler.get_revision<Revision>();

	if (revision.measured_age)
	{
		revision.measured_age->change(revision_handler.get_model_transaction(), ti);
	}
	else
	{
		revision.measured_age = GPlatesModel::RevisionedReference<GmlTimeInstant>::attach(
				revision_handler.get_model_transaction(), *this, ti);
	}

	revision_handler.commit();
}


const boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type>
GPlatesPropertyValues::GpmlHotSpotTrailMark::measured_age_range() const
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.measured_age_range)
	{
		return boost::none;
	}

	return GPlatesUtils::static_pointer_cast<const GmlTimePeriod>(
			revision.measured_age_range->get_revisionable());
}


const boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type>
GPlatesPropertyValues::GpmlHotSpotTrailMark::measured_age_range()
{
	const Revision &revision = get_current_revision<Revision>();

	if (!revision.measured_age_range)
	{
		return boost::none;
	}

	return revision.measured_age_range->get_revisionable();
}


void
GPlatesPropertyValues::GpmlHotSpotTrailMark::set_measured_age_range(
		GmlTimePeriod::non_null_ptr_type tp)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	Revision &revision = revision_handler.get_revision<Revision>();

	if (revision.measured_age_range)
	{
		revision.measured_age_range->change(revision_handler.get_model_transaction(), tp);
	}
	else
	{
		revision.measured_age_range = GPlatesModel::RevisionedReference<GmlTimePeriod>::attach(
				revision_handler.get_model_transaction(), *this, tp);
	}

	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GpmlHotSpotTrailMark::print_to(
		std::ostream &os) const
{
	const Revision &revision = get_current_revision<Revision>();

	os << "[ " << *revision.position.get_revisionable() << " , ";
	if (revision.trail_width)
	{
		os << *revision.trail_width->get_revisionable();
	}
	os << " , ";
	if (revision.measured_age)
	{
		os << *revision.measured_age->get_revisionable();
	}
	os << " , ";
	if (revision.measured_age_range)
	{
		os << *revision.measured_age_range->get_revisionable();
	}
	return os << " ]";
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GpmlHotSpotTrailMark::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// In this method we are operating on a (bubble up) cloned version of the current revision.

	if (child_revisionable == revision.position.get_revisionable())
	{
		return revision.position.clone_revision(transaction);
	}
	if (revision.trail_width &&
		child_revisionable == revision.trail_width->get_revisionable())
	{
		return revision.trail_width->clone_revision(transaction);
	}
	if (revision.measured_age &&
		child_revisionable == revision.measured_age->get_revisionable())
	{
		return revision.measured_age->clone_revision(transaction);
	}
	if (revision.measured_age_range &&
		child_revisionable == revision.measured_age_range->get_revisionable())
	{
		return revision.measured_age_range->clone_revision(transaction);
	}

	// The child property value that bubbled up the modification should be one of our children.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

	// To keep compiler happy - won't be able to get past 'Abort()'.
	return GPlatesModel::Revision::non_null_ptr_type(NULL);
}


GPlatesPropertyValues::GpmlHotSpotTrailMark::Revision::Revision(
		GPlatesModel::ModelTransaction &transaction_,
		RevisionContext &child_context_,
		const GmlPoint::non_null_ptr_type &position_,
		const boost::optional<GpmlMeasure::non_null_ptr_type> &trail_width_,
		const boost::optional<GmlTimeInstant::non_null_ptr_type> &measured_age_,
		const boost::optional<GmlTimePeriod::non_null_ptr_type> &measured_age_range_) :
	position(
			GPlatesModel::RevisionedReference<GmlPoint>::attach(
					transaction_, child_context_, position_))
{
	if (trail_width_)
	{
		trail_width = GPlatesModel::RevisionedReference<GpmlMeasure>::attach(
				transaction_, child_context_, trail_width_.get());
	}

	if (measured_age_)
	{
		measured_age = GPlatesModel::RevisionedReference<GmlTimeInstant>::attach(
				transaction_, child_context_, measured_age_.get());
	}

	if (measured_age_range_)
	{
		measured_age_range = GPlatesModel::RevisionedReference<GmlTimePeriod>::attach(
				transaction_, child_context_, measured_age_range_.get());
	}
}


GPlatesPropertyValues::GpmlHotSpotTrailMark::Revision::Revision(
		const Revision &other_,
		boost::optional<RevisionContext &> context_,
		RevisionContext &child_context_) :
	PropertyValue::Revision(context_),
	position(other_.position),
	trail_width(other_.trail_width),
	measured_age(other_.measured_age),
	measured_age_range(other_.measured_age_range)
{
	// Clone data members that were not deep copied.
	position.clone(child_context_);

	if (trail_width)
	{
		trail_width->clone(child_context_);
	}

	if (measured_age)
	{
		measured_age->clone(child_context_);
	}

	if (measured_age_range)
	{
		measured_age_range->clone(child_context_);
	}
}


GPlatesPropertyValues::GpmlHotSpotTrailMark::Revision::Revision(
		const Revision &other_,
		boost::optional<RevisionContext &> context_) :
	PropertyValue::Revision(context_),
	position(other_.position),
	trail_width(other_.trail_width),
	measured_age(other_.measured_age),
	measured_age_range(other_.measured_age_range)
{
}


bool
GPlatesPropertyValues::GpmlHotSpotTrailMark::Revision::equality(
		const GPlatesModel::Revision &other) const
{
	const Revision &other_revision = dynamic_cast<const Revision &>(other);

	return *position.get_revisionable() == *other_revision.position.get_revisionable() &&
			opt_eq(trail_width, other_revision.trail_width) &&
			opt_eq(measured_age, other_revision.measured_age) &&
			opt_eq(measured_age_range, other_revision.measured_age_range) &&
			PropertyValue::Revision::equality(other);
}
