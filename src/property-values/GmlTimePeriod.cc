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

#include "GmlTimePeriod.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/BubbleUpRevisionHandler.h"
#include "model/ModelTransaction.h"


const GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type
GPlatesPropertyValues::GmlTimePeriod::create(
		GmlTimeInstant::non_null_ptr_type begin_,
		GmlTimeInstant::non_null_ptr_type end_,
		bool check_begin_end_times)
{
	if (check_begin_end_times)
	{
		GPlatesGlobal::Assert<BeginTimeLaterThanEndTimeException>(
				begin_->get_time_position() <= end_->get_time_position(),
				GPLATES_ASSERTION_SOURCE);
	}

	GPlatesModel::ModelTransaction transaction;
	non_null_ptr_type ptr(new GmlTimePeriod(transaction, begin_, end_));
	transaction.commit();

	return ptr;
}


void
GPlatesPropertyValues::GmlTimePeriod::set_begin(
		GmlTimeInstant::non_null_ptr_type begin_,
		bool check_begin_end_times)
{
	if (check_begin_end_times)
	{
		GPlatesGlobal::Assert<BeginTimeLaterThanEndTimeException>(
				begin_->get_time_position() <= end()->get_time_position(),
				GPLATES_ASSERTION_SOURCE);
	}

	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().begin.change(
			revision_handler.get_model_transaction(), begin_);
	revision_handler.commit();
}


void
GPlatesPropertyValues::GmlTimePeriod::set_end(
		GmlTimeInstant::non_null_ptr_type end_,
		bool check_begin_end_times)
{
	if (check_begin_end_times)
	{
		GPlatesGlobal::Assert<BeginTimeLaterThanEndTimeException>(
				begin()->get_time_position() <= end_->get_time_position(),
				GPLATES_ASSERTION_SOURCE);
	}

	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().end.change(
			revision_handler.get_model_transaction(), end_);
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GmlTimePeriod::print_to(
		std::ostream &os) const
{
	return os << *begin() << " - " << *end();
}


GPlatesModel::Revision::non_null_ptr_type
GPlatesPropertyValues::GmlTimePeriod::bubble_up(
		GPlatesModel::ModelTransaction &transaction,
		const Revisionable::non_null_ptr_to_const_type &child_revisionable)
{
	// Bubble up to our (parent) context (if any) which creates a new revision for us.
	Revision &revision = create_bubble_up_revision<Revision>(transaction);

	// In this method we are operating on a (bubble up) cloned version of the current revision.

	if (child_revisionable == revision.begin.get_revisionable())
	{
		return revision.begin.clone_revision(transaction);
	}

	// The child property value that bubbled up the modification should be one of our children.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			child_revisionable == revision.end.get_revisionable(),
			GPLATES_ASSERTION_SOURCE);

	return revision.end.clone_revision(transaction);
}
