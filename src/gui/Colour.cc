/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

using namespace GPlatesGui;

const Colour Colour::BLACK	(0.0, 0.0, 0.0);
const Colour Colour::WHITE	(1.0, 1.0, 1.0);
const Colour Colour::RED	(1.0, 0.0, 0.0);
const Colour Colour::GREEN	(0.0, 0.5, 0.0);

const Colour Colour::BLUE	(0.0, 0.0, 1.0);
const Colour Colour::GREY	(0.5, 0.5, 0.5);
const Colour Colour::SILVER	(0.75, 0.75, 0.75);
const Colour Colour::MAROON	(0.5, 0.0, 0.0);

const Colour Colour::PURPLE	(0.5, 0.0, 0.5);
const Colour Colour::FUSCHIA(1.0, 0.0, 1.0);
const Colour Colour::LIME	(0.0, 1.0, 0.0);
const Colour Colour::OLIVE	(0.5, 0.5, 0.0);

const Colour Colour::YELLOW	(1.0, 1.0, 0.0);
const Colour Colour::NAVY	(0.0, 0.0, 0.5);
const Colour Colour::TEAL	(0.0, 0.5, 0.5);
const Colour Colour::AQUA	(0.0, 1.0, 1.0);

Colour::Colour(const GLfloat& red, 
			   const GLfloat& green, 
			   const GLfloat& blue, 
			   const GLfloat& alpha)
{
	_rgba[RED_INDEX]   = red;
	_rgba[GREEN_INDEX] = green;
	_rgba[BLUE_INDEX]  = blue;
	_rgba[ALPHA_INDEX] = alpha;
}
