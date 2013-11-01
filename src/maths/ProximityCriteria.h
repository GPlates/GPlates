/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
 
#ifndef GPLATES_MATHS_PROXIMITYCRITERIA_H
#define GPLATES_MATHS_PROXIMITYCRITERIA_H

#include "AngularExtent.h"
#include "PointOnSphere.h"


namespace GPlatesMaths
{
	/**
	 * This class contains the parameters for the various proximity criteria.
	 */
	class ProximityCriteria
	{
	public:
		ProximityCriteria(
				const PointOnSphere &test_point_,
				const double &closeness_inclusion_threshold_);

		const PointOnSphere &
		test_point() const
		{
			return d_test_point;
		}

		const double &
		closeness_inclusion_threshold() const
		{
			return d_closeness_angular_extent_threshold.get_cosine();
		}

		const double &
		latitude_exclusion_threshold() const
		{
			return d_closeness_angular_extent_threshold.get_sine();
		}

		/**
		 * Get the closeness inclusion threshold (cosine) and latitude exclusion threshold (sine)
		 * in one angular extent object.
		 */
		const AngularExtent &
		closeness_angular_extent_threshold() const
		{
			return d_closeness_angular_extent_threshold;
		}

	private:
		PointOnSphere d_test_point;
		AngularExtent d_closeness_angular_extent_threshold;
	};

}

#endif  // GPLATES_MATHS_PROXIMITYCRITERIA_H
