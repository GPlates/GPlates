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
 *   Hamish Law <hlaw@geosci.usyd.edu.au>
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GUI_GLOBE_H_
#define _GPLATES_GUI_GLOBE_H_

#include "OpenGL.h"
#include "Colour.h"
#include "OpaqueSphere.h"
#include "SphericalGrid.h"
#include "geo/DataGroup.h"

namespace GPlatesGui
{
	class Globe
	{
		public:
			Globe()
			 : _meridian(0.0), _elevation(0.0),
			   _sphere(Colour(0.35, 0.35, 0.35)),
			   _grid(NUM_CIRCLES_LAT, NUM_CIRCLES_LON,
			    Colour::WHITE) {  }

			~Globe() {  }

			GLfloat&
			GetMeridian() { return _meridian; }

			GLfloat&
			GetElevation() { return _elevation; }

			// currently does nothing.
			void
			SetTransparency(bool trans = true) {  }

			void Paint();

		private:
			/**
			 * One circle of latitude every 30 degrees.
			 */
			static const unsigned NUM_CIRCLES_LAT = 5;

			/**
			 * One circle of longitude every 30 degrees.
			 */
			static const unsigned NUM_CIRCLES_LON = 6;

			GLfloat _meridian;
			GLfloat _elevation;

			/**
			 * The solid earth.
			 */
			OpaqueSphere _sphere;

			/**
			 * Lines of lat and lon on surface of earth.
			 */
			SphericalGrid _grid;

			void NormaliseMeridianElevation();
	};
}

#endif  /* _GPLATES_GUI_GLOBE_H_ */
