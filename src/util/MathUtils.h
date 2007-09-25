/* $Id: StringUtils.h 1485 2007-09-19 04:47:11Z glen $ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2007-09-19 14:47:11 +1000 (Wed, 19 Sep 2007) $
 *
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_UTIL_MATHUTILS_H
#define GPLATES_UTIL_MATHUTILS_H

#include <limits>
#include <cmath>

namespace GPlatesUtil
{
	template<class T>
	inline
	bool
	is_value_in_range(
			const T &value,
			const T &minimum,
			const T &maximum)
	{
		return value >= minimum - std::numeric_limits<T>::epsilon() 
			&& value <= maximum + std::numeric_limits<T>::epsilon();
	}

	template<class T>
	inline
	bool
	are_values_approx_equal(
			const T &value1,
			const T &value2)
	{
		return std::fabs(value1 - value2) < std::numeric_limits<T>::epsilon();
	}
}

#endif
