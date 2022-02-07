/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#include <cfloat>
#include <cmath>

#include "DeformationStrainRate.h"


double
GPlatesAppLogic::DeformationStrainRate::get_strain_rate_second_invariant() const
{
	// Use the geodesy definition of second invariant in Kreemer et al. 2014.
	// This amounts to sqrt[trace(D^2)] where D is the rate-of-deformation symmetric tensor.
	// See chapter 4 of "Introduction to the mechanics of a continuous medium" by Malvern).
	//
	// Note that the usual second invariant is 0.5 * [trace(D)^2 - trace(D^2)].
	// But a sqrt[trace(D^2)] is a function of invariants and hence is also invariant.
	const RateOfDeformation rate_of_deformation = get_rate_of_deformation();
	return std::sqrt(
			//
			// trace(D^2) = D_11*D_11 + D_12*D_21 + D_21*D_12 + D_22*D_22
			//            = D_11*D_11 + D_22*D_22 + 2 * D_12*D_21
			//            = D_11*D_11 + D_22*D_22 + 2 * D_12*D_12
			// 
			// where D_21 = D_12 since D is symmetric.
			//
			rate_of_deformation.theta_theta * rate_of_deformation.theta_theta +
			rate_of_deformation.phi_phi * rate_of_deformation.phi_phi +
			2 * rate_of_deformation.theta_phi * rate_of_deformation.theta_phi);
}


double
GPlatesAppLogic::DeformationStrainRate::get_strain_rate_style() const
{
	//
	// The principal values of D are (eigenvalues of 2x2 tensor D):
	// 
	// D_principal = 0.5 * (D_11 + D_22) +/- sqrt[(D_12 * D21) + ((D_11 - D_22)/2)^2]
	//             = 0.5 * (D_11 + D_22) +/- sqrt[D_12^2 + ((D_11 - D_22)/2)^2]
	// 
	// where D_21 = D_12 since D is symmetric.
	//
	const RateOfDeformation rate_of_deformation = get_rate_of_deformation();

	const double half_trace_D = 0.5 * (rate_of_deformation.theta_theta + rate_of_deformation.phi_phi);
	const double half_principal_D_diff = std::sqrt(
			rate_of_deformation.theta_phi * rate_of_deformation.theta_phi +
			0.25 * (rate_of_deformation.theta_theta - rate_of_deformation.phi_phi) *
					(rate_of_deformation.theta_theta - rate_of_deformation.phi_phi));

	const double principal_D_11 = half_trace_D + half_principal_D_diff;
	const double principal_D_22 = half_trace_D - half_principal_D_diff;

	const double abs_principal_D_11 = std::fabs(principal_D_11);
	const double abs_principal_D_22 = std::fabs(principal_D_22);
	const double max_abs_principal_D = (abs_principal_D_11 > abs_principal_D_22)
			? abs_principal_D_11
			: abs_principal_D_22;

	// NOTE: If all principal components are zero (because the strain rate is zero) then
	// we'll get NaN (zero divided by zero).
	return (principal_D_11 + principal_D_22) / max_abs_principal_D;
}
