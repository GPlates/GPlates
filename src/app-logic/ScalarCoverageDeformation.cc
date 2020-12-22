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

#include "ReconstructionGeometryUtils.h"
#include "TopologyReconstruct.h"
#include "TopologyReconstructedFeatureGeometry.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::ScalarCoverageTimeSpan(
		const initial_scalar_coverage_type &initial_scalar_coverage) :
	// We have no deformation (no geometry time span) and hence no scalars can be evolved,
	// so they're all non-evolved...
	d_non_evolved_scalar_coverage(initial_scalar_coverage),
	d_scalar_import_time(0.0)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!initial_scalar_coverage.empty(),
			GPLATES_ASSERTION_SOURCE);

	// Get the number of scalar values from the first scalar type.
	// Next we'll ensure the number of scalar values in the other scalar types matches.
	d_num_all_scalar_values = initial_scalar_coverage.begin()->second.size();
	for (const auto &scalar_coverage_item : initial_scalar_coverage)
	{
		const std::vector<double> &scalar_values = scalar_coverage_item.second;

		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				scalar_values.size() == d_num_all_scalar_values,
				GPLATES_ASSERTION_SOURCE);
	}
}


GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::ScalarCoverageTimeSpan(
		const initial_scalar_coverage_type &initial_scalar_coverage,
		TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span) :
	d_geometry_time_span(geometry_time_span),
	d_scalar_import_time(geometry_time_span->get_geometry_import_time())
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!initial_scalar_coverage.empty(),
			GPLATES_ASSERTION_SOURCE);

	// Get the number of scalar values from the first scalar type.
	// Next we'll ensure the number of scalar values in the other scalar types matches.
	const unsigned int num_original_scalar_values = initial_scalar_coverage.begin()->second.size();
	for (const auto &scalar_coverage_item : initial_scalar_coverage)
	{
		const std::vector<double> &scalar_values = scalar_coverage_item.second;

		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				scalar_values.size() == num_original_scalar_values,
				GPLATES_ASSERTION_SOURCE);
	}

	// Add the scalar values of each scalar type as either evolved or non-evolved
	// (depending on whether the scalar type is an evolved type or not).
	ScalarCoverageEvolution::InitialEvolvedScalarCoverage initial_evolved_scalar_coverage;
	for (const auto &scalar_coverage_item : initial_scalar_coverage)
	{
		const scalar_type_type &scalar_type = scalar_coverage_item.first;
		const std::vector<double> &initial_scalar_values = scalar_coverage_item.second;

		// The import scalar values might be interpolated versions of the initial scalar values if
		// the geometry in the time span was tessellated.
		const std::vector<double> import_scalar_values =
				create_import_scalar_values(initial_scalar_values, geometry_time_span);

		// The actual number of scalar values (per scalar type).
		// There might be more than the original scalar values if the geometry in the time span was tessellated.
		//
		// We're repeating this assignment a bit, but the number of scalar values shouldn't change.
		d_num_all_scalar_values = import_scalar_values.size();

		// Is the current scalar type an evolved type?
		if (boost::optional<ScalarCoverageEvolution::EvolvedScalarType> evolved_scalar_type =
			ScalarCoverageEvolution::is_evolved_scalar_type(scalar_type))
		{
			initial_evolved_scalar_coverage.add_scalar_values(evolved_scalar_type.get(), import_scalar_values);
		}
		else
		{
			d_non_evolved_scalar_coverage[scalar_type] = import_scalar_values;
		}
	}

	// Create and initialise a time span, for the evolved scalar coverage, if it has any evolved scalar types.
	if (!initial_evolved_scalar_coverage.empty())
	{
		// The import scalar values (initial scalar values).
		const ScalarCoverageEvolution::EvolvedScalarCoverage import_evolved_scalar_coverage(initial_evolved_scalar_coverage);

		d_evolved_scalar_coverage_time_span = evolved_scalar_coverage_time_span_type::create(
				geometry_time_span->get_time_range(),
				// The function to create a scalar coverage sample in rigid (non-deforming) regions...
				boost::bind(
						&ScalarCoverageTimeSpan::create_evolved_rigid_sample,
						this,
						_1, _2, _3),
				// The function to interpolate evolved scalar coverage time samples...
				boost::bind(
						&ScalarCoverageTimeSpan::interpolate_envolved_samples,
						this,
						_1, _2, _3, _4, _5),
				// Present day sample...
				//
				// The initial scalar values (at 'd_scalar_import_time').
				// Note that we'll need to modify this if 'd_scalar_import_time' is earlier
				// than the end of the time range since might be affected by time range...
				import_evolved_scalar_coverage);

		initialise_evolved_time_span(
				geometry_time_span,
				d_evolved_scalar_coverage_time_span.get(),
				import_evolved_scalar_coverage);
	}
}


void
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::initialise_evolved_time_span(
		const TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type &geometry_time_span,
		const evolved_scalar_coverage_time_span_type::non_null_ptr_type &evolved_scalar_coverage_time_span,
		const ScalarCoverageEvolution::EvolvedScalarCoverage &import_evolved_scalar_coverage)
{
	const TimeSpanUtils::TimeRange time_range = evolved_scalar_coverage_time_span->get_time_range();
	const unsigned int num_time_slots = time_range.get_num_time_slots();

	// Find the nearest time slot to the scalar import time (if it's inside the time range).
	//
	// NOTE: This mirrors what is done with the domain geometry associated with the scalar coverage.
	boost::optional<unsigned int> scalar_import_time_slot = time_range.get_nearest_time_slot(d_scalar_import_time);
	if (scalar_import_time_slot)
	{
		//
		// The scalar import time is within the time range.
		//

		// Note that we don't need to adjust the scalar import time to match the nearest time slot because
		// the geometry time span has already done that (it has the same time range as us).
		//
		// Ideally we should probably get deformation strains (from the geometry time span)
		// at the actual geometry import time and evolve the imported coverage to the nearest time slot
		// (and geometry time span should do likewise for itself), but if the user has chosen a large
		// time increment in their time range then the time slots will be spaced far apart and the
		// resulting accuracy will suffer (and this is a part of that).

		// Store the imported scalar coverage in the import time slot.
		evolved_scalar_coverage_time_span->set_sample_in_time_slot(
				import_evolved_scalar_coverage,
				scalar_import_time_slot.get()/*time_slot*/);

		// Iterate over the time range going *backwards* in time from the scalar import time (most recent)
		// to the beginning of the time range (least recent).
		evolve_time_steps(
				scalar_import_time_slot.get()/*start_time_slot*/,
				0/*end_time_slot*/,
				geometry_time_span,
				evolved_scalar_coverage_time_span,
				import_evolved_scalar_coverage);

		// Iterate over the time range going *forward* in time from the scalar import time (least recent)
		// to the end of the time range (most recent).
		evolve_time_steps(
				scalar_import_time_slot.get()/*start_time_slot*/,
				num_time_slots - 1/*end_time_slot*/,
				geometry_time_span,
				evolved_scalar_coverage_time_span,
				import_evolved_scalar_coverage);
	}
	else if (d_scalar_import_time > time_range.get_begin_time())
	{
		// The scalar import time is older than the beginning of the time range.
		// Since there's no deformation (evolution) of scalar values from the import time to the
		// beginning of the time range, the scalars at the beginning of the time range are
		// the same as those at the scalar import time.

		// Store the imported scalar values in the beginning time slot.
		evolved_scalar_coverage_time_span->set_sample_in_time_slot(
				import_evolved_scalar_coverage,
				0/*time_slot*/);

		// Iterate over the time range going *forward* in time from the beginning of the
		// time range (least recent) to the end (most recent).
		evolve_time_steps(
				0/*start_time_slot*/,
				num_time_slots - 1/*end_time_slot*/,
				geometry_time_span,
				evolved_scalar_coverage_time_span,
				import_evolved_scalar_coverage);
	}
	else // d_scalar_import_time < time_range.get_end_time() ...
	{
		// The scalar import time is younger than the end of the time range.
		// Since there's no deformation (evolution) of scalar values from the end of the time range
		// to the import time, the scalars at the end of the time range are
		// the same as those at the scalar import time.

		// Store the imported scalar values in the end time slot.
		evolved_scalar_coverage_time_span->set_sample_in_time_slot(
				import_evolved_scalar_coverage,
				num_time_slots - 1/*time_slot*/);

		// Iterate over the time range going *backwards* in time from the end of the
		// time range (most recent) to the beginning (least recent).
		evolve_time_steps(
				num_time_slots - 1/*start_time_slot*/,
				0/*end_time_slot*/,
				geometry_time_span,
				evolved_scalar_coverage_time_span,
				import_evolved_scalar_coverage);
	}
}


bool
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::evolve_time_steps(
		unsigned int start_time_slot,
		unsigned int end_time_slot,
		const TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type &geometry_time_span,
		const evolved_scalar_coverage_time_span_type::non_null_ptr_type &evolved_scalar_coverage_time_span,
		const ScalarCoverageEvolution::EvolvedScalarCoverage &import_evolved_scalar_coverage)
{
	if (start_time_slot == end_time_slot)
	{
		return true;
	}

	const TimeSpanUtils::TimeRange time_range = geometry_time_span->get_time_range();

	// Used to evolve scalar values from one time step to the next (starting with the import scalar values).
	ScalarCoverageEvolution scalar_coverage_evolution(
			import_evolved_scalar_coverage,
			time_range.get_time(start_time_slot));  // start time

	const unsigned int num_scalars = scalar_coverage_evolution.get_num_scalar_values();

	const bool reverse_reconstruct = end_time_slot > start_time_slot;
	const int time_slot_direction = (end_time_slot > start_time_slot) ? 1 : -1;

	typedef std::vector< boost::optional<GPlatesMaths::PointOnSphere> > domain_point_seq_type;
	typedef std::vector< boost::optional<DeformationStrainRate> > domain_strain_rate_seq_type;

	// Get the domain strain rates (if any) for the first time slot in the loop.
	// Note that initially all geometry points should be active (as are all our initial scalar values).
	domain_strain_rate_seq_type current_domain_strain_rates;
	geometry_time_span->get_all_geometry_data(
			time_range.get_time(start_time_slot),
			boost::none/*point_locations*/,
			boost::none/*points*/,
			current_domain_strain_rates/*strain rates*/);

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			current_domain_strain_rates.size() == num_scalars,
			GPLATES_ASSERTION_SOURCE);

	// Iterate over the time slots either backward or forward in time (depending on 'time_slot_direction').
	for (unsigned int time_slot = start_time_slot; time_slot != end_time_slot; time_slot += time_slot_direction)
	{
		const unsigned int current_time_slot = time_slot;
		const unsigned int next_time_slot = current_time_slot + time_slot_direction;

		const double next_time = time_range.get_time(next_time_slot);

		// Get the domain strain rates (if any) for the next time slot in the loop.
		domain_strain_rate_seq_type next_domain_strain_rates;

		const bool scalar_values_sample_active =
				geometry_time_span->get_all_geometry_data(next_time, boost::none, boost::none, next_domain_strain_rates);
		if (!scalar_values_sample_active)
		{
			// Next time slot is not active - so the last active time slot is the current time slot.
			if (reverse_reconstruct) // forward in time ...
			{
				d_time_slot_of_disappearance = current_time_slot;

				// Note that the end sample is inactive and so 'is_valid()' will return false for times
				// between the end of the time range and present day. And so we don't need to transfer
				// the end sample to the present day sample.
			}
			else // backward in time ...
			{
				d_time_slot_of_appearance = current_time_slot;
			}

			return false;
		}

		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				next_domain_strain_rates.size() == num_scalars,
				GPLATES_ASSERTION_SOURCE);

		// Evolve the current scalar values to the next time slot.
		scalar_coverage_evolution.evolve(
				current_domain_strain_rates,
				next_domain_strain_rates,
				next_time);

		// Set the current domain strain rates for the next time step.
		current_domain_strain_rates.swap(next_domain_strain_rates);

		// Set the (evolved) scalar values for the next time slot.
		evolved_scalar_coverage_time_span->set_sample_in_time_slot(
				scalar_coverage_evolution.get_current_evolved_scalar_coverage(),
				next_time_slot);
	}

	if (reverse_reconstruct) // forward in time ...
	{
		// The end sample is active so use it to set the present day sample since the
		// present day sample might have been affected by any deformation within the time range.
		evolved_scalar_coverage_time_span->get_present_day_sample() =
				scalar_coverage_evolution.get_current_evolved_scalar_coverage();
	}

	return true;
}


bool
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::is_valid(
		const double &reconstruction_time) const
{
	//
	// Note: This function mirrors 'TopologyReconstruct::GeometryTimeSpan::is_valid()' so that both remain in sync.
	//

	if (d_geometry_time_span &&
		// Note that these cannot be true unless there's a geometry time span anyway
		// (so we don't strictly need to check for the geometry time span)...
		(d_time_slot_of_appearance || d_time_slot_of_disappearance))
	{
		const TimeSpanUtils::TimeRange time_range = d_geometry_time_span.get()->get_time_range();

		// Determine the two nearest time slots bounding the reconstruction time (if any).
		double interpolate_time_slots;
		const boost::optional< std::pair<unsigned int/*first_time_slot*/, unsigned int/*second_time_slot*/> >
				reconstruction_time_slots = time_range.get_bounding_time_slots(reconstruction_time, interpolate_time_slots);
		if (reconstruction_time_slots)
		{
			// If the scalar coverage has a time of appearance (time slot in the time range) and the reconstruction
			// time slot is prior to it then the scalar coverage has not appeared yet.
			if (d_time_slot_of_appearance &&
				reconstruction_time_slots->first < d_time_slot_of_appearance.get())
			{
				return false;
			}

			// If the scalar coverage has a time of disappearance (time slot in the time range) and the reconstruction
			// time slot is after it then the scalar coverage has already disappeared.
			if (d_time_slot_of_disappearance &&
				reconstruction_time_slots->second > d_time_slot_of_disappearance.get())
			{
				return false;
			}
		}
		else // reconstruction time is outside the time range...
		{
			// If the scalar coverage has a time of appearance (time slot in the time range) and the reconstruction
			// time is prior to the beginning of the time range then the scalar coverage has not appeared yet.
			if (d_time_slot_of_appearance &&
				reconstruction_time >= time_range.get_begin_time())
			{
				return false;
			}

			// If the scalar coverage has a time of disappearance (time slot in the time range) and the reconstruction
			// time is after the end of the time range then the scalar coverage has already disappeared.
			if (d_time_slot_of_disappearance &&
				reconstruction_time <= time_range.get_end_time())
			{
				return false;
			}
		}
	}

	return true;
}


boost::optional<GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::ScalarCoverage>
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::get_scalar_coverage(
		const double &reconstruction_time) const
{
	if (!is_valid(reconstruction_time))
	{
		// The geometry/scalars is not valid/active at the reconstruction time.
		return boost::none;
	}

	// TODO: Implement.
	return boost::none;
}


bool
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::get_scalar_values(
		const scalar_type_type &scalar_type,
		const double &reconstruction_time,
		std::vector<double> &scalar_values) const
{
	std::vector< boost::optional<double> > all_scalar_values;
	if (!get_all_scalar_values(scalar_type, reconstruction_time, all_scalar_values))
	{
		return false;
	}

	// Return active scalar values to the caller.
	for (const boost::optional<double> &scalar_value : all_scalar_values)
	{
		if (scalar_value)
		{
			scalar_values.push_back(scalar_value.get());
		}
	}

	return true;
}


bool
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::get_all_scalar_values(
		const scalar_type_type &scalar_type,
		const double &reconstruction_time,
		std::vector< boost::optional<double> > &scalar_values) const
{
	if (!is_valid(reconstruction_time))
	{
		// The geometry/scalars is not valid/active at the reconstruction time.
		return false;
	}

	//
	// First look in the *non-evolved* scalar coverage.
	//

	auto non_evolved_iter = d_non_evolved_scalar_coverage.find(scalar_type);
	if (non_evolved_iter != d_non_evolved_scalar_coverage.end())
	{
		const std::vector<double> &non_evolved_scalar_values = non_evolved_iter->second;

		scalar_values.assign(non_evolved_scalar_values.begin(), non_evolved_scalar_values.end());

		return true;
	}

	//
	// Next look in the *evolved* scalar coverage.
	//

	if (d_evolved_scalar_coverage_time_span)
	{
		if (boost::optional<ScalarCoverageEvolution::EvolvedScalarType> evolved_scalar_type =
			ScalarCoverageEvolution::is_evolved_scalar_type(scalar_type))
		{
			const ScalarCoverageEvolution::EvolvedScalarCoverage evolved_scalar_coverage =
					d_evolved_scalar_coverage_time_span.get()->get_or_create_sample(reconstruction_time);

			scalar_values = evolved_scalar_coverage.get_evolved_scalar_values(evolved_scalar_type.get());

			return true;
		}
	}

	// The specified scalar type is not contained in this scalar coverage.
	return false;
}


GPlatesAppLogic::ScalarCoverageEvolution::EvolvedScalarCoverage
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::create_evolved_rigid_sample(
		const double &reconstruction_time,
		const double &closest_younger_sample_time,
		const ScalarCoverageEvolution::EvolvedScalarCoverage &closest_younger_sample)
{
	// Simply return the closest younger sample.
	// We are in a rigid region so the scalar values have not changed since deformation (closest younger sample).
	return closest_younger_sample;
}


GPlatesAppLogic::ScalarCoverageEvolution::EvolvedScalarCoverage
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::interpolate_envolved_samples(
		const double &interpolate_position,
		const double &first_geometry_time,
		const double &second_geometry_time,
		const ScalarCoverageEvolution::EvolvedScalarCoverage &first_sample,
		const ScalarCoverageEvolution::EvolvedScalarCoverage &second_sample)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			first_sample.get_num_scalar_values() == second_sample.get_num_scalar_values(),
			GPLATES_ASSERTION_SOURCE);

	const double reconstruction_time =
			(1 - interpolate_position) * first_geometry_time +
				interpolate_position * second_geometry_time;

	// NOTE: Mirror what the domain geometry time span does so that we end up with the same number of
	// *active* scalar values as *active* geometry points. If we don't get the same number then
	// later on we'll get an assertion failure.
	//
	// Determine whether to reconstruct backward or forward in time when interpolating.
	if (reconstruction_time > d_scalar_import_time)
	{
		// Reconstruct backward in time away from the scalar import time.
		// For now we'll just pick the nearest sample (to the scalar import time).
		return second_sample;
	}
	else
	{
		// Reconstruct forward in time away from the geometry import time.
		// For now we'll just pick the nearest sample (to the scalar import time).
		return first_sample;
	}
}


std::vector<double>
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::create_import_scalar_values(
		const std::vector<double> &scalar_values,
		TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span)
{
	//
	// Our domain geometry is being reconstructed using topologies so it might have been tessellated
	// in which case we'd need to introduce new scalar values to map to new tessellated geometry points.
	//

	// Get the information regarding tessellation of the original geometry points.
	const TopologyReconstruct::GeometryTimeSpan::interpolate_original_points_seq_type &
			interpolate_original_points = geometry_time_span->get_interpolate_original_points();
	const unsigned int num_interpolated_scalar_values = interpolate_original_points.size();

	// Number of original scalar values.
	const unsigned int num_scalar_values = scalar_values.size();

	// The potentially interpolated scalar values - we might be adding new interpolated
	// scalar values if the original domain geometry got tessellated.
	std::vector<double> interpolated_scalar_values;
	interpolated_scalar_values.reserve(num_interpolated_scalar_values);

	for (unsigned int n = 0; n < num_interpolated_scalar_values; ++n)
	{
		const TopologyReconstruct::GeometryTimeSpan::InterpolateOriginalPoints &
				interpolate_original_point = interpolate_original_points[n];

		// Indices should not equal (or exceed) the number of our original scalar values.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				interpolate_original_point.first_point_index < num_scalar_values &&
					interpolate_original_point.second_point_index < num_scalar_values,
				GPLATES_ASSERTION_SOURCE);

		// Interpolate the current scalar value between two original scalar values.
		// If the current scalar value maps to an original (non-tessellated) geometry point
		// then the interpolate ratio will be either 0.0 or 1.0.
		const double interpolated_scalar_value =
				(1.0 - interpolate_original_point.interpolate_ratio) *
						scalar_values[interpolate_original_point.first_point_index] +
				interpolate_original_point.interpolate_ratio *
						scalar_values[interpolate_original_point.second_point_index];

		interpolated_scalar_values.push_back(interpolated_scalar_value);
	}

	return interpolated_scalar_values;
}


boost::optional<std::vector<double>>
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::ScalarCoverage::get_scalar_values(
		const scalar_type_type &scalar_type) const
{
	boost::optional<const scalar_value_seq_type &> all_scalar_values = get_all_scalar_values(scalar_type);
	if (!all_scalar_values)
	{
		return boost::none;
	}

	std::vector<double> active_scalar_values;

	// Return active scalar values to the caller.
	for (const auto &scalar_value : all_scalar_values.get())
	{
		if (scalar_value)
		{
			active_scalar_values.push_back(scalar_value.get());
		}
	}

	return active_scalar_values;
}


boost::optional<const GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::scalar_value_seq_type &>
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::ScalarCoverage::get_all_scalar_values(
		const scalar_type_type &scalar_type) const
{
	auto iter = d_scalar_values_map.find(scalar_type);
	if (iter == d_scalar_values_map.end())
	{
		return boost::none;
	}

	return iter->second;
}
