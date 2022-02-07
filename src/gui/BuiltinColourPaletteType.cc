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

#include "BuiltinColourPaletteType.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "scribe/Scribe.h"
#include "scribe/TranscribeEnumProtocol.h"


const GPlatesGui::BuiltinColourPaletteType::PaletteType
GPlatesGui::BuiltinColourPaletteType::DEFAULT_PALETTE_TYPE =
		GPlatesGui::BuiltinColourPaletteType::AGE_PALETTE/*arbitrary*/;

const GPlatesGui::BuiltinColourPalettes::ColorBrewerSequentialType
GPlatesGui::BuiltinColourPaletteType::DEFAULT_COLORBREWER_SEQUENTIAL_TYPE =
		GPlatesGui::BuiltinColourPalettes::OrRd/*arbitrary*/;

const GPlatesGui::BuiltinColourPalettes::ColorBrewerDivergingType
GPlatesGui::BuiltinColourPaletteType::DEFAULT_COLORBREWER_DIVERGING_TYPE =
		GPlatesGui::BuiltinColourPalettes::Spectral/*arbitrary*/;


GPlatesGui::BuiltinColourPaletteType::BuiltinColourPaletteType(
		PaletteType palette_type) :
	d_palette_type(palette_type),
	d_colorbrewer_sequential_type(DEFAULT_COLORBREWER_SEQUENTIAL_TYPE),
	d_colorbrewer_diverging_type(DEFAULT_COLORBREWER_DIVERGING_TYPE)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_palette_type != COLORBREWER_SEQUENTIAL_PALETTE &&
				d_palette_type != COLORBREWER_DIVERGING_PALETTE,
			GPLATES_ASSERTION_SOURCE);
}


GPlatesGui::BuiltinColourPaletteType::BuiltinColourPaletteType(
		BuiltinColourPalettes::ColorBrewerSequentialType colorbrewer_sequential_type,
		const Parameters &parameters) :
	d_palette_type(COLORBREWER_SEQUENTIAL_PALETTE),
	d_parameters(parameters),
	d_colorbrewer_sequential_type(colorbrewer_sequential_type),
	d_colorbrewer_diverging_type(DEFAULT_COLORBREWER_DIVERGING_TYPE)
{
}


GPlatesGui::BuiltinColourPaletteType::BuiltinColourPaletteType(
		BuiltinColourPalettes::ColorBrewerDivergingType colorbrewer_diverging_type,
		const Parameters &parameters) :
	d_palette_type(COLORBREWER_DIVERGING_PALETTE),
	d_parameters(parameters),
	d_colorbrewer_sequential_type(DEFAULT_COLORBREWER_SEQUENTIAL_TYPE),
	d_colorbrewer_diverging_type(colorbrewer_diverging_type)
{
}


GPlatesGui::RasterColourPalette::non_null_ptr_type
GPlatesGui::BuiltinColourPaletteType::create_palette() const
{
	switch (d_palette_type)
	{
	case AGE_PALETTE:
		return GPlatesGui::RasterColourPalette::create<double>(
				BuiltinColourPalettes::create_age_palette());

	case COLORBREWER_SEQUENTIAL_PALETTE:
		return GPlatesGui::RasterColourPalette::create<double>(
				BuiltinColourPalettes::create_colorbrewer_sequential_palette(
						d_colorbrewer_sequential_type,
						d_parameters.colorbrewer_sequential_classes,
						d_parameters.colorbrewer_continuous,
						d_parameters.colorbrewer_inverted));

	case COLORBREWER_DIVERGING_PALETTE:
		return GPlatesGui::RasterColourPalette::create<double>(
				BuiltinColourPalettes::create_colorbrewer_diverging_palette(
						d_colorbrewer_diverging_type,
						d_parameters.colorbrewer_diverging_classes,
						d_parameters.colorbrewer_continuous,
						d_parameters.colorbrewer_inverted));

	default:
		break;
	}

	// Shouldn't be able to get here.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
	return RasterColourPalette::create();
}


QString
GPlatesGui::BuiltinColourPaletteType::get_palette_name() const
{
	switch (d_palette_type)
	{
	case AGE_PALETTE:
		return QString("Age");

	case COLORBREWER_SEQUENTIAL_PALETTE:
		return BuiltinColourPalettes::get_colorbrewer_sequential_palette_name(d_colorbrewer_sequential_type);

	case COLORBREWER_DIVERGING_PALETTE:
		return BuiltinColourPalettes::get_colorbrewer_diverging_palette_name(d_colorbrewer_diverging_type);

	default:
		break;
	}

	// Shouldn't be able to get here.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
	return QString();
}


GPlatesScribe::TranscribeResult
GPlatesGui::BuiltinColourPaletteType::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Provide default values for failed parameters instead of returning failure.
	// This way a future version of GPlates can add or remove parameters and still be backward/forward compatible.

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_palette_type, "palette_type"))
	{
		d_palette_type = DEFAULT_PALETTE_TYPE;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_colorbrewer_sequential_type, "colorbrewer_sequential_type"))
	{
		d_colorbrewer_sequential_type = DEFAULT_COLORBREWER_SEQUENTIAL_TYPE;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_colorbrewer_diverging_type, "colorbrewer_diverging_type"))
	{
		d_colorbrewer_diverging_type = DEFAULT_COLORBREWER_DIVERGING_TYPE;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_parameters, "parameters"))
	{
		d_parameters = Parameters();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesGui::BuiltinColourPaletteType::Parameters::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Provide default values for failed parameters instead of returning failure.
	// This way a future version of GPlates can add or remove parameters and still be backward/forward compatible.
	static const Parameters DEFAULT_PARAMS;

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, colorbrewer_sequential_classes, "colorbrewer_sequential_classes"))
	{
		colorbrewer_sequential_classes = DEFAULT_PARAMS.colorbrewer_sequential_classes;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, colorbrewer_diverging_classes, "colorbrewer_diverging_classes"))
	{
		colorbrewer_diverging_classes = DEFAULT_PARAMS.colorbrewer_diverging_classes;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, colorbrewer_continuous, "colorbrewer_continuous"))
	{
		colorbrewer_continuous = DEFAULT_PARAMS.colorbrewer_continuous;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, colorbrewer_inverted, "colorbrewer_inverted"))
	{
		colorbrewer_inverted = DEFAULT_PARAMS.colorbrewer_inverted;
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesGui::transcribe(
		GPlatesScribe::Scribe &scribe,
		BuiltinColourPaletteType::PaletteType &palette_type,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("AGE_PALETTE", BuiltinColourPaletteType::AGE_PALETTE),
		GPlatesScribe::EnumValue("COLORBREWER_SEQUENTIAL_PALETTE", BuiltinColourPaletteType::COLORBREWER_SEQUENTIAL_PALETTE),
		GPlatesScribe::EnumValue("COLORBREWER_DIVERGING_PALETTE", BuiltinColourPaletteType::COLORBREWER_DIVERGING_PALETTE)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			palette_type,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}
