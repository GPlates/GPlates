/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <cmath>
#include "GeoTimeInstant.h"


inline
const GPlatesModel::GeoTimeInstant
GPlatesModel::GeoTimeInstant::create_distant_past() {
	return GeoTimeInstant(INFINITY);
}


inline
const GPlatesModel::GeoTimeInstant
GPlatesModel::GeoTimeInstant::create_distant_future() {
	return GeoTimeInstant(-INFINITY);
}


inline
bool
GPlatesModel::GeoTimeInstant::is_distant_past() const {
	// Return whether the time-position is positive infinity.
	return (isinf(d_time_position) && (d_time_position > 0));
}


inline
bool
GPlatesModel::GeoTimeInstant::is_distant_future() const {
	// Return whether the time-position is negative infinity.
	return (isinf(d_time_position) && (d_time_position < 0));
}


inline
bool
GPlatesModel::GeoTimeInstant::is_real() const {
	return std::isfinite(d_time_position);
}


namespace {

	/*
	 * See "src/maths/Real.cc" for a hand-wavey justification of this choice of value.
	 *
	 * For what it's worth, this represents a precision of about eight-and-three-quarter hours,
	 * which is not too bad for geological time.
	 */
	static const double Epsilon = 1.0e-9;
}


bool
GPlatesModel::GeoTimeInstant::is_coincident_with(
		const GeoTimeInstant &other) const {
	/*
	 * To double-check:  Do we want infinities to compare equal or unequal?
	 */

	return (fabs(d_time_position - other.d_time_position) < ::Epsilon);
}
