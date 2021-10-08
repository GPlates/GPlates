/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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
#include "GreatCircleArc.h"
#include "Vector3D.h"
#include "IndeterminateResultException.h"
#include "InvalidOperationException.h"


GPlatesMaths::GreatCircleArc::ParameterStatus
GPlatesMaths::GreatCircleArc::test_parameter_status(
 const PointOnSphere &p1,
 const PointOnSphere &p2) {

	const UnitVector3D &u1 = p1.position_vector();
	const UnitVector3D &u2 = p2.position_vector();

	real_t dp = dot(u1, u2);

	if (dp >= 1.0) {

		// parallel => start-point same as end-point => no arc
		return INVALID_IDENTICAL_ENDPOINTS;
	}
	if (dp <= -1.0) {

		// antiparallel => start-point and end-point antipodal =>
		// indeterminate arc
		return INVALID_ANTIPODAL_ENDPOINTS;
	}

	return VALID;
}


GPlatesMaths::GreatCircleArc
GPlatesMaths::GreatCircleArc::create(
 const PointOnSphere &p1,
 const PointOnSphere &p2) {

	ParameterStatus ps = test_parameter_status(p1, p2);

	/*
	 * First, we ensure that these two endpoints do in fact define a single
	 * unique great-circle arc.
	 */
	if (ps == INVALID_IDENTICAL_ENDPOINTS) {

		// start-point same as end-point => no arc
		std::ostringstream oss;

		oss
		 << "Attempted to calculate a great-circle arc from "
		 << "duplicate endpoints "
		 << p1
		 << " and "
		 << p2
		 << ".";
		throw IndeterminateResultException(oss.str().c_str());
	}
	if (ps == INVALID_ANTIPODAL_ENDPOINTS) {

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
	 * Since u1 and u2 are unit vectors, the magnitude of v will be
	 * in the half-open range (0, 1], and will be equal to the sine
	 * of the smaller of the two angles between u1 and u2.
	 *
	 * Note that the magnitude of v cannot be _equal_ to zero,
	 * since the vectors are neither parallel nor antiparallel.
	 */
	UnitVector3D rot_axis = v.get_normalisation();

	return GreatCircleArc(p1, p2, rot_axis);
}


GPlatesMaths::GreatCircleArc
GPlatesMaths::GreatCircleArc::create(
 const PointOnSphere &p1,
 const PointOnSphere &p2,
 const UnitVector3D &rot_axis) {

	ParameterStatus ps = test_parameter_status(p1, p2);

	/*
	 * First, we ensure that these two endpoints do in fact define a single
	 * unique great-circle arc.
	 */
	if (ps == INVALID_IDENTICAL_ENDPOINTS) {

		// start-point same as end-point => no arc
		std::ostringstream oss;
		
		oss
		 << "Attempted to calculate a great-circle arc from "
		 << "duplicate endpoints "
		 << p1
		 << " and "
		 << p2
		 << ".";
		throw IndeterminateResultException(oss.str().c_str());
	}
	if (ps == INVALID_ANTIPODAL_ENDPOINTS) {

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
	 * Ensure that 'rot_axis' does, in fact, qualify as a rotation axis
	 * (ie. that it is collinear with (ie. parallel or antiparallel to)
	 * the cross product of 'u1' and 'u2').
	 *
	 * To do this, we calculate the cross product.
	 * (We use the Vector3D cross product, which is faster).
	 */
	const UnitVector3D &u1 = p1.position_vector();
	const UnitVector3D &u2 = p2.position_vector();

	Vector3D v = cross(Vector3D(u1), Vector3D(u2));
	if ( ! collinear(v, Vector3D(rot_axis))) {

		// 'rot_axis' is not the axis which rotates 'u1' into 'u2'
		std::ostringstream oss;
		
		oss
		 << "Attempted to calculate a great-circle arc from "
		 << "an invalid triple of vectors ("
		 << u1
		 << " and "
		 << u2
		 << " around "
		 << rot_axis
		 << ").";
		throw InvalidOperationException(oss.str().c_str());
	}

	return GreatCircleArc(p1, p2, rot_axis);
}


namespace {

	inline
	GPlatesMaths::UnitVector3D
	calculate_u_of_closest(
	 const GPlatesMaths::UnitVector3D &u_test_point,
	 const GPlatesMaths::UnitVector3D &rotation_axis) {

		/*
		 * The projection of 'u_test_point' in the direction of
		 * 'rotation_axis'.
		 */
		GPlatesMaths::Vector3D proj =
		 dot(u_test_point, rotation_axis) * rotation_axis;

		/*
		 * The projection of 'u_test_point' perpendicular to the
		 * direction of 'rotation_axis'.
		 */
		GPlatesMaths::Vector3D perp =
		 GPlatesMaths::Vector3D(u_test_point) - proj;

		return perp.get_normalisation();
	}

}


bool
GPlatesMaths::GreatCircleArc::is_close_to(
 const PointOnSphere &test_point,
 const real_t &closeness_inclusion_threshold,
 const real_t &latitude_exclusion_threshold,
 real_t &closeness) const {

	/*
	 * The following acronyms will be used:
	 * - GC = great-circle
	 * - GCA = great-circle-arc
	 */

	// First, a few convenient aliases.
	const UnitVector3D &n = rotation_axis();  // The "normal" to the GC.
	const UnitVector3D &t = test_point.position_vector();

	real_t closeness_to_poles = abs(dot(t, n));

	// This first if-statement should weed out most of the no-hopers.
	if (closeness_to_poles.isPreciselyGreaterThan(
	     latitude_exclusion_threshold.dval())) {

		/*
		 * 'test_point' lies within latitudes sufficiently far above or
		 * below the GC that there is no chance it could be "close to"
		 * this GCA.
		 */
		return false;

	} else {

		/*
		 * 'test_point' is sufficiently far from the poles (and hence,
		 * close to the GC) that it could be "close to" this GCA.
		 *
		 * In particular, because 'test_point' is far from the poles,
		 * it cannot be coincident with either of the poles, and hence
		 * there must exist a unique and well-defined "closest point"
		 * on the GC to 'test_point'.
		 */

		// A few more convenient aliases.
		const UnitVector3D &a = start_point().position_vector();
		const UnitVector3D &b = end_point().position_vector();

		// The unit-vector of the "closest point" on the GC.
		const UnitVector3D c = calculate_u_of_closest(t, n);

		real_t closeness_a_to_b = dot(a, b);
		real_t closeness_c_to_a = dot(c, a);
		real_t closeness_c_to_b = dot(c, b);

		if (closeness_c_to_a.isPreciselyGreaterThan(
		     closeness_a_to_b.dval()) &&
		    closeness_c_to_b.isPreciselyGreaterThan(
		     closeness_a_to_b.dval())) {

			/*
			 * C is closer to A than B is to A, and also closer to
			 * B than A is to B, so C must lie _between_ A and B,
			 * which means it lies on the GCA.
			 *
			 * Hence, C is the closest point on the GCA to
			 * 'test_point'.
			 */
			closeness = dot(t, c);
			return
			 (closeness.isPreciselyGreaterThan(
			   closeness_inclusion_threshold.dval()));

		} else {

			/*
			 * C does not lie between A and B, so either A or B
			 * will be the closest point on the GCA to
			 * 'test_point'.
			 */
			if (closeness_c_to_a.isPreciselyGreaterThan(
			     closeness_c_to_b.dval())) {

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

			} else {

				closeness = dot(t, b);
			}

			return
			 (closeness.isPreciselyGreaterThan(
			   closeness_inclusion_threshold.dval()));
		}
	}
}


bool
GPlatesMaths::arcs_are_near_each_other(
 const GreatCircleArc &arc1,
 const GreatCircleArc &arc2) {

	real_t
	 arc1_start_dot_arc2_start =
	  dot(arc1.start_point().position_vector(), arc2.start_point().position_vector()),
	 arc1_start_dot_arc2_end =
	  dot(arc1.start_point().position_vector(), arc2.end_point().position_vector()),
	 arc1_end_dot_arc2_start =
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
GPlatesMaths::arcs_are_directed_equivalent(
 const GreatCircleArc &arc1,
 const GreatCircleArc &arc2) {

	const PointOnSphere
	 &arc1_start = arc1.start_point(),
	 &arc1_end = arc1.end_point(),
	 &arc2_start = arc2.start_point(),
	 &arc2_end = arc2.end_point();

	return (points_are_coincident(arc1_start, arc2_start) &&
		points_are_coincident(arc1_end, arc2_end));
}


bool
GPlatesMaths::arcs_are_undirected_equivalent(
 const GreatCircleArc &arc1,
 const GreatCircleArc &arc2) {

	if ( ! arcs_lie_on_same_great_circle(arc1, arc2)) {

		// There is no way the arcs can be equivalent.
		return false;
	}

	const PointOnSphere
	 &arc1_start = arc1.start_point(),
	 &arc1_end = arc1.end_point(),
	 &arc2_start = arc2.start_point(),
	 &arc2_end = arc2.end_point();

	return ((points_are_coincident(arc1_start, arc2_start) &&
		 points_are_coincident(arc1_end, arc2_end)) ||
		(points_are_coincident(arc1_start, arc2_end) &&
		 points_are_coincident(arc1_end, arc2_start)));
}


std::pair< GPlatesMaths::PointOnSphere, GPlatesMaths::PointOnSphere >
GPlatesMaths::calculate_intersections_of_extended_arcs(
 const GreatCircleArc &arc1,
 const GreatCircleArc &arc2) {

	if (arcs_lie_on_same_great_circle(arc1, arc2)) {

		throw
		 IndeterminateResultException("Attempted to calculate the "
		  "unique intersection points\n of identical great-circles.");
	}
	Vector3D v = cross(arc1.rotation_axis(), arc2.rotation_axis());
	UnitVector3D normalised_v = v.get_normalisation();

	PointOnSphere inter_point1(normalised_v);
	PointOnSphere inter_point2( -normalised_v);

	return std::make_pair(inter_point1, inter_point2);
}
