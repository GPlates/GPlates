/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_POINTINPOLYGON_H
#define GPLATES_MATHS_POINTINPOLYGON_H

#include <vector>
#include <boost/shared_ptr.hpp>

#include "UnitVector3D.h"

#include "global/PointerTraits.h"


namespace GPlatesMaths
{
	class PointOnSphere;
	class PolygonOnSphere;

	namespace PointInPolygon
	{
		class SphericalLuneTree;

		/**
		 * Some profile timings (2026, MSVC 2022, a polygon of 1024 edges and 0.2 radians radius, in
		 * microseconds) for test points uniformly over the globe, near the polygon, and near its
		 * outline (in that order):
		 *
		 * 1) non-member is_point_in_polygon():  5.6 / 4.6 / 3.4 per point
		 *    (and the polygon's centroid and outside point, once, about 25)
		 *
		 * 2) build_ologn_hint = false:
		 *      Polygon constructor:             84
		 *      Polygon::is_point_in_polygon():  0.03 / 0.69 / 3.3
		 *
		 * 3) build_ologn_hint = true:
		 *      Polygon constructor:             150
		 *      Polygon::is_point_in_polygon():  0.004 / 0.03 / 0.14
		 *
		 * The constructors cost about 15 calls of (1) and the queries of (2) scale with the number
		 * of edges, as do those of (1), at every size measured (4 to 4096 edges). So use:
		 * 1) fewer than about 15 points tested per polygon,
		 * 2) more points, if most are far from the polygon's outline (where its bounds decide),
		 * 3) more points otherwise, or many more points (thousands, even far from the outline).
		 *
		 * When the number of test points is not known use (3), or PolygonOnSphere's adaptive mode.
		 */

		/**
		 * Tests if @a point is inside @a polygon which can be a non-simple (eg, self-intersecting) polygon.
		 *
		 * The point-in-polygon test is done by counting the number of polygon edges crossed (in the
		 * exterior ring and any interior rings) from a point outside the polygon (see
		 * PolygonOnSphere::get_outside_point, and PointInPolygon::InteriorReference for how each
		 * ring's interior is defined) to the point being tested. So if an interior ring intersects
		 * the exterior ring then it is possible for a test point, that is outside the exterior ring
		 * but inside the interior ring, to pass the point-in-polygon test. This might not be
		 * desirable but it is similar to how filled polygons are currently rendered and so at least
		 * there is some consistency there (and reconstructed rasters use the point-in-polygon test
		 * when generating a polygon mesh so it's consistent too).
		 *
		 * This is useful *only* when testing fewer than a handful of points against a specific polygon
		 * (determined by profiling). Unless you know this for sure it is best to use
		 * class @a Polygon instead.
		 *
		 * It is O(n) in the number of polygon edges and has no bounds testing but it requires less
		 * preprocessing than @a Polygon but this is only beneficial if a handful or fewer points
		 * are tested against a specific polygon.
		 *
		 * If @a use_point_on_polygon_threshold is true then the point is considered *on* the outline
		 * (and hence classified as *inside* the polygon) if it is within an (extremely small) threshold
		 * distance from the polygon's outline (about 1.4e-6 radians, or 9 metres, from any edge or
		 * vertex, at every speed). An example where this is useful is avoiding a point
		 * exactly on the dateline not getting included by a polygon that has an edge exactly aligned
		 * with the dateline.
		 * So usually this should be true (the default) and only turned off for some specific cases
		 * (such as the polyline intersections code where a point extremely close to a polygon,
		 * but still outside it, should not be considered inside it).
		 */
		bool
		is_point_in_polygon(
				const PointOnSphere &point,
				const PolygonOnSphere &polygon,
				bool use_point_on_polygon_threshold = true);


		/**
		 * A data structure, wrapped around a @a PolygonOnSphere, that optimises
		 * point-in-polygon tests and works with non-simple (eg, self-intersecting) polygons.
		 *
		 * Use this class if you test more than a handful of points against the same polygon
		 * (determined by profiling).
		 * In this case it should be *significantly* more efficient than using
		 * the non-member function @a is_point_in_polygon.
		 */
		class Polygon
		{
		public:
			/**
			 * Constructor - @a polygon can be non-simple (eg, self-intersecting).
			 *
			 * If @a build_ologn_hint is true then an O(log(N)) spherical lune tree
			 * (in the number of polygon edges N) will be built provided there are enough edges
			 * in the polygon to make it worthwhile.
			 *
			 * If you know you are testing less than *200* points against a specific polygon
			 * then set @a build_ologn_hint to 'false'. Otherwise it is safest to
			 * set @a build_ologn_hint to 'true' as it will be significantly faster for
			 * larger numbers of points tested against a specific polygon.
			 *
			 * There is early bounds testing regardless of the value of @a build_ologn_hint
			 * which (in addition to caching any pre-processing) accounts for the significant
			 * gains over the non-member point-in-polygon function.
			 *
			 * If @a keep_shared_reference_to_polygon is true then a shared pointer to @a polygon
			 * is kept internally in order to ensure the polygon is alive when testing a point
			 * against it. This is set to false by PolygonOnSphere itself since it has a
			 * shared pointer to us (otherwise we'd get a memory island and hence a memory leak).
			 */
			explicit
			Polygon(
					const GPlatesGlobal::PointerTraits<const PolygonOnSphere>::non_null_ptr_type &polygon,
					bool build_ologn_hint = true,
					bool keep_shared_reference_to_polygon = true);


			/**
			 * Tests if @a point is inside the polygon passed into the constructor.
			 *
			 * The point-in-polygon test is done by counting the number of polygon edges crossed (in
			 * the exterior ring and any interior rings) from a point outside the polygon (see
			 * PolygonOnSphere::get_outside_point, and PointInPolygon::InteriorReference for how
			 * each ring's interior is defined) to the point being tested. So if an interior ring
			 * intersects the exterior ring then it is possible for a test point, that is outside
			 * the exterior ring but inside the interior ring, to pass the point-in-polygon test.
			 * This might not be desirable but it is similar to how filled polygons are currently
			 * rendered and so at least there is some consistency there (and reconstructed rasters
			 * use the point-in-polygon test when generating a polygon mesh so it's consistent too).
			 *
			 * If @a use_point_on_polygon_threshold is true then the point is considered *on* the outline
			 * (and hence classified as *inside* the polygon) if it is within an (extremely small) threshold
			 * distance from the polygon's outline (about 1.4e-6 radians, or 9 metres, from any edge
			 * or vertex, at every speed). An example where this is useful is avoiding a point
			 * exactly on the dateline not getting included by a polygon that has an edge exactly aligned
			 * with the dateline.
			 * So usually this should be true (the default) and only turned off for some specific cases
			 * (such as the polyline intersections code where a point extremely close to a polygon,
			 * but still outside it, should not be considered inside it).
			 */
			bool
			is_point_in_polygon(
					const PointOnSphere &point,
					bool use_point_on_polygon_threshold = true) const;


			/**
			 * The number of calls to @a is_point_in_polygon so far that the bounds testing did
			 * not decide (so polygon edges were tested).
			 *
			 * PolygonOnSphere's adaptive mode uses this to decide when to build the O(log(N)) tree.
			 */
			unsigned int
			get_num_queries_testing_edges() const;

		private:
			/**
			 * Bounds testing and an optional O(log(N)) tree (in the number of polygons edges N).
			 *
			 * Using a boost::shared_ptr instead of a boost::scoped_ptr to make this class copyable.
			 */
			boost::shared_ptr<SphericalLuneTree> d_spherical_lune_tree;
		};


		/**
		 * The point outside a polygon from which point-in-polygon counts crossings, and how each
		 * ring's interior was decided.
		 *
		 * A polygon's interior is the points inside an odd number of its rings (exterior and
		 * interior alike). A ring's interior is the points whose winding number differs by an odd
		 * number from the ring's mean winding number rounded to the nearest integer: its smaller
		 * side, for a ring that doesn't cross itself (ties, a ring with area within 1e-8 steradians
		 * of half the sphere, take the side to the ring's left). For a ring that crosses itself only
		 * slightly (a small twist or loop), the region round it is the outside and every lobe is
		 * inside, whichever way it is wound, as parity from outside the ring gives; no separate code
		 * path decides that. Except that a ring within the open hemisphere about the polygon's
		 * boundary centroid C keeps parity from -C, which differs only for a ring that winds round
		 * some region more than once.
		 *
		 * If every ring is within that hemisphere the outside point is -C, as it always was.
		 * Otherwise it is the candidate furthest from the outline, of -C, C and 32 fixed directions,
		 * that is outside. Its antipode is the apex of the fans that compute area and draw filled
		 * polygons, so they agree with point-in-polygon.
		 *
		 * See 'docs/design/architecture/foundation/spherical-geometry.md'.
		 */
		class InteriorReference
		{
		public:
			/**
			 * @a is_ring_in_boundary_centroid_hemisphere has the exterior ring first, and is empty
			 * if every ring is in the hemisphere (the outside point is then -C).
			 */
			InteriorReference(
					const UnitVector3D &outside_point,
					const std::vector<bool> &is_ring_in_boundary_centroid_hemisphere) :
				d_outside_point(outside_point),
				d_is_ring_in_boundary_centroid_hemisphere(is_ring_in_boundary_centroid_hemisphere)
			{  }

			//! The point (outside the polygon) from which crossings are counted.
			const UnitVector3D &
			get_outside_point() const
			{
				return d_outside_point;
			}

			/**
			 * Whether every ring is within the open hemisphere about the boundary centroid C, so
			 * the outside point is -C (and everything is calculated as it was before the rule).
			 */
			bool
			is_every_ring_in_boundary_centroid_hemisphere() const
			{
				return d_is_ring_in_boundary_centroid_hemisphere.empty();
			}

			/**
			 * Whether the exterior ring is within the open hemisphere about the boundary centroid C,
			 * so its interior is parity from -C (and its area the fan sum from C).
			 */
			bool
			is_exterior_ring_in_boundary_centroid_hemisphere() const
			{
				return is_ring_in_boundary_centroid_hemisphere(0);
			}

			//! As @a is_exterior_ring_in_boundary_centroid_hemisphere, for an interior ring.
			bool
			is_interior_ring_in_boundary_centroid_hemisphere(
					unsigned int interior_ring_index) const
			{
				return is_ring_in_boundary_centroid_hemisphere(interior_ring_index + 1);
			}

		private:
			UnitVector3D d_outside_point;
			std::vector<bool> d_is_ring_in_boundary_centroid_hemisphere;

			bool
			is_ring_in_boundary_centroid_hemisphere(
					unsigned int ring_index) const
			{
				return d_is_ring_in_boundary_centroid_hemisphere.empty() ||
						d_is_ring_in_boundary_centroid_hemisphere[ring_index];
			}
		};

		/**
		 * Calculates the @a InteriorReference of @a polygon.
		 *
		 * PolygonOnSphere::get_interior_reference() caches it, so use that instead.
		 */
		InteriorReference
		calculate_interior_reference(
				const PolygonOnSphere &polygon);


		/**
		 * A second point from which crossings can be counted, about 90 degrees from the outside
		 * point, with its status (inside or outside the polygon).
		 *
		 * A test point within about 1e-6 radians of the outside point, or of its antipode, has no
		 * well-defined crossings arc from the outside point. Its crossings are counted from here
		 * instead.
		 */
		struct SecondReferencePoint
		{
			SecondReferencePoint(
					const UnitVector3D &point_,
					bool is_in_polygon_) :
				point(point_),
				is_in_polygon(is_in_polygon_)
			{  }

			UnitVector3D point;
			bool is_in_polygon;
		};

		/**
		 * Calculates the @a SecondReferencePoint of @a polygon.
		 *
		 * PolygonOnSphere::get_second_reference_point() caches it, so use that instead.
		 */
		SecondReferencePoint
		calculate_second_reference_point(
				const PolygonOnSphere &polygon);


		/**
		 * Returns the integer offset of a ring's outside winding level, from its fan sum.
		 *
		 * @a ring_fan_signed_area is the sum of the signed areas of the triangles (A, v_i, v_i+1)
		 * over the ring's edges, from an apex A. It equals the integral of (w(x) - w(-A)) over the
		 * sphere, where w is the ring's winding number, so dividing by 4 pi and rounding gives the
		 * ring's outside winding level less w(-A). So -A is in the ring's interior if the result is
		 * odd, and the ring's signed area (its interior's) is the fan sum less 4 pi times the result.
		 *
		 * A fan sum within 1e-8 steradians of an odd multiple of 2 pi (a ring that halves the sphere)
		 * is rounded down, so the ring's interior is the side to its left.
		 */
		int
		get_ring_winding_offset(
				const double &ring_fan_signed_area);


		/**
		 * Return the cosine (dot product of two points) used for the point-ON-polygon threshold.
		 *
		 * This is only provided in case a client wants to know the threshold. For example, to move
		 * a point OFF a polygon outline such that it won't get tested as *inside* the polygon.
		 */
		double
		get_point_on_polygon_threshold_cosine();

		/**
		 * Return the sine (dot product of point and GCA plane) used for the point-ON-polygon threshold.
		 *
		 * This is only provided in case a client wants to know the threshold. For example, to move
		 * a point OFF a polygon outline such that it won't get tested as *inside* the polygon.
		 */
		double
		get_point_on_polygon_threshold_sine();
	}
}

#endif // GPLATES_MATHS_POINTINPOLYGON_H
