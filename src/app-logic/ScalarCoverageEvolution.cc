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

#include <algorithm>
#include <utility>
#include <cmath>
#include <boost/bind.hpp>
#include <boost/scoped_array.hpp>
#include <boost/utility/in_place_factory.hpp>

#include "ScalarCoverageEvolution.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"


namespace GPlatesAppLogic
{
	// Thermal expansion coefficient [1/C].
	constexpr double THERMAL_ALPHA = 3.28e-5;

	// Asthenosphere temperature [C].
	constexpr double TEMPERATURE_ASTHENOSPHERE = 1350;

	// Sea water density [kg/m^3].
	constexpr double DENSITY_WATER = 1.03e3;

	// Mantle density at 0 degrees [kg/m^3].
	constexpr double DENSITY_MANTLE = 3.33e3;

	// Crust density [kg/m^3].
	constexpr double DENSITY_CRUST = 2.8e3;

	// Asthenosphere density [kg/m^3].
	const double DENSITY_ASTHENOSPHERE = DENSITY_MANTLE * (1.0 - THERMAL_ALPHA * TEMPERATURE_ASTHENOSPHERE);

	// Thickness of lithosphere (in km).
	constexpr double LITHOSPHERIC_THICKNESS_KMS = 125.0;

	// Depth resolution to use when solving the 1D temperature advection-diffusion equation for each surface point.
	// A value of 25 is equivalent to an interval of 5 km (when lithospheric thickness is 125 km).
	constexpr unsigned int NUM_TEMPERATURE_DIFFUSION_DEPTH_INTERVALS = 25;
	constexpr unsigned int NUM_TEMPERATURE_DIFFUSION_DEPTH_SAMPLES = NUM_TEMPERATURE_DIFFUSION_DEPTH_INTERVALS + 1;
	const double INVERSE_NUM_TEMPERATURE_DIFFUSION_DEPTH_INTERVALS = 1.0 / NUM_TEMPERATURE_DIFFUSION_DEPTH_INTERVALS;

	// The depth spacing between temperature diffusion depth samples (in km).
	const double TEMPERATURE_DIFFISION_DEPTH_RESOLUTION_KMS =
			LITHOSPHERIC_THICKNESS_KMS / NUM_TEMPERATURE_DIFFUSION_DEPTH_INTERVALS;

	// Limit the number of surface points to solve temperature diffusion at the same time (in a group)
	// in order to minimise memory usage for storing temperature depth profile and also, to a lesser extent,
	// to minimise CPU cache misses (when subtracting temperature profile of previous time step from
	// current time step - the previous time step will still be in the CPU cache).
	constexpr unsigned int NUM_POINTS_IN_TEMPERATURE_DIFFUSION_GROUP = 1000;
}


GPlatesAppLogic::ScalarCoverageEvolution::scalar_type_type
GPlatesAppLogic::ScalarCoverageEvolution::get_scalar_type(
		EvolvedScalarType evolved_scalar_type)
{
	static const GPlatesPropertyValues::ValueObjectType GPML_CRUSTAL_THICKNESS =
			GPlatesPropertyValues::ValueObjectType::create_gpml("CrustalThickness");

	static const GPlatesPropertyValues::ValueObjectType GPML_CRUSTAL_STRETCHING_FACTOR =
			GPlatesPropertyValues::ValueObjectType::create_gpml("CrustalStretchingFactor");

	static const GPlatesPropertyValues::ValueObjectType GPML_CRUSTAL_THINNING_FACTOR =
			GPlatesPropertyValues::ValueObjectType::create_gpml("CrustalThinningFactor");

	static const GPlatesPropertyValues::ValueObjectType GPML_TECTONIC_SUBSIDENCE =
			GPlatesPropertyValues::ValueObjectType::create_gpml("TectonicSubsidence");

	switch (evolved_scalar_type)
	{
	case CRUSTAL_THICKNESS_KMS:
		return GPML_CRUSTAL_THICKNESS;

	case CRUSTAL_STRETCHING_FACTOR:
		return GPML_CRUSTAL_STRETCHING_FACTOR;

	case CRUSTAL_THINNING_FACTOR:
		return GPML_CRUSTAL_THINNING_FACTOR;

	case TECTONIC_SUBSIDENCE_KMS:
		return GPML_TECTONIC_SUBSIDENCE;

	default:
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
	}
}


boost::optional<GPlatesAppLogic::ScalarCoverageEvolution::EvolvedScalarType>
GPlatesAppLogic::ScalarCoverageEvolution::is_evolved_scalar_type(
		const scalar_type_type &scalar_type)
{
	static const GPlatesPropertyValues::ValueObjectType GPML_CRUSTAL_THICKNESS =
			GPlatesPropertyValues::ValueObjectType::create_gpml("CrustalThickness");

	static const GPlatesPropertyValues::ValueObjectType GPML_CRUSTAL_STRETCHING_FACTOR =
			GPlatesPropertyValues::ValueObjectType::create_gpml("CrustalStretchingFactor");

	static const GPlatesPropertyValues::ValueObjectType GPML_CRUSTAL_THINNING_FACTOR =
			GPlatesPropertyValues::ValueObjectType::create_gpml("CrustalThinningFactor");

	static const GPlatesPropertyValues::ValueObjectType GPML_TECTONIC_SUBSIDENCE =
			GPlatesPropertyValues::ValueObjectType::create_gpml("TectonicSubsidence");

	if (scalar_type == GPML_CRUSTAL_THICKNESS)
	{
		return CRUSTAL_THICKNESS_KMS;
	}
	else if (scalar_type == GPML_CRUSTAL_STRETCHING_FACTOR)
	{
		return CRUSTAL_STRETCHING_FACTOR;
	}
	else if (scalar_type == GPML_CRUSTAL_THINNING_FACTOR)
	{
		return CRUSTAL_THINNING_FACTOR;
	}
	else if (scalar_type == GPML_TECTONIC_SUBSIDENCE)
	{
		return TECTONIC_SUBSIDENCE_KMS;
	}

	return boost::none;
}


GPlatesAppLogic::ScalarCoverageEvolution::ScalarCoverageEvolution(
		const InitialEvolvedScalarCoverage &initial_scalar_coverage,
		const double &initial_time,
		TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span) :
	d_geometry_time_span(geometry_time_span),
	d_initial_scalar_coverage(initial_scalar_coverage),
	d_initial_time(initial_time),
	d_num_scalar_values(initial_scalar_coverage.get_num_scalar_values()),
	d_scalar_coverage_time_span(
			time_span_type::create(
					geometry_time_span->get_time_range(),
					// The function to create a scalar coverage sample in rigid (non-deforming) regions...
					boost::bind(
							&ScalarCoverageEvolution::create_time_span_rigid_sample,
							this, _1, _2, _3),
					// The function to interpolate evolved scalar coverage time samples...
					boost::bind(
							&ScalarCoverageEvolution::interpolate_time_span_samples,
							this, _1, _2, _3, _4, _5),
					// Present day sample...
					//
					// Note that we'll modify this if the initial time is earlier than (before) the end
					// of the time range since deformation in time range can change present day scalar values...
					EvolvedScalarCoverage::create(d_num_scalar_values))),
	d_have_initialised_tectonic_subsidence(false)
{
	initialise();
}


void
GPlatesAppLogic::ScalarCoverageEvolution::initialise()
{
	PROFILE_FUNC();

	// The scalar coverage at the import time was stored in the present day sample.
	EvolvedScalarCoverage::non_null_ptr_type import_scalar_coverage =
			d_scalar_coverage_time_span->get_present_day_sample();

	const TimeSpanUtils::TimeRange time_range = d_scalar_coverage_time_span->get_time_range();
	const unsigned int num_time_slots = time_range.get_num_time_slots();

	// Find the nearest time slot to the initial time (if it's inside the time range).
	//
	// NOTE: This mirrors what is done with the domain geometry associated with the scalar coverage.
	boost::optional<unsigned int> initial_time_slot = time_range.get_nearest_time_slot(d_initial_time);
	if (initial_time_slot)
	{
		//
		// The initial time is within the time range.
		//

		// Note that we don't need to adjust the initial time to match the nearest time slot because
		// the geometry time span has already done that (it has the same time range as us).
		//
		// Ideally we should probably get deformation strains (from the geometry time span)
		// at the actual geometry import time and evolve the initial coverage to the nearest time slot
		// (and geometry time span should do likewise for itself), but if the user has chosen a large
		// time increment in their time range then the time slots will be spaced far apart and the
		// resulting accuracy will suffer (and this is a part of that).

		// Store the initial scalar coverage in the initial time slot.
		d_scalar_coverage_time_span->set_sample_in_time_slot(
				import_scalar_coverage,
				initial_time_slot.get()/*time_slot*/);

		// Iterate over the time range going *backwards* in time from the initial time (most recent)
		// to the beginning of the time range (least recent).
		evolve_time_steps(
				initial_time_slot.get()/*start_time_slot*/,
				0/*end_time_slot*/);

		// Iterate over the time range going *forward* in time from the initial time (least recent)
		// to the end of the time range (most recent).
		evolve_time_steps(
				initial_time_slot.get()/*start_time_slot*/,
				num_time_slots - 1/*end_time_slot*/);
	}
	else if (d_initial_time > time_range.get_begin_time())
	{
		// The initial time is older than the beginning of the time range.
		// Since there's no deformation (evolution) of scalar values from the initial time to the
		// beginning of the time range, the scalars at the beginning of the time range are
		// the same as those at the initial time.

		// Store the initial scalar values in the beginning time slot.
		d_scalar_coverage_time_span->set_sample_in_time_slot(
				import_scalar_coverage,
				0/*time_slot*/);

		// Iterate over the time range going *forward* in time from the beginning of the
		// time range (least recent) to the end (most recent).
		evolve_time_steps(
				0/*start_time_slot*/,
				num_time_slots - 1/*end_time_slot*/);
	}
	else // initial_time < time_range.get_end_time() ...
	{
		// The initial time is younger than the end of the time range.
		// Since there's no deformation (evolution) of scalar values from the end of the time range
		// to the initial time, the scalars at the end of the time range are
		// the same as those at the initial time.

		// Store the initial scalar values in the end time slot.
		d_scalar_coverage_time_span->set_sample_in_time_slot(
				import_scalar_coverage,
				num_time_slots - 1/*time_slot*/);

		// Iterate over the time range going *backwards* in time from the end of the
		// time range (most recent) to the beginning (least recent).
		evolve_time_steps(
				num_time_slots - 1/*start_time_slot*/,
				0/*end_time_slot*/);
	}
}


void
GPlatesAppLogic::ScalarCoverageEvolution::evolve_time_steps(
		unsigned int start_time_slot,
		unsigned int end_time_slot)
{
	if (start_time_slot == end_time_slot)
	{
		return;
	}

	const TimeSpanUtils::TimeRange time_range = d_geometry_time_span->get_time_range();
	const double start_time = time_range.get_time(start_time_slot);

	const bool reverse_reconstruct = end_time_slot > start_time_slot;
	const int time_slot_direction = (end_time_slot > start_time_slot) ? 1 : -1;

	// Get the domain strain rates (if any) for the first time slot in the loop.
	// Note that initially all geometry points should be active (as are all our initial scalar values).
	typedef std::vector< boost::optional<DeformationStrainRate> > domain_strain_rate_seq_type;
	domain_strain_rate_seq_type current_domain_strain_rates;
	d_geometry_time_span->get_all_geometry_data(
			start_time,
			boost::none/*points*/,
			boost::none/*points_locations*/,
			current_domain_strain_rates/*strain rates*/);

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			current_domain_strain_rates.size() == d_num_scalar_values,
			GPLATES_ASSERTION_SOURCE);

	// Start with the default coverage state (crustal thickness *factor*) which we'll evolve and copy
	// into the next scalar coverage and so on.
	boost::optional<EvolvedScalarCoverage::non_null_ptr_type> current_scalar_coverage;
	EvolvedScalarCoverage::State current_scalar_coverage_state(d_num_scalar_values);

	// Iterate over the time slots either backward or forward in time (depending on 'time_slot_direction').
	for (unsigned int time_slot = start_time_slot; time_slot != end_time_slot; time_slot += time_slot_direction)
	{
		const unsigned int current_time_slot = time_slot;
		const unsigned int next_time_slot = current_time_slot + time_slot_direction;

		const double current_time = time_range.get_time(current_time_slot);
		const double next_time = time_range.get_time(next_time_slot);

		// Get the domain strain rates (if any) for the next time slot in the loop.
		domain_strain_rate_seq_type next_domain_strain_rates;

		if (!d_geometry_time_span->get_all_geometry_data(next_time, boost::none, boost::none, next_domain_strain_rates))
		{
			// Return early - the next time slot is not active - so the last active time slot is the current time slot.
			return;
		}

		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				next_domain_strain_rates.size() == d_num_scalar_values,
				GPLATES_ASSERTION_SOURCE);

		// Evolve the current scalar values to the next time slot.
		//
		// Note: This updates the *current* state so that it becomes the *next* state.
		evolve_time_step(
				current_scalar_coverage_state,
				current_domain_strain_rates,
				next_domain_strain_rates,
				current_time,
				next_time);

		// Set the (evolved) scalar values for the next time slot.
		EvolvedScalarCoverage::non_null_ptr_type next_scalar_coverage =
				EvolvedScalarCoverage::create(current_scalar_coverage_state);
		d_scalar_coverage_time_span->set_sample_in_time_slot(
				next_scalar_coverage,
				next_time_slot);

		// Set the current scalar coverage.
		current_scalar_coverage = next_scalar_coverage;

		// Set the current domain strain rates for the next time step.
		current_domain_strain_rates.swap(next_domain_strain_rates);
	}

	if (reverse_reconstruct) // forward in time ...
	{
		// We should only be able to get here if we have a current scalar coverage
		// (ie, start_time_slot != end_time_slot).
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				current_scalar_coverage,
				GPLATES_ASSERTION_SOURCE);

		// The end sample is active so use it to set the present day sample since the
		// present day sample might have been affected by any deformation within the time range.
		d_scalar_coverage_time_span->get_present_day_sample() = current_scalar_coverage.get();
	}
}


void
GPlatesAppLogic::ScalarCoverageEvolution::evolve_time_step(
		EvolvedScalarCoverage::State &current_scalar_coverage_state,
		const std::vector< boost::optional<DeformationStrainRate> > &current_deformation_strain_rates,
		const std::vector< boost::optional<DeformationStrainRate> > &next_deformation_strain_rates,
		const double &current_time,
		const double &next_time)
{
	// Input array sizes should match.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_num_scalar_values == current_deformation_strain_rates.size() &&
				d_num_scalar_values == next_deformation_strain_rates.size(),
			GPLATES_ASSERTION_SOURCE);

	double time_increment = next_time - current_time;
	bool forward_in_time;
	if (time_increment < 0)
	{
		// Time increment should always be positive.
		time_increment = -time_increment;

		// We're going forward in time (from old to young times).
		forward_in_time = true;
	}
	else
	{
		// We're going backward in time (from young to old times).
		forward_in_time = false;
	}
	const bool time_increment_greater_than_one = time_increment > 1 + 1e-6;

	//
	// Strain rates are in 1/sec (multiplying by this number converts to 1/My).
	//
	const double seconds_in_a_million_years = 365.25 * 24 * 3600 * 1.0e6;

	//
	// Evolve the current scalar values from the current time to the next time.
	//
	// Note that 'boost::optional' is used for each point's strain rate (at current and next times).
	// This represents whether the associated point is active. Points can become inactive over time (active->inactive) but
	// do not get re-activated (inactive->active). So if the current strain rate is inactive then so should the next strain rate.
	// Also the active state of the current scalar value should match that of the current deformation strain rate
	// (because they represent the same point). If the current scalar value is inactive then it just remains inactive.
	// And if the current scalar value is active then it becomes inactive if the next strain rate is inactive, otherwise
	// both (current and next) strain rates are active and the scalar value is evolved from its current value to its next value.
	// This ensures the active state of the next scalar values match that of the next deformation strain rate
	// (which in turn comes from the active state of the associated domain geometry point).
	//

	for (unsigned int scalar_value_index = 0; scalar_value_index < d_num_scalar_values; ++scalar_value_index)
	{
		// If current scalar value is inactive then it remains inactive.
		if (!current_scalar_coverage_state.scalar_values_are_active[scalar_value_index])
		{
			// If the current scalar value is inactive then so must the current (and next) dilatation strain rates.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					!current_deformation_strain_rates[scalar_value_index] &&
						!next_deformation_strain_rates[scalar_value_index],
					GPLATES_ASSERTION_SOURCE);
			continue;
		}

		// The current scalar value is active so the current dilatation strain rate must also be active.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				current_deformation_strain_rates[scalar_value_index],
				GPLATES_ASSERTION_SOURCE);

		// If the next strain rate is inactive then the scalar value becomes inactive (for the next time step).
		if (!next_deformation_strain_rates[scalar_value_index])
		{
			// The current scalar value is no longer active (for the next time step).
			current_scalar_coverage_state.scalar_values_are_active[scalar_value_index] = false;

			continue;
		}

		double current_dilatation_per_my = seconds_in_a_million_years *
				current_deformation_strain_rates[scalar_value_index]->get_strain_rate_dilatation();
		double next_dilatation_per_my = seconds_in_a_million_years *
				next_deformation_strain_rates[scalar_value_index]->get_strain_rate_dilatation();

		//
		// The rate of change of crustal thickness is (going forward in time):
		//
		//   dH/dt = H' = -H * S
		//
		// ...where S is the strain rate dilatation.
		//
		// We use the central difference scheme to solve the above ordinary differential equation (ODE):
		//
		//   H(n+1) - H(n)
		//   ------------- = (H'(n+1) + H'(n)) / 2
		//         dt
		//
		//                 = (-H(n+1) * S(n+1) + -H(n) * S(n)) / 2
		//
		//   H(n+1) * (1 + S(n+1)*dt/2) = H(n) * (1 - S(n)*dt/2)
		//
		//   H(n+1) = H(n) * (1 - S(n)*dt/2) / (1 + S(n+1)*dt/2)
		//
		// However we make a slight variation where we replace both S(n) and S(n+1) by their average.
		// This helps to smooth out fluctuations in the dilatation strain rate.
		//
		//   H(n+1) = H(n) * (1 - k) / (1 + k)
		//
		// ...with...
		//
		//        k = (S(n) + S(n+1))/2 * dt/2
		//
		// We also individually clamp S(n) and S(n+1) before taking the average.
		// This is so that '1 - k' and '1 + k' don't become unstable in the above equation
		// (in other words we want |k| < 1 so that '1 - k' and '1 + k' can't become negative, since
		// a negative crustal thickness makes no sense).
		// Note that clamping the strain rate (S) is usually a good idea anyway since strain rates can
		// get excessively large at times (in some parts of some topological networks).
		//

		// Clamp dilatation to 1.0 in units of 1/Myr, which is equivalent to 3.17e-14 in units of 1/second.
		// This is about 6 times the default clamping (disabled by default) of 5e-15 1/second in a
		// topological network visual layer, and so the user still has the option to clamp further than this
		// (in other words we're not clamping too excessively here).
		//
		// This clamping is equivalent to clamping 'k' to 0.5 (when dt=1My).

		if (current_dilatation_per_my > 1)
		{
			current_dilatation_per_my = 1;
		}
		else if (current_dilatation_per_my < -1)
		{
			current_dilatation_per_my = -1;
		}

		if (next_dilatation_per_my > 1)
		{
			next_dilatation_per_my = 1;
		}
		else if (next_dilatation_per_my < -1)
		{
			next_dilatation_per_my = -1;
		}

		const double average_dilatation_per_my = 0.5 * (current_dilatation_per_my + next_dilatation_per_my);

		const double k = 0.5 * time_increment * average_dilatation_per_my;

		double crustal_thickness_multiplier;
		if (time_increment_greater_than_one)
		{
			// Time increment is > 1My, so there's still a chance of instability due to |k| >= 1
			// (because our clamping assumed a time increment of 1My).
			//
			// But even if there's no instability we'll just proceed with a time increment of 1My
			// because that gets us accuracy comparable to a time increment of 1My with little extra effort
			// (although we're not getting dilatation strain rates every 1My, so it's not as accurate as
			// a 1My time increment).
			// To do this note that we can write:
			//
			//   H(n+1) = H(n) * [(1 - k/dt) / (1 + k/dt)] ^ dt
			//
			// ...noting that 'n+1' and 'n' are separated by one interval of 'dt' which can be *larger* than 1My,
			// and 'k/dt' is essentially equivalent to the k value for a 1My time increment.
			// So the above equation is basically calculating:
			// 
			//   H(t=t0+dt) = (1 - k/dt) / (1 + k/dt) * H(t=t0+dt-1)
			//              = (1 - k/dt) / (1 + k/dt) * (1 - k/dt) / (1 + k/dt) * H(t=t0+dt-2)
			//              = ... * H(t=t0)
			//              = ([(1 - k/dt) / (1 + k/dt)] ^ dt) * H(t=t0)
			// 
			const double k_over_1my = k / time_increment;

			crustal_thickness_multiplier = (1.0 - k_over_1my) / (1.0 + k_over_1my);

			crustal_thickness_multiplier = std::pow(crustal_thickness_multiplier, time_increment);
		}
		else
		{
			// Time increment is <= 1My, so there's no chance of instability due to |k| >= 1.
			crustal_thickness_multiplier = (1.0 - k) / (1.0 + k);
		}

		// The crustal thinning equation assumes we're going forward in time.
		// So if we're going backward in time then we need to invert the multiplier.
		//
		//   H(n+1) = m(n+1) * H(n)
		//   H(n)   = H(n+1) / m(n+1)
		//
		// Note that this also has the benefit of making crustal thinning reversible - in the sense
		// that you could start at t0 and solve backward in time to tn and then solve forward in time
		// to t0 and you'd end up with the same crustal thickness you started with.
		// 
		if (!forward_in_time)
		{
			crustal_thickness_multiplier = 1.0 / crustal_thickness_multiplier;
		}

		// Update the crustal thickness factor (ratio of crustal thickness to initial crustal thickness).
		current_scalar_coverage_state.crustal_thickness_factor[scalar_value_index] *= crustal_thickness_multiplier;
	}
}


void
GPlatesAppLogic::ScalarCoverageEvolution::initialise_tectonic_subsidence() const
{
	PROFILE_FUNC();

	const TimeSpanUtils::TimeRange time_range = d_scalar_coverage_time_span->get_time_range();
	const unsigned int num_time_slots = time_range.get_num_time_slots();
	if (num_time_slots == 0)
	{
		return;
	}

	//
	// Working space for lithospheric temperature calculations.
	//
	// Store temperature advection-diffusion depth samples for a group of surface points for two time steps.
	// We'll ping-pong between these two time buffers as we evolve the temperature depth profile through time.
	boost::scoped_array<double> temperature_depth_working_space_1(
			new double[NUM_POINTS_IN_TEMPERATURE_DIFFUSION_GROUP * NUM_TEMPERATURE_DIFFUSION_DEPTH_SAMPLES]);
	boost::scoped_array<double> temperature_depth_working_space_2(
			new double[NUM_POINTS_IN_TEMPERATURE_DIFFUSION_GROUP * NUM_TEMPERATURE_DIFFUSION_DEPTH_SAMPLES]);
	// Store a lithospheric temperature (integrated over depth) for each time slot for each point.
	boost::scoped_array<double> lithospheric_temperature_integrated_over_depth_kms_working_space(
			new double[num_time_slots * NUM_POINTS_IN_TEMPERATURE_DIFFUSION_GROUP]);

	unsigned int num_temperature_diffusion_groups = d_num_scalar_values / NUM_POINTS_IN_TEMPERATURE_DIFFUSION_GROUP;
	// Might be a last group if there's a remainder.
	if ((d_num_scalar_values % NUM_POINTS_IN_TEMPERATURE_DIFFUSION_GROUP) != 0)
	{
		++num_temperature_diffusion_groups;
	}

	// Iterate over the groups.
	for (unsigned int group_index = 0; group_index < num_temperature_diffusion_groups; ++group_index)
	{
		// Start/end indices into scalar values of the current group.
		const unsigned int scalar_values_start_index = group_index * NUM_POINTS_IN_TEMPERATURE_DIFFUSION_GROUP;
		unsigned int scalar_values_end_index = scalar_values_start_index + NUM_POINTS_IN_TEMPERATURE_DIFFUSION_GROUP;
		if (scalar_values_end_index > d_num_scalar_values)
		{
			scalar_values_end_index = d_num_scalar_values;
		}

		// We haven't yet started evolving lithospheric temperature for any points in the current group.
		// For each point, when it first becomes active, we'll initialise it with a linear temperature-depth profile.
		std::vector<bool> have_started_evolving_lithospheric_temperature(
				scalar_values_end_index - scalar_values_start_index,  // num scalar values in current group
				false);

		// Starting with a linear temperature gradient through the depth of the lithosphere
		// calculate the 1D temperature (with depth) through time at each surface point location.
		// Crustal stretching causes advection of hot asthenosphere (increasing temperature) while
		// diffusion cools back towards the original linear temperature gradient.
		evolve_lithospheric_temperature(
				have_started_evolving_lithospheric_temperature,
				lithospheric_temperature_integrated_over_depth_kms_working_space.get(),
				temperature_depth_working_space_1.get(),
				temperature_depth_working_space_2.get(),
				scalar_values_start_index,
				scalar_values_end_index);

		// The temperature changes in turn cause density changes that, along with density changes due to
		// crustal stretching (since crustal and mantle densities differ), result in isostatic subsidence.
		evolve_tectonic_subsidence(
				lithospheric_temperature_integrated_over_depth_kms_working_space.get(),
				scalar_values_start_index,
				scalar_values_end_index);
	}
}


void
GPlatesAppLogic::ScalarCoverageEvolution::evolve_lithospheric_temperature(
		std::vector<bool> &have_started_evolving_lithospheric_temperature,
		double *const lithospheric_temperature_integrated_over_depth_kms,
		double *current_temperature_depth,
		double *next_temperature_depth,
		unsigned int scalar_values_start_index,
		unsigned int scalar_values_end_index) const
{
	PROFILE_FUNC();

	const TimeSpanUtils::TimeRange time_range = d_scalar_coverage_time_span->get_time_range();
	const unsigned int num_time_slots = time_range.get_num_time_slots();
	if (num_time_slots == 0)
	{
		return;
	}

	boost::optional<EvolvedScalarCoverage::non_null_ptr_type &> current_scalar_coverage;

	// Find the first active scalar coverage (going forward in time).
	unsigned int time_slot;
	for (time_slot = 0; time_slot < num_time_slots - 1 /*end time slot*/; ++time_slot)
	{
		current_scalar_coverage = d_scalar_coverage_time_span->get_sample_in_time_slot(time_slot);
		if (current_scalar_coverage)
		{
			break;
		}
	}

	if (!current_scalar_coverage)
	{
		// Shouldn't be able to get here since at least the initial/import time slot should be active,
		// but if no scalar coverages time slots are active then nothing to do, so return early.
		return;
	}

	// Get the domain strain rates for the current time slot.
	typedef std::vector< boost::optional<DeformationStrainRate> > domain_strain_rate_seq_type;
	domain_strain_rate_seq_type current_domain_strain_rates;
	d_geometry_time_span->get_all_geometry_data(
			time_range.get_time(time_slot),
			boost::none/*points*/,
			boost::none/*points_locations*/,
			current_domain_strain_rates/*strain rates*/,
			boost::none,/*strains*/
			scalar_values_start_index,
			scalar_values_end_index);
	// We should have active strain rates since we have an active scalar coverage for the current time slot.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			current_domain_strain_rates.size() == scalar_values_end_index - scalar_values_start_index,
			GPLATES_ASSERTION_SOURCE);

	// Iterate over the rest of the time range going *forward* in time until/if we find an inactive slot.
	for ( ; time_slot < num_time_slots - 1 /*end time slot*/; ++time_slot)
	{
		const unsigned int current_time_slot = time_slot;
		const unsigned int next_time_slot = current_time_slot + 1;

		// Get the next scalar coverage in the next time slot.
		boost::optional<EvolvedScalarCoverage::non_null_ptr_type &> next_scalar_coverage =
				d_scalar_coverage_time_span->get_sample_in_time_slot(next_time_slot);
		if (!next_scalar_coverage)
		{
			// Return early - the next time slot is not active - so the last active time slot is the current time slot.
			return;
		}

		// Get the domain strain rates for the next time slot in the loop.
		domain_strain_rate_seq_type next_domain_strain_rates;
		d_geometry_time_span->get_all_geometry_data(
				time_range.get_time(next_time_slot),
				boost::none/*points*/,
				boost::none/*points_locations*/,
				next_domain_strain_rates,
				boost::none,/*strains*/
				scalar_values_start_index,
				scalar_values_end_index);
		// We should have active strain rates since we have an active scalar coverage for the next time slot.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				next_domain_strain_rates.size() == scalar_values_end_index - scalar_values_start_index,
				GPLATES_ASSERTION_SOURCE);

		// Each lithospheric-temperature-integrated-over-depth array contains
		// 'scalar_values_end_index - scalar_values_start_index' values, which is equal to
		// NUM_POINTS_IN_TEMPERATURE_DIFFUSION_GROUP for all but the last group.
		double *const current_lithospheric_temperature_integrated_over_depth_kms =
				lithospheric_temperature_integrated_over_depth_kms + current_time_slot * NUM_POINTS_IN_TEMPERATURE_DIFFUSION_GROUP;
		double *const next_lithospheric_temperature_integrated_over_depth_kms =
				lithospheric_temperature_integrated_over_depth_kms + next_time_slot * NUM_POINTS_IN_TEMPERATURE_DIFFUSION_GROUP;

		// Evolve the current lithospheric temperature depth profile into the next time slot.
		evolve_lithospheric_temperature_time_step(
				current_domain_strain_rates,
				next_domain_strain_rates,
				current_scalar_coverage.get()->state,
				next_scalar_coverage.get()->state,
				have_started_evolving_lithospheric_temperature,
				current_lithospheric_temperature_integrated_over_depth_kms,
				next_lithospheric_temperature_integrated_over_depth_kms,
				current_temperature_depth,
				next_temperature_depth,
				scalar_values_start_index,
				scalar_values_end_index);

		// Swap the current and next temperature depth profiles.
		//
		// The next temperature depth profile in the current time step becomes the
		// current temperature depth profile in the next time step.
		//
		// And the space occupied by the current temperature depth profile in the current time step
		// will be used for the next temperature depth profile in the next time step.
		std::swap(current_temperature_depth, next_temperature_depth);

		// Set the current domain strain rates for the next time step.
		current_domain_strain_rates.swap(next_domain_strain_rates);

		// Set the current scalar coverage for the next time step.
		current_scalar_coverage = next_scalar_coverage;
	}
}


void
GPlatesAppLogic::ScalarCoverageEvolution::evolve_lithospheric_temperature_time_step(
		const std::vector< boost::optional<DeformationStrainRate> > &current_deformation_strain_rates,
		const std::vector< boost::optional<DeformationStrainRate> > &next_deformation_strain_rates,
		const EvolvedScalarCoverage::State &current_scalar_coverage_state,
		EvolvedScalarCoverage::State &next_scalar_coverage_state,
		// Each of the following three arrays contains
		// 'scalar_values_end_index - scalar_values_start_index' values...
		std::vector<bool> &have_started_evolving_lithospheric_temperature,
		double *const current_lithospheric_temperature_integrated_over_depth_kms,
		double *const next_lithospheric_temperature_integrated_over_depth_kms,
		// Each of the 'scalar_values_end_index - scalar_values_start_index' points (scalar values) in
		// temperature-depth profile contains NUM_TEMPERATURE_DIFFUSION_DEPTH_SAMPLES depth samples...
		double *const current_temperature_depth,
		double *const next_temperature_depth,
		unsigned int scalar_values_start_index,
		unsigned int scalar_values_end_index) const
{
	// Input array sizes should match.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			scalar_values_end_index - scalar_values_start_index == current_deformation_strain_rates.size() &&
				scalar_values_end_index - scalar_values_start_index == next_deformation_strain_rates.size(),
			GPLATES_ASSERTION_SOURCE);

	//
	// Strain rates are in 1/sec (multiplying by this number converts to 1/My).
	//
	const double seconds_in_a_million_years = 365.25 * 24 * 3600 * 1.0e6;

	for (unsigned int scalar_value_index = scalar_values_start_index;
		scalar_value_index < scalar_values_end_index;
		++scalar_value_index)
	{
		// If the current scalar value is not active then either we've not reached the active time span of the
		// current point or we've already gone past it (forward in time). Either way there's nothing to do.
		if (!current_scalar_coverage_state.scalar_values_are_active[scalar_value_index])
		{
			continue;
		}

		const unsigned int scalar_value_index_in_group = scalar_value_index - scalar_values_start_index;

		// Pointers to the start of the temperature-depth profile of the current point (scalar value)
		// for the current and next times.
		double *const current_temperature_depth_of_current_point = current_temperature_depth +
				scalar_value_index_in_group * NUM_TEMPERATURE_DIFFUSION_DEPTH_SAMPLES;
		double *const next_temperature_depth_of_current_point = next_temperature_depth +
				scalar_value_index_in_group * NUM_TEMPERATURE_DIFFUSION_DEPTH_SAMPLES;

		// Initialise starting temperature profile (linear) if first time visiting the current point.
		if (!have_started_evolving_lithospheric_temperature[scalar_value_index_in_group])
		{
			// Initial temperature profile is linear from 0 C at surface to Asthenosphere temperature at bottom of lithosphere.
			for (unsigned int d = 0; d < NUM_TEMPERATURE_DIFFUSION_DEPTH_SAMPLES; ++d)
			{
				current_temperature_depth_of_current_point[d] = double(d) *
						INVERSE_NUM_TEMPERATURE_DIFFUSION_DEPTH_INTERVALS * TEMPERATURE_ASTHENOSPHERE;
			}

			// Integrate the temperature-depth profile over depth.
			double current_lithospheric_temperature_trapezoidal_sum =
					0.5 * current_temperature_depth_of_current_point[0] +
					0.5 * current_temperature_depth_of_current_point[NUM_TEMPERATURE_DIFFUSION_DEPTH_SAMPLES - 1];
			for (unsigned int d = 1; d < NUM_TEMPERATURE_DIFFUSION_DEPTH_SAMPLES - 1; ++d)
			{
				current_lithospheric_temperature_trapezoidal_sum += current_temperature_depth_of_current_point[d];
			}
			// Store integral in caller's array.
			current_lithospheric_temperature_integrated_over_depth_kms[scalar_value_index_in_group] =
					current_lithospheric_temperature_trapezoidal_sum * TEMPERATURE_DIFFISION_DEPTH_RESOLUTION_KMS;

			have_started_evolving_lithospheric_temperature[scalar_value_index_in_group] = true;
		}

		// If the next scalar value is not active then we cannot evolve temperature to the next time,
		// in which case we've just reached the end of the active time span for the current point.
		if (!next_scalar_coverage_state.scalar_values_are_active[scalar_value_index])
		{
			continue;
		}

		// The current and next scalar values are active so the current and next dilatation strain rates must also.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				current_deformation_strain_rates[scalar_value_index_in_group] &&
					next_deformation_strain_rates[scalar_value_index_in_group],
				GPLATES_ASSERTION_SOURCE);

		double current_dilatation_per_my = seconds_in_a_million_years *
				current_deformation_strain_rates[scalar_value_index_in_group]->get_strain_rate_dilatation();
		double next_dilatation_per_my = seconds_in_a_million_years *
				next_deformation_strain_rates[scalar_value_index_in_group]->get_strain_rate_dilatation();

		// TODO: Solve temperature advection-diffusion equation from current time to next time.
		//       In the meantime we'll just copy linear profile from current time to next time
		//       (which means lithospheric temperature advection-diffusion will have no effect on subsidence).
		for (unsigned int d = 0; d < NUM_TEMPERATURE_DIFFUSION_DEPTH_SAMPLES; ++d)
		{
			next_temperature_depth_of_current_point[d] = current_temperature_depth_of_current_point[d];
		}

		// Integrate the temperature-depth profile over depth.
		double next_lithospheric_temperature_trapezoidal_sum =
				0.5 * next_temperature_depth_of_current_point[0] +
				0.5 * next_temperature_depth_of_current_point[NUM_TEMPERATURE_DIFFUSION_DEPTH_SAMPLES - 1];
		for (unsigned int d = 1; d < NUM_TEMPERATURE_DIFFUSION_DEPTH_SAMPLES - 1; ++d)
		{
			next_lithospheric_temperature_trapezoidal_sum += next_temperature_depth_of_current_point[d];
		}
		// Store in caller's array.
		next_lithospheric_temperature_integrated_over_depth_kms[scalar_value_index_in_group] =
				next_lithospheric_temperature_trapezoidal_sum * TEMPERATURE_DIFFISION_DEPTH_RESOLUTION_KMS;
	}
}


void
GPlatesAppLogic::ScalarCoverageEvolution::evolve_tectonic_subsidence(
		const double *const lithospheric_temperature_integrated_over_depth_kms,
		unsigned int scalar_values_start_index,
		unsigned int scalar_values_end_index) const
{
	const TimeSpanUtils::TimeRange time_range = d_scalar_coverage_time_span->get_time_range();
	const unsigned int num_time_slots = time_range.get_num_time_slots();

	// Find the nearest time slot to the initial time (if it's inside the time range).
	//
	// NOTE: This mirrors what we already did for crustal thickness factor (see constructor).
	boost::optional<unsigned int> initial_time_slot = time_range.get_nearest_time_slot(d_initial_time);
	if (initial_time_slot)
	{
		// Iterate over the time range going *backwards* in time from the initial time (most recent)
		// to the beginning of the time range (least recent).
		evolve_tectonic_subsidence_time_steps(
				lithospheric_temperature_integrated_over_depth_kms,
				initial_time_slot.get()/*start_time_slot*/,
				0/*end_time_slot*/,
				scalar_values_start_index,
				scalar_values_end_index);

		// Iterate over the time range going *forward* in time from the initial time (least recent)
		// to the end of the time range (most recent).
		evolve_tectonic_subsidence_time_steps(
				lithospheric_temperature_integrated_over_depth_kms,
				initial_time_slot.get()/*start_time_slot*/,
				num_time_slots - 1/*end_time_slot*/,
				scalar_values_start_index,
				scalar_values_end_index);
	}
	else if (d_initial_time > time_range.get_begin_time())
	{
		// The initial time is older than the beginning of the time range.
		// Since there's no deformation (evolution) of scalar values from the initial time to the
		// beginning of the time range, the scalars at the beginning of the time range are
		// the same as those at the initial time.

		// Iterate over the time range going *forward* in time from the beginning of the
		// time range (least recent) to the end (most recent).
		evolve_tectonic_subsidence_time_steps(
				lithospheric_temperature_integrated_over_depth_kms,
				0/*start_time_slot*/,
				num_time_slots - 1/*end_time_slot*/,
				scalar_values_start_index,
				scalar_values_end_index);
	}
	else // initial_time < time_range.get_end_time() ...
	{
		// The initial time is younger than the end of the time range.
		// Since there's no deformation (evolution) of scalar values from the end of the time range
		// to the initial time, the scalars at the end of the time range are
		// the same as those at the initial time.

		// Iterate over the time range going *backwards* in time from the end of the
		// time range (most recent) to the beginning (least recent).
		evolve_tectonic_subsidence_time_steps(
				lithospheric_temperature_integrated_over_depth_kms,
				num_time_slots - 1/*start_time_slot*/,
				0/*end_time_slot*/,
				scalar_values_start_index,
				scalar_values_end_index);
	}
}


void
GPlatesAppLogic::ScalarCoverageEvolution::evolve_tectonic_subsidence_time_steps(
		const double *const lithospheric_temperature_integrated_over_depth_kms,
		unsigned int start_time_slot,
		unsigned int end_time_slot,
		unsigned int scalar_values_start_index,
		unsigned int scalar_values_end_index) const
{
	if (start_time_slot == end_time_slot)
	{
		return;
	}

	const int time_slot_direction = (end_time_slot > start_time_slot) ? 1 : -1;

	// Get the initial scalar coverage in the initial time slot.
	boost::optional<EvolvedScalarCoverage::non_null_ptr_type &> initial_scalar_coverage =
			d_scalar_coverage_time_span->get_sample_in_time_slot(start_time_slot);
	// We should have a scalar coverage in the initial time slot.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			initial_scalar_coverage,
			GPLATES_ASSERTION_SOURCE);

	// Set the initial tectonic subsidence scalar values for *all* initial values (if haven't already), not
	// just the initial values in the current group [scalar_values_start_index, scalar_values_end_index].
	// This is only necessary the first time a group of scalar values is visited (not the second, third, etc).
	if (!initial_scalar_coverage.get()->state.tectonic_subsidence_kms)
	{
		if (const boost::optional<std::vector<double>> &initial_tectonic_subsidence_kms =
			d_initial_scalar_coverage.get_initial_scalar_values(TECTONIC_SUBSIDENCE_KMS))
		{
			// Copy the initial tectonic subsidence scalar values into our initial scalar coverage.
			initial_scalar_coverage.get()->state.tectonic_subsidence_kms = initial_tectonic_subsidence_kms.get();
		}
		else // have no initial tectonic subsidence...
		{
			// Set all initial tectonic subsidence to zero (sea level).
			//
			// Initialise the 'std::vector<double>' directly into the 'boost::optional<std::vector<double>>'.
			initial_scalar_coverage.get()->state.tectonic_subsidence_kms = boost::in_place(d_num_scalar_values, 0.0);
		}
	}

	EvolvedScalarCoverage::non_null_ptr_type current_scalar_coverage = initial_scalar_coverage.get();

	// Iterate over the time slots either backward or forward in time (depending on 'time_slot_direction').
	for (unsigned int time_slot = start_time_slot; time_slot != end_time_slot; time_slot += time_slot_direction)
	{
		const unsigned int current_time_slot = time_slot;
		const unsigned int next_time_slot = current_time_slot + time_slot_direction;

		// Get the next scalar coverage in the next time slot.
		boost::optional<EvolvedScalarCoverage::non_null_ptr_type &> next_scalar_coverage =
				d_scalar_coverage_time_span->get_sample_in_time_slot(next_time_slot);
		if (!next_scalar_coverage)
		{
			// Return early - the next time slot is not active - so the last active time slot is the current time slot.
			return;
		}

		// Allocate memory for *all* the next tectonic subsidence values (if haven't already), not just
		// the values in the current group [scalar_values_start_index, scalar_values_end_index].
		// This is only necessary the first time a group of scalar values is visited (not the second, third, etc).
		if (!next_scalar_coverage.get()->state.tectonic_subsidence_kms)
		{
			next_scalar_coverage.get()->state.tectonic_subsidence_kms = boost::in_place(d_num_scalar_values, 0.0/*arbitrary*/);
		}

		// Each lithospheric-temperature-integrated-over-depth array contains
		// 'scalar_values_end_index - scalar_values_start_index' values, which is equal to
		// NUM_POINTS_IN_TEMPERATURE_DIFFUSION_GROUP for all but the last group.
		const double *const current_lithospheric_temperature_integrated_over_depth_kms =
				lithospheric_temperature_integrated_over_depth_kms + current_time_slot * NUM_POINTS_IN_TEMPERATURE_DIFFUSION_GROUP;
		const double *const next_lithospheric_temperature_integrated_over_depth_kms =
				lithospheric_temperature_integrated_over_depth_kms + next_time_slot * NUM_POINTS_IN_TEMPERATURE_DIFFUSION_GROUP;

		// Evolve the current tectonic subsidence scalar values into the next time slot.
		evolve_tectonic_subsidence_time_step(
				current_scalar_coverage->state,
				next_scalar_coverage.get()->state,
				current_lithospheric_temperature_integrated_over_depth_kms,
				next_lithospheric_temperature_integrated_over_depth_kms,
				scalar_values_start_index,
				scalar_values_end_index);

		// Set the current scalar coverage for the next time step.
		current_scalar_coverage = next_scalar_coverage.get();
	}
}


void
GPlatesAppLogic::ScalarCoverageEvolution::evolve_tectonic_subsidence_time_step(
		const EvolvedScalarCoverage::State &current_scalar_coverage_state,
		EvolvedScalarCoverage::State &next_scalar_coverage_state,
		// Each lithospheric-temperature-integrated-over-depth array contains
		// 'scalar_values_end_index - scalar_values_start_index' values...
		const double *const current_lithospheric_temperature_integrated_over_depth_kms,
		const double *const next_lithospheric_temperature_integrated_over_depth_kms,
		unsigned int scalar_values_start_index,
		unsigned int scalar_values_end_index) const
{
	// Initial crustal thickness values (or null if not provided).
	const double *const initial_crustal_thickness_kms =
			d_initial_scalar_coverage.get_initial_scalar_values(CRUSTAL_THICKNESS_KMS)
			? d_initial_scalar_coverage.get_initial_scalar_values(CRUSTAL_THICKNESS_KMS)->data()
			: nullptr;

	const double *const current_tectonic_subsidence_kms = current_scalar_coverage_state.tectonic_subsidence_kms->data();
	double *const next_tectonic_subsidence_kms = next_scalar_coverage_state.tectonic_subsidence_kms->data();

	for (unsigned int scalar_value_index = scalar_values_start_index;
		scalar_value_index < scalar_values_end_index;
		++scalar_value_index)
	{
		// If next scalar value is inactive then we cannot evolve it.
		if (!next_scalar_coverage_state.scalar_values_are_active[scalar_value_index])
		{
			continue;
		}

		// The next scalar value is active and so must the current scalar value.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				current_scalar_coverage_state.scalar_values_are_active[scalar_value_index],
				GPLATES_ASSERTION_SOURCE);

		const unsigned int scalar_value_index_in_group = scalar_value_index - scalar_values_start_index;

		// Crustal thickness depends on initial values T(i) (or default values if not provided) and
		// the calculated crustal thickness *factor*:
		// 
		// T(n)    = [T(n)/T(i)] * T(i)
		//         = crustal_thickness_factor * T(i)
		//
		// crustal_thickness = initial_crustal_thickness * crustal_thickness_factor
		//
		const double current_crustal_thickness_kms = initial_crustal_thickness_kms
				? initial_crustal_thickness_kms[scalar_value_index] *
						current_scalar_coverage_state.crustal_thickness_factor[scalar_value_index]
				: DEFAULT_INITIAL_CRUSTAL_THICKNESS_KMS *
						current_scalar_coverage_state.crustal_thickness_factor[scalar_value_index];

		//
		// Calculate the difference in tectonic subsidence from current time to next time.
		//

		const double crustal_thickness_factor_current_to_next =
				next_scalar_coverage_state.crustal_thickness_factor[scalar_value_index] /
				current_scalar_coverage_state.crustal_thickness_factor[scalar_value_index];
		double delta_tectonic_subsidence_kms = (DENSITY_MANTLE - DENSITY_CRUST) * current_crustal_thickness_kms *
				(1.0 - crustal_thickness_factor_current_to_next);

		// Add in the subsidence contribution due to the change in lithospheric temperature
		// (and hence density) from current time to next time.
		delta_tectonic_subsidence_kms += -THERMAL_ALPHA * DENSITY_MANTLE * (
				next_lithospheric_temperature_integrated_over_depth_kms[scalar_value_index_in_group] -
				current_lithospheric_temperature_integrated_over_depth_kms[scalar_value_index_in_group]);

		// If currently below sea level then factor in the density of water.
		if (current_tectonic_subsidence_kms[scalar_value_index] >= 0)
		{
			delta_tectonic_subsidence_kms /= DENSITY_ASTHENOSPHERE - DENSITY_WATER;
		}
		else
		{
			delta_tectonic_subsidence_kms /= DENSITY_ASTHENOSPHERE;
		}

		// Update the tectonic subsidence.
		next_tectonic_subsidence_kms[scalar_value_index] =
				current_tectonic_subsidence_kms[scalar_value_index] + delta_tectonic_subsidence_kms;
	}
}


GPlatesAppLogic::ScalarCoverageEvolution::EvolvedScalarCoverage::non_null_ptr_type
GPlatesAppLogic::ScalarCoverageEvolution::create_time_span_rigid_sample(
		const double &reconstruction_time,
		const double &closest_younger_sample_time,
		const EvolvedScalarCoverage::non_null_ptr_type &closest_younger_sample)
{
	// Simply return the closest younger sample.
	// We are in a rigid region so the scalar values have not changed since deformation (closest younger sample).
	return closest_younger_sample;
}


GPlatesAppLogic::ScalarCoverageEvolution::EvolvedScalarCoverage::non_null_ptr_type
GPlatesAppLogic::ScalarCoverageEvolution::interpolate_time_span_samples(
		const double &interpolate_position,
		const double &first_geometry_time,
		const double &second_geometry_time,
		const EvolvedScalarCoverage::non_null_ptr_type &first_sample,
		const EvolvedScalarCoverage::non_null_ptr_type &second_sample)
{
	const double reconstruction_time =
			(1 - interpolate_position) * first_geometry_time +
				interpolate_position * second_geometry_time;

	// NOTE: Mirror what the domain geometry time span does so that we end up with the same number of
	// *active* scalar values as *active* geometry points. If we don't get the same number then
	// later on we'll get an assertion failure.
	//
	// Determine whether to reconstruct backward or forward in time when interpolating.
	if (reconstruction_time > d_initial_time)
	{
		// Reconstruct backward in time away from the initial time.
		// For now we'll just pick the nearest sample (to the initial time).
		return second_sample;
	}
	else
	{
		// Reconstruct forward in time away from the initial time.
		// For now we'll just pick the nearest sample (to the initial time).
		return first_sample;
	}
}


void
GPlatesAppLogic::ScalarCoverageEvolution::get_scalar_values(
		EvolvedScalarType evolved_scalar_type,
		const double &reconstruction_time,
		std::vector<double> &scalar_values,
		std::vector<bool> &scalar_values_are_active) const
{
	// If the caller requested tectonic subsidence then initialise it (if haven't already).
	if (evolved_scalar_type == TECTONIC_SUBSIDENCE_KMS)
	{
		if (!d_have_initialised_tectonic_subsidence)
		{
			initialise_tectonic_subsidence();
			d_have_initialised_tectonic_subsidence = true;
		}
	}

	// Get the scalar coverage at the requested reconstruction time.
	EvolvedScalarCoverage::non_null_ptr_type scalar_coverage =
			d_scalar_coverage_time_span->get_or_create_sample(reconstruction_time);
	const EvolvedScalarCoverage::State &scalar_coverage_state = scalar_coverage->state;

	// Copy the active status of each scalar value.
	scalar_values_are_active = scalar_coverage_state.scalar_values_are_active;

	switch (evolved_scalar_type)
	{
	case CRUSTAL_THICKNESS_KMS:
	{
		// Crustal thickness depends on initial values T(i) (or default values if not provided) and
		// the calculated crustal thickness *factor*:
		// 
		// T(n)    = [T(n)/T(i)] * T(i)
		//         = crustal_thickness_factor * T(i)
		//
		// crustal_thickness = initial_crustal_thickness * crustal_thickness_factor
		//
		if (const boost::optional<std::vector<double>> &initial_crustal_thickness_kms_opt =
			d_initial_scalar_coverage.get_initial_scalar_values(CRUSTAL_THICKNESS_KMS))
		{
			// Initial values were provided for crustal thickness.
			const std::vector<double> &initial_crustal_thickness_kms = initial_crustal_thickness_kms_opt.get();

			scalar_values.reserve(d_num_scalar_values);
			for (unsigned int n = 0; n < d_num_scalar_values; ++n)
			{
				// crustal_thickness = initial_crustal_thickness * crustal_thickness_factor
				scalar_values.push_back(
						initial_crustal_thickness_kms[n] * scalar_coverage_state.crustal_thickness_factor[n]);
			}
		}
		else
		{
			// No initial values were provided so just T(i) = DEFAULT_INITIAL_CRUSTAL_THICKNESS_KMS.
			scalar_values.reserve(d_num_scalar_values);
			for (unsigned int n = 0; n < d_num_scalar_values; ++n)
			{
				// crustal_thickness = initial_crustal_thickness * crustal_thickness_factor
				//                   = DEFAULT_INITIAL_CRUSTAL_THICKNESS_KMS * crustal_thickness_factor
				scalar_values.push_back(
						DEFAULT_INITIAL_CRUSTAL_THICKNESS_KMS * scalar_coverage_state.crustal_thickness_factor[n]);
			}
		}
	}
	break;

	case CRUSTAL_STRETCHING_FACTOR:
	{
		// Crustal stretching factor depends on initial values (or default values if not provided) and
		// the calculated crustal thickness *factor*:
		// 
		// beta(i,j) = T(j)/T(i)  // This is the stretching factor at initial time t=i relative to some other time t=j
		// beta(n,j) = T(j)/T(n)  // This is the stretching factor at current time t=n relative to some other time t=j
		//           = T(j)/T(i) * T(i)/T(n)
		//           = beta(i,j) * T(i)/T(n)
		//           = beta(i,j) * (1/crustal_thickness_factor)
		//           = beta(i,j) / crustal_thickness_factor
		//
		// beta = initial_beta / crustal_thickness_factor
		//
		if (const boost::optional<std::vector<double>> &initial_crustal_stretching_factor_opt =
			d_initial_scalar_coverage.get_initial_scalar_values(CRUSTAL_STRETCHING_FACTOR))
		{
			// Initial values were provided for crustal stretching factor.
			const std::vector<double> &initial_crustal_stretching_factor = initial_crustal_stretching_factor_opt.get();

			scalar_values.reserve(d_num_scalar_values);
			for (unsigned int n = 0; n < d_num_scalar_values; ++n)
			{
				// beta = initial_beta / crustal_thickness_factor
				scalar_values.push_back(
						initial_crustal_stretching_factor[n] / scalar_coverage_state.crustal_thickness_factor[n]);
			}
		}
		else
		{
			// No initial values were provided so assume initial_beta = 1.0.
			scalar_values.reserve(d_num_scalar_values);
			for (unsigned int n = 0; n < d_num_scalar_values; ++n)
			{
				// beta = initial_beta / crustal_thickness_factor
				//      = 1 / crustal_thickness_factor
				scalar_values.push_back(
						1.0 / scalar_coverage_state.crustal_thickness_factor[n]);
			}
		}
	}
	break;

	case CRUSTAL_THINNING_FACTOR:
	{
		// Crustal thinning factor depends on initial values (or default values if not provided) and
		// the calculated crustal thickness *factor*:
		// 
		// gamma(i,j) = 1 - T(i)/T(j)  // This is the thinning factor at initial time t=i relative to some other time t=j
		// gamma(n,j) = 1 - T(n)/T(j)  // This is the thinning factor at current time t=n relative to some other time t=j
		//            = 1 - T(n)/T(i) * T(i)/T(j)
		//            = 1 - T(n)/T(i) * (1 - gamma(i,j))
		//            = 1 - crustal_thickness_factor * (1 - gamma(i,j))
		//            = 1 - crustal_thickness_factor * (1 - initial_gamma)
		//
		// gamma = 1 - (1 - initial_gamma) * crustal_thickness_factor
		//
		if (const boost::optional<std::vector<double>> &initial_crustal_thinning_factor_opt =
			d_initial_scalar_coverage.get_initial_scalar_values(CRUSTAL_THINNING_FACTOR))
		{
			// Initial values were provided for crustal thinning factor.
			const std::vector<double> &initial_crustal_thinning_factor = initial_crustal_thinning_factor_opt.get();

			scalar_values.reserve(d_num_scalar_values);
			for (unsigned int n = 0; n < d_num_scalar_values; ++n)
			{
				// gamma = 1 - (1 - initial_gamma) * crustal_thickness_factor
				scalar_values.push_back(
						1.0 - (1.0 - initial_crustal_thinning_factor[n]) * scalar_coverage_state.crustal_thickness_factor[n]);
			}
		}
		else
		{
			// No initial values were provided so assume initial_gamma = 0.0.
			scalar_values.reserve(d_num_scalar_values);
			for (unsigned int n = 0; n < d_num_scalar_values; ++n)
			{
				// gamma = 1 - (1 - initial_gamma) * crustal_thickness_factor
				//       = 1 - (1 - 0) * crustal_thickness_factor
				//       = 1 - crustal_thickness_factor
				scalar_values.push_back(
						1.0 - scalar_coverage_state.crustal_thickness_factor[n]);
			}
		}
	}
	break;

	case TECTONIC_SUBSIDENCE_KMS:
	{
		// Tectonic subsidence should be initialised.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				scalar_coverage_state.tectonic_subsidence_kms,
				GPLATES_ASSERTION_SOURCE);

		scalar_values = scalar_coverage_state.tectonic_subsidence_kms.get();
	}
	break;

	default:
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
	}
}


void
GPlatesAppLogic::ScalarCoverageEvolution::InitialEvolvedScalarCoverage::add_initial_scalar_values(
		EvolvedScalarType evolved_scalar_type,
		const std::vector<double> &initial_scalar_values)
{
	// All scalar types should have the same number of scalar values.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			initial_scalar_values.size() == d_num_scalar_values,
			GPLATES_ASSERTION_SOURCE);

	// Copy the 'std::vector<double>' directly into the 'boost::optional<std::vector<double>>'.
	d_initial_scalar_values[evolved_scalar_type] = boost::in_place(
			initial_scalar_values.begin(),
			initial_scalar_values.end());
}


GPlatesAppLogic::ScalarCoverageEvolution::EvolvedScalarCoverage::State::State(
		unsigned int num_scalar_values) :
	// Initially all scalar values are active...
	scalar_values_are_active(num_scalar_values, true),
	// Initially the "crustal thickness divided by the initial crustal thickness" is 1.0...
	crustal_thickness_factor(num_scalar_values, 1.0)
{
}
