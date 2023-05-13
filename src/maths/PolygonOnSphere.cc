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
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/utility/in_place_factory.hpp>

#include "Centroid.h"
#include "ConstGeometryOnSphereVisitor.h"
#include "HighPrecision.h"
#include "PointInPolygon.h"
#include "PolygonOnSphere.h"
#include "PolygonProximityHitDetail.h"
#include "PolyGreatCircleArcBoundingTree.h"
#include "ProximityCriteria.h"
#include "SmallCircleBounds.h"
#include "SphericalArea.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/InvalidParametersException.h"
#include "global/UninitialisedIteratorException.h"

#include "scribe/Scribe.h"

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

			boost::optional<real_t> exterior_ring_arc_length;
			boost::optional< std::vector<real_t> > interior_ring_arc_lengths;
			boost::optional<UnitVector3D> outline_centroid_including_interior_rings;
			boost::optional<UnitVector3D> outline_centroid_excluding_interior_rings;
			boost::optional<UnitVector3D> interior_centroid;
			boost::optional<InnerOuterBoundingSmallCircle> inner_outer_bounding_small_circle;
			boost::optional<real_t> signed_area;
			boost::optional<PolygonOrientation::Orientation> orientation;

			PolygonOnSphere::PointInPolygonSpeedAndMemory point_in_polygon_speed_and_memory;
			unsigned int num_point_in_polygon_calls;
			boost::optional<PointInPolygon::Polygon> point_in_polygon_tester;
			boost::optional<PolygonOnSphere::bounding_tree_type> polygon_bounding_tree;
			boost::optional<PolygonOnSphere::ring_bounding_tree_type> exterior_polygon_bounding_tree;
			boost::optional< std::vector< boost::shared_ptr<PolygonOnSphere::ring_bounding_tree_type> > > interior_polygon_bounding_trees;
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


	namespace
	{
		void
		is_close_to_polygon_ring(
				const PolygonOnSphere::ring_const_iterator &ring_begin,
				const PolygonOnSphere::ring_const_iterator &ring_end,
				const PointOnSphere &test_point,
				const AngularExtent &closeness_angular_extent_threshold,
				real_t &closest_closeness_so_far,
				boost::optional<PointOnSphere> &closest_point)
		{
			PolygonOnSphere::ring_const_iterator ring_iter = ring_begin;
			for ( ; ring_iter != ring_end; ++ring_iter)
			{
				const GreatCircleArc &gca = *ring_iter;

				// No need to initialise this to -1 (ie, min-dot-product).
				real_t gca_closeness;

				boost::optional<PointOnSphere> gca_closest_point =
						gca.is_close_to(
								test_point,
								closeness_angular_extent_threshold,
								gca_closeness);
				if (gca_closest_point)
				{
					if (!closest_point ||
						gca_closeness.is_precisely_greater_than(closest_closeness_so_far.dval()))
					{
						closest_closeness_so_far = gca_closeness;
						closest_point = gca_closest_point.get();
					}
				}
			}
		}


		real_t
		calculate_ring_arc_length(
				const PolygonOnSphere::ring_const_iterator &ring_begin,
				const PolygonOnSphere::ring_const_iterator &ring_end)
		{
			real_t ring_arc_length(0);

			PolygonOnSphere::ring_const_iterator ring_iter = ring_begin;
			for ( ; ring_iter != ring_end; ++ring_iter)
			{
				const GreatCircleArc &gca = *ring_iter;

				ring_arc_length += gca.arc_length();
			}

			return ring_arc_length;
		}


		void
		tessellate_ring(
				std::vector<PointOnSphere> &tessellated_ring_points,
				const PolygonOnSphere::ring_const_iterator &ring_begin,
				const PolygonOnSphere::ring_const_iterator &ring_end,
				const real_t &max_angular_extent)
		{
			PolygonOnSphere::ring_const_iterator ring_iter = ring_begin;
			for ( ; ring_iter != ring_end; ++ring_iter)
			{
				const GreatCircleArc &gca = *ring_iter;

				// Tessellate the current great circle arc.
				tessellate(tessellated_ring_points, gca, max_angular_extent);

				// Remove the tessellated arc's end point.
				// Otherwise the next arc's start point will duplicate it.
				//
				// NOTE: We also remove the *last* arc's end point because otherwise the start point
				// of the *first* arc will duplicate it.
				//
				// Tessellating a great circle arc should always add at least two points.
				// So we should always be able to remove one point (the arc end point).
				tessellated_ring_points.pop_back();
			}
		}
	}
}


const unsigned
GPlatesMaths::PolygonOnSphere::s_min_num_ring_points = 3;


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


GPlatesMaths::PolygonOnSphere::ConstructionParameterValidity
GPlatesMaths::PolygonOnSphere::evaluate_segment_endpoint_validity(
		const PointOnSphere &p1,
		const PointOnSphere &p2)
{
	GreatCircleArc::ConstructionParameterValidity cpv =
			GreatCircleArc::evaluate_construction_parameter_validity(p1, p2);

	// Using a switch-statement, along with GCC's "-Wswitch" option (implicitly enabled by
	// "-Wall"), will help to ensure that no cases are missed.
	switch (cpv)
	{
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
	if (this->is_close_to(criteria.test_point(), criteria.closeness_angular_extent_threshold(), closeness))
	{
		// OK, this polygon is close to the test point.
		return make_maybe_null_ptr(PolygonProximityHitDetail::create(
				this->get_non_null_pointer(),
				closeness.dval()));
	}
	else
	{
		return ProximityHitDetail::null;
	}
}


GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
GPlatesMaths::PolygonOnSphere::test_vertex_proximity(
		const ProximityCriteria &criteria) const
{
	ring_vertex_const_iterator 
		v_it = exterior_ring_vertex_begin(),
		v_end = exterior_ring_vertex_end();

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

	is_close_to_polygon_ring(
			exterior_ring_begin(),
			exterior_ring_end(),
			test_point,
			closeness_angular_extent_threshold,
			closest_closeness_so_far,
			closest_point);

	for (unsigned int interior_ring_index = 0;
		interior_ring_index < number_of_interior_rings();
		++interior_ring_index)
	{
		is_close_to_polygon_ring(
				interior_ring_begin(interior_ring_index),
				interior_ring_end(interior_ring_index),
				test_point,
				closeness_angular_extent_threshold,
				closest_closeness_so_far,
				closest_point);
	}

	return closest_point;
}


GPlatesMaths::real_t
GPlatesMaths::PolygonOnSphere::get_arc_length() const
{
	real_t arc_length = get_exterior_ring_arc_length();

	// Handle common case where polygon has no interior rings.
	const unsigned int num_interior_rings = number_of_interior_rings();
	if (num_interior_rings == 0)
	{
		return arc_length;
	}

	for (unsigned int interior_ring_index = 0;
		interior_ring_index < num_interior_rings;
		++interior_ring_index)
	{
		arc_length += get_interior_ring_arc_length(interior_ring_index);
	}

	return arc_length;
}


const GPlatesMaths::real_t &
GPlatesMaths::PolygonOnSphere::get_exterior_ring_arc_length() const
{
	if (!d_cached_calculations)
	{
		d_cached_calculations = new PolygonOnSphereImpl::CachedCalculations();
	}

	// Calculate the total exterior arc length if it's not cached.
	if (!d_cached_calculations->exterior_ring_arc_length)
	{
		d_cached_calculations->exterior_ring_arc_length = calculate_ring_arc_length(
				exterior_ring_begin(),
				exterior_ring_end());
	}

	return d_cached_calculations->exterior_ring_arc_length.get();
}


const GPlatesMaths::real_t &
GPlatesMaths::PolygonOnSphere::get_interior_ring_arc_length(
		unsigned int interior_ring_index) const
{
	if (!d_cached_calculations)
	{
		d_cached_calculations = new PolygonOnSphereImpl::CachedCalculations();
	}

	const unsigned int num_interior_rings = number_of_interior_rings();

	// Calculate the total arc length of each interior ring if they're not cached.
	if (!d_cached_calculations->interior_ring_arc_lengths)
	{
		d_cached_calculations->interior_ring_arc_lengths = std::vector<real_t>();
		std::vector<real_t> &interior_arc_lengths = d_cached_calculations->interior_ring_arc_lengths.get();

		interior_arc_lengths.reserve(num_interior_rings);

		for (unsigned int i = 0; i < num_interior_rings; ++i)
		{
			interior_arc_lengths.push_back(
					calculate_ring_arc_length(
							interior_ring_begin(i),
							interior_ring_end(i)));
		}
	}

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			interior_ring_index < num_interior_rings,
			GPLATES_ASSERTION_SOURCE);

	return d_cached_calculations->interior_ring_arc_lengths.get()[interior_ring_index];
}


GPlatesMaths::real_t
GPlatesMaths::PolygonOnSphere::get_area() const
{
	return abs(get_signed_area());
}


const GPlatesMaths::real_t &
GPlatesMaths::PolygonOnSphere::get_signed_area() const
{
	if (!d_cached_calculations)
	{
		d_cached_calculations = new PolygonOnSphereImpl::CachedCalculations();
	}

	// Calculate the area of this polygon if it's not cached.
	if (!d_cached_calculations->signed_area)
	{
		d_cached_calculations->signed_area = SphericalArea::calculate_polygon_signed_area(*this);
	}

	return d_cached_calculations->signed_area.get();
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
		// If we already have the signed area then just use that.
		if (d_cached_calculations->signed_area)
		{
			d_cached_calculations->orientation =
					d_cached_calculations->signed_area->is_precisely_less_than(0)
					? PolygonOrientation::CLOCKWISE
					: PolygonOrientation::COUNTERCLOCKWISE;
		}
		else
		{
			d_cached_calculations->orientation =
					PolygonOrientation::calculate_polygon_orientation(*this);
		}
	}

	return d_cached_calculations->orientation.get();
}


bool
GPlatesMaths::PolygonOnSphere::is_point_in_polygon(
		const PointOnSphere &point,
		PointInPolygonSpeedAndMemory speed_and_memory,
		bool use_point_on_polygon_threshold) const
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
		return d_cached_calculations->point_in_polygon_tester->is_point_in_polygon(point, use_point_on_polygon_threshold);
	}

	// Since the low-speed test does not include a bounds test we will perform one here
	// (provided we have a bounding small circle) for quick rejection of points outside polygon.
	if (d_cached_calculations->inner_outer_bounding_small_circle)
	{
		if (d_cached_calculations->inner_outer_bounding_small_circle
			->get_outer_bounding_small_circle().test(point) == BoundingSmallCircle::OUTSIDE_BOUNDS)
		{
			// Point is outside polygon.
			return false;
		}
	}

	// The low speed test doesn't have any cached structures - it's just a function call.
	return PointInPolygon::is_point_in_polygon(point, *this, use_point_on_polygon_threshold);
}


const GPlatesMaths::UnitVector3D &
GPlatesMaths::PolygonOnSphere::get_boundary_centroid() const
{
	if (!d_cached_calculations)
	{
		d_cached_calculations = new PolygonOnSphereImpl::CachedCalculations();
	}

	// Calculate the centroid excluding interior rings if it's not cached.
	if (!d_cached_calculations->outline_centroid_excluding_interior_rings)
	{
		d_cached_calculations->outline_centroid_excluding_interior_rings =
				Centroid::calculate_outline_centroid(*this, false/*use_interior_rings*/);
	}

	return d_cached_calculations->outline_centroid_excluding_interior_rings.get();
}


const GPlatesMaths::UnitVector3D &
GPlatesMaths::PolygonOnSphere::get_outline_centroid(
		bool use_interior_rings) const
{
	if (!use_interior_rings)
	{
		return get_boundary_centroid();
	}

	if (!d_cached_calculations)
	{
		d_cached_calculations = new PolygonOnSphereImpl::CachedCalculations();
	}

	// Calculate the centroid including interior rings if it's not cached.
	if (!d_cached_calculations->outline_centroid_including_interior_rings)
	{
		// If there are no interior rings then just re-use the boundary centroid and cache it.
		if (number_of_interior_rings() == 0)
		{
			d_cached_calculations->outline_centroid_including_interior_rings = get_boundary_centroid();
		}
		else
		{
			d_cached_calculations->outline_centroid_including_interior_rings =
					Centroid::calculate_outline_centroid(*this, true/*use_interior_rings*/);
		}
	}

	return d_cached_calculations->outline_centroid_including_interior_rings.get();
}


const GPlatesMaths::UnitVector3D &
GPlatesMaths::PolygonOnSphere::get_interior_centroid() const
{
	if (!d_cached_calculations)
	{
		d_cached_calculations = new PolygonOnSphereImpl::CachedCalculations();
	}

	// Calculate the centroid if it's not cached.
	if (!d_cached_calculations->interior_centroid)
	{
		d_cached_calculations->interior_centroid = Centroid::calculate_interior_centroid(*this);
	}

	return d_cached_calculations->interior_centroid.get();
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
		// The boundary centroid will be the bounding small circle centre.
		InnerOuterBoundingSmallCircleBuilder inner_outer_bounding_small_circle_builder(
				get_boundary_centroid());
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

	// Calculate the small circle bounding tree for *all* rings, if it's not cached.
	if (!d_cached_calculations->polygon_bounding_tree)
	{
		// Since our 'const_iterator' covers all rings (exterior and interior) we need to partition
		// the sequence into separate disconnected partitions (rings) to get a good bounding tree.
		//
		// We only need separators *between* partitions (rings) which means we only need to insert
		// separators at the beginning of interior rings. Note that the beginning of the first
		// interior ring is the same as the end of the exterior ring. So we advance our 'const_iterator'
		// to the beginning of each interior ring and copy those iterators as partition separators.
		boost::optional<const bounding_tree_type::partition_separator_seq_type &> partition_separators;
		bounding_tree_type::partition_separator_seq_type partition_separators_storage;
		if (!d_interior_rings.empty())
		{
			// The first partition separator is at the end of the exterior ring
			// (which is also the beginning of the first interior ring).
			const_iterator partition_separator = begin();
			std::advance(partition_separator, d_exterior_ring.size());

			ring_sequence_const_iterator interior_ring_seq_iter = d_interior_rings.begin();
			ring_sequence_const_iterator interior_ring_seq_end = d_interior_rings.end();
			for ( ; interior_ring_seq_iter != interior_ring_seq_end; ++interior_ring_seq_iter)
			{
				partition_separators_storage.push_back(partition_separator);

				// Advance to the beginning of the next interior ring.
				const ring_type &interior_ring = *interior_ring_seq_iter;
				std::advance(partition_separator, interior_ring.size());
			}

			// We're using partitions (since have interior rings).
			partition_separators = partition_separators_storage;
		}
		// else not using partitions, so leave 'partition_separators' as none.

		// Pass the PolyGreatCircleArcBoundingTree constructor parameters to construct a new object
		// directly in-place inside the boost::optional since PolyGreatCircleArcBoundingTree is non-copyable.
		//
		// Note that we *don't* ask the bounding tree to keep a shared reference to us
		// otherwise we get circular shared pointer references and a memory leak.
		d_cached_calculations->polygon_bounding_tree = boost::in_place(begin(), end(), partition_separators);
	}

	return d_cached_calculations->polygon_bounding_tree.get();
}


const GPlatesMaths::PolygonOnSphere::ring_bounding_tree_type &
GPlatesMaths::PolygonOnSphere::get_exterior_ring_bounding_tree() const
{
	if (!d_cached_calculations)
	{
		d_cached_calculations = new PolygonOnSphereImpl::CachedCalculations();
	}

	// Calculate the exterior small circle bounding tree if it's not cached.
	if (!d_cached_calculations->exterior_polygon_bounding_tree)
	{
		// Pass the PolyGreatCircleArcBoundingTree constructor parameters to construct a new object
		// directly in-place inside the boost::optional since PolyGreatCircleArcBoundingTree is non-copyable.
		//
		// Note that we *don't* ask the bounding tree to keep a shared reference to us
		// otherwise we get circular shared pointer references and a memory leak.
		d_cached_calculations->exterior_polygon_bounding_tree = boost::in_place(exterior_ring_begin(), exterior_ring_end());
	}

	return d_cached_calculations->exterior_polygon_bounding_tree.get();
}


const GPlatesMaths::PolygonOnSphere::ring_bounding_tree_type &
GPlatesMaths::PolygonOnSphere::get_interior_ring_bounding_tree(
		unsigned int interior_ring_index) const
{
	if (!d_cached_calculations)
	{
		d_cached_calculations = new PolygonOnSphereImpl::CachedCalculations();
	}

	const unsigned int num_interior_rings = number_of_interior_rings();

	// Calculate the small circle bounding tree of each interior ring if they're not cached.
	if (!d_cached_calculations->interior_polygon_bounding_trees)
	{
		typedef std::vector< boost::shared_ptr<ring_bounding_tree_type> > bounding_tree_seq_type;

		d_cached_calculations->interior_polygon_bounding_trees = bounding_tree_seq_type();
		bounding_tree_seq_type &interior_polygon_bounding_trees =
				d_cached_calculations->interior_polygon_bounding_trees.get();

		interior_polygon_bounding_trees.reserve(num_interior_rings);

		for (unsigned int i = 0; i < num_interior_rings; ++i)
		{
			// Note that we *don't* ask the bounding tree to keep a shared reference to us
			// otherwise we get circular shared pointer references and a memory leak.
			interior_polygon_bounding_trees.push_back(
					boost::shared_ptr<ring_bounding_tree_type>(
							new ring_bounding_tree_type(
									interior_ring_begin(i),
									interior_ring_end(i))));
		}
	}

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			interior_ring_index < num_interior_rings,
			GPLATES_ASSERTION_SOURCE);

	return *d_cached_calculations->interior_polygon_bounding_trees.get()[interior_ring_index];
}


GPlatesScribe::TranscribeResult
GPlatesMaths::PolygonOnSphere::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Transcribe the vertices of each ring instead of segments because the segments (great circle arcs)
	// contain duplicate vertices (end of segment contains same vertex as start of next segment).
	if (scribe.is_saving())
	{
		// Exterior ring.
		const std::vector<PointOnSphere> exterior_ring_vertices_(exterior_ring_vertex_begin(), exterior_ring_vertex_end());
		scribe.save(TRANSCRIBE_SOURCE, exterior_ring_vertices_, "exterior_ring");

		const GPlatesScribe::ObjectTag interior_rings_tag("interior_rings");

		// Number of interior rings.
		const unsigned int num_interior_rings = number_of_interior_rings();
		scribe.save(TRANSCRIBE_SOURCE, num_interior_rings, interior_rings_tag.sequence_size());

		// Interior rings.
		for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; ++interior_ring_index)
		{
			const std::vector<PointOnSphere> interior_vertices_(
					interior_ring_vertex_begin(interior_ring_index),
					interior_ring_vertex_end(interior_ring_index));
			scribe.save(TRANSCRIBE_SOURCE, interior_vertices_, interior_rings_tag[interior_ring_index]);
		}
	}
	else // loading
	{
		// Exterior ring.
		std::vector<PointOnSphere> exterior_ring_vertices_;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, exterior_ring_vertices_, "exterior_ring"))
		{
			return scribe.get_transcribe_result();
		}

		const GPlatesScribe::ObjectTag interior_rings_tag("interior_rings");

		// Number of interior rings.
		unsigned int num_interior_rings;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, num_interior_rings, interior_rings_tag.sequence_size()))
		{
			return scribe.get_transcribe_result();
		}

		if (num_interior_rings > 0)
		{
			// Interior rings.
			std::vector<std::vector<PointOnSphere>> interior_rings;
			interior_rings.resize(num_interior_rings);

			for (unsigned int interior_ring_index = 0; interior_ring_index < num_interior_rings; ++interior_ring_index)
			{
				if (!scribe.transcribe(TRANSCRIBE_SOURCE, interior_rings[interior_ring_index], interior_rings_tag[interior_ring_index]))
				{
					return scribe.get_transcribe_result();
				}
			}

			// Add the exterior and interior rings (as great circle arc segments).
			generate_rings_and_swap(
					*this,
					exterior_ring_vertices_.begin(), exterior_ring_vertices_.end(),
					interior_rings.begin(), interior_rings.end());
		}
		else // num_interior_rings == 0 ...
		{
			// Add the exterior ring only (as great circle arc segments).
			generate_rings_and_swap(
					*this,
					exterior_ring_vertices_.begin(), exterior_ring_vertices_.end());
		}
	}

	// Record base/derived inheritance relationship.
	if (!scribe.transcribe_base<GeometryOnSphere, PolygonOnSphere>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


const GPlatesMaths::GreatCircleArc &
GPlatesMaths::PolygonOnSphere::ConstIterator::dereference() const
{
	GPlatesGlobal::Assert<GPlatesGlobal::UninitialisedIteratorException>(
			d_polygon_ptr,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to dereference an uninitialised iterator.");

	return *d_current_ring_iter;
}


void
GPlatesMaths::PolygonOnSphere::ConstIterator::increment()
{
	if (d_polygon_ptr == NULL)
	{
		// This iterator is uninitialised, so this function will be a no-op.
		return;
	}

	// Make sure caller not attempting to increment beyond last ring.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_ring_iter != d_current_ring_ptr->end(),
			GPLATES_ASSERTION_SOURCE);

	++d_current_ring_iter;

	if (d_current_ring_iter == d_current_ring_ptr->end())
	{
		if (d_current_ring_id < d_polygon_ptr->d_interior_rings.size())
		{
			// Advance to an interior ring (from either the exterior ring or an interior ring).
			++d_current_ring_id;

			// Note: Ring id and interior ring index are offset by one.
			d_current_ring_ptr = &d_polygon_ptr->d_interior_rings[d_current_ring_id - 1];
			d_current_ring_iter = d_current_ring_ptr->begin();
		}
		else
		{
			// We're at end of all rings so just leave current iterator pointing to end
			// of current ring (which is either the exterior ring, or last interior ring if any).
			return;
		}
	}
}


void
GPlatesMaths::PolygonOnSphere::ConstIterator::decrement()
{
	if (d_polygon_ptr == NULL)
	{
		// This iterator is uninitialised, so this function will be a no-op.
		return;
	}

	if (d_current_ring_iter == d_current_ring_ptr->begin())
	{
		// Make sure caller not attempting to decrement prior to first (exterior) ring.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_current_ring_id > 0,
				GPLATES_ASSERTION_SOURCE);

		--d_current_ring_id;

		if (d_current_ring_id == 0)
		{
			// We've moved into the exterior ring (from the first interior ring).
			d_current_ring_ptr = &d_polygon_ptr->d_exterior_ring;
		}
		else
		{
			// Note: Ring id and interior ring index are offset by one.
			d_current_ring_ptr = &d_polygon_ptr->d_interior_rings[d_current_ring_id - 1];
		}

		d_current_ring_iter = d_current_ring_ptr->end();
	}

	--d_current_ring_iter;
}


bool
GPlatesMaths::PolygonOnSphere::ConstIterator::equal(
		const ConstIterator &other) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::UninitialisedIteratorException>(
			d_polygon_ptr && other.d_polygon_ptr,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to compare an uninitialised iterator.");

	return d_current_ring_id == other.d_current_ring_id &&
			d_current_ring_iter == other.d_current_ring_iter;
}


void
GPlatesMaths::PolygonOnSphere::ConstIterator::advance(
		ConstIterator::difference_type n)
{
	if (d_polygon_ptr == NULL)
	{
		// This iterator is uninitialised, so this function will be a no-op.
		return;
	}

	if (n > 0)
	{
		// Advance forward through the rings if necessary.
		while (n >= d_current_ring_ptr->end() - d_current_ring_iter)
		{
			// Advance forward through all remaining elements in the current ring.
			n -= d_current_ring_ptr->end() - d_current_ring_iter;

			if (d_current_ring_id < d_polygon_ptr->d_interior_rings.size())
			{
				// Advance to an interior ring (from either the exterior ring or an interior ring).
				++d_current_ring_id;

				// Note: Ring id and interior ring index are offset by one.
				d_current_ring_ptr = &d_polygon_ptr->d_interior_rings[d_current_ring_id - 1];
				d_current_ring_iter = d_current_ring_ptr->begin();
			}
			else
			{
				// Make sure we've not been asked to advance *past* the end of all rings.
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						n == 0,
						GPLATES_ASSERTION_SOURCE);

				// We're at end of all rings so just leave current iterator pointing to end
				// of current ring (which is either the exterior ring, or last interior ring if any).
				d_current_ring_iter = d_current_ring_ptr->end();
				return;
			}
		}
		
		// The desired iterator is now in the current ring, so advance (forward) within the current ring.
		std::advance(d_current_ring_iter, n);
	}
	else if (n < 0)
	{
		// Advance backward through the rings if necessary.
		while (n < d_current_ring_ptr->begin() - d_current_ring_iter)
		{
			// Advance backward through all remaining elements in the current ring.
			//
			// Note: This might subtract zero if current iterator at beginning of current ring.
			// In this case we will just be advancing (backward) to the previous ring with
			// no change in 'n' until the next look iteration.
			n -= d_current_ring_ptr->begin() - d_current_ring_iter;

			// Make sure we've not been asked to advance *before* the beginning of all rings.
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					d_current_ring_id > 0,
					GPLATES_ASSERTION_SOURCE);

			--d_current_ring_id;

			if (d_current_ring_id == 0)
			{
				// We've moved into the exterior ring (from the first interior ring).
				d_current_ring_ptr = &d_polygon_ptr->d_exterior_ring;
			}
			else
			{
				// Note: Ring id and interior ring index are offset by one.
				d_current_ring_ptr = &d_polygon_ptr->d_interior_rings[d_current_ring_id - 1];
			}

			d_current_ring_iter = d_current_ring_ptr->end();
		}
		
		// The desired iterator is now in the current ring, so advance (backward) within the current ring.
		std::advance(d_current_ring_iter, n);
	}
}


GPlatesMaths::PolygonOnSphere::ConstIterator::difference_type
GPlatesMaths::PolygonOnSphere::ConstIterator::distance_to(
		const ConstIterator &other) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::UninitialisedIteratorException>(
			d_polygon_ptr && other.d_polygon_ptr,
			GPLATES_ASSERTION_SOURCE,
			"Attempted to compare an uninitialised iterator.");

	const int ring_id_difference =
			static_cast<int>(other.d_current_ring_id) - static_cast<int>(d_current_ring_id);

	if (ring_id_difference == 0)
	{
		// Both iterators reference the same ring, so just return the difference.
		return std::distance(d_current_ring_iter, other.d_current_ring_iter);
	}

	difference_type difference = 0;

	// Ring iteration variables for going forward or backward through the rings.
	int ring_id_increment;
	unsigned int num_rings_to_difference;
	if (ring_id_difference > 0)
	{
		ring_id_increment = 1;
		num_rings_to_difference = ring_id_difference;

		// Add in the difference from current iterator to the beginning of the current ring.
		// We use begin of current ring since we'll be adding current ring size to go to next ring.
		difference += std::distance(d_current_ring_iter, d_current_ring_ptr->begin());

		// Add in the difference from beginning of current ring (in 'other' iterator) to
		// current ring iterator (in 'other' iterator).
		// We use begin of ring since we used begin above.
		difference += std::distance(other.d_current_ring_ptr->begin(), other.d_current_ring_iter);
	}
	else // ring_id_difference < 0 ...
	{
		ring_id_increment = -1;
		num_rings_to_difference = -ring_id_difference;

		// Add in the difference from current iterator to the end of the current ring.
		// We use end of current ring since we'll be subtracting current ring size to go to previous ring.
		difference += std::distance(d_current_ring_iter, d_current_ring_ptr->end());

		// Add in the difference from end of current ring (in 'other' iterator) to
		// current ring iterator (in 'other' iterator).
		// We use end of ring since we used end above.
		difference += std::distance(other.d_current_ring_ptr->end(), other.d_current_ring_iter);
	}

	// Advance (forward or backward) through the rings.
	unsigned int ring_id = d_current_ring_id;
	for (unsigned int n = 0; n < num_rings_to_difference; ++n, ring_id += ring_id_increment)
	{
		if (ring_id == 0)
		{
			difference += ring_id_increment * d_polygon_ptr->d_exterior_ring.size();
		}
		else
		{
			// Note: Ring id and interior ring index are offset by one.
			difference += ring_id_increment * d_polygon_ptr->d_interior_rings[ring_id - 1].size();
		}
	}

	return difference;
}


GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
GPlatesMaths::tessellate(
		const PolygonOnSphere &polygon,
		const real_t &max_angular_extent)
{
	// Tessellate the exterior ring.
	std::vector<PointOnSphere> tessellated_exterior_ring;
	tessellate_ring(
			tessellated_exterior_ring,
			polygon.exterior_ring_begin(),
			polygon.exterior_ring_end(),
			max_angular_extent);

	if (polygon.number_of_interior_rings() == 0)
	{
		return PolygonOnSphere::create(tessellated_exterior_ring);
	}

	// Tessellate the interior rings.
	std::vector< std::vector<PointOnSphere> > tessellated_interior_rings;
	tessellated_interior_rings.resize(polygon.number_of_interior_rings());

	unsigned int interior_ring_index = 0;
	PolygonOnSphere::ring_sequence_const_iterator interior_rings_iter = polygon.interior_rings_begin();
	PolygonOnSphere::ring_sequence_const_iterator interior_rings_end = polygon.interior_rings_end();
	for ( ; interior_rings_iter != interior_rings_end; ++interior_rings_iter, ++interior_ring_index)
	{
		tessellate_ring(
				tessellated_interior_rings[interior_ring_index],
				interior_rings_iter->begin(),
				interior_rings_iter->end(),
				max_angular_extent);
	}

	return PolygonOnSphere::create(
			tessellated_exterior_ring,
			tessellated_interior_rings.begin(),
			tessellated_interior_rings.end());
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
