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

#include <boost/cstdint.hpp>
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
#ifndef BOOST_NO_INT64_T

	/**
	 * A 64-bit counter that delegates to boost::uint64_t.
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
		// Use built-in 64-bit integers where available.
		boost::uint64_t d_counter;
	};

#else

	/*
	 * A 64-bit implementation just in case we happen to run into a compiler without 64-bit integers.
	 *
	 * Shouldn't really happen on any systems that Qt supports but better to be sure.
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
				boost::uint32_t low = 0) :
			d_high(0),
			d_low(low)
		{  }

		Counter64 &
		operator++()
		{
			++d_low;
			if (d_low == 0) // integer overflow
			{
				++d_high;
			}
			return *this;
		}

		bool
		operator==(
				const Counter64 &other) const
		{
			return d_low == other.d_low && d_high == other.d_high;
		}

		bool
		operator<(
				const Counter64 &other) const
		{
			return
				(d_high < other.d_high) ||
				((d_high == other.d_high) && (d_low < other.d_low));
		}

	private:
		boost::uint32_t d_high, d_low; // This should be portable.
	};

#endif
}

#endif // GPLATES_UTILS_COUNTER64_H
