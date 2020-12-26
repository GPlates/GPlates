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

#include <boost/optional.hpp>

#include "ScalarCoverageDeformation.h"

#include "ReconstructionGeometryUtils.h"
#include "TopologyReconstruct.h"
#include "TopologyReconstructedFeatureGeometry.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::ScalarCoverageTimeSpan(
		const initial_scalar_coverage_type &initial_scalar_coverage) :
	// We have no deformation (no geometry time span) and hence no scalars can be evolved,
	// so they're all non-evolved...
	d_non_evolved_scalar_coverage(initial_scalar_coverage),
	d_scalar_import_time(0.0)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!initial_scalar_coverage.empty(),
			GPLATES_ASSERTION_SOURCE);

	// Get the number of scalar values from the first scalar type.
	// Next we'll ensure the number of scalar values in the other scalar types matches.
	d_num_all_scalar_values = initial_scalar_coverage.begin()->second.size();
	for (const auto &scalar_coverage_item : initial_scalar_coverage)
	{
		const std::vector<double> &scalar_values = scalar_coverage_item.second;

		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				scalar_values.size() == d_num_all_scalar_values,
				GPLATES_ASSERTION_SOURCE);
	}
}


GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::ScalarCoverageTimeSpan(
		const initial_scalar_coverage_type &initial_scalar_coverage,
		TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span) :
	d_geometry_time_span(geometry_time_span),
	d_scalar_import_time(geometry_time_span->get_geometry_import_time()),
	d_num_all_scalar_values(geometry_time_span->get_num_all_geometry_points())
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!initial_scalar_coverage.empty(),
			GPLATES_ASSERTION_SOURCE);

	// Get the number of original scalar values from the first scalar type.
	// Next we'll ensure the number of original scalar values in the other scalar types matches.
	//
	// Note: This might be less than the actual number of scalar values if the geometry in the
	// time span was tessellated (the actual number is 'd_num_all_scalar_values'). If so then we'll
	// generate the missing scalar values (below) by interpolating the original scalar values.
	const unsigned int num_original_scalar_values = initial_scalar_coverage.begin()->second.size();
	for (const auto &scalar_coverage_item : initial_scalar_coverage)
	{
		const std::vector<double> &original_scalar_values = scalar_coverage_item.second;

		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				original_scalar_values.size() == num_original_scalar_values,
				GPLATES_ASSERTION_SOURCE);
	}

	// Add the actual (ie, possibly tessellated) scalar values of each scalar type as either evolved or
	// non-evolved (depending on whether the scalar type is an evolved type or not).
	ScalarCoverageEvolution::InitialEvolvedScalarCoverage initial_evolved_scalar_coverage(d_num_all_scalar_values);
	for (const auto &scalar_coverage_item : initial_scalar_coverage)
	{
		const scalar_type_type &scalar_type = scalar_coverage_item.first;
		const std::vector<double> &initial_scalar_values = scalar_coverage_item.second;

		// The import scalar values might be interpolated versions of the initial scalar values if
		// the geometry in the time span was tessellated.
		const std::vector<double> import_scalar_values =
				create_import_scalar_values(initial_scalar_values, geometry_time_span);

		// The actual number of scalar values (per scalar type).
		// There might be more than the original scalar values if the geometry in the time span was tessellated.
		//
		// We're repeating this assignment a bit, but the number of scalar values shouldn't change.
		d_num_all_scalar_values = import_scalar_values.size();

		// Is the current scalar type an evolved type?
		if (boost::optional<ScalarCoverageEvolution::EvolvedScalarType> evolved_scalar_type =
			ScalarCoverageEvolution::is_evolved_scalar_type(scalar_type))
		{
			initial_evolved_scalar_coverage.add_initial_scalar_values(evolved_scalar_type.get(), import_scalar_values);
		}
		else
		{
			d_non_evolved_scalar_coverage[scalar_type] = import_scalar_values;
		}
	}

	// Create and initialise a time span for the evolved scalar coverage.
	//
	// Note: We do this even if there are no initial scalar values corresponding to any *evolved* scalar types.
	//       This is because we can still query evolved scalar types assuming default initial values.
	//
	// Evolve scalar values over time (starting with the import scalar values) and
	// store them in the returned scalar coverage time span.
	d_evolved_scalar_coverage_time_span = ScalarCoverageEvolution::create(
			initial_evolved_scalar_coverage,
			d_scalar_import_time,
			geometry_time_span);
}


bool
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::is_valid(
		const double &reconstruction_time) const
{
	// If we have a geometry time span then just delegate to it since it determines whether all geometry
	// points (and hence scalar values) have been deactivated at the specified reconstruction time.
	if (d_geometry_time_span)
	{
		return d_geometry_time_span.get()->is_valid(reconstruction_time);
	}

	// There's no geometry time span so the (non-evolved) scalar values are never deactivated.
	return true;
}


boost::optional<GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::ScalarCoverage>
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::get_scalar_coverage(
		const double &reconstruction_time) const
{
	if (!is_valid(reconstruction_time))
	{
		// The geometry/scalars is not valid/active at the reconstruction time.
		return boost::none;
	}

	return ScalarCoverage(this);
}


bool
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::get_scalar_values(
		const scalar_type_type &scalar_type,
		const double &reconstruction_time,
		std::vector<double> &scalar_values) const
{
	std::vector<double> all_scalar_values;
	std::vector<bool> all_scalar_values_are_active;
	if (!get_all_scalar_values(scalar_type, reconstruction_time, all_scalar_values, all_scalar_values_are_active))
	{
		return false;
	}

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_num_all_scalar_values == all_scalar_values.size() &&
				d_num_all_scalar_values == all_scalar_values_are_active.size(),
			GPLATES_ASSERTION_SOURCE);

	// Return active scalar values to the caller.
	for (unsigned int n = 0; n < d_num_all_scalar_values; ++n)
	{
		if (all_scalar_values_are_active[n])
		{
			scalar_values.push_back(all_scalar_values[n]);
		}
	}

	return true;
}


bool
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::get_all_scalar_values(
		const scalar_type_type &scalar_type,
		const double &reconstruction_time,
		std::vector<double> &scalar_values,
		std::vector<bool> &scalar_values_are_active) const
{
	if (!is_valid(reconstruction_time))
	{
		// The geometry/scalars is not valid/active at the reconstruction time.
		return false;
	}

	//
	// First look in the *non-evolved* scalar coverage.
	//

	auto non_evolved_iter = d_non_evolved_scalar_coverage.find(scalar_type);
	if (non_evolved_iter != d_non_evolved_scalar_coverage.end())
	{
		const std::vector<double> &non_evolved_scalar_values = non_evolved_iter->second;

		// Copy the non-evolved scalar values.
		scalar_values.assign(non_evolved_scalar_values.begin(), non_evolved_scalar_values.end());

		// If we have a geometry time span then it means the geometry was reconstructed by topologies
		// and hence the geometry points can be deactivated over time. So we also need to deactivate
		// the associated scalar values.
		if (d_geometry_time_span)
		{
			if (!d_geometry_time_span.get()->get_points_are_active(reconstruction_time, scalar_values_are_active))
			{
				// Shouldn't really get here since we've already passed 'is_valid()' above.
				return false;
			}
		}
		else
		{
			// All scalar values are active (because they can never get deactivated).
			scalar_values_are_active.resize(d_num_all_scalar_values, true);
		}

		return true;
	}

	//
	// Next look in the *evolved* scalar coverage.
	//

	if (d_evolved_scalar_coverage_time_span)
	{
		if (boost::optional<ScalarCoverageEvolution::EvolvedScalarType> evolved_scalar_type =
			ScalarCoverageEvolution::is_evolved_scalar_type(scalar_type))
		{
			// Note that, unlike non-evolved scalar values, the effects of point deactivation from
			// the associated geometry time span have already been taken into account here
			// (because the geometry time span both evolved and deactivated scalar values before they
			// got stored in the evolved scalar coverage time span).
			d_evolved_scalar_coverage_time_span.get()->get_scalar_values(
					evolved_scalar_type.get(),
					reconstruction_time,
					scalar_values,
					scalar_values_are_active);

			return true;
		}
	}

	// The specified scalar type is not contained in this scalar coverage.
	return false;
}


bool
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::get_are_scalar_values_active(
		const double &reconstruction_time,
		std::vector<bool> &scalar_values_are_active) const
{
	if (!is_valid(reconstruction_time))
	{
		// The geometry/scalars is not valid/active at the reconstruction time.
		return false;
	}

	// If we have a geometry time span then it means the geometry was reconstructed by topologies
	// and hence the geometry points can be deactivated over time. So we also need to deactivate
	// the associated scalar values.
	if (d_geometry_time_span)
	{
		if (!d_geometry_time_span.get()->get_points_are_active(reconstruction_time, scalar_values_are_active))
		{
			// Shouldn't really get here since we've already passed 'is_valid()' above.
			return false;
		}
	}
	else
	{
		// All scalar values are active (because they can never get deactivated).
		scalar_values_are_active.resize(get_num_all_scalar_values(), true);
	}

	return true;
}


std::vector<double>
GPlatesAppLogic::ScalarCoverageDeformation::ScalarCoverageTimeSpan::create_import_scalar_values(
		const std::vector<double> &scalar_values,
		TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span)
{
	//
	// Our domain geometry is being reconstructed using topologies so it might have been tessellated
	// in which case we'd need to introduce new scalar values to map to new tessellated geometry points.
	//

	// Get the information regarding tessellation of the original geometry points.
	const TopologyReconstruct::GeometryTimeSpan::interpolate_original_points_seq_type &
			interpolate_original_points = geometry_time_span->get_interpolate_original_points();
	const unsigned int num_interpolated_scalar_values = interpolate_original_points.size();

	// Number of original scalar values.
	const unsigned int num_scalar_values = scalar_values.size();

	// The potentially interpolated scalar values - we might be adding new interpolated
	// scalar values if the original domain geometry got tessellated.
	std::vector<double> interpolated_scalar_values;
	interpolated_scalar_values.reserve(num_interpolated_scalar_values);

	for (unsigned int n = 0; n < num_interpolated_scalar_values; ++n)
	{
		const TopologyReconstruct::GeometryTimeSpan::InterpolateOriginalPoints &
				interpolate_original_point = interpolate_original_points[n];

		// Indices should not equal (or exceed) the number of our original scalar values.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				interpolate_original_point.first_point_index < num_scalar_values &&
					interpolate_original_point.second_point_index < num_scalar_values,
				GPLATES_ASSERTION_SOURCE);

		// Interpolate the current scalar value between two original scalar values.
		// If the current scalar value maps to an original (non-tessellated) geometry point
		// then the interpolate ratio will be either 0.0 or 1.0.
		const double interpolated_scalar_value =
				(1.0 - interpolate_original_point.interpolate_ratio) *
						scalar_values[interpolate_original_point.first_point_index] +
				interpolate_original_point.interpolate_ratio *
						scalar_values[interpolate_original_point.second_point_index];

		interpolated_scalar_values.push_back(interpolated_scalar_value);
	}

	return interpolated_scalar_values;
}
