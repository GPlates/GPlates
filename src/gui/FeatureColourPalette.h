/* $Id$ */

/**
 * @file 
 * Contains the definition of the FeatureColourPalette class.
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

#include "ColourPalette.h"
#include "model/FeatureType.h"

#include <map>

namespace GPlatesGui
{
	class Colour;
	
	/**
	 * FeatureColourPalette maps feature types to colours.
	 *
	 * NOTE: After implementing CPT-file support, we might as well make the
	 * default feature type colour palette a CPT-file and have no native feature
	 * type palettes.
	 */
	class FeatureColourPalette :
		public ColourPalette<GPlatesModel::FeatureType>
	{
	public:

		FeatureColourPalette();

		boost::optional<Colour>
		get_colour(
				const GPlatesModel::FeatureType &feature_type) const;
	
	private:

		//! A mapping of FeatureType to Colours
		std::map<GPlatesModel::FeatureType, Colour> d_colours;

	};
}

#endif  /* GPLATES_GUI_FEATURECOLOURPALETTE_H */
