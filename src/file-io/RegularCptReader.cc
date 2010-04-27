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

// On some versions of g++ with some versions of Qt, it's not liking at() and
// operator[] in QStringList.
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#endif

#include <QDebug>
#include <QFile>
#include <QStringList>
#include <QTextStream>

#include "RegularCptReader.h"

#include "gui/GenericContinuousColourPalette.h"
#include "gui/GMTColourNames.h"

#include "maths/Real.h"


using GPlatesMaths::Real;

namespace
{
	/**
	 * At the end of every line, there are two optional strings.
	 *
	 * The first is one of L, U or B.
	 *
	 * The second starts with a semi-colon.
	 *
	 * This function checks whether these strings are of the right format.
	 */
	bool
	check_optional_strings(
			const QStringList &list,
			int starting_index)
	{
		if (starting_index == list.count())
		{
			// No tokens.
			return true;
		}

		if (starting_index + 1 == list.count())
		{
			// Precisely 1 token left.
			QString a = list[starting_index];
			return a == "L" || a == "U" || a == "B";
		}
		else if (starting_index + 2 == list.count())
		{
			// Precisely 2 tokens left.
			QString a = list[starting_index];
			QString b = list[starting_index + 1];
			return (a == "L" || a == "U" || a == "B") &&
				b.startsWith(";");
		}
		else
		{
			return false;
		}
	}


	/**
	 * Attempts to process a line as a comment.
	 *
	 * Returns true if successful.
	 */
	bool
	try_process_comment(
			const QString &line,
			bool &hsv)
	{
		if (line == "# COLOR_MODEL = HSV" || line == "# COLOR_MODEL = +HSV")
		{
			hsv = true;
		}
		return line.startsWith("#");
	}


	bool
	in_rgb_range(
			int value)
	{
		return 0 <= value && value <= 255;
	}


	GPlatesGui::Colour
	make_rgb_colour(
			int r, int g, int b)
	{
		return GPlatesGui::Colour(
				static_cast<GLfloat>(r / 255.0f),
				static_cast<GLfloat>(g / 255.0f),
				static_cast<GLfloat>(b / 255.0f));
	}


	bool
	in_h_range(
			int h)
	{
		return 0 <= h && h <= 360;
	}


	bool
	in_sv_range(
			double value)
	{
		return Real(0.0) <= Real(value) && Real(value) <= Real(1.0);
	}


	GPlatesGui::Colour
	make_hsv_colour(
			int h, double s, double v)
	{
		return GPlatesGui::Colour::from_hsv(GPlatesGui::HSVColour(h / 360.0, s, v));
	}


	bool
	in_cmyk_range(
			int value)
	{
		return 0 <= value && value <= 100;
	}


	GPlatesGui::Colour
	make_cmyk_colour(
			int c, int m, int y, int k)
	{
		return GPlatesGui::Colour::from_cmyk(
				GPlatesGui::CMYKColour(
					c / 100.0,
					m / 100.0,
					y / 100.0,
					k / 100.0));
	}


	bool
	in_grey_range(
			int value)
	{
		return 0 <= value && value <= 255;
	}


	GPlatesGui::Colour
	make_grey_colour(
			int value)
	{
		GLfloat f = static_cast<GLfloat>(value / 255.0f);
		return GPlatesGui::Colour(f, f, f);
	}


	/**
	 * Attempts to process a line as one containing two RGB values.
	 *
	 * The format is:
	 *
	 *		x R G B y R G B
	 *
	 * with two optional strings at the end.
	 *
	 * Returns true if successful.
	 */
	bool
	try_process_rgb(
			const QStringList &list,
			GPlatesGui::GenericContinuousColourPalette &palette)
	{
		if (list.count() < 8 || list.count() > 10)
		{
			return false;
		}

		// For conversions.
		bool ok;

		// Try and parse the first 8 tokens.
		double x, y;
		int r1, g1, b1, r2, g2, b2;

		x = list[0].toFloat(&ok);
		if (!ok)
		{
			return false;
		}
		r1 = list[1].toInt(&ok);
		if (!ok || !in_rgb_range(r1))
		{
			return false;
		}
		g1 = list[2].toInt(&ok);
		if (!ok || !in_rgb_range(g1))
		{
			return false;
		}
		b1 = list[3].toInt(&ok);
		if (!ok || !in_rgb_range(b1))
		{
			return false;
		}

		y = list[4].toFloat(&ok);
		if (!ok || Real(y) < Real(x))
		{
			return false;
		}
		r2 = list[5].toInt(&ok);
		if (!ok || !in_rgb_range(r2))
		{
			return false;
		}
		g2 = list[6].toInt(&ok);
		if (!ok || !in_rgb_range(g2))
		{
			return false;
		}
		b2 = list[7].toInt(&ok);
		if (!ok || !in_rgb_range(b2))
		{
			return false;
		}

		// Check the last 2 tokens, if any.
		check_optional_strings(list, 8);

		// Store in palette.
		palette.add_colour_slice(
				GPlatesGui::ColourSlice(
					x, y, make_rgb_colour(r1, g1, b1), make_rgb_colour(r2, g2, b2)));
		return true;
	}


	/**
	 * Attempts to process a line as one containing two HSV values.
	 *
	 * The format is:
	 *
	 *		x H S V y H S V
	 *
	 * with two optional strings at the end.
	 *
	 * Returns true if successful.
	 */
	bool
	try_process_hsv(
			const QStringList &list,
			GPlatesGui::GenericContinuousColourPalette &palette)
	{
		if (list.count() < 8 || list.count() > 10)
		{
			return false;
		}

		// For conversions.
		bool ok;

		// Try and parse the first 8 tokens.
		double x, y;
		int h1, h2;
		double s1, v1, s2, v2;

		x = list[0].toFloat(&ok);
		if (!ok)
		{
			return false;
		}
		h1 = list[1].toInt(&ok);
		if (!ok || !in_h_range(h1))
		{
			return false;
		}
		s1 = list[2].toFloat(&ok);
		if (!ok || !in_sv_range(s1))
		{
			return false;
		}
		v1 = list[3].toFloat(&ok);
		if (!ok || !in_sv_range(v1))
		{
			return false;
		}

		y = list[4].toFloat(&ok);
		if (!ok || Real(y) < Real(x))
		{
			return false;
		}
		h2 = list[5].toInt(&ok);
		if (!ok || !in_h_range(h2))
		{
			return false;
		}
		s2 = list[6].toFloat(&ok);
		if (!ok || !in_sv_range(s2))
		{
			return false;
		}
		v2 = list[7].toFloat(&ok);
		if (!ok || !in_sv_range(v2))
		{
			return false;
		}

		// Check the last 2 tokens, if any.
		check_optional_strings(list, 8);

		// Store in palette.
		palette.add_colour_slice(
				GPlatesGui::ColourSlice(
					x, y, make_hsv_colour(h1, s1, v1), make_hsv_colour(h2, s2, v2)));
		return true;
	}


	bool
	try_process_rgb_or_hsv(
			const QStringList &list,
			GPlatesGui::GenericContinuousColourPalette &palette,
			bool &hsv)
	{
		if (hsv)
		{
			return try_process_hsv(list, palette);
		}
		else
		{
			return try_process_rgb(list, palette);
		}
	}


	/**
	 * Attempts to process a line as one containing two CMYK values.
	 *
	 * The format is:
	 *
	 *		x C M Y K y C M Y K
	 *
	 * with two optional strings at the end.
	 *
	 * Returns true if successful.
	 */
	bool
	try_process_cmyk(
			const QStringList &list,
			GPlatesGui::GenericContinuousColourPalette &palette)
	{
		if (list.count() < 10 || list.count() > 12)
		{
			return false;
		}

		// For conversions.
		bool ok;

		// Try and parse the first 10 tokens.
		double x, y;
		int c1, m1, y1, k1, c2, m2, y2, k2;

		x = list[0].toFloat(&ok);
		if (!ok)
		{
			return false;
		}
		c1 = list[1].toInt(&ok);
		if (!ok || !in_cmyk_range(c1))
		{
			return false;
		}
		m1 = list[2].toInt(&ok);
		if (!ok || !in_cmyk_range(m1))
		{
			return false;
		}
		y1 = list[3].toInt(&ok);
		if (!ok || !in_cmyk_range(y1))
		{
			return false;
		}
		k1 = list[4].toInt(&ok);
		if (!ok || !in_cmyk_range(k1))
		{
			return false;
		}

		y = list[5].toFloat(&ok);
		if (!ok || Real(y) < Real(x))
		{
			return false;
		}
		c2 = list[6].toInt(&ok);
		if (!ok || !in_cmyk_range(c2))
		{
			return false;
		}
		m2 = list[7].toInt(&ok);
		if (!ok || !in_cmyk_range(m2))
		{
			return false;
		}
		y2 = list[8].toInt(&ok);
		if (!ok || !in_cmyk_range(y2))
		{
			return false;
		}
		k2 = list[9].toInt(&ok);
		if (!ok || !in_cmyk_range(k2))
		{
			return false;
		}

		// Check the last 2 tokens, if any.
		check_optional_strings(list, 10);

		// Store in palette.
		palette.add_colour_slice(
				GPlatesGui::ColourSlice(
					x, y, make_cmyk_colour(c1, m1, y1, k1), make_cmyk_colour(c2, m2, y2, k2)));
		return true;
	}

	
	/**
	 * Attempts to process a line as one containing two greyshade values.
	 *
	 * The format is:
	 *
	 *		x G y G
	 *
	 * with two optional strings at the end.
	 *
	 * Returns true if successful.
	 */
	bool
	try_process_grey(
			const QStringList &list,
			GPlatesGui::GenericContinuousColourPalette &palette)
	{
		if (list.count() < 4 || list.count() > 6)
		{
			return false;
		}

		// For conversions.
		bool ok;

		// Try and parse the first 4 tokens.
		double x, y;
		int g1, g2;

		x = list[0].toFloat(&ok);
		if (!ok)
		{
			return false;
		}
		g1 = list[1].toInt(&ok);
		if (!ok || !in_grey_range(g1))
		{
			return false;
		}

		y = list[2].toFloat(&ok);
		if (!ok || Real(y) < Real(x))
		{
			return false;
		}
		g2 = list[3].toInt(&ok);
		if (!ok || !in_grey_range(g2))
		{
			return false;
		}

		// Check the last 2 tokens, if any.
		check_optional_strings(list, 4);

		// Store in palette.
		palette.add_colour_slice(
				GPlatesGui::ColourSlice(
					x, y, make_grey_colour(g1), make_grey_colour(g2)));
		return true;
	}

	
	/**
	 * Attempts to process a line as one containing two colour names.
	 *
	 * The format is:
	 *
	 *		x Name y Name
	 *
	 * with two optional strings at the end.
	 *
	 * Returns true if successful.
	 */
	bool
	try_process_name(
			const QStringList &list,
			GPlatesGui::GenericContinuousColourPalette &palette)
	{
		if (list.count() < 4 || list.count() > 6)
		{
			return false;
		}

		// For conversions.
		bool ok;

		// Try and parse the first 4 tokens.
		double x, y;
		boost::optional<GPlatesGui::Colour> lower_colour, upper_colour;

		x = list[0].toFloat(&ok);
		if (!ok)
		{
			return false;
		}
		lower_colour = GPlatesGui::GMTColourNames::instance().get_colour(list[1].toStdString());
		if (!lower_colour)
		{
			return false;
		}

		y = list[2].toFloat(&ok);
		if (!ok || Real(y) < Real(x))
		{
			return false;
		}
		upper_colour = GPlatesGui::GMTColourNames::instance().get_colour(list[3].toStdString());
		if (!upper_colour)
		{
			return false;
		}

		// Check the last 2 tokens, if any.
		check_optional_strings(list, 4);

		// Store in palette.
		palette.add_colour_slice(
				GPlatesGui::ColourSlice(
					x, y, *lower_colour, *upper_colour));
		return true;
	}


	/**
	 *  Parse the content of a "fbn" line, i.e. a line containing one of 
	 *  F R G B  (foreground rgb value)
	 *  B R G B  (background rgb value)      
	 *  N R G B  (nan rgb value)           
	 * 
	 * The first item should be a single character, and should be one of "F", "B" or "N"
	 * The second, third and fourth terms should be integers in the range 0-255, representing
	 * red, green and blue respectively.
	 *
	 * Note: could also be HSV!
	 */
	bool
	try_process_fbn(
			const QStringList &list,
			GPlatesGui::GenericContinuousColourPalette &palette,
			bool hsv)
	{
		if (list.count() != 4)
		{
			return false;
		}

		if (hsv)
		{
			int h;
			double s, v;
			bool ok = false;
			// First h value
			h = list.at(1).toInt(&ok);
			if (!ok){
				return false;
			}
			// First s value
			s = list.at(2).toFloat(&ok);
			if (!ok){
				return false;
			}
			// First v value
			v = list.at(3).toFloat(&ok);
			if (!ok){
				return false;
			}

			if (!(in_h_range(h) && in_sv_range(s) && in_sv_range(v)))
			{
				return false;
			}

			QString letter = list.at(0);
			if (letter == "B")
			{
				palette.set_background_colour(make_hsv_colour(h, s, v));
				return true;
			}
			else if (letter == "F")
			{
				palette.set_foreground_colour(make_hsv_colour(h, s, v));
				return true;
			}
			else if (letter == "N")
			{
				palette.set_nan_colour(make_hsv_colour(h, s, v));
				return true;
			}
		}
		else // RGB
		{
			int r,g,b;
			bool ok = false;
			// First r value
			r = list.at(1).toInt(&ok);
			if (!ok){
				return false;
			}
			// First g value
			g = list.at(2).toInt(&ok);
			if (!ok){
				return false;
			}
			// First b value
			b = list.at(3).toInt(&ok);
			if (!ok){
				return false;
			}

			if (!(in_rgb_range(r) && in_rgb_range(g) && in_rgb_range(b)))
			{
				return false;
			}

			QString letter = list.at(0);
			if (letter == "B")
			{
				palette.set_background_colour(make_rgb_colour(r, g, b));
				return true;
			}
			else if (letter == "F")
			{
				palette.set_foreground_colour(make_rgb_colour(r, g, b));
				return true;
			}
			else if (letter == "N")
			{
				palette.set_nan_colour(make_rgb_colour(r, g, b));
				return true;
			}
		}

		return false;
	}


	void
	process_line(
			const QString &filename,
			const QString &line,
			GPlatesGui::GenericContinuousColourPalette &palette,
			bool &hsv)
	{
		if (try_process_comment(line, hsv))
		{
			return;
		}

		QStringList list = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);

		if (try_process_rgb_or_hsv(list, palette, hsv))
		{
			return;
		}

		if (try_process_cmyk(list, palette))
		{
			return;
		}

		if (try_process_grey(list, palette))
		{
			return;
		}

		if (try_process_name(list, palette))
		{
			return;
		}

		if (try_process_fbn(list, palette, hsv))
		{
			return;
		}

		throw GPlatesFileIO::ErrorReadingCptFileException(filename + ": could not parse " + line);
	}
}


GPlatesGui::GenericContinuousColourPalette *
GPlatesFileIO::RegularCptReader::read_file(
		QString filename)
{
	QFile qfile(filename);
	
	if (!qfile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		throw ErrorReadingCptFileException(filename + " could not be opened.");
	}

	QTextStream text_stream(&qfile);
	bool hsv = false;

	GPlatesGui::GenericContinuousColourPalette *palette = new GPlatesGui::GenericContinuousColourPalette();
	
	while (!text_stream.atEnd())
	{
		QString line = text_stream.readLine().trimmed();
		process_line(filename, line, *palette, hsv);
	}

	return palette;
}

