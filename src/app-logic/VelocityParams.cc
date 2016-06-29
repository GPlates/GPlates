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

#include "VelocityParams.h"

#include "scribe/Scribe.h"
#include "scribe/TranscribeEnumProtocol.h"


GPlatesAppLogic::VelocityParams::VelocityParams() :
	// Default to using surfaces since that's how GPlates started out calculating velocities...
	d_solve_velocities_method(SOLVE_VELOCITIES_OF_SURFACES_AT_DOMAIN_POINTS),
	d_delta_time_type(VelocityDeltaTime::T_PLUS_DELTA_T_TO_T),
	d_delta_time(1.0),
	d_is_boundary_smoothing_enabled(false),
	d_boundary_smoothing_angular_half_extent_degrees(1.0),
	// Default to no smoothing inside deforming regions...
	d_exclude_deforming_regions_from_smoothing(true)
{
}


bool
GPlatesAppLogic::VelocityParams::operator==(
		const VelocityParams &rhs) const
{
	return d_solve_velocities_method == rhs.d_solve_velocities_method &&
			d_delta_time_type == rhs.d_delta_time_type &&
			d_delta_time == rhs.d_delta_time &&
			d_is_boundary_smoothing_enabled == rhs.d_is_boundary_smoothing_enabled &&
			d_boundary_smoothing_angular_half_extent_degrees == rhs.d_boundary_smoothing_angular_half_extent_degrees &&
			d_exclude_deforming_regions_from_smoothing == rhs.d_exclude_deforming_regions_from_smoothing;
}


bool
GPlatesAppLogic::VelocityParams::operator<(
		const VelocityParams &rhs) const
{
	if (d_solve_velocities_method < rhs.d_solve_velocities_method)
	{
		return true;
	}
	if (d_solve_velocities_method > rhs.d_solve_velocities_method)
	{
		return false;
	}

	if (d_delta_time_type < rhs.d_delta_time_type)
	{
		return true;
	}
	if (d_delta_time_type > rhs.d_delta_time_type)
	{
		return false;
	}

	if (d_delta_time < rhs.d_delta_time)
	{
		return true;
	}
	if (d_delta_time > rhs.d_delta_time)
	{
		return false;
	}

	if (d_is_boundary_smoothing_enabled < rhs.d_is_boundary_smoothing_enabled)
	{
		return true;
	}
	if (d_is_boundary_smoothing_enabled > rhs.d_is_boundary_smoothing_enabled)
	{
		return false;
	}

	if (d_boundary_smoothing_angular_half_extent_degrees < rhs.d_boundary_smoothing_angular_half_extent_degrees)
	{
		return true;
	}
	if (d_boundary_smoothing_angular_half_extent_degrees > rhs.d_boundary_smoothing_angular_half_extent_degrees)
	{
		return false;
	}

	if (d_exclude_deforming_regions_from_smoothing < rhs.d_exclude_deforming_regions_from_smoothing)
	{
		return true;
	}
	if (d_exclude_deforming_regions_from_smoothing > rhs.d_exclude_deforming_regions_from_smoothing)
	{
		return false;
	}

	return false;
}


GPlatesScribe::TranscribeResult
GPlatesAppLogic::VelocityParams::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Provide default values for failed parameters instead of returning failure.
	// This way a future version of GPlates can add or remove parameters and still be backward/forward compatible.
	static const VelocityParams DEFAULT_PARAMS;

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_solve_velocities_method, "solve_velocities_method"))
	{
		d_solve_velocities_method = DEFAULT_PARAMS.d_solve_velocities_method;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_delta_time_type, "delta_time_type"))
	{
		d_delta_time_type = DEFAULT_PARAMS.d_delta_time_type;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_delta_time, "delta_time"))
	{
		d_delta_time = DEFAULT_PARAMS.d_delta_time;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_is_boundary_smoothing_enabled, "is_boundary_smoothing_enabled"))
	{
		d_is_boundary_smoothing_enabled = DEFAULT_PARAMS.d_is_boundary_smoothing_enabled;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_boundary_smoothing_angular_half_extent_degrees,
			"boundary_smoothing_angular_half_extent_degrees"))
	{
		d_boundary_smoothing_angular_half_extent_degrees = DEFAULT_PARAMS.d_boundary_smoothing_angular_half_extent_degrees;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_exclude_deforming_regions_from_smoothing,
			"exclude_deforming_regions_from_smoothing"))
	{
		d_exclude_deforming_regions_from_smoothing = DEFAULT_PARAMS.d_exclude_deforming_regions_from_smoothing;
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesAppLogic::transcribe(
		GPlatesScribe::Scribe &scribe,
		VelocityParams::SolveVelocitiesMethodType &solve_velocities_method_type,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	//          So don't change the string ids even if the enum name changes.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("SOLVE_VELOCITIES_OF_SURFACES_AT_DOMAIN_POINTS", VelocityParams::SOLVE_VELOCITIES_OF_SURFACES_AT_DOMAIN_POINTS),
		GPlatesScribe::EnumValue("SOLVE_VELOCITIES_OF_DOMAIN_POINTS", VelocityParams::SOLVE_VELOCITIES_OF_DOMAIN_POINTS)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			solve_velocities_method_type,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}
