/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "PointOnSphere.h"


bool
GPlatesMaths::PointOnSphere::is_close_to(
 const PointOnSphere &test_point,
 const real_t &closeness_inclusion_threshold,
 real_t &closeness) const {

	closeness = dot(unitvector(), test_point.unitvector());
	return
	 (closeness.isPreciselyGreaterThan(
	   closeness_inclusion_threshold.dval()));
}
