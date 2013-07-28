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

#include "GpmlTimeSample.h"


const GPlatesPropertyValues::GpmlTimeSample
GPlatesPropertyValues::GpmlTimeSample::clone() const
{
	boost::intrusive_ptr<XsString> cloned_description;
	if (d_description)
	{
		cloned_description.reset(d_description.get()->clone().get());
	}

	return GpmlTimeSample(
			d_value->clone(),
			d_valid_time->clone(),
			cloned_description,
			d_value_type,
			d_is_disabled);
}


bool
GPlatesPropertyValues::GpmlTimeSample::operator==(
		const GpmlTimeSample &other) const
{
	return *d_value == *other.d_value &&
		*d_valid_time == *other.d_valid_time &&
		boost::equal_pointees(d_description, other.d_description) &&
		d_value_type == other.d_value_type &&
		d_is_disabled == other.d_is_disabled;
}

