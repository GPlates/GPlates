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

#ifndef _GPLATES_GUI_COLOUR_H_
#define _GPLATES_GUI_COLOUR_H_

#include <iostream>
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
			static const Colour 
				BLACK, WHITE, RED, GREEN, 
				BLUE, GREY, SILVER, MAROON,
				PURPLE, FUSCHIA, LIME, OLIVE,
				YELLOW, NAVY, TEAL, AQUA;

			/**
			 * Construct a colour with the given red, green and
			 * blue components.
			 * 
			 * The parameters represent the percentage of red,
			 * green and blue in the resulting colour.
			 * 
			 * The parameters should be in the range 0.0 - 1.0
			 * inclusive.  Values outside this range will not
			 * be clamped, since OpenGL does its own clamping.
			 */
			explicit
			Colour(const GLfloat& red   = 0.0, 
				   const GLfloat& green = 0.0,
			 	   const GLfloat& blue  = 0.0, 
				   const GLfloat& alpha = 1.0);

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
			 * Indices of the respective colour componets.
			 */
			static const size_t RED_INDEX = 0, 
								GREEN_INDEX = 1, 
								BLUE_INDEX = 2, 
								ALPHA_INDEX = 3;
			/**
			 * Number of components in an RGBA colour.
			 */
			static const size_t RGBA_SIZE = 4;

			/**
			 * The storage space for the colour components.
			 *
			 * This is an array because that allows it to be
			 * passed to OpenGL as a vector, which is often
			 * faster than passing (and hence copying each
			 * individual component.
			 */
			GLfloat _rgba[RGBA_SIZE];
	};


	inline std::ostream &
	operator<<(std::ostream &os, const Colour &c) {

		os << "("
		 << c.Red() << ", "
		 << c.Green() << ", "
		 << c.Blue() << ", "
		 << c.Alpha() << ")";

		return os;
	}
}

#endif  /* _GPLATES_GUI_COLOUR_H_ */
