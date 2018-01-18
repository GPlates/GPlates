/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#include <cmath>

#include "DeformationStrain.h"

#include "maths/MathsUtils.h"


const GPlatesAppLogic::DeformationStrain::StrainPrincipal
GPlatesAppLogic::DeformationStrain::get_strain_principal() const
{
	const double f_squared =
			d_deformation_gradient.theta_theta * d_deformation_gradient.theta_theta +
			d_deformation_gradient.theta_phi * d_deformation_gradient.theta_phi +
			d_deformation_gradient.phi_theta * d_deformation_gradient.phi_theta +
			d_deformation_gradient.phi_phi * d_deformation_gradient.phi_phi;
	const double f_det =
			d_deformation_gradient.theta_theta * d_deformation_gradient.phi_phi -
			d_deformation_gradient.theta_phi * d_deformation_gradient.phi_theta;

	const double strain_variation = std::sqrt(f_squared * f_squared - 4.0 * f_det * f_det);
	const double strain1 = std::sqrt(0.5 * (f_squared + strain_variation)) - 1.0;
	const double strain2 = std::sqrt(0.5 * (f_squared - strain_variation)) - 1.0;

	const double f_angle_y = 2.0 * (
			d_deformation_gradient.theta_theta * d_deformation_gradient.phi_theta +
			d_deformation_gradient.theta_phi * d_deformation_gradient.phi_phi);
	const double f_angle_x =
			d_deformation_gradient.theta_theta * d_deformation_gradient.theta_theta +
			d_deformation_gradient.theta_phi * d_deformation_gradient.theta_phi -
			d_deformation_gradient.phi_theta * d_deformation_gradient.phi_theta -
			d_deformation_gradient.phi_phi * d_deformation_gradient.phi_phi;
	const double angle = 0.5 * std::atan2(f_angle_y, f_angle_x);

	return StrainPrincipal(strain1, strain2, angle);
}


const GPlatesAppLogic::DeformationStrain
GPlatesAppLogic::accumulate_strain(
		const DeformationStrain &previous_strain,
		const DeformationStrainRate &previous_strain_rate,
		const DeformationStrainRate &current_strain_rate,
		const double &time_increment)
{
	//
	// The rate of change of deformation gradient tensor F is:
	//
	//   dF/dt = L * F
	//
	// ...where L is the velocity spatial gradient (see chapter 4 of
	// "Introduction to the mechanics of a continuous medium" by Malvern).
	//
	// We use the central difference scheme to solve the above ordinary differential equation (ODE):
	//
	//   F(n+1) - F(n)
	//   ------------- = (L(n+1)*F(n+1) + L(n)*F(n)) / 2
	//         dt
	//
	//   (I - L(n+1)*dt/2) * F(n+1) = (I + L(n)*dt/2) * F(n)
	//
	// ...which in matrix form is...
	//
	//   F(n+1) = inverse[I - L(n+1)*dt/2] * (I + L(n)*dt/2) * F(n)
	//
	// ...which is 2x2 matrix inversion and subsequent multiplication.
	// 
	// Inverse of 2x2 matrix:
	//
	//   |a11 a12|
	//   |a21 a22|
	//
	// ...is...
	//
	//           1          |a22  -a12|
	//   -----------------  |-a21  a11|
	//   (a11*a22-a12*a21)
	//
	//
	//   F(n+1) = inverse[I - L(n+1)*dt/2] * (I + L(n)*dt/2) * F(n)
	//
	//   |F11(n+1) F12(n+1)|    1  | 1 - L22(n+1)*dt/2 L12(n+1)*dt/2    | * | 1 + L11(n)*dt/2 L12(n)*dt/2     | * |F11(n) F12(n)|
	//   |F21(n+1) F22(n+1)| = --- | L21(n+1)*dt/2    1 - L11(n+1)*dt/2 |   | L21(n)*dt/2     1 + L22(n)*dt/2 |   |F21(n) F22(n)|
	//                          D
	//
	// ...where D is Determinant(I - L(n+1)*dt/2)...
	//
	//   D = (1 - L11(n+1)*dt/2) * (1 - L22(n+1)*dt/2) - (L12(n+1)*dt/2) * (L21(n+1)*dt/2)
	//

	const DeformationStrain::DeformationGradient &prev_f = previous_strain.get_deformation_gradient();
	const DeformationStrainRate::VelocitySpatialGradient &prev_l = previous_strain_rate.get_velocity_spatial_gradient();
	const DeformationStrainRate::VelocitySpatialGradient &curr_l = current_strain_rate.get_velocity_spatial_gradient();

	const double dt = time_increment;

	// Determinant of matrix to invert.
	const double d = (1 - 0.5 * curr_l.theta_theta * dt) * (1 - 0.5 * curr_l.phi_phi * dt) -
			(0.5 * curr_l.theta_phi * dt) * (0.5 * curr_l.phi_theta * dt);

	// Avoid divide-by-zero.
	if (d > -GPlatesMaths::EPSILON &&
		d < GPlatesMaths::EPSILON)
	{
		// Unable to invert matrix - this shouldn't happen for well-behaved values of velocity spatial gradient.
		// Return the previous strain.
		return DeformationStrain(prev_f);
	}
	const double inv_d = 1.0 / d;

	double inv_curr_l_mat[2][2];
	inv_curr_l_mat[0][0] = inv_d * (1 - 0.5 * curr_l.phi_phi * dt);
	inv_curr_l_mat[0][1] = inv_d * (0.5 * curr_l.theta_phi * dt);
	inv_curr_l_mat[1][0] = inv_d * (0.5 * curr_l.phi_theta * dt);
	inv_curr_l_mat[1][1] = inv_d * (1 - 0.5 * curr_l.theta_theta * dt);

	double prev_l_mat[2][2];
	prev_l_mat[0][0] = 1 + 0.5 * prev_l.theta_theta * dt;
	prev_l_mat[0][1] = 0.5 * prev_l.theta_phi * dt;
	prev_l_mat[1][0] = 0.5 * prev_l.phi_theta * dt;
	prev_l_mat[1][1] = 1 + 0.5 * prev_l.phi_phi * dt;

	double prev_f_mat[2][2];
	prev_f_mat[0][0] = prev_f.theta_theta;
	prev_f_mat[0][1] = prev_f.theta_phi;
	prev_f_mat[1][0] = prev_f.phi_theta;
	prev_f_mat[1][1] = prev_f.phi_phi;

	double inv_curr_l_mult_prev_l_mat[2][2];
	for (int i = 0; i < 2; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			inv_curr_l_mult_prev_l_mat[i][j] =
				inv_curr_l_mat[i][0] * prev_l_mat[0][j] +
				inv_curr_l_mat[i][1] * prev_l_mat[1][j];
		}
	}

	double curr_f[2][2];
	for (int i = 0; i < 2; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			curr_f[i][j] =
				inv_curr_l_mult_prev_l_mat[i][0] * prev_f_mat[0][j] +
				inv_curr_l_mult_prev_l_mat[i][1] * prev_f_mat[1][j];
		}
	}

	return DeformationStrain(DeformationStrain::DeformationGradient(curr_f[0][0], curr_f[0][1], curr_f[1][0], curr_f[1][1]));
}


const GPlatesAppLogic::DeformationStrain
GPlatesAppLogic::interpolate_strain(
		const DeformationStrain &first_strain,
		const DeformationStrain &second_strain,
		const double &position)
{
	const DeformationStrain::DeformationGradient &first_deformation_gradient = first_strain.get_deformation_gradient();
	const DeformationStrain::DeformationGradient &second_deformation_gradient = second_strain.get_deformation_gradient();

	const double f_theta_theta = (1 - position) * first_deformation_gradient.theta_theta + position * second_deformation_gradient.theta_theta;
	const double f_theta_phi = (1 - position) * first_deformation_gradient.theta_phi + position * second_deformation_gradient.theta_phi;
	const double f_phi_theta = (1 - position) * first_deformation_gradient.phi_theta + position * second_deformation_gradient.phi_theta;
	const double f_phi_phi = (1 - position) * first_deformation_gradient.phi_phi + position * second_deformation_gradient.phi_phi;

	return DeformationStrain(DeformationStrain::DeformationGradient(f_theta_theta, f_theta_phi, f_phi_theta, f_phi_phi));
}
