/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_COLOURPALETTEUTILS_H
#define GPLATES_GUI_COLOURPALETTEUTILS_H

#include <utility>
#include <boost/optional.hpp>
#include <QString>

#include "AgeColourPalettes.h"
#include "ColourPalette.h"
#include "ColourPaletteVisitor.h"
#include "CptColourPalette.h"
#include "RasterColourPalette.h"

#include "file-io/ReadErrorAccumulation.h"


namespace GPlatesGui
{
	namespace ColourPaletteUtils
	{
		/**
		 * Reads the specified palette file and returns a raster colour palette.
		 *
		 * If @a allow_integer_colour_palette is true then integer (categorical) colour palettes
		 * are allowed in addition to real-valued (regular) palettes, otherwise just real-valued.
		 *
		 * For example, if the colour palette is integer-valued, it will only colour rasters that
		 * are themselves integer-valued. Whereas if the colour palette is real-valued, it will colour
		 * rasters that are either integer or real-valued. Thus an integer-valued colour palette
		 * is not suitable for any data that is real-valued (such as 3D scalar fields or
		 * reconstructed scalar coverages).
		 *
		 * Any read errors are accumulated into @a read_errors.
		 *
		 * Note: RasterColourPaletteType::get_type() can be used to detect if a colour palette
		 * was successfully read (ie, is not invalid).
		 */
		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type
		read_cpt_raster_colour_palette(
				const QString &palette_file_name,
				bool allow_integer_colour_palette,
				GPlatesFileIO::ReadErrorAccumulation &read_errors);


		/**
		 * Returns the range of the specified colour palette.
		 *
		 * Returns none if the contained palette type has no range (eg, is not a numeric type).
		 */
		template <typename PaletteKeyType>
		boost::optional< std::pair<double, double> >
		get_range(
				const ColourPalette<PaletteKeyType> &colour_palette);

		/**
		 * An overload accepting RasterColourPalette instead of ColourPalette<>.
		 */
		boost::optional< std::pair<double, double> >
		get_range(
				const RasterColourPalette &raster_colour_palette);
	}
}


//
// Implementation
//

namespace GPlatesGui
{
	namespace ColourPaletteUtils
	{
		namespace Implementation
		{
			/**
			 * Extract the range of values covered by a colour palette, which is also
			 * returned, adapted into an integer colour palette.
			 */
			class RangeVisitor :
					public ConstColourPaletteVisitor
			{
			public:

				boost::optional< std::pair<double, double> >
				get_range() const
				{
					return d_range;
				}


				virtual
				void
				visit_age_colour_palette(
						const AgeColourPalette &colour_palette)
				{
					const std::pair<GPlatesMaths::Real, GPlatesMaths::Real> range = colour_palette.get_range();
					d_range = std::make_pair(range.first.dval(), range.second.dval());
				}

				virtual
				void
				visit_int32_categorical_cpt_colour_palette(
						const CategoricalCptColourPalette<boost::int32_t> &colour_palette)
				{
					if (colour_palette.get_range())
					{
						d_range = std::pair<double, double>(colour_palette.get_range().get());
					}
				}

				virtual
				void
				visit_uint32_categorical_cpt_colour_palette(
						const CategoricalCptColourPalette<boost::uint32_t> &colour_palette)
				{
					if (colour_palette.get_range())
					{
						d_range = std::pair<double, double>(colour_palette.get_range().get());
					}
				}

				virtual
				void
				visit_regular_cpt_colour_palette(
						const RegularCptColourPalette &colour_palette)
				{
					if (colour_palette.get_range())
					{
						const std::pair<GPlatesMaths::Real, GPlatesMaths::Real> range =
								colour_palette.get_range().get();
						d_range = std::make_pair(range.first.dval(), range.second.dval());
					}
				}

			private:

				boost::optional< std::pair<double, double> > d_range;
			};
		}


		template <typename PaletteKeyType>
		boost::optional< std::pair<double, double> >
		get_range(
				const ColourPalette<PaletteKeyType> &colour_palette)
		{
			Implementation::RangeVisitor visitor;
			colour_palette.accept_visitor(visitor);
			return visitor.get_range();
		}


		inline
		boost::optional< std::pair<double, double> >
		get_range(
				const RasterColourPalette &raster_colour_palette)
		{
			Implementation::RangeVisitor visitor;
			raster_colour_palette.accept_visitor(visitor);
			return visitor.get_range();
		}
	}
}

#endif // GPLATES_GUI_COLOURPALETTEUTILS_H
