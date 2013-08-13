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

#include <vector>
#include <boost/cstdint.hpp>
#include <boost/variant.hpp>

#include "Colour.h"
#include "ColourPalette.h"
#include "ColourPaletteVisitor.h"
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
				StaticVisitorType &visitor)
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
		{  
		}

		template<class ColourPalettePointerType>
		explicit
		RasterColourPalette(
				const ColourPalettePointerType &colour_palette) :
			d_colour_palette(colour_palette)
		{ 
		}

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
	 * A function to get a colour out of the palette 
	 */
	namespace RasterColourPaletteColour
	{
		boost::optional<GPlatesGui::Colour>
		get_colour(
				const RasterColourPalette &colour_palette,
				const double &value);
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

		double
		get_lower_bound() const;

		double
		get_upper_bound() const;

		const std::vector<ColourSlice> &
		get_colour_slices() const
		{
			return d_inner_palette->get_entries();
		}

		boost::optional<Colour>
		get_background_colour() const
		{
			return d_inner_palette->get_background_colour();
		}

		boost::optional<Colour>
		get_foreground_colour() const
		{
			return d_inner_palette->get_foreground_colour();
		}

		boost::optional<Colour>
		get_nan_colour() const
		{
			return d_inner_palette->get_nan_colour();
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

	/**
	 * The default colour palette used to colour non-RGBA rasters upon file loading.
	 * The colour palette covers a range of values up to two standard deviations
	 * away from the mean.
	 */
	class UserColourPalette :
			public ColourPalette<double>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<UserColourPalette> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const UserColourPalette> non_null_ptr_to_const_type;

		/**
		 * Constructs a UserColourPalette, given the max and the
		 * standard deviation of the values in the raster.
		 */
		static
		non_null_ptr_type
		create();

		static
		non_null_ptr_type
		create(
				double range1_max,
				double range1_min,
				double range2_max,
				double range2_min,
				GPlatesGui::Colour max_colour,
				GPlatesGui::Colour mid_colour,
				GPlatesGui::Colour min_colour);

		virtual
		boost::optional<Colour>
		get_colour(
				double value) const;

		virtual
		void
		accept_visitor(
				ConstColourPaletteVisitor &visitor) const
		{
			visitor.visit_user_colour_palette(*this);
		}

		virtual
		void
		accept_visitor(
				ColourPaletteVisitor &visitor)
		{
			visitor.visit_user_colour_palette(*this);
		}

		double
		get_max() const
		{
			return d_range1_max;
		}

		double
		get_min() const
		{
			return d_range2_min;
		}

		double
		get_lower_bound() const;

		double
		get_upper_bound() const;

		const std::vector<ColourSlice> &
		get_colour_slices() const
		{
			return d_inner_palette->get_entries();
		}

		boost::optional<Colour>
		get_background_colour() const
		{
			return d_inner_palette->get_background_colour();
		}

		boost::optional<Colour>
		get_foreground_colour() const
		{
			return d_inner_palette->get_foreground_colour();
		}

		boost::optional<Colour>
		get_nan_colour() const
		{
			return d_inner_palette->get_nan_colour();
		}

	private:

		UserColourPalette(
				double range1_max,
				double range1_min,
				double range2_max,
				double range2_min,
				GPlatesGui::Colour max_colour,
				GPlatesGui::Colour mid_colour,
				GPlatesGui::Colour min_colour);

		RegularCptColourPalette::non_null_ptr_type d_inner_palette;

		double d_range1_max;
		double d_range1_min;
		double d_range2_max;
		double d_range2_min;
		GPlatesGui::Colour d_max_colour;
		GPlatesGui::Colour d_mid_colour;
		GPlatesGui::Colour d_min_colour;
	};


	/**
	 * The default 3D scalar field colour palette used when colouring by scalar value.
	 *
	 * The colour palette covers the range of values [0, 1].
	 * This palette is useful when the mapping to a specific scalar field scalar range is done
	 * elsewhere (such as via the GPU hardware) - then the range of scalar values (such as
	 * mean +/- std_deviation) that map to [0,1] can be handled by the GPU hardware (requires more
	 * advanced hardware though - but 3D scalar fields rely on that anyway).
	 */
	class DefaultScalarFieldScalarColourPalette :
			public ColourPalette<double>
	{
	public:

		/**
		 * Constructs a DefaultScalarFieldScalarColourPalette.
		 */
		static
		non_null_ptr_type
		create();

		virtual
		boost::optional<Colour>
		get_colour(
				double value) const;

		virtual
		void
		accept_visitor(
				ConstColourPaletteVisitor &visitor) const
		{
			visitor.visit_default_scalar_field_scalar_colour_palette(*this);
		}

		virtual
		void
		accept_visitor(
				ColourPaletteVisitor &visitor)
		{
			visitor.visit_default_scalar_field_scalar_colour_palette(*this);
		}

		static
		double
		get_lower_bound()
		{
			return 0;
		}

		static
		double
		get_upper_bound()
		{
			return 1;
		}

		const std::vector<ColourSlice> &
		get_colour_slices() const
		{
			return d_inner_palette->get_entries();
		}

		boost::optional<Colour>
		get_background_colour() const
		{
			return d_inner_palette->get_background_colour();
		}

		boost::optional<Colour>
		get_foreground_colour() const
		{
			return d_inner_palette->get_foreground_colour();
		}

		boost::optional<Colour>
		get_nan_colour() const
		{
			return d_inner_palette->get_nan_colour();
		}

	private:

		DefaultScalarFieldScalarColourPalette();

		RegularCptColourPalette::non_null_ptr_type d_inner_palette;
	};


	/**
	 * The default 3D scalar field colour palette used when colouring by gradient magnitude.
	 *
	 * The colour palette covers the range of values [-1, 1].
	 * When the back side of an isosurface (towards the half-space with lower scalar values)
	 * is visible then the gradient magnitude is mapped to the range [0,1] and the front side
	 * is mapped to the range [-1,0].
	 *
	 * Like @a DefaultScalarFieldScalarColourPalette this palette is useful for more advanced GPU
	 * hardware that can explicitly handle the re-mapping of gradient magnitude ranges to [-1,1].
	 */
	class DefaultScalarFieldGradientColourPalette :
			public ColourPalette<double>
	{
	public:

		/**
		 * Constructs a DefaultScalarFieldGradientColourPalette.
		 */
		static
		non_null_ptr_type
		create();

		virtual
		boost::optional<Colour>
		get_colour(
				double value) const;

		virtual
		void
		accept_visitor(
				ConstColourPaletteVisitor &visitor) const
		{
			visitor.visit_default_scalar_field_gradient_colour_palette(*this);
		}

		virtual
		void
		accept_visitor(
				ColourPaletteVisitor &visitor)
		{
			visitor.visit_default_scalar_field_gradient_colour_palette(*this);
		}

		static
		double
		get_lower_bound()
		{
			return -1;
		}

		static
		double
		get_upper_bound()
		{
			return 1;
		}

		const std::vector<ColourSlice> &
		get_colour_slices() const
		{
			return d_inner_palette->get_entries();
		}

		boost::optional<Colour>
		get_background_colour() const
		{
			return d_inner_palette->get_background_colour();
		}

		boost::optional<Colour>
		get_foreground_colour() const
		{
			return d_inner_palette->get_foreground_colour();
		}

		boost::optional<Colour>
		get_nan_colour() const
		{
			return d_inner_palette->get_nan_colour();
		}

	private:

		DefaultScalarFieldGradientColourPalette();

		RegularCptColourPalette::non_null_ptr_type d_inner_palette;
	};
}


#endif  /* GPLATES_GUI_RASTERCOLOURPALETTE_H */
