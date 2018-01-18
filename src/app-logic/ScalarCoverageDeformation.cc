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

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::ScalarCoverageTimeSpan(
		const std::vector<double> &scalar_values) :
	d_scalar_values_time_span(
			scalar_values_time_span_type::create(
					// Use a dummy time range since we're not populating the time span...
					TimeSpanUtils::TimeRange(1.0, 0.0, 2),
					// The function to create a scalar values sample in rigid (non-deforming) regions.
					// This will always return the same scalars since we don't populate the time span...
					boost::bind(
							&ScalarCoverageTimeSpan::create_rigid_scalar_values_sample,
							this,
							_1,
							_2,
							_3),
					// The function to interpolate scalar values...
					boost::bind(
							&ScalarCoverageTimeSpan::interpolate_scalar_values_sample,
							this,
							_1,
							_2,
							_3,
							_4,
							_5),
					create_import_sample(scalar_values))),
	d_scalar_import_time(0.0)
{
}


GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::ScalarCoverageTimeSpan(
		const TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type &geometry_time_span,
		const std::vector<double> &scalar_values,
		boost::optional<scalar_evolution_function_type> scalar_evolution_function) :
	d_scalar_values_time_span(
			scalar_values_time_span_type::create(
					geometry_time_span->get_time_range(),
					// The function to create a scalar values sample in rigid (non-deforming) regions...
					boost::bind(
							&ScalarCoverageTimeSpan::create_rigid_scalar_values_sample,
							this,
							_1,
							_2,
							_3),
					// The function to interpolate scalar values...
					boost::bind(
							&ScalarCoverageTimeSpan::interpolate_scalar_values_sample,
							this,
							_1,
							_2,
							_3,
							_4,
							_5),
					// The initial scalar values (at 'd_scalar_import_time').
					// Note that we'll need to modify this if 'd_scalar_import_time' is earlier
					// than the end of the time range since might be affected by time range...
					create_import_sample(scalar_values, geometry_time_span))),
	d_scalar_import_time(0.0) // will get updated later
{
	initialise_time_span(geometry_time_span, scalar_evolution_function);
}


void
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::initialise_time_span(
		const TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type &geometry_time_span,
		const boost::optional<scalar_evolution_function_type> &scalar_evolution_function)
{
	const TimeSpanUtils::TimeRange time_range = d_scalar_values_time_span->get_time_range();
	const unsigned int num_time_slots = time_range.get_num_time_slots();

	// The initial scalar values - they were stored in the present day sample but represent the
	// scalar values at the geometry import time of the geometry time span.
	// Note that initially all the scalar values are active (because all initial geometry points should be active).
	const scalar_value_seq_type import_scalar_values = d_scalar_values_time_span->get_present_day_sample();
	d_scalar_import_time = geometry_time_span->get_geometry_import_time();

	// Find the nearest time slot to the scalar import time (if it's inside the time range).
	//
	// NOTE: This mirrors what is done with the domain geometry associated with these scalar values.
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
		// at the actual geometry import time and evolve the imported scalars to the nearest time slot
		// (and geometry time span should do likewise for itself), but if the user has chosen a large
		// time increment in their time range then the time slots will be spaced far apart and the
		// resulting accuracy will suffer (and this is a part of that).

		// Store the imported scalar values in the import time slot.
		d_scalar_values_time_span->set_sample_in_time_slot(
				import_scalar_values,
				scalar_import_time_slot.get()/*time_slot*/);

		// Iterate over the time range going *backwards* in time from the scalar import time (most recent)
		// to the beginning of the time range (least recent).
		scalar_value_seq_type current_scalar_values = import_scalar_values;
		evolve_time_steps(
				current_scalar_values,
				scalar_import_time_slot.get()/*start_time_slot*/,
				0/*end_time_slot*/,
				geometry_time_span,
				scalar_evolution_function);

		// Iterate over the time range going *forward* in time from the scalar import time (least recent)
		// to the end of the time range (most recent).
		//
		// If the end sample is active then use it to set the present day sample.
		current_scalar_values = import_scalar_values;
		if (evolve_time_steps(
				current_scalar_values,
				scalar_import_time_slot.get()/*start_time_slot*/,
				num_time_slots - 1/*end_time_slot*/,
				geometry_time_span,
				scalar_evolution_function))
		{
			// Reset the present day scalar values.
			// They might have been affected by any deformation within the time range.
			d_scalar_values_time_span->get_present_day_sample() = current_scalar_values;
		}
	}
	else if (d_scalar_import_time > time_range.get_begin_time())
	{
		// The scalar import time is older than the beginning of the time range.
		// Since there's no deformation (evolution) of scalar values from the import time to the
		// beginning of the time range, the scalars at the beginning of the time range are
		// the same as those at the scalar import time.

		// Store the imported scalar values in the beginning time slot.
		d_scalar_values_time_span->set_sample_in_time_slot(
				import_scalar_values,
				0/*time_slot*/);

		// Iterate over the time range going *forward* in time from the beginning of the
		// time range (least recent) to the end (most recent).
		//
		// If the end sample is active then use it to set the present day sample.
		scalar_value_seq_type current_scalar_values = import_scalar_values;
		if (evolve_time_steps(
				current_scalar_values,
				0/*start_time_slot*/,
				num_time_slots - 1/*end_time_slot*/,
				geometry_time_span,
				scalar_evolution_function))
		{
			// Reset the present day scalar values.
			// They might have been affected by any deformation within the time range.
			d_scalar_values_time_span->get_present_day_sample() = current_scalar_values;
		}
	}
	else // d_scalar_import_time < time_range.get_end_time() ...
	{
		// The scalar import time is younger than the end of the time range.
		// Since there's no deformation (evolution) of scalar values from the end of the time range
		// to the import time, the scalars at the end of the time range are
		// the same as those at the scalar import time.

		// Store the imported scalar values in the end time slot.
		d_scalar_values_time_span->set_sample_in_time_slot(
				import_scalar_values,
				num_time_slots - 1/*time_slot*/);

		// Iterate over the time range going *backwards* in time from the end of the
		// time range (most recent) to the beginning (least recent).
		scalar_value_seq_type current_scalar_values = import_scalar_values;
		evolve_time_steps(
				current_scalar_values,
				num_time_slots - 1/*start_time_slot*/,
				0/*end_time_slot*/,
				geometry_time_span,
				scalar_evolution_function);

		// Note that we don't need to reset the present day scalar values since the scalar
		// import time is after (younger than) the end of the time range and hence the
		// present day scalar values are not affected by deformation in the time range.
	}
}


bool
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::evolve_time_steps(
		scalar_value_seq_type &current_scalar_values,
		unsigned int start_time_slot,
		unsigned int end_time_slot,
		const TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type &geometry_time_span,
		const boost::optional<scalar_evolution_function_type> &scalar_evolution_function)
{
	if (start_time_slot == end_time_slot)
	{
		return true;
	}

	const unsigned int num_scalars = current_scalar_values.size();

	const TimeSpanUtils::TimeRange time_range = d_scalar_values_time_span->get_time_range();

	const bool reverse_reconstruct = end_time_slot > start_time_slot;
	const int time_slot_direction = (end_time_slot > start_time_slot) ? 1 : -1;

	typedef std::vector< boost::optional<GPlatesMaths::PointOnSphere> > domain_point_seq_type;
	typedef std::vector< boost::optional<DeformationStrainRate> > domain_strain_rate_seq_type;

	// Get the domain strain rates (if any) for the first time slot in the loop (if evolving scalar values).
	// Note that initially all geometry points should be active (as are all our initial scalar values).
	domain_strain_rate_seq_type current_domain_strain_rates;
	if (scalar_evolution_function)
	{
		geometry_time_span->get_all_geometry_data(
				time_range.get_time(start_time_slot),
				boost::none/*points*/,
				current_domain_strain_rates/*strain rates*/);

		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				current_domain_strain_rates.size() == num_scalars,
				GPLATES_ASSERTION_SOURCE);
	}

	// Iterate over the time slots either backward or forward in time (depending on 'time_slot_direction').
	for (unsigned int time_slot = start_time_slot; time_slot != end_time_slot; time_slot += time_slot_direction)
	{
		const unsigned int current_time_slot = time_slot;
		const unsigned int next_time_slot = current_time_slot + time_slot_direction;

		const double current_time = time_range.get_time(current_time_slot);
		const double next_time = time_range.get_time(next_time_slot);

		// Get the domain points or strain rates (if any) for the next time slot in the loop.
		//
		// Note that, if we're *not* evolving scalar values, then we avoid accessing the strain rates
		// because they take longer to calculate and we don't need them (we don't really need the
		// points either - we just want the active state of the geometry points).
		domain_point_seq_type next_domain_points;
		domain_strain_rate_seq_type next_domain_strain_rates;

		const bool scalar_values_sample_active = scalar_evolution_function
				? geometry_time_span->get_all_geometry_data(next_time, boost::none, next_domain_strain_rates)
				: geometry_time_span->get_all_geometry_data(next_time, next_domain_points, boost::none);
		if (!scalar_values_sample_active)
		{
			// Next time slot is not active - so the last active time slot is the current time slot.
			if (reverse_reconstruct) // forward in time ...
			{
				d_time_slot_of_disappearance = current_time_slot;
			}
			else // backward in time ...
			{
				d_time_slot_of_appearance = current_time_slot;
			}

			return false;
		}

		// Evolve the current scalar values to the next time slot if we have a function to do that.
		if (scalar_evolution_function)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					next_domain_strain_rates.size() == num_scalars,
					GPLATES_ASSERTION_SOURCE);

			// The scalar evolution function also handles changes in active state of each point.
			scalar_evolution_function.get()(
					current_scalar_values,
					current_domain_strain_rates,
					next_domain_strain_rates,
					current_time,
					next_time);

			// Set the current domain strain rates for the next time step.
			current_domain_strain_rates.swap(next_domain_strain_rates);
		}
		else // no evolution of scalars...
		{
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					next_domain_points.size() == num_scalars,
					GPLATES_ASSERTION_SOURCE);

			// The scalar values don't change (not being evolved), however their active state can change.
			for (unsigned int n = 0; n < num_scalars; ++n)
			{
				// De-activate the current scalar value if the associated domain geometry point is inactive.
				// Note that points should only become inactive (they should never go from inactive to active)
				// so we only need to deactivate scalar values (not activate them).
				if (!next_domain_points[n])
				{
					// The current scalar is actually going to be our next scalar (transitioning from current to next).
					current_scalar_values[n] = boost::none;
				}
			}
		}

		// Set the (evolved) scalar values for the next time slot.
		d_scalar_values_time_span->set_sample_in_time_slot(current_scalar_values, next_time_slot);
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

	if (d_time_slot_of_appearance ||
		d_time_slot_of_disappearance)
	{
		const TimeSpanUtils::TimeRange time_range = d_scalar_values_time_span->get_time_range();

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


bool
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::get_scalar_values(
		const double &reconstruction_time,
		std::vector<double> &scalar_values) const
{
	if (!is_valid(reconstruction_time))
	{
		// The geometry/scalars is not valid/active at the reconstruction time.
		return false;
	}

	const scalar_value_seq_type scalar_values_sample =
			d_scalar_values_time_span->get_or_create_sample(reconstruction_time);

	// Return active scalar values to the caller.
	scalar_value_seq_type::const_iterator scalar_values_sample_iter = scalar_values_sample.begin();
	scalar_value_seq_type::const_iterator scalar_values_sample_end = scalar_values_sample.end();
	for ( ; scalar_values_sample_iter != scalar_values_sample_end; ++scalar_values_sample_iter)
	{
		const boost::optional<double> &scalar_value = *scalar_values_sample_iter;
		if (scalar_value)
		{
			scalar_values.push_back(scalar_value.get());
		}
	}

	return true;
}


bool
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::get_all_scalar_values(
		const double &reconstruction_time,
		std::vector< boost::optional<double> > &scalar_values) const
{
	if (!is_valid(reconstruction_time))
	{
		// The geometry/scalars is not valid/active at the reconstruction time.
		return false;
	}

	const scalar_value_seq_type scalar_values_sample =
			d_scalar_values_time_span->get_or_create_sample(reconstruction_time);

	// Return all scalar values to the caller.
	scalar_values.insert(
			scalar_values.end(),
			scalar_values_sample.begin(),
			scalar_values_sample.end());

	return true;
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
		const double &first_geometry_time,
		const double &second_geometry_time,
		const scalar_value_seq_type &first_sample,
		const scalar_value_seq_type &second_sample)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			first_sample.size() == second_sample.size(),
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
		// For now we'll just pick the nearest sample.
		return second_sample;
	}
	else
	{
		// Reconstruct forward in time away from the geometry import time.
		// For now we'll just pick the nearest sample.
		return first_sample;
	}
}


GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::scalar_value_seq_type
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::create_import_sample(
		const std::vector<double> &original_scalar_values,
		boost::optional<TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type> geometry_time_span)
{
	// If our domain geometry is not being reconstructing using topologies then it's not being
	// tessellated either and hence we don't need to introduce any new scalar values to map
	// to new tessellated geometry points.
	if (!geometry_time_span)
	{
		const unsigned int num_original_scalar_values = original_scalar_values.size();

		scalar_value_seq_type original_scalar_value_seq;
		original_scalar_value_seq.reserve(num_original_scalar_values);

		// Just copy the scalar values.
		for (unsigned int n = 0; n < num_original_scalar_values; ++n)
		{
			original_scalar_value_seq.push_back(original_scalar_values[n]);
		}

		return original_scalar_value_seq;
	}

	// Get the information regarding tessellation of the original geometry points.
	const TopologyReconstruct::GeometryTimeSpan::interpolate_original_points_seq_type &
			interpolate_original_points = geometry_time_span.get()->get_interpolate_original_points();
	const unsigned int num_interpolate_original_points = interpolate_original_points.size();

	// Number of original scalar values.
	const unsigned int num_original_scalar_values = original_scalar_values.size();

	// The final scalar values - we might be adding new interpolated scalar values if the
	// original domain geometry got tessellated.
	scalar_value_seq_type scalar_value_seq;
	scalar_value_seq.reserve(num_interpolate_original_points);

	for (unsigned int n = 0; n < num_interpolate_original_points; ++n)
	{
		const TopologyReconstruct::GeometryTimeSpan::InterpolateOriginalPoints &
				interpolate_original_point = interpolate_original_points[n];

		// Indices should not equal (or exceed) the number of our original scalar values.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				interpolate_original_point.first_point_index < num_original_scalar_values &&
					interpolate_original_point.second_point_index < num_original_scalar_values,
				GPLATES_ASSERTION_SOURCE);

		// Interpolate the current scalar value between two original scalar values.
		// If the current scalar value maps to an original (non-tessellated) geometry point
		// then the interpolate ratio will be either 0.0 or 1.0.
		const double scalar_value =
				(1.0 - interpolate_original_point.interpolate_ratio) *
						original_scalar_values[interpolate_original_point.first_point_index] +
				interpolate_original_point.interpolate_ratio *
						original_scalar_values[interpolate_original_point.second_point_index];

		scalar_value_seq.push_back(scalar_value);
	}

	return scalar_value_seq;
}
