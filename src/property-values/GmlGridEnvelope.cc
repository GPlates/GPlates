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
#include <boost/foreach.hpp>

#include "GmlGridEnvelope.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"


const GPlatesPropertyValues::GmlGridEnvelope::non_null_ptr_type
GPlatesPropertyValues::GmlGridEnvelope::create(
		const integer_list_type &low_,
		const integer_list_type &high_)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			low_.size() == high_.size(), GPLATES_ASSERTION_SOURCE);

	return new GmlGridEnvelope(low_, high_);
}


void
GPlatesPropertyValues::GmlGridEnvelope::set_low_and_high(
		const integer_list_type &low_,
		const integer_list_type &high_)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			low_.size() == high_.size(), GPLATES_ASSERTION_SOURCE);

	d_low = low_;
	d_high = high_;
}


std::ostream &
GPlatesPropertyValues::GmlGridEnvelope::print_to(
		std::ostream &os) const
{
	os << "{ ";

	BOOST_FOREACH(int d, d_low)
	{
		os << d << " ";
	}

	os << "} { ";

	BOOST_FOREACH(int d, d_high)
	{
		os << d << " ";
	}

	os << "}";

	return os;
}

