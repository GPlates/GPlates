/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include "PropertyValueFinder.h"

#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPiecewiseAggregation.h"


void
GPlatesFeatureVisitors::Implementation::visit_gpml_constant_value(
		GPlatesModel::ConstFeatureVisitor::gpml_constant_value_type &gpml_constant_value,
		GPlatesModel::ConstFeatureVisitor &visitor)
{
	gpml_constant_value.get_value()->accept_visitor(visitor);
}


void
GPlatesFeatureVisitors::Implementation::visit_gpml_constant_value(
		GPlatesModel::FeatureVisitor::gpml_constant_value_type &gpml_constant_value,
		GPlatesModel::FeatureVisitor &visitor)
{
	GPlatesModel::PropertyValue::non_null_ptr_type property_value =
			gpml_constant_value.get_value()->clone();
	property_value->accept_visitor(visitor);
	gpml_constant_value.set_value(property_value);
}


void
GPlatesFeatureVisitors::Implementation::visit_gpml_irregular_sampling_at_reconstruction_time(
		GPlatesModel::ConstFeatureVisitor::gpml_irregular_sampling_type &gpml_irregular_sampling,
		GPlatesModel::ConstFeatureVisitor &visitor,
		const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time)
{
	std::vector< GPlatesPropertyValues::GpmlTimeSample >::const_iterator 
		iter = gpml_irregular_sampling.get_time_samples().begin(),
		end = gpml_irregular_sampling.get_time_samples().end();
	for ( ; iter != end; ++iter)
	{
		// If time of time sample matches our reconstruction time then visit.
		//
		// FIXME: Should really visit the two closest samples and interpolate them, but
		// that's hard to do when the property being visited is a template type.
		if (reconstruction_time.is_coincident_with(iter->get_valid_time()->get_time_position()))
		{
			iter->get_value()->accept_visitor(visitor);
			return;
		}
	}
}


void
GPlatesFeatureVisitors::Implementation::visit_gpml_irregular_sampling_at_reconstruction_time(
		GPlatesModel::FeatureVisitor::gpml_irregular_sampling_type &gpml_irregular_sampling,
		GPlatesModel::FeatureVisitor &visitor,
		const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time)
{
	std::vector< GPlatesPropertyValues::GpmlTimeSample > time_samples =
			gpml_irregular_sampling.get_time_samples();

	std::vector< GPlatesPropertyValues::GpmlTimeSample >::iterator 
		iter = time_samples.begin(),
		end = time_samples.end();
	for ( ; iter != end; ++iter)
	{
		// If time of time sample matches our reconstruction time then visit.
		//
		// FIXME: Should really visit the two closest samples and interpolate them, but
		// that's hard to do when the property being visited is a template type.
		if (reconstruction_time.is_coincident_with(iter->get_valid_time()->get_time_position()))
		{
			iter->get_value()->accept_visitor(visitor);
			return;
		}
	}

	gpml_irregular_sampling.set_time_samples(time_samples);
}


void
GPlatesFeatureVisitors::Implementation::visit_gpml_piecewise_aggregation_at_reconstruction_time(
		GPlatesModel::ConstFeatureVisitor::gpml_piecewise_aggregation_type &gpml_piecewise_aggregation,
		GPlatesModel::ConstFeatureVisitor &visitor,
		const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time)
{
	std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator iter =
		gpml_piecewise_aggregation.get_time_windows().begin();
	std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator end =
		gpml_piecewise_aggregation.get_time_windows().end();
	for ( ; iter != end; ++iter)
	{
		// If the time window covers our reconstruction time then visit.
		if (iter->get_valid_time()->contains(reconstruction_time))
		{
			iter->get_time_dependent_value()->accept_visitor(visitor);

			// Break out of loop since time windows should be non-overlapping.
			// If we don't break out we might visit the property value twice if it
			// falls on the boundary between two time periods (due to numerical tolerance).
			return;
		}
	}
}


void
GPlatesFeatureVisitors::Implementation::visit_gpml_piecewise_aggregation_at_reconstruction_time(
		GPlatesModel::FeatureVisitor::gpml_piecewise_aggregation_type &gpml_piecewise_aggregation,
		GPlatesModel::FeatureVisitor &visitor,
		const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time)
{
	std::vector<GPlatesPropertyValues::GpmlTimeWindow> time_windows =
			gpml_piecewise_aggregation.get_time_windows();

	std::vector<GPlatesPropertyValues::GpmlTimeWindow>::iterator iter =
		time_windows.begin();
	std::vector<GPlatesPropertyValues::GpmlTimeWindow>::iterator end =
		time_windows.end();
	for ( ; iter != end; ++iter)
	{
		// If the time window covers our reconstruction time then visit.
		if (iter->get_valid_time()->contains(reconstruction_time))
		{
			iter->get_time_dependent_value()->accept_visitor(visitor);

			// Break out of loop since time windows should be non-overlapping.
			// If we don't break out we might visit the property value twice if it
			// falls on the boundary between two time periods (due to numerical tolerance).
			return;
		}
	}

	gpml_piecewise_aggregation.set_time_windows(time_windows);
}
