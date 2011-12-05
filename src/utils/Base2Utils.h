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

#ifndef GPLATES_UTILS_BASE2UTILS_H
#define GPLATES_UTILS_BASE2UTILS_H

#include <boost/cstdint.hpp>


namespace GPlatesUtils
{
	/**
	 * Various utilities to do with base-2 arithmetic.
	 *
	 * Most of this comes from Sean Eron Anderson at http://graphics.stanford.edu/~seander/bithacks.html
	 */
	namespace Base2
	{
		/**
		 * Determines if the specified integer is a power-of-two.
		 *
		 * NOTE: Does not work for a @a value of zero.
		 */
		inline
		bool
		is_power_of_two(
				unsigned int value)
		{
			return (value & (value - 1)) == 0;
		}


		/**
		 * Determines the previous lower power-of-two of the specified integer.
		 *
		 * Returns @a value if it is already a power-of-two.
		 *
		 * NOTE: Does not work for a @a value of zero.
		 */
		unsigned int
		previous_power_of_two(
				boost::uint32_t value);


		/**
		 * Determines the next higher power-of-two of the specified integer.
		 *
		 * Returns @a value if it is already a power-of-two.
		 *
		 * NOTE: Does not work for a @a value of zero.
		 */
		unsigned int
		next_power_of_two(
				boost::uint32_t value);


		/**
		 * Determines the previous lower power-of-two of the specified integer and returns
		 * the log base 2 of that result.
		 *
		 * Returns log2 of @a value if @a value is already a power-of-two.
		 *
		 * NOTE: Does not work for a @a value of zero.
		 */
		unsigned int
		log2_previous_power_of_two(
				boost::uint32_t value);


		/**
		 * Determines the next higher power-of-two of the specified integer and returns
		 * the log base 2 of that result.
		 *
		 * Returns log2 of @a value if @a value is already a power-of-two.
		 *
		 * NOTE: Does not work for a @a value of zero.
		 */
		unsigned int
		log2_next_power_of_two(
				boost::uint32_t value);


		/**
		 * Determines the log base 2 of @a value (where @a value *must* be a power-of-two).
		 *
		 * NOTE: @a value must already be a power-of-two.
		 *
		 * NOTE: Does not work for a @a value of zero.
		 */
		unsigned int
		log2_power_of_two(
				boost::uint32_t value);
	}
}

#endif // GPLATES_UTILS_BASE2UTILS_H
