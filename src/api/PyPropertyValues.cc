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

#include "global/python.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlPlateId.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;

namespace GPlatesApi
{
	// Default argument overloads of 'GPlatesPropertyValues::GmlTimeInstant::create'.
	BOOST_PYTHON_FUNCTION_OVERLOADS(
			gml_time_instant_create_overloads,
			GPlatesPropertyValues::GmlTimeInstant::create, 1, 2)

	/**
	 * Returns a modifiable begin time instant from a time period.
	 *
	 * TODO: Fix revisioning such that the time period returns a 'non-const' time instant.
	 */
	GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
	gml_time_period_get_begin(
			GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type gml_time_period)
	{
		return gml_time_period->get_begin()->clone();
	}

	/**
	 * Returns a modifiable end time instant from a time period.
	 *
	 * TODO: Fix revisioning such that the time period returns a 'non-const' time instant.
	 */
	GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
	gml_time_period_get_end(
			GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type gml_time_period)
	{
		return gml_time_period->get_end()->clone();
	}

	/**
	 * Handle implicit conversions of non_null_intrusive_ptr for the specified derived/base classes.
	 *
	 * Both 'SourceType' and 'TargetType' should be non-const.
	 */
	template <class SourceType, class TargetType>
	void
	implicitly_convertible()
	{
		// Enable a python-wrapped SourceType to be used when a TargetType is requested.
		bp::implicitly_convertible<
				GPlatesUtils::non_null_intrusive_ptr<SourceType>,
				GPlatesUtils::non_null_intrusive_ptr<TargetType> >();
		bp::implicitly_convertible<
				GPlatesUtils::non_null_intrusive_ptr<SourceType>,
				GPlatesUtils::non_null_intrusive_ptr<const TargetType> >();
		bp::implicitly_convertible<
				GPlatesUtils::non_null_intrusive_ptr<const SourceType>,
				GPlatesUtils::non_null_intrusive_ptr<const TargetType> >();
	}
}


void
export_property_values()
{
	/*
	 * Base property value wrapper class.
	 *
	 * Enables 'isinstance(obj, PropertyValue)' in python - not that it's that useful.
	 */
	bp::class_<GPlatesModel::PropertyValue, boost::noncopyable>("PropertyValue", bp::no_init);

	//
	// GeoTimeInstant
	//
	bp::class_<GPlatesPropertyValues::GeoTimeInstant>("GeoTimeInstant", bp::init<double>())
		.def("create_distant_past", &GPlatesPropertyValues::GeoTimeInstant::create_distant_past)
		.staticmethod("create_distant_past")
  		.def("create_distant_future", &GPlatesPropertyValues::GeoTimeInstant::create_distant_future)
		.staticmethod("create_distant_future")
		.def("get_value",
				&GPlatesPropertyValues::GeoTimeInstant::value,
				bp::return_value_policy<bp::copy_const_reference>())
		.def("is_distant_past", &GPlatesPropertyValues::GeoTimeInstant::is_distant_past)
		.def("is_distant_future", &GPlatesPropertyValues::GeoTimeInstant::is_distant_future)
 		.def(bp::self == bp::self)
 		.def(bp::self != bp::self)
 		.def(bp::self < bp::self)
 		.def(bp::self <= bp::self)
 		.def(bp::self > bp::self)
 		.def(bp::self >= bp::self)
	;

	//
	// GmlTimeInstant
	//
	bp::class_<
			GPlatesPropertyValues::GmlTimeInstant,
			GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue> >("GmlTimeInstant", bp::no_init)
 		.def("create",
				&GPlatesPropertyValues::GmlTimeInstant::create,
				GPlatesApi::gml_time_instant_create_overloads())
 		.staticmethod("create")
  		.def("get_time_position",
				&GPlatesPropertyValues::GmlTimeInstant::get_time_position,
				bp::return_value_policy<bp::copy_const_reference>())
 		.def("set_time_position", &GPlatesPropertyValues::GmlTimeInstant::set_time_position)
	;

	// Enable a python-wrapped GmlTimeInstant to be used when a PropertyValue is requested.
	GPlatesApi::implicitly_convertible<
			GPlatesPropertyValues::GmlTimeInstant,
			GPlatesModel::PropertyValue>();
	// Enable a python-wrapped 'non-const' GmlTimeInstant to be used when a 'const' GmlTimeInstant is requested.
	bp::implicitly_convertible<
			GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type,
			GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_to_const_type>();

	//
	// GmlTimePeriod
	//
	bp::class_<
			GPlatesPropertyValues::GmlTimePeriod,
			GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue> >("GmlTimePeriod", bp::no_init)
 		.def("create", &GPlatesPropertyValues::GmlTimePeriod::create)
 		.staticmethod("create")
		.def("get_begin", &GPlatesApi::gml_time_period_get_begin)
 		.def("set_begin", &GPlatesPropertyValues::GmlTimePeriod::set_begin)
  		.def("get_end", &GPlatesApi::gml_time_period_get_end)
 		.def("set_end", &GPlatesPropertyValues::GmlTimePeriod::set_end)
	;

	// Enable a python-wrapped GmlTimePeriod to be used when a PropertyValue is requested.
	GPlatesApi::implicitly_convertible<
			GPlatesPropertyValues::GmlTimePeriod,
			GPlatesModel::PropertyValue>();
	// Enable a python-wrapped 'non-const' GmlTimePeriod to be used when a 'const' GmlTimePeriod is requested.
	bp::implicitly_convertible<
			GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type,
			GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type>();

	//
	// GpmlPlateId
	//
	bp::class_<
			GPlatesPropertyValues::GpmlPlateId,
			GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue> >("GpmlPlateId", bp::no_init)
 		.def("create", &GPlatesPropertyValues::GpmlPlateId::create)
 		.staticmethod("create")
  		.def("get_value", &GPlatesPropertyValues::GpmlPlateId::get_value)
 		.def("set_value", &GPlatesPropertyValues::GpmlPlateId::set_value)
	;

	// Enable a python-wrapped GpmlPlateId to be used when a PropertyValue is requested.
	GPlatesApi::implicitly_convertible<
			GPlatesPropertyValues::GpmlPlateId,
			GPlatesModel::PropertyValue>();
	// Enable a python-wrapped 'non-const' GpmlPlateId to be used when a 'const' GpmlPlateId is requested.
	bp::implicitly_convertible<
			GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type,
			GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type>();

}

#endif // GPLATES_NO_PYTHON
