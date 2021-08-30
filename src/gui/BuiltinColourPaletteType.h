/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_BUILTINCOLOURPALETTETYPE_H
#define GPLATES_GUI_BUILTINCOLOURPALETTETYPE_H

#include <QString>

#include "BuiltinColourPalettes.h"
#include "RasterColourPalette.h"

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"


namespace GPlatesGui
{
	/**
	 * Used to define the type of a built-in colour palette.
	 *
	 * This also avoids having a very large number of enumerations due to the various combinations
	 * of possible ColorBrewer palettes, for example.
	 */
	class BuiltinColourPaletteType
	{
	public:

		/**
		 * Some pre-defined internal palette types are provided for convenience.
		 */
		enum PaletteType
		{
			// This is a small group of age palettes...
			AGE_PALETTE,

			// This is a group of sequential ColorBrewer palettes...
			COLORBREWER_SEQUENTIAL_PALETTE,
			// This is a group of diverging ColorBrewer palettes...
			COLORBREWER_DIVERGING_PALETTE

			// NOTE: Any new values should also be added to @a transcribe.
		};


		/**
		 * Parameters that may be related to the palette type.
		 *
		 * Currently only ColorBrewer palette parameters are stored here.
		 */
		struct Parameters
		{
			Parameters(
					BuiltinColourPalettes::ColorBrewer::Sequential::Classes colorbrewer_sequential_classes_ =
							BuiltinColourPalettes::ColorBrewer::Sequential::Classes::Nine,
					BuiltinColourPalettes::ColorBrewer::Diverging::Classes colorbrewer_diverging_classes_ =
							BuiltinColourPalettes::ColorBrewer::Diverging::Classes::Eleven,
					bool colorbrewer_continuous_ = true,
					bool colorbrewer_inverted_ = false) :
				colorbrewer_sequential_classes(colorbrewer_sequential_classes_),
				colorbrewer_diverging_classes(colorbrewer_diverging_classes_),
				colorbrewer_continuous(colorbrewer_continuous_),
				colorbrewer_inverted(colorbrewer_inverted_)
			{  }

			// ColorBrewer parameters.
			BuiltinColourPalettes::ColorBrewer::Sequential::Classes colorbrewer_sequential_classes;
			BuiltinColourPalettes::ColorBrewer::Diverging::Classes colorbrewer_diverging_classes;
			bool colorbrewer_continuous;
			bool colorbrewer_inverted;

		private: // Transcribe for sessions/projects...

			friend class GPlatesScribe::Access;

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);
		};


		/**
		 * Construct an age type.
		 */
		explicit
		BuiltinColourPaletteType(
				BuiltinColourPalettes::Age::Type age_type);

		/**
		 * Construct a ColorBrewer sequential palette type.
		 */
		BuiltinColourPaletteType(
				BuiltinColourPalettes::ColorBrewer::Sequential::Type colorbrewer_sequential_type,
				const Parameters &parameters);

		/**
		 * Construct a ColorBrewer diverging palette type.
		 */
		BuiltinColourPaletteType(
				BuiltinColourPalettes::ColorBrewer::Diverging::Type colorbrewer_diverging_type,
				const Parameters &parameters);


		/**
		 * Creates a colour palette.
		 */
		RasterColourPalette::non_null_ptr_type
		create_palette() const;


		/**
		 * Returns the name of the colour palette.
		 *
		 * This is useful for displaying in the GUI.
		 */
		QString
		get_palette_name() const;


		/**
		 * Return the palette type.
		 */
		PaletteType
		get_palette_type() const
		{
			return d_palette_type;
		}


		/**
		 * Return the parameters.
		 */
		const Parameters &
		get_parameters() const
		{
			return d_parameters;
		}


		/**
		 * Return the age palette type (if @a get_palette_type returns @a AGE_PALETTE).
		 */
		BuiltinColourPalettes::Age::Type
		get_age_type() const
		{
			return d_age_type;
		}

		/**
		 * Return the ColorBrewer sequential palette type (if @a get_palette_type returns @a COLORBREWER_SEQUENTIAL_PALETTE).
		 */
		BuiltinColourPalettes::ColorBrewer::Sequential::Type
		get_colorbrewer_sequential_type() const
		{
			return d_colorbrewer_sequential_type;
		}

		/**
		 * Return the ColorBrewer diverging palette type (if @a get_palette_type returns @a COLORBREWER_DIVERGING_PALETTE).
		 */
		BuiltinColourPalettes::ColorBrewer::Diverging::Type
		get_colorbrewer_diverging_type() const
		{
			return d_colorbrewer_diverging_type;
		}

	private:
		PaletteType d_palette_type;
		Parameters d_parameters;

		// This is onlky used if @a d_palette_type is @a AGE_PALETTE.
		BuiltinColourPalettes::Age::Type d_age_type;
		// These are only used if @a d_palette_type is @a COLORBREWER_SEQUENTIAL_PALETTE or @a COLORBREWER_DIVERGING_PALETTE.
		BuiltinColourPalettes::ColorBrewer::Sequential::Type d_colorbrewer_sequential_type;
		BuiltinColourPalettes::ColorBrewer::Diverging::Type d_colorbrewer_diverging_type;

		
		static const PaletteType DEFAULT_PALETTE_TYPE;
		static const BuiltinColourPalettes::Age::Type DEFAULT_AGE_TYPE;
		static const BuiltinColourPalettes::ColorBrewer::Sequential::Type DEFAULT_COLORBREWER_SEQUENTIAL_TYPE;
		static const BuiltinColourPalettes::ColorBrewer::Diverging::Type DEFAULT_COLORBREWER_DIVERGING_TYPE;

	private: // Transcribe for sessions/projects...

		friend class GPlatesScribe::Access;

		// Default constructor makes transcribing easier.
		BuiltinColourPaletteType() :
				d_palette_type(DEFAULT_PALETTE_TYPE),
				d_age_type(DEFAULT_AGE_TYPE),
				d_colorbrewer_sequential_type(DEFAULT_COLORBREWER_SEQUENTIAL_TYPE),
				d_colorbrewer_diverging_type(DEFAULT_COLORBREWER_DIVERGING_TYPE)
		{  }

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data);
	};


	/**
	 * Transcribe for sessions/projects.
	 */
	GPlatesScribe::TranscribeResult
	transcribe(
			GPlatesScribe::Scribe &scribe,
			BuiltinColourPaletteType::PaletteType &palette_type,
			bool transcribed_construct_data);
}

#endif // GPLATES_GUI_BUILTINCOLOURPALETTETYPE_H
