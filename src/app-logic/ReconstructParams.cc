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


// Topology reconstruction parameters.
const double GPlatesAppLogic::ReconstructParams::INITIAL_TIME_RANGE_END = 0.0;
const double GPlatesAppLogic::ReconstructParams::INITIAL_TIME_RANGE_BEGIN = 20.0; 
const double GPlatesAppLogic::ReconstructParams::INITIAL_TIME_RANGE_INCREMENT = 1.0;
const double GPlatesAppLogic::ReconstructParams::INITIAL_LINE_TESSELLATION_DEGREES = 0.5;

GPlatesAppLogic::ReconstructParams::ReconstructParams() :
	d_reconstruct_by_plate_id_outside_active_time_period(false),
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
			TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_THRESHOLD_VELOCITY_DELTA),
	d_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary(
			TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_THRESHOLD_DISTANCE_TO_BOUNDARY_IN_KMS_PER_MY),
	d_topology_reconstruction_deactivate_points_that_fall_outside_a_network(
			TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_DEACTIVATE_POINTS_THAT_FALL_OUTSIDE_A_NETWORK)
{
}


bool
GPlatesAppLogic::ReconstructParams::operator==(
		const ReconstructParams &rhs) const
{
	return
		d_reconstruct_by_plate_id_outside_active_time_period == rhs.d_reconstruct_by_plate_id_outside_active_time_period &&
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
		d_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary == rhs.d_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary &&
		d_topology_reconstruction_deactivate_points_that_fall_outside_a_network == rhs.d_topology_reconstruction_deactivate_points_that_fall_outside_a_network;
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

	if (d_topology_reconstruction_deactivate_points_that_fall_outside_a_network < rhs.d_topology_reconstruction_deactivate_points_that_fall_outside_a_network)
	{
		return true;
	}
	if (d_topology_reconstruction_deactivate_points_that_fall_outside_a_network > rhs.d_topology_reconstruction_deactivate_points_that_fall_outside_a_network)
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

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_topology_reconstruction_deactivate_points_that_fall_outside_a_network,
			// Using similar tag name as original tags above...
			"deformation_deactivate_points_that_fall_outside_a_network"))
	{
		d_topology_reconstruction_deactivate_points_that_fall_outside_a_network =
				DEFAULT_PARAMS.d_topology_reconstruction_deactivate_points_that_fall_outside_a_network;
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
