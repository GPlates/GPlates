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

#include <boost/intrusive_ptr.hpp>
#include <boost/optional.hpp>

#include "Colour.h"

#include "model/types.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"
#include "utils/Select.h"
#include "utils/TypeTraits.h"


namespace GPlatesGui 
{
	// Forward declarations.
	template <bool> class ColourPaletteVisitorBase;
	typedef ColourPaletteVisitorBase<true> ConstColourPaletteVisitor;
	typedef ColourPaletteVisitorBase<false> ColourPaletteVisitor;

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
	template<typename KeyType>
	class ColourPalette :
			public GPlatesUtils::ReferenceCount<ColourPalette<KeyType> >
	{
	public:

		typedef KeyType key_type;
		typedef typename GPlatesUtils::TypeTraits<key_type>::argument_type value_type;

		typedef ColourPalette<KeyType> this_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<this_type> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const this_type> non_null_ptr_to_const_type;
		typedef boost::intrusive_ptr<this_type> maybe_null_ptr_type;
		typedef boost::intrusive_ptr<const this_type> maybe_null_ptr_to_const_type;

		virtual
		~ColourPalette()
		{  }

		/**
		 * Retrieves the Colour associated with the @a value provided.
		 * @returns boost::none if no Colour is assocated with the value.
		 *
		 * Note that the parameter @a value is of type KeyType if KeyType is a
		 * built-in type, and of type const KeyType & otherwise.
		 */
		virtual
		boost::optional<Colour>
		get_colour(
				value_type value) const = 0;

		virtual
		void
		accept_visitor(
				ConstColourPaletteVisitor &) const
		{  }

		virtual
		void
		accept_visitor(
				ColourPaletteVisitor &)
		{  }
	};
}

#endif  /* GPLATES_GUI_COLOURPALETTE_H */
