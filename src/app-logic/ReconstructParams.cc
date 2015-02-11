/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
 
#include "ReconstructParams.h"

#include "scribe/Scribe.h"
#include "scribe/TranscribeEnumProtocol.h"


const double
GPlatesAppLogic::ReconstructParams::INITIAL_VGP_DELTA_T = 5.;

// Interpolate settings
const double GPlatesAppLogic::ReconstructParams::INITIAL_INTERPOLATE_START_T =  0.0;
const double GPlatesAppLogic::ReconstructParams::INITIAL_INTERPOLATE_FINAL_T =  20.0; 
const double GPlatesAppLogic::ReconstructParams::INITIAL_INTERPOLATE_DELTA_T =  1.0; 

GPlatesAppLogic::ReconstructParams::ReconstructParams() :
	d_reconstruct_by_plate_id_outside_active_time_period(false),
	d_vgp_visibility_setting(DELTA_T_AROUND_AGE),
	d_vgp_earliest_time(GPlatesPropertyValues::GeoTimeInstant::create_distant_past()),
	d_vgp_latest_time(GPlatesPropertyValues::GeoTimeInstant::create_distant_future()),
	d_vgp_delta_t(INITIAL_VGP_DELTA_T),
	d_deformation_end_time( INITIAL_INTERPOLATE_START_T ),
	d_deformation_begin_time( INITIAL_INTERPOLATE_FINAL_T ),
	d_deformation_time_increment( INITIAL_INTERPOLATE_DELTA_T )
{
}


bool
GPlatesAppLogic::ReconstructParams::should_draw_vgp(
		double current_time,
		const boost::optional<double> &age) const
{
	// Check the render settings and use them to decide if the vgp should be drawn for 
	// the current time.
	
	GPlatesPropertyValues::GeoTimeInstant geo_time = 
							GPlatesPropertyValues::GeoTimeInstant(current_time);

	switch(d_vgp_visibility_setting)
	{
		case ALWAYS_VISIBLE:
			return true;
		case TIME_WINDOW:
			if ( geo_time.is_later_than_or_coincident_with(d_vgp_earliest_time) && 
				 geo_time.is_earlier_than_or_coincident_with(d_vgp_latest_time) )
			{
				return true;
			}
			break;
		case DELTA_T_AROUND_AGE:
			if (age)
			{
				GPlatesPropertyValues::GeoTimeInstant earliest_time =
					GPlatesPropertyValues::GeoTimeInstant(*age + d_vgp_delta_t.dval());
				GPlatesPropertyValues::GeoTimeInstant latest_time =
					GPlatesPropertyValues::GeoTimeInstant(*age - d_vgp_delta_t.dval());
				
				if ((geo_time.is_later_than_or_coincident_with(earliest_time)) &&
					(geo_time.is_earlier_than_or_coincident_with(latest_time)))
				{
					return true;
				}
			}
			break;			
	}
	return false;
}


bool
GPlatesAppLogic::ReconstructParams::operator==(
		const ReconstructParams &rhs) const
{
	return
		d_reconstruct_by_plate_id_outside_active_time_period == rhs.d_reconstruct_by_plate_id_outside_active_time_period &&
		d_vgp_visibility_setting == rhs.d_vgp_visibility_setting &&
		d_vgp_earliest_time == rhs.d_vgp_earliest_time &&
		d_vgp_latest_time == rhs.d_vgp_latest_time &&
		d_vgp_delta_t == rhs.d_vgp_delta_t &&
		d_deformation_end_time == rhs.d_deformation_end_time &&
		d_deformation_begin_time == rhs.d_deformation_begin_time &&
		d_deformation_time_increment == rhs.d_deformation_time_increment;
}


bool
GPlatesAppLogic::ReconstructParams::operator<(
		const ReconstructParams &rhs) const
{
	if (d_reconstruct_by_plate_id_outside_active_time_period < rhs.d_reconstruct_by_plate_id_outside_active_time_period)
	{
		return true;
	}
	if (d_reconstruct_by_plate_id_outside_active_time_period > rhs.d_reconstruct_by_plate_id_outside_active_time_period)
	{
		return false;
	}

	if (d_vgp_visibility_setting < rhs.d_vgp_visibility_setting)
	{
		return true;
	}
	if (d_vgp_visibility_setting > rhs.d_vgp_visibility_setting)
	{
		return false;
	}

	if (d_vgp_earliest_time < rhs.d_vgp_earliest_time)
	{
		return true;
	}
	if (d_vgp_earliest_time > rhs.d_vgp_earliest_time)
	{
		return false;
	}

	if (d_vgp_latest_time < rhs.d_vgp_latest_time)
	{
		return true;
	}
	if (d_vgp_latest_time > rhs.d_vgp_latest_time)
	{
		return false;
	}

	if (d_vgp_delta_t < rhs.d_vgp_delta_t)
	{
		return true;
	}
	if (d_vgp_delta_t > rhs.d_vgp_delta_t)
	{
		return false;
	}

	if (d_deformation_end_time < rhs.d_deformation_end_time)
	{
		return true;
	}
	if (d_deformation_end_time > rhs.d_deformation_end_time)
	{
		return false;
	}

	if (d_deformation_begin_time < rhs.d_deformation_begin_time)
	{
		return true;
	}
	if (d_deformation_begin_time > rhs.d_deformation_begin_time)
	{
		return false;
	}

	if (d_deformation_time_increment < rhs.d_deformation_time_increment)
	{
		return true;
	}
	if (d_deformation_time_increment > rhs.d_deformation_time_increment)
	{
		return false;
	}

	return false;
}


GPlatesScribe::TranscribeResult
GPlatesAppLogic::ReconstructParams::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	//
	// Use default values if unable to load values from archive (better for backward/forward compatibility).
	//
	ReconstructParams default_reconstruct_params;

	if (!scribe.transcribe(
			TRANSCRIBE_SOURCE,
			d_reconstruct_by_plate_id_outside_active_time_period,
			"d_reconstruct_by_plate_id_outside_active_time_period"))
	{
		d_reconstruct_by_plate_id_outside_active_time_period =
				default_reconstruct_params.get_reconstruct_by_plate_id_outside_active_time_period();
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_vgp_visibility_setting, "d_vgp_visibility_setting"))
	{
		d_vgp_visibility_setting = default_reconstruct_params.get_vgp_visibility_setting();
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_vgp_earliest_time, "d_vgp_earliest_time"))
	{
		d_vgp_earliest_time = default_reconstruct_params.get_vgp_earliest_time();
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_vgp_latest_time, "d_vgp_latest_time"))
	{
		d_vgp_latest_time = default_reconstruct_params.get_vgp_latest_time();
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_vgp_delta_t, "d_vgp_delta_t"))
	{
		d_vgp_delta_t = default_reconstruct_params.get_vgp_delta_t();
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_deformation_end_time, "d_deformation_end_time"))
	{
		d_deformation_end_time = default_reconstruct_params.get_deformation_end_time();
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_deformation_begin_time, "d_deformation_begin_time"))
	{
		d_deformation_begin_time = default_reconstruct_params.get_deformation_begin_time();
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_deformation_time_increment, "d_deformation_time_increment"))
	{
		d_deformation_time_increment = default_reconstruct_params.get_deformation_time_increment();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesAppLogic::transcribe(
		GPlatesScribe::Scribe &scribe,
		ReconstructParams::VGPVisibilitySetting &vgp_visibility_setting,
		bool transcribed_construct_data)
{
	// WARNING: Changing the string ids will break backward/forward compatibility.
	static const GPlatesScribe::EnumValue enum_values[] =
	{
		GPlatesScribe::EnumValue("ALWAYS_VISIBLE", ReconstructParams::ALWAYS_VISIBLE),
		GPlatesScribe::EnumValue("TIME_WINDOW", ReconstructParams::TIME_WINDOW),
		GPlatesScribe::EnumValue("DELTA_T_AROUND_AGE", ReconstructParams::DELTA_T_AROUND_AGE)
	};

	return GPlatesScribe::transcribe_enum_protocol(
			TRANSCRIBE_SOURCE,
			scribe,
			vgp_visibility_setting,
			enum_values,
			enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
}
