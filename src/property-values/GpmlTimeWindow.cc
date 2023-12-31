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


const GPlatesPropertyValues::GpmlTimeWindow
GPlatesPropertyValues::GpmlTimeWindow::deep_clone() const
{
	GpmlTimeWindow dup(*this);

	GPlatesModel::PropertyValue::non_null_ptr_type cloned_time_dependent_value =
			d_time_dependent_value->deep_clone_as_prop_val();
	dup.d_time_dependent_value = cloned_time_dependent_value;

	GmlTimePeriod::non_null_ptr_type cloned_valid_time = d_valid_time->deep_clone();
	dup.d_valid_time = cloned_valid_time;

	return dup;
}


std::ostream &
GPlatesPropertyValues::operator<<(
		std::ostream &os,
		const GpmlTimeWindow &time_window)
{
	return os << *time_window.time_dependent_value();
}


bool
GPlatesPropertyValues::GpmlTimeWindow::operator==(
		const GpmlTimeWindow &other) const
{
	return *d_time_dependent_value == *other.d_time_dependent_value &&
		*d_valid_time == *other.d_valid_time &&
		d_value_type == other.d_value_type;
}
