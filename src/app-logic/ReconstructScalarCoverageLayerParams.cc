/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#include <algorithm>
#include <cmath>
#include <limits>

#include "ReconstructScalarCoverageLayerParams.h"


void
GPlatesAppLogic::ReconstructScalarCoverageLayerParams::set_reconstruct_scalar_coverage_params(
		const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params)
{
	if (d_reconstruct_scalar_coverage_params == reconstruct_scalar_coverage_params)
	{
		return;
	}

	d_reconstruct_scalar_coverage_params = reconstruct_scalar_coverage_params;

	Q_EMIT modified_reconstruct_scalar_coverage_params(*this);
	emit_modified();
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerParams::set_scalar_type(
		GPlatesPropertyValues::ValueObjectType scalar_type)
{
	// Is the selected scalar type one of the available scalar types in the scalar coverage features?
	// If not, then change the scalar type to be the first of the available scalar types.
	// This can happen if the scalar coverage features are reloaded from file and no longer contain
	// the currently selected scalar type.
	std::vector<GPlatesPropertyValues::ValueObjectType> scalar_types;
	d_layer_proxy->get_scalar_types(scalar_types);
	if (!scalar_types.empty() &&
		std::find(scalar_types.begin(), scalar_types.end(), scalar_type) == scalar_types.end())
	{
		scalar_type = scalar_types.front();
	}

	// Set the current scalar type.
	d_layer_proxy->set_current_scalar_type(scalar_type);

	// See if we are up-to-date with respect to the layer proxy.
	// We could be out-of-date if setting the scalar type above changed the scalar type, or
	// if something else changed in the layer proxy (eg, list of scalar types, the scalar statistics, etc).
	if (!d_layer_proxy->get_subject_token().is_observer_up_to_date(d_layer_proxy_observer_token))
	{
		d_layer_proxy->get_subject_token().update_observer(d_layer_proxy_observer_token);

		emit_modified();
	}
}


const GPlatesPropertyValues::ValueObjectType &
GPlatesAppLogic::ReconstructScalarCoverageLayerParams::get_scalar_type() const
{
	// Update the current scalar type if the scalar coverage feature has changed.
	//
	// We need this since we don't get notified *directly* of changes in the Reconstruct layer
	// that our Reconstruct Scalar Coverage layer is connected to. For example, if the
	// scalar coverage features are reloaded from file they might no longer contain
	// the currently selected scalar type.
	//
	// This update also ensures that the we will have a valid current-scalar-type if
	// we're currently being called by ReconstructScalarCoverageVisualLayerParams
	// (to re-map its colour palette according to the scalar coverage statistics).
	const_cast<ReconstructScalarCoverageLayerParams *>(this)->update();

	return d_layer_proxy->get_current_scalar_type();
}


boost::optional<GPlatesPropertyValues::ScalarCoverageStatistics>
GPlatesAppLogic::ReconstructScalarCoverageLayerParams::get_scalar_statistics(
		const GPlatesPropertyValues::ValueObjectType &scalar_type) const
{
	// Scalar statistics.
	//
	// mean = M = sum(Xi) / N
	// std_dev  = sqrt[(sum(Xi^2) / N - M^2]
	//
	// ...where N is total number of scalar samples.
	//
	unsigned int total_num_scalars = 0;
	double total_scalar_min = (std::numeric_limits<double>::max)();
	double total_scalar_max = (std::numeric_limits<double>::min)();
	double total_scalar_sum = 0;
	double total_scalar_sum_squares = 0;

	// Get the time history of scalar values for the specified scalar type.
	// We don't want to calculate statistics just on the original scalar values because they might
	// be constant (such as initial crustal thickness at geometry import time).
	typedef ReconstructScalarCoverageLayerProxy::ReconstructedScalarCoverageTimeSpan time_span_type;
	std::vector<time_span_type> time_spans;
	d_layer_proxy->get_reconstructed_scalar_coverage_time_spans(time_spans, scalar_type);

	std::vector<time_span_type>::const_iterator time_spans_iter = time_spans.begin();
	std::vector<time_span_type>::const_iterator time_spans_end = time_spans.end();
	for ( ; time_spans_iter != time_spans_end; ++time_spans_iter)
	{
		const time_span_type &time_span = *time_spans_iter;

		const time_span_type::scalar_coverage_time_span_seq_type &scalar_coverage_time_spans =
				time_span.get_scalar_coverage_time_spans();
		time_span_type::scalar_coverage_time_span_seq_type::const_iterator scalar_coverage_time_spans_iter =
				scalar_coverage_time_spans.begin();
		time_span_type::scalar_coverage_time_span_seq_type::const_iterator scalar_coverage_time_spans_end =
				scalar_coverage_time_spans.end();
		for ( ; scalar_coverage_time_spans_iter != scalar_coverage_time_spans_end; ++scalar_coverage_time_spans_iter)
		{
			const time_span_type::ScalarCoverageTimeSpan &scalar_coverage_time_span = *scalar_coverage_time_spans_iter;

			// See if we need to look at a history of scalar values or just present day.
			if (scalar_coverage_time_span.get_geometry_time_span())
			{
				// Look at scalar values throughout the history of reconstructed scalar values since
				// topologically reconstructed (hence scalars potentially evolved due to deformation).
				const TimeSpanUtils::TimeRange time_range =
						scalar_coverage_time_span.get_scalar_coverage_time_span()->get_time_range();

				// The number of *active* and *inactive* scalar points is always constant.
				const unsigned int num_scalar_values =
						scalar_coverage_time_span.get_scalar_coverage_time_span()->get_num_all_scalar_values();

				// Some points/scalars may get deactivated sooner than others so track statistics of individual points.
				std::vector<unsigned int> num_scalars(num_scalar_values, 0);
				std::vector<double> scalar_sums(num_scalar_values, 0.0);
				std::vector<double> scalar_sum_squares(num_scalar_values, 0.0);

				// Iterate over the time history of scalar values.
				const unsigned int num_time_slots = time_range.get_num_time_slots();
				for (unsigned int time_slot = 0; time_slot < num_time_slots; ++time_slot)
				{
					const double time = time_range.get_time(time_slot);

					// Get *active* and *inactive* points/scalars.
					std::vector< boost::optional<double> > all_scalar_values;
					if (scalar_coverage_time_span.get_scalar_coverage_time_span()->get_all_scalar_values(time, all_scalar_values))
					{
						for (unsigned int scalar_value_index = 0; scalar_value_index < num_scalar_values; ++scalar_value_index)
						{
							const boost::optional<double> &scalar_opt = all_scalar_values[scalar_value_index];
							if (scalar_opt)
							{
								const double scalar = scalar_opt.get();

								scalar_sums[scalar_value_index] += scalar;
								scalar_sum_squares[scalar_value_index] += scalar * scalar;
								++num_scalars[scalar_value_index];

								if (scalar < total_scalar_min)
								{
									total_scalar_min = scalar;
								}
								if (scalar > total_scalar_max)
								{
									total_scalar_max = scalar;
								}
							}
						}
					}
				}

				for (unsigned int scalar_value_index = 0; scalar_value_index < num_scalar_values; ++scalar_value_index)
				{
					// There should be at least one active scalar value for each point in the entire time span history.
					// But we'll still check just in case.
					const unsigned int num_current_scalars = num_scalars[scalar_value_index];
					if (num_current_scalars != 0)
					{
						const double inv_num_current_scalars = 1.0 / num_current_scalars;

						// Scale back the statistic sums such that the current point only counts as one scalar value
						// instead of 'num_current_scalars'. This way all points are treated equally - just because
						// one point gets deactivated sooner than another doesn't mean it contributes less to the
						// total mean and standard deviation.
						total_scalar_sum += inv_num_current_scalars * scalar_sums[scalar_value_index];
						total_scalar_sum_squares += inv_num_current_scalars * scalar_sum_squares[scalar_value_index];
						++total_num_scalars;
					}
				}
			}
			else // not topologically reconstructed (hence scalars not evolved due to deformation)...
			{
				// Just need to look at scalar values at present day.
				// The scalar values don't change with time so it actually doesn't matter which time we choose.
				std::vector<double> scalar_values;
				if (scalar_coverage_time_span.get_scalar_coverage_time_span()->
						get_scalar_values(0.0/*reconstruction_time*/, scalar_values))
				{ // Should always return true since no point deactivation because not topological reconstructed.
					total_num_scalars += scalar_values.size();

					std::vector<double>::const_iterator scalar_values_iter = scalar_values.begin();
					std::vector<double>::const_iterator scalar_values_end = scalar_values.end();
					for ( ; scalar_values_iter != scalar_values_end; ++scalar_values_iter)
					{
						const double scalar = *scalar_values_iter;
						total_scalar_sum += scalar;
						total_scalar_sum_squares += scalar * scalar;

						if (scalar < total_scalar_min)
						{
							total_scalar_min = scalar;
						}
						if (scalar > total_scalar_max)
						{
							total_scalar_max = scalar;
						}
					}
				}
			}
		}
	}

	if (total_num_scalars == 0)
	{
		// There were no scalar coverages of the requested scalar type.
		return boost::none;
	}

	const double total_scalar_mean = total_scalar_sum / total_num_scalars;

	const double total_scalar_variance = total_scalar_sum_squares / total_num_scalars -
			total_scalar_mean * total_scalar_mean;
	// Protect 'sqrt' in case variance is slightly negative due to numerical precision.
	const double total_scalar_standard_deviation = (total_scalar_variance > 0)
			? std::sqrt(total_scalar_variance)
			: 0;

	return GPlatesPropertyValues::ScalarCoverageStatistics(
			total_scalar_min,
			total_scalar_max,
			total_scalar_mean,
			total_scalar_standard_deviation);
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerParams::update()
{
	if (d_layer_proxy->get_subject_token().is_observer_up_to_date(d_layer_proxy_observer_token))
	{
		return;
	}

	// We're not up-to-date with respect to the layer proxy...
	//
	// We need this since we don't get notified *directly* of changes in the Reconstruct layer
	// that our Reconstruct Scalar Coverage layer is connected to. For example, if the
	// scalar coverage features are reloaded from file they might no longer contain
	// the currently selected scalar type and so we'll have to change it.

	// Is the selected scalar type one of the available scalar types in the scalar coverage features?
	// If not, then change the scalar type to be the first of the available scalar types.
	// This can happen if the scalar coverage features are reloaded from file and no longer contain
	// the currently selected scalar type.
	std::vector<GPlatesPropertyValues::ValueObjectType> scalar_types;
	d_layer_proxy->get_scalar_types(scalar_types);
	const GPlatesPropertyValues::ValueObjectType &current_scalar_type = d_layer_proxy->get_current_scalar_type();
	if (!scalar_types.empty() &&
		std::find(scalar_types.begin(), scalar_types.end(), current_scalar_type) == scalar_types.end())
	{
		// Set the current scalar type using the default scalar type index of zero.
		d_layer_proxy->set_current_scalar_type(scalar_types.front());
	}

	// We are now up-to-date with respect to the layer proxy.
	//
	// Note that we do this after setting the scalar type on the layer proxy since that just
	// invalidates its subject token again and hence we'll (incorrectly) always need updating.
	d_layer_proxy->get_subject_token().update_observer(d_layer_proxy_observer_token);

	// We always emit modified signal if we were out-of-date with Reconstruct layer since
	// it could have changed the list of scalar types, the scalar statistics, etc.
	emit_modified();
}
