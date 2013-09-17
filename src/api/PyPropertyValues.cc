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

#include <algorithm>
#include <iterator>
#include <boost/optional.hpp>

#include "PythonConverterUtils.h"

#include "global/CompilerWarnings.h"

#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/stl_iterator.hpp>

#include "model/FeatureVisitor.h"
#include "model/ModelUtils.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlHotSpotTrailMark.h"
#include "property-values/GpmlInterpolationFunction.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"
#include "property-values/XsString.h"


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
}


void
export_property_value()
{
	/*
	 * PropertyValue - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	 *
	 * Base property value wrapper class.
	 *
	 * Enables 'isinstance(obj, PropertyValue)' in python - not that it's that useful.
	 *
	 * NOTE: We don't normally return a 'PropertyValue::non_null_ptr_type' to python because then
	 * python is unable to access the attributes of the derived property value type.
	 * For this reason usually the derived property value is returned using:
	 *   'PythonConverterUtils::get_property_value_as_derived_type()'
	 */
	bp::class_<
			GPlatesModel::PropertyValue,
			GPlatesModel::PropertyValue::non_null_ptr_type,
			boost::noncopyable>(
					"PropertyValue",
					"The base class inherited by all derived property value classes. "
					"Property values are equality (``==``, ``!=``) comparable. Two property values will only "
					"compare equal if they have the same derived property value *type* (and the same internal values). "
					"For example, a :class:`GpmlPlateId` property value instance and a :class:`XsInteger` "
					"property value instance will always compare as ``False``.\n"
					"\n"
					"The list of derived property value classes includes:\n"
					"\n"
					"* :class:`GmlPoint`\n"
					"* :class:`GmlTimeInstant`\n"
					"* :class:`GmlTimePeriod`\n"
					"* :class:`GpmlConstantValue`\n"
					"* :class:`GpmlFiniteRotationSlerp`\n"
					//"* :class:`GpmlHotSpotTrailMark`\n"
					"* :class:`GpmlIrregularSampling`\n"
					"* :class:`GpmlPiecewiseAggregation`\n"
					"* :class:`GpmlPlateId`\n"
					"* :class:`XsBoolean`\n"
					"* :class:`XsDouble`\n"
					"* :class:`XsInteger`\n"
					"* :class:`XsString`\n"
					"\n"
					"The following subset of derived property value classes are time-dependent wrappers:\n"
					"\n"
					"* :class:`GpmlConstantValue`\n"
					"* :class:`GpmlIrregularSampling`\n"
					"* :class:`GpmlPiecewiseAggregation`\n"
					"\n",
					bp::no_init)
		.def("clone",
				&GPlatesApi::property_value_clone,
				"clone() -> PropertyValue\n"
				"  Create a duplicate of this property value (derived) instance, including a recursive copy "
				"of any nested property values that this instance might contain.\n"
				"\n"
				"  :rtype: :class:`PropertyValue`\n")
		.def("accept_visitor",
				&GPlatesApi::property_value_accept_visitor,
				(bp::arg("visitor")),
				"accept_visitor(visitor)\n"
				"  Accept a property value visitor so that it can visit this property value. "
				"As part of the visitor pattern, this enables the visitor instance to discover "
				"the derived class type of this property. Note that there is no common interface "
				"shared by all property value types, hence the visitor pattern provides one way "
				"to find out which type of property value is being visited.\n"
				"\n"
				"  :param visitor: the visitor instance visiting this property value\n"
				"  :type visitor: :class:`PropertyValueVisitor`\n")
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;
}

//////////////////////////////////////////////////////////////////////////
// NOTE: Please keep the property values alphabetically ordered.
//////////////////////////////////////////////////////////////////////////

void
export_geo_time_instant()
{
	//
	// GeoTimeInstant - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<GPlatesPropertyValues::GeoTimeInstant>(
			"GeoTimeInstant",
			"Represents an instant in geological time. This class is able to represent:\n"
			"\n"
			"* time-instants with a *specific* time-position relative to the present-day\n"
			"* time-instants in the *distant past*\n"
			"* time-instants in the *distant future*\n"
			"\n"
			"Note that *positive* values represent times in the *past* and *negative* values represent "
			"times in the *future*. This can be confusing at first, but the reason for this is "
			"geological times are represented by how far in the *past* to go back compared to present day.\n"
			"\n"
			"All comparison operators (==, !=, <, <=, >, >=) are supported. The comparisons are such that "
			"times further in the past are *less than* more recent times. Note that this is the opposite "
			"of comparisons of floating-point values returned by :meth:`get_value`.\n"
			"\n"
			// Note that we put the __init__ docstring in the class docstring.
			// See the comment in 'BOOST_PYTHON_MODULE(pygplates)' for an explanation...
			"GeoTimeInstant(time_value)\n"
			"  Construct a GeoTimeInstant instance from a floating-point time position.\n"
			"  ::\n"
			"\n"
			"    time_instant = pygplates.GeoTimeInstant(time_value)\n",
			bp::init<double>((bp::arg("time_value"))))
		.def("create_distant_past",
				&GPlatesPropertyValues::GeoTimeInstant::create_distant_past,
				"create_distant_past() -> GeoTimeInstant\n"
				"  Create a GeoTimeInstant instance for the distant past.\n"
				"  ::\n"
				"\n"
				"    distant_past = pygplates.GeoTimeInstant.create_distant_past()\n"
				"\n"
				"  This is basically creating a time-instant which is infinitely far in the past, "
				"as if we'd created a GeoTimeInstant with a time-position value of infinity.\n"
				"\n"
				"  All distant-past time-instants will compare earlier than all non-distant-past time-instants.\n")
		.staticmethod("create_distant_past")
		.def("create_distant_future",
				&GPlatesPropertyValues::GeoTimeInstant::create_distant_future,
				"create_distant_future() -> GeoTimeInstant\n"
				"  Create a GeoTimeInstant instance for the distant future.\n"
				"  ::\n"
				"\n"
				"    distant_past = pygplates.GeoTimeInstant.create_distant_past()\n"
				"\n"
				"  This is basically creating a time-instant which is infinitely far in the future, "
				"as if we'd created a GeoTimeInstant with a time-position value of minus-infinity.\n"
				"\n"
				"  All distant-future time-instants will compare later than all non-distant-future time-instants.\n")
		.staticmethod("create_distant_future")
		.def("get_value",
				&GPlatesPropertyValues::GeoTimeInstant::value,
				"get_value() -> float\n"
				"  Access the floating-point representation of the time-position of this instance. "
				"Units are in Ma (millions of year ago).\n"
				"\n"
				"  **NOTE** that this value may not be meaningful if :meth:`is_real` returns ``False``. "
				"Currently, if :meth:`is_distant_past` is ``True`` then *get_value* returns infinity and if "
				":meth:`is_distant_future` is ``True`` then *get_value* returns minus-infinity.\n"
				"\n"
				"  Note that positive values represent times in the past and negative values represent "
				"times in the future. So comparing values returned by *get_value* will give opposite "
				"comparison (``<``, ``>``, etc) results than comparing :class:`GeoTimeInstant` values directly. "
				"However you should only *compare* :class:`GeoTimeInstant` objects since they handle "
				"issues related to finite floating-point precision (most notably when comparing for equality).\n"
				"  ::\n"
				"\n"
				"    time10Ma = pygplates.GeoTimeInstant(10)\n"
				"    time20Ma = pygplates.GeoTimeInstant(20)\n"
				"    assert(time20Ma < time10Ma)\n"
				"    assert(time20Ma.get_value() > time10Ma.get_value())\n"
				"\n"
				"  :rtype: float\n")
		.def("is_distant_past",
				&GPlatesPropertyValues::GeoTimeInstant::is_distant_past,
				"is_distant_past() -> bool\n"
				"  Returns ``True`` if this instance is a time-instant in the distant past.\n"
				"\n"
				"  :rtype: bool\n")
		.def("is_distant_future",
				&GPlatesPropertyValues::GeoTimeInstant::is_distant_future,
				"is_distant_future() -> bool\n"
				"  Returns ``True`` if this instance is a time-instant in the distant future.\n"
				"\n"
				"  :rtype: bool\n")
		.def("is_real",
				&GPlatesPropertyValues::GeoTimeInstant::is_real,
				"is_real() -> bool\n"
				"  Returns ``True`` if this instance is a time-instant whose time-position may be "
				"expressed as a *real* floating-point number. If :meth:`is_real` is ``True`` then "
				"both :meth:`is_distant_past` and :meth:`is_distant_future` will be ``False``.\n"
				"\n"
				"  :rtype: bool\n")
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
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


namespace GPlatesApi
{
	const GPlatesPropertyValues::GmlPoint::non_null_ptr_type
	gml_point_create(
			const GPlatesMaths::PointOnSphere &point)
	{
		// Use the default value for the second argument.
		return GPlatesPropertyValues::GmlPoint::create(point);
	}

	const GPlatesMaths::PointOnSphere
	gml_point_get_point(
			GPlatesPropertyValues::GmlPoint::non_null_ptr_type gml_point)
	{
		return *gml_point->get_point();
	}

	void
	gml_point_set_point(
			GPlatesPropertyValues::GmlPoint::non_null_ptr_type gml_point,
			const GPlatesMaths::PointOnSphere &point)
	{
		gml_point->set_point(point.clone_as_point());
	}
}

void
export_gml_point()
{
	//
	// GmlPoint - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GmlPoint,
			GPlatesPropertyValues::GmlPoint::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GmlPoint",
					"A property value representing a point geometry.\n",
					bp::no_init)
		.def("create",
				&GPlatesApi::gml_point_create,
				(bp::arg("point")),
				"create(point) -> GmlPoint\n"
				"  Create a property value representing a point geometry.\n"
				"  ::\n"
				"\n"
				"    point_property = pygplates.GmlPoint.create(point)\n"
				"\n"
				"  :param point: the point geometry\n"
				"  :type point: :class:`PointOnSphere`\n")
		.staticmethod("create")
		.def("get_point",
				&GPlatesApi::gml_point_get_point,
				"get_point() -> PointOnSphere\n"
				"  Returns the point geometry of this property value.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n")
		.def("set_point",
				&GPlatesApi::gml_point_set_point,
				(bp::arg("point")),
				"set_point(point)\n"
				"  Sets the point geometry of this property value.\n"
				"\n"
				"  :param point: the point geometry\n"
				"  :type point: :class:`PointOnSphere`\n")
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GpmlPlateId,
			GPlatesModel::PropertyValue>();
}


void
export_gml_time_instant()
{
	//
	// GmlTimeInstant - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GmlTimeInstant,
			GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GmlTimeInstant",
					"A property value representing an instant in geological time.\n",
					bp::no_init)
		.def("create",
				&GPlatesModel::ModelUtils::create_gml_time_instant,
				(bp::arg("time_position")),
				"create(time_position) -> GmlTimeInstant\n"
				"  Create a property value representing a specific time instant.\n"
				"  ::\n"
				"\n"
				"    time_instant = pygplates.GmlTimeInstant.create(time_position)\n"
				"\n"
				"  :param time_position: the time position\n"
				"  :type time_position: :class:`GeoTimeInstant`\n")
		.staticmethod("create")
		.def("get_time",
				&GPlatesPropertyValues::GmlTimeInstant::get_time_position,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_time() -> GeoTimeInstant\n"
				"  Returns the time position of this property value.\n"
				"\n"
				"  :rtype: :class:`GeoTimeInstant`\n")
		.def("set_time",
				&GPlatesPropertyValues::GmlTimeInstant::set_time_position,
				(bp::arg("time_position")),
				"set_time(time_position)\n"
				"  Sets the time position of this property value.\n"
				"\n"
				"  :param time_position: the time position\n"
				"  :type time_position: :class:`GeoTimeInstant`\n")
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GmlTimeInstant,
			GPlatesModel::PropertyValue>();
}


namespace GPlatesApi
{
	GPlatesPropertyValues::GeoTimeInstant
	gml_time_period_get_begin_time(
			GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type gml_time_period)
	{
		return gml_time_period->begin()->get_time_position();
	}

	void
	gml_time_period_set_begin_time(
			GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type gml_time_period,
			const GPlatesPropertyValues::GeoTimeInstant &time_position)
	{
		gml_time_period->begin()->set_time_position(time_position);
	}

	GPlatesPropertyValues::GeoTimeInstant
	gml_time_period_get_end_time(
			GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type gml_time_period)
	{
		return gml_time_period->end()->get_time_position();
	}

	void
	gml_time_period_set_end_time(
			GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type gml_time_period,
			const GPlatesPropertyValues::GeoTimeInstant &time_position)
	{
		gml_time_period->end()->set_time_position(time_position);
	}
}

void
export_gml_time_period()
{
	//
	// GmlTimePeriod - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GmlTimePeriod,
			GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GmlTimePeriod",
					"A property value representing a period in geological time (time of appearance to time of disappearance).\n",
					bp::no_init)
		.def("create",
				&GPlatesModel::ModelUtils::create_gml_time_period,
				(bp::arg("begin_time_position"), bp::arg("end_time_position")),
				"create(begin_time_position, end_time_position) -> GmlTimePeriod\n"
				"  Create a property value representing a specific time period.\n"
				"  ::\n"
				"\n"
				"    time_period = pygplates.GmlTimePeriod.create(begin_time_position, end_time_position)\n"
				"\n"
				"  :param begin_time_position: the begin time position (time of appearance)\n"
				"  :type begin_time_position: :class:`GeoTimeInstant`\n"
				"  :param end_time_position: the end time position (time of disappearance)\n"
				"  :type end_time_position: :class:`GeoTimeInstant`\n")
		.staticmethod("create")
		.def("get_begin_time",
				&GPlatesApi::gml_time_period_get_begin_time,
				"get_begin_time() -> GeoTimeInstant\n"
				"  Returns the begin time position (time of appearance) of this property value.\n"
				"\n"
				"  :rtype: :class:`GeoTimeInstant`\n")
		.def("set_begin_time",
				&GPlatesApi::gml_time_period_set_begin_time,
				(bp::arg("time_position")),
				"set_begin_time(time_position)\n"
				"  Sets the begin time position (time of appearance) of this property value.\n"
				"\n"
				"  :param time_position: the begin time position (time of appearance)\n"
				"  :type time_position: :class:`GeoTimeInstant`\n")
		.def("get_end_time",
				&GPlatesApi::gml_time_period_get_end_time,
				"get_end_time() -> GeoTimeInstant\n"
				"  Returns the end time position (time of disappearance) of this property value.\n"
				"\n"
				"  :rtype: :class:`GeoTimeInstant`\n")
		.def("set_end_time",
				&GPlatesApi::gml_time_period_set_end_time,
				(bp::arg("time_position")),
				"set_end_time(time_position)\n"
				"  Sets the end time position (time of disappearance) of this property value.\n"
				"\n"
				"  :param time_position: the end time position (time of disappearance)\n"
				"  :type time_position: :class:`GeoTimeInstant`\n")
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GmlTimePeriod,
			GPlatesModel::PropertyValue>();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type
	gpml_constant_value_create(
			GPlatesModel::PropertyValue::non_null_ptr_type property_value,
			boost::optional<GPlatesUtils::UnicodeString> description = boost::none)
	{
		// ModelUtils takes care of determining the structural type of the nested property value.
		return GPlatesModel::ModelUtils::create_gpml_constant_value(property_value, description);
	}

DISABLE_GCC_WARNING("-Wshadow")
	// Default argument overloads of 'GPlatesPropertyValues::GpmlConstantValue::create'.
	BOOST_PYTHON_FUNCTION_OVERLOADS(
			gpml_constant_value_create_overloads,
			gpml_constant_value_create, 1, 2)
ENABLE_GCC_WARNING("-Wshadow")

	// Return base property value to python as its derived property value type.
	bp::object/*derived property value non_null_intrusive_ptr*/
	gpml_constant_value_get_value(
			GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type gpml_constant_value)
	{
		// The derived property value type is needed otherwise python is unable to access the derived attributes.
		return PythonConverterUtils::get_property_value_as_derived_type(gpml_constant_value->value());
	}
}

void
export_gpml_constant_value()
{
	//
	// GpmlConstantValue - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlConstantValue,
			GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlConstantValue",
					"The most basic case of a time-dependent property value is one "
					"that is constant for all time. The other two types are :class:`GpmlIrregularSampling` "
					"and :class:`GpmlPiecewiseAggregation`. The GPlates Geological Information Model (GPGIM) "
					"defines those properties that are time-dependent (see http://www.gplates.org/gpml.html) and "
					"those that are not. For example, a :class:`GpmlPlateId` property value is used "
					"in *gpml:reconstructionPlateId* properties, of general :class:`feature types<FeatureType>`, and also in "
					"*gpml:relativePlate* properties of motion path features. In the former case "
					"it is expected to be wrapped in a :class:`GpmlConstantValue` while in the latter "
					"case it is not.\n"
					"  ::\n"
					"\n"
					"    reconstruction_plate_id = pygplates.Property.create(\n"
					"        pygplates.PropertyName.create_gpml('reconstructionPlateId'),\n"
					"        pygplates.GpmlConstantValue.create(\n"
					"            pygplates.GpmlPlateId.create(701)))\n"
					"\n"
					"    relative_plate_id = pygplates.Property.create(\n"
					"        pygplates.PropertyName.create_gpml('relativePlate'),\n"
					"        pygplates.GpmlPlateId.create(701))\n"
					"\n"
					"If a property is created without a time-dependent wrapper where one is expected, "
					"or vice versa, then you can still save it to a GPML file and a subsequent read "
					"of that file will attempt to correct the property when it is created during "
					"the reading phase (by the GPML file format reader). This usually works for the "
					"simpler :class:`GpmlConstantValue` time-dependent wrapper but does not always "
					"work for the more advanced :class:`GpmlIrregularSampling` and "
					":class:`GpmlPiecewiseAggregation` time-dependent wrapper types.\n",
					bp::no_init)
		.def("create",
				&GPlatesApi::gpml_constant_value_create,
				GPlatesApi::gpml_constant_value_create_overloads(
					(bp::arg("property_value"),
						bp::arg("description") = boost::optional<GPlatesUtils::UnicodeString>()),
					"create(property_value[, description=None]) -> GpmlConstantValue\n"
					"  Wrap a property value in a time-dependent wrapper that identifies the "
					"property value as constant for all time. Optionally provide a description string.\n"
					"  ::\n"
					"\n"
					"    constant_property_value = pygplates.GpmlConstantValue.create(property_value)\n"
					"\n"
					"  If *description* is ``None`` then :meth:`get_description` will return ``None``.\n"
					"\n"
					"  :param property_value: arbitrary property value\n"
					"  :type property_value: :class:`PropertyValue`\n"
					"  :param description: description of this constant value wrapper\n"
					"  :type description: string or None\n"))
		.staticmethod("create")
		.def("get_value",
				&GPlatesApi::gpml_constant_value_get_value,
				"get_value() -> PropertyValue\n"
				"  Returns the property value contained in this constant value wrapper.\n"
				"\n"
				"  :rtype: :class:`PropertyValue`\n")
		.def("set_value",
				&GPlatesPropertyValues::GpmlConstantValue::set_value,
				(bp::arg("property_value")),
				"set_value(property_value)\n"
				"  Sets the property value of this constant value wrapper. "
				"This essentially replaces the previous property value. "
				"Note that an alternative is to directly modify the property value returned by :meth:`get_value` "
				"using its property value methods.\n"
				"\n"
				"  :param property_value: arbitrary property value\n"
				"  :type property_value: :class:`PropertyValue`\n")
		.def("get_description",
				&GPlatesPropertyValues::GpmlConstantValue::get_description,
				"get_description() -> string or None\n"
				"  Returns the *optional* description of this constant value wrapper, or ``None``.\n"
				"\n"
				"  :rtype: string or None\n")
		.def("set_description",
				&GPlatesPropertyValues::GpmlConstantValue::set_description,
				(bp::arg("description") = boost::optional<GPlatesUtils::UnicodeString>()),
				"set_description([description=None])\n"
				"  Sets the description of this constant value wrapper, or removes it if ``None`` specified.\n"
				"\n"
				"  :param description: description of this constant value wrapper\n"
				"  :type description: string or None\n")
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GpmlConstantValue,
			GPlatesModel::PropertyValue>();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GpmlFiniteRotationSlerp::non_null_ptr_type
	gpml_finite_rotation_slerp_create()
	{
		return GPlatesPropertyValues::GpmlFiniteRotationSlerp::create(
				GPlatesPropertyValues::StructuralType::create_gpml("FiniteRotation"));
	}
}

void
export_gpml_finite_rotation_slerp()
{
	//
	// GpmlFiniteRotationSlerp - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlFiniteRotationSlerp,
			GPlatesPropertyValues::GpmlFiniteRotationSlerp::non_null_ptr_type,
			bp::bases<GPlatesPropertyValues::GpmlInterpolationFunction>,
			boost::noncopyable>(
					"GpmlFiniteRotationSlerp",
					"An interpolation function designed to interpolate between finite rotations.\n"
					"\n"
					"There are no (non-static) methods or attributes in this class. The presence of an instance of this "
					"property value is simply intended to signal that interpolation should be Spherical "
					"Linear intERPolation (SLERP). Currently this is the only type of interpolation function "
					"(the only type derived from :class:`GpmlInterpolationFunction`).\n",
					bp::no_init)
		.def("create",
				&GPlatesApi::gpml_finite_rotation_slerp_create,
				"create() -> GpmlFiniteRotationSlerp\n"
				"  Create an instance of GpmlFiniteRotationSlerp.\n"
				"  ::\n"
				"\n"
				"    finite_rotation_slerp = pygplates.GpmlFiniteRotationSlerp.create()\n")
		.staticmethod("create")
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class GpmlInterpolationFunction.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GpmlFiniteRotationSlerp,
			GPlatesPropertyValues::GpmlInterpolationFunction>();
}


void
export_gpml_hot_spot_trail_mark()
{
#if 0
	// Use the 'non-const' overload so GmlTimeInstant can be modified via python...
	const boost::optional<GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type>
			(GPlatesPropertyValues::GpmlHotSpotTrailMark::*measured_age)() =
					&GPlatesPropertyValues::GpmlHotSpotTrailMark::measured_age;

	//
	// GpmlHotSpotTrailMark - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlHotSpotTrailMark,
			GPlatesPropertyValues::GpmlHotSpotTrailMark::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlHotSpotTrailMark",
					"The marks that define the HotSpotTrail.\n",
					bp::no_init)
		//.def("create", &GPlatesPropertyValues::GpmlHotSpotTrailMark::create)
		//.staticmethod("create")
		//.def("position", &GPlatesPropertyValues::GpmlHotSpotTrailMark::position)
		//.def("set_position", &GPlatesPropertyValues::GpmlHotSpotTrailMark::set_position)
		.def("get_measured_age", measured_age)
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GpmlHotSpotTrailMark,
			GPlatesModel::PropertyValue>();
#endif
}


void
export_gpml_interpolation_function()
{
	/*
	 * GpmlInterpolationFunction - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	 *
	 * Base class for interpolation function property values.
	 *
	 * Enables 'isinstance(obj, GpmlInterpolationFunction)' in python - not that it's that useful.
	 *
	 * NOTE: We don't return a 'GpmlInterpolationFunction::non_null_ptr_type' to python because then
	 * python is unable to access the attributes of the derived interpolation function property value type.
	 * For this reason usually the derived interpolation function property value is returned using:
	 *   'PythonConverterUtils::get_property_value_as_derived_type()'
	 */
	bp::class_<
			GPlatesPropertyValues::GpmlInterpolationFunction,
			GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlInterpolationFunction",
					"The base class inherited by all derived *interpolation function* property value classes.\n"
					"\n"
					"The list of derived interpolation function property value classes includes:\n"
					"\n"
					"* :class:`GpmlFiniteRotationSlerp`\n",
					bp::no_init)
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GpmlInterpolationFunction,
			GPlatesModel::PropertyValue>();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type
	gpml_irregular_sampling_create(
			bp::object time_samples, // Any python sequence (eg, list, tuple).
			boost::optional<GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type>
					interpolation_function = boost::none)
	{
		// Begin/end iterators over the python time samples sequence.
		bp::stl_input_iterator<GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type>
				time_samples_begin(time_samples),
				time_samples_end;

		// Copy into a vector.
		std::vector<GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type> time_samples_vector;
		std::copy(time_samples_begin, time_samples_end, std::back_inserter(time_samples_vector));

		// We need at least one time sample to determine the value type, otherwise we need to
		// ask the python user for it and that might be a little confusing for them.
		if (time_samples_vector.empty())
		{
			PyErr_SetString(
					PyExc_RuntimeError,
					"pygplates.GpmlIrregularSampling::create() requires a non-empty "
						"sequence of GpmlTimeSample elements");
			bp::throw_error_already_set();
		}

		return GPlatesPropertyValues::GpmlIrregularSampling::create(
				time_samples_vector,
				interpolation_function,
				time_samples_vector[0]->get_value_type());
	}

DISABLE_GCC_WARNING("-Wshadow")
	// Default argument overloads of 'GPlatesPropertyValues::v::create'.
	BOOST_PYTHON_FUNCTION_OVERLOADS(
			gpml_irregular_sampling_create_overloads,
			gpml_irregular_sampling_create, 1, 2)
ENABLE_GCC_WARNING("-Wshadow")

	const GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeSample>::non_null_ptr_type
	gpml_irregular_sampling_get_time_samples(
			GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type gpml_irregular_sampling)
	{
		return &gpml_irregular_sampling->time_samples();
	}
}

void
export_gpml_irregular_sampling()
{
	// Use the 'non-const' overload...
	const boost::optional<GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type>
			(GPlatesPropertyValues::GpmlIrregularSampling::*get_interpolation_function)() =
					&GPlatesPropertyValues::GpmlIrregularSampling::interpolation_function;

	//
	// GpmlIrregularSampling - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlIrregularSampling,
			GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlIrregularSampling",
					"A time-dependent property consisting of a sequence of time samples irregularly spaced in time.\n",
					bp::no_init)
		.def("create",
				&GPlatesApi::gpml_irregular_sampling_create,
				GPlatesApi::gpml_irregular_sampling_create_overloads(
					(bp::arg("time_samples"),
						bp::arg("interpolation_function") =
							boost::optional<GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type>()),
					"create(time_samples[, interpolation_function=None]) -> GpmlIrregularSampling\n"
					"  Create an irregularly sampled time-dependent property from a sequence of time samples. "
					"Optionally provide an interpolation function.\n"
					"\n"
					"  **NOTE** that the sequence of time samples must **not** be empty (for technical implementation reasons), "
					"otherwise a *RuntimeError* exception will be thrown.\n"
					"  ::\n"
					"\n"
					"    irregular_sampling = pygplates.GpmlIrregularSampling.create(time_samples)\n"
					"\n"
					"  :param time_samples: A sequence of :class:`GpmlTimeSample` elements.\n"
					"  :type time_samples: Any python sequence such as a ``list`` or a ``tuple``\n"
					"  :param interpolation_function: identifies function used to interpolate\n"
					"  :type interpolation_function: an instance derived from :class:`GpmlInterpolationFunction`\n"))
		.staticmethod("create")
		.def("get_time_samples",
				&GPlatesApi::gpml_irregular_sampling_get_time_samples,
				"get_time_samples() -> GpmlTimeSampleList\n"
				"  Returns the :class:`time samples<GpmlTimeSampleList>` in a sequence that behaves as a python ``list``. "
				"Modifying the returned sequence will modify the internal state of the *GpmlIrregularSampling* "
				"instance.\n"
				"  ::\n"
				"\n"
				"    time_samples = irregular_sampling.get_time_samples()\n"
				"\n"
				"    # Sort samples by time ('reverse=True' orders backwards in time from present day to past times)\n"
				"    time_samples.sort(key = lambda x: x.get_time(), reverse = True)\n"
				"\n"
				"  :rtype: :class:`GpmlTimeSampleList`\n")
		.def("get_interpolation_function",
				get_interpolation_function,
				"get_interpolation_function() -> GpmlInterpolationFunction\n"
				"  Returns the function used to interpolate between time samples, or ``None``.\n"
				"\n"
				"  :rtype: an instance derived from :class:`GpmlInterpolationFunction`, or ``None``\n")
		.def("set_interpolation_function",
				&GPlatesPropertyValues::GpmlIrregularSampling::set_interpolation_function,
				(bp::arg("interpolation_function") =
					boost::optional<GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type>()),
				"set_interpolation_function([interpolation_function=None])\n"
				"  Sets the function used to interpolate between time samples, "
				"or removes it if ``None`` specified.\n"
				"\n"
				"  :param interpolation_function: the function used to interpolate between time samples\n"
				"  :type interpolation_function: an instance derived from :class:`GpmlInterpolationFunction`, or None\n")
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GpmlIrregularSampling,
			GPlatesModel::PropertyValue>();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type
	gpml_piecewise_aggregation_create(
			bp::object time_windows) // Any python sequence (eg, list, tuple).
	{
		// Begin/end iterators over the python time windows sequence.
		bp::stl_input_iterator<GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type>
				time_windows_begin(time_windows),
				time_windows_end;

		// Copy into a vector.
		std::vector<GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type> time_windows_vector;
		std::copy(time_windows_begin, time_windows_end, std::back_inserter(time_windows_vector));

		// We need at least one time sample to determine the value type, otherwise we need to
		// ask the python user for it and that might be a little confusing for them.
		if (time_windows_vector.empty())
		{
			PyErr_SetString(
					PyExc_RuntimeError,
					"pygplates.GpmlPiecewiseAggregation::create() requires a non-empty "
						"sequence of GpmlTimeWindow elements");
			bp::throw_error_already_set();
		}

		return GPlatesPropertyValues::GpmlPiecewiseAggregation::create(
				time_windows_vector,
				time_windows_vector[0]->get_value_type());
	}

	const GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow>::non_null_ptr_type
	gpml_piecewise_aggregation_get_time_windows(
			GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type gpml_piecewise_aggregation)
	{
		return &gpml_piecewise_aggregation->time_windows();
	}
}

void
export_gpml_piecewise_aggregation()
{
	//
	// GpmlPiecewiseAggregation - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlPiecewiseAggregation,
			GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlPiecewiseAggregation",
					"A time-dependent property consisting of a sequence of time windows each with a *constant* "
					"property value.\n",
					bp::no_init)
		.def("create",
				&GPlatesApi::gpml_piecewise_aggregation_create,
				(bp::arg("time_windows")),
				"create(time_windows) -> GpmlPiecewiseAggregation\n"
				"  Create a piecewise-constant time-dependent property from a sequence of time windows.\n"
				"\n"
				"  **NOTE** that the sequence of time windows must **not** be empty (for technical implementation reasons), "
				"otherwise a *RuntimeError* exception will be thrown.\n"
				"  ::\n"
				"\n"
				"    piecewise_aggregation = pygplates.GpmlPiecewiseAggregation.create(time_windows)\n"
				"\n"
				"  :param time_windows: A sequence of :class:`GpmlTimeWindow` elements.\n"
				"  :type time_windows: Any python sequence such as a ``list`` or a ``tuple``\n")
		.staticmethod("create")
		.def("get_time_windows",
				&GPlatesApi::gpml_piecewise_aggregation_get_time_windows,
				"get_time_windows() -> GpmlTimeWindowList\n"
				"  Returns the :class:`time windows<GpmlTimeWindowList>` in a sequence that behaves as a python ``list``. "
				"Modifying the returned sequence will modify the internal state of the *GpmlPiecewiseAggregation* "
				"instance.\n"
				"  ::\n"
				"\n"
				"    time_windows = piecewise_aggregation.get_time_windows()\n"
				"\n"
				"    # Sort windows by begin time ('reverse=True' orders backwards in time from present day to past times)\n"
				"    time_windows.sort(key = lambda x: x.get_begin_time(), reverse = True)\n"
				"\n"
				"  :rtype: :class:`GpmlTimeWindowList`\n")
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GpmlPiecewiseAggregation,
			GPlatesModel::PropertyValue>();
}


void
export_gpml_plate_id()
{
	//
	// GpmlPlateId - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlPlateId,
			GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlPlateId",
					"A property value that represents a plate id. A plate id is an integer that "
					"identifies a particular tectonic plate and is typically used to look up a "
					"rotation in a :class:`ReconstructionTree`.",
					bp::no_init)
		.def("create",
				&GPlatesPropertyValues::GpmlPlateId::create,
				(bp::arg("plate_id")),
				"create(plate_id) -> GpmlPlateId\n"
				"  Create a plate id property value from an integer plate id.\n"
				"  ::\n"
				"\n"
				"    plate_id_property = pygplates.GpmlPlateId.create(plate_id)\n"
				"\n"
				"  :param plate_id: integer plate id\n"
				"  :type plate_id: int\n")
		.staticmethod("create")
		.def("get_plate_id",
				&GPlatesPropertyValues::GpmlPlateId::get_value,
				"get_plate_id() -> int\n"
				"  Returns the integer plate id.\n"
				"\n"
				"  :rtype: int\n")
		.def("set_plate_id",
				&GPlatesPropertyValues::GpmlPlateId::set_value,
				(bp::arg("plate_id")),
				"set_plate_id(plate_id)\n"
				"  Sets the integer plate id.\n"
				"\n"
				"  :param plate_id: integer plate id\n"
				"  :type plate_id: int\n")
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GpmlPlateId,
			GPlatesModel::PropertyValue>();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type
	gpml_time_sample_create(
			GPlatesModel::PropertyValue::non_null_ptr_type property_value,
			const GPlatesPropertyValues::GeoTimeInstant &time,
			boost::optional<GPlatesPropertyValues::TextContent> description = boost::none,
			bool is_disabled = false)
	{
		boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_type> description_xs_string;
		if (description)
		{
			description_xs_string = GPlatesPropertyValues::XsString::create(description->get());
		}

		return GPlatesPropertyValues::GpmlTimeSample::create(
				property_value,
				GPlatesModel::ModelUtils::create_gml_time_instant(time),
				description_xs_string,
				property_value->get_structural_type(),
				is_disabled);
	}

DISABLE_GCC_WARNING("-Wshadow")
	// Default argument overloads of 'GPlatesPropertyValues::GpmlTimeSample::create'.
	BOOST_PYTHON_FUNCTION_OVERLOADS(
			gpml_time_sample_create_overloads,
			gpml_time_sample_create, 2, 4)
ENABLE_GCC_WARNING("-Wshadow")

	// Return base property value to python as its derived property value type.
	bp::object/*derived property value non_null_intrusive_ptr*/
	gpml_time_sample_get_value(
			GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type gpml_time_sample)
	{
		// The derived property value type is needed otherwise python is unable to access the derived attributes.
		return PythonConverterUtils::get_property_value_as_derived_type(gpml_time_sample->value());
	}

	GPlatesPropertyValues::GeoTimeInstant
	gpml_time_sample_get_time(
			GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type gpml_time_sample)
	{
		return gpml_time_sample->valid_time()->get_time_position();
	}

	void
	gpml_time_sample_set_time(
			GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type gpml_time_sample,
			const GPlatesPropertyValues::GeoTimeInstant &time_position)
	{
		gpml_time_sample->valid_time()->set_time_position(time_position);
	}

	// Make it easier for client by converting from XsString to a regular string.
	boost::optional<GPlatesPropertyValues::TextContent>
	gpml_time_sample_get_description(
			GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type gpml_time_sample)
	{
		boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_type> xs_string =
				gpml_time_sample->description();
		if (!xs_string)
		{
			return boost::none;
		}

		return xs_string.get()->get_value();
	}

	// Make it easier for client by converting from a regular string to XsString.
	void
	gpml_time_sample_set_description(
			GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type gpml_time_sample,
			boost::optional<GPlatesPropertyValues::TextContent> description)
	{
		boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_type> xs_string;
		if (description)
		{
			xs_string = GPlatesPropertyValues::XsString::create(description->get());
		}

		gpml_time_sample->set_description(xs_string);
	}
}

void
export_gpml_time_sample()
{
	//
	// GpmlTimeSample - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlTimeSample,
			GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type,
			boost::noncopyable>(
					"GpmlTimeSample",
					"A time sample associates an arbitrary property value with a specific time instant. "
					"Typically a sequence of time samples are used in a :class:`GpmlIrregularSampling`. "
					"The most common example of this is a time-dependent sequence of total reconstruction poles.\n"
					"\n"
					"Time samples are equality (``==``, ``!=``) comparable. This includes comparing the property value "
					"in the two time samples being compared (see :class:`PropertyValue`) as well as the time instant, "
					"description string and disabled flag.\n",
					bp::no_init)
		.def("create",
				&GPlatesApi::gpml_time_sample_create,
				GPlatesApi::gpml_time_sample_create_overloads(
					(bp::arg("property_value"),
						bp::arg("time"),
						bp::arg("description") = boost::optional<GPlatesPropertyValues::TextContent>(),
						bp::arg("is_disabled") = false),
					"create(property_value, time[, description=None[, is_disabled=False]]) -> GpmlTimeSample\n"
					"  Create a time sample given a property value and time and optionally a description string "
					"and disabled flag.\n"
					"  ::\n"
					"\n"
					"    time_sample = pygplates.GpmlTimeSample.create(property_value, time)\n"
					"\n"
					"  :param property_value: arbitrary property value\n"
					"  :type property_value: :class:`PropertyValue`\n"
					"  :param time: the time position associated with the property value\n"
					"  :type time: :class:`GeoTimeInstant`\n"
					"  :param description: description of the time sample\n"
					"  :type description: string or None\n"
					"  :param is_disabled: whether time sample is disabled or not\n"
					"  :type is_disabled: bool\n"))
		.staticmethod("create")
		.def("get_value",
				&GPlatesApi::gpml_time_sample_get_value,
				"get_value() -> PropertyValue\n"
				"  Returns the property value of this time sample.\n"
				"\n"
				"  :rtype: :class:`PropertyValue`\n")
		.def("set_value",
				&GPlatesPropertyValues::GpmlTimeSample::set_value,
				(bp::arg("property_value")),
				"set_value(property_value)\n"
				"  Sets the property value associated with this time sample. "
				"This essentially replaces the previous property value. "
				"Note that an alternative is to directly modify the property value returned by :meth:`get_value` "
				"using its property value methods.\n"
				"\n"
				"  :param property_value: arbitrary property value\n"
				"  :type property_value: :class:`PropertyValue`\n")
		.def("get_time",
				&GPlatesApi::gpml_time_sample_get_time,
				"get_time() -> GeoTimeInstant\n"
				"  Returns the time position of this time sample.\n"
				"\n"
				"  :rtype: :class:`GeoTimeInstant`\n")
		.def("set_time",
				&GPlatesApi::gpml_time_sample_set_time,
				(bp::arg("time")),
				"set_time(time)\n"
				"  Sets the time position associated with this time sample.\n"
				"\n"
				"  :param time: the time position associated with the property value\n"
				"  :type time: :class:`GeoTimeInstant`\n")
		.def("get_description",
				&GPlatesApi::gpml_time_sample_get_description,
				"get_description() -> string or None\n"
				"  Returns the description of this time sample, or ``None``.\n"
				"\n"
				"  :rtype: string or None\n")
		.def("set_description",
				&GPlatesApi::gpml_time_sample_set_description,
				(bp::arg("description") = boost::optional<GPlatesPropertyValues::TextContent>()),
				"set_description([description=None])\n"
				"  Sets the description associated with this time sample, or removes it if ``None`` specified.\n"
				"\n"
				"  :param description: description of the time sample\n"
				"  :type description: string or None\n")
		.def("is_disabled",
				&GPlatesPropertyValues::GpmlTimeSample::is_disabled,
				"is_disabled() -> bool\n"
				"  Returns whether this time sample is disabled or not. "
				"For example, a disabled total reconstruction pole (in a GpmlIrregularSampling sequence) "
				"is ignored when interpolating rotations at some arbitrary time.\n"
				"\n"
				"  :rtype: bool\n")
		.def("set_disabled",
				&GPlatesPropertyValues::GpmlTimeSample::set_disabled,
				(bp::arg("is_disabled")),
				"set_disabled(is_disabled)\n"
				"  Sets whether this time sample is disabled or not.\n"
				"\n"
				"  :param is_disabled: whether time sample is disabled or not\n"
				"  :type is_disabled: bool\n")
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Enable boost::optional<GpmlTimeSample::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type>();

	// Registers 'non-const' to 'const' conversions.
	boost::python::implicitly_convertible<
			GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type,
			GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_to_const_type>();
	boost::python::implicitly_convertible<
			boost::optional<GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type>,
			boost::optional<GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_to_const_type> >();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type
	gpml_time_window_create(
			GPlatesModel::PropertyValue::non_null_ptr_type property_value,
			const GPlatesPropertyValues::GeoTimeInstant &begin_time,
			const GPlatesPropertyValues::GeoTimeInstant &end_time)
	{
		return GPlatesPropertyValues::GpmlTimeWindow::create(
				property_value,
				GPlatesModel::ModelUtils::create_gml_time_period(begin_time, end_time),
				property_value->get_structural_type());
	}

	// Return base property value to python as its derived property value type.
	bp::object/*derived property value non_null_intrusive_ptr*/
	gpml_time_window_get_value(
			GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type gpml_time_window)
	{
		// The derived property value type is needed otherwise python is unable to access the derived attributes.
		return PythonConverterUtils::get_property_value_as_derived_type(gpml_time_window->time_dependent_value());
	}

	GPlatesPropertyValues::GeoTimeInstant
	gpml_time_window_get_begin_time(
			GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type gpml_time_window)
	{
		return gpml_time_window->valid_time()->begin()->get_time_position();
	}

	void
	gpml_time_window_set_begin_time(
			GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type gpml_time_window,
			const GPlatesPropertyValues::GeoTimeInstant &begin_time)
	{
		gpml_time_window->valid_time()->begin()->set_time_position(begin_time);
	}

	GPlatesPropertyValues::GeoTimeInstant
	gpml_time_window_get_end_time(
			GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type gpml_time_window)
	{
		return gpml_time_window->valid_time()->end()->get_time_position();
	}

	void
	gpml_time_window_set_end_time(
			GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type gpml_time_window,
			const GPlatesPropertyValues::GeoTimeInstant &end_time)
	{
		gpml_time_window->valid_time()->end()->set_time_position(end_time);
	}
}

void
export_gpml_time_window()
{
	//
	// GpmlTimeWindow - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlTimeWindow,
			GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type,
			boost::noncopyable>(
					"GpmlTimeWindow",
					"A time window associates an arbitrary property value with a specific time period. "
					"The property value does not vary over the time period of the window. "
					"Typically a sequence of time windows are used in a :class:`GpmlPiecewiseAggregation` "
					"where the sequence of windows form a *piecewise-constant* (staircase function) "
					"property value over time (since each time window has a *constant* property value) "
					"assuming the windows do not have overlaps or gaps in time.\n"
					"\n"
					"Time windows are equality (``==``, ``!=``) comparable. This includes comparing the property value "
					"in the two time windows being compared (see :class:`PropertyValue`) as well as the time period.\n",
					bp::no_init)
		.def("create",
				&GPlatesApi::gpml_time_window_create,
				(bp::arg("property_value"), bp::arg("begin_time"), bp::arg("end_time")),
				"create(property_value, begin_time, end_time) -> GpmlTimeWindow\n"
				"  Create a time window given a property value and time range.\n"
				"  ::\n"
				"\n"
				"    time_window = pygplates.GpmlTimeWindow.create(property_value, begin_time, end_time)\n"
				"\n"
				"  Note that *begin_time* must be further in the past than the *end_time* "
				"``begin_time < end_time``.\n"
				"\n"
				"  :param property_value: arbitrary property value\n"
				"  :type property_value: :class:`PropertyValue`\n"
				"  :param begin_time: the begin time of the time window\n"
				"  :type begin_time: :class:`GeoTimeInstant`\n"
				"  :param end_time: the end time of the time window\n"
				"  :type end_time: :class:`GeoTimeInstant`\n")
		.staticmethod("create")
		.def("get_value",
				&GPlatesApi::gpml_time_window_get_value,
				"get_value() -> PropertyValue\n"
				"  Returns the property value of this time window.\n"
				"\n"
				"  :rtype: :class:`PropertyValue`\n")
		.def("set_value",
				&GPlatesPropertyValues::GpmlTimeWindow::set_time_dependent_value,
				(bp::arg("property_value")),
				"set_value(property_value)\n"
				"  Sets the property value associated with this time window. "
				"This essentially replaces the previous property value. "
				"Note that an alternative is to directly modify the property value returned by :meth:`get_value` "
				"using its property value methods.\n"
				"\n"
				"  :param property_value: arbitrary property value\n"
				"  :type property_value: :class:`PropertyValue`\n")
		.def("get_begin_time",
				&GPlatesApi::gpml_time_window_get_begin_time,
				"get_begin_time() -> GeoTimeInstant\n"
				"  Returns the begin time of this time window.\n"
				"\n"
				"  :rtype: :class:`GeoTimeInstant`\n")
		.def("set_begin_time",
				&GPlatesApi::gpml_time_window_set_begin_time,
				(bp::arg("time")),
				"set_begin_time(time)\n"
				"  Sets the begin time of this time window.\n"
				"\n"
				"  :param time: the begin time of this time window\n"
				"  :type time: :class:`GeoTimeInstant`\n")
		.def("get_end_time",
				&GPlatesApi::gpml_time_window_get_end_time,
				"get_end_time() -> GeoTimeInstant\n"
				"  Returns the end time of this time window.\n"
				"\n"
				"  :rtype: :class:`GeoTimeInstant`\n")
		.def("set_end_time",
				&GPlatesApi::gpml_time_window_set_end_time,
				(bp::arg("time")),
				"set_end_time(time)\n"
				"  Sets the end time of this time window.\n"
				"\n"
				"  :param time: the end time of this time window\n"
				"  :type time: :class:`GeoTimeInstant`\n")
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Enable boost::optional<GpmlTimeWindow::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type>();

	// Registers 'non-const' to 'const' conversions.
	boost::python::implicitly_convertible<
			GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type,
			GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_to_const_type>();
	boost::python::implicitly_convertible<
			boost::optional<GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type>,
			boost::optional<GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_to_const_type> >();
}


void
export_xs_boolean()
{
	//
	// XsBoolean - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::XsBoolean,
			GPlatesPropertyValues::XsBoolean::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"XsBoolean",
					"A property value that represents a boolean value. "
					"The 'Xs' prefix is there since this type of property value is associated with the "
					"*XML Schema Instance Namespace*.\n",
					bp::no_init)
		.def("create",
				&GPlatesPropertyValues::XsBoolean::create,
				(bp::arg("boolean_value")),
				"create(boolean_value) -> XsBoolean\n"
				"  Create a boolean property value from a boolean value.\n"
				"  ::\n"
				"\n"
				"    boolean_property = pygplates.XsBoolean.create(boolean_value)\n"
				"\n"
				"  :param boolean_value: the boolean value\n"
				"  :type boolean_value: bool\n")
		.staticmethod("create")
		.def("get_boolean",
				&GPlatesPropertyValues::XsBoolean::get_value,
				"get_boolean() -> bool\n"
				"  Returns the boolean value.\n"
				"\n"
				"  :rtype: bool\n")
		.def("set_boolean",
				&GPlatesPropertyValues::XsBoolean::set_value,
				(bp::arg("boolean_value")),
				"set_boolean(boolean_value)\n"
				"  Sets the boolean value.\n"
				"\n"
				"  :param boolean_value: the boolean value\n"
				"  :type boolean_value: bool\n")
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::XsBoolean,
			GPlatesModel::PropertyValue>();
}


void
export_xs_double()
{
	//
	// XsDouble - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::XsDouble,
			GPlatesPropertyValues::XsDouble::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"XsDouble",
					"A property value that represents a *double*-precision floating-point number. "
					"Note that, in python, the ``float`` built-in type is double-precision. "
					"The 'Xs' prefix is there since this type of property value is associated with the "
					"*XML Schema Instance Namespace*.\n",
					bp::no_init)
		.def("create",
				&GPlatesPropertyValues::XsDouble::create,
				(bp::arg("float_value")),
				"create(float_value) -> XsDouble\n"
				"  Create a floating-point property value from a ``float``.\n"
				"  ::\n"
				"\n"
				"    float_property = pygplates.XsDouble.create(float_value)\n"
				"\n"
				"  :param float_value: the floating-point value\n"
				"  :type float_value: float\n")
		.staticmethod("create")
		.def("get_double",
				&GPlatesPropertyValues::XsDouble::get_value,
				"get_double() -> float\n"
				"  Returns the floating-point value.\n"
				"\n"
				"  :rtype: float\n")
		.def("set_double",
				&GPlatesPropertyValues::XsDouble::set_value,
				(bp::arg("float_value")),
				"set_double(float_value)\n"
				"  Sets the floating-point value.\n"
				"\n"
				"  :param float_value: the floating-point value\n"
				"  :type float_value: float\n")
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::XsDouble,
			GPlatesModel::PropertyValue>();
}


void
export_xs_integer()
{
	//
	// XsInteger - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::XsInteger,
			GPlatesPropertyValues::XsInteger::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"XsInteger",
					"A property value that represents an integer number. "
					"The 'Xs' prefix is there since this type of property value is associated with the "
					"*XML Schema Instance Namespace*.\n",
					bp::no_init)
		.def("create",
				&GPlatesPropertyValues::XsInteger::create,
				(bp::arg("integer_value")),
				"create(integer_value) -> XsInteger\n"
				"  Create an integer property value from an ``int``.\n"
				"  ::\n"
				"\n"
				"    integer_property = pygplates.XsInteger.create(integer_value)\n"
				"\n"
				"  :param integer_value: the integer value\n"
				"  :type integer_value: int\n")
		.staticmethod("create")
		.def("get_integer",
				&GPlatesPropertyValues::XsInteger::get_value,
				"get_integer() -> int\n"
				"  Returns the integer value.\n"
				"\n"
				"  :rtype: int\n")
		.def("set_integer",
				&GPlatesPropertyValues::XsInteger::set_value,
				(bp::arg("integer_value")),
				"set_integer(integer_value)\n"
				"  Sets the integer value.\n"
				"\n"
				"  :param integer_value: the integer value\n"
				"  :type integer_value: int\n")
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::XsInteger,
			GPlatesModel::PropertyValue>();
}


void
export_xs_string()
{
	//
	// XsString - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::XsString,
			GPlatesPropertyValues::XsString::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"XsString",
					"A property value that represents a string. "
					"The 'Xs' prefix is there since this type of property value is associated with the "
					"*XML Schema Instance Namespace*.\n",
					bp::no_init)
		.def("create",
				&GPlatesPropertyValues::XsString::create,
				(bp::arg("string")),
				"create(string) -> XsString\n"
				"  Create a string property value from a string.\n"
				"  ::\n"
				"\n"
				"    string_property = pygplates.XsString.create(string)\n"
				"\n"
				"  :param string: the string\n"
				"  :type string: string\n")
		.staticmethod("create")
		.def("get_string",
				&GPlatesPropertyValues::XsString::get_value,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_string() -> string\n"
				"  Returns the string.\n"
				"\n"
				"  :rtype: string\n")
		.def("set_string",
				&GPlatesPropertyValues::XsString::set_value,
				(bp::arg("string")),
				"set_string(string)\n"
				"  Sets the string.\n"
				"\n"
				"  :param string: the string\n"
				"  :type string: string\n")
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::XsString,
			GPlatesModel::PropertyValue>();
}


void
export_property_values()
{
	// Since PropertyValue is the base class it must be registered first to avoid runtime error.
	export_property_value();

	//////////////////////////////////////////////////////////////////////////
	// NOTE: Please keep the property values alphabetically ordered.
	//       Unless there are inheritance dependencies.
	//////////////////////////////////////////////////////////////////////////

	export_geo_time_instant();

	export_gml_point();
	export_gml_time_instant();
	export_gml_time_period();

	export_gpml_constant_value();

	// GpmlInterpolationFunction and its derived classes.
	// Since GpmlInterpolationFunction is the base class it must be registered first to avoid runtime error.
	export_gpml_interpolation_function();
	export_gpml_finite_rotation_slerp();

	export_gpml_hot_spot_trail_mark();
	export_gpml_irregular_sampling();
	export_gpml_piecewise_aggregation();
	export_gpml_plate_id();
	export_gpml_time_sample(); // Not actually a property value.
	export_gpml_time_window(); // Not actually a property value.

	export_xs_boolean();
	export_xs_double();
	export_xs_integer();
	export_xs_string();
}

// This is here at the end of the layer because the problem appears to reside
// in a template being instantiated at the end of the compilation unit.
DISABLE_GCC_WARNING("-Wshadow")

#endif // GPLATES_NO_PYTHON
