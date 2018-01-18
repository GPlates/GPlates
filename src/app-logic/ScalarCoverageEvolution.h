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

#include "DeformationStrainRate.h"
#include "ReconstructScalarCoverageParams.h"

#include "property-values/ValueObjectType.h"


namespace GPlatesAppLogic
{
	/**
	 * Convenience typedef for a function that evolves a sequence of (per-point) scalar values from one time to another.
	 *
	 * Note that 'boost::optional' is used for each point's scalar, strain rate and strain.
	 * This represents whether the associated point is active. Points can become inactive over time (active->inactive) but
	 * do not get re-activated (inactive->active). So if the initial strain rate is inactive then so should the final strain rate.
	 * Also the active state of the initial scalar value should match that of the initial deformation strain rate
	 * (because they represent the same point). If the initial scalar value is inactive then it just remains inactive.
	 * And if the initial scalar value is active then it becomes inactive if the final strain rate is inactive, otherwise
	 * both (initial and final) strain rates are active and the scalar value is evolved from its initial value to its final value.
	 * This ensures the active state of the final scalar values matches that of the final deformation strain rate
	 * (which in turn comes from the active state of the associated domain geometry point).
	 */
	typedef boost::function<
			void (
					std::vector< boost::optional<double> > &,                      // initial->final per-point scalars
					const std::vector< boost::optional<DeformationStrainRate> > &, // initial per-point input deformation strain rates
					const std::vector< boost::optional<DeformationStrainRate> > &, // final per-point input deformation strain rates
					const double &,                                                // initial time
					const double &)>                                               // final time
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
		enum CrustalThicknessType
		{
			// This can be absolute thickness (eg, in kms), or
			// a thickness ratio such as T/Ti (where T/Ti is current/initial thickness)...
			CRUSTAL_THICKNESS,
			// Stretching (beta) factor where 'beta = Ti/T' ...
			CRUSTAL_STRETCHING_FACTOR,
			// Thinning (gamma) factor where 'gamma = (1 - T/Ti)' ...
			CRUSTAL_THINNING_FACTOR
		};

		/**
		 * Evolves the crustal thickness values in @a input_output_crustal_thickness.
		 *
		 * Each values in @a input_output_crustal_thickness is replaced with its updated value.
		 *
		 * Other quantities besides crustal thickness (or thickness ratio) in @a input_output_crustal_thickness
		 * that are related to crustal thickness can be specified with @a crustal_thickness_type.
		 *
		 * Throws exception if the sizes of the input arrays do not match.
		 */
		void
		crustal_thinning(
				std::vector< boost::optional<double> > &input_output_crustal_thickness,
				const std::vector< boost::optional<DeformationStrainRate> > &initial_deformation_strain_rates,
				const std::vector< boost::optional<DeformationStrainRate> > &final_deformation_strain_rates,
				const double &initial_time,
				const double &final_time,
				CrustalThicknessType crustal_thickness_type = CRUSTAL_THICKNESS);
	}
}

#endif // GPLATES_APP_LOGIC_SCALARCOVERAGEEVOLUTION_H
