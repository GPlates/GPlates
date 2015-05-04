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

#include <boost/bind.hpp>

#include "ScalarCoverageEvolution.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


boost::optional<GPlatesAppLogic::scalar_evolution_function_type>
GPlatesAppLogic::get_scalar_evolution_function(
		const GPlatesPropertyValues::ValueObjectType &scalar_type,
		const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params)
{
	static const GPlatesPropertyValues::ValueObjectType CRUSTAL_THICKNESS =
			GPlatesPropertyValues::ValueObjectType::create_gpml("CrustalThickness");

	if (scalar_type == CRUSTAL_THICKNESS)
	{
		//
		// TODO: Get these quantities from ReconstructScalarCoverageParams (from layer GUI).
		//
		const double oceanic_density = 1;
		const double continental_density = 1;

		return scalar_evolution_function_type(
				boost::bind(
						&ScalarCoverageEvolution::crustal_thinning,
						_1, _2, _3, _4, _5,
						oceanic_density,
						continental_density));
	}

	// No evolution needed - scalar values don't change over time.
	return boost::none;
}


void
GPlatesAppLogic::ScalarCoverageEvolution::crustal_thinning(
	 std::vector<double> &output_crustal_thickness,
	 const std::vector<double> &input_crustal_thickness,
	 const std::vector<GeometryDeformation::DeformationInfo> &deformation_info,
	 const double &initial_time,
	 const double &final_time,
	 const double &oceanic_density,
	 const double &continental_density)
{
	// Input array sizes should match.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			input_crustal_thickness.size() == deformation_info.size(),
			GPLATES_ASSERTION_SOURCE);

	// TODO: Implement crustal thinning.
	//
	// Currently just copies input to output.
	output_crustal_thickness.insert(
			output_crustal_thickness.end(),
			input_crustal_thickness.begin(),
			input_crustal_thickness.end());
}
