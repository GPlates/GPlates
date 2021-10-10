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


const GPlatesGui::Colour GPlatesGui::Colour::BLACK	(0.0, 0.0, 0.0);
const GPlatesGui::Colour GPlatesGui::Colour::WHITE	(1.0, 1.0, 1.0);
const GPlatesGui::Colour GPlatesGui::Colour::RED	(1.0, 0.0, 0.0);
const GPlatesGui::Colour GPlatesGui::Colour::GREEN	(0.0, 0.5, 0.0);

const GPlatesGui::Colour GPlatesGui::Colour::BLUE	(0.0, 0.0, 1.0);
const GPlatesGui::Colour GPlatesGui::Colour::GREY	(0.5, 0.5, 0.5);
const GPlatesGui::Colour GPlatesGui::Colour::SILVER	(0.75, 0.75, 0.75);
const GPlatesGui::Colour GPlatesGui::Colour::MAROON	(0.5, 0.0, 0.0);

const GPlatesGui::Colour GPlatesGui::Colour::PURPLE	(0.5, 0.0, 0.5);
const GPlatesGui::Colour GPlatesGui::Colour::FUSCHIA	(1.0, 0.0, 1.0);
const GPlatesGui::Colour GPlatesGui::Colour::LIME	(0.0, 1.0, 0.0);
const GPlatesGui::Colour GPlatesGui::Colour::OLIVE	(0.5, 0.5, 0.0);

const GPlatesGui::Colour GPlatesGui::Colour::YELLOW	(1.0, 1.0, 0.0);
const GPlatesGui::Colour GPlatesGui::Colour::NAVY	(0.0, 0.0, 0.5);
const GPlatesGui::Colour GPlatesGui::Colour::TEAL	(0.0, 0.5, 0.5);
const GPlatesGui::Colour GPlatesGui::Colour::AQUA	(0.0, 1.0, 1.0);


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
