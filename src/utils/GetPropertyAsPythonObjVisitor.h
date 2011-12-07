/* $Id: GetValueFromPropertyVisitor.h 11559 2011-05-16 07:41:01Z mchin $ */

/**
 * \file 
 * $Revision: 11559 $
 * $Date: 2011-05-16 17:41:01 +1000 (Mon, 16 May 2011) $
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
#ifndef GPLATESDATAMINING_GETPROPERTYASPYTHONOBJVISITOR_H
#define GPLATESDATAMINING_GETPROPERTYASPYTHONOBJVISITOR_H

#include "global/python.h"

#include <vector>
#include <string>
#include <QString>

#include "api/PythonUtils.h"

#include "property-values/XsString.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"

#include "utils/FeatureUtils.h"

#if !defined(GPLATES_NO_PYTHON)

namespace bp=boost::python;

namespace GPlatesUtils
{
	/*
	* Get property value as python object.
	* This visitor visits one property value each time. 
	* Use outer loop to iterator through all properties in a feature.
	*/

	class GetPropertyAsPythonObjVisitor : 
		public GPlatesModel::ConstFeatureVisitor
	{
	public:
		
		void 
		visit_enumeration(
				enumeration_type &enumeration);

		
		void 
		visit_gml_data_block(
				gml_data_block_type &v);


		void 
		visit_gml_line_string(
				gml_line_string_type &gml_line_string);

		
		void 
		visit_gml_multi_point(
				gml_multi_point_type &gml_multi_point);


		void 
		visit_gml_orientable_curve(
				gml_orientable_curve_type &gml_orientable_curve);


		void
		visit_gml_point(
				gml_point_type &gml_point);


		void 
		visit_gml_polygon(
				gml_polygon_type &gml_polygon);


		void 
		visit_gml_time_instant(
				gml_time_instant_type &gml_time_instant);


		void
		visit_gml_time_period(
				gml_time_period_type &gml_time_period);


		void
		visit_gpml_plate_id(
				gpml_plate_id_type &gpml_plate_id);


		void 
		visit_gpml_constant_value(
				gpml_constant_value_type &gpml_constant_value)
		{
			gpml_constant_value.value()->accept_visitor(*this);
		}


		void 
		visit_gpml_feature_reference(
				gpml_feature_reference_type &v);


		void 
		visit_gpml_feature_snapshot_reference(
				gpml_feature_snapshot_reference_type &v);


		void 
		visit_gpml_finite_rotation(
				gpml_finite_rotation_type &v);


		void 
		visit_gpml_finite_rotation_slerp(
				gpml_finite_rotation_slerp_type &v);


		void 
		visit_gpml_hot_spot_trail_mark(
				gpml_hot_spot_trail_mark_type &v);


		void 
		visit_gpml_irregular_sampling(
				gpml_irregular_sampling_type &v);


		void 
		visit_gpml_key_value_dictionary(
				gpml_key_value_dictionary_type &v);


		void
		visit_gpml_measure(
				gpml_measure_type &v);


		void 
		visit_gpml_old_plates_header(
				gpml_old_plates_header_type &v) ;


		void
		visit_gpml_piecewise_aggregation(
				gpml_piecewise_aggregation_type &v);


		void
		visit_gpml_polarity_chron_id(
				gpml_polarity_chron_id_type &v);


		void 
		visit_gpml_property_delegate(
				gpml_property_delegate_type &v);


		void
		visit_gpml_revision_id(
				gpml_revision_id_type &v);


		void
		visit_gpml_topological_polygon(
				gpml_topological_polygon_type &v);
		

		void
		visit_gpml_topological_line_section(
				gpml_topological_line_section_type &v);


		void
		visit_gpml_topological_intersection(
				gpml_topological_intersection_type &v);


		void
		visit_gpml_topological_point(
				gpml_topological_point_type &v);


		void 
		visit_uninterpreted_property_value(
				uninterpreted_property_value_type &v);


		void 
		visit_xs_boolean(
				xs_boolean_type &v)
		{
			d_val = bp::object(v.value());
		}


		void
		visit_xs_double(
				xs_double_type &v)
		{
			d_val = bp::object(v.value());
		}
		

		void 
		visit_xs_integer(
				xs_integer_type &v)
		{
			d_val = bp::object(v.value());
		}


		void 
		visit_xs_string(
				xs_string_type &xs_string)
		{
			const QByteArray buf = xs_string.value().get().qstring().toUtf8();
			d_val = bp::object(bp::str(buf.constData()));
		}


		bp::object
		get_data()
		{
			return d_val;
		}

	protected:
		QString
		to_qstring(
				const GPlatesModel::PropertyValue& data);
		
	private:
		bp::object d_val;
	};
}

#endif //GPLATES_NO_PYTHON
#endif //GPLATESDATAMINING_GETPROPERTYASPYTHONOBJVISITOR_H



