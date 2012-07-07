/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_COLOURPALETTEVISITOR_H
#define GPLATES_GUI_COLOURPALETTEVISITOR_H

#include <boost/cstdint.hpp>

#include "utils/SetConst.h"


namespace GPlatesGui 
{
	// Forward declarations of supported ColourPalette derivations.
	class AgeColourPalette;
	template <typename> class CategoricalCptColourPalette;
	class DefaultPlateIdColourPalette;
	class DefaultRasterColourPalette;
	class DefaultNormalisedRasterColourPalette;
	class FeatureTypeColourPalette;
	class RegionalPlateIdColourPalette;
	class RegularCptColourPalette;

	/**
	 * This class is a base class for visitors that visit ColourPalettes.
	 * For convenience, typedefs are provided below to cover the const and non-const cases.
	 */
	template<bool Const>
	class ColourPaletteVisitorBase
	{
	public:

		// Typedefs to give the supported derivations the appropriate const-ness.
		typedef typename GPlatesUtils::SetConst<AgeColourPalette, Const>::type age_colour_palette_type;
		typedef typename GPlatesUtils::SetConst<DefaultPlateIdColourPalette, Const>::type default_plate_id_colour_palette_type;
		typedef typename GPlatesUtils::SetConst<DefaultRasterColourPalette, Const>::type default_raster_colour_palette_type;
		typedef typename GPlatesUtils::SetConst<DefaultNormalisedRasterColourPalette, Const>::type default_normalised_raster_colour_palette_type;
		typedef typename GPlatesUtils::SetConst<FeatureTypeColourPalette, Const>::type feature_type_colour_palette_type;
		typedef typename GPlatesUtils::SetConst<RegionalPlateIdColourPalette, Const>::type regional_plate_id_colour_palette_type;
		typedef typename GPlatesUtils::SetConst<RegularCptColourPalette, Const>::type regular_cpt_colour_palette_type;
		typedef typename GPlatesUtils::SetConst<CategoricalCptColourPalette<boost::int32_t>, Const>::type int32_categorical_cpt_colour_palette_type;
		typedef typename GPlatesUtils::SetConst<CategoricalCptColourPalette<boost::uint32_t>, Const>::type uint32_categorical_cpt_colour_palette_type;

		virtual
		~ColourPaletteVisitorBase()
		{  }

		virtual
		void
		visit_age_colour_palette(
				age_colour_palette_type &)
		{  }

		virtual
		void
		visit_default_plate_id_colour_palette(
				default_plate_id_colour_palette_type &)
		{  }

		virtual
		void
		visit_default_raster_colour_palette(
				default_raster_colour_palette_type &)
		{  }

		virtual
		void
		visit_default_normalised_raster_colour_palette(
				default_normalised_raster_colour_palette_type &)
		{  }

		virtual
		void
		visit_feature_type_colour_palette(
				feature_type_colour_palette_type &)
		{  }

		virtual
		void
		visit_regional_plate_id_colour_palette(
				regional_plate_id_colour_palette_type &)
		{  }

		virtual
		void
		visit_regular_cpt_colour_palette(
				regular_cpt_colour_palette_type &)
		{  }

		virtual
		void
		visit_int32_categorical_cpt_colour_palette(
				int32_categorical_cpt_colour_palette_type &)
		{  }

		virtual
		void
		visit_uint32_categorical_cpt_colour_palette(
				uint32_categorical_cpt_colour_palette_type &)
		{  }

	protected:

		explicit
		ColourPaletteVisitorBase()
		{  }
	};


	/**
	 * This is the base class for visitors that visit const ColourPalettes.
	 */
	typedef ColourPaletteVisitorBase<true> ConstColourPaletteVisitor;

	/**
	 * This is the base class for visitors that visit non-const ColourPalettes.
	 */
	typedef ColourPaletteVisitorBase<false> ColourPaletteVisitor;
}

#endif  /* GPLATES_GUI_COLOURPALETTEVISITOR_H */
