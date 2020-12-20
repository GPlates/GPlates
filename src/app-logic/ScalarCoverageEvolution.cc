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

#include "ScalarCoverageEvolution.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * Strain rates are in 1/sec (multiplying by this number converts to 1/My).
		 */
		const double SECONDS_IN_A_MILLION_YEARS = 365.25 * 24 * 3600 * 1.0e6;
	}
}


boost::optional<GPlatesAppLogic::scalar_evolution_function_type>
GPlatesAppLogic::get_scalar_evolution_function(
		const GPlatesPropertyValues::ValueObjectType &scalar_type,
		const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params)
{
	static const GPlatesPropertyValues::ValueObjectType CRUSTAL_THICKNESS_TYPE =
			GPlatesPropertyValues::ValueObjectType::create_gpml("CrustalThickness");
	static const GPlatesPropertyValues::ValueObjectType CRUSTAL_STRETCHING_FACTOR_TYPE =
			GPlatesPropertyValues::ValueObjectType::create_gpml("CrustalStretchingFactor");
	static const GPlatesPropertyValues::ValueObjectType CRUSTAL_THINNING_FACTOR_TYPE =
			GPlatesPropertyValues::ValueObjectType::create_gpml("CrustalThinningFactor");

	if (scalar_type == CRUSTAL_THICKNESS_TYPE)
	{
		return scalar_evolution_function_type(
				boost::bind(&ScalarCoverageEvolution::crustal_thinning, _1, _2, _3, _4, _5,
						ScalarCoverageEvolution::CRUSTAL_THICKNESS));
	}
	else if (scalar_type == CRUSTAL_STRETCHING_FACTOR_TYPE)
	{
		return scalar_evolution_function_type(
				boost::bind(&ScalarCoverageEvolution::crustal_thinning, _1, _2, _3, _4, _5,
						ScalarCoverageEvolution::CRUSTAL_STRETCHING_FACTOR));
	}
	else if (scalar_type == CRUSTAL_THINNING_FACTOR_TYPE)
	{
		return scalar_evolution_function_type(
				boost::bind(&ScalarCoverageEvolution::crustal_thinning, _1, _2, _3, _4, _5,
						ScalarCoverageEvolution::CRUSTAL_THINNING_FACTOR));
	}

	// No evolution needed - scalar values don't change over time.
	return boost::none;
}


void
GPlatesAppLogic::ScalarCoverageEvolution::crustal_thinning(
		std::vector< boost::optional<double> > &input_output_crustal_thickness,
		const std::vector< boost::optional<DeformationStrainRate> > &initial_deformation_strain_rates,
		const std::vector< boost::optional<DeformationStrainRate> > &final_deformation_strain_rates,
		const double &initial_time,
		const double &final_time,
		CrustalThicknessType crustal_thickness_type)
{
	// Input array sizes should match.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			input_output_crustal_thickness.size() == initial_deformation_strain_rates.size() &&
				initial_deformation_strain_rates.size() == final_deformation_strain_rates.size(),
			GPLATES_ASSERTION_SOURCE);

	if (input_output_crustal_thickness.empty())
	{
		// If empty then avoid getting pointers to zeroth array elements (which would crash).
		return;
	}

	double time_increment = final_time - initial_time;
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

	const unsigned int num_crustal_thicknesses = input_output_crustal_thickness.size();

	// Convert from crustal thickness related scalar to crustal thickness (if necessary).
	switch (crustal_thickness_type)
	{
	case CRUSTAL_STRETCHING_FACTOR:
		for (unsigned int n = 0; n < num_crustal_thicknesses; ++n)
		{
			boost::optional<double> &crustal_thickness = input_output_crustal_thickness[n];
			if (crustal_thickness)
			{
				// Convert from 'beta = Ti/T' to the thickness ratio 'T/Ti'.
				crustal_thickness.get() = 1.0 / crustal_thickness.get();
			}
		}
		break;

	case CRUSTAL_THINNING_FACTOR:
		for (unsigned int n = 0; n < num_crustal_thicknesses; ++n)
		{
			boost::optional<double> &crustal_thickness = input_output_crustal_thickness[n];
			if (crustal_thickness)
			{
				// Convert from 'gamma = (1 - T/Ti)' to the thickness ratio 'T/Ti'.
				crustal_thickness.get() = 1.0 - crustal_thickness.get();
			}
		}
		break;

	case CRUSTAL_THICKNESS:
	default:
		// Values are already thickness (or thickness ratio) so no change needed.
		break;
	}

	for (unsigned int n = 0; n < num_crustal_thicknesses; ++n)
	{
		// If initial scalar value is inactive then it remains inactive.
		if (!input_output_crustal_thickness[n])
		{
			continue;
		}

		// If the initial strain rate is inactive then so should the final strain rate.
		// Actually the active state of initial strain rate should match the initial scalar value.
		// And if the final strain rate is inactive then the scalar value becomes inactive.
		// However we test active state of both (initial and final) strain rates just in case.
		if (!final_deformation_strain_rates[n] ||
			!initial_deformation_strain_rates[n])
		{
			input_output_crustal_thickness[n] = boost::none;
			continue;
		}

		double initial_dilatation_per_my = SECONDS_IN_A_MILLION_YEARS *
				initial_deformation_strain_rates[n]->get_strain_rate_dilatation();
		double final_dilatation_per_my = SECONDS_IN_A_MILLION_YEARS *
				final_deformation_strain_rates[n]->get_strain_rate_dilatation();

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

		input_output_crustal_thickness[n].get() *= crustal_thickness_multiplier;
	}

	// Convert from crustal thickness back to crustal thickness related scalar (if necessary).
	switch (crustal_thickness_type)
	{
	case CRUSTAL_STRETCHING_FACTOR:
		for (unsigned int n = 0; n < num_crustal_thicknesses; ++n)
		{
			boost::optional<double> &crustal_thickness = input_output_crustal_thickness[n];
			if (crustal_thickness)
			{
				// Convert from the thickness ratio 'T/Ti' back to 'beta = Ti/T'.
				crustal_thickness.get() = 1.0 / crustal_thickness.get();
			}
		}
		break;

	case CRUSTAL_THINNING_FACTOR:
		for (unsigned int n = 0; n < num_crustal_thicknesses; ++n)
		{
			boost::optional<double> &crustal_thickness = input_output_crustal_thickness[n];
			if (crustal_thickness)
			{
				// Convert from the thickness ratio 'T/Ti' back to 'gamma = (1 - T/Ti)'.
				crustal_thickness.get() = 1.0 - crustal_thickness.get();
			}
		}
		break;

	case CRUSTAL_THICKNESS:
	default:
		// Values are already thickness (or thickness ratio) so no change needed.
		break;
	}
}
