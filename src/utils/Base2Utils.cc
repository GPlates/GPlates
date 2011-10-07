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

#include "Base2Utils.h"


unsigned int
GPlatesUtils::Base2::previous_power_of_two(
		boost::uint32_t value)
{
	const unsigned int next_power_of_two_value = next_power_of_two(value);

	// If 'value' is a power-of-two then return it otherwise return the next higher
	// power-of-two divided by two.
	return (next_power_of_two_value == value)
		? next_power_of_two_value
		: (next_power_of_two_value >> 1);
}


unsigned int
GPlatesUtils::Base2::next_power_of_two(
		boost::uint32_t value)
{
	// Using boost::uint32_t since algorithm works for 32-bit integers.
	--value;
	value |= value >> 1;
	value |= value >> 2;
	value |= value >> 4;
	value |= value >> 8;
	value |= value >> 16;
	++value;

	return value;
}
