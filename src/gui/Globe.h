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
 */

#ifndef _GPLATES_GUI_GLOBE_H_
#define _GPLATES_GUI_GLOBE_H_

#include "OpenGL.h"
#include "Colour.h"
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
				
				gluDeleteNurbsRenderer(_nurbs_renderer);
				gluDeleteQuadric(_sphere);
			}

			GLint&
			GetSlices() { return _slices; }

			GLint&
			GetStacks() { return _stacks; }

			GLfloat&
			GetMeridian() { return _meridian; }

			GLfloat&
			GetElevation() { return _elevation; }

			void
			Paint(const GPlatesGeo::DataGroup*);

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
			 * One slice every 5 degrees.
			 */
			static const GLint DEFAULT_SLICES = 36;

			/**
			 * One stack every 5 degrees.
			 */
			static const GLint DEFAULT_STACKS = 18;

			Colour _colour;

			/**
			 * The Globe's radius will almost always be one.
			 */
			GLfloat _radius;

			/**
			 * The stacks and slices determine the 'resolution' of 
			 * the rendered Globe.
			 */
			GLint _slices;
			GLint _stacks;

			GLUquadricObj *_sphere;
			GLUnurbsObj *_nurbs_renderer;

			GLfloat _meridian;
			GLfloat _elevation;

			void
			NormaliseMeridianElevation();
	};
}

#endif  /* _GPLATES_GUI_GLOBE_H_ */
