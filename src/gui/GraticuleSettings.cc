/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2018 The University of Sydney, Australia
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

#include "GraticuleSettings.h"

#include "maths/MathsUtils.h"

#include "scribe/Scribe.h"


namespace
{
	GPlatesGui::Colour
	get_default_graticules_colour()
	{
		GPlatesGui::Colour result = GPlatesGui::Colour::get_silver();
		result.alpha() = 0.5f;
		return result;
	}
}


const double GPlatesGui::GraticuleSettings::DEFAULT_GRATICULE_DELTA_LAT = GPlatesMaths::PI / 6.0; // 30 degrees
const double GPlatesGui::GraticuleSettings::DEFAULT_GRATICULE_DELTA_LON = GPlatesMaths::PI / 6.0; // 30 degrees
const GPlatesGui::Colour GPlatesGui::GraticuleSettings::DEFAULT_GRATICULE_COLOUR = get_default_graticules_colour();


GPlatesScribe::TranscribeResult
GPlatesGui::GraticuleSettings::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Provide default values for failed parameters instead of returning failure.
	// This way a future version of GPlates can add or remove parameters and still be backward/forward compatible.
	static const GraticuleSettings DEFAULT;

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_delta_lat, "delta_lat"))
	{
		d_delta_lat = DEFAULT.d_delta_lat;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_delta_lon, "delta_lon"))
	{
		d_delta_lon = DEFAULT.d_delta_lon;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_colour, "colour"))
	{
		d_colour = DEFAULT.d_colour;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_line_width_hint, "line_width_hint"))
	{
		d_line_width_hint = DEFAULT.d_line_width_hint;
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
