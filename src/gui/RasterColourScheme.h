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

#ifndef GPLATES_GUI_RASTERCOLOURSCHEME_H
#define GPLATES_GUI_RASTERCOLOURSCHEME_H

#include <boost/cstdint.hpp>
#include <boost/variant.hpp>

#include "ColourPalette.h"

#include "global/unicode.h"

#include "property-values/TextContent.h"

#include "utils/ReferenceCount.h"
#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesGui
{
	// FIXME: Decouple the storage of the colour palette from the band name.

	/**
	 * RasterColourScheme stores a band name and a colour palette to be applied to
	 * non-RGBA rasters.
	 *
	 * Because ColourPalette is templated by the type of the look-up key, this
	 * class contains functionality to hold different types of ColourPalette.
	 */
	class RasterColourScheme :
			public GPlatesUtils::ReferenceCount<RasterColourScheme>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<RasterColourScheme> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const RasterColourScheme> non_null_ptr_to_const_type;

		typedef GPlatesPropertyValues::TextContent band_name_string_type;


		/**
		 * The type of the ColourPalette encapsulated by RasterColourScheme.
		 */
		enum ColourPaletteType
		{
			USE_DEFAULT, // Use default colour palette instead of user supplied one.
			INT32,
			UINT32,
			DOUBLE
		};

		/**
		 * Creates a new instance of RasterColourScheme, to colour rasters according
		 * to the given @a band_name, using the default colour palette for rasters.
		 */
		static
		non_null_ptr_type
		create(
				const UnicodeString &band_name);

		/**
		 * Creates a new instance of RasterColourScheme, to colour rasters according
		 * to the given @a band_name, using the default colour palette for rasters.
		 */
		static
		non_null_ptr_type
		create(
				const band_name_string_type &band_name);

		/**
		 * Creates a new instance of RasterColourScheme, to colour rasters according
		 * to the given @a band_name, using the given @a colour_palette.
		 *
		 * The template parameter @a PaletteKeyType must be one of boost::int32_t,
		 * boost::uint32_t or double. If your ColourPalette isn't one of these types,
		 * consider using ColourSchemeAdapter first.
		 */
		template<typename PaletteKeyType>
		static
		non_null_ptr_type
		create(
				const UnicodeString &band_name,
				const typename ColourPalette<PaletteKeyType>::non_null_ptr_type &colour_palette)
		{
			return new RasterColourScheme(band_name, colour_palette);
		}

		/**
		 * Creates a new instance of RasterColourScheme, using the same colour
		 * palette as @a existing, but using the new @a band_name.
		 */
		static
		non_null_ptr_type
		create_from_existing(
				const non_null_ptr_type &existing,
				const UnicodeString &band_name);

		const band_name_string_type &
		get_band_name() const;

		template<typename PaletteKeyType>
		typename ColourPalette<PaletteKeyType>::non_null_ptr_type
		get_colour_palette()
		{
			return boost::get<typename ColourPalette<PaletteKeyType>::non_null_ptr_type>(
					d_colour_palette);
		}

		template<typename PaletteKeyType>
		typename ColourPalette<PaletteKeyType>::non_null_ptr_to_const_type
		get_colour_palette() const
		{
			return boost::get<typename ColourPalette<PaletteKeyType>::non_null_ptr_type>(
					d_colour_palette);
		}

		ColourPaletteType
		get_type() const;

	private:

		RasterColourScheme(
				const band_name_string_type &band_name);

		RasterColourScheme(
				const UnicodeString &band_name,
				const ColourPalette<boost::int32_t>::non_null_ptr_type &colour_palette);

		RasterColourScheme(
				const UnicodeString &band_name,
				const ColourPalette<boost::uint32_t>::non_null_ptr_type &colour_palette);

		RasterColourScheme(
				const UnicodeString &band_name,
				const ColourPalette<double>::non_null_ptr_type &colour_palette);

		typedef boost::variant
		<
			int, // Just so that we can default construct d_colour_palette.
			ColourPalette<boost::int32_t>::non_null_ptr_type,
			ColourPalette<boost::uint32_t>::non_null_ptr_type,
			ColourPalette<double>::non_null_ptr_type
		> variant_type;

		band_name_string_type d_band_name;
		variant_type d_colour_palette;
		ColourPaletteType d_colour_palette_type;
	};
}

#endif  /* GPLATES_GUI_RASTERCOLOURSCHEME_H */
