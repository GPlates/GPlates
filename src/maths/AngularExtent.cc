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

#include "SmallCircleBounds.h"


GPlatesMaths::AngularExtent
GPlatesMaths::AngularExtent::create_from_cosine(
		const double &cosine_angle)
{
	const double cosine_angle_square = cosine_angle * cosine_angle;
	const double sine_angle = (cosine_angle_square < 1)
			? std::sqrt(1 - cosine_angle_square)
			: 0;

	return AngularExtent(cosine_angle, sine_angle);
}


GPlatesMaths::AngularExtent
GPlatesMaths::AngularExtent::create(
		const BoundingSmallCircle &bounding_small_circle)
{
	return AngularExtent(
			bounding_small_circle.get_small_circle_boundary_cosine(),
			bounding_small_circle.get_small_circle_boundary_sine());
}
