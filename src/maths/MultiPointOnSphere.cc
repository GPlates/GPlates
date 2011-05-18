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

#include "MultiPointOnSphere.h"

#include "Centroid.h"
#include "ConstGeometryOnSphereVisitor.h"
#include "MultiPointProximityHitDetail.h"
#include "ProximityCriteria.h"
#include "SmallCircleBounds.h"

#include "global/IntrusivePointerZeroRefCountException.h"

#include "utils/ReferenceCount.h"


namespace GPlatesMaths
{
	namespace MultiPointOnSphereImpl
	{
		/**
		 * Cached results of calculations performed on the multipoint geometry.
		 */
		struct CachedCalculations :
				public GPlatesUtils::ReferenceCount<CachedCalculations>
		{
			boost::optional<BoundingSmallCircle> bounding_small_circle;
		};
	}
}


const unsigned
GPlatesMaths::MultiPointOnSphere::s_min_num_collection_points = 1;


GPlatesMaths::MultiPointOnSphere::~MultiPointOnSphere()
{
	// Destructor defined in '.cc' so ~boost::intrusive_ptr<> has access to
	// MultiPointOnSphereImpl::CachedCalculations.
}


GPlatesMaths::MultiPointOnSphere::MultiPointOnSphere() :
	GeometryOnSphere()
{
	// Constructor defined in '.cc' so ~boost::intrusive_ptr<> has access to
	// MultiPointOnSphereImpl::CachedCalculations - because compiler must
	// generate code that destroys already constructed members if constructor throws.
}


GPlatesMaths::MultiPointOnSphere::MultiPointOnSphere(
		const MultiPointOnSphere &other) :
	GeometryOnSphere(),
	d_points(other.d_points),
	// Since MultiPointOnSphere is immutable we can just share the cached calculations.
	d_cached_calculations(other.d_cached_calculations)
{
	// Constructor defined in '.cc' so ~boost::intrusive_ptr<> has access to
	// MultiPointOnSphereImpl::CachedCalculations - because compiler must
	// generate code that destroys already constructed members if constructor throws.
}


GPlatesMaths::MultiPointOnSphere &
GPlatesMaths::MultiPointOnSphere::operator=(
		const MultiPointOnSphere &other)
{
	// Use the copy+swap idiom to enable strong exception safety.
	MultiPointOnSphere dup(other);
	this->swap(dup);
	return *this;
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

GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
GPlatesMaths::MultiPointOnSphere::test_vertex_proximity(
	const ProximityCriteria &criteria) const
{
	const_iterator 
		it = begin(),
		it_end = end();
		

	real_t closeness;
	real_t closest_closeness_so_far = 0.;
	unsigned int index_of_closest_closeness = 0;
	bool have_found_close_vertex = false;
	for (unsigned int index = 0; it != it_end ; ++it, ++index)
	{
		ProximityHitDetail::maybe_null_ptr_type hit = it->test_proximity(criteria);
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
		return make_maybe_null_ptr(MultiPointProximityHitDetail::create(
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
				if (point_closeness.is_precisely_greater_than(
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


const GPlatesMaths::UnitVector3D &
GPlatesMaths::MultiPointOnSphere::get_centroid() const
{
	// We use the centroid for the centre of the bounding small circle.
	// Getting it from there avoids having to store it twice.
	// There's extra calculations required to generate the bounding small circle but
	// generally if the client wants the centroid they also want the bounding small circle.
	return get_bounding_small_circle().get_centre();
}


const GPlatesMaths::BoundingSmallCircle &
GPlatesMaths::MultiPointOnSphere::get_bounding_small_circle() const
{
	if (!d_cached_calculations)
	{
		d_cached_calculations = new MultiPointOnSphereImpl::CachedCalculations();
	}

	// Calculate the bounding small circle if it's not cached.
	if (!d_cached_calculations->bounding_small_circle)
	{
		// The centroid will be the bounding small circle centre.
		BoundingSmallCircleBuilder bounding_small_circle_builder(
				Centroid::calculate_points_centroid(*this));
		// Add the points to define the bounds.
		bounding_small_circle_builder.add(*this);

		d_cached_calculations->bounding_small_circle =
				bounding_small_circle_builder.get_bounding_small_circle();
	}

	return d_cached_calculations->bounding_small_circle.get();
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

