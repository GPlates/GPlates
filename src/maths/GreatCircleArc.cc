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


namespace
{
	inline
	GPlatesMaths::UnitVector3D
	calculate_u_of_closest(
			const GPlatesMaths::UnitVector3D &u_test_point,
			const GPlatesMaths::UnitVector3D &rotation_axis)
	{
		// The projection of 'u_test_point' in the direction of 'rotation_axis'.
		GPlatesMaths::Vector3D proj = dot(u_test_point, rotation_axis) * rotation_axis;

		// The projection of 'u_test_point' perpendicular to the direction of
		// 'rotation_axis'.
		GPlatesMaths::Vector3D perp = GPlatesMaths::Vector3D(u_test_point) - proj;

		return perp.get_normalisation();
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
			const GPlatesMaths::GreatCircleArc &great_circle_arc,
			const GPlatesMaths::PointOnSphere &test_point,
			boost::optional<GPlatesMaths::PointOnSphere> &closest_point_on_great_circle_arc,
			GPlatesMaths::real_t &closeness)
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
			const GPlatesMaths::UnitVector3D &n = great_circle_arc.rotation_axis();  // The "normal" to the GC.
			const GPlatesMaths::UnitVector3D &t = test_point.position_vector();

			// A few more convenient aliases.
			const GPlatesMaths::UnitVector3D &a = great_circle_arc.start_point().position_vector();
			const GPlatesMaths::UnitVector3D &b = great_circle_arc.end_point().position_vector();

			// The unit-vector of the "closest point" on the GC.
			const GPlatesMaths::UnitVector3D c = calculate_u_of_closest(t, n);

			GPlatesMaths::real_t closeness_a_to_b = dot(a, b);
			GPlatesMaths::real_t closeness_c_to_a = dot(c, a);
			GPlatesMaths::real_t closeness_c_to_b = dot(c, b);

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
}


const GPlatesMaths::GreatCircleArc
GPlatesMaths::GreatCircleArc::create(
		const PointOnSphere &p1,
		const PointOnSphere &p2)
{
	const UnitVector3D &u1 = p1.position_vector();
	const UnitVector3D &u2 = p2.position_vector();

	const real_t dot_p1_p2 = dot(u1, u2);

	const ConstructionParameterValidity cpv = evaluate_construction_parameter_validity(u1, u2, dot_p1_p2);

	/*
	 * First, we ensure that these two endpoints do in fact define a single
	 * unique great-circle arc.
	 */
	if (cpv == INVALID_ANTIPODAL_ENDPOINTS) {

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
		const real_t &closeness_inclusion_threshold,
		const real_t &latitude_exclusion_threshold,
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

		if( is_strictly_positive(closeness_inclusion_threshold) )
		{
			real_t closeness_to_poles = abs(dot(test_point.position_vector(), rotation_axis()));

			// This first if-statement should weed out most of the no-hopers.
			if (closeness_to_poles.is_precisely_greater_than(latitude_exclusion_threshold.dval())) 
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
		
		if (!closeness.is_precisely_greater_than(closeness_inclusion_threshold.dval()))
		{
			return boost::none;
		}

		return closest_point_on_great_circle_arc;
	}

	// The arc is point-like.
	if (!start_point().is_close_to(test_point, closeness_inclusion_threshold, closeness))
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
	const GreatCircleArcFeature gca_feature = calculate_closest_feature(
			*this, test_point, closest_point_on_great_circle_arc, closeness);

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
