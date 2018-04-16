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
		 * This is trace(D): trace of the rate-of-deformation symmetric tensor D.
		 */
		double
		get_strain_rate_dilatation() const
		{
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
			const RateOfDeformation rate_of_deformation = get_rate_of_deformation();
			return std::sqrt(
					rate_of_deformation.theta_theta * rate_of_deformation.theta_theta +
					rate_of_deformation.phi_phi * rate_of_deformation.phi_phi +
					//
					// The factor of 2 is due to D being symmetric (ie, D_theta_phi = D_phi_theta) and hence:
					//
					//   D_theta_phi * D_theta_phi = D_phi_theta * D_phi_theta
					//
					2 * rate_of_deformation.theta_phi * rate_of_deformation.theta_phi);
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
