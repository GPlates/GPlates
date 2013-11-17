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
#include <boost/cstdint.hpp>
#include <boost/none.hpp>  /* boost::none */
#include <boost/optional.hpp>  /* boost::optional */
#include <boost/tuple/tuple.hpp>

#include "AngularDistance.h"
#include "AngularExtent.h"
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
				const PointOnSphere &p2)
		{
			const UnitVector3D &u1 = p1.position_vector();
			const UnitVector3D &u2 = p2.position_vector();

			return evaluate_construction_parameter_validity(u1, u2, dot(u1, u2));
		}

		/**
		 * Make a great circle arc beginning at @a p1 and ending at @a p2.
		 *
		 * NOTE: Only set @a check_validity to false if you are sure that construction parameter
		 * validity will not be violated. This is *only* useful in areas of code that require
		 * efficiency and where we are certain that @a IndeterminateResultException will not be thrown
		 * (for example because we've already called @a evaluate_construction_parameter_validity).
		 *
		 * @throws IndeterminateResultException when one of the following occurs:
		 *   - @a p1 and @a p2 are antipodal (that is, they are diametrically opposite on the globe).
		 */
		static
		const GreatCircleArc
		create(
				const PointOnSphere &p1,
				const PointOnSphere &p2,
				bool check_validity = true);


		static
		const GreatCircleArc
		create_rotated_arc(
				const FiniteRotation &rot,
				const GreatCircleArc &arc);


		/**
		 * Create the antipodal great circle arc of @a arc.
		 *
		 * The antipodal arc has the same rotation axis but it's end points are
		 * antipodal versions of the end points of @a arc.
		 */
		static
		const GreatCircleArc
		create_antipodal_arc(
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
		is_zero_length() const;

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
		 * The measure of what is "close" is provided by @a closeness_angular_extent_threshold.
		 *
		 * If @a test_point is "close", the function will calculate
		 * exactly @em how close, and store that value in @a closeness and
		 * return the closest point on the GreatCircleArc.
		 */
		boost::optional<GPlatesMaths::PointOnSphere>
		is_close_to(
				const PointOnSphere &test_point,
				const AngularExtent &closeness_angular_extent_threshold,
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

		bool
		operator!=(
				const GreatCircleArc &other) const
		{
			return !(*this == other);
		}

	protected:

		/**
		 * Construct a great-circle arc instance.
		 */
		GreatCircleArc(
				const PointOnSphere &p1,
				const PointOnSphere &p2,
				const real_t &dot_p1_p2):
			d_start_point(p1),
			d_end_point(p2),
			d_dot_of_endpoints(dot_p1_p2)
		{  }

	private:

		struct RotationInfo
		{
			//! For creating a *zero* length GCA.
			RotationInfo() :
				d_rot_axis(UnitVector3D::zBasis()/*dummy vector - not used*/),
				d_is_zero_length(true)
			{  }

			//! For creating a *non-zero* length GCA.
			explicit
			RotationInfo(
					const UnitVector3D &rot_axis_) :
				d_rot_axis(rot_axis_),
				d_is_zero_length(false)
			{  }

			/**
			 * The rotation axis - only valid if @a d_is_zero_length is false.
			 */
			UnitVector3D d_rot_axis;

			/**
			 * Whether the arc is zero-length and hence has no valid rotation axis.
			 */
			bool d_is_zero_length;
		};


		PointOnSphere d_start_point, d_end_point;

		real_t d_dot_of_endpoints;

		/**
		 * The rotation information.
		 *
		 * This is only calculated when needed - if boost::none then means it hasn't been calculated yet.
		 * This saves a noticeable amount of CPU time when the rotation axis is not actually needed
		 * such as displaying reconstructed polylines while animating the reconstruction time.
		 */
		mutable boost::optional<RotationInfo> d_rotation_info;


		static
		ConstructionParameterValidity
		evaluate_construction_parameter_validity(
				const UnitVector3D &p1,
				const UnitVector3D &p2,
				const real_t &dot_p1_p2);

		void
		calculate_rotation_info() const;
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
	 * Determine whether the two great-circle arcs @a arc1 and @a arc2 intersect each other.
	 *
	 * If @a intersection is specified then returns the intersection position (if the arcs intersected).
	 *
	 * Note that if both arcs lie on the same great circle, and they overlap each other, then the
	 * returned intersection point will be an arbitrary arc end point of one of the arcs.
	 */
	bool
	intersect(
			const GreatCircleArc &arc1,
			const GreatCircleArc &arc2,
			boost::optional<UnitVector3D &> intersection = boost::none);


	/**
	 * Returns the minimum angular distance between two great circle arcs.
	 *
	 * If they intersect then the returned angular distance will be zero.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then then closest points are *not* stored in
	 * @a closest_positions_on_arcs (even if it's not none).
	 *
	 * If @a closest_positions_on_arcs is specified then the closest point on each arc is stored in
	 * the unit vectors it references (unless the threshold is exceeded, if specified).
	 */
	AngularDistance
	minimum_distance(
			const GreatCircleArc &arc1,
			const GreatCircleArc &arc2,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<
					boost::tuple<UnitVector3D &/*closest point on arc1*/, UnitVector3D &/*closest point on arc2*/>
							> closest_positions_on_arcs = boost::none);


	/**
	 * Returns the minimum angular distance between a unit vector and a great circle arc, and
	 * optionally the closest point on the arc - optionally within a minimum threshold distance.
	 *
	 * If @a minimum_distance_threshold is specified then the returned distance will either be less
	 * than the threshold or AngularDistance::PI (maximum possible distance) to signify threshold exceeded.
	 * If the threshold is exceeded then then closest point is *not* stored in
	 * @a closest_position_on_great_circle_arc (even if it's not none).
	 *
	 * If @a closest_position_on_great_circle_arc is specified then the closest point on the arc
	 * is stored in the unit vector it references (unless the threshold is exceeded, if specified).
	 */
	AngularDistance
	minimum_distance(
			const UnitVector3D &position_vector,
			const GreatCircleArc &arc,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<UnitVector3D &> closest_position_on_great_circle_arc = boost::none);

	/**
	 * Overload of @a minimum_distance between a point and a great circle arc.
	 */
	inline
	AngularDistance
	minimum_distance(
			const GreatCircleArc &arc,
			const UnitVector3D &position_vector,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<UnitVector3D &> closest_position_on_great_circle_arc = boost::none)
	{
		return minimum_distance(
				position_vector,
				arc,
				minimum_distance_threshold,
				closest_position_on_great_circle_arc);
	}

	/**
	 * Overload of @a minimum_distance between a point and a great circle arc.
	 */
	inline
	AngularDistance
	minimum_distance(
			const PointOnSphere &point,
			const GreatCircleArc &arc,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<UnitVector3D &> closest_position_on_great_circle_arc = boost::none)
	{
		return minimum_distance(
				point.position_vector(),
				arc,
				minimum_distance_threshold,
				closest_position_on_great_circle_arc);
	}

	/**
	 * Overload of @a minimum_distance between a point and a great circle arc.
	 */
	inline
	AngularDistance
	minimum_distance(
			const GreatCircleArc &arc,
			const PointOnSphere &point,
			boost::optional<const AngularExtent &> minimum_distance_threshold = boost::none,
			boost::optional<UnitVector3D &> closest_position_on_great_circle_arc = boost::none)
	{
		return minimum_distance(
				point.position_vector(),
				arc,
				minimum_distance_threshold,
				closest_position_on_great_circle_arc);
	}


	/**
	 * Returns the maximum angular distance between two great circle arcs.
	 *
	 * If @a maximum_distance_threshold is specified then the returned distance will either be greater
	 * than the threshold or AngularDistance::ZERO (minimum possible distance) to signify threshold not exceeded.
	 * If the threshold is not exceeded then then furthest points are *not* stored in
	 * @a furthest_positions_on_arcs (even if it's not none).
	 *
	 * If @a furthest_positions_on_arcs is specified then the furthest point on each arc is stored in
	 * the unit vectors it references (unless the threshold is not exceeded, if specified).
	 */
	AngularDistance
	maximum_distance(
			const GreatCircleArc &arc1,
			const GreatCircleArc &arc2,
			boost::optional<const AngularExtent &> maximum_distance_threshold = boost::none,
			boost::optional<
					boost::tuple<UnitVector3D &/*furthest point on arc1*/, UnitVector3D &/*furthest point on arc2*/>
							> furthest_positions_on_arcs = boost::none);


	/**
	 * Returns the maximum angular distance between a unit vector and a great circle arc, and
	 * optionally the furthest point on the arc - optionally exceeding a maximum threshold distance.
	 *
	 * If @a maximum_distance_threshold is specified then the returned distance will either be greater
	 * than the threshold or AngularDistance::ZERO (minimum possible distance) to signify threshold not exceeded.
	 * If the threshold is not exceeded then then furthest point is *not* stored in
	 * @a furthest_position_on_great_circle_arc (even if it's not none).
	 *
	 * If @a furthest_position_on_great_circle_arc is specified then the furthest point on the arc
	 * is stored in the unit vector it references (unless the threshold is not exceeded, if specified).
	 */
	AngularDistance
	maximum_distance(
			const UnitVector3D &position_vector,
			const GreatCircleArc &arc,
			boost::optional<const AngularExtent &> maximum_distance_threshold = boost::none,
			boost::optional<UnitVector3D &> furthest_position_on_great_circle_arc = boost::none);

	/**
	 * Overload of @a maximum_distance between a point and a great circle arc.
	 */
	inline
	AngularDistance
	maximum_distance(
			const GreatCircleArc &arc,
			const UnitVector3D &position_vector,
			boost::optional<const AngularExtent &> maximum_distance_threshold = boost::none,
			boost::optional<UnitVector3D &> furthest_position_on_great_circle_arc = boost::none)
	{
		return maximum_distance(
				position_vector,
				arc,
				maximum_distance_threshold,
				furthest_position_on_great_circle_arc);
	}

	/**
	 * Overload of @a maximum_distance between a point and a great circle arc.
	 */
	inline
	AngularDistance
	maximum_distance(
			const PointOnSphere &point,
			const GreatCircleArc &arc,
			boost::optional<const AngularExtent &> maximum_distance_threshold = boost::none,
			boost::optional<UnitVector3D &> furthest_position_on_great_circle_arc = boost::none)
	{
		return maximum_distance(
				point.position_vector(),
				arc,
				maximum_distance_threshold,
				furthest_position_on_great_circle_arc);
	}

	/**
	 * Overload of @a maximum_distance between a point and a great circle arc.
	 */
	inline
	AngularDistance
	maximum_distance(
			const GreatCircleArc &arc,
			const PointOnSphere &point,
			boost::optional<const AngularExtent &> maximum_distance_threshold = boost::none,
			boost::optional<UnitVector3D &> furthest_position_on_great_circle_arc = boost::none)
	{
		return maximum_distance(
				point.position_vector(),
				arc,
				maximum_distance_threshold,
				furthest_position_on_great_circle_arc);
	}


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
