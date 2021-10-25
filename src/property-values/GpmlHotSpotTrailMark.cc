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

#include "GpmlHotSpotTrailMark.h"


namespace
{
	template<class T>
	bool
	opt_eq(
			const boost::optional<T> &opt1,
			const boost::optional<T> &opt2)
	{
		if (opt1)
		{
			if (!opt2)
			{
				return false;
			}
			return **opt1 == **opt2;
		}
		else
		{
			return !opt2;
		}
	}
}


const GPlatesPropertyValues::GpmlHotSpotTrailMark::non_null_ptr_type
GPlatesPropertyValues::GpmlHotSpotTrailMark::deep_clone() const
{
	GpmlHotSpotTrailMark::non_null_ptr_type dup = clone();

	GmlPoint::non_null_ptr_type cloned_position = d_position->deep_clone();
	dup->d_position = cloned_position;

	if (d_trail_width) {
		GpmlMeasure::non_null_ptr_type cloned_trail_width =
				(*d_trail_width)->deep_clone();
		dup->d_trail_width = cloned_trail_width;
	}

	if (d_measured_age) {
		GmlTimeInstant::non_null_ptr_type cloned_measured_age =
				(*d_measured_age)->deep_clone();
		dup->d_measured_age = cloned_measured_age;
	}

	if (d_measured_age_range) {
		GmlTimePeriod::non_null_ptr_type cloned_measured_age_range =
				(*d_measured_age_range)->deep_clone();
		dup->d_measured_age_range = cloned_measured_age_range;
	}

	return dup;
}


std::ostream &
GPlatesPropertyValues::GpmlHotSpotTrailMark::print_to(
		std::ostream &os) const
{
	os << "[ " << *d_position << " , ";
	if (d_trail_width)
	{
		os << **d_trail_width;
	}
	os << " , ";
	if (d_measured_age)
	{
		os << **d_measured_age;
	}
	os << " , ";
	if (d_measured_age_range)
	{
		os << **d_measured_age_range;
	}
	return os << " ]";
}


bool
GPlatesPropertyValues::GpmlHotSpotTrailMark::directly_modifiable_fields_equal(
		const GPlatesModel::PropertyValue &other) const
{
	try
	{
		const GpmlHotSpotTrailMark &other_casted =
			dynamic_cast<const GpmlHotSpotTrailMark &>(other);
		return *d_position == *other_casted.d_position &&
			opt_eq(d_trail_width, other_casted.d_trail_width) &&
			opt_eq(d_measured_age, other_casted.d_measured_age) &&
			opt_eq(d_measured_age_range, other_casted.d_measured_age_range);
	}
	catch (const std::bad_cast &)
	{
		return false;
	}
}
