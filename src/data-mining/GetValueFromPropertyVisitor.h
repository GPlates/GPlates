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
#include "property-values/GpmlPlateId.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"

namespace GPlatesDataMining
{
	/*
	* TODO
	*/
	template < class DataType >
	class GetValueFromPropertyVisitor : 
		public GPlatesModel::ConstFeatureVisitor
	{
	public:

		virtual
		void
		visit_enumeration(
				enumeration_type &enumeration)
		{  }

		virtual
		void
		visit_gml_data_block(
				gml_data_block_type &gml_data_block)
		{  }

		virtual
		void
		visit_gml_line_string(
				gml_line_string_type &gml_line_string)
		{  }

		virtual
		void
		visit_gml_multi_point(
				gml_multi_point_type &gml_multi_point)
		{  }

		virtual
		void
		visit_gml_orientable_curve(
				gml_orientable_curve_type &gml_orientable_curve)
		{  }

		virtual
		void
		visit_gml_point(
				gml_point_type &gml_point)
		{  }

		virtual
		void
		visit_gml_polygon(
				gml_polygon_type &gml_polygon)
		{  }

		virtual
		void
		visit_gml_time_instant(
				gml_time_instant_type &gml_time_instant)
		{  }

		virtual
		void
		visit_gml_time_period(
				gml_time_period_type &gml_time_period)
		{  }

		virtual
		void
		visit_gpml_constant_value(
				gpml_constant_value_type &gpml_constant_value)
		{
			gpml_constant_value.value()->accept_visitor(*this);
		}

		virtual
		void
		visit_gpml_feature_reference(
				gpml_feature_reference_type &gpml_feature_reference)
		{  }

		virtual
		void
		visit_gpml_feature_snapshot_reference(
				gpml_feature_snapshot_reference_type &gpml_feature_snapshot_reference)
		{  }

		virtual
		void
		visit_gpml_finite_rotation(
				gpml_finite_rotation_type &gpml_finite_rotation)
		{  }

		virtual
		void
		visit_gpml_finite_rotation_slerp(
				gpml_finite_rotation_slerp_type &gpml_finite_rotation_slerp)
		{  }

		virtual
		void
		visit_gpml_hot_spot_trail_mark(
				gpml_hot_spot_trail_mark_type &gpml_hot_spot_trail_mark)
		{  }

		virtual
		void
		visit_gpml_irregular_sampling(
				gpml_irregular_sampling_type &gpml_irregular_sampling)
		{  }

		virtual
		void
		visit_gpml_key_value_dictionary(
				gpml_key_value_dictionary_type &gpml_key_value_dictionary)
		{  }

		virtual
		void
		visit_gpml_measure(
				gpml_measure_type &gpml_measure)
		{  }

		virtual
		void
		visit_gpml_old_plates_header(
				gpml_old_plates_header_type &gpml_old_plates_header) 
		{  }

		virtual
		void
		visit_gpml_piecewise_aggregation(
			gpml_piecewise_aggregation_type &gpml_piecewise_aggregation)
		{  }

		virtual
		void
		visit_gpml_plate_id(
				gpml_plate_id_type &gpml_plate_id)
		{  }

		virtual
		void
		visit_gpml_polarity_chron_id(
				gpml_polarity_chron_id_type &gpml_polarity_chron_id)
		{  }

		virtual
		void
		visit_gpml_property_delegate(
				gpml_property_delegate_type &gpml_property_delegate)
		{  }

		virtual
		void
		visit_gpml_revision_id(
				gpml_revision_id_type &gpml_revision_id) 
		{  }

		virtual
		void
		visit_gpml_topological_polygon(
				gpml_topological_polygon_type &gpml_toplogical_polygon)
		{  }

		virtual
		void
		visit_gpml_topological_line_section(
				gpml_topological_line_section_type &gpml_toplogical_line_section)
		{  }

		virtual
		void
		visit_gpml_topological_intersection(
				gpml_topological_intersection_type &gpml_toplogical_intersection)
		{  }

		virtual
		void
		visit_gpml_topological_point(
				gpml_topological_point_type &gpml_toplogical_point)
		{  }

		virtual
		void
		visit_uninterpreted_property_value(
				uninterpreted_property_value_type &uninterpreted_prop_val) 
		{  }

		virtual
		void
		visit_xs_boolean(
				xs_boolean_type &xs_boolean)
		{ }

		virtual
		void
		visit_xs_double(
				xs_double_type &xs_double)
		{ }

		virtual
		void
		visit_xs_integer(
				xs_integer_type &xs_integer)
		{ }

		void
		visit_xs_string(
				xs_string_type &xs_string)
		{ }

		std::vector< DataType >&
		get_data()
		{
			return d_data;
		}
		
	protected:
		std::vector< DataType > d_data;
	};


	template<>
	inline
	void
	GetValueFromPropertyVisitor<OpaqueData>::visit_xs_string(
			xs_string_type &xs_string)
	{
		d_data.push_back(
				GPlatesUtils::make_qstring_from_icu_string(
						xs_string.value().get()));
	}


	template<>
	inline
	void
	GetValueFromPropertyVisitor<OpaqueData>::visit_xs_boolean(
			xs_boolean_type &xs_boolean)
	{
		d_data.push_back(xs_boolean.value());
	}

	template<>
	inline
	void
	GetValueFromPropertyVisitor<OpaqueData>::visit_xs_double(
			xs_double_type &xs_double)
	{
		d_data.push_back(xs_double.value());
	}

	template<>
	inline
	void
	GetValueFromPropertyVisitor<OpaqueData>::visit_xs_integer(
			xs_integer_type &xs_integer)
	{
		d_data.push_back(xs_integer.value());
	}

	template<>
	inline
	void
	GetValueFromPropertyVisitor< QString >::visit_xs_string(
			xs_string_type &xs_string)
	{
		d_data.push_back(
				GPlatesUtils::make_qstring_from_icu_string(
						xs_string.value().get()));
	}

	template<>
	inline
	void
	GetValueFromPropertyVisitor< std::string >::visit_xs_string(
			xs_string_type &xs_string)
	{
		d_data.push_back(
				GPlatesUtils::make_qstring_from_icu_string(
						xs_string.value().get()).toStdString());
	}

	template<>
	inline
	void
	GetValueFromPropertyVisitor< double >::visit_xs_string(
			xs_string_type &xs_string)
	{
		
	}

	template<>
	inline
	void
	GetValueFromPropertyVisitor< QString >::visit_gpml_plate_id(
			gpml_plate_id_type &gpml_plate_id)
	{
		QString str;
		str.setNum( gpml_plate_id.value() );
		d_data.push_back(str);
	}

	template<>
	inline
	void
	GetValueFromPropertyVisitor< QString >::visit_xs_boolean(
			xs_boolean_type &xs_boolean)
	{
		if(xs_boolean.value())
		{
			d_data.push_back("true");
		}
		else
		{
			d_data.push_back("false");
		}
	}

	template<>
	inline
	void
	GetValueFromPropertyVisitor< bool >::visit_xs_boolean(
			xs_boolean_type &xs_boolean)
	{
		d_data.push_back(xs_boolean.value());
	}

	template<>
	inline
	void
	GetValueFromPropertyVisitor< QString >::visit_xs_double(
			xs_double_type &xs_double)
	{
		QString tmp;
		tmp.setNum(xs_double.value());
		d_data.push_back(tmp);
	}

	template<>
	inline
	void
	GetValueFromPropertyVisitor< double >::visit_xs_double(
			xs_double_type &xs_double)
	{
		d_data.push_back(xs_double.value());
	}

	template<>
	inline
	void
	GetValueFromPropertyVisitor< QString >::visit_xs_integer(
			xs_integer_type &xs_integer)
	{
		QString tmp;
		tmp.setNum(xs_integer.value());
		d_data.push_back(tmp);
	}

	template<>
	inline
	void
	GetValueFromPropertyVisitor< int >::visit_xs_integer(
			xs_integer_type &xs_integer)
	{
		
		d_data.push_back(xs_integer.value());
	}

	template<>
	inline
	void
	GetValueFromPropertyVisitor< double >::visit_xs_integer(
			xs_integer_type &xs_integer)
	{
		d_data.push_back(static_cast<double>(xs_integer.value()));
	}

}

#endif



