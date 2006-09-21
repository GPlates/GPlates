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
 *
 */

#ifndef _GPLATES_MATHS_GRIDONSPHERE_H_
#define _GPLATES_MATHS_GRIDONSPHERE_H_

#include "global/types.h"
#include "GreatCircle.h"
#include "SmallCircle.h"
#include "PointOnSphere.h"
#include "types.h"

namespace GPlatesMaths
{
	using GPlatesGlobal::index_t;

	/** 
	 * Represents a grid of points on the surface of a sphere. 
	 *
	 * Similarly to the classes 'PointOnSphere' and 'PolylineOnSphere',
	 * this class deals only with geographical positions, not geo-data;
	 * in contrast to its aforementioned siblings, this class does not
	 * actually <em>store</em> geographical data: rather, it acts as a
	 * template, storing the information which allows it to calculate
	 * where a particular grid element will be located. The data is managed
	 * by the \ref GPlatesGeo::GridData class.
	 *
	 * We define a grid by specifying a \ref GPlatesMaths::SmallCircle (SC)
	 * and a \ref GPlatesMaths::GreatCircle (GC). The SC is to be
	 * coincident with the latitudinal border of the grid that is closer to
	 * to the equator, whilst the GC is to be coincident with one
	 * longitudinal border of the grid. In the case of grids that straddle
	 * the equator either latitudinal border can be chosen for the SC.
	 *
	 * Instead of actually passing a SC and GC to the GridOnSphere
	 * constructor, we pass three \ref GPlatesMaths::PointOnSphere objects
	 * that define the origin of the grid, the first grid point along the
	 * SC, and the first grid point along the GC.
	 *
	 * The <strong>origin</strong> of the grid is defined to be the
	 * anterior intersection of the SC and the GC.
	 *
	 * @image html fig_grid.png
	 * @image latex fig_grid.eps
	 */
	class GridOnSphere
	{
		public:
			/**
			 * Call this "factory method" to create a GridOnSphere
			 * instance from three grid-points.
			 *
			 * This is the function you are expected to use
			 * when you create a GridOnSphere instance after 
			 * parsing a grid file.  It takes three grid-points
			 * and calculates the rest for you.  It also performs
			 * detailed error-checking.
			 *
			 * @param origin The point of origin of the grid.
			 * @param next_along_lat The next point along the
			 *   line-of-latitude from the origin.
			 * @param next_along_lon The next point along the
			 *   line-of-longitude from the origin.
			 */
			static GridOnSphere
			Create(const PointOnSphere &origin,
			       const PointOnSphere &next_along_lat,
			       const PointOnSphere &next_along_lon);


			/**
			 * Create a GridOnSphere instance, passing it all
			 * of its member data at once.
			 *
			 * In general, this constructor should only be called
			 * by the finite rotation functions.
			 */
			GridOnSphere(const SmallCircle &line_of_lat,
			             const GreatCircle &line_of_lon,
			             const PointOnSphere &orig,
			             const real_t &delta_along_lat,
			             const real_t &delta_along_lon) :

			 _line_of_lat(line_of_lat),
			 _line_of_lon(line_of_lon),
			 _origin(orig),
			 _delta_along_lat(delta_along_lat),
			 _delta_along_lon(delta_along_lon) {

				AssertInvariantHolds();
			}


			SmallCircle
			lineOfLat() const { return _line_of_lat; }


			GreatCircle
			lineOfLon() const { return _line_of_lon; }


			PointOnSphere
			origin() const { return _origin; }


			real_t
			deltaAlongLat() const { return _delta_along_lat; }


			real_t
			deltaAlongLon() const { return _delta_along_lon; }


			/**
			 * Return the point-on-sphere corresponding to the
			 * grid indices @a x and @a y.
			 */
			PointOnSphere resolve(index_t x, index_t y) const;

		protected:
			/**
			 * Assert the class invariant: that the great circle
			 * is perpendicular to the small circle, and the
			 * origin lies at their intersection.
			 */
			void AssertInvariantHolds() const;

		private:
			SmallCircle _line_of_lat;
			GreatCircle _line_of_lon;

			PointOnSphere _origin;

			/// Angular delta along the line of lat
			real_t _delta_along_lat;

			/// Angular delta along the line of lon
			real_t _delta_along_lon;


			/**
			 * Ensure the origin of the grid does not lie on either
			 * the North or South pole.
			 *
			 * This should only be invoked by the 'Create' function.
			 * @param o The origin.
			 */
			static void EnsureValidOrigin(const PointOnSphere &o);


			/**
			 * Given the unit-vector of the origin of the grid,
			 * the unit-vector of the the next point along the
			 * line-of-latitude from the origin, and the
			 * unit-vector of the North pole, calculate the
			 * angular delta (in radians) from the origin to the
			 * next point.
			 *
			 * This angular delta should be the angle of rotation
			 * about the North pole which would rotate the origin
			 * into the next point.
			 *
			 * @param orig The unit-vector of the origin.
			 * @param next The unit-vector of the next point along.
			 * @param north The unit-vector of the North pole.
			 *
			 * A special function is required for the line-of-lat
			 * (as opposed to the line-of-lon) because @a orig
			 * and @a next are not necessarily orthogonal to
			 * @a north.
			 */
			static real_t
			calcDeltaAlongLat(const UnitVector3D &orig,
			                  const UnitVector3D &next,
			                  const UnitVector3D &north);


			/**
			 * Given two unit-vectors @a orig and @a next,
			 * as well as the unit-vector of an axis, calculate
			 * the angular delta (in radians) about the axis
			 * from @a orig to @a next.
			 */
			static real_t
			calcDelta(const UnitVector3D &orig,
			          const UnitVector3D &next,
			          const UnitVector3D &axis);
	};
}

#endif  // _GPLATES_MATHS_GRIDONSPHERE_H_
