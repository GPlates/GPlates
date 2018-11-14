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

#ifndef GPLATES_APP_LOGIC_DEFORMATION_STRAIN_RATE_H
#define GPLATES_APP_LOGIC_DEFORMATION_STRAIN_RATE_H

#include <cfloat>
#include <cmath>


namespace GPlatesAppLogic
{
	/**
	 * Stores the spatial gradients of velocity L from which the rate-of-deformation symmetric tensor D is obtained via:
	 *
	 *   D = (L + transpose(L)) / 2
	 * 
	 * Both L and D are in units of (1/second).
	 */
	class DeformationStrainRate
	{
	public:

		/**
		 * Spatial gradients of velocity L.
		 */
		struct VelocitySpatialGradient
		{
			//! Zero velocity gradient (non-deforming).
			VelocitySpatialGradient() :
				theta_theta(0),
				theta_phi(0),
				phi_theta(0),
				phi_phi(0)
			{  }

			VelocitySpatialGradient(
					const double &theta_theta_,
					const double &theta_phi_,
					const double &phi_theta_,
					const double &phi_phi_) :
				theta_theta(theta_theta_),
				theta_phi(theta_phi_),
				phi_theta(phi_theta_),
				phi_phi(phi_phi_)
			{  }

			double theta_theta;
			double theta_phi;
			double phi_theta;
			double phi_phi;
		};

		/**
		 * Rate-of-deformation symmetric tensor D.
		 * 
		 *   D = (L + transpose(L)) / 2
		 *
		 * ...which means diagonal components of D and L are the same (due to above equation)
		 * and the off-diagonal components of D are equal (since D is symmetric).
		 */
		struct RateOfDeformation
		{
			explicit
			RateOfDeformation(
					const VelocitySpatialGradient &velocity_spatial_gradient) :
				theta_theta(velocity_spatial_gradient.theta_theta),
				theta_phi(0.5 * (velocity_spatial_gradient.theta_phi + velocity_spatial_gradient.phi_theta)),
				phi_theta(theta_phi),
				phi_phi(velocity_spatial_gradient.phi_phi)
			{  }

			double theta_theta;
			double theta_phi;
			double phi_theta;
			double phi_phi;
		};


		//! Zero strain rate (non-deforming).
		DeformationStrainRate()
		{  }

		/**
		 * Specify the (theta, theta), (theta, phi), (phi, theta) and (phi, phi) components
		 * of the velocity spatial gradient tensor L.
		 *
		 * The rate-of-deformation symmetric tensor D is then obtained via:
		 *
		 *   D = (L + transpose(L)) / 2
		 * 
		 * Where L is defined in chapter 4 of "Introduction to the mechanics of a continuous medium" by Malvern).
		 */
		DeformationStrainRate(
				const double &velocity_gradient_theta_theta,
				const double &velocity_gradient_theta_phi,
				const double &velocity_gradient_phi_theta,
				const double &velocity_gradient_phi_phi) :
			d_velocity_spatial_gradient(
					velocity_gradient_theta_theta,
					velocity_gradient_theta_phi,
					velocity_gradient_phi_theta,
					velocity_gradient_phi_phi)
		{  }


		/**
		 * Return the spatial gradients of velocity L.
		 */
		const VelocitySpatialGradient &
		get_velocity_spatial_gradient() const
		{
			return d_velocity_spatial_gradient;
		}


		/**
		 * Return the rate-of-deformation symmetric tensor D.
		 */
		RateOfDeformation
		get_rate_of_deformation() const
		{
			return RateOfDeformation(d_velocity_spatial_gradient);
		}


		/**
		 * Return the strain rate dilatation.
		 *
		 * This is trace(D): trace of the rate-of-deformation symmetric tensor D
		 * (see section 4.11 of "Continuum mechanics for engineers" by Mase).
		 * 
		 * Essentially an initial parallelepiped of volume dV formed by parallel edge vectors dX1, dX2, dX2
		 * is deformed into a parallelepiped of volume dv formed by parallel edge vectors dx1, dx2, dx2.
		 *
		 * We want to find (dv/dt)/dv which is the rate of change of dv. The following carries on
		 * from the derivation in the comment of DeformationStrain::get_strain_dilatation() where
		 * 
		 *   dv = det(F) * dV
		 * 
		 * where F is the deformation gradient tensor.
		 * 
		 *   dv/dt = d(det(F))/dt * dV + det(F) * dV/dt
		 *         = d(det(F))/dt * dV
		 * 
		 * using dV/dt = 0 since initial volume does not change.
		 * The following identity is used
		 * 
		 *   d(det(F))/dt = trace(dF/dt * inverse(F)) * det(F)
		 *                = trace(L) * det(F)
		 *                = trace(D) * det(F)
		 * 
		 * where dF/dt = L * F was used, and trace(L)=trace(D) since D is symmetric part of L=D+W
		 * (skew-symmetric part W has trace of zero).
		 * So now
		 * 
		 *   dv/dt = trace(D) * det(F) * dV
		 *         = trace(D) * dv
		 * 
		 * or
		 * 
		 *   (dv/dt) / dv = trace(D)
		 */
		double
		get_strain_rate_dilatation() const
		{
			// Another way to look at this is to consider the initial dV and hence deformed dv volumes
			// to be aligned with the principal axes such that deformed dx1, dx2 and dx3 are orthogonal
			// (as are initial dX1, dX2 and dX3)
			// 
			//   (dv/dt)/dv = d(dx1 * dx2 * dx3)/dt / (dx1 * dx2 * dx3)
			//              = (d(dx1)/dt / dx1) + (d(dx2)/dt / dx2) + (d(dx3)/dt / dx3)
			//              = D_principal_11 + D_principal_22 + D_principal_33
			//              = trace(D_principal)
			//              = trace(D)   # since trace is invariant.

			// The following is the equivalent to:
			//
			//   return get_rate_of_deformation().theta_theta + get_rate_of_deformation().phi_phi;
			//
			// ...since we only need the diagonal components of D (which are same as L).
			return d_velocity_spatial_gradient.theta_theta + d_velocity_spatial_gradient.phi_phi;
		}

		/**
		 * Return the strain rate second invariant.
		 *
		 * This is sqrt[trace(D^2)]: trace of the square of the rate-of-deformation symmetric tensor D.
		 */
		double
		get_strain_rate_second_invariant() const
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


		/**
		 * Return the strain rate style.
		 *
		 * This is a quantity defined in Kreemer et al. 2014 as:
		 *
		 *      D_principal_11 + D_principal_22
		 *   --------------------------------------
		 *   max(|D_principal_11|, |D_principal_22|)
		 *
		 * ...and the result is typically clamped to the range [-1, 1], although that's not done here.
		 */
		double
		get_strain_rate_style() const
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

			return (principal_D_11 + principal_D_22) / max_abs_principal_D;
		}


		friend
		DeformationStrainRate
		operator+(
				const DeformationStrainRate &lhs,
				const DeformationStrainRate &rhs)
		{
			return DeformationStrainRate(
					lhs.d_velocity_spatial_gradient.theta_theta + rhs.d_velocity_spatial_gradient.theta_theta,
					lhs.d_velocity_spatial_gradient.theta_phi + rhs.d_velocity_spatial_gradient.theta_phi,
					lhs.d_velocity_spatial_gradient.phi_theta + rhs.d_velocity_spatial_gradient.phi_theta,
					lhs.d_velocity_spatial_gradient.phi_phi + rhs.d_velocity_spatial_gradient.phi_phi);
		}

		friend
		DeformationStrainRate
		operator*(
				double scale,
				const DeformationStrainRate &strain_rate)
		{
			return DeformationStrainRate(
					scale * strain_rate.d_velocity_spatial_gradient.theta_theta,
					scale * strain_rate.d_velocity_spatial_gradient.theta_phi,
					scale * strain_rate.d_velocity_spatial_gradient.phi_theta,
					scale * strain_rate.d_velocity_spatial_gradient.phi_phi);
		}

		friend
		DeformationStrainRate
		operator*(
				const DeformationStrainRate &strain_rate,
				double scale)
		{
			return scale * strain_rate;
		}

	private:

		VelocitySpatialGradient d_velocity_spatial_gradient;
	};
}

#endif // GPLATES_APP_LOGIC_DEFORMATION_STRAIN_RATE_H
