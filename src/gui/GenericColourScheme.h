/* $Id$ */

/**
 * @file 
 * Contains the definition of the GenericColourScheme class.
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

#ifndef GPLATES_GUI_GENERICCOLOURSCHEME_H
#define GPLATES_GUI_GENERICCOLOURSCHEME_H

#include "Colour.h"
#include "ColourScheme.h"
#include "ColourPalette.h"

#include <boost/optional.hpp>

namespace GPlatesGui
{
	/**
	 * GenericColourScheme takes a reconstruction geometry, extracts a property
	 * and maps that property to a colour using a colour palette.
	 *
	 * PropertyExtractorType needs to have the function operator implemented. The
	 * function operator should return a boost::optional of the property's type;
	 * boost::none is returned if the value does not exist.
	 *
	 * PropertyExtractorType also needs to publicly typedef the property's type as
	 * 'return_type'.
	 */
	template<class PropertyExtractorType>
	class GenericColourScheme :
			public ColourScheme
	{
	private:

		typedef ColourPalette<typename PropertyExtractorType::return_type> ColourPaletteType;

	public:

		/**
		 * GenericColourScheme constructor
		 * @param colour_palette_ptr A pointer to a colour palette. Ownership of the
		 * pointer passes to this instance of GenericColourScheme; the memory pointed
		 * at by the pointer is deallocated when this instance is destructed.
		 */
		GenericColourScheme(
				typename ColourPaletteType::non_null_ptr_type colour_palette_ptr,
				const PropertyExtractorType &property_extractor) :
			d_colour_palette_ptr(colour_palette_ptr),
			d_property_extractor(property_extractor)
		{
		}

		//! Destructor
		~GenericColourScheme()
		{
		}

		/**
		 * Returns a colour for a particular @a reconstruction_geometry, or
		 * boost::none if it does not have the necessary parameters or if the
		 * reconstruction geometry should not be drawn for some other reason
		 */

		boost::optional<Colour>
		get_colour(
				const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry) const
		{
			boost::optional<typename PropertyExtractorType::return_type> value =
				d_property_extractor(reconstruction_geometry);
			if (value)
			{
				return d_colour_palette_ptr->get_colour(*value);
			}
			else
			{
				return PROPERTY_NOT_FOUND_COLOUR;
			}
		}
	
	private:

		typename ColourPaletteType::non_null_ptr_type d_colour_palette_ptr;
		PropertyExtractorType d_property_extractor;

		static const Colour PROPERTY_NOT_FOUND_COLOUR;
	};


	template<class PropertyExtractorType>
	const Colour
	GenericColourScheme<PropertyExtractorType>::PROPERTY_NOT_FOUND_COLOUR = Colour::get_grey();


	template<typename ColourPalettePointerType, class PropertyExtractorType>
	inline
	ColourScheme::non_null_ptr_type
	make_colour_scheme(
			ColourPalettePointerType colour_palette_ptr,
			const PropertyExtractorType &property_extractor)
	{
		return new GenericColourScheme<PropertyExtractorType>(
				colour_palette_ptr,
				property_extractor);
	}
}

#endif  /* GPLATES_GUI_GENERICCOLOURSCHEME_H */
