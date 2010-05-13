/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
 * (as "CptImporter.cc")
 *
 * Copyright (C) 2010 The University of Sydney, Australia
 * (as "RegularCptReader.cc")
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

#include "CptReaderUtils.h"

#include "gui/Colour.h"
#include "gui/GMTColourNames.h"

#include "maths/Real.h"


using GPlatesMaths::Real;

namespace GPlatesFileIO
{
	namespace CptReaderUtils
	{
		template<>
		int
		parse_token<int>(const QString &token)
		{
			bool ok;
			int result = token.toInt(&ok);
			if (ok)
			{
				return result;
			}
			else
			{
				throw BadTokenException();
			}
		}


		template<>
		double
		parse_token<double>(const QString &token)
		{
			bool ok;
			double result = token.toDouble(&ok);
			if (ok)
			{
				return result;
			}
			else
			{
				throw BadTokenException();
			}
		}


		template<>
		const QString &
		parse_token<const QString &>(const QString &token)
		{
			return token;
		}


		template<>
		GPlatesGui::ColourScaleAnnotation::Type
		parse_token<GPlatesGui::ColourScaleAnnotation::Type>(const QString &token)
		{
			if (token == "L")
			{
				return GPlatesGui::ColourScaleAnnotation::LOWER;
			}
			else if (token == "U")
			{
				return GPlatesGui::ColourScaleAnnotation::UPPER;
			}
			else if (token == "B")
			{
				return GPlatesGui::ColourScaleAnnotation::BOTH;
			}
			else
			{
				throw BadTokenException();
			}
		}


		template<>
		boost::tuples::null_type
		parse_components<boost::tuples::null_type>(
				const QStringList &tokens,
				int starting_index)
		{
			return boost::tuples::null_type();
		}
	}
}


bool
GPlatesFileIO::CptReaderUtils::in_rgb_range(
		int value)
{
	return 0 <= value && value <= 255;
}


GPlatesGui::Colour
GPlatesFileIO::CptReaderUtils::make_rgb_colour(
		int r, int g, int b)
{
	if (in_rgb_range(r) && in_rgb_range(g) && in_rgb_range(b))
	{
		return GPlatesGui::Colour(
				static_cast<GLfloat>(r / 255.0f),
				static_cast<GLfloat>(g / 255.0f),
				static_cast<GLfloat>(b / 255.0f));
	}
	else
	{
		throw BadComponentsException();
	}
}


bool
GPlatesFileIO::CptReaderUtils::in_h_range(
		int value)
{
	return 0 <= value && value <= 360;
}


bool
GPlatesFileIO::CptReaderUtils::in_sv_range(
		double value)
{
	return Real(0.0) <= Real(value) && Real(value) <= Real(1.0);
}


GPlatesGui::Colour
GPlatesFileIO::CptReaderUtils::make_hsv_colour(
		int h, double s, double v)
{
	if (in_h_range(h) && in_sv_range(s) && in_sv_range(v))
	{
		return GPlatesGui::Colour::from_hsv(GPlatesGui::HSVColour(
				h / 360.0, s, v));
	}
	else
	{
		throw BadComponentsException();
	}
}


bool
GPlatesFileIO::CptReaderUtils::in_cmyk_range(
		int value)
{
	return 0 <= value && value <= 100;
}


GPlatesGui::Colour
GPlatesFileIO::CptReaderUtils::make_cmyk_colour(
		int c, int m, int y, int k)
{
	if (in_cmyk_range(c) && in_cmyk_range(m) && in_cmyk_range(y) && in_cmyk_range(k))
	{
		return GPlatesGui::Colour::from_cmyk(
				GPlatesGui::CMYKColour(
					c / 100.0,
					m / 100.0,
					y / 100.0,
					k / 100.0));
	}
	else
	{
		throw BadComponentsException();
	}
}


bool
GPlatesFileIO::CptReaderUtils::in_grey_range(
		int value)
{
	return 0 <= value && value <= 255;
}


GPlatesGui::Colour
GPlatesFileIO::CptReaderUtils::make_grey_colour(
		int value)
{
	if (in_grey_range(value))
	{
		GLfloat f = static_cast<GLfloat>(value / 255.0f);
		return GPlatesGui::Colour(f, f, f);
	}
	else
	{
		throw BadComponentsException();
	}
}


GPlatesGui::Colour
GPlatesFileIO::CptReaderUtils::make_gmt_colour(
		const QString &name)
{
	boost::optional<GPlatesGui::Colour> result =
		GPlatesGui::GMTColourNames::instance().get_colour(name.toStdString());
	if (result)
	{
		return *result;
	}
	else
	{
		throw BadComponentsException();
	}
}

