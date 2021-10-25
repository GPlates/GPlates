/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_SCALARCOVERAGESTATISTICS_H
#define GPLATES_PROPERTYVALUES_SCALARCOVERAGESTATISTICS_H

#include <boost/optional.hpp>


namespace GPlatesPropertyValues
{
	/**
	 * Contains statistics about one or more scalar coverages (geometries with per-point scalar values).
	 */
	struct ScalarCoverageStatistics
	{
		ScalarCoverageStatistics(
				const double &minimum_,
				const double &maximum_,
				const double &mean_,
				const double &standard_deviation_) :
			minimum(minimum_),
			maximum(maximum_),
			mean(mean_),
			standard_deviation(standard_deviation_)
		{  }

		double minimum;
		double maximum;
		double mean;
		double standard_deviation;
	};
}

#endif // GPLATES_PROPERTYVALUES_SCALARCOVERAGESTATISTICS_H
