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

#include "GpmlTimeSample.h"


namespace
{
	bool
	intrusive_ptr_eq(
			const boost::intrusive_ptr<GPlatesPropertyValues::XsString> &p1,
			const boost::intrusive_ptr<GPlatesPropertyValues::XsString> &p2)
	{
		if (p1)
		{
			if (!p2)
			{
				return false;
			}
			return *p1 == *p2;
		}
		else
		{
			return !p2;
		}
	}
}


const GPlatesPropertyValues::GpmlTimeSample
GPlatesPropertyValues::GpmlTimeSample::deep_clone() const
{
	GpmlTimeSample dup(*this);

	GPlatesModel::PropertyValue::non_null_ptr_type cloned_value =
			d_value->deep_clone_as_prop_val();
	dup.d_value = cloned_value;

	GmlTimeInstant::non_null_ptr_type cloned_valid_time = d_valid_time->deep_clone();
	dup.d_valid_time = cloned_valid_time;

	if (d_description) {
		XsString::non_null_ptr_type cloned_description = d_description->deep_clone();
		dup.d_description = boost::intrusive_ptr<XsString>(cloned_description.get());
	}

	return dup;
}


bool
GPlatesPropertyValues::GpmlTimeSample::operator==(
		const GpmlTimeSample &other) const
{
	return *d_value == *other.d_value &&
		*d_valid_time == *other.d_valid_time &&
		intrusive_ptr_eq(d_description, other.d_description) &&
		d_value_type == other.d_value_type &&
		d_is_disabled == other.d_is_disabled;
}
