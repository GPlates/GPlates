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
#include <boost/utility/compare_pointees.hpp>

#include "GpmlTimeSample.h"


bool
GPlatesPropertyValues::GpmlTimeSample::operator==(
		const GpmlTimeSample &other) const
{
	return *d_value.get_const() == *other.d_value.get_const() &&
		*d_valid_time.get_const() == *other.d_valid_time.get_const() &&
		boost::equal_pointees(d_description.get_const(), other.d_description.get_const()) &&
		d_value_type == other.d_value_type &&
		d_is_disabled == other.d_is_disabled;
}


std::ostream &
GPlatesPropertyValues::operator<<(
		std::ostream &os,
		const GpmlTimeSample &time_sample)
{
	return os << *time_sample.get_value();
}
