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

#include <boost/cstdint.hpp>
#include <boost/variant.hpp>

#include "ColourPalette.h"
#include "CptColourPalette.h"

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

		struct empty {  };

		typedef boost::variant<
			empty, // boost::variant requires the first type be default-constructible; signifies no colour palette.
			ColourPalette<boost::int32_t>::non_null_ptr_type,
			ColourPalette<boost::uint32_t>::non_null_ptr_type,
			ColourPalette<double>::non_null_ptr_type
		> variant_type;

		template<typename PaletteKeyType>
		static
		non_null_ptr_type
		create(
				const typename ColourPalette<PaletteKeyType>::non_null_ptr_type &colour_palette)
		{
			return new RasterColourPalette(colour_palette);
		}

		static
		non_null_ptr_type
		create()
		{
			return new RasterColourPalette();
		}

		/**
		 * Apply a static visitor to the boost::variant wrapped in this instance.
		 */
		template<class StaticVisitorType>
		typename StaticVisitorType::result_type
		apply_visitor(
				const StaticVisitorType &visitor)
		{
			return boost::apply_visitor(visitor, d_colour_palette);
		}

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
				const RasterColourPalette &colour_palette);
	}


	/**
	 * The default colour palette used to colour non-RGBA rasters upon file loading.
	 * The colour palette covers a range of values up to two standard deviations
	 * away from the mean.
	 */
	class DefaultRasterColourPalette :
			public ColourPalette<double>
	{
	public:

		/**
		 * Constructs a DefaultRasterColourPalette, given the mean and the
		 * standard deviation of the values in the raster.
		 */
		static
		non_null_ptr_type
		create(
				double mean,
				double std_dev);

		virtual
		boost::optional<Colour>
		get_colour(
				double value) const;

		virtual
		void
		accept_visitor(
				ConstColourPaletteVisitor &visitor) const
		{
			visitor.visit_default_raster_colour_palette(*this);
		}

		virtual
		void
		accept_visitor(
				ColourPaletteVisitor &visitor)
		{
			visitor.visit_default_raster_colour_palette(*this);
		}

		RegularCptColourPalette::non_null_ptr_to_const_type
		get_inner_palette() const
		{
			return d_inner_palette;
		}

		double
		get_mean() const
		{
			return d_mean;
		}

		double
		get_std_dev() const
		{
			return d_std_dev;
		}

	private:

		DefaultRasterColourPalette(
				double mean,
				double std_dev);

		RegularCptColourPalette::non_null_ptr_type d_inner_palette;
		double d_mean;
		double d_std_dev;

		static const int NUM_STD_DEV_AWAY_FROM_MEAN = 2;
	};
}

#endif  /* GPLATES_GUI_RASTERCOLOURPALETTE_H */
