/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
#ifndef GPLATESDATAMINING_GETVALUEFROMPROPERTYVISITOR_H
#define GPLATESDATAMINING_GETVALUEFROMPROPERTYVISITOR_H

#include <vector>
#include <string>
#include <QString>

#include "DataTable.h"

#include "property-values/XsString.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"

namespace GPlatesDataMining
{
	/*
	* Get OpaqueData data from property.
	*/
	class GetValueFromPropertyVisitor : 
		public GPlatesModel::ConstFeatureVisitor
	{
	public:
		void visit_enumeration(enumeration_type &enumeration);

		void visit_gml_data_block(gml_data_block_type &gml_data_block){}

		void visit_gml_line_string(gml_line_string_type &gml_line_string);

		void visit_gml_multi_point(gml_multi_point_type &gml_multi_point);

		void visit_gml_orientable_curve(gml_orientable_curve_type &gml_orientable_curve);

		void visit_gml_point(gml_point_type &gml_point);

		void visit_gml_polygon(gml_polygon_type &gml_polygon);

		void visit_gml_time_instant(gml_time_instant_type &gml_time_instant);

		void visit_gml_time_period(gml_time_period_type &gml_time_period);

		void visit_gpml_plate_id(gpml_plate_id_type &gpml_plate_id);

		void visit_gpml_constant_value(gpml_constant_value_type &gpml_constant_value)
		{
			gpml_constant_value.value()->accept_visitor(*this);
		}

		void visit_gpml_feature_reference(gpml_feature_reference_type &gpml_feature_reference){}

		void visit_gpml_feature_snapshot_reference(gpml_feature_snapshot_reference_type &gpml_feature_snapshot_reference){}

		void visit_gpml_finite_rotation(gpml_finite_rotation_type &gpml_finite_rotation){}

		void visit_gpml_finite_rotation_slerp(gpml_finite_rotation_slerp_type &gpml_finite_rotation_slerp){}

		void visit_gpml_hot_spot_trail_mark(gpml_hot_spot_trail_mark_type &gpml_hot_spot_trail_mark){}

		void visit_gpml_irregular_sampling(gpml_irregular_sampling_type &gpml_irregular_sampling){}

		void visit_gpml_key_value_dictionary(gpml_key_value_dictionary_type &gpml_key_value_dictionary){}

		void visit_gpml_measure(gpml_measure_type &gpml_measure){}

		void visit_gpml_old_plates_header(gpml_old_plates_header_type &gpml_old_plates_header) {}

		void visit_gpml_piecewise_aggregation(gpml_piecewise_aggregation_type &gpml_piecewise_aggregation){}

		void visit_gpml_polarity_chron_id(gpml_polarity_chron_id_type &gpml_polarity_chron_id){}

		void visit_gpml_property_delegate(gpml_property_delegate_type &gpml_property_delegate){}

		void visit_gpml_revision_id(gpml_revision_id_type &gpml_revision_id){}

		void visit_gpml_topological_polygon(gpml_topological_polygon_type &gpml_toplogical_polygon){}
		
		void visit_gpml_topological_line_section(gpml_topological_line_section_type &gpml_toplogical_line_section){}

		void visit_gpml_topological_intersection(gpml_topological_intersection_type &gpml_toplogical_intersection){}

		void visit_gpml_topological_point(gpml_topological_point_type &gpml_toplogical_point){}

		void visit_uninterpreted_property_value(uninterpreted_property_value_type &uninterpreted_prop_val){}

		inline
		void visit_xs_boolean(xs_boolean_type &xs_boolean)
		{
			d_data.push_back(xs_boolean.value());
		}

		inline
		void visit_xs_double(xs_double_type &xs_double)
		{
			d_data.push_back(xs_double.value());
		}
		
		inline
		void visit_xs_integer(xs_integer_type &xs_integer)
		{
			d_data.push_back(xs_integer.value());
		}

		void visit_xs_string(xs_string_type &xs_string)
		{
			d_data.push_back(xs_string.value().get().qstring());
		}

		inline
		std::vector<OpaqueData>&
		get_data()
		{
			return d_data;
		}

	protected:
		QString
		to_qstring(
				const GPlatesModel::PropertyValue& data);
		
	private:
		std::vector<OpaqueData> d_data;
	};
}

#endif



