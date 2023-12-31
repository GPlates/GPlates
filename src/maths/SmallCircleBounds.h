/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_MATHS_SMALLCIRCLEBOUNDS_H
#define GPLATES_MATHS_SMALLCIRCLEBOUNDS_H

#include <cmath>
#include <utility>
#include <boost/optional.hpp>


#include "AngularDistance.h"
#include "AngularExtent.h"
#include "GreatCircleArc.h"
#include "MultiPointOnSphere.h"
#include "PointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"
#include "types.h"
#include "UnitVector3D.h"


namespace GPlatesMaths
{
	class FiniteRotation;
	class Rotation;


	/**
	 * A small circle that encloses a region.
	 */
	class BoundingSmallCircle
	{
	public:

		/**
		 * Ideally use @a BoundingSmallCircleBuilder to construct this.
		 *
		 * @a angular_extent_radius is the angular extent (radius) of the small circle boundary
		 * with the small circle centre.
		 *
		 * This is also the minimum dot product of any geometry, bounded by this small circle,
		 * with the small circle centre.
		 */
		BoundingSmallCircle(
				const UnitVector3D &small_circle_centre,
				const AngularExtent &angular_extent_radius) :
			d_small_circle_centre(small_circle_centre),
			d_angular_extent(angular_extent_radius)
		{  }


		/**
		 * The result of testing a primitive against the bounding region
		 * can return fully outside, fully inside or intersecting.
		 */
		enum Result
		{
			OUTSIDE_BOUNDS,
			INSIDE_BOUNDS,
			INTERSECTING_BOUNDS
		};


		//! Test a point against the bounding region.
		Result
		test(
				const UnitVector3D &point) const;


		//! Test a great circle arc against the bounding region.
		Result
		test(
				const GreatCircleArc &gca) const;


		/**
		 * Test a sequence of great circle arcs against the bounding region.
		 *
		 * NOTE: The sequence of great circle arcs must form a continuous connected sequence -
		 * in other words the end of one arc must be the beginning of the next, etc, as is
		 * the case for a subsection of a polygon ring or polyline.
		 */
		template <typename GreatCircleArcForwardIter>
		Result
		test(
				GreatCircleArcForwardIter great_circle_arc_begin,
				GreatCircleArcForwardIter great_circle_arc_end) const;


		//! Test a point against the bounding region.
		Result
		test(
				const PointOnSphere &point) const
		{
			return test(point.position_vector());
		}


		//! Test a multi-point against the bounding region.
		Result
		test(
				const MultiPointOnSphere &multi_point) const;


		//! Test a polyline against the bounding region.
		Result
		test(
				const PolylineOnSphere &polyline) const
		{
			return test(polyline.begin(), polyline.end());
		}


		/**
		 * Test a polygon against the bounding region.
		 *
		 * Note that, like a polyline, this only tests if the outline of the polygon
		 * intersects the bounding small circle.
		 * Note that the polygon outline includes the exterior ring and any interior rings.
		 */
		Result
		test(
				const PolygonOnSphere &polygon) const;


		/**
		 * Test a filled polygon against the bounding region.
		 *
		 * This test differs from the regular polygon test in that we are not just
		 * testing if the outline of the polygon intersects the bounding region
		 * but also if the filled interior of the polygon intersects the bounding region.
		 *
		 * If the polygon outline is completely outside the small circle *and*
		 * the polygon surrounds the small circle then 'INTERSECTING_BOUNDS' is returned.
		 */
		Result
		test_filled_polygon(
				const PolygonOnSphere &polygon) const;


		/**
		 * Sets the small circle centre.
		 *
		 * This is the equivalent of moving this same small circle to a new location on the globe
		 * (eg, when moving the geometry bounding by this small circle).
		 */
		void
		set_centre(
				const UnitVector3D &centre)
		{
			d_small_circle_centre = centre;
		}

		/**
		 * Returns the small circle centre.
		 */
		const UnitVector3D &
		get_centre() const
		{
			return d_small_circle_centre;
		}


		/**
		 * Returns the angular extent (radius) of the small circle boundary with the small circle centre.
		 *
		 * This gives a measure of the minimum dot product of any part of any geometry,
		 * bounded by this small circle, with the small circle centre.
		 */
		const AngularExtent &
		get_angular_extent() const
		{
			return d_angular_extent;
		}


		/**
		 * Creates a bounding small circle extended by the specified angle.
		 *
		 * Returns a small circle with radius PI (ie, covering entire globe) if the sum of
		 * the angle of 'this' bounding small circle and @a angular_expansion exceeds PI.
		 */
		BoundingSmallCircle
		expand(
				const AngularExtent &angular_expansion) const
		{
			return BoundingSmallCircle(get_centre(), d_angular_extent + angular_expansion);
		}


		/**
		 * Creates a bounding small circle contracted by the specified angle.
		 *
		 * Returns a small circle of zero radius if @a angular_contraction is larger then the angle
		 * of 'this' bounding small circle.
		 */
		BoundingSmallCircle
		contract(
				const AngularExtent &angular_contraction) const
		{
			return BoundingSmallCircle(get_centre(), d_angular_extent - angular_contraction);
		}

	private:
		UnitVector3D d_small_circle_centre;
		AngularExtent d_angular_extent;

		friend class InnerOuterBoundingSmallCircle;
	};


	/**
	 * Apply the specified finite rotation to get a new bounding small circle.
	 *
	 * This is synonymous with rotating the bounded geometry.
	 */
	const BoundingSmallCircle
	operator*(
			const FiniteRotation &rotation,
			const BoundingSmallCircle &bounding_small_circle);

	/**
	 * Apply the specified rotation to get a new bounding small circle.
	 *
	 * This is synonymous with rotating the bounded geometry.
	 */
	const BoundingSmallCircle
	operator*(
			const Rotation &rotation,
			const BoundingSmallCircle &bounding_small_circle);


	/**
	 * Creates the optimal small circle that bounds the two specified bounding small circles.
	 *
	 * Returns the tightest fitting bounding small circle.
	 * This is useful when building a bounding binary tree in a bottom-up fashion (from leaves to root).
	 */
	BoundingSmallCircle
	create_optimal_bounding_small_circle(
			const BoundingSmallCircle &bounding_small_circle_1,
			const BoundingSmallCircle &bounding_small_circle_2);


	namespace SmallCircleBoundsImpl
	{
		inline
		bool
		intersect(
				const BoundingSmallCircle &bounding_small_circle,
				const double &point_dot_circle_centre)
		{
			// A point and a small circle intersect (or overlap) if the angle between the point and
			// the small circle centre is less than the small circle interior angle.
			return AngularExtent::create_from_cosine(point_dot_circle_centre).is_precisely_less_than(
					bounding_small_circle.get_angular_extent());
		}

		inline
		bool
		intersect(
				const BoundingSmallCircle &bounding_small_circle_1,
				const BoundingSmallCircle &bounding_small_circle_2,
				const double &dot_product_circle_centres)
		{
			// Two small circles intersect (or overlap) if the angle between their centres
			// is less than the sum of their interior angles.
			return AngularExtent::create_from_cosine(dot_product_circle_centres).is_precisely_less_than(
					bounding_small_circle_1.get_angular_extent() + bounding_small_circle_2.get_angular_extent());
		}

		inline
		AngularDistance
		minimum_distance(
				const BoundingSmallCircle &bounding_small_circle,
				const double &point_dot_circle_centre)
		{
			// Minimum distance between a point and a small circle is the angle between the point and
			// the small circle centre less the small circle interior angle.
			// Note that (AngularExtent - AngularExtent) clamps to zero (eg, if point intersects circle).
			return (
					AngularExtent::create_from_cosine(point_dot_circle_centre) -
					bounding_small_circle.get_angular_extent()
				).get_angular_distance();
		}

		inline
		AngularDistance
		minimum_distance(
				const BoundingSmallCircle &bounding_small_circle_1,
				const BoundingSmallCircle &bounding_small_circle_2,
				const double &dot_product_circle_centres)
		{
			// Minimum distance between two small circles is the angle between their centres
			// less the sum of their interior angles.
			// Note that (AngularExtent - AngularExtent) clamps to zero (eg, if circles intersect).
			return (
					AngularExtent::create_from_cosine(dot_product_circle_centres) -
					bounding_small_circle_1.get_angular_extent() -
					bounding_small_circle_2.get_angular_extent()
				).get_angular_distance();
		}
	}


	/**
	 * Returns true if the point intersects (is inside) the bounding small circle.
	 */
	inline
	bool
	intersect(
			const PointOnSphere &point,
			const BoundingSmallCircle &bounding_small_circle)
	{
		return SmallCircleBoundsImpl::intersect(
				bounding_small_circle,
				dot(point.position_vector(), bounding_small_circle.get_centre()).dval());
	}

	/**
	 * Returns true if the point intersects (is inside) the bounding small circle.
	 *
	 * This function simply reverses the arguments of the other @a intersect overload.
	 */
	inline
	bool
	intersect(
			const BoundingSmallCircle &bounding_small_circle,
			const PointOnSphere &point)
	{
		return intersect(point, bounding_small_circle);
	}


	/**
	 * Returns true if the position intersects (is inside) the bounding small circle.
	 *
	 * This function is a convenience overload using @a UnitVector3D instead of @a PointOnSphere.
	 */
	inline
	bool
	intersect(
			const UnitVector3D &position,
			const BoundingSmallCircle &bounding_small_circle)
	{
		return SmallCircleBoundsImpl::intersect(
				bounding_small_circle,
				dot(position, bounding_small_circle.get_centre()).dval());
	}

	/**
	 * Returns true if the position intersects (is inside) the bounding small circle.
	 *
	 * This function simply reverses the arguments of the other @a intersect overload.
	 */
	inline
	bool
	intersect(
			const BoundingSmallCircle &bounding_small_circle,
			const UnitVector3D &position)
	{
		return intersect(position, bounding_small_circle);
	}


	/**
	 * Returns true if the two bounding small circles intersect each other.
	 */
	inline
	bool
	intersect(
			const BoundingSmallCircle &bounding_small_circle_1,
			const BoundingSmallCircle &bounding_small_circle_2)
	{
		return SmallCircleBoundsImpl::intersect(
				bounding_small_circle_1,
				bounding_small_circle_2,
				dot(bounding_small_circle_1.get_centre(), bounding_small_circle_2.get_centre()).dval());
	}


	/**
	 * Returns the minimum angular distance between a point and a bounding small circle.
	 *
	 * If the point is inside the bounding small circle then the returned angular distance will be zero.
	 */
	inline
	AngularDistance
	minimum_distance(
			const PointOnSphere &point,
			const BoundingSmallCircle &bounding_small_circle)
	{
		return SmallCircleBoundsImpl::minimum_distance(
				bounding_small_circle,
				dot(point.position_vector(), bounding_small_circle.get_centre()).dval());
	}

	/**
	 * Returns the minimum angular distance between a point and a bounding small circle.
	 *
	 * This function simply reverses the arguments of the other @a minimum_distance overload.
	 */
	inline
	AngularDistance
	minimum_distance(
			const BoundingSmallCircle &bounding_small_circle,
			const PointOnSphere &point)
	{
		return minimum_distance(point, bounding_small_circle);
	}


	/**
	 * Returns the minimum angular distance between a position and a bounding small circle.
	 *
	 * This function is a convenience overload using @a UnitVector3D instead of @a PointOnSphere.
	 */
	inline
	AngularDistance
	minimum_distance(
			const UnitVector3D &position,
			const BoundingSmallCircle &bounding_small_circle)
	{
		return SmallCircleBoundsImpl::minimum_distance(
				bounding_small_circle,
				dot(position, bounding_small_circle.get_centre()).dval());
	}

	/**
	 * Returns the minimum angular distance between a position and a bounding small circle.
	 *
	 * This function simply reverses the arguments of the other @a minimum_distance overload.
	 */
	inline
	AngularDistance
	minimum_distance(
			const BoundingSmallCircle &bounding_small_circle,
			const UnitVector3D &position)
	{
		return minimum_distance(position, bounding_small_circle);
	}


	/**
	 * Returns the minimum angular distance between two bounding small circles.
	 *
	 * If they intersect then the returned angular distance will be zero.
	 */
	inline
	AngularDistance
	minimum_distance(
			const BoundingSmallCircle &bounding_small_circle_1,
			const BoundingSmallCircle &bounding_small_circle_2)
	{
		return SmallCircleBoundsImpl::minimum_distance(
				bounding_small_circle_1,
				bounding_small_circle_2,
				dot(bounding_small_circle_1.get_centre(), bounding_small_circle_2.get_centre()).dval());
	}


	/**
	 * Used to incrementally build an @a BoundingSmallCircle.
	 */
	class BoundingSmallCircleBuilder
	{
	public:
		explicit
		BoundingSmallCircleBuilder(
				const UnitVector3D &small_circle_centre);


		/**
		 * Adds a point on the sphere - the bound is expanded if necessary to bound the point.
		 */
		void
		add(
				const UnitVector3D &point);


		/**
		 * Adds a great circle arc - the bound is expanded if necessary to bound the great circle arc.
		 */
		void
		add(
				const GreatCircleArc &gca)
		{
			const AngularDistance max_distance_to_gca = maximum_distance(d_small_circle_centre, gca);
			if (max_distance_to_gca.is_precisely_greater_than(d_maximum_distance))
			{
				d_maximum_distance = max_distance_to_gca;
			}
		}


		/**
		 * Adds a sequence of great circle arcs and calls @a add on each great circle arc.
		 */
		template <typename GreatCircleArcForwardIter>
		void
		add(
				GreatCircleArcForwardIter great_circle_arc_begin,
				GreatCircleArcForwardIter great_circle_arc_end);


		/**
		 * Adds a point on the sphere - the bound is expanded if necessary to bound the point.
		 */
		void
		add(
				const PointOnSphere &point)
		{
			add(point.position_vector());
		}


		/**
		 * Adds a multi-point on the sphere - the bound is expanded if necessary to bound the points.
		 */
		void
		add(
				const MultiPointOnSphere &multi_point);


		/**
		 * Adds the great circle arcs in a polygon - the bound is expanded if necessary to bound them.
		 *
		 * Note that the concept of inside/outside polygon (eg, point-in-polygon test) does not apply here.
		 * The polygon's exterior ring and interior rings are added as if they were independent GCA sequences.
		 */
		void
		add(
				const PolygonOnSphere &polygon)
		{
			add(polygon.exterior_ring_begin(), polygon.exterior_ring_end());

			for (unsigned int i = 0; i < polygon.number_of_interior_rings(); ++i)
			{
				add(polygon.interior_ring_begin(i), polygon.interior_ring_end(i));
			}
		}


		/**
		 * Adds the great circle arcs in a polyline - the bound is expanded if necessary to bound them.
		 */
		void
		add(
				const PolylineOnSphere &polyline)
		{
			add(polyline.begin(), polyline.end());
		}


		/**
		 * Expands the bound to include the small circle of @a bounding_small_circle.
		 */
		void
		add(
				const BoundingSmallCircle &bounding_small_circle);


		/**
		 * Returns the bounding small circle of all primitives added so far.
		 *
		 * @a angular_expansion is used to expand the bound to account for numerical precision.
		 *
		 * The default expansion matches the closeness threshold for intersection detection
		 * (used when determining if one geometry is close enough to be *touching* another geometry).
		 * This ensures that the bounding small circle expands to include the *touching* region around
		 * the geometries contained within the bounding small circle.
		 *
		 * A non-zero angular expansion means tests for intersection/inclusion can return false positives
		 * but in most situations this is desirable because then a more accurate test will reveal there's
		 * no intersection/inclusion - whereas the other way around can give the wrong result.
		 *
		 * Note that if the sum of the angle of bounding small circle and @a angular_expansion exceeds
		 * PI then a small circle with radius PI (ie, covering entire globe) is returned.
		 */
		BoundingSmallCircle
		get_bounding_small_circle(
				const AngularExtent &angular_expansion = get_default_angular_expansion()) const
		{
			return BoundingSmallCircle(
					d_small_circle_centre,
					AngularExtent(d_maximum_distance) + angular_expansion);
		}

	private:
		UnitVector3D d_small_circle_centre;
		AngularDistance d_maximum_distance;

	public:
		/**
		 * Default angular expansion used to expand returned bounding small circles.
		 *
		 * The default expansion matches the closeness threshold for intersection detection
		 * (used when determining if one geometry is close enough to be *touching* another geometry).
		 * This ensures that the bounding small circle expands to include the *touching* region around
		 * the geometries contained within the bounding small circle.
		 */
		static
		const AngularExtent &
		get_default_angular_expansion();
	};


	/**
	 * Two concentric small circles that enclose an annular region.
	 */
	class InnerOuterBoundingSmallCircle
	{
	public:

		/**
		 * Use @a InnerOuterBoundingSmallCircleBuilder to construct this.
		 *
		 * @a outer_angular_extent_radius is the angular extent (radius) of the *outer* small circle
		 * boundary with the small circle centre.
		 * @a inner_angular_extent_radius is the angular extent (radius) of the *inner* small circle
		 * boundary with the small circle centre.

		 * These are also the minimum and maximum dot products of any geometry, bounded by these
		 * small circles, with the small circle(s) common centre.
		 */
		InnerOuterBoundingSmallCircle(
				const UnitVector3D &small_circle_centre,
				const AngularExtent &outer_angular_extent_radius,
				const AngularExtent &inner_angular_extent_radius) :
			d_outer_small_circle(small_circle_centre, outer_angular_extent_radius),
			d_inner_angular_extent(inner_angular_extent_radius)
		{  }

		/**
		 * The result of testing a primitive against the bounding region
		 * can return fully outside, fully inside or intersecting the region
		 * between the inner and outer bounds.
		 */
		enum Result
		{
			OUTSIDE_OUTER_BOUNDS,
			INSIDE_INNER_BOUNDS,
			INTERSECTING_BOUNDS
		};


		//! Test a point against the bounding region.
		Result
		test(
				const UnitVector3D &point) const;


		//! Test a great circle arc against the bounding region.
		Result
		test(
				const GreatCircleArc &gca) const;


		/**
		 * Test a sequence of great circle arcs against the bounding region.
		 *
		 * NOTE: The sequence of great circle arcs must form a continuous connected sequence -
		 * in other words the end of one arc must be the beginning of the next, etc, as is
		 * the case for a subsection of a polygon ring or polyline.
		 */
		template <typename GreatCircleArcForwardIter>
		Result
		test(
				GreatCircleArcForwardIter great_circle_arc_begin,
				GreatCircleArcForwardIter great_circle_arc_end) const;


		//! Test a point against the bounding region.
		Result
		test(
				const PointOnSphere &point) const
		{
			return test(point.position_vector());
		}


		//! Test a multi-point against the bounding region.
		Result
		test(
				const MultiPointOnSphere &multi_point) const;


		//! Test a polyline against the bounding region.
		Result
		test(
				const PolylineOnSphere &polyline) const
		{
			return test(polyline.begin(), polyline.end());
		}


		/**
		 * Test a polygon against the bounding region.
		 *
		 * Note that, like a polyline, this only tests if the outline of the polygon
		 * intersects the bounding region.
		 * Note that the polygon outline includes the exterior ring and any interior rings.
		 *
		 * If one or more polygon rings are outside outer bounds and one or more are inside
		 * inner bounds (eg, exterior ring is outside outer bounds and its one interior ring
		 * is inside inner bounds) then this is considered intersecting.
		 */
		Result
		test(
				const PolygonOnSphere &polygon) const;


		/**
		 * Test a filled polygon against the bounding region.
		 *
		 * This test differs from the regular polygon test in that we are not just
		 * testing if the outline of the polygon intersects the bounding region
		 * but also if the interior of the polygon intersects the bounding region.
		 *
		 * If the polygon outline is completely outside the outer small circle *and*
		 * the polygon surrounds the outer small circle then 'INTERSECTING_BOUNDS' is returned.
		 */
		Result
		test_filled_polygon(
				const PolygonOnSphere &polygon) const;


		/**
		 * Sets the centre of both small circles.
		 *
		 * This is the equivalent of moving this same inner/outer small circle to a new location
		 * on the globe (eg, when moving the geometry bounding by this inner/outer small circle).
		 */
		void
		set_centre(
				const UnitVector3D &centre)
		{
			d_outer_small_circle.set_centre(centre);
		}

		/**
		 * Returns the centre of both small circles.
		 */
		const UnitVector3D &
		get_centre() const
		{
			return d_outer_small_circle.get_centre();
		}


		/**
		 * Returns the angular extent (radius) of the *inner* small circle boundary with the small circle centre.
		 *
		 * This gives a measure of the maximum dot product of any part of any geometry,
		 * bounded by the annular region, with the small circle centre.
		 */
		const AngularExtent &
		get_inner_angular_extent() const
		{
			return d_inner_angular_extent;
		}


		/**
		 * Returns the angular extent (radius) of the *outer* small circle boundary with the small circle centre.
		 *
		 * This gives a measure of the minimum dot product of any part of any geometry,
		 * bounded by the annular region, with the small circle centre.
		 */
		const AngularExtent &
		get_outer_angular_extent() const
		{
			return d_outer_small_circle.get_angular_extent();
		}


		/**
		 * Returns the inner small circle boundary - a simplification that ignores the outer bounds.
		 */
		const BoundingSmallCircle
		get_inner_bounding_small_circle() const
		{
			return BoundingSmallCircle(
					d_outer_small_circle.get_centre(),
					d_inner_angular_extent);
		}


		/**
		 * Returns the bounding small circle - a simplification that ignores the inner bounds.
		 *
		 * This is equivalent to the outer bounding small circle.
		 */
		const BoundingSmallCircle &
		get_outer_bounding_small_circle() const
		{
			return d_outer_small_circle;
		}

	private:
		BoundingSmallCircle d_outer_small_circle;
		AngularExtent d_inner_angular_extent;
	};


	/**
	 * Apply the specified finite rotation to get a new inner/outer bounding small circle.
	 *
	 * This is synonymous with rotating the bounded geometry.
	 */
	const InnerOuterBoundingSmallCircle
	operator*(
			const FiniteRotation &rotation,
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle);

	/**
	 * Apply the specified rotation to get a new inner/outer bounding small circle.
	 *
	 * This is synonymous with rotating the bounded geometry.
	 */
	const InnerOuterBoundingSmallCircle
	operator*(
			const Rotation &rotation,
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle);


	namespace SmallCircleBoundsImpl
	{
		bool
		intersect(
				const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle,
				const double &point_dot_circle_centre);

		bool
		intersect(
				const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle,
				const BoundingSmallCircle &bounding_small_circle,
				const double &dot_product_circle_centres);

		bool
		intersect(
				const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle_1,
				const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle_2,
				const double &dot_product_circle_centres);

		AngularDistance
		minimum_distance(
				const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle,
				const double &point_dot_circle_centre);

		AngularDistance
		minimum_distance(
				const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle,
				const BoundingSmallCircle &bounding_small_circle,
				const double &dot_product_circle_centres);

		AngularDistance
		minimum_distance(
				const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle_1,
				const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle_2,
				const double &dot_product_circle_centres);

		inline
		bool
		is_inside_inner_bounding_small_circle(
				const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle,
				const BoundingSmallCircle &bounding_small_circle,
				const double &dot_product_circle_centres)
		{
			// angle_between_centres + angle_small_circle < angle_inner_circle
			return (
					AngularExtent::create_from_cosine(dot_product_circle_centres) +
					bounding_small_circle.get_angular_extent()
				).is_precisely_less_than(inner_outer_bounding_small_circle.get_inner_angular_extent());
		}

		inline
		bool
		is_inside_inner_bounding_small_circle(
				const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle_1,
				const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle_2,
				const double &dot_product_circle_centres)
		{
			return is_inside_inner_bounding_small_circle(
				inner_outer_bounding_small_circle_1,
				inner_outer_bounding_small_circle_2.get_outer_bounding_small_circle(),
				dot_product_circle_centres);
		}
	}


	/**
	 * Returns true if the point intersects the inner-outer bounding small circle.
	 *
	 * Note that if @a point is completely inside the inner bounding small circle
	 * of @a inner_outer_bounding_small_circle then false is returned.
	 */
	inline
	bool
	intersect(
			const PointOnSphere &point,
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle)
	{
		return SmallCircleBoundsImpl::intersect(
				inner_outer_bounding_small_circle,
				dot(point.position_vector(), inner_outer_bounding_small_circle.get_centre()).dval());
	}

	/**
	 * Returns true if the point intersects the inner-outer bounding small circle.
	 *
	 * This function simply reverses the arguments of the other @a intersect overload.
	 */
	inline
	bool
	intersect(
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle,
			const PointOnSphere &point)
	{
		return intersect(point, inner_outer_bounding_small_circle);
	}


	/**
	 * Returns true if the position intersects the inner-outer bounding small circle.
	 *
	 * This function is a convenience overload using @a UnitVector3D instead of @a PointOnSphere.
	 */
	inline
	bool
	intersect(
			const UnitVector3D &position,
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle)
	{
		return SmallCircleBoundsImpl::intersect(
				inner_outer_bounding_small_circle,
				dot(position, inner_outer_bounding_small_circle.get_centre()).dval());
	}

	/**
	 * Returns true if the position intersects the inner-outer bounding small circle.
	 *
	 * This function simply reverses the arguments of the other @a intersect overload.
	 */
	inline
	bool
	intersect(
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle,
			const UnitVector3D &position)
	{
		return intersect(position, inner_outer_bounding_small_circle);
	}


	/**
	 * Returns true if a bounding small circle intersects an inner-outer bounding small circle.
	 *
	 * Note that if @a bounding_small_circle is completely inside the inner bounding small circle
	 * of @a inner_outer_bounding_small_circle then false is returned.
	 *
	 * This is useful for testing if a geometry can possibly intersect the line geometry of
	 * a polygon (the inner-outer bounds are for the polygon) which is useful when
	 * clipping geometries to a polygon.
	 */
	inline
	bool
	intersect(
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle,
			const BoundingSmallCircle &bounding_small_circle)
	{
		return SmallCircleBoundsImpl::intersect(
				inner_outer_bounding_small_circle,
				bounding_small_circle,
				dot(
					inner_outer_bounding_small_circle.get_centre(),
					bounding_small_circle.get_centre()).dval());
	}


	/**
	 * Returns true if a bounding small circle intersects an inner-outer bounding small circle.
	 *
	 * This function simply reverses the arguments of the other @a intersect overload.
	 */
	inline
	bool
	intersect(
			const BoundingSmallCircle &bounding_small_circle,
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle)
	{
		return intersect(inner_outer_bounding_small_circle, bounding_small_circle);
	}


	/**
	 * Returns true if the two inner-outer bounding small circles intersect each other.
	 *
	 * Note that if the outer bounds of either is completely inside the inner bounds of the
	 * other then false is returned.
	 */
	inline
	bool
	intersect(
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle_1,
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle_2)
	{
		return SmallCircleBoundsImpl::intersect(
				inner_outer_bounding_small_circle_1,
				inner_outer_bounding_small_circle_2,
				dot(
					inner_outer_bounding_small_circle_1.get_centre(),
					inner_outer_bounding_small_circle_2.get_centre()).dval());
	}


	/**
	 * Returns the minimum angular distance between a point and an inner-outer bounding small circle.
	 *
	 * If the point is inside the annular region then the returned angular distance will be zero.
	 */
	inline
	AngularDistance
	minimum_distance(
			const PointOnSphere &point,
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle)
	{
		return SmallCircleBoundsImpl::minimum_distance(
				inner_outer_bounding_small_circle,
				dot(point.position_vector(), inner_outer_bounding_small_circle.get_centre()).dval());
	}

	/**
	 * Returns the minimum angular distance between a point and an inner-outer bounding small circle.
	 *
	 * This function simply reverses the arguments of the other @a minimum_distance overload.
	 */
	inline
	AngularDistance
	minimum_distance(
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle,
			const PointOnSphere &point)
	{
		return minimum_distance(point, inner_outer_bounding_small_circle);
	}


	/**
	 * Returns the minimum angular distance between a position and an inner-outer bounding small circle.
	 *
	 * This function is a convenience overload using @a UnitVector3D instead of @a PointOnSphere.
	 */
	inline
	AngularDistance
	minimum_distance(
			const UnitVector3D &position,
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle)
	{
		return SmallCircleBoundsImpl::minimum_distance(
				inner_outer_bounding_small_circle,
				dot(position, inner_outer_bounding_small_circle.get_centre()).dval());
	}

	/**
	 * Returns the minimum angular distance between a position and an inner-outer bounding small circle.
	 *
	 * This function simply reverses the arguments of the other @a minimum_distance overload.
	 */
	inline
	AngularDistance
	minimum_distance(
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle,
			const UnitVector3D &position)
	{
		return minimum_distance(position, inner_outer_bounding_small_circle);
	}


	/**
	 * Returns the minimum angular distance of a bounding small circle to an inner-outer
	 * bounding small circle.
	 *
	 * If they intersect then the returned angular distance will be zero.
	 *
	 * Note that if @a bounding_small_circle is completely inside the inner bounding small circle
	 * of @a inner_outer_bounding_small_circle then a non-zero distance will be returned.
	 *
	 * This is useful for testing how close a geometry can possibly get to the line geometry of
	 * a polygon (the inner-outer bounds are for the polygon) which is useful when determining
	 * whether or not to calculate the actual minimum distance of the geometry to the polygon.
	 */
	inline
	AngularDistance
	minimum_distance(
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle,
			const BoundingSmallCircle &bounding_small_circle)
	{
		return SmallCircleBoundsImpl::minimum_distance(
				inner_outer_bounding_small_circle,
				bounding_small_circle,
				dot(
					inner_outer_bounding_small_circle.get_centre(),
					bounding_small_circle.get_centre()).dval());
	}


	/**
	 * Returns the minimum angular distance of a bounding small circle to an inner-outer
	 * bounding small circle.
	 *
	 * This function simply reverses the arguments of the other @a intersect overload.
	 */
	inline
	AngularDistance
	minimum_distance(
			const BoundingSmallCircle &bounding_small_circle,
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle)
	{
		return minimum_distance(inner_outer_bounding_small_circle, bounding_small_circle);
	}


	/**
	 * Returns the minimum angular distance between two inner-outer bounding small circles.
	 *
	 * If they intersect then the returned angular distance will be zero.
	 *
	 * Note that if the outer bounds of either is completely inside the inner bounds of the
	 * other then a non-zero distance will be returned.
	 */
	inline
	AngularDistance
	minimum_distance(
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle_1,
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle_2)
	{
		return SmallCircleBoundsImpl::minimum_distance(
				inner_outer_bounding_small_circle_1,
				inner_outer_bounding_small_circle_2,
				dot(
					inner_outer_bounding_small_circle_1.get_centre(),
					inner_outer_bounding_small_circle_2.get_centre()).dval());
	}


	/**
	 * Returns true if @a bounding_small_circle is completely inside the inner bounding small circle
	 * of @a inner_outer_bounding_small_circle.
	 *
	 * This is useful for testing if a geometry is completely inside a polygon (the inner-outer
	 * bounds are for the polygon).
	 */
	inline
	bool
	is_inside_inner_bounding_small_circle(
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle,
			const BoundingSmallCircle &bounding_small_circle)
	{
		return SmallCircleBoundsImpl::is_inside_inner_bounding_small_circle(
				inner_outer_bounding_small_circle,
				bounding_small_circle,
				dot(
					inner_outer_bounding_small_circle.get_centre(),
					bounding_small_circle.get_centre()).dval());
	}


	/**
	 * Returns true if @a bounding_small_circle is completely inside the inner bounding small circle
	 * of @a inner_outer_bounding_small_circle.
	 *
	 * This function simply reverses the arguments of the other @a is_inside_inner_bounding_small_circle overload.
	 */
	inline
	bool
	is_inside_inner_bounding_small_circle(
			const BoundingSmallCircle &bounding_small_circle,
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle)
	{
		return is_inside_inner_bounding_small_circle(inner_outer_bounding_small_circle, bounding_small_circle);
	}


	/**
	 * Returns true if @a inner_outer_bounding_small_circle_2 is completely inside the
	 * inner bounding small circle of @a inner_outer_bounding_small_circle_1.
	 *
	 * This is useful for testing if a polygon is completely inside another polygon.
	 */
	inline
	bool
	is_inside_inner_bounding_small_circle(
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle_1,
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle_2)
	{
		return SmallCircleBoundsImpl::is_inside_inner_bounding_small_circle(
				inner_outer_bounding_small_circle_1,
				inner_outer_bounding_small_circle_2.get_outer_bounding_small_circle(),
				dot(
					inner_outer_bounding_small_circle_1.get_centre(),
					inner_outer_bounding_small_circle_2.get_centre()).dval());
	}


	/**
	 * Used to incrementally build an @a InnerOuterBoundingSmallCircle.
	 */
	class InnerOuterBoundingSmallCircleBuilder
	{
	public:
		explicit
		InnerOuterBoundingSmallCircleBuilder(
				const UnitVector3D &small_circle_centre);


		/**
		 * Adds a point on the sphere - the inner/outer bounds are contracted/expanded
		 * if necessary to bound the point.
		 */
		void
		add(
				const UnitVector3D &point);


		/**
		 * Adds a great circle arc - the inner/outer bounds are contracted/expanded
		 * if necessary to bound the great circle arc.
		 */
		void
		add(
				const GreatCircleArc &gca)
		{
			const AngularDistance min_distance_to_gca = minimum_distance(d_small_circle_centre, gca);
			if (min_distance_to_gca.is_precisely_less_than(d_minimum_distance))
			{
				d_minimum_distance = min_distance_to_gca;
			}

			const AngularDistance max_distance_to_gca = maximum_distance(d_small_circle_centre, gca);
			if (max_distance_to_gca.is_precisely_greater_than(d_maximum_distance))
			{
				d_maximum_distance = max_distance_to_gca;
			}
		}


		/**
		 * Adds a sequence of great circle arcs and calls @a add on each great circle arc.
		 */
		template <typename GreatCircleArcForwardIter>
		void
		add(
				GreatCircleArcForwardIter great_circle_arc_begin,
				GreatCircleArcForwardIter great_circle_arc_end);


		/**
		 * Adds a point on the sphere - the inner/outer bounds are contracted/expanded
		 * if necessary to bound the point.
		 */
		void
		add(
				const PointOnSphere &point)
		{
			add(point.position_vector());
		}


		/**
		 * Adds a multi-point on the sphere - the inner/outer bounds are contracted/expanded
		 * if necessary to bound the points.
		 */
		void
		add(
				const MultiPointOnSphere &multi_point);


		/**
		 * Adds the great circle arcs in a polygon - the inner/outer bounds are contracted/expanded
		 * if necessary to bound the polygon.
		 *
		 * Note that the concept of inside/outside polygon (eg, point-in-polygon test) does not apply here.
		 * The polygon's exterior ring and interior rings are added as if they were independent GCA sequences.
		 */
		void
		add(
				const PolygonOnSphere &polygon)
		{
			add(polygon.exterior_ring_begin(), polygon.exterior_ring_end());

			for (unsigned int i = 0; i < polygon.number_of_interior_rings(); ++i)
			{
				add(polygon.interior_ring_begin(i), polygon.interior_ring_end(i));
			}
		}


		/**
		 * Adds the great circle arcs in a polyline - the inner/outer bounds are contracted/expanded
		 * if necessary to bound the polyline.
		 */
		void
		add(
				const PolylineOnSphere &polyline)
		{
			add(polyline.begin(), polyline.end());
		}


		/**
		 * Expands the bound to include the small circle of @a bounding_small_circle.
		 */
		void
		add(
				const BoundingSmallCircle &bounding_small_circle);


		/**
		 * Expands/contracts the outer/inner bounds to include the region bounded by @a inner_outer_bounding_small_circle.
		 */
		void
		add(
				const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle);


		/**
		 * Returns the inner outer bounding small circle of all primitives added so far.
		 *
		 * @a inner_bound_angular_contraction is used to contract the *inner* bound to
		 * account for numerical precision.
		 * @a expand_outer_bound_delta_dot_product is used to expand the *outer* bound to
		 * account for numerical precision.
		 *
		 * The default contraction/expansion matches the closeness threshold for intersection detection
		 * (used when determining if one geometry is close enough to be *touching* another geometry).
		 * This ensures that the inner/outer bounding small circle contracts/expands to include the
		 * *touching* region around the geometries contained within the bounded region.
		 *
		 * A non-zero angular contraction/expansion means tests for intersection/inclusion can return
		 * false positives but in most situations this is desirable because then a more accurate test
		 * will reveal there's no intersection/inclusion - whereas the other way around can give the wrong result.
		 *
		 * Note that if the sum of the angle of outer bounding small circle and @a outer_bound_angular_expansion
		 * exceeds PI then an outer small circle with radius PI (ie, covering entire globe) is returned.
		 * And if @a inner_bound_angular_contraction is greater than the angle of inner bounding small circle
		 * then an inner small circle with radius zero is returned.
		 *
		 * Logs a warning and return inner/outer bounds of zero if no @a add overloads have been called so far.
		 */
		InnerOuterBoundingSmallCircle
		get_inner_outer_bounding_small_circle(
				const AngularExtent &inner_bound_angular_contraction = get_default_angular_expansion_contraction(),
				const AngularExtent &outer_bound_angular_expansion = get_default_angular_expansion_contraction()) const;

	private:
		UnitVector3D d_small_circle_centre;
		AngularDistance d_minimum_distance;
		AngularDistance d_maximum_distance;

	public:
		/**
		 * Default angular expansion/contraction used to expand outer and contract inner returned bounding small circles.
		 *
		 * The default contraction/expansion matches the closeness threshold for intersection detection
		 * (used when determining if one geometry is close enough to be *touching* another geometry).
		 * This ensures that the inner/outer bounding small circle contracts/expands to include the
		 * *touching* region around the geometries contained within the bounded region.
		 */
		static
		const AngularExtent &
		get_default_angular_expansion_contraction()
		{
			return BoundingSmallCircleBuilder::get_default_angular_expansion();
		}
	};


	//
	// Implementation
	//


	template <typename GreatCircleArcForwardIter>
	BoundingSmallCircle::Result
	BoundingSmallCircle::test(
			GreatCircleArcForwardIter great_circle_arc_begin,
			GreatCircleArcForwardIter great_circle_arc_end) const
	{
		// There's actually no great circle arcs to test.
		if (great_circle_arc_begin == great_circle_arc_end)
		{
			return OUTSIDE_BOUNDS;
		}

		const GreatCircleArc &first_gca = *great_circle_arc_begin;

		const AngularDistance min_distance_to_first_gca = minimum_distance(d_small_circle_centre, first_gca);
		if (min_distance_to_first_gca.is_precisely_greater_than(d_angular_extent))
		{
			// The first GCA is fully outside the bound.
			// Since the client has promised that the remaining GCA's are sequentially
			// connected, we only need to test for intersection.
			GreatCircleArcForwardIter great_circle_arc_iter = great_circle_arc_begin;
			for ( ++great_circle_arc_iter;
				great_circle_arc_iter != great_circle_arc_end;
				++great_circle_arc_iter)
			{
				const GreatCircleArc &gca = *great_circle_arc_iter;

				const AngularDistance min_gca_distance = minimum_distance(d_small_circle_centre, gca);

				if (min_gca_distance.is_precisely_less_than(d_angular_extent))
				{
					return INTERSECTING_BOUNDS;
				}
			}

			return OUTSIDE_BOUNDS;
		}

		const AngularDistance max_distance_to_first_gca = maximum_distance(d_small_circle_centre, first_gca);
		if (max_distance_to_first_gca.is_precisely_less_than(d_angular_extent))
		{
			// The first GCA is fully inside the bound.
			// Since the client has promised that the remaining GCA's are sequentially
			// connected, we only need to test for intersection.
			GreatCircleArcForwardIter great_circle_arc_iter = great_circle_arc_begin;
			for ( ++great_circle_arc_iter;
				great_circle_arc_iter != great_circle_arc_end;
				++great_circle_arc_iter)
			{
				const GreatCircleArc &gca = *great_circle_arc_iter;

				const AngularDistance max_gca_distance = maximum_distance(d_small_circle_centre, gca);

				if (max_gca_distance.is_precisely_greater_than(d_angular_extent))
				{
					return INTERSECTING_BOUNDS;
				}
			}

			return INSIDE_BOUNDS;
		}

		// The first great circle arc intersects the small circle.
		return INTERSECTING_BOUNDS;
	}


	template <typename GreatCircleArcForwardIter>
	void
	BoundingSmallCircleBuilder::add(
			GreatCircleArcForwardIter great_circle_arc_begin,
			GreatCircleArcForwardIter great_circle_arc_end)
	{
		for (GreatCircleArcForwardIter great_circle_arc_iter = great_circle_arc_begin;
			great_circle_arc_iter != great_circle_arc_end;
			++great_circle_arc_iter)
		{
			add(*great_circle_arc_iter);
		}
	}


	template <typename GreatCircleArcForwardIter>
	InnerOuterBoundingSmallCircle::Result
	InnerOuterBoundingSmallCircle::test(
			GreatCircleArcForwardIter great_circle_arc_begin,
			GreatCircleArcForwardIter great_circle_arc_end) const
	{
		// There's actually no great circle arcs to test.
		if (great_circle_arc_begin == great_circle_arc_end)
		{
			return OUTSIDE_OUTER_BOUNDS;
		}

		const GreatCircleArc &first_gca = *great_circle_arc_begin;

		const AngularDistance min_distance_to_first_gca = minimum_distance(d_outer_small_circle.d_small_circle_centre, first_gca);
		if (min_distance_to_first_gca.is_precisely_greater_than(d_outer_small_circle.d_angular_extent))
		{
			// The first GCA is fully outside the outer bound.
			// Since the client has promised that the remaining GCA's are sequentially
			// connected, we only need to test for intersection.
			GreatCircleArcForwardIter great_circle_arc_iter = great_circle_arc_begin;
			for ( ++great_circle_arc_iter;
				great_circle_arc_iter != great_circle_arc_end;
				++great_circle_arc_iter)
			{
				const GreatCircleArc &gca = *great_circle_arc_iter;

				const AngularDistance min_gca_distance =
						minimum_distance(d_outer_small_circle.d_small_circle_centre, gca);

				if (min_gca_distance.is_precisely_less_than(d_outer_small_circle.d_angular_extent))
				{
					return INTERSECTING_BOUNDS;
				}
			}

			return OUTSIDE_OUTER_BOUNDS;
		}

		const AngularDistance max_distance_to_first_gca = maximum_distance(d_outer_small_circle.d_small_circle_centre, first_gca);
		if (max_distance_to_first_gca.is_precisely_less_than(d_inner_angular_extent))
		{
			// The first GCA is fully inside the inner bound.
			// Since the client has promised that the remaining GCA's are sequentially
			// connected, we only need to test for intersection.
			GreatCircleArcForwardIter great_circle_arc_iter = great_circle_arc_begin;
			for ( ++great_circle_arc_iter;
				great_circle_arc_iter != great_circle_arc_end;
				++great_circle_arc_iter)
			{
				const GreatCircleArc &gca = *great_circle_arc_iter;

				const AngularDistance max_gca_distance =
						maximum_distance(d_outer_small_circle.d_small_circle_centre, gca);

				if (max_gca_distance.is_precisely_greater_than(d_inner_angular_extent))
				{
					return INTERSECTING_BOUNDS;
				}
			}

			return INSIDE_INNER_BOUNDS;
		}

		// If any great circle arcs intersect then the entire sequence intersects.
		return INTERSECTING_BOUNDS;
	}


	template <typename GreatCircleArcForwardIter>
	void
	InnerOuterBoundingSmallCircleBuilder::add(
			GreatCircleArcForwardIter great_circle_arc_begin,
			GreatCircleArcForwardIter great_circle_arc_end)
	{
		for (GreatCircleArcForwardIter great_circle_arc_iter = great_circle_arc_begin;
			great_circle_arc_iter != great_circle_arc_end;
			++great_circle_arc_iter)
		{
			add(*great_circle_arc_iter);
		}
	}
}

#endif // GPLATES_MATHS_SMALLCIRCLEBOUNDS_H
