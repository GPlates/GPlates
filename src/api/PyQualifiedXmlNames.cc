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

#include <sstream>
#include <boost/noncopyable.hpp>

#include "PythonConverterUtils.h"

#include "global/python.h"

#include "model/FeatureType.h"
#include "model/PropertyName.h"

#include "property-values/EnumerationType.h"
#include "property-values/StructuralType.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	// Only allow the single argument version of the creation function.
	// There's no need for the user to also specify the namespace alias.
	template <class QualifiedXmlNameType>
	const QualifiedXmlNameType
	qualified_xml_name_create_gpgim(
			const QString &name)
	{
		// Use the single argument overload.
		return QualifiedXmlNameType::create_gpgim(name);
	}

	// Only allow the single argument version of the creation function.
	// There's no need for the user to also specify the namespace alias.
	template <class QualifiedXmlNameType>
	const QualifiedXmlNameType
	qualified_xml_name_create_gpml(
			const QString &name)
	{
		// Use the single argument overload.
		return QualifiedXmlNameType::create_gpml(name);
	}

	// Only allow the single argument version of the creation function.
	// There's no need for the user to also specify the namespace alias.
	template <class QualifiedXmlNameType>
	const QualifiedXmlNameType
	qualified_xml_name_create_gml(
			const QString &name)
	{
		// Use the single argument overload.
		return QualifiedXmlNameType::create_gml(name);
	}

	// Only allow the single argument version of the creation function.
	// There's no need for the user to also specify the namespace alias.
	template <class QualifiedXmlNameType>
	const QualifiedXmlNameType
	qualified_xml_name_create_xsi(
			const QString &name)
	{
		// Use the single argument overload.
		return QualifiedXmlNameType::create_xsi(name);
	}
}


template <class PythonClassType/* the boost::python::class_ type*/>
void
export_qualified_xml_name(
		PythonClassType &qualified_xml_name_class,
		const char *class_name,
		const char *instance_name)
{
	// The GPlatesModel::QualifiedXmlName<> type.
	typedef typename PythonClassType::wrapped_type qualified_xml_name_type;

	//
	// The common GPlatesModel::QualifiedXmlName part of each qualified name to avoid repetition.
	//
	qualified_xml_name_class
		.def("get_namespace",
				&qualified_xml_name_type::get_namespace,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_namespace() -> string\n"
				"  Returns the namespace URI. For example, the 'gpml' namespace alias has the namespace "
				"'http://www.gplates.org/gplates'.\n")
		.def("get_namespace_alias",
				&qualified_xml_name_type::get_namespace_alias,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_namespace_alias() -> string\n"
				"  Returns the namespace alias. For example, 'gpml' (if created with 'create_gpml()').\n")
		.def("get_name",
				&qualified_xml_name_type::get_name,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_name() -> string\n"
				"  Returns the unqualified name. For example, the fully qualified name minus the "
				"'gpml:' prefix (if created with 'create_gpml()').\n")
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
		.def(bp::self < bp::self)
		.def(bp::self <= bp::self)
		.def(bp::self > bp::self)
		.def(bp::self >= bp::self)
		// For '__str__' convert to a qualified XML string...
		.def("__str__", &GPlatesModel::convert_qualified_xml_name_to_qstring<qualified_xml_name_type>)
	;

	// Enable boost::optional<GPlatesModel::QualifiedXmlName<> > to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<qualified_xml_name_type>();

	// Member to-QString conversion function.
	qualified_xml_name_class.def("to_qualified_string",
			&GPlatesModel::convert_qualified_xml_name_to_qstring<qualified_xml_name_type>,
			"to_qualified_string() -> string\n"
			"  Returns the fully qualified name. For example, 'gml:validTime' (if created with 'create_gml()').\n");

	std::stringstream from_qualified_string_docstring_stream;
	from_qualified_string_docstring_stream <<
			"create_from_qualified_string(name) -> " << class_name << " or None\n"
			"  Creates a " << class_name << " instance from a fully qualified name string.\n"
			"\n"
			"  :param name: qualified name\n"
			"  :type name: string\n"
			"  :rtype: " << class_name << " or None\n"
			"\n"
			"  The name string should have a ':' character separating the namespace alias from the unqualified name, "
			"for example 'gml:validTime'. If the namespace alias is not recognised (as 'gpml', 'gml' or 'xsi') "
			"then 'gpml' is assumed.\n"
			"\n"
			"  An over-qualified name string (eg, containing two or more ':' characters) will result "
			"in ``None`` being returned.\n"
			"  ::\n"
			"\n"
			"    " << instance_name << " = pygplates." << class_name << ".create_from_qualified_string(name)\n";

	// Static-member from-QString conversion function.
	qualified_xml_name_class.def("create_from_qualified_string",
			&GPlatesModel::convert_qstring_to_qualified_xml_name<qualified_xml_name_type>,
			from_qualified_string_docstring_stream.str().c_str());
	qualified_xml_name_class.staticmethod("create_from_qualified_string");
}


void
export_enumeration_type()
{
	//
	// EnumerationType
	//
	bp::class_<GPlatesPropertyValues::EnumerationType> enumeration_type_class(
			"EnumerationType",
			"The namespace-qualified type of an enumeration.\n"
			"\n"
			"All comparison operators (==, !=, <, <=, >, >=) are supported.\n",
			bp::no_init/*force usage of create functions*/);
	// Select the create functions appropriate for this QualifiedXmlName type...
	enumeration_type_class.def("create_gpml",
			&GPlatesApi::qualified_xml_name_create_gpml<GPlatesPropertyValues::EnumerationType>,
			"create_gpml(name) -> EnumerationType\n"
			"  Create an enumeration type qualified with the 'gpml:' prefix ('gpml:' + ``name``).\n"
			"\n"
			"  :param name: unqualified name\n"
			"  :type name: string\n"
			"\n"
			"  ::\n"
			"\n"
			"    enumeration_type = pygplates.EnumerationType.create_gpml(name)\n");
	enumeration_type_class.staticmethod("create_gpml");

	// Add the parts common to each GPlatesModel::QualifiedXmlName template instantiation (code re-use).
	export_qualified_xml_name(enumeration_type_class, "EnumerationType", "enumeration_type");
}


void
export_feature_type()
{
	//
	// FeatureType
	//
	bp::class_<GPlatesModel::FeatureType> feature_type_class(
			"FeatureType",
			"The namespace-qualified type of a feature.\n"
			"\n"
			"All comparison operators (==, !=, <, <=, >, >=) are supported.\n",
			bp::no_init/*force usage of create functions*/);
	// Select the create functions appropriate for this QualifiedXmlName type...
	feature_type_class.def("create_gpml",
			&GPlatesApi::qualified_xml_name_create_gpml<GPlatesModel::FeatureType>,
			"create_gpml(name) -> FeatureType\n"
			"  Create a feature type qualified with the 'gpml:' prefix ('gpml:' + ``name``).\n"
			"\n"
			"  :param name: unqualified name\n"
			"  :type name: string\n"
			"\n"
			"  ::\n"
			"\n"
			"    feature_type = pygplates.FeatureType.create_gpml(name)\n");
	feature_type_class.staticmethod("create_gpml");

	// Add the parts common to each GPlatesModel::QualifiedXmlName template instantiation (code re-use).
	export_qualified_xml_name(feature_type_class, "FeatureType", "feature_type");
}


void
export_property_name()
{
	//
	// PropertyName - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<GPlatesModel::PropertyName> property_name_class(
			"PropertyName",
			"The namespace-qualified name of a property.\n"
			"\n"
			"All comparison operators (==, !=, <, <=, >, >=) are supported.\n",
			bp::no_init/*force usage of create functions*/);
	// Select the create functions appropriate for this QualifiedXmlName type...
	property_name_class.def("create_gpml",
			&GPlatesApi::qualified_xml_name_create_gpml<GPlatesModel::PropertyName>,
				"create_gpml(name) -> PropertyName\n"
				"  Create a property name qualified with the 'gpml:' prefix ('gpml:' + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"\n"
				"  ::\n"
				"\n"
				"    property_name = pygplates.PropertyName.create_gpml(name)\n");
	property_name_class.staticmethod("create_gpml");
	property_name_class.def("create_gml",
			&GPlatesApi::qualified_xml_name_create_gml<GPlatesModel::PropertyName>,
				"create_gml(name) -> PropertyName\n"
				"  Create a property name qualified with the 'gml:' prefix ('gml:' + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"\n"
				"  ::\n"
				"\n"
				"    property_name = pygplates.PropertyName.create_gml(name)\n");
	property_name_class.staticmethod("create_gml");
	property_name_class.def("create_xsi",
			&GPlatesApi::qualified_xml_name_create_xsi<GPlatesModel::PropertyName>,
				"create_xsi(name) -> PropertyName\n"
				"  Create a property name qualified with the 'xsi:' prefix ('xsi:' + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"\n"
				"  ::\n"
				"\n"
				"    property_name = pygplates.PropertyName.create_xsi(name)\n");
	property_name_class.staticmethod("create_xsi");

	// Add the parts common to each GPlatesModel::QualifiedXmlName template instantiation (code re-use).
	export_qualified_xml_name(property_name_class, "PropertyName", "property_name");
}


void
export_structural_type()
{
	//
	// StructuralType - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<GPlatesPropertyValues::StructuralType> structural_type_class(
			"StructuralType",
			"The namespace-qualified structural type.\n"
			"\n"
			"All comparison operators (==, !=, <, <=, >, >=) are supported.\n",
			bp::no_init/*force usage of create functions*/);
	// Select the create functions appropriate for this QualifiedXmlName type...
	structural_type_class.def("create_gpml",
			&GPlatesApi::qualified_xml_name_create_gpml<GPlatesPropertyValues::StructuralType>,
				"create_gpml(name) -> StructuralType\n"
				"  Create a structural type qualified with the 'gpml:' prefix ('gpml:' + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"\n"
				"  ::\n"
				"\n"
				"    structural_type = pygplates.StructuralType.create_gpml(name)\n");
	structural_type_class.staticmethod("create_gpml");
	structural_type_class.def("create_gml",
			&GPlatesApi::qualified_xml_name_create_gml<GPlatesPropertyValues::StructuralType>,
				"create_gml(name) -> StructuralType\n"
				"  Create a structural type qualified with the 'gml:' prefix ('gml:' + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"\n"
				"  ::\n"
				"\n"
				"    structural_type = pygplates.StructuralType.create_gml(name)\n");
	structural_type_class.staticmethod("create_gml");
	structural_type_class.def("create_xsi",
			&GPlatesApi::qualified_xml_name_create_xsi<GPlatesPropertyValues::StructuralType>,
				"create_xsi(name) -> StructuralType\n"
				"  Create a structural type qualified with the 'xsi:' prefix ('xsi:' + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"\n"
				"  ::\n"
				"\n"
				"    structural_type = pygplates.StructuralType.create_xsi(name)\n");
	structural_type_class.staticmethod("create_xsi");

	// Add the parts common to each GPlatesModel::QualifiedXmlName template instantiation (code re-use).
	export_qualified_xml_name(structural_type_class, "StructuralType", "structural_type");
}


void
export_qualified_xml_names()
{
	export_enumeration_type();
	export_feature_type();
	export_property_name();
	export_structural_type();
}

#endif // GPLATES_NO_PYTHON
