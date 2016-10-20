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
#include <boost/optional.hpp>

#include "ScalarCoverageDeformation.h"

#include "DeformedFeatureGeometry.h"
#include "GeometryDeformation.h"
#include "ReconstructionGeometryUtils.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


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
					// The function to interpolate scalar values...
					boost::bind(
							&ScalarCoverageTimeSpan::interpolate_scalar_values_sample,
							_1,
							// Ignore the first and second sample times (don't need them)...
							_4,
							_5),
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
					// The function to interpolate scalar values...
					boost::bind(
							&ScalarCoverageTimeSpan::interpolate_scalar_values_sample,
							_1,
							// Ignore the first and second sample times (don't need them)...
							_4,
							_5),
					present_day_scalar_values))
{
	initialise_time_span(scalar_evolution_function, domain_reconstruction_time_span);
}


void
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::initialise_time_span(
		const scalar_evolution_function_type &scalar_evolution_function,
		const domain_reconstruction_time_span_type &domain_reconstruction_time_span)
{
	const unsigned int num_scalars = d_scalar_values_time_span->get_present_day_sample().size();

	// An array of zero deformation strain rates of size equal to the number of scalars being evolved
	// (which is also the same as the number of points in the deformed geometry).
	// This is in case either of two adjacent time slots have no deformation samples (which means the
	// missing time slot has zero strain).
	const std::vector<DeformationStrain> zero_deformation_strain_rates(num_scalars);

	// The time range of both the reconstructed domain features and the scalar values.
	const TimeSpanUtils::TimeRange time_range = d_scalar_values_time_span->get_time_range();
	const unsigned int num_time_slots = time_range.get_num_time_slots();

	/*signed*/ int time_slot = num_time_slots - 1;

	// Get the deformation strain rates (if any) for the first time slot in the loop.
	boost::optional<const std::vector<DeformationStrain> &> current_deformation_strain_rates =
			get_deformation_strain_rates(domain_reconstruction_time_span, time_slot);

	// Iterate over the time range going *backwards* in time from the end of the
	// time range (most recent) to the beginning (least recent).
	for ( ; time_slot > 0; --time_slot)
	{
		const double current_time = time_range.get_time(time_slot);
		const double next_time = current_time + time_range.get_time_increment();

		// Get the deformation strain rates (if any) for the next time slot.
		boost::optional<const std::vector<DeformationStrain> &> next_deformation_strain_rates =
				get_deformation_strain_rates(domain_reconstruction_time_span, time_slot - 1);

		// If there are no deformation strain rates for the current and next time slots then
		// deformation will have no effect on evolving the scalar values.
		// In which case we won't store scalar values for the current time.
		// Leaving the time slot empty means the closest younger time slot is used when the empty
		// time slot is accessed (the scalar values have not evolved/changed since the younger time slot).
		if (!current_deformation_strain_rates &&
			!next_deformation_strain_rates)
		{
			continue;
		}

		// Get the scalar values for the current time.
		// This gets the scalar values for the closest younger (deformed) scalar values if needed
		// since the scalar values do not evolve during rigid periods.
		scalar_value_seq_type current_scalar_values =
				d_scalar_values_time_span->get_or_create_sample(current_time);

		// Evolve the current scalar values to the next time slot.
		scalar_evolution_function(
				current_scalar_values,
				current_deformation_strain_rates ? current_deformation_strain_rates.get() : zero_deformation_strain_rates,
				next_deformation_strain_rates ? next_deformation_strain_rates.get() : zero_deformation_strain_rates,
				current_time,
				next_time);

		// Set the (evolved) scalar values for the next time slot.
		d_scalar_values_time_span->set_sample_in_time_slot(current_scalar_values, time_slot - 1);

		current_deformation_strain_rates = next_deformation_strain_rates;
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


GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::scalar_value_seq_type
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::interpolate_scalar_values_sample(
		const double &interpolate_position,
		const scalar_value_seq_type &first_sample,
		const scalar_value_seq_type &second_sample)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			first_sample.size() == second_sample.size(),
			GPLATES_ASSERTION_SOURCE);

	const unsigned int num_scalars = first_sample.size();
	scalar_value_seq_type interpolated_sample(num_scalars);

	for (unsigned int n = 0; n < num_scalars; ++n)
	{
		interpolated_sample[n] =
				(1 - interpolate_position) * first_sample[n] +
					interpolate_position * second_sample[n];
	}

	return interpolated_sample;
}


boost::optional<const std::vector<GPlatesAppLogic::DeformationStrain> &>
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::get_deformation_strain_rates(
		const domain_reconstruction_time_span_type &domain_reconstruction_time_span,
		unsigned int time_slot)
{
	boost::optional<const ReconstructedFeatureGeometry::non_null_ptr_type &> domain_rfg =
			domain_reconstruction_time_span.get_sample_in_time_slot(time_slot);
	if (!domain_rfg)
	{
		return boost::none;
	}

	boost::optional<const DeformedFeatureGeometry *> dfg =
			ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<const DeformedFeatureGeometry *>(
					domain_rfg.get());
	// If the RFG is not a DeformedFeatureGeometry then we have no deformation strain information.
	if (!dfg)
	{
		return boost::none;
	}

	// Return the current (per-point) deformation strain rates.
	return dfg.get()->get_point_deformation_strain_rates();
}
