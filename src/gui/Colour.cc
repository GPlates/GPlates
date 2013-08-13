/* $Id$ */

/**
 * @file 
 * Contains the implementation of the Colour class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009, 2010 The University of Sydney, Australia
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

#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <QDataStream>
#include <QSysInfo>

#include "Colour.h"

// Undefine the min and max macros as they can interfere with the min and
// max functions in std::numeric_limits<T>, on Visual Studio.
#if defined(_MSC_VER)
	#undef min
	#undef max
#endif

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
DEFINE_COLOUR(fuchsia, 1.0, 0.0, 1.0)
DEFINE_COLOUR(lime, 0.0, 1.0, 0.0)
DEFINE_COLOUR(olive, 0.5, 0.5, 0.0)

DEFINE_COLOUR(yellow, 1.0, 1.0, 0.0)
DEFINE_COLOUR(navy, 0.0, 0.0, 0.5)
DEFINE_COLOUR(teal, 0.0, 0.5, 0.5)
DEFINE_COLOUR(aqua, 0.0, 1.0, 1.0)

#undef DEFINE_COLOUR


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


void
GPlatesGui::convert_argb32_to_rgba8(
		const boost::uint32_t *argb32_pixels,
		rgba8_t *rgba8_pixels,
		unsigned int num_pixels)
{
	if (QSysInfo::ByteOrder == QSysInfo::BigEndian)
	{
		for (unsigned int i = 0; i < num_pixels; ++i)
		{
			// Convert 0xAARRGGBB to 0xRRGGBBAA.
			// Since this is big-endian the 0xRRGGBBAA will get written to memory
			// as (RR,GG,BB,AA) which is what we want.
			//
			// This integer indexing (by 'i') is free in most assembly languages.
			rgba8_pixels[i].uint32_value = (
					(argb32_pixels[i] << 8) |
					((argb32_pixels[i] >> 24) & 0xff));
		}
	}
	else // QSysInfo::LittleEndian ...
	{
		for (unsigned int i = 0; i < num_pixels; ++i)
		{
			// Convert 0xAARRGGBB to 0xAABBGGRR (ie, swap the blue and red channels).
			// Since this is little-endian the 0xAABBGGRR will get written to memory
			// as (RR,GG,BB,AA) which is what we want.
			//
			// This integer indexing (by 'i') is free in most assembly languages.
			rgba8_pixels[i].uint32_value = (
						((argb32_pixels[i] << 16) & 0xff0000) |
						((argb32_pixels[i] >> 16) & 0xff) |
						(argb32_pixels[i] & 0xff00ff00));
		}
	}
}


void
GPlatesGui::convert_rgba8_to_argb32(
		const rgba8_t *rgba8_pixels,
		boost::uint32_t *argb32_pixels,
		unsigned int num_pixels)
{
	if (QSysInfo::ByteOrder == QSysInfo::BigEndian)
	{
		for (unsigned int i = 0; i < num_pixels; ++i)
		{
			// Since this is big-endian the (RR,GG,BB,AA) in 'rgba8_t' will get read
			// as the 32-bit integer 0xRRGGBBAA.
			// Convert 0xRRGGBBAA to 0xAARRGGBB.
			//
			// This integer indexing (by 'i') is free in most assembly languages.
			argb32_pixels[i] = (
					(rgba8_pixels[i].uint32_value >> 8) |
					((rgba8_pixels[i].uint32_value << 24) & 0xff000000));
		}
	}
	else // QSysInfo::LittleEndian ...
	{
		for (unsigned int i = 0; i < num_pixels; ++i)
		{
			// Since this is little-endian the (RR,GG,BB,AA) in 'rgba8_t' will get read
			// as the 32-bit integer 0xAABBGGRR.
			//
			// Convert 0xAABBGGRR to 0xAARRGGBB (ie, swap the blue and red channels).
			//
			// This integer indexing (by 'i') is free in most assembly languages.
			argb32_pixels[i] = (
						((rgba8_pixels[i].uint32_value << 16) & 0xff0000) |
						((rgba8_pixels[i].uint32_value >> 16) & 0xff) |
						(rgba8_pixels[i].uint32_value & 0xff00ff00));
		}
	}
}


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


GPlatesGui::Colour::Colour(
		const QColor &qcolor)
{
	d_rgba[RED_INDEX] = static_cast<GLfloat>(qcolor.redF());
	d_rgba[GREEN_INDEX] = static_cast<GLfloat>(qcolor.greenF());
	d_rgba[BLUE_INDEX] = static_cast<GLfloat>(qcolor.blueF());
	d_rgba[ALPHA_INDEX] = static_cast<GLfloat>(qcolor.alphaF());
}


GPlatesGui::Colour::operator QColor() const
{
	QColor qcolor;
	qcolor.setRgbF(
			::clamp_zero_one(d_rgba[RED_INDEX]),
			::clamp_zero_one(d_rgba[GREEN_INDEX]),
			::clamp_zero_one(d_rgba[BLUE_INDEX]),
			::clamp_zero_one(d_rgba[ALPHA_INDEX]));
	return qcolor;
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


QDebug
GPlatesGui::operator <<(
		QDebug dbg,
		const Colour &c)
{
	std::ostringstream output_string_stream;
	output_string_stream << c;

	dbg.nospace() << QString::fromStdString(output_string_stream.str());

	return dbg.space();
}


QTextStream &
GPlatesGui::operator <<(
		QTextStream &stream,
		const Colour &c)
{
	std::ostringstream output_string_stream;
	output_string_stream << c;

	stream << QString::fromStdString(output_string_stream.str());

	return stream;
}


GPlatesGui::Colour
GPlatesGui::Colour::linearly_interpolate(
		const GPlatesGui::Colour &first,
		const GPlatesGui::Colour &second,
		const double &position)
{
	const double one_minus_position = (1.0 - position);

	return Colour(
			static_cast<GLfloat>(first.red() * one_minus_position +
				second.red() * position),
			static_cast<GLfloat>(first.green() * one_minus_position +
				second.green() * position),
			static_cast<GLfloat>(first.blue() * one_minus_position +
				second.blue() * position));
}


GPlatesGui::Colour
GPlatesGui::Colour::modulate(
		const Colour &first,
		const Colour &second)
{
	return Colour(
			static_cast<GLfloat>(first.red()   * second.red()),
			static_cast<GLfloat>(first.green() * second.green()),
			static_cast<GLfloat>(first.blue()  * second.blue()),
			static_cast<GLfloat>(first.alpha() * second.alpha()));
}


GPlatesGui::Colour
GPlatesGui::Colour::from_cmyk(
		const CMYKColour &cmyk)
{
	double c = cmyk.c;
	double m = cmyk.m;
	double y = cmyk.y;
	double k = cmyk.k;

	// algorithm from boost/gil/colour_convert.hpp (but I don't want to add 
	// another dependency when we're not using anything else from GIL)
	return Colour(
			static_cast<GLfloat>(1.0 - (std::min)(1.0, c * (1.0 - k) + k)),
			static_cast<GLfloat>(1.0 - (std::min)(1.0, m * (1.0 - k) + k)),
			static_cast<GLfloat>(1.0 - (std::min)(1.0, y * (1.0 - k) + k)));
}


GPlatesGui::CMYKColour
GPlatesGui::Colour::to_cmyk(
		const Colour &colour)
{
	// need to clamp here because Colour doesn't do any clamping
	double clamped_red = (std::min)(1.0, (std::max)(0.0, static_cast<double>(colour.red())));
	double clamped_green = (std::min)(1.0, (std::max)(0.0, static_cast<double>(colour.green())));
	double clamped_blue = (std::min)(1.0, (std::max)(0.0, static_cast<double>(colour.blue())));

	// algorithm from boost/gil/colour_convert.hpp (but I don't want to add
	// another dependency when we're not using anything else from GIL)
	double c = 1.0 - clamped_red;
	double m = 1.0 - clamped_green;
	double y = 1.0 - clamped_blue;
	double k = (std::min)(c, (std::min)(m, y));
	double x = 1.0 - k;
	if (x > 0.0001)
	{
		c = (c - k) / x;
		m = (m - k) / x;
		y = (y - k) / x;
	}
	else
	{
		c = 0.0;
		m = 0.0;
		y = 0.0;
	}

	return CMYKColour(c, m, y, k);
}


GPlatesGui::Colour
GPlatesGui::Colour::from_hsv(
		const HSVColour &hsv)
{
	QColor qcolor;
	qcolor.setHsvF(hsv.h, hsv.s, hsv.v, hsv.a);
	return static_cast<Colour>(qcolor);
}


GPlatesGui::HSVColour
GPlatesGui::Colour::to_hsv(
		const Colour &colour)
{
	QColor qcolor = static_cast<QColor>(colour);
	qreal h, s, v, a;
	qcolor.getHsvF(&h, &s, &v, &a);
	return HSVColour(h, s, v, a);
}


namespace
{
	static const GLfloat FLOAT_TO_UINT8 = static_cast<GLfloat>(std::numeric_limits<boost::uint8_t>::max());
	static const boost::uint8_t UINT8_MAX_VALUE = std::numeric_limits<boost::uint8_t>::max();

	inline
	boost::uint8_t
	float_to_uint8(
			GLfloat f)
	{
		int i = static_cast<int>(f * FLOAT_TO_UINT8);
		if (i < 0)
		{
			return 0;
		}
		else if (i > UINT8_MAX_VALUE)
		{
			return UINT8_MAX_VALUE;
		}
		else
		{
			return static_cast<boost::uint8_t>(i);
		}
	}
}


GPlatesGui::Colour
GPlatesGui::Colour::from_rgba8(
		const rgba8_t &rgba8)
{
	return Colour(
			rgba8.red / FLOAT_TO_UINT8,
			rgba8.blue / FLOAT_TO_UINT8,
			rgba8.green / FLOAT_TO_UINT8,
			rgba8.alpha / FLOAT_TO_UINT8);
}


GPlatesGui::rgba8_t
GPlatesGui::Colour::to_rgba8(
		const Colour &colour)
{
	const GLfloat *source_components = colour;
	rgba8_t result;
	for (unsigned int i = 0; i != Colour::RGBA_SIZE; ++i)
	{
		result.components[i] = float_to_uint8(source_components[i]);
	}
	return result;
}


GPlatesGui::Colour
GPlatesGui::Colour::from_qrgb(
		const QRgb &rgba)
{
	return QColor::fromRgba(rgba);
}


QRgb
GPlatesGui::Colour::to_qrgb(
		const Colour &colour)
{
	QColor qcolor = colour;
	return qcolor.rgba();
}

