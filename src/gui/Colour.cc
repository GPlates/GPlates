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

#include <iostream>
#include "Colour.h"


/**
 * Define a function (eg, "get_black()") that creates a local static colour object
 * and returns it.
 */
#define DEFINE_COLOUR(colour_name, r, g, b) \
	const GPlatesGui::Colour & \
	GPlatesGui::Colour::get_##colour_name() \
	{ \
		static const GPlatesGui::Colour colour_name(r, g, b); \
		return colour_name; \
	}

DEFINE_COLOUR(black, 0.0, 0.0, 0.0)
DEFINE_COLOUR(white, 1.0, 1.0, 1.0)
DEFINE_COLOUR(red, 1.0, 0.0, 0.0)
DEFINE_COLOUR(green, 0.0, 0.5, 0.0)

DEFINE_COLOUR(blue, 0.0, 0.0, 1.0)
DEFINE_COLOUR(grey, 0.5, 0.5, 0.5)
DEFINE_COLOUR(silver, 0.75, 0.75, 0.75)
DEFINE_COLOUR(maroon, 0.5, 0.0, 0.0)

DEFINE_COLOUR(purple, 0.5, 0.0, 0.5)
DEFINE_COLOUR(fuschia, 1.0, 0.0, 1.0)
DEFINE_COLOUR(lime, 0.0, 1.0, 0.0)
DEFINE_COLOUR(olive, 0.5, 0.5, 0.0)

DEFINE_COLOUR(yellow, 1.0, 1.0, 0.0)
DEFINE_COLOUR(navy, 0.0, 0.0, 0.5)
DEFINE_COLOUR(teal, 0.0, 0.5, 0.5)
DEFINE_COLOUR(aqua, 0.0, 1.0, 1.0)


GPlatesGui::Colour::Colour(
		const GLfloat &red_,
		const GLfloat &green_,
		const GLfloat &blue_,
		const GLfloat &alpha_)
{
	d_rgba[RED_INDEX]   = red_;
	d_rgba[GREEN_INDEX] = green_;
	d_rgba[BLUE_INDEX]  = blue_;
	d_rgba[ALPHA_INDEX] = alpha_;
}


std::ostream &
GPlatesGui::operator<<(
		std::ostream &os,
		const Colour &c)
{
	os << "("
			<< c.red() << ", "
			<< c.green() << ", "
			<< c.blue() << ", "
			<< c.alpha() << ")";

	return os;
}
