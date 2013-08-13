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
			return d_closeness_inclusion_threshold;
		}

		const double &
		latitude_exclusion_threshold() const
		{
			return d_latitude_exclusion_threshold;
		}

	private:
		PointOnSphere d_test_point;
		double d_closeness_inclusion_threshold;
		double d_latitude_exclusion_threshold;
	};

}

#endif  // GPLATES_MATHS_PROXIMITYCRITERIA_H
