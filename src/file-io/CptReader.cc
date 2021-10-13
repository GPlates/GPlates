/* $Id$ */

/**
 * \file 
 * Contains the template class CptReader.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
 * (as "CptImporter.cc")
 *
 * Copyright (C) 2010 The University of Sydney, Australia
 * (as "CptReader.cc")
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

#include "CptReader.h"

#include "global/CompilerWarnings.h"

#include "gui/GMTColourNames.h"

// On some versions of g++ with some versions of Qt, it's not liking at() and
// operator[] in QStringList.
DISABLE_GCC_WARNING("-Wstrict-overflow")


using boost::tuples::get;


namespace GPlatesFileIO
{
	namespace CptReaderInternals
	{
		template<>
		boost::tuples::null_type
		parse_components<boost::tuples::null_type>(
				const QStringList &tokens,
				unsigned int starting_index)
		{
			return boost::tuples::null_type();
		}


		bool
		TryProcessTokensImpl<RegularCptFileFormat>::operator()(
				const QStringList &tokens,
				ParserState<RegularCptFileFormat> &parser_state)
		{
			// Note the use of the short-circuiting mechanism.
			return try_process_regular_cpt_rgb_or_hsv_colour_slice(tokens, parser_state) ||
					try_process_regular_cpt_colour_slice<GMTNameColourSpecification>(tokens, parser_state) ||
					try_process_rgb_or_hsv_bfn<RegularCptFileFormat>(tokens, parser_state) ||
					try_process_regular_cpt_colour_slice<CMYKColourSpecification>(tokens, parser_state) ||
					try_process_regular_cpt_colour_slice<GreyColourSpecification>(tokens, parser_state) ||
					try_process_regular_cpt_colour_slice<InvisibleColourSpecification>(tokens, parser_state) ||
					try_process_regular_cpt_colour_slice<PatternFillColourSpecification>(tokens, parser_state);
		}
	}
}


bool
GPlatesFileIO::CptReaderInternals::in_rgb_range(
		int value)
{
	return 0 <= value && value <= 255;
}


GPlatesGui::Colour
GPlatesFileIO::CptReaderInternals::make_rgb_colour(
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
GPlatesFileIO::CptReaderInternals::in_h_range(
		double value)
{
	return 0.0 <= value && value <= 360.0;
}


bool
GPlatesFileIO::CptReaderInternals::in_sv_range(
		double value)
{
	return 0.0 <= value && value <= 1.0;
}


GPlatesGui::Colour
GPlatesFileIO::CptReaderInternals::make_hsv_colour(
		double h, double s, double v)
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
GPlatesFileIO::CptReaderInternals::in_cmyk_range(
		int value)
{
	return 0 <= value && value <= 100;
}


GPlatesGui::Colour
GPlatesFileIO::CptReaderInternals::make_cmyk_colour(
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
GPlatesFileIO::CptReaderInternals::in_grey_range(
		int value)
{
	return 0 <= value && value <= 255;
}


GPlatesGui::Colour
GPlatesFileIO::CptReaderInternals::make_grey_colour(
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
GPlatesFileIO::CptReaderInternals::make_gmt_colour(
		const QString &name)
{
	boost::optional<GPlatesGui::Colour> result =
		GPlatesGui::GMTColourNames::instance().get_colour(name.toLower().toStdString());
	if (result)
	{
		return *result;
	}
	else
	{
		throw BadComponentsException();
	}
}


bool
GPlatesFileIO::CptReaderInternals::try_process_regular_cpt_rgb_or_hsv_colour_slice(
		const QStringList &tokens,
		ParserState<RegularCptFileFormat> &parser_state)
{
	if (parser_state.rgb)
	{
		return try_process_regular_cpt_colour_slice<RGBColourSpecification>(tokens, parser_state);
	}
	else
	{
		return try_process_regular_cpt_colour_slice<HSVColourSpecification>(tokens, parser_state);
	}
}


boost::optional<GPlatesGui::Colour>
GPlatesFileIO::CptReaderInternals::parse_categorical_fill(
		const QString &token)
{
	if (token.contains('/'))
	{
		// R/G/B triplet.
		QStringList subtokens = token.split('/');
		if (subtokens.size() != 3)
		{
			throw BadTokenException();
		}

		// Convert the colour.
		return convert_tokens<RGBColourSpecification>(subtokens);
	}
	else if (token.startsWith('#'))
	{
		// Hexadecimal RGB code.
		if (token.length() != 7)
		{
			throw BadTokenException();
		}

		// It should be of the form #xxyyzz.
		QStringList subtokens;
		subtokens.append(token.mid(1, 2));
		subtokens.append(token.mid(3, 2));
		subtokens.append(token.mid(5, 2));

		// Convert the colour.
		return convert_tokens<HexRGBColourSpecification>(subtokens);
	}
	else if (token.contains('-'))
	{
		// H-S-V triplet.
		QStringList subtokens = token.split('-');
		if (subtokens.size() != 3)
		{
			throw BadTokenException();
		}

		// Convert the colour.
		return convert_tokens<HSVColourSpecification>(subtokens);
	}
	else
	{
		// Try parsing it as a single integer.
		try
		{
			int grey = parse_token<int>(token);
			return make_grey_colour(grey);
		}
		catch (...)
		{
		}
		
		// See whether it's a GMT colour name.
		try
		{
			return make_gmt_colour(token);
		}
		catch (...)
		{
		}

		// If it starts with a p, let's assume it's a pattern fill.
		if (is_pattern_fill_specification(token))
		{
			throw PatternFillEncounteredException();
		}

		// Don't know what we were given...
		throw BadTokenException();
	}
}


bool
GPlatesFileIO::CptReaderInternals::is_pattern_fill_specification(
		const QString &token)
{
	// For now, we just say that it's a pattern fill if it starts with 'p'.
	// There's obviously more to it, but since we don't support pattern fills,
	// this test is sufficient for now.
	return token.startsWith('p');
}


boost::optional<QString>
GPlatesFileIO::CptReaderInternals::parse_regular_cpt_label(
		const QString &token)
{
	if (token.startsWith(';'))
	{
		return token.right(token.length() - 1);
	}
	else
	{
		throw BadTokenException();
	}
}

