/* $Id$ */

/**
 * @file 
 * Contains the definition of the Colour class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 The University of Sydney, Australia
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
#include <QColor>

#include "OpenGL.h"

namespace GPlatesGui
{
	struct HSVColour
	{
		/** Hue */
		double h;
		/** Saturation */
		double s;
		/** Value */
		double v;
		/** Alpha */
		double a;

		HSVColour(
				double h_,
				double s_,
				double v_,
				double a_ = 1.0) :
			h(h_),
			s(s_),
			v(v_),
			a(a_)
		{	
		}
	};

	struct CMYKColour
	{
		/** Cyan */
		double c;
		/** Magenta */
		double m;
		/** Yellow */
		double y;
		/** Black */
		double k;

		CMYKColour(
				double c_,
				double m_,
				double y_,
				double k_) :
			c(c_),
			m(m_),
			y(y_),
			k(k_)
		{
		}
	};

	class Colour
	{
	public:
		/*
		 * Some commonly used colours.
		 *
		 * Note: functions are used instead of non-member variables to
		 * avoid undefined initialisation order of non-local static variables.
		 * For example defining one static variable equal to another.
		 * These functions have a local static colour variable.
		 */
		static const Colour & get_black();
		static const Colour & get_white();
		static const Colour & get_red();
		static const Colour & get_green();

		static const Colour & get_blue();
		static const Colour & get_grey();
		static const Colour & get_silver();
		static const Colour & get_maroon();

		static const Colour & get_purple();
		static const Colour & get_fuchsia();
		static const Colour & get_lime();
		static const Colour & get_olive();

		static const Colour & get_yellow();
		static const Colour & get_navy();
		static const Colour & get_teal();
		static const Colour & get_aqua();

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

		/**
		 * Construct a Colour from its QColor equivalent.
		 * Note: this is not an explicit constructor by design.
		 */
		Colour(
				const QColor &qcolor);

		Colour(
				const Colour &colour)
		{
			d_rgba[RED_INDEX] = colour.d_rgba[RED_INDEX];
			d_rgba[GREEN_INDEX] = colour.d_rgba[GREEN_INDEX];
			d_rgba[BLUE_INDEX] = colour.d_rgba[BLUE_INDEX];
			d_rgba[ALPHA_INDEX] = colour.d_rgba[ALPHA_INDEX];
		}

		/**
		 * Converts the Colour to a QColor.
		 */
		operator QColor() const;

		// Accessor methods

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

		// Accessor methods which allow modification

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

		/**
		 * Linearly interpolate between two colours.
		 * @param first The first colour to mix
		 * @param second The second colour to mix
		 * @param position A value between 0.0 and 1.0 (inclusive), which can be
		 * interpreted as where the returned colour lies in the range between the
		 * first colour and the second colour.
		 */
		static
		Colour
		linearly_interpolate(
				Colour first,
				Colour second,
				double position);

		/**
		 * Converts a CMYK colour to a Colour (which is RGBA). The cyan,
		 * magenta, yellow and black components of the colour must be in the range
		 * 0.0-1.0 inclusive. Alpha value of colour is set to 1.0.
		 */
		static
		Colour
		from_cmyk(
				const CMYKColour &cmyk);

		/**
		 * Converts a Colour (which is RGBA) to CMYK.
		 * @returns CMYK colour, where the component values are between 0.0-1.0 inclusive.
		 */
		static
		CMYKColour
		to_cmyk(
				const Colour &colour);

		/**
		 * Converts a HSV colour to a Colour (which is RGBA). The hue,
		 * saturation, value and alpha components of the colour must be in the range
		 * 0.0-1.0 inclusive.
		 */
		static
		Colour
		from_hsv(
				const HSVColour &hsv);

		/**
		 * Converts a Colour (which is RGBA) to HSV.
		 * @returns HSV colour, where the component values are between 0.0-1.0 inclusive.
		 */
		static
		HSVColour
		to_hsv(
				const Colour &colour);

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
