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

		static
		non_null_ptr_type
		create();

		virtual
		boost::optional<Colour>
		get_colour(
				value_type plate_id) const;

		virtual
		void
		accept_visitor(
				ConstColourPaletteVisitor &visitor) const
		{
			visitor.visit_default_plate_id_colour_palette(*this);
		}

		virtual
		void
		accept_visitor(
				ColourPaletteVisitor &visitor)
		{
			visitor.visit_default_plate_id_colour_palette(*this);
		}

	private:

		DefaultPlateIdColourPalette()
		{
		}
	};

	/**
	 * RegionalPlateIdColourPalette maps plate IDs to colours using a scheme that
	 * colours plates belonging to the same region with similar colours.
	 */
	class RegionalPlateIdColourPalette :
			public ColourPalette<GPlatesModel::integer_plate_id_type>
	{
	public:

		static
		non_null_ptr_type
		create();

		virtual
		boost::optional<Colour>
		get_colour(
				value_type plate_id) const;

		virtual
		void
		accept_visitor(
				ConstColourPaletteVisitor &visitor) const
		{
			visitor.visit_regional_plate_id_colour_palette(*this);
		}

		virtual
		void
		accept_visitor(
				ColourPaletteVisitor &visitor)
		{
			visitor.visit_regional_plate_id_colour_palette(*this);
		}

	private:

		RegionalPlateIdColourPalette()
		{
		}
	};
}

#endif  /* GPLATES_GUI_PLATEIDCOLOURPALETTES_H */
