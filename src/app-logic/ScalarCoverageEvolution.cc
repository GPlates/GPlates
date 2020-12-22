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

#include "ScalarCoverageEvolution.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


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

	switch (evolved_scalar_type)
	{
	case CRUSTAL_THICKNESS:
		return GPML_CRUSTAL_THICKNESS;

	case CRUSTAL_STRETCHING_FACTOR:
		return GPML_CRUSTAL_STRETCHING_FACTOR;

	case CRUSTAL_THINNING_FACTOR:
		return GPML_CRUSTAL_THINNING_FACTOR;

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

	return boost::none;
}


void
GPlatesAppLogic::ScalarCoverageEvolution::evolve(
		const std::vector< boost::optional<DeformationStrainRate> > &current_deformation_strain_rates,
		const std::vector< boost::optional<DeformationStrainRate> > &evolve_deformation_strain_rates,
		const double &evolve_time)
{
	const unsigned int num_scalar_values = get_num_scalar_values();

	// Input array sizes should match.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			num_scalar_values == current_deformation_strain_rates.size() &&
				current_deformation_strain_rates.size() == evolve_deformation_strain_rates.size(),
			GPLATES_ASSERTION_SOURCE);

	double time_increment = evolve_time - d_current_time;
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

	for (unsigned int n = 0; n < num_scalar_values; ++n)
	{
		// If initial scalar value is inactive then it remains inactive.
		if (!d_current_evolved_scalar_coverage.d_evolved_scalar_values[CRUSTAL_THICKNESS][n])
		{
			continue;
		}

		// If the initial strain rate is inactive then so should the final strain rate.
		// Actually the active state of initial strain rate should match the initial scalar value.
		// And if the final strain rate is inactive then the scalar value becomes inactive.
		if (!evolve_deformation_strain_rates[n])
		{
			for (unsigned int scalar_type = 0; scalar_type < NUM_EVOLVED_SCALAR_TYPES; ++scalar_type)
			{
				d_current_evolved_scalar_coverage.d_evolved_scalar_values[scalar_type][n] = boost::none;
			}

			continue;
		}

		double initial_dilatation_per_my = seconds_in_a_million_years *
				current_deformation_strain_rates[n]->get_strain_rate_dilatation();
		double final_dilatation_per_my = seconds_in_a_million_years *
				evolve_deformation_strain_rates[n]->get_strain_rate_dilatation();

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

		if (initial_dilatation_per_my > 1)
		{
			initial_dilatation_per_my = 1;
		}
		else if (initial_dilatation_per_my < -1)
		{
			initial_dilatation_per_my = -1;
		}

		if (final_dilatation_per_my > 1)
		{
			final_dilatation_per_my = 1;
		}
		else if (final_dilatation_per_my < -1)
		{
			final_dilatation_per_my = -1;
		}

		const double average_dilatation_per_my = 0.5 * (initial_dilatation_per_my + final_dilatation_per_my);

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

		double &crustal_thickness = d_current_evolved_scalar_coverage.d_evolved_scalar_values[CRUSTAL_THICKNESS][n].get();
		crustal_thickness *= crustal_thickness_multiplier;

		// beta(n+1) = T(0)/T(n+1)
		//           = 1 / (T(n+1)/T(0))
		//           = 1 / (m*T(n)/T(0))
		//           = 1 / (m / beta(n))
		//           = beta(n) / m
		double &crustal_stretching_factor = d_current_evolved_scalar_coverage.d_evolved_scalar_values[CRUSTAL_STRETCHING_FACTOR][n].get();
		crustal_stretching_factor /= crustal_thickness_multiplier;

		// gamma(n+1) = 1 - T(n+1)/T(0)
		//            = 1 - m*T(n)/T(0)
		//            = 1 - m * (1 - gamma(n))
		double &crustal_thinning_factor = d_current_evolved_scalar_coverage.d_evolved_scalar_values[CRUSTAL_THINNING_FACTOR][n].get();
		crustal_thinning_factor = 1 - crustal_thickness_multiplier * (1 - crustal_thinning_factor);
	}

	// Update the current time.
	d_current_time = evolve_time;
}


void
GPlatesAppLogic::ScalarCoverageEvolution::InitialEvolvedScalarCoverage::add_scalar_values(
		EvolvedScalarType evolved_scalar_type,
		const std::vector<double> &initial_scalar_values)
{
	// All scalar types should have the same number of scalar values.
	if (d_num_scalar_values)
	{
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				initial_scalar_values.size() == d_num_scalar_values.get(),
				GPLATES_ASSERTION_SOURCE);
	}
	else
	{
		d_num_scalar_values = initial_scalar_values.size();
	}

	d_initial_evolved_scalar_values[evolved_scalar_type] = initial_scalar_values;
}


GPlatesAppLogic::ScalarCoverageEvolution::EvolvedScalarCoverage::EvolvedScalarCoverage(
		unsigned int num_scalar_values)
{
	// Use default initial scalar values except for crustal thickness (leave as boost::none).
	// Crustal stretching and thinning factors are ratios so we can assume initial values for those.
	d_evolved_scalar_values[CRUSTAL_THICKNESS].resize(num_scalar_values, 1.0);
	d_evolved_scalar_values[CRUSTAL_STRETCHING_FACTOR].resize(num_scalar_values, 1.0);  // Ti/T
	d_evolved_scalar_values[CRUSTAL_THINNING_FACTOR].resize(num_scalar_values, 0.0);  // (1 - T/Ti)
}


GPlatesAppLogic::ScalarCoverageEvolution::EvolvedScalarCoverage::EvolvedScalarCoverage(
		const InitialEvolvedScalarCoverage &initial_evolved_scalar_coverage) :
	// Initialise with default values first (in case initial scalar values not provided for all evolved scalar types)...
	EvolvedScalarCoverage(initial_evolved_scalar_coverage.get_num_scalar_values())
{
	// For each evolved scalar type copy its initial scalar values into our current evolved scalar coverage.
	for (const auto &scalar_coverage_item : initial_evolved_scalar_coverage.get_scalar_values())
	{
		const EvolvedScalarType evolved_scalar_type = scalar_coverage_item.first;
		const std::vector<double> &initial_scalar_values = scalar_coverage_item.second;

		// Copy the 'double' scalar values into the 'boost::optional<double>' elements.
		d_evolved_scalar_values[evolved_scalar_type].assign(
				initial_scalar_values.begin(),
				initial_scalar_values.end());
	}
}
