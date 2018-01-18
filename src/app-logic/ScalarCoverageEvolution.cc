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
		const double seconds_in_a_million_years = 365.25 * 24 * 3600 * 1.0e6;


		/**
		 * Derivative function for crustal thinning.
		 */
		double 
		crustal_thinning_derivative(
				const double &time,
				const double &crustal_thickness,
				const double &dilitation)
		{
			return -crustal_thickness * dilitation;
		}


		/** 
		 * Runge Kutta order 2 integration.
		 */
		template <typename DerivativeFunctionType>
		void
		runge_kutta_order_2(
				boost::optional<double> *const values,
				const unsigned int num_values,
				const DerivativeFunctionType &derivative_function,
				const double &derivative_sign,
				const double &initial_time,
				const double &time_increment,
				const boost::optional<DeformationStrainRate> *const initial_deformation_strain_rates,
				const boost::optional<DeformationStrainRate> *const final_deformation_strain_rates)
		{
			for (unsigned int n = 0; n < num_values; ++n)
			{
				// If initial scalar value is inactive then it remains inactive.
				if (!values[n])
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
					values[n] = boost::none;
					continue;
				}

				const double initial_dilatation = initial_deformation_strain_rates[n]->get_strain_rate_dilatation();
				const double final_dilatation = final_deformation_strain_rates[n]->get_strain_rate_dilatation();

				const double K0 = time_increment * derivative_sign * derivative_function(
						initial_time,
						values[n].get(),
						seconds_in_a_million_years * initial_dilatation);

				const double average_time = initial_time + 0.5 * time_increment;

				const double average_dilatation_per_my =
						seconds_in_a_million_years * 0.5 * (initial_dilatation + final_dilatation);

				const double K1 = time_increment * derivative_sign * derivative_function(
						average_time,
						values[n].get() + 0.5 * K0,
						average_dilatation_per_my);
   
				values[n].get() += K1;
			}
		}


		/** 
		 * Runge Kutta order 4 integration.
		 */
		template <typename DerivativeFunctionType>
		void
		runge_kutta_order_4(
				boost::optional<double> *const values,
				const unsigned int num_values,
				const DerivativeFunctionType &derivative_function,
				const double &derivative_sign,
				const double &initial_time,
				const double &time_increment,
				const boost::optional<DeformationStrainRate> *const initial_deformation_strain_rates,
				const boost::optional<DeformationStrainRate> *const final_deformation_strain_rates)
		{
			for (unsigned int n = 0; n < num_values; ++n)
			{
				// If initial scalar value is inactive then it remains inactive.
				if (!values[n])
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
					values[n] = boost::none;
					continue;
				}

				const double initial_dilatation = initial_deformation_strain_rates[n]->get_strain_rate_dilatation();
				const double final_dilatation = final_deformation_strain_rates[n]->get_strain_rate_dilatation();

				const double K0 = time_increment * derivative_sign * derivative_function(
						initial_time,
						values[n].get(),
						seconds_in_a_million_years * initial_dilatation);

				const double average_time = initial_time + 0.5 * time_increment;
				const double average_dilatation_per_my =
						seconds_in_a_million_years * 0.5 * (initial_dilatation + final_dilatation);

				const double K1 = time_increment * derivative_sign * derivative_function(
						average_time,
						values[n].get() + 0.5 * K0,
						average_dilatation_per_my);

				const double K2 = time_increment * derivative_sign * derivative_function(
						average_time,
						values[n].get() + 0.5 * K1,
						average_dilatation_per_my);

				const double K3 = time_increment * derivative_sign * derivative_function(
						initial_time + time_increment,
						values[n].get() + K2,
						seconds_in_a_million_years * final_dilatation);

				values[n].get() += (K0 + 2.0 * K1 + 2.0 * K2 + K3) / 6.0;
			}
		}
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
	double derivative_sign;
	if (time_increment < 0)
	{
		// Time increment should always be positive.
		time_increment = -time_increment;

		// We're going forward in time (from old to young times) so use derivative unchanged.
		derivative_sign = 1.0;
	}
	else
	{
		// We're going backward in time (from young to old times) so invert/negate the derivative.
		derivative_sign = -1.0;
	}

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

	runge_kutta_order_2(
			&input_output_crustal_thickness[0],
			num_crustal_thicknesses,
			&crustal_thinning_derivative,
			derivative_sign,
			initial_time,
			time_increment,
			&initial_deformation_strain_rates[0],
			&final_deformation_strain_rates[0]);

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
