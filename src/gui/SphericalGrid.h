/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_SPHERICALGRID_H
#define GPLATES_GUI_SPHERICALGRID_H

#include "NurbsRenderer.h"
#include "utils/VirtualProxy.h"


namespace GPlatesGui
{
	class Colour;


	class SphericalGrid
	{
	public:
		SphericalGrid(
				unsigned num_circles_lat,
				unsigned num_circles_lon);

		~SphericalGrid()
		{  }

		void
		paint(
				const Colour &colour);

		/**
		 * Paint the circumference for vector output.
		 */
		void
		paint_circumference(
				const Colour &colour);

	private:
		// Delay creation of NurbsRenderer object until it's used.
		GPlatesUtils::VirtualProxy<NurbsRenderer> d_nurbs;

		unsigned d_num_circles_lat;
		unsigned d_num_circles_lon;

		/**
		 * The delta (in radians) between adjacent circles of latitude.
		 */
		double d_lat_delta;

		/**
		 * The delta (in radians) between adjacent circles of latitude.
		 */
		double d_lon_delta;

		/**
		 * The value of pi.
		 *
		 * FIXME:  Pi is often defined in the C Standard Library -- but is it @em always
		 * defined on @em every platform? and to the @em same precision?
		 */
		static const double s_pi;

		/**
		 * Draw a line of latitude for latitude @a lat.
		 * The angle is in radians.
		 */
		void draw_line_of_lat(
				const double &lat);

		/**
		 * Draw a line of longitude for longitude @a lon.
		 * The angle is in radians.
		 */
		void draw_line_of_lon(
				const double &lon);

		/*
		 * These two member functions are intentionally declared
		 * private to avoid object copying/assignment.
		 *
		 * (Since the data member 'd_nurbs' itself cannot be
		 * copied or assigned.)
		 */

		SphericalGrid(
				const SphericalGrid &other);

		SphericalGrid &
		operator=(
				const SphericalGrid &other);

	};
}

#endif  // GPLATES_GUI_SPHERICALGRID_H
