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

#ifndef _GPLATES_GUI_COLOUR_H_
#define _GPLATES_GUI_COLOUR_H_

#include <cstdio>  /* for size_t */
#include "OpenGL.h"

namespace GPlatesGui
{
	class Colour
	{
		public:
			/**
			 * Some commonly used colours.
			 */
			static const Colour BLACK, WHITE, RED, GREEN, BLUE, GREY;

			Colour(const GLfloat& red = 0.0, const GLfloat& green = 0.0,
			 const GLfloat& blue = 0.0, const GLfloat& alpha = 1.0);

			/**
			 * Accessor methods.
			 */
			GLfloat
			Red() const { return _rgba[RED_INDEX]; }

			GLfloat
			Green() const { return _rgba[GREEN_INDEX]; }

			GLfloat
			Blue() const { return _rgba[BLUE_INDEX]; }

			GLfloat
			Alpha() const { return _rgba[ALPHA_INDEX]; }

			/**
			 * Accessor/mutator methods.
			 */
			GLfloat&
			Red() { return _rgba[RED_INDEX]; }

			GLfloat&
			Green() { return _rgba[GREEN_INDEX]; }

			GLfloat&
			Blue() { return _rgba[BLUE_INDEX]; }

			GLfloat&
			Alpha() { return _rgba[ALPHA_INDEX]; }

			/**
			 * Type conversion operators for integration with
			 * OpenGL colour commands.
			 */
			operator GLfloat*() { return &_rgba[0]; }

			operator const GLfloat*() const { return &_rgba[0]; }

		private:
			/**
			 * Number of components in an RGBA colour.
			 */
			static const size_t RGBA_SIZE = 4;

			/**
			 * Indices of the respective colour componets.
			 */
			static const size_t RED_INDEX = 0, GREEN_INDEX = 1, 
			 BLUE_INDEX = 2, ALPHA_INDEX = 3;

			GLfloat _rgba[RGBA_SIZE];
	};
}

#endif  /* _GPLATES_GUI_COLOUR_H_ */
