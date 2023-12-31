/* $Id$ */

/**
 * @file 
 * Contains the definition of the FeatureTypeColourPalette class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_FEATURECOLOURPALETTE_H
#define GPLATES_GUI_FEATURECOLOURPALETTE_H

#include <map>
#include <boost/assign.hpp>
#include "Colour.h"
#include "ColourPalette.h"
#include "ColourPaletteVisitor.h"

#include "model/FeatureType.h"
#include "HTMLColourNames.h"


namespace GPlatesGui
{
	class Colour;
	/**
	 * FeatureTypeColourPalette maps feature types to colours.
	 */
	class FeatureTypeColourPalette :
			public ColourPalette<GPlatesModel::FeatureType>
	{
	public:

		/**
		 * The GPGIM is used to query all feature types available.
		 */
		static
		non_null_ptr_type
		create();

		virtual
		boost::optional<Colour>
		get_colour(
				const GPlatesModel::FeatureType &feature_type) const;

		virtual
		void
		accept_visitor(
				ConstColourPaletteVisitor &visitor) const
		{
			visitor.visit_feature_type_colour_palette(*this);
		}

		virtual
		void
		accept_visitor(
				ColourPaletteVisitor &visitor)
		{
			visitor.visit_feature_type_colour_palette(*this);
		}

	private:

		FeatureTypeColourPalette();

		/**
		 * A mapping of FeatureType to Colours.
		 *
		 * It's mutable because we cache the colour generated for a hitherto-unseen
		 * feature type in this map.
		 */
		mutable std::map<GPlatesModel::FeatureType, Colour> d_colours;
	};
}

#endif  /* GPLATES_GUI_FEATURECOLOURPALETTE_H */
