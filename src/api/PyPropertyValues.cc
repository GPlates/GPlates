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

#include "global/CompilerWarnings.h"
#include "global/python.h"

#include "model/FeatureVisitor.h"
#include "model/ModelUtils.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlHotSpotTrailMark.h"
#include "property-values/GpmlPlateId.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	// Return cloned property value to python as its derived property value type.
	bp::object/*derived property value non_null_intrusive_ptr*/
	property_value_clone(
			GPlatesModel::PropertyValue &property_value)
	{
		// The derived property value type is needed otherwise python is unable to access the derived attributes.
		return PythonConverterUtils::get_property_value_as_derived_type(property_value.clone());
	}

	// Select the 'non-const' overload of 'PropertyValue::accept_visitor()'.
	void
	property_value_accept_visitor(
			GPlatesModel::PropertyValue &property_value,
			GPlatesModel::FeatureVisitor &visitor)
	{
		property_value.accept_visitor(visitor);
	}

	const GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
	gml_time_instant_create(
			const GPlatesPropertyValues::GeoTimeInstant &time_position)
	{
		// Use the default value for the second argument.
		return GPlatesPropertyValues::GmlTimeInstant::create(time_position);
	}

DISABLE_GCC_WARNING("-Wshadow")

	const GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type
	gpml_constant_value_create(
			GPlatesModel::PropertyValue::non_null_ptr_type property_value,
			const GPlatesUtils::UnicodeString &description = "")
	{
		// ModelUtils takes care of determining the structural type of the nested property value.
		return GPlatesModel::ModelUtils::create_gpml_constant_value(property_value, description);
	}

	// Default argument overloads of 'GPlatesPropertyValues::GpmlConstantValue::create'.
	BOOST_PYTHON_FUNCTION_OVERLOADS(
			gpml_constant_value_create_overloads,
			gpml_constant_value_create, 1, 2)

ENABLE_GCC_WARNING("-Wshadow")

	// Return base property value to python as its derived property value type.
	bp::object/*derived property value non_null_intrusive_ptr*/
	gpml_constant_value_get_value(
			GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type constant_value)
	{
		// The derived property value type is needed otherwise python is unable to access the derived attributes.
		return PythonConverterUtils::get_property_value_as_derived_type(constant_value->value());
	}
}


void
export_property_value()
{
	/*
	 * Base property value wrapper class.
	 *
	 * Enables 'isinstance(obj, PropertyValue)' in python - not that it's that useful.
	 *
	 * NOTE: We never return a 'PropertyValue::non_null_ptr_type' to python because then python is
	 * unable to access the attributes of the derived property value type. For this reason usually
	 * the derived property value is returned using:
	 *   'PythonConverterUtils::get_property_value_as_derived_type()'
	 */
	bp::class_<GPlatesModel::PropertyValue, boost::noncopyable>("PropertyValue", bp::no_init)
		.def("clone", &GPlatesApi::property_value_clone)
		.def("accept_visitor", &GPlatesApi::property_value_accept_visitor)
	;
}

//////////////////////////////////////////////////////////////////////////
// NOTE: Please keep the property values alphabetically ordered.
//////////////////////////////////////////////////////////////////////////

void
export_geo_time_instant()
{
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

	// Enable boost::optional<GeoTimeInstant> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesPropertyValues::GeoTimeInstant>();
}


void
export_gml_time_instant()
{
	//
	// GmlTimeInstant
	//
	bp::class_<
			GPlatesPropertyValues::GmlTimeInstant,
			GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GmlTimeInstant", bp::no_init)
		.def("create", &GPlatesApi::gml_time_instant_create)
		.staticmethod("create")
		.def("get_time_position",
				&GPlatesPropertyValues::GmlTimeInstant::get_time_position,
				bp::return_value_policy<bp::copy_const_reference>())
		.def("set_time_position", &GPlatesPropertyValues::GmlTimeInstant::set_time_position)
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GmlTimeInstant,
			GPlatesModel::PropertyValue>();
}


void
export_gml_time_period()
{
	// Use the 'non-const' overload so GmlTimeInstant can be modified via python...
	const GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
			(GPlatesPropertyValues::GmlTimePeriod::*begin)() =
					&GPlatesPropertyValues::GmlTimePeriod::begin;
	// Use the 'non-const' overload so GmlTimeInstant can be modified via python...
	const GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
			(GPlatesPropertyValues::GmlTimePeriod::*end)() =
					&GPlatesPropertyValues::GmlTimePeriod::end;

	//
	// GmlTimePeriod
	//
	bp::class_<
			GPlatesPropertyValues::GmlTimePeriod,
			GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GmlTimePeriod", bp::no_init)
		.def("create", &GPlatesPropertyValues::GmlTimePeriod::create)
		.staticmethod("create")
		.def("begin", begin)
		.def("set_begin", &GPlatesPropertyValues::GmlTimePeriod::set_begin)
		.def("end", end)
		.def("set_end", &GPlatesPropertyValues::GmlTimePeriod::set_end)
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GmlTimePeriod,
			GPlatesModel::PropertyValue>();
}


void
export_gpml_constant_value()
{
	// Use the 'non-const' overload so the nested property value in GpmlConstantValue can be modified via python...
	const GPlatesModel::PropertyValue::non_null_ptr_type
			(GPlatesPropertyValues::GpmlConstantValue::*value)() =
					&GPlatesPropertyValues::GpmlConstantValue::value;

	//
	// GpmlConstantValue
	//
	bp::class_<
			GPlatesPropertyValues::GpmlConstantValue,
			GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlConstantValue", bp::no_init)
		.def("create",
				&GPlatesApi::gpml_constant_value_create,
				GPlatesApi::gpml_constant_value_create_overloads())
		.staticmethod("create")
		.def("get_value", &GPlatesApi::gpml_constant_value_get_value)
		.def("set_value", &GPlatesPropertyValues::GpmlConstantValue::set_value)
		.def("get_description",
				&GPlatesPropertyValues::GpmlConstantValue::get_description,
				bp::return_value_policy<bp::copy_const_reference>())
		.def("set_description", &GPlatesPropertyValues::GpmlConstantValue::set_description)
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GpmlConstantValue,
			GPlatesModel::PropertyValue>();
}


void
export_gpml_hot_spot_trail_mark()
{
	// Use the 'non-const' overload so GmlTimeInstant can be modified via python...
	const boost::optional<GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type>
			(GPlatesPropertyValues::GpmlHotSpotTrailMark::*measured_age)() =
					&GPlatesPropertyValues::GpmlHotSpotTrailMark::measured_age;

	//
	// GpmlHotSpotTrailMark
	//
	bp::class_<
			GPlatesPropertyValues::GpmlHotSpotTrailMark,
			GPlatesPropertyValues::GpmlHotSpotTrailMark::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlHotSpotTrailMark", bp::no_init)
		//.def("create", &GPlatesPropertyValues::GpmlHotSpotTrailMark::create)
		//.staticmethod("create")
		//.def("position", &GPlatesPropertyValues::GpmlHotSpotTrailMark::position)
		//.def("set_position", &GPlatesPropertyValues::GpmlHotSpotTrailMark::set_position)
		.def("measured_age", measured_age)
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GpmlHotSpotTrailMark,
			GPlatesModel::PropertyValue>();
}


void
export_gpml_plate_id()
{
	//
	// GpmlPlateId
	//
	bp::class_<
			GPlatesPropertyValues::GpmlPlateId,
			GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlPlateId", bp::no_init)
		.def("create", &GPlatesPropertyValues::GpmlPlateId::create)
		.staticmethod("create")
		.def("get_value", &GPlatesPropertyValues::GpmlPlateId::get_value)
		.def("set_value", &GPlatesPropertyValues::GpmlPlateId::set_value)
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GpmlPlateId,
			GPlatesModel::PropertyValue>();
}


void
export_property_values()
{
	export_property_value();

	//////////////////////////////////////////////////////////////////////////
	// NOTE: Please keep the property values alphabetically ordered.
	//////////////////////////////////////////////////////////////////////////

	export_geo_time_instant();
	export_gml_time_instant();
	export_gml_time_period();
	export_gpml_constant_value();
	export_gpml_hot_spot_trail_mark();
	export_gpml_plate_id();
}

#endif // GPLATES_NO_PYTHON
