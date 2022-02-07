/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include "AngularExtent.h"


const GPlatesMaths::AngularExtent GPlatesMaths::AngularExtent::ZERO =
		GPlatesMaths::AngularExtent::create_from_cosine_and_sine(real_t(1.0), real_t(0.0));

const GPlatesMaths::AngularExtent GPlatesMaths::AngularExtent::HALF_PI =
		GPlatesMaths::AngularExtent::create_from_cosine_and_sine(real_t(0.0), real_t(1.0));

const GPlatesMaths::AngularExtent GPlatesMaths::AngularExtent::PI =
		GPlatesMaths::AngularExtent::create_from_cosine_and_sine(real_t(-1.0), real_t(0.0));


GPlatesMaths::AngularExtent &
GPlatesMaths::AngularExtent::operator+=(
		const AngularExtent &rhs)
{
	// If 'angular_extent_1 + angular_extent_2' exceeds PI then comparing cosine(angle) doesn't work
	// because cosine starts to repeat itself. The easiest way to detect this without calculating
	// angles using 'acos' is to see if either angle exceeds PI/2 (hemisphere small circle) and then
	// revert to using 'acos' in that case (it should be relatively rare to have angular extents that big).
	if (is_strictly_negative(get_cosine()) ||
		is_strictly_negative(rhs.get_cosine()))
	{
		// Use expensive 'acos' function.
		const real_t angular_extent = acos(get_cosine()) + acos(rhs.get_cosine());
		if (angular_extent.is_precisely_greater_than(GPlatesMaths::PI))
		{
			// Clamp to PI.
			d_cosine = AngularExtent::PI.get_cosine();
			d_sine = AngularExtent::PI.get_sine();
			d_angle = AngularExtent::PI.get_angle();

			return *this;
		}

		d_cosine = cos(angular_extent);
		d_sine = boost::none; // Will get calculated if/when needed.
		d_angle = boost::none; // Will get calculated if/when needed.

		return *this;
	}

	// cos(a+b) = cos(a)cos(b) - sin(a)sin(b)
	const real_t cosine = get_cosine() * rhs.get_cosine() - get_sine() * rhs.get_sine();
	// sin(a+b) = sin(a)cos(b) + cos(a)sin(b)
	const real_t sine = get_sine() * rhs.get_cosine() + get_cosine() * rhs.get_sine();

	d_cosine = cosine;
	d_sine = sine;
	d_angle = boost::none; // Will get calculated if/when needed.

	return *this;
}


GPlatesMaths::AngularExtent &
GPlatesMaths::AngularExtent::operator-=(
		const AngularExtent &rhs)
{
	// If 'angular_extent_2' exceeds 'angular_extent_1' then clamp to zero.
	// This is same test as 'cos(angular_extent_2) < cos(angular_extent_1)'.
	if (rhs.get_cosine().is_precisely_less_than(get_cosine().dval()))
	{
		// Clamp to zero.
		d_cosine = AngularExtent::ZERO.get_cosine();
		d_sine = AngularExtent::ZERO.get_sine();
		d_angle = AngularExtent::ZERO.get_angle();

		return *this;
	}

	// cos(a-b) = cos(a)cos(b) + sin(a)sin(b)
	const real_t cosine = get_cosine() * rhs.get_cosine() + get_sine() * rhs.get_sine();
	// sin(a-b) = sin(a)cos(b) - cos(a)sin(b)
	const real_t sine = get_sine() * rhs.get_cosine() - get_cosine() * rhs.get_sine();

	d_cosine = cosine;
	d_sine = sine;
	d_angle = boost::none; // Will get calculated if/when needed.

	return *this;
}
