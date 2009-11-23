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
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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
#ifdef HAVE_PYTHON
# include <boost/python.hpp>
#endif
#include "PolylineOnSphere.h"
#include "PolylineProximityHitDetail.h"
#include "ProximityCriteria.h"
#include "ConstGeometryOnSphereVisitor.h"
#include "HighPrecision.h"
#include "global/InvalidParametersException.h"
#include "global/UninitialisedIteratorException.h"
#include "global/IntrusivePointerZeroRefCountException.h"


const unsigned
GPlatesMaths::PolylineOnSphere::s_min_num_collection_points = 2;


GPlatesMaths::PolylineOnSphere::ConstructionParameterValidity
GPlatesMaths::PolylineOnSphere::evaluate_segment_endpoint_validity(
		const PointOnSphere &p1,
		const PointOnSphere &p2)
{
	GreatCircleArc::ConstructionParameterValidity cpv =
			GreatCircleArc::evaluate_construction_parameter_validity(p1, p2);

	// Using a switch-statement, along with GCC's "-Wswitch" option (implicitly enabled by
	// "-Wall"), will help to ensure that no cases are missed.
	switch (cpv) {

	case GreatCircleArc::VALID:
		// Continue after switch block so that we don't get warnings
		// about "control reach[ing] end of non-void function".
		break;

	case GreatCircleArc::INVALID_ANTIPODAL_ENDPOINTS:
		return INVALID_ANTIPODAL_SEGMENT_ENDPOINTS;
	}
	return VALID;
}


const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
GPlatesMaths::PolylineOnSphere::get_non_null_pointer() const
{
	if (get_reference_count() == 0) {
		// How did this happen?  This should not have happened.
		//
		// Presumably, the programmer obtained the raw PolylineOnSphere pointer from inside
		// a PolylineOnSphere::non_null_ptr_type, and is invoking this member function upon
		// the instance indicated by the raw pointer, after all ref-counting pointers have
		// expired and the instance has actually been deleted.
		//
		// Regardless of how this happened, this is an error.
		throw GPlatesGlobal::IntrusivePointerZeroRefCountException(GPLATES_EXCEPTION_SOURCE,
				this);
	} else {
		// This instance is already managed by intrusive-pointers, so we can simply return
		// another intrusive-pointer to this instance.
		return non_null_ptr_to_const_type(
				this,
				GPlatesUtils::NullIntrusivePointerHandler());
	}
}


GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
GPlatesMaths::PolylineOnSphere::test_proximity(
		const ProximityCriteria &criteria) const
{
	// FIXME: This function should get its own implementation, rather than delegating to
	// 'is_close_to', to enable it to provide more hit detail (for example, whether a vertex or
	// a segment was hit).

	real_t closeness;  // Don't bother initialising this.
	if (this->is_close_to(criteria.test_point(), criteria.closeness_inclusion_threshold(),
			criteria.latitude_exclusion_threshold(), closeness)) {
		// OK, this polyline is close to the test point.
		return make_maybe_null_ptr(PolylineProximityHitDetail::create(
				this->get_non_null_pointer(),
				closeness.dval()));
	} else {
		return ProximityHitDetail::null;
	}
}


void
GPlatesMaths::PolylineOnSphere::accept_visitor(
		ConstGeometryOnSphereVisitor &visitor) const
{
	visitor.visit_polyline_on_sphere(this->get_non_null_pointer());
}


bool
GPlatesMaths::PolylineOnSphere::is_close_to(
		const PointOnSphere &test_point,
		const real_t &closeness_inclusion_threshold,
		const real_t &latitude_exclusion_threshold,
		real_t &closeness) const
{
	// First, ensure the parameters are valid.
	if (((closeness_inclusion_threshold * closeness_inclusion_threshold) +
			(latitude_exclusion_threshold * latitude_exclusion_threshold))
			!= 1.0)
	{
		/*
		 * Well, *duh*, they *should* equal 1.0: those two thresholds
		 * are supposed to form the non-hypotenuse legs (the "catheti")
		 * of a right-angled triangle inscribed in a unit circle.
		 */
		real_t calculated_hypotenuse =
				closeness_inclusion_threshold * closeness_inclusion_threshold +
				latitude_exclusion_threshold * latitude_exclusion_threshold;

		std::ostringstream oss;
		oss << "The squares of the closeness inclusion threshold ("
				<< HighPrecision< real_t >(closeness_inclusion_threshold)
				<< ")\nand the latitude exclusion threshold ("
				<< HighPrecision< real_t >(latitude_exclusion_threshold)
				<< ") sum to ("
				<< HighPrecision< real_t >(calculated_hypotenuse)
				<< ")\nrather than the expected value of 1.";

		throw GPlatesGlobal::InvalidParametersException(GPLATES_EXCEPTION_SOURCE,
				oss.str().c_str());
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
				latitude_exclusion_threshold, gca_closeness))
		{
			if (have_already_found_a_close_gca) {
				if (gca_closeness.is_precisely_greater_than(
							closest_closeness_so_far.dval()))
				{
					closest_closeness_so_far = gca_closeness;
				}
				// else, do nothing.
			} else {
				closest_closeness_so_far = gca_closeness;
				have_already_found_a_close_gca = true;
			}
		}
	}
	return have_already_found_a_close_gca;
}


void
GPlatesMaths::PolylineOnSphere::create_segment_and_append_to_seq(
		seq_type &seq, 
		const PointOnSphere &p1,
		const PointOnSphere &p2)
{
	// We'll assume that the validity of 'p1' and 'p2' to create a GreatCircleArc has been
	// evaluated in the function 'PolylineOnSphere::evaluate_construction_parameter_validity',
	// which was presumably invoked in 'PolylineOnSphere::generate_segments_and_swap' before
	// this function was.

	GreatCircleArc segment = GreatCircleArc::create(p1, p2);
	seq.push_back(segment);
}


const GPlatesMaths::PointOnSphere &
GPlatesMaths::PolylineOnSphere::VertexConstIterator::current_point() const
{
	if (d_poly_ptr == NULL) {
		// I think the exception message sums it up pretty nicely...
		throw GPlatesGlobal::UninitialisedIteratorException(GPLATES_EXCEPTION_SOURCE,
				"Attempted to dereference an uninitialised iterator.");
	}
	if (d_curr_gca == d_poly_ptr->begin() && d_gca_start_or_end == START) {
		return d_curr_gca->start_point();
	} else {
		return d_curr_gca->end_point();
	}
}


void
GPlatesMaths::PolylineOnSphere::VertexConstIterator::increment()
{
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
GPlatesMaths::PolylineOnSphere::VertexConstIterator::decrement()
{
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
		const PolylineOnSphere &poly2)
{
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
		const PolylineOnSphere &poly2)
{
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
	std::list<PointOnSphere> rev(poly2.vertex_begin(), poly2.vertex_end());
	rev.reverse();
	std::list<PointOnSphere>::const_iterator rev_iter = rev.begin();
	for (poly1_iter = poly1.vertex_begin();
			poly1_iter != poly1_end;
			++poly1_iter, ++rev_iter)
	{
		if ( ! points_are_coincident(*poly1_iter, *rev_iter)) {
			return false;
		}
	}
	return true;
}


void
GPlatesMaths::InvalidPointsForPolylineConstructionError::write_message(
				std::ostream &os) const
{
	const char *message;

	switch (d_cpv) {
	case PolylineOnSphere::VALID:
		message = "valid";
		break;
	case PolylineOnSphere::INVALID_INSUFFICIENT_DISTINCT_POINTS:
		message = "insufficient distinct points";
		break;
	case PolylineOnSphere::INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:
		message = "antipodal segment endpoints";
		break;
	default:
		// Control-flow should never reach here.
		message = NULL;
		break;
	}

	if (message)
	{
		os << message;
	}
}


#ifdef HAVE_PYTHON
/**
 * Here begin the Python wrappers
 */


using namespace boost::python;


// *** what about VertextConstIterator?


void
GPlatesMaths::export_PolylineOnSphere()
{
	class_<GPlatesMaths::PolylineOnSphere>("PolylineOnSphere", init<const PolylineOnSphere &>())
		// .def("create_on_heap", &GPlatesMaths::PolylineOnSphere::create_on_heap).staticmethod("create_on_heap")
		// .def("clone_on_heap", &GPlatesMaths::PolylineOnSphere::clone_on_heap).staticmethod("clone_on_heap")
		// *** VARIOUS TEMPLATE CONSTRUCTORS ***
		//.def("assign", &GPlatesMaths::PolylineOnSphere::operator=)
		.def("number_of_vertices", &GPlatesMaths::PolylineOnSphere::number_of_vertices)
		.def("start_point", &GPlatesMaths::PolylineOnSphere::start_point,
				return_internal_reference<1>())
		.def("end_point", &GPlatesMaths::PolylineOnSphere::end_point,
				return_internal_reference<1>())
		.def("swap", &GPlatesMaths::PolylineOnSphere::swap)
		.def("is_close_to", &GPlatesMaths::PolylineOnSphere::is_close_to,
				args("test_point", "latitude_exclusion_threshold", "closeness"))
	;
	// def("get_antipodal_point", &GPlatesMaths::get_antipodal_point, args("p"));
	def("polylines_are_directed_equivalent", &GPlatesMaths::polylines_are_directed_equivalent,
			args("poly1", "poly2"));
	def("polylines_are_undirected_equivalent", &GPlatesMaths::polylines_are_undirected_equivalent,
			args("poly1", "poly2"));
}
#endif

