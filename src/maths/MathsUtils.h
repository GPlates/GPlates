/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2007, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_MATHUTILS_H
#define GPLATES_MATHS_MATHUTILS_H

#include <limits>
#include <cmath>


namespace GPlatesMaths
{
	template<class T>
	inline
	bool
	is_in_range(
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
	are_almost_exactly_equal(
			const T &value1,
			const T &value2)
	{
		return std::fabs(value1 - value2) < std::numeric_limits<T>::epsilon();
	}


	/**
	 * Pi, the ratio of the circumference to the diameter of a circle.
	 */
	static const double Pi = 3.14159265358979323846264338;


	template<typename T>
	inline
	const T
	convert_deg_to_rad(
			const T &value_in_degrees)
	{
		return T((Pi / 180.0) * value_in_degrees);
	}


	template<typename T>
	inline
	const T
	convert_rad_to_deg(
			const T &value_in_radians)
	{
		return T((180.0 / Pi) * value_in_radians);
	}
}

#endif  // GPLATES_MATHS_MATHUTILS_H
