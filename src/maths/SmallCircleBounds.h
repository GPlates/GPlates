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

#include "types.h"
#include "UnitVector3D.h"

#include "MultiPointOnSphere.h"
#include "PointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"


namespace GPlatesMaths
{
	class Rotation;
	class FiniteRotation;

	/**
	 * Returns the minimum and maximum dot product of a point and a great circle arc.
	 *
	 * FIXME: Refactor this into "SphericalDistance.h".
	 */
	std::pair<real_t/*minimum dot product*/, real_t/*maximum dot product*/>
	get_min_max_dot_product(
			const UnitVector3D &small_circle_centre,
			const GreatCircleArc &gca);


	/**
	 * Updates the minimum and maximum dot product of a point and a great circle arc.
	 *
	 * NOTE: You must have initialised @a min_dot_product and @a max_dot_product before calling.
	 *
	 * FIXME: Refactor this into "SphericalDistance.h".
	 */
	void
	update_min_max_dot_product(
			const UnitVector3D &small_circle_centre,
			const GreatCircleArc &gca,
			double &min_dot_product,
			double &max_dot_product);


	/**
	 * Returns the minimum dot product of a point and a great circle arc.
	 *
	 * FIXME: Refactor this into "SphericalDistance.h".
	 */
	real_t
	get_min_dot_product(
			const UnitVector3D &small_circle_centre,
			const GreatCircleArc &gca);


	/**
	 * Updates the minimum dot product of a point and a great circle arc.
	 *
	 * NOTE: You must have initialised @a min_dot_product before calling.
	 *
	 * FIXME: Refactor this into "SphericalDistance.h".
	 */
	void
	update_min_dot_product(
			const UnitVector3D &small_circle_centre,
			const GreatCircleArc &gca,
			double &min_dot_product);


	/**
	 * Returns the maximum dot product of a point and a great circle arc.
	 *
	 * FIXME: Refactor this into "SphericalDistance.h".
	 */
	real_t
	get_max_dot_product(
			const UnitVector3D &small_circle_centre,
			const GreatCircleArc &gca);


	/**
	 * Updates the maximum dot product of a point and a great circle arc.
	 *
	 * NOTE: You must have initialised @a max_dot_product before calling.
	 *
	 * FIXME: Refactor this into "SphericalDistance.h".
	 */
	void
	update_max_dot_product(
			const UnitVector3D &small_circle_centre,
			const GreatCircleArc &gca,
			double &max_dot_product);


	/**
	 * A small circle that encloses a region.
	 */
	class BoundingSmallCircle
	{
	public:
		/**
		 * Use @a BoundingSmallCircleBuilder to construct this.
		 */
		BoundingSmallCircle(
				const UnitVector3D &small_circle_centre,
				const double &min_dot_product);

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
		 * NOTE: The sequence of great circle arcs must form an continuous connected sequence -
		 * in other words the end of one arc must be the beginning of the next, etc, as is
		 * the case for a subsection of a polygon or polyline.
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
		 * Note that, like a polyline, this only tests if the boundary of the polygon
		 * intersects the bounding small circle.
		 */
		Result
		test(
				const PolygonOnSphere &polygon) const
		{
			return test(polygon.begin(), polygon.end());
		}


		/**
		 * Test a filled polygon against the bounding region.
		 *
		 * This test differs from the regular polygon test in that we are not just
		 * testing if the boundary of the polygon intersects the bounding region
		 * but also if the interior of the polygon intersects the bounding region.
		 *
		 * If the polygon boundary is completely outside the small circle *and*
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
		 * Returns the cosine of any part of the small circle boundary with the small circle centre.
		 *
		 * This gives a measure of the minimum dot product of any part of any geometry,
		 * bounded by this small circle, with the small circle centre.
		 */
		const double &
		get_small_circle_boundary_cosine() const
		{
			return d_min_dot_product;
		}


		/**
		 * Returns the sine of any part of the small circle boundary with the small circle centre.
		 *
		 * This gives a measure of the magnitude of the cross product of any point *on* the
		 * bounding small circle with the small circle centre.
		 */
		const double &
		get_small_circle_boundary_sine() const
		{
			// Since we're calculating the square root we'll cache the value to
			// avoid recalculating every time it's queried.
			if (!d_magnitude_boundary_cross_product)
			{
				const double cosine_square = d_min_dot_product * d_min_dot_product;
				if (cosine_square < 1)
				{
					d_magnitude_boundary_cross_product = std::sqrt(1 - cosine_square);
				}
				else
				{
					d_magnitude_boundary_cross_product = 0;
				}
			}

			return d_magnitude_boundary_cross_product.get();
		}

	private:
		UnitVector3D d_small_circle_centre;
		double d_min_dot_product;
		mutable boost::optional<double> d_magnitude_boundary_cross_product;


		/**
		 * Private constructor only used by @a InnerOuterBoundingSmallCircle.
		 */
		BoundingSmallCircle(
				const UnitVector3D &small_circle_centre,
				const double &min_dot_product,
				const boost::optional<double> &magnitude_boundary_cross_product);

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


	namespace SmallCircleBoundsImpl
	{
		bool
		intersect(
				const BoundingSmallCircle &bounding_small_circle_1,
				const BoundingSmallCircle &bounding_small_circle_2,
				const double &dot_product_circle_centres);
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
			update_min_dot_product(d_small_circle_centre, gca, d_min_dot_product);
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
		 * Adds the great circle arcs in a polygon - the inner/outer bounds are contracted/expanded
		 * if necessary to bound the polygon.
		 */
		void
		add(
				const PolygonOnSphere &polygon)
		{
			add(polygon.begin(), polygon.end());
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
		 * Returns the bounding small circle of all primitives added so far.
		 *
		 * @a expand_bound_delta_dot_product is used to expand the bound to
		 * account for numerical precision (a negative value can be used to contract instead).
		 *
		 * This means tests for intersection/inclusion can return false positives but in most
		 * situations this is desirable because then a more accurate test will reveal there's
		 * no intersection/inclusion - whereas the other way around can give the wrong result.
		 *
		 * A value of 1e-6 means a bounding small circle of zero radius will be expanded to ~9km (on earth's surface).
		 */
		BoundingSmallCircle
		get_bounding_small_circle(
				const double &expand_bound_delta_dot_product = 1e-6) const;

	private:
		UnitVector3D d_small_circle_centre;
		double d_min_dot_product;
	};


	/**
	 * Two concentric small circles that enclose a region.
	 */
	class InnerOuterBoundingSmallCircle
	{
	public:
		/**
		 * Use @a InnerOuterBoundingSmallCircleBuilder to construct this.
		 */
		InnerOuterBoundingSmallCircle(
				const UnitVector3D &small_circle_centre,
				const double &min_dot_product,
				const double &max_dot_product);

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
		 * NOTE: The sequence of great circle arcs must form an continuous connected sequence -
		 * in other words the end of one arc must be the beginning of the next, etc, as is
		 * the case for a subsection of a polygon or polyline.
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


		//! Test a polygon against the bounding region.
		Result
		test(
				const PolygonOnSphere &polygon) const
		{
			return test(polygon.begin(), polygon.end());
		}


		/**
		 * Test a filled polygon against the bounding region.
		 *
		 * This test differs from the regular polygon test in that we are not just
		 * testing if the boundary of the polygon intersects the bounding region
		 * but also if the interior of the polygon intersects the bounding region.
		 *
		 * If the polygon boundary is completely outside the outer small circle *and*
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
			return d_outer_small_circle.d_small_circle_centre;
		}


		/**
		 * Returns the cosine of any part of the inner small circle boundary with the small circle centre.
		 *
		 * This gives a measure of the maximum dot product of any part of any geometry,
		 * bounded by this small circle, with the small circle centre.
		 */
		const double &
		get_inner_small_circle_boundary_cosine() const
		{
			return d_max_dot_product;
		}


		/**
		 * Returns the sine of any part of the inner small circle boundary with the small circle centre.
		 *
		 * This gives a measure of the magnitude of the cross product of any point *on* the
		 * inner bounding small circle with the small circle centre.
		 */
		const double &
		get_inner_small_circle_boundary_sine() const
		{
			// Since we're calculating the square root we'll cache the value to
			// avoid recalculating every time it's queried.
			if (!d_magnitude_inner_boundary_cross_product)
			{
				const double cosine_square = d_max_dot_product * d_max_dot_product;
				if (cosine_square < 1)
				{
					d_magnitude_inner_boundary_cross_product = std::sqrt(1 - cosine_square);
				}
				else
				{
					d_magnitude_inner_boundary_cross_product = 0;
				}
			}

			return d_magnitude_inner_boundary_cross_product.get();
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
		double d_max_dot_product;
		mutable boost::optional<double> d_magnitude_inner_boundary_cross_product;
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
				const BoundingSmallCircle &bounding_small_circle,
				const double &dot_product_circle_centres);

		bool
		is_inside_inner_bounding_small_circle(
				const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle,
				const BoundingSmallCircle &bounding_small_circle,
				const double &dot_product_circle_centres);

		bool
		intersect(
				const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle_1,
				const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle_2,
				const double &dot_product_circle_centres);

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
			const BoundingSmallCircle &bounding_small_circle,
			const InnerOuterBoundingSmallCircle &inner_outer_bounding_small_circle)
	{
		return intersect(inner_outer_bounding_small_circle, bounding_small_circle);
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
			update_min_max_dot_product(
					d_small_circle_centre, gca, d_min_dot_product, d_max_dot_product);
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
		 */
		void
		add(
				const PolygonOnSphere &polygon)
		{
			add(polygon.begin(), polygon.end());
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
		 * @a contract_inner_bound_delta_dot_product is used to contract the *inner* bound to
		 * account for numerical precision (a negative value can be used to expand instead).
		 * @a expand_outer_bound_delta_dot_product is used to expand the *outer* bound to
		 * account for numerical precision (a negative value can be used to contract instead).
		 *
		 * This means tests for intersection/inclusion can return false positives but in most
		 * situations this is desirable because then a more accurate test will reveal there's
		 * no intersection/inclusion - whereas the other way around can give the wrong result.
		 *
		 * @throws @a PreconditionViolationError if no @a add overloads have been called so far.
		 */
		InnerOuterBoundingSmallCircle
		get_inner_outer_bounding_small_circle(
				const double &contract_inner_bound_delta_dot_product = 1e-6,
				const double &expand_outer_bound_delta_dot_product = 1e-6) const;

	private:
		UnitVector3D d_small_circle_centre;
		double d_min_dot_product;
		double d_max_dot_product;
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

		double gca_min_dot_product = 1.0;
		double gca_max_dot_product = -1.0;

		const GreatCircleArc &first_gca = *great_circle_arc_begin;

		update_min_max_dot_product(
				d_small_circle_centre, first_gca, gca_min_dot_product, gca_max_dot_product);

		if (gca_max_dot_product < d_min_dot_product)
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

				update_max_dot_product(d_small_circle_centre, gca, gca_max_dot_product);

				if (gca_max_dot_product >= d_min_dot_product)
				{
					return INTERSECTING_BOUNDS;
				}
			}

			return OUTSIDE_BOUNDS;
		}

		if (gca_min_dot_product > d_min_dot_product)
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

				update_min_dot_product(d_small_circle_centre, gca, gca_min_dot_product);

				if (gca_min_dot_product <= d_min_dot_product)
				{
					return INTERSECTING_BOUNDS;
				}
			}

			return INSIDE_BOUNDS;
		}

		// If any great circle arcs intersect the small circle then the entire sequence intersects.
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

		double gca_min_dot_product = 1.0;
		double gca_max_dot_product = -1.0;

		const GreatCircleArc &first_gca = *great_circle_arc_begin;

		update_min_max_dot_product(
				d_outer_small_circle.d_small_circle_centre, first_gca, gca_min_dot_product, gca_max_dot_product);

		if (gca_max_dot_product < d_outer_small_circle.d_min_dot_product)
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

				update_max_dot_product(d_outer_small_circle.d_small_circle_centre, gca, gca_max_dot_product);

				if (gca_max_dot_product >= d_outer_small_circle.d_min_dot_product)
				{
					return INTERSECTING_BOUNDS;
				}
			}

			return OUTSIDE_OUTER_BOUNDS;
		}

		if (gca_min_dot_product > d_max_dot_product)
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

				update_min_dot_product(d_outer_small_circle.d_small_circle_centre, gca, gca_min_dot_product);

				if (gca_min_dot_product <= d_max_dot_product)
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


	inline
	std::pair<real_t/*minimum dot product*/, real_t/*maximum dot product*/>
	get_min_max_dot_product(
			const UnitVector3D &small_circle_centre,
			const GreatCircleArc &gca)
	{
		double min_dot_product = 1.0;
		double max_dot_product = -1.0;

		update_min_max_dot_product(small_circle_centre, gca, min_dot_product, max_dot_product);

		return std::make_pair(min_dot_product, max_dot_product);
	}


	inline
	real_t
	get_min_dot_product(
			const UnitVector3D &small_circle_centre,
			const GreatCircleArc &gca)
	{
		double min_dot_product = 1.0;

		update_min_dot_product(small_circle_centre, gca, min_dot_product);

		return min_dot_product;
	}


	inline
	real_t
	get_max_dot_product(
			const UnitVector3D &small_circle_centre,
			const GreatCircleArc &gca)
	{
		double max_dot_product = -1.0;

		update_max_dot_product(small_circle_centre, gca, max_dot_product);

		return max_dot_product;
	}
}

#endif // GPLATES_MATHS_SMALLCIRCLEBOUNDS_H
