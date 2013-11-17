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
		 * Some profile timings (on Intel 6750 Dual-core compiled in MSVC2005) running
		 * global topological plate polygon set with 12 caps files at 33x33 resolution:
		 *
		 * 1) non-member is_point_in_polygon(): 10us
		 *
		 * 2) build_ologn_hint = false:
		 *      Polygon constructor:            32us
		 *      Polygon::is_point_in_polygon(): 0.44us
		 *
		 * 3) build_ologn_hint = true:
		 *      Polygon constructor:            114us
		 *      Polygon::is_point_in_polygon(): 0.03us
		 *
		 * According to this use:
		 * 1) 0 < N < 4     points tested per polygon,
		 * 2) 4 < N < 200   points tested per polygon,
		 * 3) N > 200       points tested per polygon.
		 *
		 * When the number of test points is not known use (3).
		 */

		/**
		 * Tests if @a point is inside @a polygon which can be a non-simple (eg, self-intersecting) polygon.
		 *
		 * This is useful *only* when testing fewer than a handful of points against a specific polygon
		 * (determined by profiling). Unless you know this for sure it is best to use
		 * class @a Polygon instead.
		 *
		 * It is O(n) in the number of polygon edges and has no bounds testing but it requires less
		 * preprocessing than @a Polygon but this is only beneficial if a handful or fewer points
		 * are tested against a specific polygon.
		 *
		 * Note that "point *on* polygon" is not available due to the use of floating-point
		 * numbers instead of exact arithmetic.
		 * In any case point-on-polygon typically isn't required - it might be required if we
		 * want to find which polygon, in a non-overlapping set, a point is contained by -
		 * in which case a point on the edge of one polygon should not be on the corresponding
		 * edge of the adjacent polygon (this could happen for the velocity calculations
		 * against dynamic plate polygons) - however this sort of situation, normally handled
		 * by edge rules and exact arithmetic, can be handled by finding the first polygon
		 * the point is in and using that polygon - to get consistent results from frame-to-frame
		 * the point-in-polygon tests could be sorted by some polygon attribute such as plate id.
		 */
		bool
		is_point_in_polygon(
				const PointOnSphere &point,
				const PolygonOnSphere &polygon);


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
			 * Note that "point *on* polygon" is not available due to the use of floating-point
			 * numbers instead of exact arithmetic.
			 * In any case point-on-polygon typically isn't required - it might be required if we
			 * want to find which polygon, in a non-overlapping set, a point is contained by -
			 * in which case a point on the edge of one polygon should not be on the corresponding
			 * edge of the adjacent polygon (this could happen for the velocity calculations
			 * against dynamic plate polygons) - however this sort of situation, normally handled
			 * by edge rules and exact arithmetic, can be handled by finding the first polygon
			 * the point is in and using that polygon - to get consistent results from frame-to-frame
			 * the point-in-polygon tests could be sorted by some polygon attribute such as plate id.
			 */
			bool
			is_point_in_polygon(
					const PointOnSphere &point) const;

		private:
			/**
			 * Bounds testing and an optional O(log(N)) tree (in the number of polygons edges N).
			 *
			 * Using a boost::shared_ptr instead of a boost::scoped_ptr to make this class copyable.
			 */
			boost::shared_ptr<SphericalLuneTree> d_spherical_lune_tree;
		};
	}
}

#endif // GPLATES_MATHS_POINTINPOLYGON_H
