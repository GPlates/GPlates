/* $Id$ */

/**
 * @file 
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

#ifndef _GPLATES_GUI_SPHERICALGRID_H_
#define _GPLATES_GUI_SPHERICALGRID_H_

#include "Colour.h"
#include "NurbsRenderer.h"

namespace GPlatesGui
{
	class SphericalGrid
	{
		public:
			SphericalGrid(unsigned num_circles_lat,
			              unsigned num_circles_lon,
			              const Colour &colour);

			~SphericalGrid() {  }

		private:
			/*
			 * These two member functions intentionally declared
			 * private to avoid object copying/assignment.
			 * [Since the data member '_nurbs' itself cannot be
			 * copied or assigned.]
			 */
			SphericalGrid(const SphericalGrid &other);

			SphericalGrid &operator=(const SphericalGrid &other);

		public:
			void Paint();

		private:
			void drawLineOfLat(double lat);  // in radians
			void drawLineOfLon(double lon);

			NurbsRenderer _nurbs;

			unsigned _num_circles_lat;
			unsigned _num_circles_lon;

			Colour _colour;

			double _lat_delta;  // in radians
			double _lon_delta;

			/**
			 * FIXME: Pi is often defined in the C Standard Library
			 * -- but is it <em>always</em> defined on
			 * <em>every</em> platform? and to the <em>same
			 * precision</em>?
			 */
			static const double pi;

#if 0  // FIXME: implement display lists
			/**
			 * Magic number to refer to the grid's display list.
			 * XXX: we need to implement a mechanism by which
			 * we can guarantee that this number is unique among
			 * all the display lists.
			 */ 
			static const int GRID = 42;
#endif
	};
}

#endif /* _GPLATES_GUI_SPHERICALGRID_H_ */
