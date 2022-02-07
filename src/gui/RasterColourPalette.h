/* $Id$ */

/**
 * @file 
 * Contains colour palettes suitable for rasters.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_RASTERCOLOURPALETTE_H
#define GPLATES_GUI_RASTERCOLOURPALETTE_H

#include <utility>
#include <vector>
#include <boost/cstdint.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "Colour.h"
#include "ColourPalette.h"
#include "ColourPaletteVisitor.h"

#include "utils/ReferenceCount.h"
#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesGui
{
	/**
	 * RasterColourPalette is a convenience wrapper around a boost::variant over
	 * pointers to ColourPalette<int32_t>, ColourPalette<uint32_t> and
	 * ColourPalette<double>; i.e those types of ColourPalettes that can be used
	 * to colour non-RGBA rasters. If you have a ColourPalette that is not of
	 * one of these types that you would like to store in RasterColourPalette,
	 * run it through a ColourPaletteAdapter first. For convenience,
	 * RasterColourPalette is also able to represent a null colour palette.
	 */
	class RasterColourPalette :
			public GPlatesUtils::ReferenceCount<RasterColourPalette>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<RasterColourPalette> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const RasterColourPalette> non_null_ptr_to_const_type;


		/**
		 * Wrap a ColourPalette<> in a RasterColourPalette.
		 */
		template<typename PaletteKeyType>
		static
		non_null_ptr_type
		create(
				const typename ColourPalette<PaletteKeyType>::non_null_ptr_type &colour_palette)
		{
			return new RasterColourPalette(colour_palette);
		}

		/**
		 * Create an empty RasterColourPalette.
		 */
		static
		non_null_ptr_type
		create()
		{
			return new RasterColourPalette();
		}


		/**
		 * Accept a standard 'ConstColourPaletteVisitor' (as opposed to a boost variant visitor.
		 */
		void
		accept_visitor(
				ConstColourPaletteVisitor &colour_palette_visitor) const;

		/**
		 * Accept a standard 'ColourPaletteVisitor' (as opposed to a boost variant visitor.
		 */
		void
		accept_visitor(
				ColourPaletteVisitor &colour_palette_visitor) const;


		struct empty {  };

		typedef boost::variant<
			empty, // boost::variant requires the first type be default-constructible; signifies no colour palette.
			ColourPalette<boost::int32_t>::non_null_ptr_type,
			ColourPalette<boost::uint32_t>::non_null_ptr_type,
			ColourPalette<double>::non_null_ptr_type
		> variant_type;


		/**
		 * Apply a static visitor to the boost::variant wrapped in this instance.
		 */
		template<class StaticVisitorType>
		typename StaticVisitorType::result_type
		apply_visitor(
				const StaticVisitorType &visitor) const
		{
			return boost::apply_visitor(visitor, d_colour_palette);
		}

		/**
		 * Apply a static visitor to the boost::variant wrapped in this instance.
		 */
		template<class StaticVisitorType>
		typename StaticVisitorType::result_type
		apply_visitor(
				StaticVisitorType &visitor) const
		{
			return boost::apply_visitor(visitor, d_colour_palette);
		}

	private:

		explicit
		RasterColourPalette() :
			d_colour_palette(empty())
		{  }

		template<class ColourPalettePointerType>
		explicit
		RasterColourPalette(
				const ColourPalettePointerType &colour_palette) :
			d_colour_palette(colour_palette)
		{  }

		variant_type d_colour_palette;
	};


	namespace RasterColourPaletteType
	{
		enum Type
		{
			INVALID,
			INT32,
			UINT32,
			DOUBLE
		};


		/**
		 * Returns the type of the @a ColourPalette encapsulated inside a
		 * @a RasterColourPalette.
		 *
		 * The preferred way of switching on the type of a @a RasterColourPalette is
		 * through the use of static visitors applied using @a apply_visitor, but this
		 * exists as an alternative.
		 */
		RasterColourPaletteType::Type
		get_type(
				const RasterColourPalette &raster_colour_palette);
	}


	namespace RasterColourPaletteExtract
	{
		/**
		 * Returns the @a ColourPalette, of the specified type, encapsulated inside a @a RasterColourPalette.
		 *
		 * Returns none if the contained palette type is not 'ColourPalette<PaletteKeyType>'.
		 *
		 * The preferred way of switching on the type of a @a RasterColourPalette is
		 * through the use of static visitors applied using @a apply_visitor, but this
		 * exists as an alternative.
		 */
		template <typename PaletteKeyType>
		boost::optional<typename ColourPalette<PaletteKeyType>::non_null_ptr_type>
		get_colour_palette(
				const RasterColourPalette &raster_colour_palette);
	}
}


//
// Implementation
//

namespace GPlatesGui
{
	namespace RasterColourPaletteExtract
	{
		namespace Implementation
		{
			template <typename PaletteKeyType>
			class ExtractVisitor :
					public boost::static_visitor<
								boost::optional<typename ColourPalette<PaletteKeyType>::non_null_ptr_type> >
			{
			public:

				// Look for a specific ColourPalette type (specifically with key 'PaletteKeyType')...
				boost::optional<typename ColourPalette<PaletteKeyType>::non_null_ptr_type>
				operator()(
						const typename ColourPalette<PaletteKeyType>::non_null_ptr_type &colour_palette) const
				{
					return colour_palette;
				}

				// General operator catches everything else...
				template <typename VariantBoundedType>
				boost::optional<typename ColourPalette<PaletteKeyType>::non_null_ptr_type>
				operator()(
						const VariantBoundedType &) const
				{
					return boost::none;
				}
			};
		}

		template <typename PaletteKeyType>
		boost::optional<typename ColourPalette<PaletteKeyType>::non_null_ptr_type>
		get_colour_palette(
				const RasterColourPalette &raster_colour_palette)
		{
			return boost::apply_visitor(
					Implementation::ExtractVisitor<PaletteKeyType>(),
					raster_colour_palette);
		}
	}
}

#endif  /* GPLATES_GUI_RASTERCOLOURPALETTE_H */
