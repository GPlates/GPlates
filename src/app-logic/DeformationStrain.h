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

#ifndef GPLATES_APP_LOGIC_DEFORMATION_STRAIN_H
#define GPLATES_APP_LOGIC_DEFORMATION_STRAIN_H

#include <cfloat>
#include <cmath>

#ifdef _MSC_VER
#ifndef copysign
#define copysign _copysign
#endif
#endif


namespace GPlatesAppLogic
{
	/**
	 * Can be used to store either:
	 * - instantaneous strain (also known as strain rate), or
	 * - accumulated strain (also known as total strain or just strain).
	 */
	class DeformationStrain
	{
	public:

		struct StrainPrincipal
		{
			StrainPrincipal(
					double principal1_,
					double principal2_) :
				principal1(principal1_),
				principal2(principal2_)
			{  }

			/**
			 * The larger principle strain.
			 *
			 * If @a get_strain_principal_angle returns zero then this is aligned with co-latitude
			 * (ie, the direction from North to South).
			 */
			double principal1;

			/**
			 * The smaller principle strain.
			 *
			 * If @a get_strain_principal_angle returns zero then this is aligned with longitude
			 * (ie, the direction from West to East).
			 */
			double principal2;
		};


		//! Zero strain (non-deforming).
		DeformationStrain() :
			d_strain_theta_theta(0),
			d_strain_phi_phi(0),
			d_strain_theta_phi(0)
		{  }

		DeformationStrain(
				const double &strain_theta_theta,
				const double &strain_phi_phi,
				const double &strain_theta_phi) :
			d_strain_theta_theta(strain_theta_theta),
			d_strain_phi_phi(strain_phi_phi),
			d_strain_theta_phi(strain_theta_phi)
		{  }

		double
		get_strain_theta_theta() const
		{
			return d_strain_theta_theta;
		}

		double
		get_strain_phi_phi() const
		{
			return d_strain_phi_phi;
		}

		double
		get_strain_theta_phi() const
		{
			return d_strain_theta_phi;
		}

		double
		get_dilatation() const
		{
			return d_strain_theta_theta + d_strain_phi_phi;
		}

		double
		get_second_invariant() const
		{
			const double second_invariant_squared = d_strain_theta_theta * d_strain_phi_phi -
					d_strain_theta_phi * d_strain_theta_phi;
			return copysign(
					std::sqrt(std::abs(second_invariant_squared)),
					second_invariant_squared);
		}

		/**
		 * Return the principle strain.
		 *
		 * If @a get_strain_principal_angle returns zero then this is aligned with co-latitude
		 * (ie, the direction from North to South).
		 */
		const StrainPrincipal
		get_strain_principal() const
		{
			const double SR_variation = std::sqrt(
					d_strain_theta_phi * d_strain_theta_phi +
					0.25 * (d_strain_theta_theta - d_strain_phi_phi) *
						(d_strain_theta_theta - d_strain_phi_phi));
			const double SR1 = 0.5 * (d_strain_theta_theta + d_strain_phi_phi) + SR_variation;
			const double SR2 = 0.5 * (d_strain_theta_theta + d_strain_phi_phi) - SR_variation;

			return StrainPrincipal(SR1, SR2);
		}

		/**
		 * Return the angle to rotate the principle strain directions.
		 *
		 * This is a counter-clockwise rotation (for positive angles) when viewed from above the globe.
		 */
		double
		get_strain_principal_angle() const
		{
			return 0.5 * std::atan2(
					2.0 * d_strain_theta_phi,
					d_strain_theta_theta - d_strain_phi_phi);
		}


		friend
		DeformationStrain
		operator+(
				const DeformationStrain &lhs,
				const DeformationStrain &rhs)
		{
			return DeformationStrain(
					lhs.d_strain_theta_theta + rhs.d_strain_theta_theta,
					lhs.d_strain_phi_phi + rhs.d_strain_phi_phi,
					lhs.d_strain_theta_phi + rhs.d_strain_theta_phi);
		}

		friend
		DeformationStrain
		operator*(
				double scale,
				const DeformationStrain &di)
		{
			return DeformationStrain(
					scale * di.d_strain_theta_theta,
					scale * di.d_strain_phi_phi,
					scale * di.d_strain_theta_phi);
		}

		friend
		DeformationStrain
		operator*(
				const DeformationStrain &di,
				double scale)
		{
			return scale * di;
		}

	private:

		double d_strain_theta_theta;
		double d_strain_phi_phi;
		double d_strain_theta_phi;
	};
}

#endif // GPLATES_APP_LOGIC_DEFORMATION_STRAIN_H
