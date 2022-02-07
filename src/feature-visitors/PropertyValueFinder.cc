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

#include <vector>
#include <boost/optional.hpp>

#include "PropertyValueFinder.h"

#include "maths/MathsUtils.h"

#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/XsDouble.h"


namespace GPlatesFeatureVisitors
{
	namespace
	{
		/**
		 * Interpolation of an irregularly-sampled time-dependent property between the two time
		 * samples that surround a specific time instant.
		 *
		 * Not all property value types can be interpolated (eg, it makes no sense to interpolate
		 * a string) and so this only applies to certain types.
		 */
		class InterpolateIrregularSamplingVisitor :
				public GPlatesModel::ConstFeatureVisitor
		{
		public:

			InterpolateIrregularSamplingVisitor(
					const GPlatesModel::PropertyValue &property_value1,
					const GPlatesModel::PropertyValue &property_value2,
					const double &time1,
					const double &time2,
					const double &target_time) :
				d_property_value1(property_value1),
				d_property_value2(property_value2),
				d_time1(time1),
				d_time2(time2),
				d_target_time(target_time)
			{  }

			/**
			 * Returns the interpolated property value if the property value 'type' is interpolable.
			 */
			boost::optional<GPlatesModel::PropertyValue::non_null_ptr_to_const_type>
			interpolate()
			{
				d_interpolated_property_value = boost::none;

				// Visit the first property value to get its type.
				d_property_value1.accept_visitor(*this);

				return d_interpolated_property_value;
			}

		private:

			virtual
			void
			visit_gpml_finite_rotation(
					gpml_finite_rotation_type &gpml_finite_rotation1)
			{
				// We can't interpolate if both times are equal.
				if (GPlatesMaths::are_almost_exactly_equal(d_time1, d_time2))
				{
					d_interpolated_property_value =
							GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_to_const_type(
									&gpml_finite_rotation1);
					return;
				}

				// Get the second property value.
				boost::optional<GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_to_const_type>
						gpml_finite_rotation2 =
								get_property_value<GPlatesPropertyValues::GpmlFiniteRotation>(
										d_property_value2);
				// Second property value should be same type as the first.
				if (gpml_finite_rotation2)
				{
					// If either of the finite rotations has an axis hint, use it.
					boost::optional<GPlatesMaths::UnitVector3D> axis_hint;
					if (gpml_finite_rotation1.get_finite_rotation().axis_hint())
					{
						axis_hint = gpml_finite_rotation1.get_finite_rotation().axis_hint();
					}
					else if (gpml_finite_rotation2.get()->get_finite_rotation().axis_hint())
					{
						axis_hint = gpml_finite_rotation2.get()->get_finite_rotation().axis_hint();
					}

					d_interpolated_property_value = GPlatesPropertyValues::GpmlFiniteRotation::create(
							GPlatesMaths::interpolate(
									gpml_finite_rotation1.get_finite_rotation(),
									gpml_finite_rotation2.get()->get_finite_rotation(),
									d_time1,
									d_time2,
									d_target_time,
									axis_hint));
				}
			}

			virtual
			void
			visit_xs_double(
					xs_double_type &xs_double1)
			{
				// We can't interpolate if both times are equal.
				if (GPlatesMaths::are_almost_exactly_equal(d_time1, d_time2))
				{
					d_interpolated_property_value =
							GPlatesPropertyValues::XsDouble::non_null_ptr_to_const_type(
									&xs_double1);
					return;
				}

				// Get the second property value.
				boost::optional<GPlatesPropertyValues::XsDouble::non_null_ptr_to_const_type>
						xs_double2 = get_property_value<GPlatesPropertyValues::XsDouble>(d_property_value2);
				// Second property value should be same type as the first.
				if (xs_double2)
				{
					const double interpolation = (d_target_time - d_time1) / (d_time2 - d_time1);

					d_interpolated_property_value = GPlatesPropertyValues::XsDouble::create(
							(1 - interpolation) * xs_double1.get_value() +
								interpolation * xs_double2.get()->get_value());
				}
			}

			//
			// NOTE: GpmlMeasure is another candidate for interpolation, but currently the GPGIM
			// states it is not time-dependent. Also would need to figure out how to merge XML
			// attributes of the two time samples being interpolated.
			//


			const GPlatesModel::PropertyValue &d_property_value1;
			const GPlatesModel::PropertyValue &d_property_value2;
			const double &d_time1;
			const double &d_time2;
			const double &d_target_time;

			boost::optional<GPlatesModel::PropertyValue::non_null_ptr_to_const_type> d_interpolated_property_value;
		};
	}
}

void
GPlatesFeatureVisitors::Implementation::visit_gpml_constant_value(
		GPlatesModel::ConstFeatureVisitor::gpml_constant_value_type &gpml_constant_value,
		GPlatesModel::ConstFeatureVisitor &property_value_finder_visitor)
{
	gpml_constant_value.value()->accept_visitor(property_value_finder_visitor);
}


void
GPlatesFeatureVisitors::Implementation::visit_gpml_irregular_sampling_at_reconstruction_time(
		GPlatesModel::ConstFeatureVisitor::gpml_irregular_sampling_type &gpml_irregular_sampling,
		GPlatesModel::ConstFeatureVisitor &property_value_finder_visitor,
		const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
		const std::type_info &property_value_type_info)
{
	//
	// Interpolates an irregularly-sampled property value if it's interpolable, otherwise returns
	// property value at nearest time sample (to reconstruction time).
	//

	const GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeSample> &time_samples =
			gpml_irregular_sampling.time_samples();

	// Optimisation to avoid interpolating a property value when it's the
	// wrong type and will just get discarded anyway.
	if (time_samples.empty() ||
		typeid(*time_samples.front().get()->value()) != property_value_type_info)
	{
		return;
	}

	// Get a list of the *enabled* time samples.
	std::vector<GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_to_const_type> enabled_time_samples;

	for (GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeSample>::const_iterator iter = time_samples.begin();
		iter != time_samples.end();
		++iter)
	{
		GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_to_const_type time_sample = *iter;
		if (!time_sample->is_disabled())
		{
			enabled_time_samples.push_back(time_sample);
		}
	}

	// Return early if all time samples are disabled.
	if (enabled_time_samples.empty())
	{
		return;
	}

	// If the requested time is later than the first (most-recent) time sample then
	// it is outside the time range of the time sample sequence.
	if (reconstruction_time > enabled_time_samples[0]->valid_time()->get_time_position())
	{
		return;
	}

	// Find adjacent time samples that span the requested time.
	for (unsigned int i = 1; i < enabled_time_samples.size(); ++i)
	{
		if (reconstruction_time >= enabled_time_samples[i]->valid_time()->get_time_position())
		{
			// The requested time is later than (more recent) or equal to the sample's time...

			const double time1 = enabled_time_samples[i-1]->valid_time()->get_time_position().value();
			const double time2 = enabled_time_samples[i]->valid_time()->get_time_position().value();
			const double target_time = reconstruction_time.value();

			InterpolateIrregularSamplingVisitor interpolate_visitor(
					*enabled_time_samples[i-1]->value(),
					*enabled_time_samples[i]->value(),
					time1,
					time2,
					target_time);
			boost::optional<GPlatesModel::PropertyValue::non_null_ptr_to_const_type>
					interpolated_property_value = interpolate_visitor.interpolate();

			// If property value 'type' is interpolable...
			if (interpolated_property_value)
			{
				// Add the interpolated property value to the list of found property values
				// (if it's the correct property value type).
				interpolated_property_value.get()->accept_visitor(property_value_finder_visitor);
			}
		}
	}
}


void
GPlatesFeatureVisitors::Implementation::visit_gpml_piecewise_aggregation_at_reconstruction_time(
		GPlatesModel::ConstFeatureVisitor::gpml_piecewise_aggregation_type &gpml_piecewise_aggregation,
		GPlatesModel::ConstFeatureVisitor &property_value_finder_visitor,
		const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time)
{
	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator iter =
			gpml_piecewise_aggregation.time_windows().begin();
	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator end =
			gpml_piecewise_aggregation.time_windows().end();
	for ( ; iter != end; ++iter)
	{
		// If the time window covers our reconstruction time then visit.
		if (iter->get()->valid_time()->contains(reconstruction_time))
		{
			iter->get()->time_dependent_value()->accept_visitor(property_value_finder_visitor);

			// Break out of loop since time windows should be non-overlapping.
			// If we don't break out we might visit the property value twice if it
			// falls on the boundary between two time periods (due to numerical tolerance).
			return;
		}
	}
}
