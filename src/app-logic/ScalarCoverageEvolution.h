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

#include "DeformationStrain.h"
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
					std::vector<double> &,                  // per-point input/output scalars
					const std::vector<DeformationStrain> &, // initial per-point input deformation strain rates
					const std::vector<DeformationStrain> &, // final per-point input deformation strain rates
					const double &,                         // initial time
					const double &)>                        // final time
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
		 * Evolves the crustal thickness values in @a input_output_crustal_thickness.
		 *
		 * Each values in @a input_output_crustal_thickness is replaced with its updated value.
		 *
		 * Throws exception if the sizes of the input arrays do not match.
		 */
		void
		crustal_thinning(
				std::vector<double> &input_output_crustal_thickness,
				const std::vector<DeformationStrain> &initial_deformation_strain_rates,
				const std::vector<DeformationStrain> &final_deformation_strain_rates,
				const double &initial_time,
				const double &final_time);
	}
}

#endif // GPLATES_APP_LOGIC_SCALARCOVERAGEEVOLUTION_H
