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
 */

#ifndef _GPLATES_GUI_GLOBE_H_
#define _GPLATES_GUI_GLOBE_H_

#include "OpenGL.h"
#include "Colour.h"
#include "SphericalGrid.h"
#include "geo/DataGroup.h"

namespace GPlatesGui
{
	class Globe
	{
		public:
			Globe(Colour  colour = DEFAULT_COLOUR,
				  GLfloat radius = DEFAULT_RADIUS,
				  GLint   slices = DEFAULT_SLICES,
				  GLint   stacks = DEFAULT_STACKS);

			~Globe() { 
				
				gluDeleteQuadric(_sphere);
			}

			GLfloat&
			GetMeridian() { return _meridian; }

			GLfloat&
			GetElevation() { return _elevation; }

			void
			SetTransparency(bool trans = true);

			void
			Paint();

		private:
			/**
			 * Globe is initially white.
			 */
			static const Colour DEFAULT_COLOUR;
			
			/**
			 * Unit radius to ease calculations and display of data.
			 */
			static const GLfloat DEFAULT_RADIUS;

			/**
			 * One slice every 30 degrees.
			 */
			static const GLint DEFAULT_SLICES = 12;

			/**
			 * One stack every 30 degrees.
			 */
			static const GLint DEFAULT_STACKS = 6;

			Colour _colour;

			/**
			 * The Globe's radius will almost always be one.
			 */
			GLfloat _radius;

			GLfloat _meridian;
			GLfloat _elevation;

			GLUquadricObj *_sphere;	/*< The solid earth. */
			SphericalGrid  _grid;	/*< Lines of lat and lon on surface. */ 

			void
			NormaliseMeridianElevation();
	};
}

#endif  /* _GPLATES_GUI_GLOBE_H_ */
