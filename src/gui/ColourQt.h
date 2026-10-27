/* $Id$ */

/**
 * @file 
 * Conversions between Colour and Qt's QColor, and the HSV colour model (which Qt implements).
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2026 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_COLOURQT_H
#define GPLATES_GUI_COLOURQT_H

#include <QColor>

#include "Colour.h"


namespace GPlatesGui
{
	//
	// Everything in this file depends on Qt Gui (QColor), which is why it is kept out of
	// "Colour.h": the pygplates module compiles "Colour.h" but links only Qt Core, so "Colour.h"
	// must not depend on Qt Gui and this file must not be included from anything the module
	// compiles (the pygplates-source-closure test enforces both).
	//

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
		{  }

		/**
		 * Linearly interpolate between two colours.
		 * @param first The first colour to mix
		 * @param second The second colour to mix
		 * @param position A value between 0.0 and 1.0 (inclusive), which can be
		 * interpreted as where the returned colour lies in the range between the
		 * first colour and the second colour.
		 */
		static
		HSVColour
		linearly_interpolate(
				const HSVColour &first,
				const HSVColour &second,
				const double &position);
	};


	/**
	 * Converts a QColor to a Colour, preserving the alpha component.
	 */
	Colour
	colour_from_qcolor(
			const QColor &qcolor);

	/**
	 * Converts a Colour to a QColor, preserving the alpha component.
	 *
	 * Colour components are not clamped to [0,1] (OpenGL does its own clamping) but QColor
	 * requires it, so each component is clamped here.
	 */
	QColor
	qcolor_from_colour(
			const Colour &colour);

	/**
	 * Converts a HSV colour to a Colour (which is RGBA). The hue,
	 * saturation, value and alpha components of the colour must be in the range
	 * 0.0-1.0 inclusive.
	 */
	Colour
	colour_from_hsv(
			const HSVColour &hsv);

	/**
	 * Converts a Colour (which is RGBA) to HSV.
	 * @returns HSV colour, where the component values are between 0.0-1.0 inclusive.
	 */
	HSVColour
	hsv_from_colour(
			const Colour &colour);
}

#endif  // GPLATES_GUI_COLOURQT_H
