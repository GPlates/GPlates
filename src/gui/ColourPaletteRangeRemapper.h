/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_COLOURPALETTERANGEREMAPPER_H
#define GPLATES_GUI_COLOURPALETTERANGEREMAPPER_H

#include <utility>
#include <boost/optional.hpp>

#include "ColourPalette.h"
#include "ColourPaletteAdapter.h"
#include "ColourPaletteVisitor.h"
#include "CptColourPalette.h"
#include "RasterColourPalette.h"

#include "maths/Real.h"
#include "maths/MathsUtils.h"

#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesGui
{
	/**
	 * Remaps the value range of the specified colour palette.
	 *
	 * Returns none (currently) if @a colour_palette is not a @a RegularCptColourPalette.
	 * This is because each ColourSlice in palette is remapped and only @a RegularCptColourPalette has them.
	 *
	 * The colours in the returned palette are the same but the colour slice ranges are
	 * scaled and translated to map to the new range.
	 */
	template <typename KeyType>
	boost::optional<ColourPalette<double>::non_null_ptr_type>
	remap_colour_palette_range(
			const GPlatesUtils::non_null_intrusive_ptr< ColourPalette<KeyType> > &colour_palette,
			const double &remapped_lower_bound,
			const double &remapped_upper_bound);


	/**
	 * Remaps the value range of the specified raster colour palette.
	 *
	 * Same as the other overload but accepts a @a RasterColourPalette.
	 */
	boost::optional<ColourPalette<double>::non_null_ptr_type>
	remap_colour_palette_range(
			const RasterColourPalette::non_null_ptr_to_const_type &colour_palette,
			const double &remapped_lower_bound,
			const double &remapped_upper_bound);

	
	namespace ColourPaletteRangeRemapperInternals
	{
		inline
		double
		get_double_value(
				double d)
		{
			return d;
		}

		inline
		double
		get_double_value(
				GPlatesMaths::Real r)
		{
			return r.dval();
		}

		class RangeRemapperVisitor :
				public ConstColourPaletteVisitor
		{
		public:

			RangeRemapperVisitor(
					const double &remapped_lower_bound,
					const double &remapped_upper_bound) :
				d_remapped_lower_bound(remapped_lower_bound),
				d_remapped_upper_bound(remapped_upper_bound)
			{  }


			boost::optional<ColourPalette<double>::non_null_ptr_type>
			get_remapped_colour_palette() const
			{
				return d_remapped_colour_palette;
			}


			virtual
			void
			visit_regular_cpt_colour_palette(
					const RegularCptColourPalette &colour_palette)
			{
				boost::optional< std::pair<GPlatesMaths::Real, GPlatesMaths::Real> > range =
						colour_palette.get_range();
				if (!range)
				{
					return;
				}

				generate_remapped_colour_palette(
						colour_palette.get_background_colour(),
						colour_palette.get_foreground_colour(),
						colour_palette.get_nan_colour(),
						get_double_value(range->first),
						get_double_value(range->second),
						colour_palette.get_entries());
			}

		private:

			double d_remapped_lower_bound;
			double d_remapped_upper_bound;
			boost::optional<ColourPalette<double>::non_null_ptr_type> d_remapped_colour_palette;


			void
			generate_remapped_colour_palette(
					const boost::optional<Colour> &background_colour,
					const boost::optional<Colour> &foreground_colour,
					const boost::optional<Colour> &nan_colour,
					double lower_bound,
					double upper_bound,
					const std::vector<ColourSlice> &colour_slices)
			{
				double inv_range = 0.0;
				if (!GPlatesMaths::are_almost_exactly_equal(lower_bound, upper_bound))
				{
					if (lower_bound > upper_bound)
					{
						std::swap(lower_bound, upper_bound);
					}
					inv_range = 1.0 / (upper_bound - lower_bound);
				}

				RegularCptColourPalette::non_null_ptr_type remapped_colour_palette =
						RegularCptColourPalette::create();

				if (background_colour)
				{
					remapped_colour_palette->set_background_colour(background_colour.get());
				}
				if (foreground_colour)
				{
					remapped_colour_palette->set_foreground_colour(foreground_colour.get());
				}
				if (nan_colour)
				{
					remapped_colour_palette->set_nan_colour(nan_colour.get());
				}

				for (unsigned int n = 0; n < colour_slices.size(); ++n)
				{
					const ColourSlice &colour_slice = colour_slices[n];
					const ColourSlice::value_type lower_interp = (colour_slice.lower_value() - lower_bound) * inv_range;
					const ColourSlice::value_type upper_interp = (colour_slice.upper_value() - lower_bound) * inv_range;

					ColourSlice remapped_colour_slice(colour_slice);
					remapped_colour_slice.set_lower_value(
							d_remapped_lower_bound + lower_interp * (d_remapped_upper_bound - d_remapped_lower_bound));
					remapped_colour_slice.set_upper_value(
							d_remapped_lower_bound + upper_interp * (d_remapped_upper_bound - d_remapped_lower_bound));

					remapped_colour_palette->add_entry(remapped_colour_slice);
				}

				d_remapped_colour_palette =
						GPlatesGui::convert_colour_palette<GPlatesMaths::Real, double>(
								remapped_colour_palette, RealToBuiltInConverter<double>());
			}

		};

		class RasterColourPaletteRangeRemapperVisitor :
				public boost::static_visitor< boost::optional<ColourPalette<double>::non_null_ptr_type> >
		{
		public:

			RasterColourPaletteRangeRemapperVisitor(
					const double &remapped_lower_bound,
					const double &remapped_upper_bound) :
				d_remapped_lower_bound(remapped_lower_bound),
				d_remapped_upper_bound(remapped_upper_bound)
			{  }


			boost::optional<ColourPalette<double>::non_null_ptr_type>
			operator()(
					const GPlatesGui::RasterColourPalette::empty &) const
			{
				return boost::none;
			}


			template<class ColourPaletteType>
			boost::optional<ColourPalette<double>::non_null_ptr_type>
			operator()(
					const GPlatesUtils::non_null_intrusive_ptr<ColourPaletteType> &colour_palette) const
			{
				return remap_colour_palette_range(colour_palette, d_remapped_lower_bound, d_remapped_upper_bound);
			}

		private:

			double d_remapped_lower_bound;
			double d_remapped_upper_bound;

		};
	}


	template <typename KeyType>
	boost::optional<ColourPalette<double>::non_null_ptr_type>
	remap_colour_palette_range(
			const GPlatesUtils::non_null_intrusive_ptr< ColourPalette<KeyType> > &colour_palette,
			const double &remapped_lower_bound,
			const double &remapped_upper_bound)
	{
		ColourPaletteRangeRemapperInternals::RangeRemapperVisitor visitor(
				remapped_lower_bound,
				remapped_upper_bound);
		colour_palette->accept_visitor(visitor);

		return visitor.get_remapped_colour_palette();
	}


	inline
	boost::optional<ColourPalette<double>::non_null_ptr_type>
	remap_colour_palette_range(
			const RasterColourPalette::non_null_ptr_to_const_type &colour_palette,
			const double &remapped_lower_bound,
			const double &remapped_upper_bound)
	{
		ColourPaletteRangeRemapperInternals::RasterColourPaletteRangeRemapperVisitor visitor(
				remapped_lower_bound,
				remapped_upper_bound);
		return colour_palette->apply_visitor(visitor);
	}
}

#endif // GPLATES_GUI_COLOURPALETTERANGEREMAPPER_H
