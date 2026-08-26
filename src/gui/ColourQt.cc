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

#include <QtGlobal>

#include "ColourQt.h"


namespace
{
	inline
	qreal
	clamp_zero_one(
			qreal value)
	{
		if (value < 0.0)
		{
			return 0.0;
		}
		else if (value > 1.0)
		{
			return 1.0;
		}
		else
		{
			return value;
		}
	}
}


GPlatesGui::HSVColour
GPlatesGui::HSVColour::linearly_interpolate(
		const GPlatesGui::HSVColour &first,
		const GPlatesGui::HSVColour &second,
		const double &position)
{
	const double one_minus_position = (1.0 - position);

	// If either colour has a saturation of zero then it is achromatic (ie, gray or white) and hence
	// the hue value is meaningless. In this case we want both colours to have the same hue so that
	// we don't unnecessarily interpolate through a range of hues.
	double first_h = first.h;
	double second_h = second.h;
	if (first.s < 1e-12)
	{
		first_h = second_h;
	}
	else if (second.s < 1e-12)
	{
		second_h = first_h;
	}

	// Hue is cyclic (wraps from 1.0 back to 0.0).
	// So we need to take the shortest path between two colours.
	const double h_delta = second_h - first_h;
	double h_interp;
	if (h_delta < -0.5)
	{
		h_interp = first_h * one_minus_position + (1.0 + second_h) * position;
		if (h_interp > 1.0)
		{
			h_interp -= 1.0;
		}
	}
	else if (h_delta > 0.5)
	{
		h_interp = (1.0 + first_h) * one_minus_position + second_h * position;
		if (h_interp > 1.0)
		{
			h_interp -= 1.0;
		}
	}
	else // Shortest path is directly between the two colours (no wrapping needed)...
	{
		h_interp = first_h * one_minus_position + second_h * position;
	}

	return HSVColour(
			h_interp,
			first.s * one_minus_position + second.s * position,
			first.v * one_minus_position + second.v * position,
			first.a * one_minus_position + second.a * position);
}


GPlatesGui::Colour
GPlatesGui::colour_from_qcolor(
		const QColor &qcolor)
{
	return Colour(
			static_cast<float>(qcolor.redF()),
			static_cast<float>(qcolor.greenF()),
			static_cast<float>(qcolor.blueF()),
			static_cast<float>(qcolor.alphaF()));
}


QColor
GPlatesGui::qcolor_from_colour(
		const Colour &colour)
{
	QColor qcolor;
	qcolor.setRgbF(
			::clamp_zero_one(colour.red()),
			::clamp_zero_one(colour.green()),
			::clamp_zero_one(colour.blue()),
			::clamp_zero_one(colour.alpha()));
	return qcolor;
}


GPlatesGui::Colour
GPlatesGui::colour_from_hsv(
		const HSVColour &hsv)
{
	QColor qcolor;
	qcolor.setHsvF(hsv.h, hsv.s, hsv.v, hsv.a);
	return colour_from_qcolor(qcolor);
}


GPlatesGui::HSVColour
GPlatesGui::hsv_from_colour(
		const Colour &colour)
{
	const QColor qcolor = qcolor_from_colour(colour);
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	float h, s, v, a;
#else
	qreal h, s, v, a;
#endif
	qcolor.getHsvF(&h, &s, &v, &a);
	// Qt returns -1 for achromatic colours (ie, grays, where saturation is zero).
	// Set to a value in the range [0,1] since that's the expected range.
	if (h < 0)
	{
		h = 0;
	}

	return HSVColour(h, s, v, a);
}
