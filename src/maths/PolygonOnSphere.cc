/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2005, 2006, 2007, 2008 The University of Sydney, Australia
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

#include <ostream>
#include <sstream>
#include <boost/utility/in_place_factory.hpp>

#include "Centroid.h"
#include "ConstGeometryOnSphereVisitor.h"
#include "HighPrecision.h"
#include "PolygonOnSphere.h"
#include "PolygonProximityHitDetail.h"
#include "PolyGreatCircleArcBoundingTree.h"
#include "ProximityCriteria.h"
#include "SmallCircleBounds.h"
#include "SphericalArea.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/IntrusivePointerZeroRefCountException.h"
#include "global/InvalidParametersException.h"
#include "global/UninitialisedIteratorException.h"

#include "utils/ReferenceCount.h"


namespace GPlatesMaths
{
	namespace PolygonOnSphereImpl
	{
		/**
		 * Cached results of calculations performed on the polygon geometry.
		 */
		struct CachedCalculations :
				public GPlatesUtils::ReferenceCount<CachedCalculations>
		{
			explicit
			CachedCalculations() :
				// Start off in low-speed mode for point-in-polygon tests.
				// The user can specify faster speeds if they want (or used adaptive mode).
				point_in_polygon_speed_and_memory(PolygonOnSphere::LOW_SPEED_NO_SETUP_NO_MEMORY_USAGE),
				num_point_in_polygon_calls(0)
			{
				// Currently the speed has to be less than medium speed because otherwise the
				// point-in-polygon test will dereference the point-in-polygon tester which won't
				// be initialised - the lowest speed test requires no point-in-polygon tester.
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						point_in_polygon_speed_and_memory < PolygonOnSphere::MEDIUM_SPEED_MEDIUM_SETUP_MEDIUM_MEMORY_USAGE,
						GPLATES_ASSERTION_SOURCE);
			}

			boost::optional<UnitVector3D> centroid;
			boost::optional<InnerOuterBoundingSmallCircle> inner_outer_bounding_small_circle;
			boost::optional<real_t> area;
			boost::optional<PolygonOrientation::Orientation> orientation;

			PolygonOnSphere::PointInPolygonSpeedAndMemory point_in_polygon_speed_and_memory;
			unsigned int num_point_in_polygon_calls;
			boost::optional<PointInPolygon::Polygon> point_in_polygon_tester;
			boost::optional<PolygonOnSphere::bounding_tree_type> polygon_bounding_tree;
		};


		/**
		 * Build a point-in-polygon tester of medium or high (if @a high_speed is true) speed
		 * and cache the result in @a cached_calculations.
		 */
		void
		build_and_cache_point_in_polygon_tester(
				const PolygonOnSphere &polygon,
				CachedCalculations &cached_calculations,
				bool high_speed)
		{
			// Build an O(log N) point-in-polygon structure for the fastest point-in-polygon test.
			const bool build_ologn_hint = high_speed;

			// Note that we ask the point-in-polygon structure *not* to keep a shared reference
			// to us otherwise we get circular shared pointer references and a memory leak.
			cached_calculations.point_in_polygon_tester =
					PointInPolygon::Polygon(
							GPlatesUtils::get_non_null_pointer(&polygon),
							build_ologn_hint,
							false/*keep_shared_reference_to_polygon*/);

			cached_calculations.point_in_polygon_speed_and_memory =
					high_speed
					? PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE
					: PolygonOnSphere::MEDIUM_SPEED_MEDIUM_SETUP_MEDIUM_MEMORY_USAGE;
		}
	}
}


const unsigned
GPlatesMaths::PolygonOnSphere::s_min_num_collection_points = 3;


GPlatesMaths::PolygonOnSphere::~PolygonOnSphere()
{
	// Destructor defined in '.cc' so ~boost::intrusive_ptr<> has access to
	// PolygonOnSphereImpl::CachedCalculations.
}


GPlatesMaths::PolygonOnSphere::PolygonOnSphere() :
	GeometryOnSphere()
{
	// Constructor defined in '.cc' so ~boost::intrusive_ptr<> has access to
	// PolygonOnSphereImpl::CachedCalculations - because compiler must
	// generate code that destroys already constructed members if constructor throws.
}


GPlatesMaths::PolygonOnSphere::PolygonOnSphere(
		const PolygonOnSphere &other) :
	GeometryOnSphere(),
	d_seq(other.d_seq),
	// Since PolygonOnSphere is immutable we can just share the cached calculations.
	d_cached_calculations(other.d_cached_calculations)
{
	// Constructor defined in '.cc' so ~boost::intrusive_ptr<> has access to
	// PolygonOnSphereImpl::CachedCalculations - because compiler must
	// generate code that destroys already constructed members if constructor throws.
}


GPlatesMaths::PolygonOnSphere::ConstructionParameterValidity
GPlatesMaths::PolygonOnSphere::evaluate_segment_endpoint_validity(
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
GPlatesMaths::PolygonOnSphere::test_proximity(
		const ProximityCriteria &criteria) const
{
	// FIXME: This function should get its own implementation, rather than delegating to
	// 'is_close_to', to enable it to provide more hit detail (for example, whether a vertex or
	// a segment was hit).

	real_t closeness;  // Don't bother initialising this.
	if (this->is_close_to(criteria.test_point(), criteria.closeness_angular_extent_threshold(), closeness)) {
		// OK, this polygon is close to the test point.
		return make_maybe_null_ptr(PolygonProximityHitDetail::create(
				this->get_non_null_pointer(),
				closeness.dval()));
	} else {
		return ProximityHitDetail::null;
	}
}


GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
GPlatesMaths::PolygonOnSphere::test_vertex_proximity(
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
		return make_maybe_null_ptr(PolygonProximityHitDetail::create(
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
GPlatesMaths::PolygonOnSphere::accept_visitor(
		ConstGeometryOnSphereVisitor &visitor) const
{
	visitor.visit_polygon_on_sphere(this->get_non_null_pointer());
}


boost::optional<GPlatesMaths::PointOnSphere>
GPlatesMaths::PolygonOnSphere::is_close_to(
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
GPlatesMaths::PolygonOnSphere::create_segment_and_append_to_seq(
		seq_type &seq, 
		const PointOnSphere &p1,
		const PointOnSphere &p2)
{
	// We'll assume that the validity of 'p1' and 'p2' to create a GreatCircleArc has been
	// evaluated in the function 'PolygonOnSphere::evaluate_construction_parameter_validity',
	// which was presumably invoked in 'PolygonOnSphere::generate_segments_and_swap' before
	// this function was.

	GreatCircleArc segment = GreatCircleArc::create(p1, p2);
	seq.push_back(segment);
}


const GPlatesMaths::UnitVector3D &
GPlatesMaths::PolygonOnSphere::get_centroid() const
{
	if (!d_cached_calculations)
	{
		d_cached_calculations = new PolygonOnSphereImpl::CachedCalculations();
	}

	// Calculate the centroid if it's not cached.
	if (!d_cached_calculations->centroid)
	{
		d_cached_calculations->centroid = Centroid::calculate_centroid(*this);
	}

	return d_cached_calculations->centroid.get();
}


const GPlatesMaths::BoundingSmallCircle &
GPlatesMaths::PolygonOnSphere::get_bounding_small_circle() const
{
	return get_inner_outer_bounding_small_circle().get_outer_bounding_small_circle();
}


const GPlatesMaths::InnerOuterBoundingSmallCircle &
GPlatesMaths::PolygonOnSphere::get_inner_outer_bounding_small_circle() const
{
	if (!d_cached_calculations)
	{
		d_cached_calculations = new PolygonOnSphereImpl::CachedCalculations();
	}

	// Calculate the inner/outer bounding small circle if it's not cached.
	if (!d_cached_calculations->inner_outer_bounding_small_circle)
	{
		// The centroid will be the bounding small circle centre.
		InnerOuterBoundingSmallCircleBuilder inner_outer_bounding_small_circle_builder(get_centroid());
		// Add the polygon great-circle-arc sections to define the inner/outer bounds.
		inner_outer_bounding_small_circle_builder.add(*this);

		d_cached_calculations->inner_outer_bounding_small_circle =
				inner_outer_bounding_small_circle_builder.get_inner_outer_bounding_small_circle();
	}

	return d_cached_calculations->inner_outer_bounding_small_circle.get();
}


const GPlatesMaths::PolygonOnSphere::bounding_tree_type &
GPlatesMaths::PolygonOnSphere::get_bounding_tree() const
{
	if (!d_cached_calculations)
	{
		d_cached_calculations = new PolygonOnSphereImpl::CachedCalculations();
	}

	// Calculate the small circle bounding tree if it's not cached.
	if (!d_cached_calculations->polygon_bounding_tree)
	{
		// Pass the PolyGreatCircleArcBoundingTree constructor parameters to construct a new object
		// directly in-place inside the boost::optional since PolyGreatCircleArcBoundingTree is non-copyable.
		d_cached_calculations->polygon_bounding_tree = boost::in_place(begin(), end());
	}

	return d_cached_calculations->polygon_bounding_tree.get();
}


GPlatesMaths::real_t
GPlatesMaths::PolygonOnSphere::get_area() const
{
	return abs(get_signed_area());
}


GPlatesMaths::real_t
GPlatesMaths::PolygonOnSphere::get_signed_area() const
{
	if (!d_cached_calculations)
	{
		d_cached_calculations = new PolygonOnSphereImpl::CachedCalculations();
	}

	// Calculate the area of this polygon if it's not cached.
	if (!d_cached_calculations->area)
	{
		d_cached_calculations->area = SphericalArea::calculate_polygon_signed_area(*this);
	}

	return d_cached_calculations->area.get();
}


GPlatesMaths::PolygonOrientation::Orientation
GPlatesMaths::PolygonOnSphere::get_orientation() const
{
	if (!d_cached_calculations)
	{
		d_cached_calculations = new PolygonOnSphereImpl::CachedCalculations();
	}

	// Calculate the orientation of this polygon if it's not cached.
	if (!d_cached_calculations->orientation)
	{
		d_cached_calculations->orientation = PolygonOrientation::calculate_polygon_orientation(*this);
	}

	return d_cached_calculations->orientation.get();
}


GPlatesMaths::PointInPolygon::Result
GPlatesMaths::PolygonOnSphere::is_point_in_polygon(
		const PointOnSphere &point,
		PointInPolygonSpeedAndMemory speed_and_memory) const
{
	if (!d_cached_calculations)
	{
		d_cached_calculations = new PolygonOnSphereImpl::CachedCalculations();
	}

	// Keep track of the total number of calls for the adaptive speed mode.
	++d_cached_calculations->num_point_in_polygon_calls;

	switch (speed_and_memory)
	{
	case MEDIUM_SPEED_MEDIUM_SETUP_MEDIUM_MEMORY_USAGE:
	case HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE:
		// Set up the point-in-polygon structure if the caller has requested medium or high speed testing.
		// We only need to build a point-in-polygon structure if the caller has requested a speed
		// above the current speed setting.
		if (speed_and_memory > d_cached_calculations->point_in_polygon_speed_and_memory)
		{
			build_and_cache_point_in_polygon_tester(
					*this,
					*d_cached_calculations,
					speed_and_memory == HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE);
		}
		break;

	case ADAPTIVE:
		// Adapt the speed according to the number of point-in-polygon calls made so far.
		//
		// This is based on:
		//
		// LOW_SPEED_NO_SETUP_NO_MEMORY_USAGE              0 < N < 4     points tested per polygon,
		// MEDIUM_SPEED_MEDIUM_SETUP_MEDIUM_MEMORY_USAGE   4 < N < 200   points tested per polygon,
		// HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE         N > 200       points tested per polygon.
		if (d_cached_calculations->num_point_in_polygon_calls >= 200)
		{
			if (d_cached_calculations->point_in_polygon_speed_and_memory < HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE)
			{
				// High speed...
				build_and_cache_point_in_polygon_tester(*this, *d_cached_calculations, true/*high_speed*/);
			}
		}
		else if (d_cached_calculations->num_point_in_polygon_calls >= 4)
		{
			if (d_cached_calculations->point_in_polygon_speed_and_memory < MEDIUM_SPEED_MEDIUM_SETUP_MEDIUM_MEMORY_USAGE)
			{
				// Medium speed...
				build_and_cache_point_in_polygon_tester(*this, *d_cached_calculations, false/*high_speed*/);
			}
		}
		break;

	case LOW_SPEED_NO_SETUP_NO_MEMORY_USAGE:
	default:
		// Do nothing.
		//
		// Note that if the caller requests a low speed test but we have cached a medium or high
		// speed test then we'll use that since it's already there and it's faster.
		break;
	}

	// If we have an optimised point-in-polygon tester then use it.
	if (d_cached_calculations->point_in_polygon_tester)
	{
		return d_cached_calculations->point_in_polygon_tester->is_point_in_polygon(point);
	}

	// Since the low-speed test does not include a bounds test we will perform one here
	// (provided we have a bounding small circle) for quick rejection of points outside polygon.
	if (d_cached_calculations->inner_outer_bounding_small_circle)
	{
		if (d_cached_calculations->inner_outer_bounding_small_circle
			->get_outer_bounding_small_circle().test(point) == BoundingSmallCircle::OUTSIDE_BOUNDS)
		{
			return PointInPolygon::POINT_OUTSIDE_POLYGON;
		}
	}

	// The low speed test doesn't have any cached structures - it's just a function call.
	return PointInPolygon::is_point_in_polygon(point, *this);
}


void
GPlatesMaths::InvalidPointsForPolygonConstructionError::write_message(
		std::ostream &os) const
{
	const char *message;
	switch (d_cpv) {
	case PolygonOnSphere::VALID:
		message = "valid";
		break;
	case PolygonOnSphere::INVALID_INSUFFICIENT_DISTINCT_POINTS:
		message = "insufficient distinct points";
		break;
	case PolygonOnSphere::INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:
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

