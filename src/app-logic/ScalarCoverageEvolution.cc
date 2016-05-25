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
		 * Convert from millions of years to seconds.
		 *
		 * Strain rates are in metres/sec (multiplying converts to metres).
		 */
		const double million_years_to_seconds = 365.25 * 24 * 3600 * 1.0e6;

		/**
		 * Convert metres to kms.
		 */
		const double metres_to_kilometres = 1e-3;


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
				double *const values,
				const unsigned int num_values,
				const DerivativeFunctionType &derivative_function,
				const double &initial_time,
				const double &time_increment,
				const GeometryDeformation::DeformationInfo *const initial_deformation_infos,
				const GeometryDeformation::DeformationInfo *const final_deformation_infos)
		{
			for (unsigned int n = 0; n < num_values; ++n)
			{
				const double K0 = time_increment * derivative_function(
						initial_time,
						values[n],
						million_years_to_seconds * initial_deformation_infos[n].dilitation);

 						// FIXME: check on the units of this stuff ... ?

						//metres_to_kilometres * million_years_to_seconds * initial_deformation_infos[n].dilitation);

				const double average_time = initial_time + 0.5 * time_increment;

				const double average_dilatation = million_years_to_seconds * 0.5 * (
						initial_deformation_infos[n].dilitation + final_deformation_infos[n].dilitation);

				// FIXME: check on the units of this stuff ... ?

				//const double average_dilatation = metres_to_kilometres * million_years_to_seconds * 0.5 * (
				//		initial_deformation_infos[n].dilitation + final_deformation_infos[n].dilitation);

				const double K1 = time_increment * derivative_function(
						average_time,
						values[n] + 0.5 * K0,
						average_dilatation);
   
				values[n] += K1;

				qDebug() << "rko2: n = " << n 
					<< "; i_dil = " << initial_deformation_infos[n].dilitation
					<< "; f_dil = " << final_deformation_infos[n].dilitation 
					<< "; a_dil = " << average_dilatation 
					<< "; v[n] = " << values[n];
			}
		}


		/** 
		 * Runge Kutta order 4 integration.
		 */
		template <typename DerivativeFunctionType>
		void
		runge_kutta_order_4(
				double *const values,
				const unsigned int num_values,
				const DerivativeFunctionType &derivative_function,
				const double &initial_time,
				const double &time_increment,
				const GeometryDeformation::DeformationInfo *const initial_deformation_infos,
				const GeometryDeformation::DeformationInfo *const final_deformation_infos)
		{
			for (unsigned int n = 0; n < num_values; ++n)
			{
				const double K0 = time_increment * derivative_function(
						initial_time,
						values[n],
						metres_to_kilometres * million_years_to_seconds * initial_deformation_infos[n].dilitation);

				const double average_time = initial_time + 0.5 * time_increment;
				const double average_dilatation = metres_to_kilometres * million_years_to_seconds * 0.5 * (
						initial_deformation_infos[n].dilitation + final_deformation_infos[n].dilitation);

				const double K1 = time_increment * derivative_function(
						average_time,
						values[n] + 0.5 * K0,
						average_dilatation);

				const double K2 = time_increment * derivative_function(
						average_time,
						values[n] + 0.5 * K1,
						average_dilatation);

				const double K3 = time_increment * derivative_function(
						initial_time + time_increment,
						values[n] + K2,
						metres_to_kilometres * million_years_to_seconds * final_deformation_infos[n].dilitation);

				values[n] += (K0 + 2.0 * K1 + 2.0 * K2 + K3) / 6.0;
			}
		}
	}
}


boost::optional<GPlatesAppLogic::scalar_evolution_function_type>
GPlatesAppLogic::get_scalar_evolution_function(
		const GPlatesPropertyValues::ValueObjectType &scalar_type,
		const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params)
{
	static const GPlatesPropertyValues::ValueObjectType CRUSTAL_THICKNESS =
			GPlatesPropertyValues::ValueObjectType::create_gpml("CrustalThickness");

	if (scalar_type == CRUSTAL_THICKNESS)
	{
		return scalar_evolution_function_type(&ScalarCoverageEvolution::crustal_thinning);
	}

	// No evolution needed - scalar values don't change over time.
	return boost::none;
}


void
GPlatesAppLogic::ScalarCoverageEvolution::crustal_thinning(
		std::vector<double> &input_output_crustal_thickness,
		const std::vector<GeometryDeformation::DeformationInfo> &initial_deformation_info,
		const std::vector<GeometryDeformation::DeformationInfo> &final_deformation_info,
		const double &initial_time,
		const double &final_time)
{
	// Input array sizes should match.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			input_output_crustal_thickness.size() == initial_deformation_info.size() &&
				initial_deformation_info.size() == final_deformation_info.size(),
			GPLATES_ASSERTION_SOURCE);

    int n = input_output_crustal_thickness.size();
    qDebug() << "ct: number of points n=" << n;

    //double min0 = *std::min_element( input_output_crustal_thickness.begin(), input_output_crustal_thickness.end() );
    //double max0 = *std::max_element( input_output_crustal_thickness.begin(), input_output_crustal_thickness.end() );


	runge_kutta_order_2(
			&input_output_crustal_thickness[0],
			input_output_crustal_thickness.size(),
			&crustal_thinning_derivative,
			initial_time,
			final_time - initial_time,
			&initial_deformation_info[0],
			&final_deformation_info[0]);

    double min1 = *std::min_element( input_output_crustal_thickness.begin(), input_output_crustal_thickness.end() );
    double max1 = *std::max_element( input_output_crustal_thickness.begin(), input_output_crustal_thickness.end() );
    qDebug() << "ct: n=" << n << "; time =" << final_time << "; min=" << min1 << "; max1=" << max1;
}
