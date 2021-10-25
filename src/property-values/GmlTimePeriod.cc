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
#include <typeinfo>

#include "GmlTimePeriod.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


const GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type
GPlatesPropertyValues::GmlTimePeriod::create(
		GmlTimeInstant::non_null_ptr_type begin_,
		GmlTimeInstant::non_null_ptr_type end_,
		bool check_begin_end_times)
{
	if (check_begin_end_times)
	{
		GPlatesGlobal::Assert<BeginTimeLaterThanEndTimeException>(
				begin_->time_position() <= end_->time_position(),
				GPLATES_ASSERTION_SOURCE);
	}

	return non_null_ptr_type(new GmlTimePeriod(begin_, end_));
}


void
GPlatesPropertyValues::GmlTimePeriod::set_begin(
		GmlTimeInstant::non_null_ptr_type begin_,
		bool check_begin_end_times)
{
	if (check_begin_end_times)
	{
		GPlatesGlobal::Assert<BeginTimeLaterThanEndTimeException>(
				begin_->time_position() <= end()->time_position(),
				GPLATES_ASSERTION_SOURCE);
	}

	d_begin = begin_;
	update_instance_id();
}


void
GPlatesPropertyValues::GmlTimePeriod::set_end(
		GmlTimeInstant::non_null_ptr_type end_,
		bool check_begin_end_times)
{
	if (check_begin_end_times)
	{
		GPlatesGlobal::Assert<BeginTimeLaterThanEndTimeException>(
				begin()->time_position() <= end_->time_position(),
				GPLATES_ASSERTION_SOURCE);
	}

	d_end = end_;
	update_instance_id();
}


const GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type
GPlatesPropertyValues::GmlTimePeriod::deep_clone() const
{
	GmlTimePeriod::non_null_ptr_type dup = clone();

	GmlTimeInstant::non_null_ptr_type cloned_begin = d_begin->deep_clone();
	dup->d_begin = cloned_begin;
	GmlTimeInstant::non_null_ptr_type cloned_end = d_end->deep_clone();
	dup->d_end = cloned_end;

	return dup;
}


std::ostream &
GPlatesPropertyValues::GmlTimePeriod::print_to(
		std::ostream &os) const
{
	return os << *d_begin << " - " << *d_end;
}


bool
GPlatesPropertyValues::GmlTimePeriod::directly_modifiable_fields_equal(
		const GPlatesModel::PropertyValue &other) const
{
	try
	{
		const GmlTimePeriod &other_casted =
			dynamic_cast<const GmlTimePeriod &>(other);
		return *d_begin == *other_casted.d_begin &&
			*d_end == *other_casted.d_end;
	}
	catch (const std::bad_cast &)
	{
		// Should never get here, but doesn't hurt to check.
		return false;
	}
}

