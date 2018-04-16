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

#include "DeformationStrainRate.h"


namespace GPlatesAppLogic
{
	/**
	 * Stores the deformation gradient tensor which can be used to get the strain tensor
	 * (also known as total strain or just strain) and its principal components.
	 *
	 * In chapter 4 of "Introduction to the mechanics of a continuous medium" by Malvern:
	 * The deformation gradient tensor is F in the case of finite strain.
	 * The strain tensor E = 0.5 * (F_transpose * F - I).
	 */
	class DeformationStrain
	{
	public:

		/**
		 * The deformation gradient tensor F.
		 */
		struct DeformationGradient
		{
			//! Identity deformation gradient (non-deforming).
			DeformationGradient() :
				theta_theta(1),
				theta_phi(0),
				phi_theta(0),
				phi_phi(1)
			{  }

			DeformationGradient(
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


		struct StrainPrincipal
		{
			StrainPrincipal(
					const double &principal1_,
					const double &principal2_,
					const double &angle_) :
				principal1(principal1_),
				principal2(principal2_),
				angle(angle_)
			{  }

			/**
			 * The larger principle strain.
			 *
			 * If @a angle is zero then this is aligned with co-latitude
			 * (ie, the direction from North to South).
			 */
			double principal1;

			/**
			 * The smaller principle strain.
			 *
			 * If @a angle is zero then this is aligned with longitude
			 * (ie, the direction from West to East).
			 */
			double principal2;

			/**
			 * The angle to rotate the principle strain directions.
			 *
			 * This is a counter-clockwise rotation (for positive angles) when viewed from above the globe.
			 */
			double angle;
		};


		//! Identity strain.
		DeformationStrain()
		{  }

		explicit
		DeformationStrain(
				const DeformationGradient &deformation_gradient) :
			d_deformation_gradient(deformation_gradient)
		{  }


		/**
		 * Return the deformation gradient tensor F.
		 */
		const DeformationGradient &
		get_deformation_gradient() const
		{
			return d_deformation_gradient;
		}


		/**
		 * Return the principle strain.
		 */
		const StrainPrincipal
		get_strain_principal() const;

		double
		get_strain_dilatation() const
		{
			return 0;
		}

	private:

		DeformationGradient d_deformation_gradient;
	};


	/**
	 * Accumulate the previous strain using both the previous and current strain rates (units in 1/second)
	 * over a time increment (units in seconds).
	 */
	const DeformationStrain
	accumulate_strain(
			const DeformationStrain &previous_strain,
			const DeformationStrainRate &previous_strain_rate,
			const DeformationStrainRate &current_strain_rate,
			const double &time_increment);


	/**
	 * Linearly interpolate between two strains.
	 *
	 * @param position A value between 0.0 and 1.0 (inclusive), which can be
	 * interpreted as where the returned strain lies in the range between the
	 * first strain and the second strain.
	 */
	const DeformationStrain
	interpolate_strain(
			const DeformationStrain &first_strain,
			const DeformationStrain &second_strain,
			const double &position);
}

#endif // GPLATES_APP_LOGIC_DEFORMATION_STRAIN_H
