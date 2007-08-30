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
	return GeoTimeInstant(TimePositionTypes::DistantPast);
}


const GPlatesModel::GeoTimeInstant
GPlatesModel::GeoTimeInstant::create_distant_future() {
	return GeoTimeInstant(TimePositionTypes::DistantFuture);
}


bool
GPlatesModel::GeoTimeInstant::is_distant_past() const {
	// Return whether the time-position is in the distant past.
	return (d_type == TimePositionTypes::DistantPast);
}


bool
GPlatesModel::GeoTimeInstant::is_distant_future() const {
	// Return whether the time-position is in the distant future.
	return (d_type == TimePositionTypes::DistantFuture);
}


bool
GPlatesModel::GeoTimeInstant::is_real() const {
	return (d_type == TimePositionTypes::Real);
}


bool
GPlatesModel::GeoTimeInstant::is_coincident_with(
		const GeoTimeInstant &other) const {
	if (d_type != other.d_type) {
		// At least one of the time-positions is "distant" (and if both are "distant",
		// they're not the same sort of "distant"), so there's no chance that these two
		// time-instants can be equal.
		return false;
	}

	// Note:  For now, if we encounter two time-instants which are both in the "distant past",
	// we're going to consider them equal.  (Even though we don't actually know what the times
	// in the "distant past" were, I think that it will be appropriate for the program to treat
	// the two time-instants in the same way, so we'll call them equal.)  Similarly for two
	// time-instants which are both in the "distant future".

	using namespace GPlatesUtil::FloatingPointComparisons;

	return (geo_times_are_approx_equal(d_value, other.d_value));
}
