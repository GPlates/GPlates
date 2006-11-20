/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
 *  (under the name "PolyLineOnSphere.cc")
 * Copyright (C) 2006 The University of Sydney, Australia
 *  (under the name "PolylineOnSphere.cc")
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

#include <list>
#include <sstream>
#include "PolylineOnSphere.h"
#include "HighPrecision.h"
#include "InvalidPolylineException.h"
#include "global/InvalidParametersException.h"
#include "global/UninitialisedIteratorException.h"


GPlatesMaths::PolylineOnSphere::ConstructionParameterValidity
GPlatesMaths::PolylineOnSphere::evaluate_segment_endpoint_validity(
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
GPlatesMaths::PolylineOnSphere::is_close_to(
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
GPlatesMaths::PolylineOnSphere::create_segment_and_append_to_seq(
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
			 << "Attempted to create a polyline line-segment from "
			 << "duplicate endpoints "
			 << p1
			 << " and "
			 << p2
			 << ".";
			throw InvalidPolylineException(oss.str().c_str());
			// FIXME: Should be
			// 'PolylineSegmentConstructionException', a derived
			// class of 'PolylineConstructionException'.
		}

	 case GreatCircleArc::INVALID_ANTIPODAL_ENDPOINTS:

		{
			// The start-point and the end-point are antipodal
			// => indeterminate segment
			std::ostringstream oss;

			oss
			 << "Attempted to create a polyline line-segment from "
			 << "antipodal endpoints "
			 << p1
			 << " and "
			 << p2
			 << ".";
			throw InvalidPolylineException(oss.str().c_str());
			// FIXME: Should be
			// 'PolylineSegmentConstructionException', a derived
			// class of 'PolylineConstructionException'.
		}
	}

	// We should only have arrived at this point if
	// GreatCircleArc::test_parameter_status returned
	// GreatCircleArc::VALID.
	GreatCircleArc segment = GreatCircleArc::create(p1, p2);
	seq.push_back(segment);
}


const GPlatesMaths::PointOnSphere &
GPlatesMaths::PolylineOnSphere::VertexConstIterator::current_point() const {

	if (d_poly_ptr == NULL) {

		// I think the exception message sums it up pretty nicely...
		throw GPlatesGlobal::UninitialisedIteratorException(
		 "Attempted to dereference an uninitialised iterator.");
	}

	if (d_curr_gca == d_poly_ptr->begin() && d_gca_start_or_end == START) {

		return d_curr_gca->start_point();

	} else {

		return d_curr_gca->end_point();
	}
}


void
GPlatesMaths::PolylineOnSphere::VertexConstIterator::increment() {

	if (d_poly_ptr == NULL) {

		// This iterator is uninitialised, so this function will be a
		// no-op.
		return;
	}

	if (d_curr_gca == d_poly_ptr->begin() && d_gca_start_or_end == START) {

		d_gca_start_or_end = END;

	} else {

		++d_curr_gca;
	}
}


void
GPlatesMaths::PolylineOnSphere::VertexConstIterator::decrement() {

	if (d_poly_ptr == NULL) {

		// This iterator is uninitialised, so this function will be a
		// no-op.
		return;
	}

	if (d_curr_gca == d_poly_ptr->begin() && d_gca_start_or_end == END) {

		d_gca_start_or_end = START;

	} else {

		--d_curr_gca;
	}
}


bool
GPlatesMaths::polylines_are_directed_equivalent(
 const PolylineOnSphere &poly1,
 const PolylineOnSphere &poly2) {

	if (poly1.number_of_vertices() != poly2.number_of_vertices()) {

		// There is no way the two polylines can be equivalent.
		return false;
	}
	// Else, we know the two polylines contain the same number of vertices,
	// so we only need to check the end-of-sequence conditions for 'poly1'
	// in our iteration.

	PolylineOnSphere::vertex_const_iterator
	 poly1_iter = poly1.vertex_begin(),
	 poly1_end = poly1.vertex_end(),
	 poly2_iter = poly2.vertex_begin(),
	 poly2_end = poly2.vertex_end();
	for ( ; poly1_iter != poly1_end; ++poly1_iter, ++poly2_iter) {

		if ( ! points_are_coincident(*poly1_iter, *poly2_iter)) {

			return false;
		}
	}
	return true;
}


bool
GPlatesMaths::polylines_are_undirected_equivalent(
 const PolylineOnSphere &poly1,
 const PolylineOnSphere &poly2) {

	if (poly1.number_of_vertices() != poly2.number_of_vertices()) {

		// There is no way the two polylines can be equivalent.
		return false;
	}
	// Else, we know the two polylines contain the same number of vertices,
	// so we only need to check the end-of-sequence conditions for 'poly1'
	// in our iteration.

	PolylineOnSphere::vertex_const_iterator
	 poly1_iter = poly1.vertex_begin(),
	 poly1_end = poly1.vertex_end(),
	 poly2_iter = poly2.vertex_begin();
	for ( ; poly1_iter != poly1_end; ++poly1_iter, ++poly2_iter) {

		if ( ! points_are_coincident(*poly1_iter, *poly2_iter)) {

			break;
		}
	}
	// So, we're out of the loop.  Why?  Did we make it through all the
	// polyline's vertices (in which case, the polylines are equivalent),
	// or did we break before the end (in which case, we need to try the
	// reverse)?
	if (poly1_iter == poly1_end) {

		// We made it all the way through the vertices, so the
		// polylines are equivalent.
		return true;
	}

	// Let's try comparing 'poly1' with the reverse of 'poly2'.
	std::list< PointOnSphere >
	 rev(poly2.vertex_begin(), poly2.vertex_end());
	rev.reverse();

	std::list< PointOnSphere >::const_iterator rev_iter = rev.begin();
	for (poly1_iter = poly1.vertex_begin();
	     poly1_iter != poly1_end;
	     ++poly1_iter, ++rev_iter) {

		if ( ! points_are_coincident(*poly1_iter, *rev_iter)) {

			return false;
		}
	}
	return true;
}
