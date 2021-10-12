/* $Id$ */

/**
 * @file 
 * Contains the definition of the ColourPalette class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_COLOURPALETTE_H
#define GPLATES_GUI_COLOURPALETTE_H

#include "Colour.h"
#include "model/types.h"
#include <boost/optional.hpp>

namespace GPlatesGui 
{
	/**
	 * ColourPalette maps KeyType values to Colours, the mapping being either
	 * continuous or discrete.
	 *
	 * ColourPalette vs ColourScheme: ColourSchemes assign colours to
	 * reconstruction geometries. Some ColourSchemes may extract properties out of
	 * the reconstruction geometries (e.g. plate id) and then delegate the task of
	 * assigning a colour to the property (e.g. plate id) to a ColourPalette. This
	 * avoids duplication of code, e.g. without this use of the strategy pattern,
	 * two different ColourSchemes that colour by plate id would both need to
	 * implement code to extract the plate id.
	 *
	 * If your colour palette can deal with multiple KeyTypes, multiply inherit
	 * from ColourPalette<KeyType-1>, ColourPalette<KeyType-2> ...
	 */
	template <class KeyType>
	class ColourPalette
	{
	public:

		/**
		 * Retrieves the Colour associated with the @a value provided.
		 * @returns boost::none if no Colour is assocated with the value.
		 */
		virtual
		boost::optional<Colour>
		get_colour(
				const KeyType &value) const = 0;

		//! Destructor
		virtual
		~ColourPalette()
		{
		}

	};

	/**
	 * ColourPalette template specialisation for int
	 */
	template <>
	class ColourPalette<int>
	{
	public:

		virtual
		boost::optional<Colour>
		get_colour(
				int value) const = 0;

		virtual
		~ColourPalette()
		{
		}

	};

	/**
	 * ColourPalette template specialisation for double
	 */
	template <>
	class ColourPalette<double>
	{
	public:

		virtual
		boost::optional<Colour>
		get_colour(
				double value) const = 0;

		virtual
		~ColourPalette()
		{
		}

	};

	/**
	 * ColourPalette template specialisation for GPlatesModel::integer_plate_id_type
	 */
	template <>
	class ColourPalette<GPlatesModel::integer_plate_id_type>
	{
	public:

		virtual
		boost::optional<Colour>
		get_colour(
				GPlatesModel::integer_plate_id_type value) const = 0;

		virtual
		~ColourPalette()
		{
		}

	};
}

#endif  /* GPLATES_GUI_COLOURPALETTE_H */
