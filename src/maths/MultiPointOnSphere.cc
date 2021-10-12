/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifdef HAVE_PYTHON
# include <boost/python.hpp>
#endif
#include "MultiPointOnSphere.h"
#include "MultiPointProximityHitDetail.h"
#include "ProximityCriteria.h"
#include "ConstGeometryOnSphereVisitor.h"
#include "global/IntrusivePointerZeroRefCountException.h"


const unsigned
GPlatesMaths::MultiPointOnSphere::s_min_num_collection_points = 1;


const GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type
GPlatesMaths::MultiPointOnSphere::get_non_null_pointer() const
{
	if (get_reference_count() == 0) {
		// How did this happen?  This should not have happened.
		//
		// Presumably, the programmer obtained the raw MultiPointOnSphere pointer from
		// inside a MultiPointOnSphere::non_null_ptr_type, and is invoking this member
		// function upon the instance indicated by the raw pointer, after all ref-counting
		// pointers have expired and the instance has actually been deleted.
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
GPlatesMaths::MultiPointOnSphere::test_proximity(
		const ProximityCriteria &criteria) const
{
	// FIXME: This function should get its own implementation, rather than delegating to
	// 'is_close_to', to enable it to provide more hit detail (for example, which point was
	// hit).

	real_t closeness;  // Don't bother initialising this.
	if (this->is_close_to(criteria.test_point(), criteria.closeness_inclusion_threshold(),
			closeness)) {
		// OK, this multi-point is close to the test point.
		return make_maybe_null_ptr(MultiPointProximityHitDetail::create(
				this->get_non_null_pointer(),
				closeness.dval()));
	} else {
		return ProximityHitDetail::null;
	}
}


void
GPlatesMaths::MultiPointOnSphere::accept_visitor(
		ConstGeometryOnSphereVisitor &visitor) const
{
	visitor.visit_multi_point_on_sphere(this->get_non_null_pointer());
}


bool
GPlatesMaths::MultiPointOnSphere::is_close_to(
		const PointOnSphere &test_point,
		const real_t &closeness_inclusion_threshold,
		real_t &closeness) const
{
	real_t &closest_closeness_so_far = closeness;  // A descriptive alias.
	bool have_already_found_a_close_point = false;

	const_iterator iter = begin(), the_end = end();
	for ( ; iter != the_end; ++iter) {
		const PointOnSphere &the_point = *iter;

		// Don't bother initialising this.
		real_t point_closeness;

		if (the_point.is_close_to(test_point,
				closeness_inclusion_threshold, point_closeness))
		{
			if (have_already_found_a_close_point) {
				if (point_closeness.isPreciselyGreaterThan(
							closest_closeness_so_far.dval()))
				{
					closest_closeness_so_far = point_closeness;
				}
				// else, do nothing.
			} else {
				closest_closeness_so_far = point_closeness;
				have_already_found_a_close_point = true;
			}
		}
	}
	return have_already_found_a_close_point;
}


bool
GPlatesMaths::multi_points_are_ordered_equivalent(
		const MultiPointOnSphere &mp1,
		const MultiPointOnSphere &mp2)
{
	if (mp1.number_of_points() != mp2.number_of_points()) {
		// There is no way the two multi-points can be equivalent.
		return false;
	}
	// Else, we know the two multi-points contain the same number of points,
	// so we only need to check the end-of-sequence conditions for 'mp1'
	// in our iteration.

	MultiPointOnSphere::const_iterator mp1_iter = mp1.begin();
	MultiPointOnSphere::const_iterator mp1_end = mp1.end();
	MultiPointOnSphere::const_iterator mp2_iter = mp2.begin();
	MultiPointOnSphere::const_iterator mp2_end = mp2.end();
	for ( ; mp1_iter != mp1_end; ++mp1_iter, ++mp2_iter) {
		if ( ! points_are_coincident(*mp1_iter, *mp2_iter)) {
			return false;
		}
	}
	return true;
}


#ifdef HAVE_PYTHON
/**
 * Here begin the Python wrappers
 */


using namespace boost::python;


void
GPlatesMaths::export_MultiPointOnSphere()
{
	class_<GPlatesMaths::MultiPointOnSphere>("MultiPointOnSphere", init<const MultiPointOnSphere &>())
		// .def("create_on_heap", &GPlatesMaths::MultiPointOnSphere::create_on_heap).staticmethod("create_on_heap")
		// .def("clone_on_heap", &GPlatesMaths::MultiPointOnSphere::clone_on_heap).staticmethod("clone_on_heap")
		// *** VARIOUS TEMPLATE CONSTRUCTORS ***
		//.def("assign", &GPlatesMaths::MultiPointOnSphere::operator=)
		.def("number_of_vertices", &GPlatesMaths::MultiPointOnSphere::number_of_points)
		.def("start_point", &GPlatesMaths::MultiPointOnSphere::start_point,
				return_internal_reference<1>())
		.def("end_point", &GPlatesMaths::MultiPointOnSphere::end_point,
				return_internal_reference<1>())
		.def("swap", &GPlatesMaths::MultiPointOnSphere::swap)
		.def("is_close_to", &GPlatesMaths::MultiPointOnSphere::is_close_to,
				args("test_point", "latitude_exclusion_threshold", "closeness"))
	;
}
#endif

