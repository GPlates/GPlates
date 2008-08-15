/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-07-24 19:17:34 -0700 (Thu, 24 Jul 2008) $
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_FEATUREVISITORS_VALUEFINDER_H
#define GPLATES_FEATUREVISITORS_VALUEFINDER_H

#include <vector>

#include "model/ConstFeatureVisitor.h"
#include "model/PropertyName.h"
#include "model/PropertyValue.h"
#include "model/types.h"

namespace GPlatesFeatureVisitors
{
	/**
	 * This feature visitor finds values contained within the feature.
	 */
	class ValueFinder:
			public GPlatesModel::ConstFeatureVisitor
	{

	public:

		typedef std::vector<std::string> value_container_type;
		typedef value_container_type::iterator value_container_iterator;

		// FIXME:  We should also pass the current reconstruction time, so we can correctly
		// handle time-dependent property values.
		ValueFinder()
		{  }

		explicit
		ValueFinder(
				const GPlatesModel::PropertyName &property_name_to_allow)
		{
			d_property_names_to_allow.push_back(property_name_to_allow);
		}

		virtual
		~ValueFinder() 
		{  }

		void
		add_property_name_to_allow(
				const GPlatesModel::PropertyName &property_name_to_allow)
		{
			d_property_names_to_allow.push_back(property_name_to_allow);
		}

		virtual
		void
		visit_feature_handle(
				const GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_inline_property_container(
				const GPlatesModel::InlinePropertyContainer &inline_property_container);


		// Please keep these property-value types ordered alphabetically.

#if 0
		virtual
		void
		visit_enumeration(
				const GPlatesPropertyValues::Enumeration &enumeration);

		virtual
		void
		visit_gml_line_string(
				const GPlatesPropertyValues::GmlLineString &gml_line_string);

		virtual
		void
		visit_gml_orientable_curve(
				const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve);

		virtual
		void
		visit_gml_point(
				const GPlatesPropertyValues::GmlPoint &gml_point);

		virtual
		void
		visit_gml_polygon(
				const GPlatesPropertyValues::GmlPolygon &gml_polygon);

		virtual
		void
		visit_gml_time_instant(
				const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant);

		virtual
		void
		visit_gml_time_period(
				const GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

#endif
		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);
#if 0

		virtual
		void
		visit_gpml_feature_reference(
				const GPlatesPropertyValues::GpmlFeatureReference &gpml_feature_reference);

		virtual
		void
		visit_gpml_feature_snapshot_reference(
				const GPlatesPropertyValues::GpmlFeatureSnapshotReference &gpml_feature_snapshot_reference);

		virtual
		void
		visit_gpml_finite_rotation(
				const GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation);

		virtual
		void
		visit_gpml_finite_rotation_slerp(
				const GPlatesPropertyValues::GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp);

		virtual
		void
		visit_gpml_hot_spot_trail_mark(
				const GPlatesPropertyValues::GpmlHotSpotTrailMark &gpml_hot_spot_trail_mark);

		virtual
		void
		visit_gpml_irregular_sampling(
				const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling);

		virtual
		void
		visit_gpml_measure(
				const GPlatesPropertyValues::GpmlMeasure &gpml_measure);
#endif

		virtual
		void
		visit_gpml_old_plates_header(
				const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header);

#if 0
		virtual
		void
		visit_gpml_piecewise_aggregation(
				const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation);
#endif

		virtual
		void
		visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);
#if 0

		virtual
		void
		visit_gpml_polarity_chron_id(
				const GPlatesPropertyValues::GpmlPolarityChronId &gpml_polarity_chron_id);

		virtual
		void
		visit_gpml_property_delegate(
				const GPlatesPropertyValues::GpmlPropertyDelegate &gpml_property_delegate);

		virtual
		void
		visit_gpml_revision_id(
				const GPlatesPropertyValues::GpmlRevisionId &gpml_revision_id);

		virtual
		void
		visit_gpml_time_sample(
				const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample);

		virtual
		void
		visit_gpml_time_window(
				const GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window);

		virtual
		void
		visit_uninterpreted_property_value(
				const GPlatesPropertyValues::UninterpretedPropertyValue &uninterpreted_prop_val);

		virtual
		void
		visit_xs_boolean(
				const GPlatesPropertyValues::XsBoolean &xs_boolean);

		virtual
		void
		visit_xs_double(
				const GPlatesPropertyValues::XsDouble &xs_double);

		virtual
		void
		visit_xs_integer(
				const GPlatesPropertyValues::XsInteger &xs_integer);
#endif 

		virtual
		void
		visit_xs_string(
				const GPlatesPropertyValues::XsString &xs_string);



		// access 

		value_container_iterator
		found_values_begin()
		{
			return d_found_value.begin();
		}

		value_container_iterator
		found_values_end()
		{
			return d_found_value.end();
		}

		void
		clear_value()
		{
			d_found_value.clear();
		}

	private:

		std::vector<GPlatesModel::PropertyName> d_property_names_to_allow;

		value_container_type d_found_value;
	};
}

#endif  // GPLATES_FEATUREVISITORS_VALUEFINDER_H
