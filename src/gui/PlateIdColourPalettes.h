/* $Id$ */

/**
 * @file 
 * Contains the definition of the DefaultPlateIdColourPalette and
 * RegionalPlateIdColourPalette classes.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_PLATEIDCOLOURPALETTES_H
#define GPLATES_GUI_PLATEIDCOLOURPALETTES_H

#include "Colour.h"
#include "ColourPalette.h"
#include "model/types.h"

#include <map>

namespace GPlatesGui
{
	/**
	 * DefaultPlateIdColourPalette maps plate IDs to colours using a scheme that
	 * aims to make adjacent plates stand out from each other.
	 */
	class DefaultPlateIdColourPalette :
		public ColourPalette<GPlatesModel::integer_plate_id_type>
	{
	public:

		boost::optional<Colour>
		get_colour(
				GPlatesModel::integer_plate_id_type plate_id) const;

	private:

		//! The colours to rotate through
		static
		Colour
		DEFAULT_COLOUR_ARRAY[];

		//! The size of DEFAULT_COLOUR_ARRAY
		static
		size_t
		DEFAULT_COLOUR_ARRAY_SIZE;
	};

	/**
	 * RegionalPlateIdColourPalette maps plate IDs to colours using a scheme that
	 * colours plates belonging to the same region with similar colours.
	 */
	class RegionalPlateIdColourPalette :
		public ColourPalette<GPlatesModel::integer_plate_id_type>
	{
	public:

		boost::optional<Colour>
		get_colour(
				GPlatesModel::integer_plate_id_type plate_id) const;

	private:

		//! Base colours for the regions
		static
		Colour
		REGIONAL_COLOUR_ARRAY[];

		//! The size of REGIONAL_COLOUR_ARRAY
		static
		size_t
		REGIONAL_COLOUR_ARRAY_SIZE;
	};
}

#endif  /* GPLATES_GUI_PLATEIDCOLOURPALETTES_H */
