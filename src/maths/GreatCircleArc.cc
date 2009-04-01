/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 The University of Sydney, Australia
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

#include "GreatCircleArc.h"
#include "Vector3D.h"
#include "FiniteRotation.h"
#include "IndeterminateResultException.h"
#include "IndeterminateArcRotationAxisException.h"


GPlatesMaths::GreatCircleArc::ConstructionParameterValidity
GPlatesMaths::GreatCircleArc::evaluate_construction_parameter_validity(
		const PointOnSphere &p1,
		const PointOnSphere &p2)
{
	const UnitVector3D &u1 = p1.position_vector();
	const UnitVector3D &u2 = p2.position_vector();

	real_t dp = dot(u1, u2);

#if 0  // Identical endpoints are now valid.
	if (dp >= 1.0) {

		// parallel => start-point same as end-point => no arc
		return INVALID_IDENTICAL_ENDPOINTS;
	}
#endif
	if (dp <= -1.0) {

		// antiparallel => start-point and end-point antipodal =>
		// indeterminate arc
		return INVALID_ANTIPODAL_ENDPOINTS;
	}

	return VALID;
}


const GPlatesMaths::GreatCircleArc
GPlatesMaths::GreatCircleArc::create(
		const PointOnSphere &p1,
		const PointOnSphere &p2)
{
	ConstructionParameterValidity cpv = evaluate_construction_parameter_validity(p1, p2);

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
		throw IndeterminateResultException(oss.str().c_str());
	}

	/*
	 * Now we want to calculate the unit vector normal to the plane
	 * of rotation (this vector also known as the "rotation axis").
	 *
	 * To do this, we calculate the cross product.
	 */
	const UnitVector3D &u1 = p1.position_vector();
	const UnitVector3D &u2 = p2.position_vector();

	Vector3D v = cross(u1, u2);

	/*
	 * Since u1 and u2 are unit vectors which might be parallel (but won't be antiparallel),
	 * the magnitude of v will be in the range [0, 1], and will be equal to the sine of the
	 * smaller of the two angles between u1 and u2.
	 */
	if (v.magSqrd() <= 0.0) {
		// The points are coincident, which means there is no determinate rotation axis.
		return GreatCircleArc(p1, p2);
	} else {
		// The points are non-coincident, which means there is a determinate rotation axis.
		UnitVector3D rot_axis = v.get_normalisation();
		return GreatCircleArc(p1, p2, rot_axis);
	}
}


const GPlatesMaths::GreatCircleArc
GPlatesMaths::GreatCircleArc::create_rotated_arc(
		const FiniteRotation &rot,
		const GreatCircleArc &arc)
{
	PointOnSphere start = rot * arc.start_point();
	PointOnSphere end   = rot * arc.end_point();
	if (arc.is_zero_length()) {
		// The arc is pointlike, so there is no determinate rotation axis.
		return GreatCircleArc(start, end);
	} else {
		// There is a determinate rotation axis.
		UnitVector3D rot_axis = rot * arc.rotation_axis();
		return GreatCircleArc(start, end, rot_axis);
	}
}


const GPlatesMaths::UnitVector3D &
GPlatesMaths::GreatCircleArc::rotation_axis() const
{
	if (is_zero_length()) {
		throw IndeterminateArcRotationAxisException(*this);
	}
	return *d_rot_axis;
}


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
	 * If, and only if, the returned feature is @a GCA_ARC then the closest point on the
	 * great circle arc is returned in @a closest_point_on_great_circle_arc.
	 */
	GreatCircleArcFeature
	calculate_closest_feature(
			const GPlatesMaths::GreatCircleArc &great_circle_arc,
			const GPlatesMaths::PointOnSphere &test_point,
			boost::optional<GPlatesMaths::UnitVector3D> &closest_point_on_great_circle_arc,
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

			if (closeness_c_to_a.isPreciselyGreaterThan(closeness_a_to_b.dval()) &&
				closeness_c_to_b.isPreciselyGreaterThan(closeness_a_to_b.dval())) {

				/*
				 * C is closer to A than B is to A, and also closer to
				 * B than A is to B, so C must lie _between_ A and B,
				 * which means it lies on the GCA.
				 *
				 * Hence, C is the closest point on the GCA to
				 * 'test_point'.
				 */
				closeness = dot(t, c);
				closest_point_on_great_circle_arc = c;
				return GCA_ARC;

			} else {

				/*
				 * C does not lie between A and B, so either A or B
				 * will be the closest point on the GCA to
				 * 'test_point'.
				 */
				if (closeness_c_to_a.isPreciselyGreaterThan(closeness_c_to_b.dval())) {

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
					return GCA_START_POINT;

				} else {

					closeness = dot(t, b);
					return GCA_END_POINT;
				}
			}
		}

		// The arc is point-like so return the start point.
		closeness = calculate_closeness(test_point, great_circle_arc.start_point());
		return GCA_START_POINT;
	}
}


bool
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

		real_t closeness_to_poles = abs(dot(test_point.position_vector(), rotation_axis()));

		// This first if-statement should weed out most of the no-hopers.
		if (closeness_to_poles.isPreciselyGreaterThan(latitude_exclusion_threshold.dval())) {

			/*
			 * 'test_point' lies within latitudes sufficiently far above or
			 * below the GC that there is no chance it could be "close to"
			 * this GCA.
			 */
			return false;

		} else {
			// Get the closest feature of this GCA to 'test_point'.
			boost::optional<UnitVector3D> closest_point_on_great_circle_arc;
			calculate_closest_feature(
					*this, test_point, closest_point_on_great_circle_arc, closeness);
			
			return closeness.isPreciselyGreaterThan(closeness_inclusion_threshold.dval());
		}
	}

	// The arc is point-like.
	return start_point().is_close_to(test_point, closeness_inclusion_threshold, closeness);
}

GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
GPlatesMaths::GreatCircleArc::get_closest_point(
		const PointOnSphere &test_point) const
{
	// Get the closest feature of this GCA to 'test_point'.
	real_t closeness;
	boost::optional<UnitVector3D> closest_point_on_great_circle_arc;
	const GreatCircleArcFeature gca_feature = calculate_closest_feature(
			*this, test_point, closest_point_on_great_circle_arc, closeness);

	if (gca_feature == GCA_ARC)
	{
		return PointOnSphere::create_on_heap(*closest_point_on_great_circle_arc);
	}
	else if (gca_feature == GCA_END_POINT)
	{
		return end_point().get_non_null_pointer();
	}

	return start_point().get_non_null_pointer();
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
