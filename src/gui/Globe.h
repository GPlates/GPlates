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

#include "OpenGL.h"
#include "Colour.h"

namespace GPlatesGui
{
	class Globe
	{
		public:
			Globe(Colour  colour = DEFAULT_COLOUR,
				  GLfloat radius = DEFAULT_RADIUS,
				  GLuint  slices = DEFAULT_SLICES,
				  GLuint  stacks = DEFAULT_STACKS)
			 : _colour(colour), _radius(radius), _slices(slices), 
			   _stacks(stacks), _meridian(0.0), _elevation(0.0)  {
			
				_sphere = gluNewQuadric();
				_nurbs_renderer = gluNewNurbsRenderer();
			}

			~Globe() { 
				
				gluDeleteNurbsRenderer(_nurbs_renderer);
				gluDeleteQuadric(_sphere);
			}

			GLuint&
			GetSlices() { return _slices; }

			GLuint&
			GetStacks() { return _stacks; }

			GLfloat&
			GetMeridian() { return _meridian; }

			GLfloat&
			GetElevation() { return _elevation; }

			void
			Draw();

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
			static const GLuint DEFAULT_SLICES = 72;

			/**
			 * One stack every 5 degrees.
			 */
			static const GLuint DEFAULT_STACKS = 36;

			Colour _colour;

			/**
			 * The Globe's radius will almost always be one.
			 */
			GLfloat _radius;

			/**
			 * The stacks and slices determine the 'resolution' of 
			 * the rendered Globe.
			 */
			GLuint _slices;
			GLuint _stacks;

			GLUquadricObj *_sphere;
			GLUnurbsObj *_nurbs_renderer;

			GLfloat _meridian;
			GLfloat _elevation;

			void
			NormaliseMeridianElevation();
	};
}
