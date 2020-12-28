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
#include <cmath>
#include <boost/bind.hpp>
#include <boost/utility/in_place_factory.hpp>

#include "ScalarCoverageEvolution.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


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
	case CRUSTAL_THICKNESS:
		return GPML_CRUSTAL_THICKNESS;

	case CRUSTAL_STRETCHING_FACTOR:
		return GPML_CRUSTAL_STRETCHING_FACTOR;

	case CRUSTAL_THINNING_FACTOR:
		return GPML_CRUSTAL_THINNING_FACTOR;

	case TECTONIC_SUBSIDENCE:
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
		return CRUSTAL_THICKNESS;
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
		return TECTONIC_SUBSIDENCE;
	}

	return boost::none;
}


GPlatesAppLogic::ScalarCoverageEvolution::ScalarCoverageEvolution(
		const InitialEvolvedScalarCoverage &initial_scalar_coverage,
		const double &initial_time,
		TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span) :
	d_geometry_time_span(geometry_time_span),
	d_num_scalar_values(initial_scalar_coverage.get_num_scalar_values()),
	d_initial_scalar_coverage(initial_scalar_coverage),
	d_initial_time(initial_time),
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
	d_have_evolved_tectonic_subsidence(false)
{
	// The scalar coverage at the import time was stored in the present day sample.
	EvolvedScalarCoverage::non_null_ptr_type import_scalar_coverage =
			d_scalar_coverage_time_span->get_present_day_sample();

	const TimeSpanUtils::TimeRange time_range = d_scalar_coverage_time_span->get_time_range();
	const unsigned int num_time_slots = time_range.get_num_time_slots();

	// Find the nearest time slot to the initial time (if it's inside the time range).
	//
	// NOTE: This mirrors what is done with the domain geometry associated with the scalar coverage.
	boost::optional<unsigned int> initial_time_slot = time_range.get_nearest_time_slot(initial_time);
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
	else if (initial_time > time_range.get_begin_time())
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
			boost::none/*point*/,
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

		const bool scalar_values_sample_active =
				d_geometry_time_span->get_all_geometry_data(next_time, boost::none, boost::none, next_domain_strain_rates);
		if (!scalar_values_sample_active)
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

	for (unsigned int n = 0; n < d_num_scalar_values; ++n)
	{
		// If current scalar value is inactive then it remains inactive.
		if (!current_scalar_coverage_state.scalar_values_are_active[n])
		{
			// If the current scalar value is inactive then so must the current (and next) dilatation strain rates.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					!current_deformation_strain_rates[n] &&
						!next_deformation_strain_rates[n],
					GPLATES_ASSERTION_SOURCE);
			continue;
		}

		// The current scalar value is active so the current dilatation strain rate must also be active.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				current_deformation_strain_rates[n],
				GPLATES_ASSERTION_SOURCE);

		// If the next strain rate is inactive then the scalar value becomes inactive (for the next time step).
		if (!next_deformation_strain_rates[n])
		{
			// The current scalar value is no longer active (for the next time step).
			current_scalar_coverage_state.scalar_values_are_active[n] = false;

			continue;
		}

		double current_dilatation_per_my = seconds_in_a_million_years *
				current_deformation_strain_rates[n]->get_strain_rate_dilatation();
		double next_dilatation_per_my = seconds_in_a_million_years *
				next_deformation_strain_rates[n]->get_strain_rate_dilatation();

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
		//

		// Clamp dilatation to 1.0 in units of 1/Myr, which is equivalent to 3.17e-14 in units of 1/second.
		// This is about 6 times the default clamping (disabled by default) of 5e-15 1/second in a
		// topological network visual layer, and so the user still has the option to clamp further than this.
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
		//   H(n+1) = m * H(n)
		//   H(n)   = H(n+1) / m
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
		current_scalar_coverage_state.crustal_thickness_factor[n] *= crustal_thickness_multiplier;
	}
}


void
GPlatesAppLogic::ScalarCoverageEvolution::evolve_tectonic_subsidence() const
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
				initial_time_slot.get()/*start_time_slot*/,
				0/*end_time_slot*/);

		// Iterate over the time range going *forward* in time from the initial time (least recent)
		// to the end of the time range (most recent).
		evolve_tectonic_subsidence_time_steps(
				initial_time_slot.get()/*start_time_slot*/,
				num_time_slots - 1/*end_time_slot*/);
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
				0/*start_time_slot*/,
				num_time_slots - 1/*end_time_slot*/);
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
				num_time_slots - 1/*start_time_slot*/,
				0/*end_time_slot*/);
	}
}


void
GPlatesAppLogic::ScalarCoverageEvolution::evolve_tectonic_subsidence_time_steps(
		unsigned int start_time_slot,
		unsigned int end_time_slot) const
{
	if (start_time_slot == end_time_slot)
	{
		return;
	}

	const int time_slot_direction = (end_time_slot > start_time_slot) ? 1 : -1;

	// Get the initial scalar coverage in the initial time slot.
	boost::optional<EvolvedScalarCoverage::non_null_ptr_type &> current_scalar_coverage_opt =
			d_scalar_coverage_time_span->get_sample_in_time_slot(start_time_slot);
	// We should have a scalar coverage in the initial time slot.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			current_scalar_coverage_opt,
			GPLATES_ASSERTION_SOURCE);
	EvolvedScalarCoverage::non_null_ptr_type current_scalar_coverage = current_scalar_coverage_opt.get();

	// Set the initial tectonic subsidence scalar values (that we'll subsequently evolve).
	if (const boost::optional<std::vector<double>> &initial_tectonic_subsidence =
		d_initial_scalar_coverage.get_initial_scalar_values(TECTONIC_SUBSIDENCE))
	{
		// Copy the initial tectonic subsidence scalar values into our initial scalar coverage.
		current_scalar_coverage->state.tectonic_subsidence = initial_tectonic_subsidence.get();
	}
	else // have no initial tectonic subsidence...
	{
		// Set all initial tectonic subsidence to zero (sea level).
		//
		// Initialise the 'std::vector<double>' directly into the 'boost::optional<std::vector<double>>'.
		current_scalar_coverage->state.tectonic_subsidence = boost::in_place(d_num_scalar_values, 0.0);
	}

	// Iterate over the time slots either backward or forward in time (depending on 'time_slot_direction').
	for (unsigned int time_slot = start_time_slot; time_slot != end_time_slot; time_slot += time_slot_direction)
	{
		const unsigned int current_time_slot = time_slot;
		const unsigned int next_time_slot = current_time_slot + time_slot_direction;

		// Get the next scalar coverage in the next time slot.
		boost::optional<EvolvedScalarCoverage::non_null_ptr_type &> next_scalar_coverage_opt =
				d_scalar_coverage_time_span->get_sample_in_time_slot(next_time_slot);
		if (!next_scalar_coverage_opt)
		{
			// Return early - the next time slot is not active - so the last active time slot is the current time slot.
			return;
		}
		EvolvedScalarCoverage::non_null_ptr_type next_scalar_coverage = next_scalar_coverage_opt.get();

		// Evolve the current tectonic subsidence scalar values into the next time slot.
		evolve_tectonic_subsidence_time_step(
				current_scalar_coverage->state,
				next_scalar_coverage->state);

		// Set the current scalar coverage for the next time step.
		current_scalar_coverage = next_scalar_coverage;
	}
}


void
GPlatesAppLogic::ScalarCoverageEvolution::evolve_tectonic_subsidence_time_step(
		const EvolvedScalarCoverage::State &current_scalar_coverage_state,
		EvolvedScalarCoverage::State &next_scalar_coverage_state) const
{
	// Copy the current tectonic subsidence scalar values into the next scalar values.
	//
	// Then later below we'll just add in the difference in tectonic subsidence from current to next.
	next_scalar_coverage_state.tectonic_subsidence = current_scalar_coverage_state.tectonic_subsidence.get();

	for (unsigned int n = 0; n < d_num_scalar_values; ++n)
	{
		// If next scalar value is inactive then we cannot evolve it.
		if (!next_scalar_coverage_state.scalar_values_are_active[n])
		{
			continue;
		}

		// The next scalar value is active and so must the current scalar value.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				current_scalar_coverage_state.scalar_values_are_active[n],
				GPLATES_ASSERTION_SOURCE);

		// Crustal thickness depends on initial values T(i) (or default values if not provided) and
		// the calculated crustal thickness *factor*:
		// 
		// T(n)    = [T(n)/T(i)] * T(i)
		//         = crustal_thickness_factor * T(i)
		//
		// crustal_thickness = initial_crustal_thickness * crustal_thickness_factor
		//
		double current_crustal_thickness;
		if (const boost::optional<std::vector<double>> &initial_crustal_thickness =
			d_initial_scalar_coverage.get_initial_scalar_values(CRUSTAL_THICKNESS))
		{
			// crustal_thickness = initial_crustal_thickness * crustal_thickness_factor
			current_crustal_thickness = initial_crustal_thickness.get()[n] *
					current_scalar_coverage_state.crustal_thickness_factor[n];
		}
		else
		{
			// crustal_thickness = initial_crustal_thickness * crustal_thickness_factor
			//                   = DEFAULT_INITIAL_CRUSTAL_THICKNESS_KMS * crustal_thickness_factor
			current_crustal_thickness = DEFAULT_INITIAL_CRUSTAL_THICKNESS_KMS *
					current_scalar_coverage_state.crustal_thickness_factor[n];
		}

		//
		// Calculate the difference in tectonic subsidence from current time to next time.
		//
		const double crustal_thickness_factor_current_to_next =
				next_scalar_coverage_state.crustal_thickness_factor[n] /
				current_scalar_coverage_state.crustal_thickness_factor[n];
		double delta_tectonic_subsidence = (DENSITY_MANTLE - DENSITY_CRUST) * current_crustal_thickness *
				(1.0 - crustal_thickness_factor_current_to_next);
		// If currently below sea level then factor in the density of water.
		if (current_scalar_coverage_state.tectonic_subsidence.get()[n] >= 0)
		{
			delta_tectonic_subsidence /= DENSITY_ASTHENOSPHERE - DENSITY_WATER;
		}
		else
		{
			delta_tectonic_subsidence /= DENSITY_ASTHENOSPHERE;
		}

		// Update the tectonic subsidence.
		next_scalar_coverage_state.tectonic_subsidence.get()[n] += delta_tectonic_subsidence;
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
	// If the caller requested tectonic subsidence then evolve it (if haven't already).
	if (evolved_scalar_type == TECTONIC_SUBSIDENCE)
	{
		if (!d_have_evolved_tectonic_subsidence)
		{
			evolve_tectonic_subsidence();
			d_have_evolved_tectonic_subsidence = true;
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
	case CRUSTAL_THICKNESS:
	{
		// Crustal thickness depends on initial values T(i) (or default values if not provided) and
		// the calculated crustal thickness *factor*:
		// 
		// T(n)    = [T(n)/T(i)] * T(i)
		//         = crustal_thickness_factor * T(i)
		//
		// crustal_thickness = initial_crustal_thickness * crustal_thickness_factor
		//
		if (const boost::optional<std::vector<double>> &initial_crustal_thickness_opt =
			d_initial_scalar_coverage.get_initial_scalar_values(CRUSTAL_THICKNESS))
		{
			// Initial values were provided for crustal thickness.
			const std::vector<double> &initial_crustal_thickness = initial_crustal_thickness_opt.get();

			scalar_values.reserve(d_num_scalar_values);
			for (unsigned int n = 0; n < d_num_scalar_values; ++n)
			{
				// crustal_thickness = initial_crustal_thickness * crustal_thickness_factor
				scalar_values.push_back(
						initial_crustal_thickness[n] * scalar_coverage_state.crustal_thickness_factor[n]);
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

	case TECTONIC_SUBSIDENCE:
	{
		// Tectonic subsidence should be initialised.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				scalar_coverage_state.tectonic_subsidence,
				GPLATES_ASSERTION_SOURCE);

		scalar_values = scalar_coverage_state.tectonic_subsidence.get();
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
