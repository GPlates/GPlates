/* $Id$ */

/**
 * @file 
 * Contains the definition of the ColourSchemeFactory class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_COLOURSCHEMEFACTORY_H
#define GPLATES_GUI_COLOURSCHEMEFACTORY_H

#include "Colour.h"
#include "ColourScheme.h"

namespace GPlatesAppLogic
{
	class Reconstruct;
}

namespace GPlatesMaths
{
	class Real;
}

namespace GPlatesGui
{
	template<class T> class ColourPalette;

	namespace ColourSchemeFactory
	{
		/**
		 * Creates a colour scheme that colours all geometries with the given @a colour.
		 */
		ColourScheme::non_null_ptr_type
		create_single_colour_scheme(
				const Colour &colour);

		/**
		 * Creates the default plate id colour scheme; this colour scheme aims to
		 * give nearby plates vastly different colours.
		 */
		ColourScheme::non_null_ptr_type
		create_default_plate_id_colour_scheme();

		/**
		 * Creates the regional plate id colour scheme; this colour scheme aims to
		 * give plates in the same region (i.e. sharing the same leading digit) a
		 * different shade of the same colour.
		 */
		ColourScheme::non_null_ptr_type
		create_regional_plate_id_colour_scheme();

		/**
		 * Creates a colour scheme that colours geometries by their age based on the
		 * current reconstruction time, using default (rainbow-like) colours.
		 */
		ColourScheme::non_null_ptr_type
		create_default_age_colour_scheme(
				const GPlatesAppLogic::Reconstruct &reconstruct);

		/**
		 * Creates a colour scheme that colours geometries by their age based on the
		 * current reconstruction time, using shades of grey.
		 */
		ColourScheme::non_null_ptr_type
		create_monochrome_age_colour_scheme(
				const GPlatesAppLogic::Reconstruct &reconstruct);

		/**
		 * Creates a colour scheme that colours geometries by their age based on the
		 * current reconstruction time, using the provided @a palette.
		 */
		ColourScheme::non_null_ptr_type
		create_custom_age_colour_scheme(
				const GPlatesAppLogic::Reconstruct &reconstruct,
				ColourPalette<GPlatesMaths::Real> *palette);

		/**
		 * Creates a colour scheme that colours geometries by their underlying feature type.
		 */
		ColourScheme::non_null_ptr_type
		create_default_feature_colour_scheme();
		
	}
}

#endif  /* GPLATES_GUI_COLOURSCHEMEFACTORY_H */
