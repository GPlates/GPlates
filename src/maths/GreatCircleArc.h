/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GPLATES_MATHS_GREATCIRCLEARC_H
#define GPLATES_MATHS_GREATCIRCLEARC_H

#include <utility>  /* pair */
#include "PointOnSphere.h"

namespace GPlatesMaths {

	/** 
	 * A great-circle arc on the surface of a sphere.
	 *
	 * It has no public constructors.  To create an instance,
	 * use the 'create' static member functions.
	 *
	 * Note:
	 * - no great circle arc may have duplicate points as the start-
	 *    and end-point (or else no arc is specified).
	 * - no great circle arc may have antipodal endpoints (or else
	 *    the arc is not clearly specified).
	 * - no great circle may span an angle greater than pi radians
	 *    (this is an effect of the dot and cross products of vectors:
	 *    the angle between any two vectors is defined to always lie
	 *    in the range [0, PI] radians).
	 *
	 * Thus, the angle of the arc must lie in the open range (0, PI).
	 *
	 * Use the static member function 'test_parameter_status' to test
	 * in advance whether the endpoints are going to be valid.
	 */
	class GreatCircleArc {

	 public:

		/**
		 * FIXME: Change this to 'ConstructionParameterValidity'.
		 * And fix the goddamn indentation (how did I do it?).
		 */
	       enum ParameterStatus {

		       VALID,
		       INVALID_IDENTICAL_ENDPOINTS,
		       INVALID_ANTIPODAL_ENDPOINTS
	       };

	       /**
		* Test in advance whether the supplied great circle arc
		* creation parameters would be valid or not.
		*
		* FIXME: Change this function-name to
		* 'evaluate_construction_parameter_validity'.
		*/
	       static
	       ParameterStatus
	       test_parameter_status(
		const PointOnSphere &p1,
		const PointOnSphere &p2);

		/**
		 * Make a great circle arc beginning at @a p1 and
		 * ending at @a p2.
		 *
		 * @throws IndeterminateResultException when one of the 
		 *   following occurs:
		 *   - @a p1 and @a p2 are the same;
		 *   - @a p1 and @a p2 are antipodal (that is, they are
		 *     diametrically opposite on the globe).
		 */
		static
		GreatCircleArc
		create(
		 const PointOnSphere &p1,
		 const PointOnSphere &p2);

		/**
		 * @overload
		 *
		 * @throws InvalidOperationException when @a rot_axis
		 *   is not collinear with the cross product of the
		 *   vectors pointing from the origin to @a p1 and
		 *   @a p2 respectively.
		 */
		static
		GreatCircleArc
		create(
		 const PointOnSphere &p1,
		 const PointOnSphere &p2,
		 const UnitVector3D &rot_axis);

		/**
		 * Return the start-point of the arc.
		 */
		const PointOnSphere &
		start_point() const {
			
			return d_start_point;
		}

		/**
		 * Return the end-point of the arc.
		 */
		const PointOnSphere &
		end_point() const {
			
			return d_end_point;
		}

		/**
		 * Return the rotation axis of the arc.
		 */
		const UnitVector3D &
		rotation_axis() const {
			
			return d_rot_axis;
		}

		/**
		 * Return the (pre-computed) dot-product of the unit-vectors of
		 * the endpoints of the arc.
		 */
		const real_t &
		dot_of_endpoints() const {

			return d_dot_of_endpoints;
		}

		/**
		 * Evaluate whether @a test_point is "close" to this arc.
		 *
		 * The measure of what is "close" is provided by
		 * @a closeness_inclusion_threshold.
		 *
		 * If @a test_point is "close", the function will calculate
		 * exactly @em how close, and store that value in
		 * @a closeness.
		 *
		 * For more information, read the comment before
		 * @a GPlatesState::Layout::find_close_data.
		 */
		bool
		is_close_to(
		 const PointOnSphere &test_point,
		 const real_t &closeness_inclusion_threshold,
		 const real_t &latitude_exclusion_threshold,
		 real_t &closeness) const;

	 protected:

		GreatCircleArc(
		 const PointOnSphere &p1,
		 const PointOnSphere &p2,
		 const UnitVector3D &rot_axis) :
		 d_start_point(p1),
		 d_end_point(p2),
		 d_rot_axis(rot_axis),
		 d_dot_of_endpoints(dot(p1.position_vector(), p2.position_vector())) {  }

	 private:

		PointOnSphere d_start_point, d_end_point;

		UnitVector3D d_rot_axis;

		real_t d_dot_of_endpoints;

	};


	/**
	 * Determine whether the two great-circle arcs @a arc1 and @a arc2 are
	 * "near" each other.
	 *
	 * Obviously, "near" is a pretty subjective term, but if the two arcs
	 * are "near" each other according to this function, there is a chance
	 * they might overlap or intersect; conversely, if the two arcs are
	 * @em not "near" each other, there is @em no chance they might overlap
	 * or intersect.
	 *
	 * This operation is relatively computationally-inexpensive (three
	 * dot-products and a bunch of comparisons and boolean ORs), so it's a
	 * good way to eliminate no-hopers before expensive intersection or
	 * overlap calculations.
	 */
	bool
	arcs_are_near_each_other(
	 const GreatCircleArc &arc1,
	 const GreatCircleArc &arc2);


	/**
	 * Determine whether the two great-circle arcs @a arc1 and @a arc2 lie
	 * on the same great-circle.  This test ignores the directedness of the
	 * arcs.
	 *
	 * This operation is computationally-inexpensive (a dot-product and a
	 * comparison).
	 */
	inline
	bool
	arcs_lie_on_same_great_circle(
	 const GreatCircleArc &arc1,
	 const GreatCircleArc &arc2) {

		return collinear(arc1.rotation_axis(), arc2.rotation_axis());
	}


	/**
	 * Determine whether the two great-circle arcs @a @arc1 and @a arc2 are
	 * equivalent when the directedness of the arcs is taken into account.
	 */
	bool
	arcs_are_directed_equivalent(
	 const GreatCircleArc &arc1,
	 const GreatCircleArc &arc2);


	/**
	 * Determine whether the two great-circle arcs @a @arc1 and @a arc2 are
	 * equivalent when the directedness of the arcs is ignored.
	 */
	bool
	arcs_are_undirected_equivalent(
	 const GreatCircleArc &arc1,
	 const GreatCircleArc &arc2);


	/**
	 * Calculate the two unique (antipodal) points at which these two
	 * great-circle arcs would intersect if they were "extended" to whole
	 * great-circles.
	 *
	 * It is assumed that these arcs lie on distinct great-circles; if they
	 * do indeed lie on the same great-circle, an
	 * IndeterminateResultException will be thrown.
	 */
	std::pair< PointOnSphere, PointOnSphere >
	calculate_intersections_of_extended_arcs(
	 const GreatCircleArc &arc1,
	 const GreatCircleArc &arc2);

}

#endif  // GPLATES_MATHS_GREATCIRCLEARC_H
