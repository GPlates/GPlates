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

#ifndef GPLATES_APP_LOGIC_SCALARCOVERAGEEVOLUTION_H
#define GPLATES_APP_LOGIC_SCALARCOVERAGEEVOLUTION_H

#include <vector>
#include <boost/function.hpp>
#include <boost/optional.hpp>

#include "GeometryDeformation.h"
#include "ReconstructScalarCoverageParams.h"

#include "property-values/ValueObjectType.h"


namespace GPlatesAppLogic
{
	/**
	 * Convenience typedef for a function that evolves a sequence of (per-point) scalar values
	 * from one time to another.
	 */
	typedef boost::function<
			void (
					std::vector<double> &,                                    // per-point output scalars
					const std::vector<double> &,                              // per-point input scalars
					const std::vector<GeometryDeformation::DeformationInfo> &,// per-point input deform info
					const double &,                                           // initial time
					const double &)>                                          // final time
							scalar_evolution_function_type;


	/**
	 * Returns the function used to evolve scalars over time for the specified scalar type.
	 *
	 * Returns none if scalar values don't change over time (ie, if no evolution needed).
	 */
	boost::optional<scalar_evolution_function_type>
	get_scalar_evolution_function(
			const GPlatesPropertyValues::ValueObjectType &scalar_type,
			const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params);


	/**
	 * This namespace contains functions that evolve the scalar values (in a scalar coverage) over a time interval.
	 *
	 * For example a scalar coverage containing crustal thickness scalars will get evolved using
	 * the @a crustal_thinning function.
	 */
	namespace ScalarCoverageEvolution
	{
		/**
		 * Evolves the crustal thickness from @a input_crustal_thickness to @a output_crustal_thickness.
		 *
		 * Output values are appended to @a output_crustal_thickness.
		 *
		 * Throws exception if the sizes of the input arrays do not match.
		 */
		void
		crustal_thinning(
			std::vector<double> &output_crustal_thickness,
			const std::vector<double> &input_crustal_thickness,
			const std::vector<GeometryDeformation::DeformationInfo> &deformation_info,
			const double &initial_time,
			const double &final_time,
			const double &oceanic_density,
			const double &continental_density);
	}
}

#endif // GPLATES_APP_LOGIC_SCALARCOVERAGEEVOLUTION_H
