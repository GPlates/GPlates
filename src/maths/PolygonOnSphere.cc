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
#include "PolygonOnSphere.h"
#include "HighPrecision.h"
#include "InvalidPolygonException.h"
#include "global/InvalidParametersException.h"
#include "global/UninitialisedIteratorException.h"


const unsigned
GPlatesMaths::PolygonOnSphere::MIN_NUM_COLLECTION_POINTS = 3;


GPlatesMaths::PolygonOnSphere::ConstructionParameterValidity
GPlatesMaths::PolygonOnSphere::evaluate_segment_endpoint_validity(
 const PointOnSphere &p1,
 const PointOnSphere &p2) {

	GreatCircleArc::ParameterStatus s =
	 GreatCircleArc::test_parameter_status(p1, p2);

	// Using a switch-statement, along with GCC's "-Wswitch" option
	// (implicitly enabled by "-Wall"), will help to ensure that no cases
	// are missed.
	switch (s) {

	 case GreatCircleArc::VALID:

		// Continue after switch block so that we don't get warnings
		// about "control reach[ing] end of non-void function".
		break;

	 case GreatCircleArc::INVALID_IDENTICAL_ENDPOINTS:

		return INVALID_DUPLICATE_SEGMENT_ENDPOINTS;

	 case GreatCircleArc::INVALID_ANTIPODAL_ENDPOINTS:

		return INVALID_ANTIPODAL_SEGMENT_ENDPOINTS;
	}
	return VALID;
}


bool
GPlatesMaths::PolygonOnSphere::is_close_to(
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


void
GPlatesMaths::PolygonOnSphere::create_segment_and_append_to_seq(
 seq_type &seq, 
 const PointOnSphere &p1,
 const PointOnSphere &p2,
 bool should_silently_drop_dups) {

	GreatCircleArc::ParameterStatus s =
	 GreatCircleArc::test_parameter_status(p1, p2);

	// Using a switch-statement, along with GCC's "-Wswitch" option
	// (implicitly enabled by "-Wall"), will help to ensure that no cases
	// are missed.
	switch (s) {

	 case GreatCircleArc::VALID:

		// Continue after switch.
		break;

	 case GreatCircleArc::INVALID_IDENTICAL_ENDPOINTS:

		if (should_silently_drop_dups) {

			// You heard the man:  We should silently drop
			// duplicates, instead of throwing an exception.
			return;

		} else {

			// The start-point was the same as the end-point
			// => no segment.
			std::ostringstream oss;

			oss
			 << "Attempted to create a polygon line-segment from "
			 << "duplicate endpoints "
			 << p1
			 << " and "
			 << p2
			 << ".";
			throw InvalidPolygonException(oss.str().c_str());
			// FIXME: Should be
			// 'PolygonSegmentConstructionException', a derived
			// class of 'PolygonConstructionException'.
		}

	 case GreatCircleArc::INVALID_ANTIPODAL_ENDPOINTS:

		{
			// The start-point and the end-point are antipodal
			// => indeterminate segment
			std::ostringstream oss;

			oss
			 << "Attempted to create a polygon line-segment from "
			 << "antipodal endpoints "
			 << p1
			 << " and "
			 << p2
			 << ".";
			throw InvalidPolygonException(oss.str().c_str());
			// FIXME: Should be
			// 'PolygonSegmentConstructionException', a derived
			// class of 'PolygonConstructionException'.
		}
	}

	// We should only have arrived at this point if
	// GreatCircleArc::test_parameter_status returned
	// GreatCircleArc::VALID.
	GreatCircleArc segment = GreatCircleArc::create(p1, p2);
	seq.push_back(segment);
}


const GPlatesMaths::PointOnSphere &
GPlatesMaths::PolygonOnSphere::VertexConstIterator::current_point() const {

	if (d_poly_ptr == NULL) {

		// I think the exception message sums it up pretty nicely...
		throw GPlatesGlobal::UninitialisedIteratorException(
		 "Attempted to dereference an uninitialised iterator.");
	}
	return d_curr_gca->start_point();
}
