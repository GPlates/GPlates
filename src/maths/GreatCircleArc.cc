/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009, 2010 The University of Sydney, Australia
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

#include <sstream>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>

#include "GreatCircleArc.h"
#include "FiniteRotation.h"
#include "IndeterminateResultException.h"
#include "IndeterminateArcRotationAxisException.h"
#include "PolylineOnSphere.h"
#include "Rotation.h"
#include "Vector3D.h"


namespace GPlatesMaths
{
	namespace
	{
		inline
		UnitVector3D
		calculate_closest_position_on_great_circle(
				const UnitVector3D &test_point,
				const UnitVector3D &great_circle_rotation_axis,
				const real_t &test_point_dot_rotation_axis)
		{
			// The projection of 'test_point' in the direction of 'great_circle_rotation_axis'.
			Vector3D proj = test_point_dot_rotation_axis * great_circle_rotation_axis;

			// The projection of 'test_point' perpendicular to the direction of
			// 'rotation_axis'.
			Vector3D perp = Vector3D(test_point) - proj;

			return perp.get_normalisation();
		}

		inline
		UnitVector3D
		calculate_closest_position_on_great_circle(
				const UnitVector3D &test_point,
				const UnitVector3D &rotation_axis)
		{
			return calculate_closest_position_on_great_circle(
					test_point,
					rotation_axis,
					dot(test_point, rotation_axis));
		}

		/**
		 * Feature type of @a GreatCircleArc.
		 */
		enum GreatCircleArcFeature { GCA_START_POINT, GCA_END_POINT, GCA_ARC };

		/**
		 * Returns feature of @a great_circle_arc that is closest to @a test_point.
		 * The closeness (dot product) of closest point on GCA to @a test_point is returned
		 * in @a closeness.
		 */
		GreatCircleArcFeature
		calculate_closest_feature(
				const GreatCircleArc &great_circle_arc,
				const PointOnSphere &test_point,
				boost::optional<PointOnSphere> &closest_point_on_great_circle_arc,
				real_t &closeness)
		{
			/*
			 * The following acronyms will be used:
			 * - GC = great-circle
			 * - GCA = great-circle-arc
			 */

			// Firstly, let's check whether this arc has a determinate rotation axis.  If it doesn't,
			// then its start-point will be coincident with its end-point, which will mean the arc is
			// point-like, in which case, we can fall back to point comparisons.
			if ( ! great_circle_arc.is_zero_length()) {
				// The arc has a determinate rotation axis.

				// First, a few convenient aliases.
				const UnitVector3D &n = great_circle_arc.rotation_axis();  // The "normal" to the GC.
				const UnitVector3D &t = test_point.position_vector();

				// A few more convenient aliases.
				const UnitVector3D &a = great_circle_arc.start_point().position_vector();
				const UnitVector3D &b = great_circle_arc.end_point().position_vector();

				// The unit-vector of the "closest point" on the GC.
				const UnitVector3D c = calculate_closest_position_on_great_circle(t, n);

				real_t closeness_a_to_b = dot(a, b);
				real_t closeness_c_to_a = dot(c, a);
				real_t closeness_c_to_b = dot(c, b);

				if (closeness_c_to_a.is_precisely_greater_than(closeness_a_to_b.dval()) &&
					closeness_c_to_b.is_precisely_greater_than(closeness_a_to_b.dval())) {

					/*
					 * C is closer to A than B is to A, and also closer to
					 * B than A is to B, so C must lie _between_ A and B,
					 * which means it lies on the GCA.
					 *
					 * Hence, C is the closest point on the GCA to
					 * 'test_point'.
					 */
					closeness = dot(t, c);
					closest_point_on_great_circle_arc = boost::in_place(boost::cref(c));
					return GCA_ARC;

				} else {

					/*
					 * C does not lie between A and B, so either A or B
					 * will be the closest point on the GCA to
					 * 'test_point'.
					 */
					if (closeness_c_to_a.is_precisely_greater_than(closeness_c_to_b.dval())) {

						/*
						 * C is closer to A than it is to B, so by
						 * Pythagoras' Theorem (which still holds
						 * approximately, since we're dealing with a
						 * thin, almost-cylindrical, strip of spherical
						 * surface around the equator) we assert that
						 * 'test_point' must be closer to A than it is
						 * to B.
						 */
						closeness = dot(t, a);
						closest_point_on_great_circle_arc = great_circle_arc.start_point();
						return GCA_START_POINT;

					} else {

						closeness = dot(t, b);
						closest_point_on_great_circle_arc = great_circle_arc.end_point();
						return GCA_END_POINT;
					}
				}
			}

			// The arc is point-like so return the start point.
			closeness = calculate_closeness(test_point, great_circle_arc.start_point());
			closest_point_on_great_circle_arc = great_circle_arc.start_point();
			return GCA_START_POINT;
		}

		/**
		 * Returns the minimum distance of a position to a great circle arc, where the position
		 * is inside the lune of the (non-zero length) great circle arc.
		 *
		 * NOTE: The @a position_vector must not equal the @a arc_plane_normal otherwise
		 * @a calculate_closest_position_on_great_circle will throw an exception when attempting
		 * to normalise a zero length vector.
		 */
		AngularDistance
		minimum_distance_for_position_inside_arc_lune(
				const UnitVector3D &position_vector,
				const UnitVector3D &arc_plane_normal,
				boost::optional<UnitVector3D &> closest_position_on_great_circle_arc,
				boost::optional<const AngularExtent &> minimum_distance_threshold)
		{
			const real_t position_dot_arc_normal = dot(position_vector, arc_plane_normal);

			// If there's a threshold and it's less than 90 degrees then we can quickly
			// reject positions with a single dot product before we do more expensive calculations.
			if (minimum_distance_threshold
				&& is_strictly_positive(minimum_distance_threshold->get_cosine()))
			{
				// Instead of testing closeness to the GCA we test closeness to the GCA poles.
				const real_t closeness_to_gca_poles = abs(position_dot_arc_normal);

				// If close enough to the GCA poles then it means we've exceeded threshold distance
				// to the GCA itself so return the maximum possible distance (PI) to signal this.
				if (closeness_to_gca_poles.is_precisely_greater_than(minimum_distance_threshold->get_sine().dval()))
				{
					return AngularDistance::PI;
				}
			}
			// Else either there's no threshold or the threshold is greater than 90 degrees.
			// In the latter case the position will always be less than the threshold distance from
			// the GCA because it's within the GCA's lune and all positions within the lune are
			// within 90 degrees of the GCA. So we don't need to test the threshold again.

			// Set caller's closest position *after* passing threshold test (if any).
			if (closest_position_on_great_circle_arc)
			{
				// Set the caller's closest point.
				closest_position_on_great_circle_arc.get() =
						calculate_closest_position_on_great_circle(
								position_vector,
								arc_plane_normal,
								position_dot_arc_normal);

				return AngularDistance::create_from_cosine(
						dot(position_vector, closest_position_on_great_circle_arc.get()));
			}
			// We don't need to calculate the closed point...

			// It's cheaper to calculate sine of minimum angular distance and then convert to cosine.
			// This still requires a 'sqrt', but it's better than calculating cosine as:
			//
			// dot(
			//     position_vector,
			//     calculate_closest_position_on_great_circle(
			//           position_vector,
			//           arc_plane_normal))
			//
			// ...which requires two dot products, a sqrt and a division.

			const real_t &sine_min_angular_distance = position_dot_arc_normal;
			const real_t cosine_min_angular_distance =
					sqrt(1 - sine_min_angular_distance * sine_min_angular_distance);

			return AngularDistance::create_from_cosine(cosine_min_angular_distance);
		}

		/**
		 * Returns the minimum distance of a position to a great circle arc, where the position
		 * is outside the lune of the (non-zero length) great circle arc.
		 */
		AngularDistance
		minimum_distance_for_position_outside_arc_lune(
				const UnitVector3D &position_vector,
				const UnitVector3D &arc_start_position,
				const UnitVector3D &arc_end_position,
				boost::optional<UnitVector3D &> closest_position_on_great_circle_arc,
				boost::optional<const AngularExtent &> minimum_distance_threshold)
		{
			// Position is outside lune of great circle arc so one of the arc end points must be closest.
			const AngularDistance distance_to_arc_start =
					AngularDistance::create_from_cosine(dot(arc_start_position, position_vector));
			const AngularDistance distance_to_arc_end =
					AngularDistance::create_from_cosine(dot(arc_end_position, position_vector));

			if (distance_to_arc_start.is_precisely_less_than(distance_to_arc_end))
			{
				// If there's a threshold and the minimum distance is greater than the threshold then
				// return the maximum possible distance (PI) to signal this.
				if (minimum_distance_threshold &&
					distance_to_arc_start.is_precisely_greater_than(minimum_distance_threshold.get()))
				{
					return AngularDistance::PI;
				}

				// Set caller's closest position *after* passing threshold test (if any).
				if (closest_position_on_great_circle_arc)
				{
					closest_position_on_great_circle_arc.get() = arc_start_position;
				}

				return distance_to_arc_start;
			}

			// If there's a threshold and the minimum distance is greater than the threshold then
			// return the maximum possible distance (PI) to signal this.
			if (minimum_distance_threshold &&
				distance_to_arc_end.is_precisely_greater_than(minimum_distance_threshold.get()))
			{
				return AngularDistance::PI;
			}

			// Set caller's closest position *after* passing threshold test (if any).
			if (closest_position_on_great_circle_arc)
			{
				closest_position_on_great_circle_arc.get() = arc_end_position;
			}

			return distance_to_arc_end;
		}
	}
}


const GPlatesMaths::GreatCircleArc
GPlatesMaths::GreatCircleArc::create(
		const PointOnSphere &p1,
		const PointOnSphere &p2,
		bool check_validity)
{
	const UnitVector3D &u1 = p1.position_vector();
	const UnitVector3D &u2 = p2.position_vector();

	const real_t dot_p1_p2 = dot(u1, u2);

	if (check_validity)
	{
		const ConstructionParameterValidity cpv = evaluate_construction_parameter_validity(u1, u2, dot_p1_p2);

		/*
		 * First, we ensure that these two endpoints do in fact define a single
		 * unique great-circle arc.
		 */
		if (cpv == INVALID_ANTIPODAL_ENDPOINTS)
		{

			// start-point and end-point antipodal => indeterminate arc
			std::ostringstream oss;

			oss
			 << "Attempted to calculate a great-circle arc from "
			 << "antipodal endpoints "
			 << p1
			 << " and "
			 << p2
			 << ".";
			throw IndeterminateResultException(GPLATES_EXCEPTION_SOURCE,
					oss.str().c_str());
		}
	}

	return GreatCircleArc(p1, p2, dot_p1_p2);
}


const GPlatesMaths::GreatCircleArc
GPlatesMaths::GreatCircleArc::create_rotated_arc(
		const FiniteRotation &rot,
		const GreatCircleArc &arc)
{
	const PointOnSphere rot_start = rot * arc.start_point();
	const PointOnSphere rot_end   = rot * arc.end_point();

	// Create the GCA without a cached rotation axis.
	GreatCircleArc gca(rot_start, rot_end, arc.dot_of_endpoints());

	if (arc.is_zero_length())
	{
		// The arc is pointlike, so there is no determinate rotation axis.

		// NOTE: Boost 1.34 does not support nullary in-place (ie, "boost::in_place()") so we
		// use copy-assignment instead.
		gca.d_rotation_info = RotationInfo();
	}
	else
	{
		// There is a determinate rotation axis.
		const UnitVector3D rot_axis = rot * arc.rotation_axis();

		// Set the cached rotation axis.
		gca.d_rotation_info = boost::in_place(boost::cref(rot_axis));
	}

	return gca;
}


const GPlatesMaths::GreatCircleArc
GPlatesMaths::GreatCircleArc::create_antipodal_arc(
		const GreatCircleArc &arc)
{
	GreatCircleArc antipodal_arc(
			PointOnSphere(-arc.d_start_point.position_vector()),
			PointOnSphere(-arc.d_end_point.position_vector()),
			arc.d_dot_of_endpoints);

	// Copy the rotation information (if it exists) because it's the same so there's
	// no point having to re-calculate it later (if/when needed).
	antipodal_arc.d_rotation_info = arc.d_rotation_info;

	return antipodal_arc;
}


bool
GPlatesMaths::GreatCircleArc::is_zero_length() const
{
	if (!d_rotation_info)
	{
		calculate_rotation_info();
	}

	return d_rotation_info->d_is_zero_length;
}


const GPlatesMaths::UnitVector3D &
GPlatesMaths::GreatCircleArc::rotation_axis() const
{
	if (is_zero_length())
	{
		throw IndeterminateArcRotationAxisException(GPLATES_EXCEPTION_SOURCE, *this);
	}

	if (!d_rotation_info)
	{
		calculate_rotation_info();
	}

	return d_rotation_info->d_rot_axis;
}


boost::optional<GPlatesMaths::PointOnSphere>
GPlatesMaths::GreatCircleArc::is_close_to(
		const PointOnSphere &test_point,
		const AngularExtent &closeness_angular_extent_threshold,
		real_t &closeness) const
{
	/*
	 * The following acronyms will be used:
	 * - GC = great-circle
	 * - GCA = great-circle-arc
	 */

	// Firstly, let's check whether this arc has a determinate rotation axis.  If it doesn't,
	// then its start-point will be coincident with its end-point, which will mean the arc is
	// point-like, in which case, we can fall back to point comparisons.
	if ( ! is_zero_length()) {
		// The arc has a determinate rotation axis.

		if( is_strictly_positive(closeness_angular_extent_threshold.get_cosine()) )
		{
			real_t closeness_to_poles = abs(dot(test_point.position_vector(), rotation_axis()));

			// This first if-statement should weed out most of the no-hopers.
			//
			// This is designed to enable a quick elimination of "no-hopers" (test-points
			// which can easily be determined to have no chance of being
			// "close"), leaving only plausible test-points to proceed to
			// the more expensive proximity tests.  If you imagine a
			// line-segment of this polyline as an arc along the equator,
			// then there will be a threshold latitude above and below the
			// equator, beyond which there is no chance of a test-point
			// being "close" to that segment.
			if (closeness_to_poles.is_precisely_greater_than(closeness_angular_extent_threshold.get_sine().dval())) 
			{

				/*
				 * 'test_point' lies within latitudes sufficiently far above or
				 * below the GC that there is no chance it could be "close to"
				 * this GCA.
				 */
				return boost::none;
			}
		}
		
		// Get the closest feature of this GCA to 'test_point'.
		boost::optional<PointOnSphere> closest_point_on_great_circle_arc;
		calculate_closest_feature(
				*this, test_point, closest_point_on_great_circle_arc, closeness);
		
		if (!closeness.is_precisely_greater_than(closeness_angular_extent_threshold.get_cosine().dval()))
		{
			return boost::none;
		}

		return closest_point_on_great_circle_arc;
	}

	// The arc is point-like.
	if (!start_point().is_close_to(test_point, closeness_angular_extent_threshold.get_cosine(), closeness))
	{
		return boost::none;
	}

	return start_point();
}

GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
GPlatesMaths::GreatCircleArc::get_closest_point(
		const PointOnSphere &test_point) const
{
	// Get the closest feature of this GCA to 'test_point'.
	real_t closeness;
	boost::optional<PointOnSphere> closest_point_on_great_circle_arc;
	calculate_closest_feature(*this, test_point, closest_point_on_great_circle_arc, closeness);

	return closest_point_on_great_circle_arc->get_non_null_pointer();
}

GPlatesMaths::GreatCircleArc::ConstructionParameterValidity
GPlatesMaths::GreatCircleArc::evaluate_construction_parameter_validity(
		const UnitVector3D &p1,
		const UnitVector3D &p2,
		const real_t &dot_p1_p2)
{
#if 0  // Identical endpoints are now valid.
	if (dp >= 1.0) {

		// parallel => start-point same as end-point => no arc
		return INVALID_IDENTICAL_ENDPOINTS;
	}
#endif

	if (dot_p1_p2 <= -1.0)
	{

		// antiparallel => start-point and end-point antipodal =>
		// indeterminate arc
		return INVALID_ANTIPODAL_ENDPOINTS;
	}

	return VALID;
}


void
GPlatesMaths::GreatCircleArc::calculate_rotation_info() const
{
	/*
	 * Now we want to calculate the unit vector normal to the plane
	 * of rotation (this vector also known as the "rotation axis").
	 *
	 * To do this, we calculate the cross product.
	 */
	const Vector3D v = cross(d_start_point.position_vector(), d_end_point.position_vector());

	/*
	 * Since start/end points are unit vectors which might be parallel (but won't be antiparallel),
	 * the magnitude of v will be in the range [0, 1], and will be equal to the sine of the
	 * smaller of the two angles between p1 and p2.
	 */
	if (v.magSqrd() <= 0.0)
	{
		// The points are coincident, which means there is no determinate rotation axis.
		// NOTE: Boost 1.34 does not support nullary in-place (ie, "boost::in_place()") so we
		// use copy-assignment instead.
		d_rotation_info = RotationInfo();
	}
	else
	{
		// The points are non-coincident, which means there is a determinate rotation axis.
		const UnitVector3D rot_axis = v.get_normalisation();
		d_rotation_info = boost::in_place(boost::cref(rot_axis));
	}
}


void
GPlatesMaths::tessellate(
		std::vector<PointOnSphere> &tessellation_points,
		const GreatCircleArc &great_circle_arc,
		const real_t &max_segment_angular_extent)
{
	const PointOnSphere &start_point = great_circle_arc.start_point();
	const PointOnSphere &end_point = great_circle_arc.end_point();

	// If there's no rotation axis...
	if (great_circle_arc.is_zero_length())
	{
		tessellation_points.push_back(start_point);
		tessellation_points.push_back(end_point);
	}

	// The angular extent of the great circle arc being subdivided.
	const double gca_angular_extent = acos(great_circle_arc.dot_of_endpoints()).dval();

	//
	// Note: Using static_cast<int> instead of static_cast<unsigned int> since
	// Visual Studio optimises for 'int' and not 'unsigned int'.
	//
	// The '+1' is to round up instead of down.
	// It also means we don't need to test for the case of only one segment.
	const int num_segments = 1 + static_cast<int>(gca_angular_extent / max_segment_angular_extent.dval());
	const double segment_angular_extent = gca_angular_extent / num_segments;

	// Create the rotation to generate segment points.
	const Rotation segment_rotation =
			Rotation::create(great_circle_arc.rotation_axis(), segment_angular_extent);

	// Generate the segment points.
	const int num_initial_tessellation_points = tessellation_points.size();
	tessellation_points.reserve(num_initial_tessellation_points + num_segments + 1);
	tessellation_points.push_back(start_point);
	for (int n = 0; n < num_segments - 1; ++n)
	{
		const PointOnSphere &segment_start_point = tessellation_points[num_initial_tessellation_points + n];
		const PointOnSphere segment_end_point(segment_rotation * segment_start_point.position_vector());

		tessellation_points.push_back(segment_end_point);
	}

	// The final point added is the original arc's end point.
	// This avoids numerical error in the final point due to accumulated rotations.
	tessellation_points.push_back(end_point);
}

bool
GPlatesMaths::arcs_are_near_each_other(
		const GreatCircleArc &arc1,
		const GreatCircleArc &arc2)
{
	real_t arc1_start_dot_arc2_start =
			dot(arc1.start_point().position_vector(), arc2.start_point().position_vector());
	real_t arc1_start_dot_arc2_end =
			dot(arc1.start_point().position_vector(), arc2.end_point().position_vector());
	real_t arc1_end_dot_arc2_start =
			dot(arc1.end_point().position_vector(), arc2.start_point().position_vector());

	/*
	 * arc1 and arc2 are "near" each other if one of the following is true:
	 *  - arc2.start is closer to arc1.start than arc1.end is;
	 *  - arc2.end is closer to arc1.start than arc1.end is;
	 *  - arc1.start is closer to arc2.start than arc2.end is;
	 *  - arc1.end is closer to arc2.start than arc2.end is.
	 */
	return
	 (arc1_start_dot_arc2_start >= arc1.dot_of_endpoints() ||
	  arc1_start_dot_arc2_end >= arc1.dot_of_endpoints() ||
	  arc1_start_dot_arc2_start >= arc2.dot_of_endpoints() ||
	  arc1_end_dot_arc2_start >= arc2.dot_of_endpoints());
}


bool
GPlatesMaths::intersect(
		const GreatCircleArc &arc1,
		const GreatCircleArc &arc2,
		boost::optional<UnitVector3D &> intersection)
{
	// Test most common case first (both arcs are not zero length).
	if (!arc1.is_zero_length() && !arc2.is_zero_length())
	{
		// Both arcs are not zero length and hence have rotation axes...

		// Two arcs intersect if the end points of one arc are in opposite half-spaces of the plane of
		// the other arc (and vice versa) and the start (or end) point of one arc is in the positive
		// half-space of the other arc (and the opposite true for the other arc).

		const bool arc1_start_point_on_positive_side_of_arc2 = dot(
				arc1.start_point().position_vector(),
				arc2.rotation_axis()).dval() >= 0;
		const bool arc1_end_point_on_positive_side_of_arc2 = dot(
				arc1.end_point().position_vector(),
				arc2.rotation_axis()).dval() >= 0;
		if (arc1_start_point_on_positive_side_of_arc2 == arc1_end_point_on_positive_side_of_arc2)
		{
			// No intersection found.
			return false;
		}

		const bool arc2_start_point_on_positive_side_of_arc1 = dot(
				arc2.start_point().position_vector(),
				arc1.rotation_axis()).dval() >= 0;
		const bool arc2_end_point_on_positive_side_of_arc1 = dot(
				arc2.end_point().position_vector(),
				arc1.rotation_axis()).dval() >= 0;
		if (arc2_start_point_on_positive_side_of_arc1 == arc2_end_point_on_positive_side_of_arc1)
		{
			// No intersection found.
			return false;
		}

		if (arc1_start_point_on_positive_side_of_arc2 == arc2_start_point_on_positive_side_of_arc1)
		{
			// No intersection found.
			return false;
		}

		// If caller requested the intersection position.
		if (intersection)
		{
			const Vector3D cross_arc_rotation_axes = cross(arc1.rotation_axis(), arc2.rotation_axis());

			// If both arcs are *not* on the same great circle - this is the most common case.
			if (cross_arc_rotation_axes.magSqrd() > 0)
			{
				const UnitVector3D normalised_cross_arc_rotation_axes = cross_arc_rotation_axes.get_normalisation();

				// We must choose between the two possible antipodal cross product directions based
				// on the orientation of the arcs relative to each other.
				intersection.get() = arc1_start_point_on_positive_side_of_arc2
						? normalised_cross_arc_rotation_axes
						: -normalised_cross_arc_rotation_axes;
			}
			else
			{
				// Both arcs are on the same great circle since have same (or opposite) rotation axis...

				// See if arc1's start point is on arc2...
				if (dot(arc2.start_point().position_vector(), arc1.start_point().position_vector())
						.is_precisely_greater_than(arc2.dot_of_endpoints().dval()) &&
					dot(arc2.end_point().position_vector(), arc1.start_point().position_vector())
						.is_precisely_greater_than(arc2.dot_of_endpoints().dval()))
				{
					intersection.get() = arc1.start_point().position_vector();
				}
				// See if arc1's end point is on arc2...
				else if (dot(arc2.start_point().position_vector(), arc1.end_point().position_vector())
						.is_precisely_greater_than(arc2.dot_of_endpoints().dval()) &&
					dot(arc2.end_point().position_vector(), arc1.end_point().position_vector())
						.is_precisely_greater_than(arc2.dot_of_endpoints().dval()))
				{
					intersection.get() = arc1.end_point().position_vector();
				}
				// See if arc2's start point is on arc1...
				else if (dot(arc1.start_point().position_vector(), arc2.start_point().position_vector())
						.is_precisely_greater_than(arc1.dot_of_endpoints().dval()) &&
					dot(arc1.end_point().position_vector(), arc2.start_point().position_vector())
						.is_precisely_greater_than(arc1.dot_of_endpoints().dval()))
				{
					intersection.get() = arc2.start_point().position_vector();
				}
				else
				{
					// If we get here then arc2's end point must be on arc1.
					intersection.get() = arc2.end_point().position_vector();
				}
			}
		}

		return true;
	}

	// If both arcs are zero length...
	if (arc1.is_zero_length() && arc2.is_zero_length())
	{
		if (arc1.start_point() == arc2.start_point())
		{
			// If caller requested the intersection position.
			if (intersection)
			{
				intersection.get() = arc1.start_point().position_vector();
			}
			return true;
		}

		// No intersection found.
		return false;
	}

	// If only arc1 is zero length...
	if (arc1.is_zero_length())
	{
		if (arc1.start_point().lies_on_gca(arc2))
		{
			// If caller requested the intersection position.
			if (intersection)
			{
				intersection.get() = arc1.start_point().position_vector();
			}
			return true;
		}

		// No intersection found.
		return false;
	}

	// else only arc2 is zero length...
	if (arc2.start_point().lies_on_gca(arc1))
	{
		// If caller requested the intersection position.
		if (intersection)
		{
			intersection.get() = arc2.start_point().position_vector();
		}
		return true;
	}

	// No intersection found.
	return false;
}


GPlatesMaths::AngularDistance
GPlatesMaths::minimum_distance(
		const GreatCircleArc &arc1,
		const GreatCircleArc &arc2,
		boost::optional<const AngularExtent &> minimum_distance_threshold,
		boost::optional< boost::tuple<UnitVector3D &/*arc1*/, UnitVector3D &/*arc2*/> > closest_positions_on_arcs)
{
	//
	// First see if the arcs intersect each other.
	//

	// If the caller has requested the closest points on the arcs then this
	// will be the intersection point (if any).
	boost::optional<UnitVector3D &> intersection;
	if (closest_positions_on_arcs)
	{
		// Intersection references caller's closest point on arc1.
		UnitVector3D &closest_position_on_arc1 = boost::get<0>(closest_positions_on_arcs.get());
		// Note that this is referencing (there is no copying of UnitVector3D here).
		intersection = closest_position_on_arc1;
	}

	if (intersect(arc1, arc2, intersection))
	{
		// The closest point on each arc is the same point - the intersection point.
		if (intersection)
		{
			// 'intersect()' has already assigned the intersection to the closest point on arc1.
			// So copy the intersection to the closest point on arc2.
			UnitVector3D &closest_position_on_arc2 = boost::get<1>(closest_positions_on_arcs.get());
			// Note that this assigns/copies a UnitVector3D through a reference.
			closest_position_on_arc2 = intersection.get();
		}

		return AngularDistance::ZERO;
	}

	//
	// Find the distance of each end point of one arc to the other arc (and vice versa).
	// We then take the minimum distance of these four calculations.
	//
	// Note that if either (or both) arcs are zero length then we're duplicating a little bit of
	// work but zero length arcs should be very rare anyway.
	//

	boost::optional<UnitVector3D &> closest_position_on_arc1;
	boost::optional<UnitVector3D &> closest_position_on_arc2;
	if (closest_positions_on_arcs)
	{
		closest_position_on_arc1 = boost::get<0>(closest_positions_on_arcs.get());
		closest_position_on_arc2 = boost::get<1>(closest_positions_on_arcs.get());
	}

	// Note that after each (point-to-arc minimum distance) calculation we update the threshold
	// with the updated minimum distance.
	//
	// This avoids overwriting the closest point on an arc (so far) with a point that is
	// further away. For example, if the closest point on arc2 was requested by the caller and
	// arc1's end point (second calculated) is further away from arc2 than arc1's start point
	// (first calculation) then, without using the distance from arc1's start point to arc2
	// (the current minimum distance) as a threshold, the closest point would be overridden.
	//
	// This is also an optimisation that can avoid calculating the closest point on an arc
	// (if the caller has requested the closest points) in some situations where the next
	// point-to-arc distance is greater than the current minimum distance.
	AngularExtent min_point_to_arc_distance_threshold = minimum_distance_threshold
			? minimum_distance_threshold.get()
			: AngularExtent::PI;

	// The (maximum possible) distance to return if shortest distance between both arcs
	// is not within the minimum distance threshold (if any).
	AngularDistance min_point_to_arc_distance = AngularDistance::PI;

	// Calculate minimum distance from arc1's start point to arc2.
	const AngularDistance min_distance_arc1_start_point_to_arc2 =
			minimum_distance(
					arc1.start_point().position_vector(),
					arc2,
					min_point_to_arc_distance_threshold,
					closest_position_on_arc2);
	// If shortest distance so far (within threshold)...
	if (min_distance_arc1_start_point_to_arc2.is_precisely_less_than(min_point_to_arc_distance))
	{
		min_point_to_arc_distance = min_distance_arc1_start_point_to_arc2;
		min_point_to_arc_distance_threshold = AngularExtent(min_point_to_arc_distance);

		if (closest_position_on_arc1)
		{
			closest_position_on_arc1.get() = arc1.start_point().position_vector();
		}
	}

	// Calculate minimum distance from arc1's end point to arc2.
	const AngularDistance min_distance_arc1_end_point_to_arc2 =
			minimum_distance(
					arc1.end_point().position_vector(),
					arc2,
					min_point_to_arc_distance_threshold,
					closest_position_on_arc2);
	// If shortest distance so far (within threshold)...
	if (min_distance_arc1_end_point_to_arc2.is_precisely_less_than(min_point_to_arc_distance))
	{
		min_point_to_arc_distance = min_distance_arc1_end_point_to_arc2;
		min_point_to_arc_distance_threshold = AngularExtent(min_point_to_arc_distance);

		if (closest_position_on_arc1)
		{
			closest_position_on_arc1.get() = arc1.end_point().position_vector();
		}
	}

	// Calculate minimum distance from arc2's start point to arc1.
	const AngularDistance min_distance_arc2_start_point_to_arc1 =
			minimum_distance(
					arc2.start_point().position_vector(),
					arc1,
					min_point_to_arc_distance_threshold,
					closest_position_on_arc1);
	// If shortest distance so far (within threshold)...
	if (min_distance_arc2_start_point_to_arc1.is_precisely_less_than(min_point_to_arc_distance))
	{
		min_point_to_arc_distance = min_distance_arc2_start_point_to_arc1;
		min_point_to_arc_distance_threshold = AngularExtent(min_point_to_arc_distance);

		if (closest_position_on_arc2)
		{
			closest_position_on_arc2.get() = arc2.start_point().position_vector();
		}
	}

	// Calculate minimum distance from arc2's end point to arc1.
	const AngularDistance min_distance_arc2_end_point_to_arc1 =
			minimum_distance(
					arc2.end_point().position_vector(),
					arc1,
					min_point_to_arc_distance_threshold,
					closest_position_on_arc1);
	// If shortest distance so far (within threshold)...
	if (min_distance_arc2_end_point_to_arc1.is_precisely_less_than(min_point_to_arc_distance))
	{
		min_point_to_arc_distance = min_distance_arc2_end_point_to_arc1;
		min_point_to_arc_distance_threshold = AngularExtent(min_point_to_arc_distance);

		if (closest_position_on_arc2)
		{
			closest_position_on_arc2.get() = arc2.end_point().position_vector();
		}
	}

	return min_point_to_arc_distance;
}


GPlatesMaths::AngularDistance
GPlatesMaths::minimum_distance(
		const UnitVector3D &position_vector,
		const GreatCircleArc &arc,
		boost::optional<const AngularExtent &> minimum_distance_threshold,
		boost::optional<UnitVector3D &> closest_position_on_great_circle_arc)
{
	if (arc.is_zero_length())
	{
		// Arc start and end points are the same - arbitrarily pick either as the closest point.

		const AngularDistance min_distance =
				AngularDistance::create_from_cosine(dot(position_vector, arc.start_point().position_vector()));

		// If there's a threshold and the minimum distance is greater than the threshold then
		// return the maximum possible distance (PI) to signal this.
		if (minimum_distance_threshold &&
			min_distance.is_precisely_greater_than(minimum_distance_threshold.get()))
		{
			return AngularDistance::PI;
		}

		// Set caller's closest position *after* passing threshold test (if any).
		if (closest_position_on_great_circle_arc)
		{
			closest_position_on_great_circle_arc.get() = arc.start_point().position_vector();
		}

		return min_distance;
	}

	// Great circle arc is not zero length and hence has a rotation axis...
	const UnitVector3D &arc_plane_normal = arc.rotation_axis();

	const UnitVector3D &arc_start_position = arc.start_point().position_vector();
	const UnitVector3D &arc_end_position = arc.end_point().position_vector();

	// See if the point lies within the lune of the great circle arc - see the Masters Thesis
	// "Speeding up the computation of similarity measures based on Minkowski addition in 3D".
	//
	// This happens if its endpoints are on opposite sides of the dividing plane *and*
	// the edge start point is on the positive side of the dividing plane.
	//
	// Note that we cannot call 'minimum_distance_for_position_inside_arc_lune()' when
	// 'position_vector' equals 'arc_plane_normal' (see its comment) which is when
	// 'position_cross_arc_plane_normal' is zero length - which causes both dot products to be zero.
	// So we use the epsilon testing of 'real_t' (returned by dot product) to avoid this.
	const Vector3D position_cross_arc_plane_normal = cross(position_vector, arc_plane_normal);
	if (dot(position_cross_arc_plane_normal, arc_start_position) > 0 &&
		dot(position_cross_arc_plane_normal, arc_end_position) < 0)
	{
		return minimum_distance_for_position_inside_arc_lune(
				position_vector,
				arc_plane_normal,
				closest_position_on_great_circle_arc,
				minimum_distance_threshold);
	}

	return minimum_distance_for_position_outside_arc_lune(
			position_vector,
			arc_start_position,
			arc_end_position,
			closest_position_on_great_circle_arc,
			minimum_distance_threshold);
}


GPlatesMaths::AngularDistance
GPlatesMaths::maximum_distance(
		const GreatCircleArc &arc1,
		const GreatCircleArc &arc2,
		boost::optional<const AngularExtent &> maximum_distance_threshold,
		boost::optional< boost::tuple<UnitVector3D &/*arc1*/, UnitVector3D &/*arc2*/> > furthest_positions_on_arcs)
{
	// The maximum distance between two great circle arcs is equal to PI minus the minimum distance
	// between one great circle arc and the antipodal of the other great circle arc - see the Masters Thesis
	// "Speeding up the computation of similarity measures based on Minkowski addition in 3D".

	// Convert maximum distance threshold to a minimum distance threshold.
	// Instead of excluding distances below a maximum we exclude distances above a minimum.
	boost::optional<AngularExtent> minimum_distance_threshold_storage;
	boost::optional<const AngularExtent &> minimum_distance_threshold;
	if (maximum_distance_threshold)
	{
		minimum_distance_threshold_storage = AngularExtent::PI - maximum_distance_threshold.get();
		minimum_distance_threshold = minimum_distance_threshold_storage;
	}

	const AngularDistance min_distance =
			minimum_distance(
					arc1,
					GreatCircleArc::create_antipodal_arc(arc2),
					minimum_distance_threshold,
					furthest_positions_on_arcs);

	if (furthest_positions_on_arcs)
	{
		UnitVector3D &furthest_position_on_arc2 = boost::get<1>(furthest_positions_on_arcs.get());
		// We need to reverse the effects of taking the antipodal of arc2.
		furthest_position_on_arc2 = -furthest_position_on_arc2;
	}

	// Convert from minimum distance to maximum distance.
	return (AngularExtent::PI - min_distance).get_angular_distance();
}


GPlatesMaths::AngularDistance
GPlatesMaths::maximum_distance(
		const UnitVector3D &position_vector,
		const GreatCircleArc &arc,
		boost::optional<const AngularExtent &> maximum_distance_threshold,
		boost::optional<UnitVector3D &> furthest_position_on_great_circle_arc)
{
	// The maximum distance of a point to a great circle arc is equal to PI minus the minimum
	// distance to the antipodal great circle arc - see the Masters Thesis
	// "Speeding up the computation of similarity measures based on Minkowski addition in 3D".
	//
	// Instead of taking the antipodal great circle arc we can take the antipodal of the point
	// and get the same results and this also means we don't have to convert the closest position
	// on antipodal arc to furthest position on original arc (by taking antipodal of result).

	// Convert maximum distance threshold to a minimum distance threshold.
	// Instead of excluding distances below a maximum we exclude distances above a minimum.
	boost::optional<AngularExtent> minimum_distance_threshold_storage;
	boost::optional<const AngularExtent &> minimum_distance_threshold;
	if (maximum_distance_threshold)
	{
		minimum_distance_threshold_storage = AngularExtent::PI - maximum_distance_threshold.get();
		minimum_distance_threshold = minimum_distance_threshold_storage;
	}

	const AngularDistance min_distance =
			minimum_distance(
					-position_vector,
					arc,
					minimum_distance_threshold,
					// Note that furthest position on arc from original point is same as
					// closest position on arc to antipodal point...
					furthest_position_on_great_circle_arc);

	// Convert from minimum distance to maximum distance.
	return (AngularExtent::PI - min_distance).get_angular_distance();
}


bool
GPlatesMaths::arcs_lie_on_same_great_circle(
		const GreatCircleArc &arc1,
		const GreatCircleArc &arc2)
{
	if (( ! arc1.is_zero_length()) && ( ! arc2.is_zero_length())) {
		// Each arc has a determinate rotation axis, so we can check whether their rotation
		// axes are collinear.
		return collinear(arc1.rotation_axis(), arc2.rotation_axis());
	} else if (arc1.is_zero_length() && arc2.is_zero_length()) {
		// OK, so neither arc has a determinate rotation axis, which means that they are
		// both of zero length (ie, they are both point-like), which means that they must
		// trivially lie on the same great circle.  This is not a very interesting result,
		// but we'll handle it for the sake of completeness.
		return true;
	} else if (arc2.is_zero_length()) {
		// arc2 is point-like, while arc1 is *not* point-like.  Hence, they will lie on the
		// same great-circle if the unit-vector of the arc2 start-point is perpendicular to
		// the rotation axis of arc1.
		return (perpendicular(arc2.start_point().position_vector(), arc1.rotation_axis()));
	} else {
		// Else, arc1 is point-like, while arc2 is *not* point-like.  Hence, they will lie
		// on the same great-circle if the unit-vector of the arc1 start-point is
		// perpendicular to the rotation axis of arc2.
		return (perpendicular(arc1.start_point().position_vector(), arc2.rotation_axis()));
	}
}


bool
GPlatesMaths::arcs_are_directed_equivalent(
		const GreatCircleArc &arc1,
		const GreatCircleArc &arc2)
{
	const PointOnSphere &arc1_start = arc1.start_point(),
			&arc1_end = arc1.end_point(),
			&arc2_start = arc2.start_point(),
			&arc2_end = arc2.end_point();

	return (points_are_coincident(arc1_start, arc2_start) &&
			points_are_coincident(arc1_end, arc2_end));
}


bool
GPlatesMaths::arcs_are_undirected_equivalent(
		const GreatCircleArc &arc1,
		const GreatCircleArc &arc2)
{
	if ( ! arcs_lie_on_same_great_circle(arc1, arc2)) {

		// There is no way the arcs can be equivalent.
		return false;
	}

	const PointOnSphere &arc1_start = arc1.start_point(),
			&arc1_end = arc1.end_point(),
			&arc2_start = arc2.start_point(),
			&arc2_end = arc2.end_point();

	return ((points_are_coincident(arc1_start, arc2_start) &&
		 points_are_coincident(arc1_end, arc2_end)) ||
		(points_are_coincident(arc1_start, arc2_end) &&
		 points_are_coincident(arc1_end, arc2_start)));
}


bool
GPlatesMaths::GreatCircleArc::operator==(
		const GreatCircleArc &other) const
{
	// Note that we don't need to check for the derived data members since they are
	// uniquely determined by the start and end points (note that an exception is thrown when
	// trying to create an arc with antipodal points).
	return d_start_point == other.d_start_point &&
		d_end_point == other.d_end_point;
}
