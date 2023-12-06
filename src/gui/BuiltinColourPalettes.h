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

#ifndef GPLATES_GUI_BUILTINCOLOURPALETTES_H
#define GPLATES_GUI_BUILTINCOLOURPALETTES_H

#include <QString>

#include "Colour.h"
#include "ColourPalette.h"

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"


namespace GPlatesGui
{
	namespace BuiltinColourPalettes
	{
		/**
		 * The colour palette used when colouring by *scalar* value.
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
		 * The colour palette used when colouring by *gradient* magnitude.
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
		 * A multi-colour colour palette used to colour strain rate dilatation in deformation networks.
		 *
		 * The blending of colours is linear in strain rate log space with a sample spacing of
		 * no more than @a max_log_spacing.
		 *
		 * Subsequently visiting the returned colour palette will visit a @a RegularCptColourPalette
		 * since the returned palette (which is actually a @a ColourPaletteAdapter) adapts one.
		 */
		ColourPalette<double>::non_null_ptr_type
		create_strain_rate_dilatation_colour_palette(
				double min_abs_strain_rate,
				double max_abs_strain_rate,
				const double &max_log_spacing = 0.3);


		/**
		 * A multi-colour colour palette used to colour second invariant of strain rate in deformation networks.
		 *
		 * The blending of colours is linear in strain rate log space with a sample spacing of
		 * no more than @a max_log_spacing.
		 *
		 * Subsequently visiting the returned colour palette will visit a @a RegularCptColourPalette
		 * since the returned palette (which is actually a @a ColourPaletteAdapter) adapts one.
		 */
		ColourPalette<double>::non_null_ptr_type
		create_strain_rate_second_invariant_colour_palette(
				double min_abs_strain_rate,
				double max_abs_strain_rate,
				const double &max_log_spacing = 0.3);


		/**
		 * A multi-colour colour palette used to colour strain rate style in deformation networks.
		 *
		 * Subsequently visiting the returned colour palette will visit a @a RegularCptColourPalette
		 * since the returned palette (which is actually a @a ColourPaletteAdapter) adapts one.
		 */
		ColourPalette<double>::non_null_ptr_type
		create_strain_rate_strain_rate_style_colour_palette(
				double min_strain_rate_style,
				double max_strain_rate_style);


		/**
		 * Age grid palettes.
		 */
		namespace Age
		{
			enum Type
			{
				Legacy,       // Original palette used in GPlates 2.2
				Traditional,  // New default palette for GPlates 2.3
				Modern,       // Alternative palette for GPlates 2.3
				Batlow        // New alternative palette for GPlates 2.4
			};

			/**
			 * Returns a name for an age colour palette.
			 *
			 * This is useful for displaying in the GUI.
			 */
			QString
			get_palette_name(
					Type type);

			/**
			 * Age grid colour palette.
			 *
			 * Age colour palettes cover a range of age values starting at 0Ma.
			 *
			 * Subsequently visiting the returned colour palette will visit a @a RegularCptColourPalette
			 * since the returned palette (which is actually a @a ColourPaletteAdapter) adapts one.
			 */
			ColourPalette<double>::non_null_ptr_type
			create_palette(
					Type type);


			//
			// Transcribe for sessions/projects.
			//

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					Type &type,
					bool transcribed_construct_data);
		}


		/**
		 * Topography palettes.
		 */
		namespace Topography
		{
			enum Type
			{
				Etopo1,       // Default palette used in GPlates 2.4
				Oleron,       // Alternative palette for GPlates 2.4
				Bukavu        // Alternative palette for GPlates 2.4
			};

			/**
			 * Returns a name for a topography colour palette.
			 *
			 * This is useful for displaying in the GUI.
			 */
			QString
			get_palette_name(
					Type type);

			/**
			 * Topography colour palette.
			 *
			 * Subsequently visiting the returned colour palette will visit a @a RegularCptColourPalette
			 * since the returned palette (which is actually a @a ColourPaletteAdapter) adapts one.
			 */
			ColourPalette<double>::non_null_ptr_type
			create_palette(
					Type type);


			//
			// Transcribe for sessions/projects.
			//

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					Type &type,
					bool transcribed_construct_data);
		}


		/**
		 * Colors from www.ColorBrewer.org by Cynthia A. Brewer, Geography, Pennsylvania State University.
		 */
		namespace ColorBrewer
		{
			/**
			 * ColorBrewer sequential palettes.
			 */
			namespace Sequential
			{
				/**
				 * ColorBrewer sequential palette types.
				 */
				enum Type
				{
					OrRd,
					PuBu,
					BuPu,
					Oranges,
					BuGn,
					YlOrBr,
					YlGn,
					Reds,
					RdPu,
					Greens,
					YlGnBu,
					Purples,
					GnBu,
					Greys,
					YlOrRd,
					PuRd,
					Blues,
					PuBuGn
				};

				/**
				 * Returns a name for a sequential ColorBrewer colour palette.
				 *
				 * This is useful for displaying in the GUI.
				 */
				QString
				get_palette_name(
						Type type);

				/**
				 * There are between 3 and 9 classes available in ColorBrewer sequential palette types.
				 */
				enum Classes
				{
					Three = 3,
					Four,
					Five,
					Six,
					Seven,
					Eight,
					Nine
				};

				/**
				 * Create a sequential ColorBrewer colour palette over the range [0,1].
				 *
				 * If @a continuous is true then the colours are linearly blended across each colour slice,
				 * otherwise a constant colour is used in each colour slice.
				 *
				 * @a invert reverses the ordering of colours.
				 *
				 * Subsequently visiting the returned colour palette will visit a @a RegularCptColourPalette
				 * since the returned palette (which is actually a @a ColourPaletteAdapter) adapts one.
				 */
				ColourPalette<double>::non_null_ptr_type
				create_palette(
						Type type,
						Classes classes,
						bool continuous,
						bool invert,
						const boost::optional<Colour> &nan_colour = boost::none);


				//
				// Transcribe for sessions/projects.
				//

				GPlatesScribe::TranscribeResult
				transcribe(
						GPlatesScribe::Scribe &scribe,
						Type &type,
						bool transcribed_construct_data);

				GPlatesScribe::TranscribeResult
				transcribe(
						GPlatesScribe::Scribe &scribe,
						Classes &classes,
						bool transcribed_construct_data);
			}


			/**
			 * ColorBrewer diverging palettes.
			 */
			namespace Diverging
			{
				/**
				 * ColorBrewer diverging palette types.
				 */
				enum Type
				{
					Spectral,
					RdYlGn,
					RdBu,
					PiYG,
					PRGn,
					RdYlBu,
					BrBG,
					RdGy,
					PuOr
				};

				/**
				 * Returns a name for a diverging ColorBrewer colour palette.
				 *
				 * This is useful for displaying in the GUI.
				 */
				QString
				get_palette_name(
						Type type);

				/**
				 * There are between 3 and 11 classes available in ColorBrewer diverging palette types.
				 */
				enum Classes
				{
					Three = 3,
					Four,
					Five,
					Six,
					Seven,
					Eight,
					Nine,
					Ten,
					Eleven
				};

				/**
				 * Create a diverging ColorBrewer colour palette over the range [-1,1].
				 *
				 * If @a continuous is true then the colours are linearly blended across each colour slice,
				 * otherwise a constant colour is used in each colour slice.
				 *
				 * @a invert reverses the ordering of colours.
				 *
				 * Subsequently visiting the returned colour palette will visit a @a RegularCptColourPalette
				 * since the returned palette (which is actually a @a ColourPaletteAdapter) adapts one.
				 */
				ColourPalette<double>::non_null_ptr_type
				create_palette(
						Type type,
						Classes classes,
						bool continuous,
						bool invert,
						const boost::optional<Colour> &nan_colour = boost::none);


				//
				// Transcribe for sessions/projects.
				//

				GPlatesScribe::TranscribeResult
				transcribe(
						GPlatesScribe::Scribe &scribe,
						Type &type,
						bool transcribed_construct_data);

				GPlatesScribe::TranscribeResult
				transcribe(
						GPlatesScribe::Scribe &scribe,
						Classes &classes,
						bool transcribed_construct_data);
			}
		}
	}
}

#endif  /* GPLATES_GUI_BUILTINCOLOURPALETTES_H */
