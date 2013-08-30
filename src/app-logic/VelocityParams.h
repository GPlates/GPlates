/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_VELOCITYPARAMS_H
#define GPLATES_APP_LOGIC_VELOCITYPARAMS_H

#include <boost/operators.hpp>
#include <boost/optional.hpp>


namespace GPlatesAppLogic
{
	/**
	 * VelocityParams is used to store additional parameters for calculating velocities in
	 * @a VelocityFieldCalculatorLayerTask layers.
	 */
	class VelocityParams :
			public boost::less_than_comparable<VelocityParams>,
			public boost::equality_comparable<VelocityParams>
	{
	public:

		//! How to calculate velocities.
		enum SolveVelocitiesMethodType
		{
			// Intersects reconstructed domain geometry with polygon/network surface and
			// calculates velocity of latter at the position of the former.
			SOLVE_VELOCITIES_OF_SURFACES_AT_DOMAIN_POINTS,

			// Calculates velocity of reconstructed domain geometry itself.
			SOLVE_VELOCITIES_OF_DOMAIN_POINTS,

			NUM_SOLVE_VELOCITY_METHODS    // This must be last.
		};


		VelocityParams() :
			// Default to using surfaces since that's how GPlates started out calculating velocities...
			d_solve_velocities_method(SOLVE_VELOCITIES_OF_SURFACES_AT_DOMAIN_POINTS)
		{  }

		SolveVelocitiesMethodType
		get_solve_velocities_method() const
		{
			return d_solve_velocities_method;
		}

		void
		set_solve_velocities_method(
				SolveVelocitiesMethodType solve_velocities_method)
		{
			d_solve_velocities_method = solve_velocities_method;
		}


		//! Equality comparison operator.
		bool
		operator==(
				const VelocityParams &rhs) const
		{
			return d_solve_velocities_method == rhs.d_solve_velocities_method;
		}

		//! Less than comparison operator.
		bool
		operator<(
				const VelocityParams &rhs) const
		{
			if (d_solve_velocities_method < rhs.d_solve_velocities_method)
			{
				return true;
			}

			return false;
		}

	private:

		SolveVelocitiesMethodType d_solve_velocities_method;

	};
}

#endif // GPLATES_APP_LOGIC_VELOCITYPARAMS_H
