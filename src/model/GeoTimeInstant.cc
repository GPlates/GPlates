/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#include <cmath>
#include "GeoTimeInstant.h"
#include "util/FloatingPointComparisons.h"


const GPlatesModel::GeoTimeInstant
GPlatesModel::GeoTimeInstant::create_distant_past() {
	return GeoTimeInstant(INFINITY);
}


const GPlatesModel::GeoTimeInstant
GPlatesModel::GeoTimeInstant::create_distant_future() {
	return GeoTimeInstant(-INFINITY);
}


bool
GPlatesModel::GeoTimeInstant::is_distant_past() const {
	// Return whether the time-position is positive infinity.
	return (isinf(d_value) && (d_value > 0));
}


bool
GPlatesModel::GeoTimeInstant::is_distant_future() const {
	// Return whether the time-position is negative infinity.
	return (isinf(d_value) && (d_value < 0));
}


bool
GPlatesModel::GeoTimeInstant::is_real() const {
	return std::isfinite(d_value);
}


bool
GPlatesModel::GeoTimeInstant::is_coincident_with(
		const GeoTimeInstant &other) const {
	/*
	 * To double-check:  Do we want infinities to compare equal or unequal?
	 */

	using namespace GPlatesUtil::FloatingPointComparisons;

	return (geo_times_are_approx_equal(d_value, other.d_value));
}
