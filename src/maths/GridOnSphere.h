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
	/** 
	 * Represents a grid of points on the surface of a sphere. 
	 */
	class GridOnSphere
	{
		public:
			using GPlatesGlobal::index_t;

			GridOnSphere(const GreatCircle &gc,
			             const SmallCircle &sc,
			             real_t delta_along_lat,
			             real_t delta_along_lon);

			PointOnSphere
			resolve(index_t x, index_t y) const;

		private:
			GreatCircle _line_of_lon;
			SmallCircle _line_of_lat;

			PointOnSphere _origin;

			real_t _delta_on_lat;
			real_t _delta_on_lon;
	};
}

#endif  // _GPLATES_MATHS_GRIDONSPHERE_H_
