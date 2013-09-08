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

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "PythonConverterUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/python.h"

#include "model/ModelUtils.h"
#include "model/TopLevelProperty.h"
#include "model/TopLevelPropertyInline.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	const GPlatesModel::TopLevelProperty::non_null_ptr_type
	top_level_property_inline_create(
			const GPlatesModel::PropertyName &property_name,
			GPlatesModel::PropertyValue::non_null_ptr_type property_value)
	{
		// Use the default value for the third argument.
		return GPlatesModel::TopLevelPropertyInline::create(property_name, property_value);
	}

	/**
	 * A wrapper than returns the actual derived property value type (converted to boost::python::object).
	 *
	 * The derived type is needed otherwise python is unable to access the derived attributes.
	 */
	bp::object/*derived property value non_null_intrusive_ptr*/
	top_level_property_get_property_value(
			GPlatesModel::TopLevelProperty::non_null_ptr_type top_level_property)
	{
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> property_value =
				// Use the default value for the second argument...
				GPlatesModel::ModelUtils::get_property_value(*top_level_property);

		// Should always have a valid *inline* top-level property (with a single property value).
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				property_value,
				GPLATES_ASSERTION_SOURCE);

		return PythonConverterUtils::get_property_value_as_derived_type(property_value.get());
	}
}


void
export_top_level_property()
{
	//
	// Property - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesModel::TopLevelProperty,
			GPlatesModel::TopLevelProperty::non_null_ptr_type,
			boost::noncopyable>(
					"Property",
					"Associates a property name with a property value.",
					bp::no_init)
		.def("create",
				&GPlatesApi::top_level_property_inline_create,
				"create(property_name, property_value) -> Property\n"
				"  Create a property given a property name and a property value.\n"
				"  ::\n"
				"\n"
				"    property = pygplates.Property.create(property_name, property_value)\n"
				"\n"
				"  :param property_name: property name\n"
				"  :type property_name: :class:`PropertyName`\n"
				"  :param property_value: property value\n"
				"  :type property_value: :class:`PropertyValue`\n")
 		.staticmethod("create")
  		.def("get_name",
				&GPlatesModel::TopLevelProperty::get_property_name,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_name() -> PropertyName\n"
				"  Returns the name of the property.\n"
				"\n"
				"  :rtype: :class:`PropertyName`\n")
		.def("get_value",
				&GPlatesApi::top_level_property_get_property_value,
				"get_value() -> PropertyValue\n"
				"  Returns the value of the property.\n"
				"\n"
				"  :rtype: :class:`PropertyValue`\n")
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
	;

	// Enable boost::optional<TopLevelProperty::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>();

	// Registers 'non-const' to 'const' conversions.
	boost::python::implicitly_convertible<
			GPlatesModel::TopLevelProperty::non_null_ptr_type,
			GPlatesModel::TopLevelProperty::non_null_ptr_to_const_type>();
	boost::python::implicitly_convertible<
			boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>,
			boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_to_const_type> >();
}

#endif // GPLATES_NO_PYTHON
