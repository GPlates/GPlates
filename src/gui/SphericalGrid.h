/* $Id$ */

/**
 * @file 
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
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_SPHERICALGRID_H_
#define _GPLATES_SPHERICALGRID_H_

#include "OpenGL.h"
#include "Colour.h"
#include "maths/types.h"

namespace GPlatesGui
{
	class SphericalGrid
	{
		public:
			SphericalGrid(const GPlatesMaths::real_t& degrees_per_lat,
			              const GPlatesMaths::real_t& degrees_per_lon,
			              const Colour& colour = Colour::WHITE);
			
			void
			Paint();

		private:
			/**
			 * Magic number to refer to the grid's display
			 * list.
			 * XXX: we need to implement a mechanism by which
			 * we can guarantee that this number is unique among
			 * all the display lists.
			 */ 
			static const int GRID = 42;
	};
}

#endif /* _GPLATES_SPHERICALGRID_H_ */
