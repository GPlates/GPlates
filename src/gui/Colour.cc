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

#include <iostream>
#include "Colour.h"

using namespace GPlatesGui;

const Colour Colour::BLACK(0.0, 0.0, 0.0);
const Colour Colour::WHITE(1.0, 1.0, 1.0);
const Colour Colour::RED  (1.0, 0.0, 0.0);
const Colour Colour::GREEN(0.0, 1.0, 0.0);
const Colour Colour::BLUE (0.0, 0.0, 1.0);
const Colour Colour::GREY (0.5, 0.5, 0.5);


Colour::Colour(const GLfloat& red, const GLfloat& green, 
 const GLfloat& blue, const GLfloat& alpha)
{
	_rgba[RED_INDEX]   = red;
	_rgba[GREEN_INDEX] = green;
	_rgba[BLUE_INDEX]  = blue;
	_rgba[ALPHA_INDEX] = alpha;
}

