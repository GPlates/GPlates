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
		 * Return the strain dilatation.
		 *
		 * This is "det(F) - 1": determinant of the deformation gradient tensor F minus one
		 * (see section 4.11 of "Continuum mechanics for engineers" by Mase).
		 *
		 * Essentially an initial parallelepiped of volume dV formed by parallel edge vectors dX1, dX2, dX2
		 * is deformed into a parallelepiped of volume dv formed by parallel edge vectors dx1, dx2, dx2.
		 *
		 * The initial volume is, using a vector dot and cross product,
		 * 
		 *   dV = dX1 . dX2 x dX3
		 *      = e_ijk * dX1_i * dX2_j * dX3_k
		 *
		 * where e_ijk is the permutation symbol. Note that indices are after the '_'.
		 * The determinant of the deformation gradient tensor F, det(F), can be written as
		 *
		 *   e_qmn * det(F) = e_ijk * F_iq * F_jm * F_kn
		 * 
		 * where F_ab is the element of F in 'a'th row and 'b'th column.
		 * The final deformed volume is
		 *
		 *   dv = dx1 . dx2 x dx3
		 *      = e_ijk * dx1_i * dx2_j * dx3_k
		 *      = e_ijk * (F_iA * dX1_A) * (F_jB * dX2_B) * (F_kC * dX3_C)
		 *      = (e_ijk * F_iA * F_jB * F_kC) * dX1_A * dX2_B * dX3_C
		 *      = (e_ABC * det(F)) * dX1_A * dX2_B * dX3_C
		 *      = det(F) * (e_ABC * dX1_A * dX2_B * dX3_C)
		 *      = det(F) * dV
		 * 
		 *   dv / dV = det(F)
		 *   (dv - dV) / dV = det(F) - 1
		 *   dilatation = det(F) - 1
		 *
		 * In our case F is 2D (not 3D) because we only consider deformation in the latitude/longitude
		 * directions (and not depth). But the result for 2D is the same as 3D if we consider
		 * dx1 to point to be along the depth dimension and have there be no normal or shear strain
		 * along the direction so that
		 * 
		 *   F = | 1  0   0  |
		 *       | 0 F22 F23 |
		 *       | 0 F32 F33 |
		 * 
		 * such that det(F) = F22 * F33 - F23 * F32 which is same as determinant of 2D sub-matrix.
		 * And dX2 and dX3 are in the latitude/longitude plane such that dV = dX1 . dX2 x dX3
		 * becomes |dX2 x dX3| which is area (per unit depth) of 2D (lat/lon) parallelepiped.
		 */
		double
		get_strain_dilatation() const
		{
			// Mathematically det(F) could be negative even though physically it should only be positive.
			// Just to be sure we'll make it positive. This is similar to taking the sqrt of the
			// determinant of Green's deformation tensor C = transpose(F) * F where det(C) = det(F) * det(F)
			// is the square of the determinant of F. The determinant is invariant under orthogonal
			// transformations so if we consider initial dV and hence deformed dv volumes to be aligned
			// with the principal axes then C is diagonal and its elements are the squares of three
			// stretch ratios dx1/dX1, dx2/dX2 and dx3/dX3 - and so dv/dV is sqrt(trace(C)) which is
			// sqrt(det(C)) which is abs(det(F)) which is invariant and hence dV and dv don't have
			// to be aligned with the principal axes.
			return std::fabs(d_deformation_gradient.theta_theta * d_deformation_gradient.phi_phi -
							d_deformation_gradient.theta_phi * d_deformation_gradient.phi_theta) -
					1.0;
		}


		/**
		 * Return the principle strain.
		 */
		const StrainPrincipal
		get_strain_principal() const;

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
