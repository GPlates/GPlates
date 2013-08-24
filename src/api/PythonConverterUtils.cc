/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include "PythonConverterUtils.h"

#include "global/python.h"

#include "model/FeatureVisitor.h"

#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlPlateId.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * Visits a property value and converts from its derived type to a python object.
	 */
	class GetPropertyValueAsDerivedTypeVisitor : 
			public GPlatesModel::FeatureVisitor
	{
	public:

		/**
		 * The derived property value retrieved after visiting a property value.
		 */
		bp::object
		get_property_value_as_derived_type()
		{
			return d_property_value;
		}


		void 
		visit_gml_time_instant(
				gml_time_instant_type &gml_time_instant)
		{
			// Use to-python converter registered for derived property value's 'non_null_ptr_type'.
			d_property_value = bp::object(gml_time_instant_type::non_null_ptr_type(&gml_time_instant));
		}


		void
		visit_gml_time_period(
				gml_time_period_type &gml_time_period)
		{
			// Use to-python converter registered for derived property value's 'non_null_ptr_type'.
			d_property_value = bp::object(gml_time_period_type::non_null_ptr_type(&gml_time_period));
		}


		void
		visit_gpml_plate_id(
				gpml_plate_id_type &gpml_plate_id)
		{
			// Use to-python converter registered for derived property value's 'non_null_ptr_type'.
			d_property_value = bp::object(gpml_plate_id_type::non_null_ptr_type(&gpml_plate_id));
		}

	private:
		bp::object d_property_value;
	};
}


bp::object/*derived property value*/
GPlatesApi::PythonConverterUtils::get_property_value_as_derived_type(
		GPlatesModel::PropertyValue::non_null_ptr_type property_value)
{
	GetPropertyValueAsDerivedTypeVisitor visitor;
	property_value->accept_visitor(visitor);
	return visitor.get_property_value_as_derived_type();
}


#endif // GPLATES_NO_PYTHON
