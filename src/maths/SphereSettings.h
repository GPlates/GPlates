/* $Id$ */

/**
 * \file 
 * This file contains typedefs that are used in various places in
 * the code; they are not tied to any particular class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_SPHERESETTINGS_H
#define GPLATES_MATHS_SPHERESETTINGS_H

#include <boost/shared_ptr.hpp>

#include "types.h"

#include "utils/Singleton.h"

namespace GPlatesMaths
{
	namespace Spherical
	{
		const real_t DefaultEarthRadius = 6378.1;
		const real_t PI = 3.14159265358979323846;
		//TODO: This is suspicious and needs more investigation.
		const real_t DotProductDistanceAccuracyTolerance = 1.0e-7; 
		//This is a dot-product value, -1 means the largest distance on sphere
		const real_t MaxDotProductDistanceOnSphere = -1; 
		//This is a dot-product value, 1 means the zero distance on sphere
		const real_t ZeroDotProductDistanceOnSphere = 1; 

		/*
		* @a SphereSettings denote the definition of sphere.
		* Not thread-safe.
		*/
		class SphereSettings
		{
		public:
			
			inline
			static 
			SphereSettings&
			instance()
			{
				static SphereSettings _instance;
				return _instance;
			}


			inline
			const real_t&
			earth_radius() const
			{
				return d_earth_radius;
			}


			inline
			const real_t&
			pi() const
			{
				return d_PI;
			}


			inline
			const real_t&
			dot_product_distance_accuracy_tolerance() const
			{
				return d_dot_product_distance_accuracy_tolerance;
			}


			inline
			const real_t
			accuracy_tolerance_of_distance_on_sphere_surface() const
			{
				return acos(1 - d_dot_product_distance_accuracy_tolerance) * earth_radius();
			}


			inline
			void
			set_earth_radius(
					const real_t &new_radius)
			{
				d_earth_radius = new_radius;
			}


			inline
			void
			set_pi(
					const real_t &new_pi)
			{
				d_PI = new_pi;
			}


			inline
			void
			set_dot_product_distance_accuracy_tolerance(
					const real_t &new__tolerance)
			{
				d_dot_product_distance_accuracy_tolerance = new__tolerance;
			}

		protected:
			SphereSettings() :
				d_earth_radius(DefaultEarthRadius),
				d_PI(PI),
				d_dot_product_distance_accuracy_tolerance(DotProductDistanceAccuracyTolerance)
			{ }
			
			SphereSettings(
					const SphereSettings&);
			
			SphereSettings&
			operator=(
					const SphereSettings&);
			
			real_t d_earth_radius;
			real_t d_PI;
			real_t d_dot_product_distance_accuracy_tolerance;
		};
	}
}

#endif  // GPLATES_MATHS_SPHERESETTINGS_H
