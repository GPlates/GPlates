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

#include "TopologyNetworkParams.h"

#include "scribe/Scribe.h"
#include "scribe/TranscribeEnumProtocol.h"


GPlatesAppLogic::TopologyNetworkParams::TopologyNetworkParams() :
	d_strain_rate_smoothing(NATURAL_NEIGHBOUR_SMOOTHING)
{
}


bool
GPlatesAppLogic::TopologyNetworkParams::operator==(
		const TopologyNetworkParams &rhs) const
{
	return
		d_strain_rate_smoothing == rhs.d_strain_rate_smoothing;
}


bool
GPlatesAppLogic::TopologyNetworkParams::operator<(
		const TopologyNetworkParams &rhs) const
{
	if (d_strain_rate_smoothing < rhs.d_strain_rate_smoothing)
	{
		return true;
	}
	if (d_strain_rate_smoothing > rhs.d_strain_rate_smoothing)
	{
		return false;
	}

	return false;
}


GPlatesScribe::TranscribeResult
GPlatesAppLogic::TopologyNetworkParams::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Provide default values for failed parameters instead of returning failure.
	// This way a future version of GPlates can add or remove parameters and still be backward/forward compatible.
	static const TopologyNetworkParams DEFAULT_PARAMS;

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_strain_rate_smoothing, "strain_rate_smoothing"))
	{
		d_strain_rate_smoothing = DEFAULT_PARAMS.d_strain_rate_smoothing;
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesAppLogic::transcribe(
		GPlatesScribe::Scribe &scribe,
		TopologyNetworkParams::StrainRateSmoothing &strain_rate_smoothing,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("NO_SMOOTHING", TopologyNetworkParams::NO_SMOOTHING),
		GPlatesScribe::EnumValue("BARYCENTRIC_SMOOTHING", TopologyNetworkParams::BARYCENTRIC_SMOOTHING),
		GPlatesScribe::EnumValue("NATURAL_NEIGHBOUR_SMOOTHING", TopologyNetworkParams::NATURAL_NEIGHBOUR_SMOOTHING)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			strain_rate_smoothing,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}
