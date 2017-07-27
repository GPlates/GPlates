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

#include "TopologyReconstruct.h"

#include "scribe/Scribe.h"
#include "scribe/TranscribeEnumProtocol.h"


const double
GPlatesAppLogic::ReconstructParams::INITIAL_VGP_DELTA_T = 5.;

// Topology reconstruction parameters.
const double GPlatesAppLogic::ReconstructParams::INITIAL_TIME_RANGE_END = 0.0;
const double GPlatesAppLogic::ReconstructParams::INITIAL_TIME_RANGE_BEGIN = 20.0; 
const double GPlatesAppLogic::ReconstructParams::INITIAL_TIME_RANGE_INCREMENT = 1.0;
const double GPlatesAppLogic::ReconstructParams::INITIAL_LINE_TESSELLATION_DEGREES = 0.5;

GPlatesAppLogic::ReconstructParams::ReconstructParams() :
	d_reconstruct_by_plate_id_outside_active_time_period(false),
	d_vgp_visibility_setting(DELTA_T_AROUND_AGE),
	d_vgp_earliest_time(GPlatesPropertyValues::GeoTimeInstant::create_distant_past()),
	d_vgp_latest_time(GPlatesPropertyValues::GeoTimeInstant::create_distant_future()),
	d_vgp_delta_t(INITIAL_VGP_DELTA_T),
	d_reconstruct_using_topologies(false),
	d_topology_reconstruction_end_time(INITIAL_TIME_RANGE_END),
	d_topology_reconstruction_begin_time(INITIAL_TIME_RANGE_BEGIN),
	d_topology_reconstruction_time_increment(INITIAL_TIME_RANGE_INCREMENT),
	d_topology_deformation_use_natural_neighbour_interpolation(true),
	d_topology_reconstruction_use_time_of_appearance(false),
	d_topology_reconstruction_enable_line_tessellation(true),
	d_topology_reconstruction_line_tessellation_degrees(INITIAL_LINE_TESSELLATION_DEGREES),
	d_topology_reconstruction_enable_lifetime_detection(true),
	d_topology_reconstruction_lifetime_detection_threshold_velocity_delta(
			TopologyReconstruct::DEFAULT_ACTIVE_POINT_PARAMETERS.threshold_velocity_delta),
	d_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary(
			TopologyReconstruct::DEFAULT_ACTIVE_POINT_PARAMETERS.threshold_distance_to_boundary_in_kms_per_my)
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
		d_reconstruct_using_topologies == rhs.d_reconstruct_using_topologies &&
		d_topology_reconstruction_end_time == rhs.d_topology_reconstruction_end_time &&
		d_topology_reconstruction_begin_time == rhs.d_topology_reconstruction_begin_time &&
		d_topology_reconstruction_time_increment == rhs.d_topology_reconstruction_time_increment &&
		d_topology_deformation_use_natural_neighbour_interpolation == rhs.d_topology_deformation_use_natural_neighbour_interpolation &&
		d_topology_reconstruction_use_time_of_appearance == rhs.d_topology_reconstruction_use_time_of_appearance &&
		d_topology_reconstruction_enable_line_tessellation == rhs.d_topology_reconstruction_enable_line_tessellation &&
		d_topology_reconstruction_line_tessellation_degrees == rhs.d_topology_reconstruction_line_tessellation_degrees &&
		d_topology_reconstruction_enable_lifetime_detection == rhs.d_topology_reconstruction_enable_lifetime_detection &&
		d_topology_reconstruction_lifetime_detection_threshold_velocity_delta == rhs.d_topology_reconstruction_lifetime_detection_threshold_velocity_delta &&
		d_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary == rhs.d_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary;
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

	if (d_reconstruct_using_topologies < rhs.d_reconstruct_using_topologies)
	{
		return true;
	}
	if (d_reconstruct_using_topologies > rhs.d_reconstruct_using_topologies)
	{
		return false;
	}

	if (d_topology_reconstruction_end_time < rhs.d_topology_reconstruction_end_time)
	{
		return true;
	}
	if (d_topology_reconstruction_end_time > rhs.d_topology_reconstruction_end_time)
	{
		return false;
	}

	if (d_topology_reconstruction_begin_time < rhs.d_topology_reconstruction_begin_time)
	{
		return true;
	}
	if (d_topology_reconstruction_begin_time > rhs.d_topology_reconstruction_begin_time)
	{
		return false;
	}

	if (d_topology_reconstruction_time_increment < rhs.d_topology_reconstruction_time_increment)
	{
		return true;
	}
	if (d_topology_reconstruction_time_increment > rhs.d_topology_reconstruction_time_increment)
	{
		return false;
	}

	if (d_topology_deformation_use_natural_neighbour_interpolation < rhs.d_topology_deformation_use_natural_neighbour_interpolation)
	{
		return true;
	}
	if (d_topology_deformation_use_natural_neighbour_interpolation > rhs.d_topology_deformation_use_natural_neighbour_interpolation)
	{
		return false;
	}

	if (d_topology_reconstruction_use_time_of_appearance < rhs.d_topology_reconstruction_use_time_of_appearance)
	{
		return true;
	}
	if (d_topology_reconstruction_use_time_of_appearance > rhs.d_topology_reconstruction_use_time_of_appearance)
	{
		return false;
	}

	if (d_topology_reconstruction_enable_line_tessellation < rhs.d_topology_reconstruction_enable_line_tessellation)
	{
		return true;
	}
	if (d_topology_reconstruction_enable_line_tessellation > rhs.d_topology_reconstruction_enable_line_tessellation)
	{
		return false;
	}

	if (d_topology_reconstruction_line_tessellation_degrees < rhs.d_topology_reconstruction_line_tessellation_degrees)
	{
		return true;
	}
	if (d_topology_reconstruction_line_tessellation_degrees > rhs.d_topology_reconstruction_line_tessellation_degrees)
	{
		return false;
	}

	if (d_topology_reconstruction_enable_lifetime_detection < rhs.d_topology_reconstruction_enable_lifetime_detection)
	{
		return true;
	}
	if (d_topology_reconstruction_enable_lifetime_detection > rhs.d_topology_reconstruction_enable_lifetime_detection)
	{
		return false;
	}

	if (d_topology_reconstruction_lifetime_detection_threshold_velocity_delta < rhs.d_topology_reconstruction_lifetime_detection_threshold_velocity_delta)
	{
		return true;
	}
	if (d_topology_reconstruction_lifetime_detection_threshold_velocity_delta > rhs.d_topology_reconstruction_lifetime_detection_threshold_velocity_delta)
	{
		return false;
	}

	if (d_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary < rhs.d_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary)
	{
		return true;
	}
	if (d_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary > rhs.d_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary)
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
	// Provide default values for failed parameters instead of returning failure.
	// This way a future version of GPlates can add or remove parameters and still be backward/forward compatible.
	static const ReconstructParams DEFAULT_PARAMS;

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_reconstruct_by_plate_id_outside_active_time_period,
				"reconstruct_by_plate_id_outside_active_time_period"))
	{
		d_reconstruct_by_plate_id_outside_active_time_period = DEFAULT_PARAMS.d_reconstruct_by_plate_id_outside_active_time_period;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_vgp_earliest_time, "vgp_earliest_time"))
	{
		d_vgp_earliest_time = DEFAULT_PARAMS.d_vgp_earliest_time;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_vgp_latest_time, "vgp_latest_time"))
	{
		d_vgp_latest_time = DEFAULT_PARAMS.d_vgp_latest_time;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_vgp_delta_t, "vgp_delta_t"))
	{
		d_vgp_delta_t = DEFAULT_PARAMS.d_vgp_delta_t;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_reconstruct_using_topologies, "reconstruct_using_topologies"))
	{
		d_reconstruct_using_topologies = DEFAULT_PARAMS.d_reconstruct_using_topologies;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_topology_reconstruction_end_time,
			// Keeping original tag name for compatibility...
			"deformation_end_time"))
	{
		d_topology_reconstruction_end_time = DEFAULT_PARAMS.d_topology_reconstruction_end_time;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_topology_reconstruction_begin_time,
			// Keeping original tag name for compatibility...
			"deformation_begin_time"))
	{
		d_topology_reconstruction_begin_time = DEFAULT_PARAMS.d_topology_reconstruction_begin_time;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_topology_reconstruction_time_increment,
			// Keeping original tag name for compatibility...
			"deformation_time_increment"))
	{
		d_topology_reconstruction_time_increment = DEFAULT_PARAMS.d_topology_reconstruction_time_increment;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_topology_deformation_use_natural_neighbour_interpolation,
			// Using similar tag name as original tags above...
			"deformation_use_natural_neighbour_interpolation"))
	{
		d_topology_deformation_use_natural_neighbour_interpolation =
				DEFAULT_PARAMS.d_topology_deformation_use_natural_neighbour_interpolation;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_topology_reconstruction_use_time_of_appearance,
			// Using similar tag name as original tags above...
			"deformation_use_time_of_appearance"))
	{
		d_topology_reconstruction_use_time_of_appearance = DEFAULT_PARAMS.d_topology_reconstruction_use_time_of_appearance;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_topology_reconstruction_enable_line_tessellation,
			// Using similar tag name as original tags above...
			"deformation_enable_line_tessellation"))
	{
		d_topology_reconstruction_enable_line_tessellation = DEFAULT_PARAMS.d_topology_reconstruction_enable_line_tessellation;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_topology_reconstruction_line_tessellation_degrees,
			// Using similar tag name as original tags above...
			"deformation_line_tessellation_degrees"))
	{
		d_topology_reconstruction_line_tessellation_degrees = DEFAULT_PARAMS.d_topology_reconstruction_line_tessellation_degrees;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_topology_reconstruction_enable_lifetime_detection,
			// Using similar tag name as original tags above...
			"deformation_enable_lifetime_detection"))
	{
		d_topology_reconstruction_enable_lifetime_detection = DEFAULT_PARAMS.d_topology_reconstruction_enable_lifetime_detection;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_topology_reconstruction_lifetime_detection_threshold_velocity_delta,
			// Using similar tag name as original tags above...
			"deformation_lifetime_detection_threshold_velocity_delta"))
	{
		d_topology_reconstruction_lifetime_detection_threshold_velocity_delta =
				DEFAULT_PARAMS.d_topology_reconstruction_lifetime_detection_threshold_velocity_delta;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary,
			// Using similar tag name as original tags above...
			"deformation_lifetime_detection_threshold_distance_to_boundary"))
	{
		d_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary =
				DEFAULT_PARAMS.d_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary;
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_vgp_visibility_setting, "vgp_visibility_setting"))
	{
		d_vgp_visibility_setting = DEFAULT_PARAMS.d_vgp_visibility_setting;
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
	//          So don't change the string ids even if the enum name changes.
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
