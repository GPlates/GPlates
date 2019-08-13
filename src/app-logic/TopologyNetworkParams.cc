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

#include "maths/types.h"

#include "scribe/Scribe.h"
#include "scribe/TranscribeEnumProtocol.h"


 // Strain rates get to around 1e-17 so we should scale that to 1.0 before doing epsilon comparisons.
const double GPlatesAppLogic::TopologyNetworkParams::COMPARE_STRAIN_RATE_SCALE = 1e+17;


GPlatesAppLogic::TopologyNetworkParams::TopologyNetworkParams() :
	d_strain_rate_smoothing(NATURAL_NEIGHBOUR_SMOOTHING)
{
}


bool
GPlatesAppLogic::TopologyNetworkParams::operator==(
		const TopologyNetworkParams &rhs) const
{
	return
		d_strain_rate_smoothing == rhs.d_strain_rate_smoothing &&
		d_strain_rate_clamping == rhs.d_strain_rate_clamping &&
		d_rift_params == rhs.d_rift_params;
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

	if (d_strain_rate_clamping < rhs.d_strain_rate_clamping)
	{
		return true;
	}
	if (d_strain_rate_clamping > rhs.d_strain_rate_clamping)
	{
		return false;
	}

	if (d_rift_params < rhs.d_rift_params)
	{
		return true;
	}
	if (d_rift_params > rhs.d_rift_params)
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

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_strain_rate_clamping, "strain_rate_clamping"))
	{
		d_strain_rate_clamping = DEFAULT_PARAMS.d_strain_rate_clamping;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_rift_params, "rift_params"))
	{
		d_rift_params = DEFAULT_PARAMS.d_rift_params;
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


GPlatesAppLogic::TopologyNetworkParams::StrainRateClamping::StrainRateClamping() :
	enable_clamping(false),
	max_total_strain_rate(5e-15)
{
}


bool
GPlatesAppLogic::TopologyNetworkParams::StrainRateClamping::operator==(
		const TopologyNetworkParams::StrainRateClamping &rhs) const
{
	return
		enable_clamping == rhs.enable_clamping &&
		GPlatesMaths::real_t(COMPARE_STRAIN_RATE_SCALE * max_total_strain_rate) ==
				GPlatesMaths::real_t(COMPARE_STRAIN_RATE_SCALE * rhs.max_total_strain_rate);
}


bool
GPlatesAppLogic::TopologyNetworkParams::StrainRateClamping::operator<(
		const TopologyNetworkParams::StrainRateClamping &rhs) const
{
	if (enable_clamping < rhs.enable_clamping)
	{
		return true;
	}
	if (enable_clamping > rhs.enable_clamping)
	{
		return false;
	}

	if (GPlatesMaths::real_t(COMPARE_STRAIN_RATE_SCALE * max_total_strain_rate) <
		GPlatesMaths::real_t(COMPARE_STRAIN_RATE_SCALE * rhs.max_total_strain_rate))
	{
		return true;
	}
	if (GPlatesMaths::real_t(COMPARE_STRAIN_RATE_SCALE * max_total_strain_rate) >
		GPlatesMaths::real_t(COMPARE_STRAIN_RATE_SCALE * rhs.max_total_strain_rate))
	{
		return false;
	}

	return false;
}


GPlatesScribe::TranscribeResult
GPlatesAppLogic::TopologyNetworkParams::StrainRateClamping::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Provide default values for failed parameters instead of returning failure.
	// This way a future version of GPlates can add or remove parameters and still be backward/forward compatible.
	static const StrainRateClamping DEFAULT_PARAMS;

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, enable_clamping, "enable_clamping"))
	{
		enable_clamping = DEFAULT_PARAMS.enable_clamping;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, max_total_strain_rate, "max_total_strain_rate"))
	{
		max_total_strain_rate = DEFAULT_PARAMS.max_total_strain_rate;
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesAppLogic::TopologyNetworkParams::RiftParams::RiftParams() :
	exponential_stretching_constant(1.0),
	strain_rate_resolution(5e-17),
	edge_length_threshold_degrees(0.1)
{
}


bool
GPlatesAppLogic::TopologyNetworkParams::RiftParams::operator==(
		const TopologyNetworkParams::RiftParams &rhs) const
{
	return
		GPlatesMaths::real_t(exponential_stretching_constant) == GPlatesMaths::real_t(rhs.exponential_stretching_constant) &&
		GPlatesMaths::real_t(COMPARE_STRAIN_RATE_SCALE * strain_rate_resolution) ==
				GPlatesMaths::real_t(COMPARE_STRAIN_RATE_SCALE * rhs.strain_rate_resolution) &&
		GPlatesMaths::real_t(edge_length_threshold_degrees) == GPlatesMaths::real_t(rhs.edge_length_threshold_degrees);
}


bool
GPlatesAppLogic::TopologyNetworkParams::RiftParams::operator<(
		const TopologyNetworkParams::RiftParams &rhs) const
{
	if (GPlatesMaths::real_t(exponential_stretching_constant) < GPlatesMaths::real_t(rhs.exponential_stretching_constant))
	{
		return true;
	}
	if (GPlatesMaths::real_t(exponential_stretching_constant) > GPlatesMaths::real_t(rhs.exponential_stretching_constant))
	{
		return false;
	}

	if (GPlatesMaths::real_t(COMPARE_STRAIN_RATE_SCALE * strain_rate_resolution) <
		GPlatesMaths::real_t(COMPARE_STRAIN_RATE_SCALE * rhs.strain_rate_resolution))
	{
		return true;
	}
	if (GPlatesMaths::real_t(COMPARE_STRAIN_RATE_SCALE * strain_rate_resolution) >
		GPlatesMaths::real_t(COMPARE_STRAIN_RATE_SCALE * rhs.strain_rate_resolution))
	{
		return false;
	}

	if (GPlatesMaths::real_t(edge_length_threshold_degrees) < GPlatesMaths::real_t(rhs.edge_length_threshold_degrees))
	{
		return true;
	}
	if (GPlatesMaths::real_t(edge_length_threshold_degrees) > GPlatesMaths::real_t(rhs.edge_length_threshold_degrees))
	{
		return false;
	}

	return false;
}


GPlatesScribe::TranscribeResult
GPlatesAppLogic::TopologyNetworkParams::RiftParams::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Provide default values for failed parameters instead of returning failure.
	// This way a future version of GPlates can add or remove parameters and still be backward/forward compatible.
	static const RiftParams DEFAULT_PARAMS;

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, exponential_stretching_constant, "exponential_stretching_constant"))
	{
		exponential_stretching_constant = DEFAULT_PARAMS.exponential_stretching_constant;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, strain_rate_resolution, "strain_rate_resolution"))
	{
		strain_rate_resolution = DEFAULT_PARAMS.strain_rate_resolution;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, edge_length_threshold_degrees, "edge_length_threshold_degrees"))
	{
		edge_length_threshold_degrees = DEFAULT_PARAMS.edge_length_threshold_degrees;
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
