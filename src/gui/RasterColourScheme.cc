/* $Id$ */

/**
 * @file 
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include "RasterColourScheme.h"


GPlatesGui::RasterColourScheme::non_null_ptr_type
GPlatesGui::RasterColourScheme::create(
		const UnicodeString &band_name)
{
	return new RasterColourScheme(GPlatesPropertyValues::TextContent(band_name));
}


GPlatesGui::RasterColourScheme::non_null_ptr_type
GPlatesGui::RasterColourScheme::create(
		const band_name_string_type &band_name)
{
	return new RasterColourScheme(band_name);
}


GPlatesGui::RasterColourScheme::non_null_ptr_type
GPlatesGui::RasterColourScheme::create_from_existing(
		const non_null_ptr_type &existing,
		const UnicodeString &band_name)
{
	switch (existing->d_colour_palette_type)
	{
		case INT32:
			return create<boost::int32_t>(
					band_name,
					existing->get_colour_palette<boost::int32_t>());

		case UINT32:
			return create<boost::uint32_t>(
					band_name,
					existing->get_colour_palette<boost::uint32_t>());

		case DOUBLE:
			return create<double>(
					band_name,
					existing->get_colour_palette<double>());

		default:
			return create(band_name);
	}
}


GPlatesGui::RasterColourScheme::RasterColourScheme(
		const band_name_string_type &band_name) :
	d_band_name(band_name),
	d_colour_palette_type(USE_DEFAULT)
{  }


GPlatesGui::RasterColourScheme::RasterColourScheme(
		const UnicodeString &band_name,
		const ColourPalette<boost::int32_t>::non_null_ptr_type &colour_palette) :
	d_band_name(band_name),
	d_colour_palette(colour_palette),
	d_colour_palette_type(INT32)
{  }


GPlatesGui::RasterColourScheme::RasterColourScheme(
		const UnicodeString &band_name,
		const ColourPalette<boost::uint32_t>::non_null_ptr_type &colour_palette) :
	d_band_name(band_name),
	d_colour_palette(colour_palette),
	d_colour_palette_type(UINT32)
{  }


GPlatesGui::RasterColourScheme::RasterColourScheme(
		const UnicodeString &band_name,
		const ColourPalette<double>::non_null_ptr_type &colour_palette) :
	d_band_name(band_name),
	d_colour_palette(colour_palette),
	d_colour_palette_type(DOUBLE)
{  }


const GPlatesGui::RasterColourScheme::band_name_string_type &
GPlatesGui::RasterColourScheme::get_band_name() const
{
	return d_band_name;
}


GPlatesGui::RasterColourScheme::ColourPaletteType
GPlatesGui::RasterColourScheme::get_type() const
{
	return d_colour_palette_type;
}

