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

#ifndef GPLATES_MATHS_GREATCIRCLEARC_H
#define GPLATES_MATHS_GREATCIRCLEARC_H

#include <functional>  /* std::unary_function */
#include <utility>  /* std::pair */
#include <vector>
#include <boost/optional.hpp>  /* boost::optional */
#include <boost/none.hpp>  /* boost::none */

#include "PointOnSphere.h"

#include "global/PointerTraits.h"

namespace GPlatesMaths
{
	class FiniteRotation;
	class PolylineOnSphere;


	/** 
	 * A great-circle arc on the surface of a sphere.
	 *
	 * This class has no public constructors.  To create an instance, use the 'create' static
	 * member function.
	 *
	 * An arc is specified by a start-point and an end-point:  If these two points are not
	 * antipodal, a unique great-circle arc (with angle-span less than PI radians) will be
	 * determined between them.
	 *
	 * Note:
	 *  -# An arc @em may have duplicate points as the start-point and end-point; this will
	 * result in an arc of zero length.  (This arc will be like a single point, and thus will
	 * not determine any particular great-circle; nevertheless, it @em will be a valid arc for
	 * our purposes.)
	 *  -# No arc may have antipodal endpoints (or else there are an infinite number of
	 * great-circle arcs which could join the two points; thus, the arc is not determined).
	 *  -# It is not possible to create an arc which spans an angle greater than PI radians.
	 * (This is a result of the dot and cross products of vectors:  The angle between any two
	 * vectors is defined to always lie in the range [0, PI] radians).
	 *
	 * Thus, the angle spanned by the arc must lie in the open range [0, PI).
	 *
	 * Use the static member function 'evaluate_construction_parameter_validity' to test
	 * in advance whether the endpoints are going to be valid.
	 */
	class GreatCircleArc
	{
	public:
		enum ConstructionParameterValidity
		{
			VALID,
			INVALID_ANTIPODAL_ENDPOINTS
		};

		/**
		 * Test in advance whether the supplied great circle arc
		 * creation parameters would be valid or not.
		 */
		static
		ConstructionParameterValidity
		evaluate_construction_parameter_validity(
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
		const GreatCircleArc
		create(
				const PointOnSphere &p1,
				const PointOnSphere &p2);


		static
		const GreatCircleArc
		create_rotated_arc(
				const FiniteRotation &rot,
				const GreatCircleArc &arc);

		/**
		 * Return the start-point of the arc.
		 */
		const PointOnSphere &
		start_point() const
		{
			return d_start_point;
		}

		/**
		 * Return the end-point of the arc.
		 */
		const PointOnSphere &
		end_point() const
		{
			return d_end_point;
		}

		/**
		 * Return the (pre-computed) dot-product of the unit-vectors of
		 * the endpoints of the arc.
		 */
		const real_t &
		dot_of_endpoints() const
		{
			return d_dot_of_endpoints;
		}

		/**
		 * Return whether this great-circle arc is of zero length.
		 *
		 * If this arc is of zero length, it will not have a determinate rotation axis;
		 * attempting to access the rotation axis will result in the
		 * IndeterminateArcRotationAxisException being thrown.
		 */
		bool
		is_zero_length() const
		{
			return d_is_zero_length;
		}

		/**
		 * Return the rotation axis of the arc.
		 *
		 * Note that the rotation axis will only be defined if the start-point and
		 * end-point of the arc are not equal.
		 *
		 * Note:  It is only valid to invoke this member function upon a GreatCircleArc
		 * instance which is of non-zero length (ie, if @a is_zero_length would return
		 * false).  Otherwise, An IndeterminateArcRotationAxisException will be thrown.
		 */
		const UnitVector3D &
		rotation_axis() const;

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

		/**
		 * Finds the closest point on this arc to @a test_point.
		 */
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
		get_closest_point(
				const PointOnSphere &test_point) const;

		bool
		operator==(
				const GreatCircleArc &other) const;

	protected:

		/**
		 * Construct a great-circle arc instance of non-zero length, with a determinate
		 * rotation axis.
		 */
		GreatCircleArc(
				const PointOnSphere &p1,
				const PointOnSphere &p2,
				const UnitVector3D &rot_axis):
			d_start_point(p1),
			d_end_point(p2),
			d_dot_of_endpoints(dot(p1.position_vector(), p2.position_vector())),
			d_is_zero_length(false),
			d_rot_axis(rot_axis)
		{  }

		/**
		 * Construct a great-circle arc instance of zero length, without a determinate
		 * rotation axis.
		 */
		GreatCircleArc(
				const PointOnSphere &p1,
				const PointOnSphere &p2):
			d_start_point(p1),
			d_end_point(p2),
			d_dot_of_endpoints(dot(p1.position_vector(), p2.position_vector())),
			d_is_zero_length(true),
			d_rot_axis(boost::none)
		{  }

	private:

		PointOnSphere d_start_point, d_end_point;

		real_t d_dot_of_endpoints;

		bool d_is_zero_length;

		boost::optional<UnitVector3D> d_rot_axis;
	};


	/**
	 * This class instantiates to a function object which determines whether a GreatCircleArc
	 * has an indeterminate rotation axis.
	 *
	 * See Josuttis99, Chapter 8 "STL Function Objects", and in particular section 8.2.4
	 * "User-Defined Function Objects for Function Adapters", for more information about
	 * @a std::unary_function.
	 */
	struct ArcHasIndeterminateRotationAxis:
			public std::unary_function<GreatCircleArc, bool>
	{
		ArcHasIndeterminateRotationAxis()
		{  }

		bool
		operator()(
				const GreatCircleArc &arc) const
		{
			return arc.is_zero_length();
		}
	};


	/**
	 * Uniformly subdivides a great circle arc into smaller great circle arcs and appends the
	 * sequence of subdivided points to @a tessellation_points.
	 *
	 * The subdivided arcs have a maximum angular extent of @a max_segment_angular_extent radians.
	 * Each arc will extend the same angle (*uniform* subdivision) which will be less than or equal to
	 * @a max_segment_angular_extent radians.
	 *
	 * Note that if @a great_circle_arc already subtends an angle less than
	 * @a max_segment_angular_extent radians then only the great circle arc end points are appended.
	 */
	void
	tessellate(
			std::vector<PointOnSphere> &tessellation_points,
			const GreatCircleArc &great_circle_arc,
			const real_t &max_segment_angular_extent);


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
	 * Determine whether the two great-circle arcs @a arc1 and @a arc2 lie on the same
	 * great-circle.  This test ignores the directedness of the arcs.
	 *
	 * This operation is relatively computationally-inexpensive (in the cheapest case, a
	 * dot-product and some boolean comparisons).
	 */
	bool
	arcs_lie_on_same_great_circle(
			const GreatCircleArc &arc1,
			const GreatCircleArc &arc2);


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
}

#endif  // GPLATES_MATHS_GREATCIRCLEARC_H
