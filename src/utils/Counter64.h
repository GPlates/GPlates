/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_UTILS_COUNTER64_H
#define GPLATES_UTILS_COUNTER64_H

#include <cstdint>
#include <boost/operators.hpp>


namespace GPlatesUtils
{
	/*
	 * A 64-bit counter than can be incremented and equality or less-than compared.
	 *
	 * NOTE: The counter is not intended to be decrementable.
	 *
	 * NOTE: This incrementing can have problems due to integer overflow and subsequent wraparound
	 * back to zero but we're using 64-bit integers which, if we incremented every CPU cycle
	 * (ie, the fastest possible incrementing) on a 3GHz system it would take 195 years to overflow.
	 * So we are safe as long as we are guaranteed to use 64-bit integers (and for this there is
	 * 64-bit simulation code for those systems that only support 32-bit integers -
	 * which should be very few). Use of 32-bit integers brings this down from 195 years to
	 * a couple of seconds so 64-bit must be used.
	 */
	class Counter64 :
			public boost::incrementable<Counter64>,
			public boost::equality_comparable<Counter64>,
			public boost::less_than_comparable<Counter64>
	{
	public:
		//! Constructor to instantiate from a 32-bit integer (defaults to zero).
		explicit
		Counter64(
				boost::uint32_t counter = 0) :
			d_counter(counter)
		{  }

		Counter64 &
		operator++()
		{
			++d_counter;
			return *this;
		}

		bool
		operator==(
				const Counter64 &other) const
		{
			return d_counter == other.d_counter;
		}

		bool
		operator<(
				const Counter64 &other) const
		{
			return d_counter < other.d_counter;
		}

	private:
		std::uint64_t d_counter;
	};
}

#endif // GPLATES_UTILS_COUNTER64_H
