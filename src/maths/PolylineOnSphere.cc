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

#include "Centroid.h"
#include "ConstGeometryOnSphereVisitor.h"
#include "HighPrecision.h"
#include "PolylineOnSphere.h"
#include "PolylineProximityHitDetail.h"
#include "PolyGreatCircleArcBoundingTree.h"
#include "ProximityCriteria.h"
#include "SmallCircleBounds.h"

#include "global/InvalidParametersException.h"
#include "global/UninitialisedIteratorException.h"
#include "global/IntrusivePointerZeroRefCountException.h"

#include "utils/ReferenceCount.h"


namespace GPlatesMaths
{
	namespace PolylineOnSphereImpl
	{
		/**
		 * Cached results of calculations performed on the polyline geometry.
		 */
		struct CachedCalculations :
				public GPlatesUtils::ReferenceCount<CachedCalculations>
		{
			boost::optional<UnitVector3D> centroid;
			boost::optional<BoundingSmallCircle> bounding_small_circle;
			boost::optional<PolylineOnSphere::bounding_tree_type> polyline_bounding_tree;
		};
	}
}


const unsigned
GPlatesMaths::PolylineOnSphere::s_min_num_collection_points = 2;


GPlatesMaths::PolylineOnSphere::~PolylineOnSphere()
{
	// Destructor defined in '.cc' so ~boost::intrusive_ptr<> has access to
	// PolylineOnSphereImpl::CachedCalculations.
}


GPlatesMaths::PolylineOnSphere::PolylineOnSphere() :
	GeometryOnSphere()
{
	// Constructor defined in '.cc' so ~boost::intrusive_ptr<> has access to
	// PolylineOnSphereImpl::CachedCalculations - because compiler must
	// generate code that destroys already constructed members if constructor throws.
}


GPlatesMaths::PolylineOnSphere::PolylineOnSphere(
		const PolylineOnSphere &other) :
	GeometryOnSphere(),
	d_seq(other.d_seq),
	// Since PolylineOnSphere is immutable we can just share the cached calculations.
	d_cached_calculations(other.d_cached_calculations)
{
	// Constructor defined in '.cc' so ~boost::intrusive_ptr<> has access to
	// PolylineOnSphereImpl::CachedCalculations - because compiler must
	// generate code that destroys already constructed members if constructor throws.
}


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


GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
GPlatesMaths::PolylineOnSphere::test_proximity(
		const ProximityCriteria &criteria) const
{
	// FIXME: This function should get its own implementation, rather than delegating to
	// 'is_close_to', to enable it to provide more hit detail (for example, whether a vertex or
	// a segment was hit).

	real_t closeness;  // Don't bother initialising this.
	if (this->is_close_to(criteria.test_point(), criteria.closeness_angular_extent_threshold(), closeness)) {
		// OK, this polyline is close to the test point.
		return make_maybe_null_ptr(PolylineProximityHitDetail::create(
				this->get_non_null_pointer(),
				closeness.dval()));
	} else {
		return ProximityHitDetail::null;
	}
}


GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
GPlatesMaths::PolylineOnSphere::test_vertex_proximity(
		const ProximityCriteria &criteria) const
{
	vertex_const_iterator 
		v_it = vertex_begin(),
		v_end = vertex_end();

	real_t closeness;
	real_t closest_closeness_so_far = 0.;
	unsigned int index_of_closest_closeness = 0;
	bool have_found_close_vertex = false;
	for (unsigned int index = 0; v_it != v_end ; ++v_it, ++index)
	{
		ProximityHitDetail::maybe_null_ptr_type hit = v_it->test_proximity(criteria);
		if (hit)
		{
			have_found_close_vertex = true;
			closeness = hit->closeness();
			if (closeness.is_precisely_greater_than(closest_closeness_so_far.dval()))
			{
				closest_closeness_so_far = closeness;
				index_of_closest_closeness = index;
			}
		}
	}	
	
	if (have_found_close_vertex)
	{
		return make_maybe_null_ptr(PolylineProximityHitDetail::create(
			this->get_non_null_pointer(),
			closest_closeness_so_far.dval(),
			index_of_closest_closeness));
	}
	else
	{
		return ProximityHitDetail::null;	 
	}

}

void
GPlatesMaths::PolylineOnSphere::accept_visitor(
		ConstGeometryOnSphereVisitor &visitor) const
{
	visitor.visit_polyline_on_sphere(this->get_non_null_pointer());
}


boost::optional<GPlatesMaths::PointOnSphere>
GPlatesMaths::PolylineOnSphere::is_close_to(
		const PointOnSphere &test_point,
		const AngularExtent &closeness_angular_extent_threshold,
		real_t &closeness) const
{
	real_t &closest_closeness_so_far = closeness;  // A descriptive alias.
	boost::optional<PointOnSphere> closest_point;

	const_iterator iter = begin(), the_end = end();
	for ( ; iter != the_end; ++iter)
	{
		const GreatCircleArc &the_gca = *iter;

		// No need to initialise this - to -1 (ie, min-dot-product).
		real_t gca_closeness;

		boost::optional<PointOnSphere> gca_closest_point =
				the_gca.is_close_to(
						test_point,
						closeness_angular_extent_threshold,
						gca_closeness);
		if (gca_closest_point)
		{
			if (closest_point)
			{
				if (gca_closeness.is_precisely_greater_than(closest_closeness_so_far.dval()))
				{
					closest_closeness_so_far = gca_closeness;
					closest_point = gca_closest_point.get();
				}
				// else, do nothing.
			}
			else
			{
				closest_closeness_so_far = gca_closeness;
				closest_point = gca_closest_point.get();
			}
		}
	}

	return closest_point;
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


const GPlatesMaths::UnitVector3D &
GPlatesMaths::PolylineOnSphere::get_centroid() const
{
	if (!d_cached_calculations)
	{
		d_cached_calculations = new PolylineOnSphereImpl::CachedCalculations();
	}

	// Calculate the centroid if it's not cached.
	if (!d_cached_calculations->centroid)
	{
		// The centroid is also the bounding small circle centre so see if that's been generated.
		if (d_cached_calculations->bounding_small_circle)
		{
			d_cached_calculations->centroid =
					d_cached_calculations->bounding_small_circle->get_centre();
		}
		else
		{
			d_cached_calculations->centroid = Centroid::calculate_centroid(*this);
		}
	}

	return d_cached_calculations->centroid.get();
}


const GPlatesMaths::BoundingSmallCircle &
GPlatesMaths::PolylineOnSphere::get_bounding_small_circle() const
{
	if (!d_cached_calculations)
	{
		d_cached_calculations = new PolylineOnSphereImpl::CachedCalculations();
	}

	// Calculate the bounding small circle if it's not cached.
	if (!d_cached_calculations->bounding_small_circle)
	{
		// The centroid will be the bounding small circle centre.
		BoundingSmallCircleBuilder bounding_small_circle_builder(get_centroid());
		// Add the polyline great-circle-arc sections to define the bounds.
		bounding_small_circle_builder.add(*this);

		d_cached_calculations->bounding_small_circle =
				bounding_small_circle_builder.get_bounding_small_circle();
	}

	return d_cached_calculations->bounding_small_circle.get();
}


const GPlatesMaths::PolylineOnSphere::bounding_tree_type &
GPlatesMaths::PolylineOnSphere::get_bounding_tree() const
{
	if (!d_cached_calculations)
	{
		d_cached_calculations = new PolylineOnSphereImpl::CachedCalculations();
	}

	// Calculate the small circle bounding tree if it's not cached.
	if (!d_cached_calculations->polyline_bounding_tree)
	{
		// Pass the PolyGreatCircleArcBoundingTree constructor parameters to construct a new object
		// directly in-place inside the boost::optional since PolyGreatCircleArcBoundingTree is non-copyable.
		d_cached_calculations->polyline_bounding_tree = boost::in_place(begin(), end());
	}

	return d_cached_calculations->polyline_bounding_tree.get();
}


const GPlatesMaths::PointOnSphere &
GPlatesMaths::PolylineOnSphere::VertexConstIterator::dereference() const
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
GPlatesMaths::PolylineOnSphere::VertexConstIterator::equal(
		const VertexConstIterator &other) const
{
	if (d_poly_ptr == NULL || other.d_poly_ptr == NULL)
	{
		// We can't really return a valid comparison result.
		throw GPlatesGlobal::UninitialisedIteratorException(
				GPLATES_EXCEPTION_SOURCE,
				"Attempted to compare an uninitialised iterator.");
	}

	return d_curr_gca == other.d_curr_gca &&
			d_gca_start_or_end == other.d_gca_start_or_end;
}


void
GPlatesMaths::PolylineOnSphere::VertexConstIterator::advance(
		VertexConstIterator::difference_type n)
{
	if (d_poly_ptr == NULL)
	{
		// This iterator is uninitialised, so this function will be a no-op.
		return;
	}

	if (n > 0)
	{
		if (d_curr_gca == d_poly_ptr->begin() && d_gca_start_or_end == START)
		{
			// Advance by one.
 			d_gca_start_or_end = END;
 			--n;

			// Advance any remaining amount.
			if (n > 0)
			{
				std::advance(d_curr_gca, n);
			}
		}
		else
		{
			std::advance(d_curr_gca, n);
		}
	}
	else if (n < 0)
	{
		if (d_curr_gca == d_poly_ptr->begin() && d_gca_start_or_end == END)
		{
			// Advance by minus one.
			d_gca_start_or_end = START;
			++n;

			// Advance any remaining amount.
			//
			// Actually this shouldn't be able to happen since we're already at the beginning
			// of the sequence, but we'll advance as requested.
			//
			// TODO: Should we check and throw exception or assert ?
			// The MSVC 'std' library only checks iterators in debug builds.
			if (n < 0)
			{
				std::advance(d_curr_gca, n);
			}
		}
		else
		{
			std::advance(d_curr_gca, n);
		}
	}
}


GPlatesMaths::PolylineOnSphere::VertexConstIterator::difference_type
GPlatesMaths::PolylineOnSphere::VertexConstIterator::distance_to(
		const VertexConstIterator &other) const
{
	if (d_poly_ptr == NULL || other.d_poly_ptr == NULL)
	{
		// We can't really return a valid distance result.
		throw GPlatesGlobal::UninitialisedIteratorException(
				GPLATES_EXCEPTION_SOURCE,
				"Attempted to compare an uninitialised iterator.");
	}

	difference_type difference = std::distance(d_curr_gca, other.d_curr_gca);

	// Make adjustments if either, or both, iterators reference the first point in the sequence.
	if (d_curr_gca == d_poly_ptr->begin() && d_gca_start_or_end == START)
	{
		++difference;
	}
	if (other.d_curr_gca == other.d_poly_ptr->begin() && other.d_gca_start_or_end == START)
	{
		--difference;
	}

	return difference;
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
			poly2_iter = poly2.vertex_begin();
			//poly2_end = poly2.vertex_end();
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

