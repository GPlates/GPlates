/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
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
 * Authors:
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 *   Dave Symonds <ds@geosci.usyd.edu.au>
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
	 * Similarly to the classes 'PointOnSphere' and 'PolyLineOnSphere',
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
			GridOnSphere(const PointOnSphere &origin,
			             const PointOnSphere &next_along_lat,
			             const PointOnSphere &next_along_lon);

			PointOnSphere resolve(index_t x, index_t y) const;

		protected:
			void AssertInvariantHolds () const;

		private:
			SmallCircle _line_of_lat;
			GreatCircle _line_of_lon;

			PointOnSphere _origin;

			real_t _delta_along_lat;
			real_t _delta_along_lon;
	};
}

#endif  // _GPLATES_MATHS_GRIDONSPHERE_H_
