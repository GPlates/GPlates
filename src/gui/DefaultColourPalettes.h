/* $Id$ */

/**
 * @file 
 * Contains colour palettes suitable for rasters.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_DEFAULTCOLOURPALETTES_H
#define GPLATES_GUI_DEFAULTCOLOURPALETTES_H

#include "Colour.h"
#include "ColourPalette.h"


namespace GPlatesGui
{
	namespace DefaultColourPalettes
	{
		/**
		 * The default colour palette used when colouring by *scalar* value.
		 *
		 * The colour palette covers the range of values [0, 1].
		 * This palette is useful when the mapping to a specific scalar range is done elsewhere
		 * (such as via the GPU hardware) - then the range of scalar values (such as
		 * mean +/- std_deviation) that map to [0,1] can be handled by the GPU hardware
		 * (requires more advanced hardware though - but 3D scalar fields rely on that anyway).
		 *
		 * Subsequently visiting the returned colour palette will visit a @a RegularCptColourPalette
		 * since the returned palette (which is actually a @a ColourPaletteAdapter) adapts one.
		 */
		ColourPalette<double>::non_null_ptr_type
		create_scalar_colour_palette();


		/**
		 * The default colour palette used when colouring by *gradient* magnitude.
		 *
		 * The colour palette covers the range of values [-1, 1].
		 * When the back side of an isosurface (towards the half-space with lower scalar values)
		 * is visible then the gradient magnitude is mapped to the range [0,1] and the front side
		 * is mapped to the range [-1,0].
		 *
		 * Like @a create_scalar_colour_palette this palette is useful for more advanced GPU
		 * hardware that can explicitly handle the re-mapping of gradient magnitude ranges to [-1,1].
		 *
		 * Subsequently visiting the returned colour palette will visit a @a RegularCptColourPalette
		 * since the returned palette (which is actually a @a ColourPaletteAdapter) adapts one.
		 */
		ColourPalette<double>::non_null_ptr_type
		create_gradient_colour_palette();


		/**
		 * A colour palette used to colour strains in deformation networks.
		 *
		 * Subsequently visiting the returned colour palette will visit a @a RegularCptColourPalette
		 * since the returned palette (which is actually a @a ColourPaletteAdapter) adapts one.
		 */
		ColourPalette<double>::non_null_ptr_type
		create_deformation_strain_colour_palette(
				const double &range1_max = 1.0,
				const double &range1_min = 0.0,
				const double &range2_max = 0.0,
				const double &range2_min = -1.0,
				const Colour &fg_colour = Colour(1, 1, 1),  /* white - fg */
				const Colour &max_colour = Colour(1, 0, 0), /* red - high */
				const Colour &mid_colour = Colour(1, 1, 1), /* white - middle */
				const Colour &min_colour = Colour(0, 0, 1), /* blue - low */
				const Colour &bg_colour = Colour(1, 1, 1)); /* white - bg */
	}
}


#endif  /* GPLATES_GUI_DEFAULTCOLOURPALETTES_H */
