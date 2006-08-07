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

#include <sstream>
#include "PolyLineOnSphere.h"
#include "HighPrecision.h"
#include "global/InvalidParametersException.h"


bool
GPlatesMaths::PolyLineOnSphere::is_close_to(
 const PointOnSphere &test_point,
 const real_t &closeness_inclusion_threshold,
 const real_t &latitude_exclusion_threshold,
 real_t &closeness) const {

	// First, ensure the parameters are valid.
	if (((closeness_inclusion_threshold * closeness_inclusion_threshold) +
	     (latitude_exclusion_threshold * latitude_exclusion_threshold))
	    != 1.0) {

		/*
		 * Well, *duh*, they *should* equal 1.0: those two thresholds
		 * are supposed to form the non-hypotenuse legs (the "catheti")
		 * of a right-angled triangle inscribed in a unit circle.
		 */
		real_t calculated_hypotenuse =
		 closeness_inclusion_threshold * closeness_inclusion_threshold +
		 latitude_exclusion_threshold * latitude_exclusion_threshold;

		std::ostringstream oss;
		oss
		 << "The squares of the closeness inclusion threshold ("
		 << HighPrecision< real_t >(closeness_inclusion_threshold)
		 << ")\nand the latitude exclusion threshold ("
		 << HighPrecision< real_t >(latitude_exclusion_threshold)
		 << ") sum to ("
		 << HighPrecision< real_t >(calculated_hypotenuse)
		 << ")\nrather than the expected value of 1.";

		throw
		 GPlatesGlobal::InvalidParametersException(oss.str().c_str());
	}

	real_t &closest_closeness_so_far = closeness;  // A descriptive alias.
	bool have_already_found_a_close_gca = false;

	const_iterator iter = begin(), the_end = end();
	for ( ; iter != the_end; ++iter) {

		const GreatCircleArc &the_gca = *iter;

		// Don't bother initialising this.
		real_t gca_closeness;

		if (the_gca.is_close_to(test_point,
		     closeness_inclusion_threshold,
		     latitude_exclusion_threshold, gca_closeness)) {

			if (have_already_found_a_close_gca) {

				if (gca_closeness.isPreciselyGreaterThan(
				     closest_closeness_so_far.dval())) {

					closest_closeness_so_far =
					 gca_closeness;
				}
				// else, do nothing.

			} else {

				closest_closeness_so_far = gca_closeness;
				have_already_found_a_close_gca = true;
			}
		}
	}
	return (have_already_found_a_close_gca);
}
