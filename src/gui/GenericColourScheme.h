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
#include "Palette.h"
#include "PlateIdColourPalettes.h"

#include <boost/optional.hpp>
#include "app-logic/ReconstructionGeometryUtils.h"
#include "presentation/Application.h"
#include "utils/FeatureUtils.h"

namespace GPlatesGui
{
	class PlateIdScheme : public ColourScheme
	{
	public:
		PlateIdScheme(const Palette* p) : d_palette(p)
		{ }

		boost::optional<Colour>
		get_colour(
				const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry) const
		{
			return get_colour(GPlatesAppLogic::ReconstructionGeometryUtils::get_plate_id(&reconstruction_geometry));
		}

		boost::optional<Colour>
		get_colour(
				const GPlatesModel::FeatureHandle& feature) const
		{
			return get_colour(GPlatesUtils::get_int_plate_id(&feature));
		}
	protected:
		boost::optional<Colour>
		get_colour(boost::optional<GPlatesModel::integer_plate_id_type> id) const
		{
			if(id)
			{
				return d_palette->get_colour(Palette::Key(static_cast<long>(*id)));
			}
			return boost::none;
		}
		
		const Palette*  d_palette;
	};


	class FeatureAgeScheme : public ColourScheme
	{
		double d_upper,d_lower;
	public:
		FeatureAgeScheme(
				const Palette* (*fun) (const double, const double),
				const double upper = 450.0,
				const double lower = 0.0) : 
			d_upper(upper),
			d_lower(lower),
			d_palette(fun(upper,lower))
		{ }

		boost::optional<Colour>
		get_colour(
				const GPlatesAppLogic::ReconstructionGeometry &r) const
		{
			boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature = 
				GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(&r);
			if(feature)
			{
				return get_colour(GPlatesUtils::get_age(
						(*feature).handle_ptr(), 
						GPlatesPresentation::current_time()));
			}
			return boost::none;
		}

		boost::optional<Colour>
		get_colour(
				const GPlatesModel::FeatureHandle& feature) const
		{
			return get_colour(GPlatesUtils::get_age(&feature, GPlatesPresentation::current_time()));
		}
	protected:
		boost::optional<Colour>
		get_colour(boost::optional<GPlatesMaths::Real> age) const
		{
			if(age)
			{
				Colour c;
				if((*age).dval() > d_upper)
				{
					c =  boost::get<0>(d_palette->get_BFN_colour());
					return c;
				}
				if((*age).dval() < d_lower)
					return boost::get<1>(d_palette->get_BFN_colour());
				return d_palette->get_colour(Palette::Key((*age).dval()));
			}
			return boost::none;
		}
		
		const Palette*  d_palette;
	};

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

		template<typename ArguType>
		boost::optional<Colour>
		get_colour_t(const ArguType& argu) const
		{
			boost::optional<typename PropertyExtractorType::return_type> value =
				d_property_extractor(argu);
			if (value)
			{
				return d_colour_palette_ptr->get_colour(*value);
			}
			else
			{
				return PROPERTY_NOT_FOUND_COLOUR;
			}
		}

		boost::optional<Colour>
		get_colour(
				const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry) const
		{
			return get_colour_t(reconstruction_geometry);
		}

		boost::optional<Colour>
		get_colour(
				const GPlatesModel::FeatureHandle& feature) const
		{
			return get_colour_t(feature);
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
