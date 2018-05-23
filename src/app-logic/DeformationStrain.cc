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

#include <algorithm> // std::swap
#include <cmath>

#include "DeformationStrain.h"

#include "maths/MathsUtils.h"


const GPlatesAppLogic::DeformationStrain::StrainPrincipal
GPlatesAppLogic::DeformationStrain::get_strain_principal() const
{
	//
	// The stretch along a deformed normal direction n is (|dx|/|dX|) where dx and dX are
	// deformed and undeformed element vectors. The square of the stretch is
	//
	//   stretch(n)^2 = |dx|^2 / |dX|^2
	//                = (dx.dx) / (dX.dX)
	//   1 / stretch(n)^2 = (dX.dX) / (dx.dx)
	//                    = ((F^-1 * dx).(F^-1 * dx)) / (dx.dx)
	//                    = (dx * transpose(F^-1) * F^-1 * dx) / (dx.dx)
	//                    = (dx * c * dx) / (dx.dx)
	//                    = (dx * c * dx) / |dx|^2
	//                    = n.c.n
	//
	// where "n = dx / |dx|" and "c = transpose(F^-1) * F^-1" is the Cauchy deformation tensor in terms
	// of the deformation gradient tensor F.
	//
	// The principal axes are orthogonal and this occurs when the two deformed normal directions
	// dx1 and dx2 are aligned with the principal axes, which happens when their dot product is zero.
	// And these principal axes rotated back into the undeformed configuration with undeformed normal
	// directions dX1 and dX2 that will also be orthogonal with a zero dot product defined by
	// 
	//   0 = dX1.dX2
	//     = (F^-1 * dx1).(F^-1 * dx2)
	//     = dx1.transpose(F^-1).(F^-1).dx2
	//     = dx1.c.dx2
	// 
	// and dividing by |dx1|*|dx2| we have
	// 
	//   0 = dx1.c.dx2 / (|dx1|*|dx2|)
	//     = n1.c.n2
	// 
	// where n1 and n2 are orthogonal principal axes in deformed configuration.
	// If we set n1 at an angle 'angle' to the original axes (and n2 at 90 degrees larger) then
	// 
	//   n1 = (cos(angle) , sin(angle))
	//   n2 = (-sin(angle), cos(angle))
	//
	//   0 = n1.c.n2
	//     = (c_22 - c_11) * sin(angle) * cos(angle) + c_12 * (cos^2(angle) - sin^2(angle))
	//
	// which is equivalent to
	// 
	//   tan(2*angle) = 2 * c_12 / (c_11 - c_22)
	// 
	// which gives us the angle of rotation of the principal axes in the deformed configuration.
	// 
	// To get c we first need the inverse of F
	// 
	//   F^-1 = 1/det(F) * |  F_22 -F_12 |
	//                     | -F_21  F_11 |
	// 
	//   c    = transpose(F^-1) * (F^-1)
	//   c_11 = 1/det(F)^2 * (F_22^2 + F_21^2)
	//   c_22 = 1/det(F)^2 * (F_12^2 + F_11^2)
	//   c_12 = 1/det(F)^2 * (-F_12*F_22 - F_11*F_21)
	//   c_21 = c_12
	//
	// And from previously we have
	// 
	//   1 / stretch(n1)^2 = n1.c.n1
	//   1 / stretch(n2)^2 = n2.c.n2
	// 
	// where
	// 
	//   n1.c.n1 = c_11 * cos^2(angle) + c_22 * sin^2(angle) + 2 * c_12 * sin(angle) * cos(angle)
	//   n2.c.n2 = c_11 * sin^2(angle) + c_22 * cos^2(angle) - 2 * c_12 * sin(angle) * cos(angle)
	//
	// and the engineering strain in the principal directions n1 and n2 are
	// 
	//   strain(n1) = (|dx1| - |dX1|) / |dX1| = stretch(n1) - 1 = 1/sqrt(n1.c.n1) - 1
	//   strain(n2) = (|dx2| - |dX2|) / |dX2| = stretch(n2) - 1 = 1/sqrt(n2.c.n2) - 1
	//
	//
	// References:
	// 
	//    Section 4.8 of "Continuum mechanics for engineers" by Mase.
	//    
	//    Principal Strains & Invariants - http://www.continuummechanics.org/principalstrain.html
	//   

	const double F_det =
			d_deformation_gradient.theta_theta * d_deformation_gradient.phi_phi -
			d_deformation_gradient.theta_phi * d_deformation_gradient.phi_theta;
	// Deformation gradient tensor F only has an inverse if it's determinant is non-zero
	// (which should always be the case, but we'll check to be sure).
	if (F_det <= 0.0)
	{
		// Return zero strain.
		return StrainPrincipal(0.0/*strain1*/, 0.0/*strain2*/, 0.0/*angle*/);
	}

	const double inv_square_F_det = 1.0 / (F_det * F_det);
	const double c_theta_theta = inv_square_F_det * (
			d_deformation_gradient.phi_phi * d_deformation_gradient.phi_phi +
			d_deformation_gradient.phi_theta * d_deformation_gradient.phi_theta);
	const double c_phi_phi = inv_square_F_det * (
			d_deformation_gradient.theta_phi * d_deformation_gradient.theta_phi +
			d_deformation_gradient.theta_theta * d_deformation_gradient.theta_theta);
	const double c_theta_phi = inv_square_F_det * (
			-d_deformation_gradient.theta_phi * d_deformation_gradient.phi_phi -
			d_deformation_gradient.theta_theta * d_deformation_gradient.phi_theta);

	double angle = 0.5 * std::atan2(2 * c_theta_phi, c_theta_theta - c_phi_phi);
	const double cos_angle = std::cos(angle);
	const double sin_angle = std::sin(angle);

	const double c1 = c_theta_theta * cos_angle * cos_angle +
			c_phi_phi * sin_angle * sin_angle +
			2.0 * c_theta_phi * sin_angle * cos_angle;
	const double c2 = c_theta_theta * sin_angle * sin_angle +
			c_phi_phi * cos_angle * cos_angle -
			2.0 * c_theta_phi * sin_angle * cos_angle;

	// Cauchy deformation tensor 'c' is symmetric and positive definite, so its eigenvalues c1 and c2 are positive.
	double strain1 = (1.0 / std::sqrt(c1)) - 1.0;
	double strain2 = (1.0 / std::sqrt(c2)) - 1.0;

	// Keep the largest strain (positive is extension, negative is compression)
	// in the first strain. Swap if necessary, including the angle which we have specified
	// to always be relative to the first strain.
	//
	// Note: It might be possible to always swap them (without checking) since the eigenvalues
	// c1 and c2 of the Cauchy deformation tensor 'c' might be such that c1 > c2 always holds, and
	// hence strain2 > strain1 is always true. But I'm not sure so we'll check anyway.
	if (strain2 > strain1)
	{
		std::swap(strain1, strain2);
		// The second strain is 90 degrees larger than the first strain.
		angle += GPlatesMaths::HALF_PI;
	}

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
