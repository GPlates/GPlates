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
#include <boost/variant.hpp>

#include "PythonConverterUtils.h"

#include "app-logic/GeometryUtils.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"

#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/stl_iterator.hpp>

#include "model/FeatureVisitor.h"
#include "model/ModelUtils.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlHotSpotTrailMark.h"
#include "property-values/GpmlInterpolationFunction.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlKeyValueDictionaryElement.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/TextContent.h"
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

	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
	property_value_get_geometry(
			GPlatesModel::PropertyValue &property_value)
	{
		// Extract the geometry from the property value.
		//
		// Since GeometryOnSphere is polymorphic boost-python will downcast it to the
		// derived geometry type before converting to a python object.
		return GPlatesAppLogic::GeometryUtils::get_geometry_from_property_value(property_value);
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
	 * which returns a 'non_null_ptr_type' to the *derived* property value type.
	 * UPDATE: Actually boost-python will convert a 'PropertyValue::non_null_ptr_type' to a reference
	 * to a derived property value type (eg, 'PropertyValue::non_null_ptr_type' -> 'GpmlPlateId &')
	 * but it can't convert to a 'non_null_ptr_type' of derived property value type
	 * (eg, 'PropertyValue::non_null_ptr_type' -> 'GpmlPlateId::non_null_ptr_type') so
	 * 'PythonConverterUtils::get_property_value_as_derived_type()' is still needed for that.
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
					"* :class:`GmlLineString`\n"
					"* :class:`GmlMultiPoint`\n"
					"* :class:`GmlOrientableCurve`\n"
					"* :class:`GmlPoint`\n"
					"* :class:`GmlPolygon`\n"
					"* :class:`GmlTimeInstant`\n"
					"* :class:`GmlTimePeriod`\n"
					"* :class:`GpmlConstantValue`\n"
					"* :class:`GpmlFiniteRotation`\n"
					//"* :class:`GpmlFiniteRotationSlerp`\n"
					//"* :class:`GpmlHotSpotTrailMark`\n"
					"* :class:`GpmlIrregularSampling`\n"
					"* :class:`GpmlKeyValueDictionary`\n"
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
					"\n"
					"You can use :meth:`get_value` to extract a value at a specific time "
					"from a time-dependent wrapper.\n",
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
		.def("get_geometry",
				&GPlatesApi::property_value_get_geometry,
				"get_geometry() -> GeometryOnSphere or None\n"
				"  Extracts the :class:`geometry<GeometryOnSphere>` if this property value contains a geometry.\n"
				"\n"
				"  :rtype: :class:`GeometryOnSphere` or None\n"
				"\n"
				"  This function searches for a geometry in the following standard geometry property value types:\n"
				"\n"
				"  * :class:`GmlPoint`\n"
				"  * :class:`GmlMultiPoint`\n"
				"  * :class:`GmlPolygon`\n"
				"  * :class:`GmlLineString`\n"
				"  * :class:`GmlOrientableCurve` (a possible wrapper around :class:`GmlLineString`)\n"
				"  * :class:`GpmlConstantValue` (a possible wrapper around any of the above)\n"
				"\n"
				"  If this property value does not contain a geometry then ``None`` is returned.\n"
				"\n"
				"  Time-dependent geometry properties are *not* yet supported, so the only time-dependent "
				"property value wrapper currently supported by this function is :class:`GpmlConstantValue`.\n"
				"\n"
				"  To extract geometry from a specific feature property:\n"
				"  ::\n"
				"\n"
				"    property_value = feature.get_value(pygplates.PropertyName.create_gpml('polePosition'))\n"
				"    if property_value:\n"
				"        geometry = property_value.get_geometry()\n"
				"\n"
				"  ...however :meth:`Feature.get_geometry` provides an easier way to extract "
				"geometry from a feature with:\n"
				"  ::\n"
				"\n"
				"    geometry = feature.get_geometry(pygplates.PropertyName.create_gpml('polePosition'))\n"
				"\n"
				"  To extract all geometries from a feature (regardless of which properties they came from):\n"
				"  ::\n"
				"\n"
				"    all_geometries = []\n"
				"    for property in feature:\n"
				"        property_value = property.get_value()\n"
				"        if property_value:\n"
				"            geometry = property_value.get_geometry()\n"
				"            if geometry:\n"
				"                all_geometries.append(geometry)\n"
				"\n"
				"  ...however again :meth:`Feature.get_geometry` does this more easily with:\n"
				"  ::\n"
				"\n"
				"    all_geometries = feature.get_geometry(lambda property: True, pygplates.PropertyReturn.all)\n")
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
		// Since we're defining '__eq__' we need to define a compatible '__hash__' or make it unhashable.
		// This is because the default '__hash__'is based on 'id()' which is not compatible and
		// would cause errors when used as key in a dictionary.
		// In python 3 fixes this by automatically making unhashable if define '__eq__' only.
		.setattr("__hash__", bp::object()/*None*/) // make unhashable
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Enable boost::optional<PropertyValue::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesModel::PropertyValue::non_null_ptr_type>();

	// Registers 'non-const' to 'const' conversions.
	boost::python::implicitly_convertible<
			GPlatesModel::PropertyValue::non_null_ptr_type,
			GPlatesModel::PropertyValue::non_null_ptr_to_const_type>();
	boost::python::implicitly_convertible<
			boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>,
			boost::optional<GPlatesModel::PropertyValue::non_null_ptr_to_const_type> >();
}

//////////////////////////////////////////////////////////////////////////
// NOTE: Please keep the property values alphabetically ordered.
//////////////////////////////////////////////////////////////////////////


void
export_gml_line_string()
{
	//
	// GmlLineString - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GmlLineString,
			GPlatesPropertyValues::GmlLineString::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GmlLineString",
					"A property value representing a polyline geometry.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesPropertyValues::GmlLineString::create,
						bp::default_call_policies(),
						(bp::arg("polyline"))),
				"__init__(polyline)\n"
				"  Create a property value representing a polyline geometry.\n"
				"\n"
				"  :param polyline: the polyline geometry\n"
				"  :type polyline: :class:`PolylineOnSphere`\n"
				"\n"
				"  ::\n"
				"\n"
				"   line_string_property = pygplates.GmlLineString(polyline)\n")
		.def("get_polyline",
				&GPlatesPropertyValues::GmlLineString::get_polyline,
				"get_polyline() -> PolylineOnSphere\n"
				"  Returns the polyline geometry of this property value.\n"
				"\n"
				"  :rtype: :class:`PolylineOnSphere`\n")
		.def("set_polyline",
				&GPlatesPropertyValues::GmlLineString::set_polyline,
				(bp::arg("polyline")),
				"set_polyline(polyline)\n"
				"  Sets the polyline geometry of this property value.\n"
				"\n"
				"  :param polyline: the polyline geometry\n"
				"  :type polyline: :class:`PolylineOnSphere`\n")
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GmlLineString,
			GPlatesModel::PropertyValue>();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type
	gml_multi_point_create(
			GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
	{
		return GPlatesPropertyValues::GmlMultiPoint::create(multi_point_on_sphere);
	}
}

void
export_gml_multi_point()
{
	//
	// GmlMultiPoint - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GmlMultiPoint,
			GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GmlMultiPoint",
					"A property value representing a multi-point geometry.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gml_multi_point_create,
						bp::default_call_policies(),
						(bp::arg("multi_point"))),
				"__init__(multi_point)\n"
				"  Create a property value representing a multi-point geometry.\n"
				"\n"
				"  :param multi_point: the multi-point geometry\n"
				"  :type multi_point: :class:`MultiPointOnSphere`\n"
				"\n"
				"  ::\n"
				"\n"
				"    multi_point_property = pygplates.GmlMultiPoint(multi_point)\n")
		.def("get_multi_point",
				&GPlatesPropertyValues::GmlMultiPoint::get_multipoint,
				"get_multi_point() -> MultiPointOnSphere\n"
				"  Returns the multi-point geometry of this property value.\n"
				"\n"
				"  :rtype: :class:`MultiPointOnSphere`\n")
		.def("set_multi_point",
				&GPlatesPropertyValues::GmlMultiPoint::set_multipoint,
				(bp::arg("multi_point")),
				"set_multi_point(multi_point)\n"
				"  Sets the multi-point geometry of this property value.\n"
				"\n"
				"  :param multi_point: the multi-point geometry\n"
				"  :type multi_point: :class:`MultiPointOnSphere`\n")
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GmlMultiPoint,
			GPlatesModel::PropertyValue>();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type
	gml_orientable_curve_create(
			GPlatesPropertyValues::GmlLineString::non_null_ptr_type gml_line_string)
	{
		// Ignore the reverse flag for now - it never gets used by any client code.
		return GPlatesModel::ModelUtils::create_gml_orientable_curve(gml_line_string);
	}
}

void
export_gml_orientable_curve()
{
	// Use the 'non-const' overload so GmlOrientableCurve can be modified via python...
	const GPlatesPropertyValues::GmlLineString::non_null_ptr_type
			(GPlatesPropertyValues::GmlOrientableCurve::*base_curve)() =
					&GPlatesPropertyValues::GmlOrientableCurve::base_curve;

	//
	// GmlOrientableCurve - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GmlOrientableCurve,
			GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GmlOrientableCurve",
					"A property value representing a polyline geometry with a positive or negative orientation. "
					"However, currently the orientation is always positive so this is essentially no different "
					"than a :class:`GmlLineString`.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gml_orientable_curve_create,
						bp::default_call_policies(),
						(bp::arg("gml_line_string")/*, bp::arg(reverse_orientation)=false)*/)),
				//"__init__(gml_line_string, [reverse_orientation=False])\n"
				"__init__(gml_line_string)\n"
				"  Create an orientable polyline property value that wraps a polyline geometry and "
				"gives it an orientation - **NOTE** currently the orientation is always *positive* "
				"so this is essentially no different than a :class:`GmlLineString`.\n"
				"\n"
				"  :param gml_line_string: the line string (polyline) property value\n"
				"  :type gml_line_string: :class:`GmlLineString`\n"
				"\n"
				"  ::\n"
				"\n"
				"    orientable_curve_property = pygplates.GmlOrientableCurve(gml_line_string)\n")
		.def("get_base_curve",
				base_curve,
				"get_base_curve() -> GmlLineString\n"
				"  Returns the line string (polyline) property value of this wrapped property value.\n"
				"\n"
				"  :rtype: :class:`GmlLineString`\n")
		.def("set_base_curve",
				&GPlatesPropertyValues::GmlOrientableCurve::set_base_curve,
				(bp::arg("base_curve")),
				"set_base_curve(base_curve)\n"
				"  Sets the line string (polyline) property value of this wrapped property value.\n"
				"\n"
				"  :param base_curve: the line string (polyline) property value\n"
				"  :type base_curve: :class:`GmlLineString`\n")
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GmlOrientableCurve,
			GPlatesModel::PropertyValue>();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GmlPoint::non_null_ptr_type
	gml_point_create(
			GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
	{
		// Use the default value for the second argument.
		return GPlatesPropertyValues::GmlPoint::create(point_on_sphere);
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
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gml_point_create,
						bp::default_call_policies(),
						(bp::arg("point"))),
				"__init__(point)\n"
				"  Create a property value representing a point geometry.\n"
				"\n"
				"  :param point: the point geometry\n"
				"  :type point: :class:`PointOnSphere`\n"
				"\n"
				"  ::\n"
				"\n"
				"    point_property = pygplates.GmlPoint(point)\n")
		.def("get_point",
				&GPlatesPropertyValues::GmlPoint::get_point,
				"get_point() -> PointOnSphere\n"
				"  Returns the point geometry of this property value.\n"
				"\n"
				"  :rtype: :class:`PointOnSphere`\n")
		.def("set_point",
				&GPlatesPropertyValues::GmlPoint::set_point,
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
			GPlatesPropertyValues::GmlPoint,
			GPlatesModel::PropertyValue>();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GmlPolygon::non_null_ptr_type
	gml_polygon_create(
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
	{
		// We ignore interior polygons for now - later they will get stored a single PolygonOnSphere...
		return GPlatesPropertyValues::GmlPolygon::create(polygon_on_sphere);
	}
}

void
export_gml_polygon()
{
	//
	// GmlPolygon - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GmlPolygon,
			GPlatesPropertyValues::GmlPolygon::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GmlPolygon",
					"A property value representing a polygon geometry.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gml_polygon_create,
						bp::default_call_policies(),
						(bp::arg("polygon"))),
				"__init__(polygon)\n"
				"  Create a property value representing a polygon geometry.\n"
				"\n"
				"  :param polygon: the polygon geometry\n"
				"  :type polygon: :class:`PolygonOnSphere`\n"
				"\n"
				"  ::\n"
				"\n"
				"   polygon_property = pygplates.GmlPolygon(polygon)\n")
		.def("get_polygon",
				// We ignore interior polygons for now - later they will get stored in a single PolygonOnSphere...
				&GPlatesPropertyValues::GmlPolygon::get_exterior,
				"get_polygon() -> PolygonOnSphere\n"
				"  Returns the polygon geometry of this property value.\n"
				"\n"
				"  :rtype: :class:`PolygonOnSphere`\n")
		.def("set_polygon",
				// We ignore interior polygons for now - later they will get stored in a single PolygonOnSphere...
				&GPlatesPropertyValues::GmlPolygon::set_exterior,
				(bp::arg("polygon")),
				"set_polygon(polygon)\n"
				"  Sets the polygon geometry of this property value.\n"
				"\n"
				"  :param polygon: the polygon geometry\n"
				"  :type polygon: :class:`PolygonOnSphere`\n")
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GmlPolygon,
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
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesModel::ModelUtils::create_gml_time_instant,
						bp::default_call_policies(),
						(bp::arg("time_position"))),
				"__init__(time_position)\n"
				"  Create a property value representing a specific time instant.\n"
				"\n"
				"  :param time_position: the time position\n"
				"  :type time_position: float or :class:`GeoTimeInstant`\n"
				"\n"
				"  ::\n"
				"\n"
				"    begin_time_instant = pygplates.GmlTimeInstant(pygplates.GeoTimeInstant.create_distant_past())\n"
				"    end_time_instant = pygplates.GmlTimeInstant(0)\n")
		.def("get_time",
				&GPlatesPropertyValues::GmlTimeInstant::get_time_position,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_time() -> float\n"
				"  Returns the time position of this property value.\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  You can use :class:`GeoTimeInstant` with the returned ``float`` to "
				"check for *distant past* or *distant future* for example:\n"
				"\n"
				"  ::\n"
				"\n"
				"    float_time = time_instant.get_time()\n"
				"    print 'Time instant is distant past: %s' % pygplates.GeoTimeInstant(float_time).is_distant_past()\n"
				"\n"
				"  ...or just test directly using ``float`` comparisons...\n"
				"\n"
				"  ::\n"
				"\n"
				"    float_time = time_instant.get_time()\n"
				"    print 'Time instant is distant past: %s' % (float_time == float('inf'))\n")
		.def("set_time",
				&GPlatesPropertyValues::GmlTimeInstant::set_time_position,
				(bp::arg("time_position")),
				"set_time(time_position)\n"
				"  Sets the time position of this property value.\n"
				"\n"
				"  :param time_position: the time position\n"
				"  :type time_position: float or :class:`GeoTimeInstant`\n")
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
			const GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
	{
		return gml_time_period.begin()->get_time_position();
	}

	void
	gml_time_period_set_begin_time(
			GPlatesPropertyValues::GmlTimePeriod &gml_time_period,
			const GPlatesPropertyValues::GeoTimeInstant &begin_time_position)
	{
		// We can check begin/end time class invariant due to our restricted (python) interface whereas
		// the C++ GmlTimePeriod cannot because clients can modify indirectly via 'GmlTimeInstant'.
		GPlatesGlobal::Assert<GPlatesPropertyValues::GmlTimePeriod::BeginTimeLaterThanEndTimeException>(
				begin_time_position <= gml_time_period.end()->get_time_position(),
				GPLATES_ASSERTION_SOURCE);

		gml_time_period.begin()->set_time_position(begin_time_position);
	}

	GPlatesPropertyValues::GeoTimeInstant
	gml_time_period_get_end_time(
			const GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
	{
		return gml_time_period.end()->get_time_position();
	}

	void
	gml_time_period_set_end_time(
			GPlatesPropertyValues::GmlTimePeriod &gml_time_period,
			const GPlatesPropertyValues::GeoTimeInstant &end_time_position)
	{
		// We can check begin/end time class invariant due to our restricted (python) interface whereas
		// the C++ GmlTimePeriod cannot because clients can modify indirectly via 'GmlTimeInstant'.
		GPlatesGlobal::Assert<GPlatesPropertyValues::GmlTimePeriod::BeginTimeLaterThanEndTimeException>(
				gml_time_period.begin()->get_time_position() <= end_time_position,
				GPLATES_ASSERTION_SOURCE);

		gml_time_period.end()->set_time_position(end_time_position);
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
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesModel::ModelUtils::create_gml_time_period,
						bp::default_call_policies(),
						(bp::arg("begin_time_position"), bp::arg("end_time_position"))),
				"__init__(begin_time_position, end_time_position)\n"
				"  Create a property value representing a specific time period.\n"
				"\n"
				"  :param begin_time_position: the begin time position (time of appearance)\n"
				"  :type begin_time_position: float or :class:`GeoTimeInstant`\n"
				"  :param end_time_position: the end time position (time of disappearance)\n"
				"  :type end_time_position: float or :class:`GeoTimeInstant`\n"
				"  :raises: GmlTimePeriodBeginTimeLaterThanEndTimeError if begin time is later than end time\n"
				"\n"
				"  ::\n"
				"\n"
				"    time_period = pygplates.GmlTimePeriod(pygplates.GeoTimeInstant.create_distant_past(), 0)\n")
		.def("get_begin_time",
				&GPlatesApi::gml_time_period_get_begin_time,
				"get_begin_time() -> float\n"
				"  Returns the begin time position (time of appearance) of this property value.\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  You can use :class:`GeoTimeInstant` with the returned ``float`` to check for "
				"*distant past* or *distant future* for example. "
				"See :meth:`GmlTimeInstant.get_time` for more details.\n")
		.def("set_begin_time",
				&GPlatesApi::gml_time_period_set_begin_time,
				(bp::arg("time_position")),
				"set_begin_time(time_position)\n"
				"  Sets the begin time position (time of appearance) of this property value.\n"
				"\n"
				"  :param time_position: the begin time position (time of appearance)\n"
				"  :type time_position: float or :class:`GeoTimeInstant`\n"
				"  :raises: GmlTimePeriodBeginTimeLaterThanEndTimeError if begin time is later than end time\n")
		.def("get_end_time",
				&GPlatesApi::gml_time_period_get_end_time,
				"get_end_time() -> float\n"
				"  Returns the end time position (time of disappearance) of this property value.\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  You can use :class:`GeoTimeInstant` with the returned ``float`` to check for "
				"*distant past* or *distant future* for example. "
				"See :meth:`GmlTimeInstant.get_time` for more details.\n")
		.def("set_end_time",
				&GPlatesApi::gml_time_period_set_end_time,
				(bp::arg("time_position")),
				"set_end_time(time_position)\n"
				"  Sets the end time position (time of disappearance) of this property value.\n"
				"\n"
				"  :param time_position: the end time position (time of disappearance)\n"
				"  :type time_position: float or :class:`GeoTimeInstant`\n"
				"  :raises: GmlTimePeriodBeginTimeLaterThanEndTimeError if begin time is later than end time\n")
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

	// Return base property value to python as its derived property value type.
	bp::object/*derived property value non_null_intrusive_ptr*/
	gpml_constant_value_get_value(
			GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
	{
		// The derived property value type is needed otherwise python is unable to access the derived attributes.
		return PythonConverterUtils::get_property_value_as_derived_type(gpml_constant_value.value());
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
					"    reconstruction_plate_id = pygplates.Property(\n"
					"        pygplates.PropertyName.create_gpml('reconstructionPlateId'),\n"
					"        pygplates.GpmlConstantValue(pygplates.GpmlPlateId(701)))\n"
					"\n"
					"    relative_plate_id = pygplates.Property(\n"
					"        pygplates.PropertyName.create_gpml('relativePlate'),\n"
					"        pygplates.GpmlPlateId(701))\n"
					"\n"
					"If a property is created without a time-dependent wrapper where one is expected, "
					"or vice versa, then you can still save it to a GPML file and a subsequent read "
					"of that file will attempt to correct the property when it is created during "
					"the reading phase (by the GPML file format reader). This usually works for the "
					"simpler :class:`GpmlConstantValue` time-dependent wrapper but does not always "
					"work for the more advanced :class:`GpmlIrregularSampling` and "
					":class:`GpmlPiecewiseAggregation` time-dependent wrapper types.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gpml_constant_value_create,
						bp::default_call_policies(),
						(bp::arg("property_value"),
							bp::arg("description") = boost::optional<GPlatesUtils::UnicodeString>())),
					"__init__(property_value, [description])\n"
					"  Wrap a property value in a time-dependent wrapper that identifies the "
					"property value as constant for all time.\n"
					"\n"
					"  :param property_value: arbitrary property value\n"
					"  :type property_value: :class:`PropertyValue`\n"
					"  :param description: description of this constant value wrapper\n"
					"  :type description: string or None\n"
					"\n"
					"  Optionally provide a description string. If *description* is not specified "
					"then :meth:`get_description` will return ``None``.\n"
					"  ::\n"
					"\n"
					"    constant_property_value = pygplates.GpmlConstantValue(property_value)\n")
		// This is a private method (has leading '_'), and we don't provide a docstring...
		// This method is accessed by pure python API code.
		.def("_get_value",
				&GPlatesApi::gpml_constant_value_get_value)
		.def("set_value",
				&GPlatesPropertyValues::GpmlConstantValue::set_value,
				(bp::arg("property_value")),
				"set_value(property_value)\n"
				"  Sets the property value of this constant value wrapper.\n"
				"\n"
				"  :param property_value: arbitrary property value\n"
				"  :type property_value: :class:`PropertyValue`\n"
				"\n"
				"  This essentially replaces the previous property value. "
				"Note that an alternative is to directly modify the property value returned by :meth:`get_value` "
				"using its property value methods.\n")
		.def("get_description",
				&GPlatesPropertyValues::GpmlConstantValue::get_description,
				"get_description() -> string or None\n"
				"  Returns the *optional* description of this constant value wrapper, or ``None``.\n"
				"\n"
				"  :rtype: string or None\n")
		.def("set_description",
				&GPlatesPropertyValues::GpmlConstantValue::set_description,
				(bp::arg("description") = boost::optional<GPlatesUtils::UnicodeString>()),
				"set_description([description])\n"
				"  Sets the description of this constant value wrapper, or removes it if none specified.\n"
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


void
export_gpml_finite_rotation()
{
	// Select the desired overload of GpmlFiniteRotation::create...
	const GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type
			(*create)(const GPlatesMaths::FiniteRotation &) =
					&GPlatesPropertyValues::GpmlFiniteRotation::create;

	//
	// GpmlFiniteRotation - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlFiniteRotation,
			GPlatesPropertyValues::GpmlFiniteRotation::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlFiniteRotation",
					"A property value that represents a finite rotation.",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						create,
						bp::default_call_policies(),
						(bp::arg("finite_rotation"))),
				"__init__(finite_rotation)\n"
				"  Create a finite rotation property value from a finite rotation.\n"
				"\n"
				"  :param finite_rotation: the finite rotation\n"
				"  :type finite_rotation: :class:`FiniteRotation`\n"
				"\n"
				"  ::\n"
				"\n"
				"    finite_rotation_property = pygplates.GpmlFiniteRotation(finite_rotation)\n")
		.def("get_finite_rotation",
				&GPlatesPropertyValues::GpmlFiniteRotation::get_finite_rotation,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_finite_rotation() -> FiniteRotation\n"
				"  Returns the finite rotation.\n"
				"\n"
				"  :rtype: :class:`FiniteRotation`\n")
		.def("set_finite_rotation",
				&GPlatesPropertyValues::GpmlFiniteRotation::set_finite_rotation,
				(bp::arg("finite_rotation")),
				"set_finite_rotation(finite_rotation)\n"
				"  Sets the finite rotation.\n"
				"\n"
				"  :param finite_rotation: the finite rotation\n"
				"  :type finite_rotation: :class:`FiniteRotation`\n")
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GpmlFiniteRotation,
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
	// Not including GpmlFiniteRotationSlerp since it is not really used (yet) in GPlates and hence
	// is just extra baggage for the python API user (we can add it later though)...
#if 0
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
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(&GPlatesApi::gpml_finite_rotation_slerp_create),
				"__init__()\n"
				"  Create an instance of GpmlFiniteRotationSlerp.\n"
				"  ::\n"
				"\n"
				"    finite_rotation_slerp = pygplates.GpmlFiniteRotationSlerp()\n")
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class GpmlInterpolationFunction.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GpmlFiniteRotationSlerp,
			GPlatesPropertyValues::GpmlInterpolationFunction>();
#endif
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
	// Not including GpmlInterpolationFunction since it is not really used (yet) in GPlates and hence
	// is just extra baggage for the python API user (we can add it later though)...
#if 0
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
#endif
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type
	gpml_irregular_sampling_create(
			bp::object time_samples // Any python sequence (eg, list, tuple).
			// Not including interpolation function since it is not really used (yet) in GPlates and hence
			// is just extra baggage for the python API user (we can add it later though)...
#if 0
			, boost::optional<GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type>
					interpolation_function = boost::none
#endif
			)
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
				// Not including interpolation function since it is not really used (yet) in GPlates and hence
				// is just extra baggage for the python API user (we can add it later though)...
#if 0
				interpolation_function,
#else
				boost::none,
#endif
				time_samples_vector[0]->get_value_type());
	}

	const GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeSample>::non_null_ptr_type
	gpml_irregular_sampling_get_time_samples(
			GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
	{
		return &gpml_irregular_sampling.time_samples();
	}
}

void
export_gpml_irregular_sampling()
{
	// Not including interpolation function since it is not really used (yet) in GPlates and hence
	// is just extra baggage for the python API user (we can add it later though)...
#if 0
	// Use the 'non-const' overload...
	const boost::optional<GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type>
			(GPlatesPropertyValues::GpmlIrregularSampling::*get_interpolation_function)() =
					&GPlatesPropertyValues::GpmlIrregularSampling::interpolation_function;
#endif

	//
	// GpmlIrregularSampling - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlIrregularSampling,
			GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlIrregularSampling",
					"A time-dependent property consisting of a sequence of time samples irregularly spaced in time.\n"
					"\n"
					"The function :func:`interpolate_total_reconstruction_sequence` interpolates a "
					"*GpmlIrregularSampling* with time samples containing :class:`GpmlFiniteRotation` instances.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gpml_irregular_sampling_create,
						bp::default_call_policies(),
						(bp::arg("time_samples")
				// Not including interpolation function since it is not really used (yet) in GPlates and hence
				// is just extra baggage for the python API user (we can add it later though)...
#if 0
							, bp::arg("interpolation_function") =
								boost::optional<GPlatesPropertyValues::GpmlInterpolationFunction::non_null_ptr_type>()
#endif
							)),
				"__init__(time_samples"
				// Not including interpolation function since it is not really used (yet) in GPlates and hence
				// is just extra baggage for the python API user (we can add it later though)...
#if 0
				", [interpolation_function]"
#endif
				")\n"
				"  Create an irregularly sampled time-dependent property from a sequence of time samples."
				// Not including interpolation function since it is not really used (yet) in GPlates and hence
				// is just extra baggage for the python API user (we can add it later though)...
#if 0
				" Optionally provide an interpolation function."
#endif
				"\n"
				"\n"
				"  :param time_samples: A sequence of :class:`GpmlTimeSample` elements.\n"
				"  :type time_samples: Any sequence such as a ``list`` or a ``tuple``\n"
				// Not including interpolation function since it is not really used (yet) in GPlates and hence
				// is just extra baggage for the python API user (we can add it later though)...
#if 0
				"  :param interpolation_function: identifies function used to interpolate\n"
				"  :type interpolation_function: an instance derived from :class:`GpmlInterpolationFunction`\n"
#endif
				"  :raises: RuntimeError if time sample sequence is empty\n"
				"\n"
				"  **NOTE** that the sequence of time samples must **not** be empty (for technical implementation reasons), "
				"otherwise a *RuntimeError* exception will be thrown.\n"
				"  ::\n"
				"\n"
				"    irregular_sampling = pygplates.GpmlIrregularSampling(time_samples)\n")
		.def("get_time_samples",
				&GPlatesApi::gpml_irregular_sampling_get_time_samples,
				"get_time_samples() -> GpmlTimeSampleList\n"
				"  Returns the :class:`time samples<GpmlTimeSampleList>` in a sequence that behaves as a python ``list``.\n"
				"\n"
				"  :rtype: :class:`GpmlTimeSampleList`\n"
				"\n"
				"  Modifying the returned sequence will modify the internal state of the *GpmlIrregularSampling* "
				"instance.\n"
				"  ::\n"
				"\n"
				"    time_samples = irregular_sampling.get_time_samples()\n"
				"\n"
				"    # Sort samples by time\n"
				"    time_samples.sort(key = lambda x: x.get_time())\n")
		// Not including interpolation function since it is not really used (yet) in GPlates and hence
		// is just extra baggage for the python API user (we can add it later though)...
#if 0
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
				"set_interpolation_function([interpolation_function])\n"
				"  Sets the function used to interpolate between time samples, "
				"or removes it if none specified.\n"
				"\n"
				"  :param interpolation_function: the function used to interpolate between time samples\n"
				"  :type interpolation_function: an instance derived from :class:`GpmlInterpolationFunction`, or None\n")
#endif
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GpmlIrregularSampling,
			GPlatesModel::PropertyValue>();
}


namespace GPlatesApi
{
	const GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type
	gpml_key_value_dictionary_create(
			bp::object elements) // Any python sequence (eg, list, tuple).
	{
		// Begin/end iterators over the python dictionary elements sequence.
		bp::stl_input_iterator<GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type>
				elements_begin(elements),
				elements_end;

		// Copy into a vector.
		std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type> elements_vector;
		std::copy(elements_begin, elements_end, std::back_inserter(elements_vector));

		return GPlatesPropertyValues::GpmlKeyValueDictionary::create(elements_vector);
	}

	const GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::non_null_ptr_type
	gpml_key_value_dictionary_get_elements(
			GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary)
	{
		return &gpml_key_value_dictionary.elements();
	}
}

void
export_gpml_key_value_dictionary()
{
	//
	// GpmlKeyValueDictionary - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlKeyValueDictionary,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type,
			bp::bases<GPlatesModel::PropertyValue>,
			boost::noncopyable>(
					"GpmlKeyValueDictionary",
					"A dictionary of key/value pairs that associates property values with key strings.\n"
					"\n"
					"This is typically used to stored attributes imported from a Shapefile so that they "
					"are available for querying and so that they get written back out when saving to Shapefile. ",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gpml_key_value_dictionary_create,
						bp::default_call_policies(),
						(bp::arg("elements"))),
				"__init__(elements)\n"
				"  Create a dictionary of key/value elements.\n"
				"\n"
				"  :param elements: A sequence of :class:`GpmlKeyValueDictionaryElement` elements.\n"
				"  :type elements: Any sequence such as a ``list`` or a ``tuple``\n"
				"\n"
				"  ::\n"
				"\n"
				"    key_value_dictionary = pygplates.GpmlKeyValueDictionary(elements)\n")
		.def("get_elements",
				&GPlatesApi::gpml_key_value_dictionary_get_elements,
				"get_elements() -> GpmlKeyValueDictionaryElementList\n"
				"  Returns the :class:`dictionary elements<GpmlKeyValueDictionaryElementList>` in a "
				"sequence that behaves as a python ``list``.\n"
				"\n"
				"  :rtype: :class:`GpmlKeyValueDictionaryElementList`\n"
				"\n"
				"  Modifying the returned sequence will modify the internal state of the *GpmlKeyValueDictionary* "
				"instance.\n"
				"  ::\n"
				"\n"
				"    elements = key_value_dictionary.get_elements()\n"
				"\n"
				"    # Sort samples by key\n"
				"    elements.sort(key = lambda x: x.get_key())\n")
	;

	// Enable boost::optional<non_null_intrusive_ptr<> > to be passed to and from python.
	// Also registers various 'const' and 'non-const' conversions to base class PropertyValue.
	GPlatesApi::PythonConverterUtils::register_optional_non_null_intrusive_ptr_and_implicit_conversions<
			GPlatesPropertyValues::GpmlKeyValueDictionary,
			GPlatesModel::PropertyValue>();
}


namespace GPlatesApi
{
	/**
	 * The key-value dictionary element value types.
	 */
	typedef boost::variant<
			int,    // 'int' before 'double' since otherwise all 'int's will match, and become, 'double's.
			double,
			GPlatesPropertyValues::TextContent>
					dictionary_element_value_type;


	/**
	 * Visits a property value to retrieve the dictionary element value from it.
	 */
	class GetDictionaryElementValueFromPropertyValueVisitor : 
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		boost::optional<dictionary_element_value_type>
		get_dictionary_element_value_from_property_value(
				const GPlatesModel::PropertyValue &property_value)
		{
			d_dictionary_element_value = boost::none;

			property_value.accept_visitor(*this);

			return d_dictionary_element_value;
		}

	private:

		virtual
		void
		visit_xs_boolean(
				const GPlatesPropertyValues::XsBoolean &xs_boolean)
		{
			d_dictionary_element_value = dictionary_element_value_type(int(xs_boolean.get_value()));
		}

		virtual
		void
		visit_xs_double(
				const GPlatesPropertyValues::XsDouble &xs_double)
		{
			d_dictionary_element_value = dictionary_element_value_type(xs_double.get_value());
		}

		virtual
		void
		visit_xs_integer(
				const GPlatesPropertyValues::XsInteger& xs_integer)
		{
			d_dictionary_element_value = dictionary_element_value_type(xs_integer.get_value());
		}

		virtual
		void
		visit_xs_string(
				const GPlatesPropertyValues::XsString &xs_string)
		{
			d_dictionary_element_value = dictionary_element_value_type(xs_string.get_value());
		}


		boost::optional<dictionary_element_value_type> d_dictionary_element_value;
	};

	/**
	 * Visits a dictionary element value (variant) to wrap a property value around it.
	 */
	class GetPropertyValueFromDictionaryElementValueVisitor :
			public boost::static_visitor<GPlatesModel::PropertyValue::non_null_ptr_type>
	{
	public:

		GPlatesModel::PropertyValue::non_null_ptr_type
		operator()(
				int value) const
		{
			return GPlatesPropertyValues::XsInteger::create(value);
		}

		GPlatesModel::PropertyValue::non_null_ptr_type
		operator()(
				const double &value) const
		{
			return GPlatesPropertyValues::XsDouble::create(value);
		}

		GPlatesModel::PropertyValue::non_null_ptr_type
		operator()(
				const GPlatesPropertyValues::TextContent &value) const
		{
			return GPlatesPropertyValues::XsString::create(value.get());
		}

	};


	// Make it easier for client by converting from a regular string to XsString.
	const GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type
	gpml_key_value_dictionary_element_create(
			const GPlatesPropertyValues::TextContent &key,
			const dictionary_element_value_type &dictionary_element_value)
	{
		const GPlatesModel::PropertyValue::non_null_ptr_type property_value =
				boost::apply_visitor(
						GetPropertyValueFromDictionaryElementValueVisitor(),
						dictionary_element_value);

		return GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
				GPlatesPropertyValues::XsString::create(
						key.get()),
						property_value,
						property_value->get_structural_type());
	}

	// Make it easier for client by converting from XsString to a regular string.
	GPlatesPropertyValues::TextContent
	gpml_key_value_dictionary_element_get_key(
			const GPlatesPropertyValues::GpmlKeyValueDictionaryElement &gpml_key_value_dictionary_element)
	{
		return gpml_key_value_dictionary_element.key()->get_value();
	}

	// Make it easier for client by converting from a regular string to XsString.
	void
	gpml_key_value_dictionary_element_set_key(
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement &gpml_key_value_dictionary_element,
			const GPlatesPropertyValues::TextContent &key)
	{
		gpml_key_value_dictionary_element.set_key(
				GPlatesPropertyValues::XsString::create(key.get()));
	}

	/**
	 * Return the dictionary element value as an integer, float or string.
	 *
	 * boost::none could be returned but it normally shouldn't since shapefile attributes should
	 * always be int, float or string so we don't document that Py_None could be returned to the caller.
	 */
	boost::optional<dictionary_element_value_type>
	gpml_key_value_dictionary_element_get_value(
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement &gpml_key_value_dictionary_element)
	{
		GetDictionaryElementValueFromPropertyValueVisitor visitor;

		// Get the int, float or string value from the property value.
		return visitor.get_dictionary_element_value_from_property_value(
				*gpml_key_value_dictionary_element.value());
	}

	void
	gpml_key_value_dictionary_element_set_value(
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement &gpml_key_value_dictionary_element,
			const dictionary_element_value_type &dictionary_element_value)
	{
		const GPlatesModel::PropertyValue::non_null_ptr_type property_value =
				boost::apply_visitor(
						GetPropertyValueFromDictionaryElementValueVisitor(),
						dictionary_element_value);

#if 0
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				property_value->get_structural_type() == gpml_key_value_dictionary_element.get_value_type(),
				GPLATES_ASSERTION_SOURCE);
#endif

		gpml_key_value_dictionary_element.set_value(property_value);
	}
}

void
export_gpml_key_value_dictionary_element()
{
	//
	// GpmlKeyValueDictionaryElement - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement,
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type,
			boost::noncopyable>(
					"GpmlKeyValueDictionaryElement",
					"An entry in a dictionary of key/value pairs. The value is an ``int``, ``float`` "
					"or ``str`` that is associated with a specific key string.\n"
					"\n"
					"Dictionary elements are equality (``==``, ``!=``) comparable. "
					"This includes comparing the values of the two elements as well as their key strings.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gpml_key_value_dictionary_element_create,
						bp::default_call_policies(),
						(bp::arg("key"), bp::arg("value"))),
				"__init__(key, value)\n"
				"  Create a key/value dictionary element given a key string and an "
				"integer, float or string value.\n"
				"\n"
				"  :param key: key\n"
				"  :type key: string\n"
				"  :param value: the value of the dictionary element\n"
				"  :type value: integer, float or string\n"
				"\n"
				"  ::\n"
				"\n"
				"    key_value_dictionary_element = pygplates.GpmlKeyValueDictionaryElement('key', 10.1)\n")
		.def("get_key",
				&GPlatesApi::gpml_key_value_dictionary_element_get_key,
				"get_key() -> string\n"
				"  Returns the key of this dictionary element.\n"
				"\n"
				"  :rtype: string\n")
		.def("set_key",
				&GPlatesApi::gpml_key_value_dictionary_element_set_key,
				(bp::arg("key")),
				"set_key(key)\n"
				"  Sets the key associated with the property value of this dictionary element.\n"
				"\n"
				"  :param key: key\n"
				"  :type key: string\n")
		.def("get_value",
				&GPlatesApi::gpml_key_value_dictionary_element_get_value,
				"get_value() -> int or float or str\n"
				"  Returns the value of this dictionary element.\n"
				"\n"
				"  :rtype: integer, float or string\n")
		.def("set_value",
				&GPlatesApi::gpml_key_value_dictionary_element_set_value,
				(bp::arg("value")),
				"set_value(value)\n"
				"  Sets the value associated with the key of this dictionary element.\n"
				"\n"
				"  :param value: the value of the dictionary element\n"
				"  :type value: integer, float or string\n"
				"\n"
				"  This replaces the previous value.\n")
		// Since we're defining '__eq__' we need to define a compatible '__hash__' or make it unhashable.
		// This is because the default '__hash__'is based on 'id()' which is not compatible and
		// would cause errors when used as key in a dictionary.
		// In python 3 fixes this by automatically making unhashable if define '__eq__' only.
		.setattr("__hash__", bp::object()/*None*/) // make unhashable
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Enable boost::optional<GpmlKeyValueDictionaryElement::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type>();

	// Registers 'non-const' to 'const' conversions.
	boost::python::implicitly_convertible<
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type,
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_to_const_type>();
	boost::python::implicitly_convertible<
			boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type>,
			boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_to_const_type> >();

	// Register the dictionary element value types.
	GPlatesApi::PythonConverterUtils::register_variant_conversion<GPlatesApi::dictionary_element_value_type>();
	// Enable boost::optional<dictionary_element_value_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesApi::dictionary_element_value_type>();
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
			GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
	{
		return &gpml_piecewise_aggregation.time_windows();
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
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gpml_piecewise_aggregation_create,
						bp::default_call_policies(),
						(bp::arg("time_windows"))),
				"__init__(time_windows)\n"
				"  Create a piecewise-constant time-dependent property from a sequence of time windows.\n"
				"\n"
				"  :param time_windows: A sequence of :class:`GpmlTimeWindow` elements.\n"
				"  :type time_windows: Any sequence such as a ``list`` or a ``tuple``\n"
				"  :raises: RuntimeError if time window sequence is empty\n"
				"\n"
				"  **NOTE** that the sequence of time windows must **not** be empty (for technical implementation reasons), "
				"otherwise a *RuntimeError* exception will be thrown.\n"
				"  ::\n"
				"\n"
				"    piecewise_aggregation = pygplates.GpmlPiecewiseAggregation(time_windows)\n")
		.def("get_time_windows",
				&GPlatesApi::gpml_piecewise_aggregation_get_time_windows,
				"get_time_windows() -> GpmlTimeWindowList\n"
				"  Returns the :class:`time windows<GpmlTimeWindowList>` in a sequence that behaves as a python ``list``.\n"
				"\n"
				"  :rtype: :class:`GpmlTimeWindowList`\n"
				"\n"
				"  Modifying the returned sequence will modify the internal state of the *GpmlPiecewiseAggregation* "
				"instance.\n"
				"  ::\n"
				"\n"
				"    time_windows = piecewise_aggregation.get_time_windows()\n"
				"\n"
				"    # Sort windows by begin time\n"
				"    time_windows.sort(key = lambda x: x.get_begin_time())\n")
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
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesPropertyValues::GpmlPlateId::create,
						bp::default_call_policies(),
						(bp::arg("plate_id"))),
				"__init__(plate_id)\n"
				"  Create a plate id property value from an integer plate id.\n"
				"\n"
				"  :param plate_id: integer plate id\n"
				"  :type plate_id: int\n"
				"\n"
				"  ::\n"
				"\n"
				"    plate_id_property = pygplates.GpmlPlateId(plate_id)\n")
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
			boost::optional<GPlatesPropertyValues::TextContent> description,
			bool is_enabled)
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
				!is_enabled/*is_disabled*/);
	}

	// Return base property value to python as its derived property value type.
	bp::object/*derived property value non_null_intrusive_ptr*/
	gpml_time_sample_get_value(
			GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample)
	{
		// The derived property value type is needed otherwise python is unable to access the derived attributes.
		return PythonConverterUtils::get_property_value_as_derived_type(gpml_time_sample.value());
	}

	GPlatesPropertyValues::GeoTimeInstant
	gpml_time_sample_get_time(
			const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample)
	{
		return gpml_time_sample.valid_time()->get_time_position();
	}

	void
	gpml_time_sample_set_time(
			GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample,
			const GPlatesPropertyValues::GeoTimeInstant &time_position)
	{
		gpml_time_sample.valid_time()->set_time_position(time_position);
	}

	// Make it easier for client by converting from XsString to a regular string.
	boost::optional<GPlatesPropertyValues::TextContent>
	gpml_time_sample_get_description(
			const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample)
	{
		boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type> xs_string =
				gpml_time_sample.description();
		if (!xs_string)
		{
			return boost::none;
		}

		return xs_string.get()->get_value();
	}

	// Make it easier for client by converting from a regular string to XsString.
	void
	gpml_time_sample_set_description(
			GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample,
			boost::optional<GPlatesPropertyValues::TextContent> description)
	{
		boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_type> xs_string;
		if (description)
		{
			xs_string = GPlatesPropertyValues::XsString::create(description->get());
		}

		gpml_time_sample.set_description(xs_string);
	}

	bool
	gpml_time_sample_is_enabled(
			const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample)
	{
		return !gpml_time_sample.is_disabled();
	}

	void
	gpml_time_sample_set_enabled(
			GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample,
			bool is_enabled)
	{
		return gpml_time_sample.set_disabled(!is_enabled);
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
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gpml_time_sample_create,
						bp::default_call_policies(),
						(bp::arg("property_value"),
							bp::arg("time"),
							bp::arg("description") = boost::optional<GPlatesPropertyValues::TextContent>(),
							bp::arg("is_enabled") = true)),
				"__init__(property_value, time, [description], [is_enabled=True])\n"
				"  Create a time sample given a property value and time and optionally a description string "
				"and disabled flag.\n"
				"\n"
				"  :param property_value: arbitrary property value\n"
				"  :type property_value: :class:`PropertyValue`\n"
				"  :param time: the time position associated with the property value\n"
				"  :type time: float or :class:`GeoTimeInstant`\n"
				"  :param description: description of the time sample\n"
				"  :type description: string or None\n"
				"  :param is_enabled: whether time sample is enabled\n"
				"  :type is_enabled: bool\n"
				"\n"
				"  ::\n"
				"\n"
				"    time_sample = pygplates.GpmlTimeSample(property_value, time)\n")
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
				"  Sets the property value associated with this time sample.\n"
				"\n"
				"  :param property_value: arbitrary property value\n"
				"  :type property_value: :class:`PropertyValue`\n"
				"\n"
				"  This essentially replaces the previous property value. "
				"Note that an alternative is to directly modify the property value returned by :meth:`get_value` "
				"using its property value methods.\n")
		.def("get_time",
				&GPlatesApi::gpml_time_sample_get_time,
				"get_time() -> float\n"
				"  Returns the time position of this time sample.\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  You can use :class:`GeoTimeInstant` with the returned ``float`` to check for "
				"*distant past* or *distant future* for example. "
				"See :meth:`GmlTimeInstant.get_time` for more details.\n")
		.def("set_time",
				&GPlatesApi::gpml_time_sample_set_time,
				(bp::arg("time")),
				"set_time(time)\n"
				"  Sets the time position associated with this time sample.\n"
				"\n"
				"  :param time: the time position associated with the property value\n"
				"  :type time: float or :class:`GeoTimeInstant`\n")
		.def("get_description",
				&GPlatesApi::gpml_time_sample_get_description,
				"get_description() -> string or None\n"
				"  Returns the description of this time sample, or ``None``.\n"
				"\n"
				"  :rtype: string or None\n")
		.def("set_description",
				&GPlatesApi::gpml_time_sample_set_description,
				(bp::arg("description") = boost::optional<GPlatesPropertyValues::TextContent>()),
				"set_description([description])\n"
				"  Sets the description associated with this time sample, or removes it if none specified.\n"
				"\n"
				"  :param description: description of the time sample\n"
				"  :type description: string or None\n")
		.def("is_enabled",
				&GPlatesApi::gpml_time_sample_is_enabled,
				"is_enabled() -> bool\n"
				"  Returns whether this time sample is enabled.\n"
				"\n"
				"  :rtype: bool\n"
				"\n"
				"  For example, only enabled total reconstruction poles (in a GpmlIrregularSampling sequence) "
				"are considered when interpolating rotations at some arbitrary time.\n")
		.def("set_enabled",
				&GPlatesApi::gpml_time_sample_set_enabled,
				(bp::arg("set_enabled")=true),
				"set_enabled([is_enabled=True])\n"
				"  Sets whether this time sample is enabled.\n"
				"\n"
				"  :param is_enabled: whether time sample is enabled (defaults to ``True``)\n"
				"  :type is_enabled: bool\n")
		.def("is_disabled",
				&GPlatesPropertyValues::GpmlTimeSample::is_disabled,
				"is_disabled() -> bool\n"
				"  Returns whether this time sample is disabled or not.\n"
				"\n"
				"  :rtype: bool\n"
				"\n"
				"  For example, a disabled total reconstruction pole (in a GpmlIrregularSampling sequence) "
				"is ignored when interpolating rotations at some arbitrary time.\n")
		.def("set_disabled",
				&GPlatesPropertyValues::GpmlTimeSample::set_disabled,
				(bp::arg("is_disabled")=true),
				"set_disabled([is_disabled=True])\n"
				"  Sets whether this time sample is disabled.\n"
				"\n"
				"  :param is_disabled: whether time sample is disabled (defaults to ``True``)\n"
				"  :type is_disabled: bool\n")
		// Since we're defining '__eq__' we need to define a compatible '__hash__' or make it unhashable.
		// This is because the default '__hash__'is based on 'id()' which is not compatible and
		// would cause errors when used as key in a dictionary.
		// In python 3 fixes this by automatically making unhashable if define '__eq__' only.
		.setattr("__hash__", bp::object()/*None*/) // make unhashable
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Enable boost::optional<GpmlTimeSample::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesPropertyValues::GpmlTimeSample::non_null_ptr_type>();

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
			GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
	{
		// The derived property value type is needed otherwise python is unable to access the derived attributes.
		return PythonConverterUtils::get_property_value_as_derived_type(gpml_time_window.time_dependent_value());
	}

	GPlatesPropertyValues::GeoTimeInstant
	gpml_time_window_get_begin_time(
			const GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
	{
		return gpml_time_window.valid_time()->begin()->get_time_position();
	}

	void
	gpml_time_window_set_begin_time(
			GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window,
			const GPlatesPropertyValues::GeoTimeInstant &begin_time)
	{
		// Use 'assert' protected function to ensure proper exception is raised if GmlTimePeriod
		// class invariant is violated.
		gml_time_period_set_begin_time(*gpml_time_window.valid_time(), begin_time);
	}

	GPlatesPropertyValues::GeoTimeInstant
	gpml_time_window_get_end_time(
			const GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
	{
		return gpml_time_window.valid_time()->end()->get_time_position();
	}

	void
	gpml_time_window_set_end_time(
			GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window,
			const GPlatesPropertyValues::GeoTimeInstant &end_time)
	{
		// Use 'assert' protected function to ensure proper exception is raised if GmlTimePeriod
		// class invariant is violated.
		gml_time_period_set_end_time(*gpml_time_window.valid_time(), end_time);
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
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::gpml_time_window_create,
						bp::default_call_policies(),
						(bp::arg("property_value"), bp::arg("begin_time"), bp::arg("end_time"))),
				"__init__(property_value, begin_time, end_time)\n"
				"  Create a time window given a property value and time range.\n"
				"\n"
				"  :param property_value: arbitrary property value\n"
				"  :type property_value: :class:`PropertyValue`\n"
				"  :param begin_time: the begin time of the time window\n"
				"  :type begin_time: float or :class:`GeoTimeInstant`\n"
				"  :param end_time: the end time of the time window\n"
				"  :type end_time: float or :class:`GeoTimeInstant`\n"
				"  :raises: GmlTimePeriodBeginTimeLaterThanEndTimeError if begin time is later than end time\n"
				"\n"
				"  ::\n"
				"\n"
				"    time_window = pygplates.GpmlTimeWindow(property_value, begin_time, end_time)\n"
				"\n"
				"  Note that *begin_time* must be further in the past than the *end_time* "
				"``begin_time > end_time``.\n")
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
				"  Sets the property value associated with this time window.\n"
				"\n"
				"  :param property_value: arbitrary property value\n"
				"  :type property_value: :class:`PropertyValue`\n"
				"\n"
				"  This essentially replaces the previous property value. "
				"Note that an alternative is to directly modify the property value returned by :meth:`get_value` "
				"using its property value methods.\n")
		.def("get_begin_time",
				&GPlatesApi::gpml_time_window_get_begin_time,
				"get_begin_time() -> float\n"
				"  Returns the begin time of this time window.\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  You can use :class:`GeoTimeInstant` with the returned ``float`` to check for "
				"*distant past* or *distant future* for example. "
				"See :meth:`GmlTimeInstant.get_time` for more details.\n")
		.def("set_begin_time",
				&GPlatesApi::gpml_time_window_set_begin_time,
				(bp::arg("time")),
				"set_begin_time(time)\n"
				"  Sets the begin time of this time window.\n"
				"\n"
				"  :param time: the begin time of this time window\n"
				"  :type time: float or :class:`GeoTimeInstant`\n"
				"  :raises: GmlTimePeriodBeginTimeLaterThanEndTimeError if begin time is later than end time\n")
		.def("get_end_time",
				&GPlatesApi::gpml_time_window_get_end_time,
				"get_end_time() -> float\n"
				"  Returns the end time of this time window.\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  You can use :class:`GeoTimeInstant` with the returned ``float`` to check for "
				"*distant past* or *distant future* for example. "
				"See :meth:`GmlTimeInstant.get_time` for more details.\n")
		.def("set_end_time",
				&GPlatesApi::gpml_time_window_set_end_time,
				(bp::arg("time")),
				"set_end_time(time)\n"
				"  Sets the end time of this time window.\n"
				"\n"
				"  :param time: the end time of this time window\n"
				"  :type time: float or :class:`GeoTimeInstant`\n"
				"  :raises: GmlTimePeriodBeginTimeLaterThanEndTimeError if begin time is later than end time\n")
		// Since we're defining '__eq__' we need to define a compatible '__hash__' or make it unhashable.
		// This is because the default '__hash__'is based on 'id()' which is not compatible and
		// would cause errors when used as key in a dictionary.
		// In python 3 fixes this by automatically making unhashable if define '__eq__' only.
		.setattr("__hash__", bp::object()/*None*/) // make unhashable
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Enable boost::optional<GpmlTimeWindow::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_type>();

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
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesPropertyValues::XsBoolean::create,
						bp::default_call_policies(),
						(bp::arg("boolean_value"))),
				"__init__(boolean_value)\n"
				"  Create a boolean property value from a boolean value.\n"
				"\n"
				"  :param boolean_value: the boolean value\n"
				"  :type boolean_value: bool\n"
				"\n"
				"  ::\n"
				"\n"
				"    boolean_property = pygplates.XsBoolean(boolean_value)\n")
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
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesPropertyValues::XsDouble::create,
						bp::default_call_policies(),
						(bp::arg("float_value"))),
				"__init__(float_value)\n"
				"  Create a floating-point property value from a ``float``.\n"
				"\n"
				"  :param float_value: the floating-point value\n"
				"  :type float_value: float\n"
				"\n"
				"  ::\n"
				"\n"
				"    float_property = pygplates.XsDouble(float_value)\n")
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
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesPropertyValues::XsInteger::create,
						bp::default_call_policies(),
						(bp::arg("integer_value"))),
				"__init__(integer_value)\n"
				"  Create an integer property value from an ``int``.\n"
				"\n"
				"  :param integer_value: the integer value\n"
				"  :type integer_value: int\n"
				"\n"
				"  ::\n"
				"\n"
				"    integer_property = pygplates.XsInteger(integer_value)\n")
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
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesPropertyValues::XsString::create,
						bp::default_call_policies(),
						(bp::arg("string"))),
				"__init__(string)\n"
				"  Create a string property value from a string.\n"
				"\n"
				"  :param string: the string\n"
				"  :type string: string\n"
				"\n"
				"  ::\n"
				"\n"
				"    string_property = pygplates.XsString(string)\n")
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

	export_gml_line_string();
	export_gml_multi_point();
	export_gml_orientable_curve();
	export_gml_point();
	export_gml_polygon();
	export_gml_time_instant();
	export_gml_time_period();

	export_gpml_constant_value();

	// GpmlInterpolationFunction and its derived classes.
	// Since GpmlInterpolationFunction is the base class it must be registered first to avoid runtime error.
	export_gpml_interpolation_function();
	export_gpml_finite_rotation_slerp();

	export_gpml_finite_rotation();
	export_gpml_hot_spot_trail_mark();
	export_gpml_irregular_sampling();
	export_gpml_key_value_dictionary();
	export_gpml_key_value_dictionary_element(); // Not actually a property value.
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
