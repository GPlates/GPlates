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

#ifndef GPLATES_GUI_COLOUR_H
#define GPLATES_GUI_COLOUR_H

#include <iosfwd>
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
		 * Indices of the respective colour componets.
		 */
		enum {
			RED_INDEX = 0,
			GREEN_INDEX = 1,
			BLUE_INDEX = 2,
			ALPHA_INDEX = 3,
			RGBA_SIZE  // the number of elements in an RGBA colour
		};

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
		Colour(
				const GLfloat &red   = 0.0,
				const GLfloat &green = 0.0,
				const GLfloat &blue  = 0.0,
				const GLfloat &alpha = 1.0);

		Colour(
				const Colour &colour)
		{
			d_rgba[RED_INDEX] = colour.d_rgba[RED_INDEX];
			d_rgba[GREEN_INDEX] = colour.d_rgba[GREEN_INDEX];
			d_rgba[BLUE_INDEX] = colour.d_rgba[BLUE_INDEX];
			d_rgba[ALPHA_INDEX] = colour.d_rgba[ALPHA_INDEX];
		}

		/**
		 * Accessor methods.
		 */

		GLfloat
		red() const
		{
			return d_rgba[RED_INDEX];
		}

		GLfloat
		green() const
		{
			return d_rgba[GREEN_INDEX];
		}

		GLfloat
		blue() const
		{
			return d_rgba[BLUE_INDEX];
		}

		GLfloat
		alpha() const
		{
			return d_rgba[ALPHA_INDEX];
		}

		/*
		 * Accessor methods which allow modification.
		 */

		GLfloat &
		red()
		{
			return d_rgba[RED_INDEX];
		}

		GLfloat &
		green()
		{
			return d_rgba[GREEN_INDEX];
		}

		GLfloat &
		blue()
		{
			return d_rgba[BLUE_INDEX];
		}

		GLfloat &
		alpha()
		{
			return d_rgba[ALPHA_INDEX];
		}

		/*
		 * Type conversion operators for integration with
		 * OpenGL colour commands.
		 */

		operator GLfloat*()
		{
			return &d_rgba[0];
		}

		operator const GLfloat*() const
		{
			return &d_rgba[0];
		}

	private:

		/**
		 * The storage space for the colour components.
		 *
		 * This is an array because that allows it to be
		 * passed to OpenGL as a vector, which is often
		 * faster than passing (and hence copying each
		 * individual component.
		 */
		GLfloat d_rgba[RGBA_SIZE];
	};


	std::ostream &
	operator<<(
			std::ostream &os,
			const Colour &c);
}

#endif  // GPLATES_GUI_COLOUR_H
