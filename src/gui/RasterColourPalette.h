/* $Id$ */

/**
 * @file 
 * Contains colour palettes suitable for rasters.
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

#ifndef GPLATES_GUI_RASTERCOLOURPALETTE_H
#define GPLATES_GUI_RASTERCOLOURPALETTE_H

#include "ColourPalette.h"
#include "CptColourPalette.h"


namespace GPlatesGui
{
	/**
	 * The default colour palette used to colour non-RGBA rasters upon file loading.
	 * The colour palette covers a range of values up to two standard deviations
	 * away from the mean.
	 */
	class DefaultRasterColourPalette :
			public ColourPalette<double>
	{
	public:

		/**
		 * Constructs a DefaultRasterColourPalette, given the mean and the
		 * standard deviation of the values in the raster.
		 */
		static
		non_null_ptr_type
		create(
				double mean,
				double std_dev);

		virtual
		boost::optional<Colour>
		get_colour(
				double value) const;

	private:

		DefaultRasterColourPalette(
				double mean,
				double std_dev);

		RegularCptColourPalette::non_null_ptr_type d_inner_palette;

		static const int NUM_STD_DEV_AWAY_FROM_MEAN = 2;
	};
}

#endif  /* GPLATES_GUI_RASTERCOLOURPALETTE_H */
