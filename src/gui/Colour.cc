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
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 */

#include <iostream>
#include "Colour.h"

using namespace GPlatesGui;

const Colour Colour::BLACK	(0x00, 0x00, 0x00);
const Colour Colour::WHITE	(0xFF, 0xFF, 0xFF);
const Colour Colour::RED	(0xFF, 0x00, 0x00);
const Colour Colour::GREEN	(0x00, 0x80, 0x00);

const Colour Colour::BLUE	(0x00, 0x00, 0xFF);
const Colour Colour::GREY	(0x80, 0x80, 0x80);
const Colour Colour::SILVER	(0xC0, 0xC0, 0xC0);
const Colour Colour::MAROON	(0x80, 0x00, 0x00);

const Colour Colour::PURPLE	(0x80, 0x00, 0x80);
const Colour Colour::FUSCHIA(0xFF, 0x00, 0xFF);
const Colour Colour::LIME	(0x00, 0xFF, 0x00);
const Colour Colour::OLIVE	(0x80, 0x80, 0x00);

const Colour Colour::YELLOW	(0xFF, 0xFF, 0x00);
const Colour Colour::NAVY	(0x00, 0x00, 0x80);
const Colour Colour::TEAL	(0x00, 0x80, 0x80);
const Colour Colour::AQUA	(0x00, 0xFF, 0xFF);

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

static GLfloat
ConvertByteToColourComponent(const GLint& byte)
{
	static const GLfloat FACTOR = 1.0 / static_cast<GLfloat>(255.0);

	return FACTOR * byte;
}

Colour::Colour(const GLint& red,
			   const GLint& green,
			   const GLint& blue
			   const GLint& alpha)
{
	_rgba[RED_INDEX]   = ConvertByteToColourComponent(red);
	_rgba[GREEN_INDEX] = ConvertByteToColourComponent(green);
	_rgba[BLUE_INDEX]  = ConvertByteToColourComponent(blue);
	_rgba[ALPHA_INDEX] = ConvertByteToColourComponent(alpha);
}
