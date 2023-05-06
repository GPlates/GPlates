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
#include "PythonHashDefVisitor.h"
#include "PythonPickle.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/python.h"

#include "model/ModelUtils.h"
#include "model/TopLevelProperty.h"
#include "model/TopLevelPropertyInline.h"


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

	GPlatesModel::PropertyValue::non_null_ptr_type
	top_level_property_get_property_value(
			GPlatesModel::TopLevelProperty &top_level_property)
	{
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> property_value =
				// Use the default value for the second argument...
				GPlatesModel::ModelUtils::get_property_value(top_level_property);

		// Should always have a valid *inline* top-level property (with a single property value).
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				property_value,
				GPLATES_ASSERTION_SOURCE);

		return property_value.get();
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
					"Associates a property name with a property value.\n"
					"\n"
					"Properties are equality (``==``, ``!=``) comparable (but not hashable "
					"- cannot be used as a key in a ``dict``). This includes comparing the property value "
					"in the two properties being compared (see :class:`PropertyValue`) as well as the property name.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::top_level_property_inline_create,
						bp::default_call_policies(),
						(bp::arg("property_name"), bp::arg("property_value"))),
				"__init__(property_name, property_value)\n"
				"  Create a property given a property name and a property value.\n"
				"\n"
				"  :param property_name: property name\n"
				"  :type property_name: :class:`PropertyName`\n"
				"  :param property_value: property value\n"
				"  :type property_value: :class:`PropertyValue`\n"
				"\n"
				"  ::\n"
				"\n"
				"    property = pygplates.Property(property_name, property_value)\n")
		.def("clone",
				&GPlatesModel::TopLevelProperty::clone,
				"clone()\n"
				"  Create a duplicate of this property instance.\n"
				"\n"
				"  :rtype: :class:`Property`\n"
				"\n"
				"  This clones the :class:`PropertyName` and the :class:`PropertyValue`.\n")
  		.def("get_name",
				&GPlatesModel::TopLevelProperty::get_property_name,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_name()\n"
				"  Returns the name of the property.\n"
				"\n"
				"  :rtype: :class:`PropertyName`\n")
		// This is a private method (has leading '_'), and we don't provide a docstring.
		// This method is accessed by pure python API code.
		.def("_get_value",
				&GPlatesApi::top_level_property_get_property_value)
		// Generate '__str__' from 'operator<<'...
		// Note: Seems we need to qualify with 'self_ns::' to avoid MSVC compile error.
		.def(bp::self_ns::str(bp::self))
		// Due to the numerical tolerance in comparisons we cannot make hashable.
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesModel::TopLevelProperty::non_null_ptr_type>())
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesModel::TopLevelProperty>();
}
