/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include <boost/bind.hpp>

#include "ScalarCoverageDeformation.h"

#include "DeformedFeatureGeometry.h"
#include "GeometryDeformation.h"
#include "ReconstructionGeometryUtils.h"


GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::ScalarCoverageTimeSpan(
		const std::vector<double> &present_day_scalar_values) :
	d_scalar_values_time_span(
			scalar_values_time_span_type::create(
					// Use a dummy time range since we're not populating the time span...
					TimeSpanUtils::TimeRange(1.0, 0.0, 2),
					// The function to create a scalar values sample in rigid (non-deforming) regions.
					// This will always return present day scalars since we don't populate the time span...
					boost::bind(
							&ScalarCoverageTimeSpan::create_rigid_scalar_values_sample,
							_1,
							_2,
							_3),
					present_day_scalar_values))
{
}


GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::ScalarCoverageTimeSpan(
		const scalar_evolution_function_type &scalar_evolution_function,
		const domain_reconstruction_time_span_type &domain_reconstruction_time_span,
		const std::vector<double> &present_day_scalar_values) :
	d_scalar_values_time_span(
			scalar_values_time_span_type::create(
					domain_reconstruction_time_span.get_time_range(),
					// The function to create a scalar values sample in rigid (non-deforming) regions...
					boost::bind(
							&ScalarCoverageTimeSpan::create_rigid_scalar_values_sample,
							_1,
							_2,
							_3),
					present_day_scalar_values))
{
	initialise_time_span(scalar_evolution_function, domain_reconstruction_time_span);
}


void
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::initialise_time_span(
		const scalar_evolution_function_type &scalar_evolution_function,
		const domain_reconstruction_time_span_type &domain_reconstruction_time_span)
{
	// The time range of both the reconstructed domain features and the scalar values.
	const TimeSpanUtils::TimeRange time_range = d_scalar_values_time_span->get_time_range();
	const unsigned int num_time_slots = time_range.get_num_time_slots();

	// Iterate over the time range going *backwards* in time from the end of the
	// time range (most recent) to the beginning (least recent).
	for (/*signed*/ int time_slot = num_time_slots - 1; time_slot > 0; --time_slot)
	{
		const double current_time = time_range.get_time(time_slot);
		const double next_time = current_time + time_range.get_time_increment();

		boost::optional<const ReconstructedFeatureGeometry::non_null_ptr_type &> domain_rfg =
				domain_reconstruction_time_span.get_sample_in_time_slot(time_slot);
		// If there is no RFG for the current time slot then continue to the next time slot.
		// Geometry will not be stored for the current time.
		if (!domain_rfg)
		{
			// Finished with the current time sample.
			continue;
		}

		boost::optional<const DeformedFeatureGeometry *> dfg =
				ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<const DeformedFeatureGeometry *>(
						domain_rfg.get());
		// If the RFG is not a DeformedFeatureGeometry then we have no deformation strain information and
		// hence the scalar values do not evolve for the current time step so continue to the next time slot.
		// Geometry will not be stored for the current time.
		if (!dfg)
		{
			// Finished with the current time sample.
			continue;
		}

		// Get the current (per-point) deformation strain info.
		const std::vector<GeometryDeformation::DeformationInfo> &current_deformation_infos =
				dfg.get()->get_point_deformation_information();

		// TODO: Skip current time sample if all (per-point) instanstaneous strains are zero.

		// Get the scalar values for the current time.
		// This gets the scalar values for the closest younger (deformed) scalar values if needed
		// since the scalar values do not evolve during rigid periods.
		const scalar_value_seq_type current_scalar_values =
				d_scalar_values_time_span->get_or_create_sample(current_time);

		// Evolve the current scalar values to the next time slot.
		scalar_value_seq_type next_scalar_values;
		scalar_evolution_function(
				next_scalar_values,
				current_scalar_values,
				current_deformation_infos,
				current_time,
				next_time);

		// Set the (evolved) scalar values for the next time slot.
		d_scalar_values_time_span->set_sample_in_time_slot(next_scalar_values, time_slot - 1);
	}
}


std::vector<double>
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::get_scalar_values(
		const double &reconstruction_time)
{
	return d_scalar_values_time_span->get_or_create_sample(reconstruction_time);
}


GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::scalar_value_seq_type
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::create_rigid_scalar_values_sample(
		const double &reconstruction_time,
		const double &closest_younger_sample_time,
		const scalar_value_seq_type &closest_younger_sample)
{
	// Simply return the closest younger sample.
	// We are in a rigid region so the scalar values have not changed since deformation (closest younger sample).
	return closest_younger_sample;
}
