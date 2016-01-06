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

#include "PyGPlatesModule.h"
#include "PythonConverterUtils.h"

#include "global/python.h"

#include "model/FeatureType.h"
#include "model/PropertyName.h"

#include "property-values/EnumerationType.h"
#include "property-values/StructuralType.h"
#include "property-values/ValueObjectType.h"


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

	template <class QualifiedXmlNameType>
	bp::object
	qualified_xml_name_hash(
			const QualifiedXmlNameType &qualified_xml_name)
	{
		// Use python builtin hash() function to hash the fully-qualified string.
		return get_builtin_hash()(
				GPlatesModel::convert_qualified_xml_name_to_qstring<QualifiedXmlNameType>(
						qualified_xml_name));
	}
}


template <class PythonClassType/* the boost::python::class_ type*/>
void
export_qualified_xml_name(
		PythonClassType &qualified_xml_name_class,
		const char *class_name,
		const char *instance_name,
		const char *example_qualified_name)
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
				"get_namespace()\n"
				"  Returns the namespace URI.\n"
				"\n"
				"  :rtype: string\n"
				"\n"
				"  For example, the 'gpml' namespace alias has the namespace "
				"'http://www.gplates.org/gplates'.\n")
		.def("get_namespace_alias",
				&qualified_xml_name_type::get_namespace_alias,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_namespace_alias()\n"
				"  Returns the namespace alias.\n"
				"\n"
				"  :rtype: string\n"
				"\n"
				"  For example, 'gpml' (if created with 'create_gpml()').\n")
		.def("get_name",
				&qualified_xml_name_type::get_name,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_name()\n"
				"  Returns the unqualified name.\n"
				"\n"
				"  :rtype: string\n"
				"\n"
				"  For example, the fully qualified name minus the "
				"'gpml:' prefix (if created with 'create_gpml()').\n")
		// Since we're defining '__eq__' we need to define a compatible '__hash__' or make it unhashable.
		// This is because the default '__hash__' is based on 'id()' which is not compatible and
		// would cause errors when used as key in a dictionary.
		// In python 3 fixes this by automatically making unhashable if define '__eq__' only.
		.def("__hash__", &GPlatesApi::qualified_xml_name_hash<qualified_xml_name_type>)
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

	std::stringstream to_qualified_string_docstring_stream;
	to_qualified_string_docstring_stream <<
			"to_qualified_string()\n"
			"  Returns the fully qualified name.\n"
			"\n"
			"  :rtype: string\n"
			"\n"
			"  For example, '" << example_qualified_name << "'.\n";

	// Member to-QString conversion function.
	qualified_xml_name_class.def("to_qualified_string",
			&GPlatesModel::convert_qualified_xml_name_to_qstring<qualified_xml_name_type>,
			to_qualified_string_docstring_stream.str().c_str());

	std::stringstream from_qualified_string_docstring_stream;
	from_qualified_string_docstring_stream <<
			"create_from_qualified_string(name)\n"
			// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
			// (like it can a pure python function) and we cannot document it in first (signature) line
			// because it messes up Sphinx's signature recognition...
			"  [*staticmethod*] Creates a " << class_name << " instance from a fully qualified name string.\n"
			"\n"
			"  :param name: qualified name\n"
			"  :type name: string\n"
			"  :rtype: " << class_name << " or None\n"
			"\n"
			"  The name string should have a ':' character separating the namespace alias from the unqualified name, "
			"for example '" << example_qualified_name
			<< "'. If the namespace alias is not recognised (as 'gpml', 'gml' or 'xsi') then 'gpml' is assumed.\n"
			"\n"
			"  An over-qualified name string (eg, containing two or more ':' characters) will result "
			"in ``None`` being returned.\n"
			"  ::\n"
			"\n"
			"    " << instance_name << " = pygplates." << class_name
			<< ".create_from_qualified_string('" << example_qualified_name << "')\n";

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
			"All comparison operators (==, !=, <, <=, >, >=) are supported. EnumerationType is "
			"hashable (can be used as a key in a ``dict``).\n",
			bp::no_init/*force usage of create functions*/);
	// Select the create functions appropriate for this QualifiedXmlName type...
	enumeration_type_class.def("create_gpml",
			&GPlatesApi::qualified_xml_name_create_gpml<GPlatesPropertyValues::EnumerationType>,
			"create_gpml(name)\n"
			// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
			// (like it can a pure python function) and we cannot document it in first (signature) line
			// because it messes up Sphinx's signature recognition...
			"  [*staticmethod*] Create an enumeration type qualified with the 'gpml:' prefix ('gpml:' + ``name``).\n"
			"\n"
			"  :param name: unqualified name\n"
			"  :type name: string\n"
			"  :rtype: :class:`EnumerationType`\n"
			"\n"
			"  ::\n"
			"\n"
			"    gpml_subduction_polarity_enumeration_type = pygplates.EnumerationType.create_gpml('SubductionPolarityEnumeration')\n");
	enumeration_type_class.staticmethod("create_gpml");

	// Add the parts common to each GPlatesModel::QualifiedXmlName template instantiation (code re-use).
	export_qualified_xml_name(
			enumeration_type_class,
			"EnumerationType",
			"enumeration_type",
			"gpml:SubductionPolarityEnumeration");
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
			"All comparison operators (==, !=, <, <=, >, >=) are supported. FeatureType is "
			"hashable (can be used as a key in a ``dict``).\n",
			bp::no_init/*force usage of create functions*/);
	// Select the create functions appropriate for this QualifiedXmlName type...
	feature_type_class.def("create_gpml",
			&GPlatesApi::qualified_xml_name_create_gpml<GPlatesModel::FeatureType>,
			"create_gpml(name)\n"
			// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
			// (like it can a pure python function) and we cannot document it in first (signature) line
			// because it messes up Sphinx's signature recognition...
			"  [*staticmethod*] Create a feature type qualified with the 'gpml:' prefix ('gpml:' + ``name``).\n"
			"\n"
			"  :param name: unqualified name\n"
			"  :type name: string\n"
			"  :rtype: :class:`FeatureType`\n"
			"\n"
			"  ::\n"
			"\n"
			"    gpml_coastline_feature_type = pygplates.FeatureType.create_gpml('Coastline')\n");
	feature_type_class.staticmethod("create_gpml");

	// Add the parts common to each GPlatesModel::QualifiedXmlName template instantiation (code re-use).
	export_qualified_xml_name(
			feature_type_class,
			"FeatureType",
			"feature_type",
			"gpml:Coastline");
}


namespace GPlatesApi
{
	//
	// Some common geometry property names....
	//
	const GPlatesModel::PropertyName gpml_average_sample_site_position =
			GPlatesModel::PropertyName::create_gpml("averageSampleSitePosition");
	const GPlatesModel::PropertyName gpml_boundary =
			GPlatesModel::PropertyName::create_gpml("boundary");
	const GPlatesModel::PropertyName gpml_center_line_of =
			GPlatesModel::PropertyName::create_gpml("centerLineOf");
	const GPlatesModel::PropertyName gpml_error_bounds =
			GPlatesModel::PropertyName::create_gpml("errorBounds");
	const GPlatesModel::PropertyName gpml_multi_position =
			GPlatesModel::PropertyName::create_gpml("multiPosition");
	const GPlatesModel::PropertyName gpml_outline_of =
			GPlatesModel::PropertyName::create_gpml("outlineOf");
	const GPlatesModel::PropertyName gpml_pole_position =
			GPlatesModel::PropertyName::create_gpml("polePosition");
	const GPlatesModel::PropertyName gpml_position =
			GPlatesModel::PropertyName::create_gpml("position");
	const GPlatesModel::PropertyName gpml_seed_points =
			GPlatesModel::PropertyName::create_gpml("seedPoints");
	const GPlatesModel::PropertyName gpml_unclassified_geometry =
			GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry");
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
			"All comparison operators (==, !=, <, <=, >, >=) are supported. PropertyName is "
			"hashable (can be used as a key in a ``dict``).\n"
			"\n"
			"As a convenience the following common *geometry* property names are available as class attributes:\n"
			"\n"
			"* ``pygplates.PropertyName.gpml_average_sample_site_position`` = "
			"pygplates.PropertyName.create_gpml('averageSampleSitePosition')\n"
			"* ``pygplates.PropertyName.gpml_boundary`` = ``pygplates.PropertyName.create_gpml('boundary')``\n"
			"* ``pygplates.PropertyName.gpml_center_line_of`` = ``pygplates.PropertyName.create_gpml('centerLineOf')``\n"
			"* ``pygplates.PropertyName.gpml_error_bounds`` = ``pygplates.PropertyName.create_gpml('errorBounds')``\n"
			"* ``pygplates.PropertyName.gpml_multi_position`` = ``pygplates.PropertyName.create_gpml('multiPosition')``\n"
			"* ``pygplates.PropertyName.gpml_outline_of`` = ``pygplates.PropertyName.create_gpml('outlineOf')``\n"
			"* ``pygplates.PropertyName.gpml_pole_position`` = ``pygplates.PropertyName.create_gpml('polePosition')``\n"
			"* ``pygplates.PropertyName.gpml_position`` = ``pygplates.PropertyName.create_gpml('position')``\n"
			"* ``pygplates.PropertyName.gpml_seed_points`` = ``pygplates.PropertyName.create_gpml('seedPoints')``\n"
			"* ``pygplates.PropertyName.gpml_unclassified_geometry`` = ``pygplates.PropertyName.create_gpml('unclassifiedGeometry')``\n",
			bp::no_init/*force usage of create functions*/);

	// Some common geometry property names...
	property_name_class.def_readonly("gpml_average_sample_site_position", GPlatesApi::gpml_average_sample_site_position);
	property_name_class.def_readonly("gpml_boundary", GPlatesApi::gpml_boundary);
	property_name_class.def_readonly("gpml_center_line_of", GPlatesApi::gpml_center_line_of);
	property_name_class.def_readonly("gpml_error_bounds", GPlatesApi::gpml_error_bounds);
	property_name_class.def_readonly("gpml_multi_position", GPlatesApi::gpml_multi_position);
	property_name_class.def_readonly("gpml_outline_of", GPlatesApi::gpml_outline_of);
	property_name_class.def_readonly("gpml_pole_position", GPlatesApi::gpml_pole_position);
	property_name_class.def_readonly("gpml_position", GPlatesApi::gpml_position);
	property_name_class.def_readonly("gpml_seed_points", GPlatesApi::gpml_seed_points);
	property_name_class.def_readonly("gpml_unclassified_geometry", GPlatesApi::gpml_unclassified_geometry);

	// Select the create functions appropriate for this QualifiedXmlName type...
	property_name_class.def("create_gpml",
			&GPlatesApi::qualified_xml_name_create_gpml<GPlatesModel::PropertyName>,
				"create_gpml(name)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a property name qualified with the 'gpml:' prefix ('gpml:' + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"  :rtype: :class:`PropertyName`\n"
				"\n"
				"  ::\n"
				"\n"
				"    gpml_reconstruction_plate_id_property_name = pygplates.PropertyName.create_gpml('reconstructionPlateId')\n");
	property_name_class.staticmethod("create_gpml");
	property_name_class.def("create_gml",
			&GPlatesApi::qualified_xml_name_create_gml<GPlatesModel::PropertyName>,
				"create_gml(name)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a property name qualified with the 'gml:' prefix ('gml:' + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"  :rtype: :class:`PropertyName`\n"
				"\n"
				"  ::\n"
				"\n"
				"    gml_valid_time_property_name = pygplates.PropertyName.create_gml('validTime')\n");
	property_name_class.staticmethod("create_gml");
	property_name_class.def("create_xsi",
			&GPlatesApi::qualified_xml_name_create_xsi<GPlatesModel::PropertyName>,
				"create_xsi(name)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a property name qualified with the 'xsi:' prefix ('xsi:' + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"  :rtype: :class:`PropertyName`\n"
				"\n"
				"  ::\n"
				"\n"
				"    property_name = pygplates.PropertyName.create_xsi(name)\n");
	property_name_class.staticmethod("create_xsi");

	// Add the parts common to each GPlatesModel::QualifiedXmlName template instantiation (code re-use).
	export_qualified_xml_name(
			property_name_class,
			"PropertyName",
			"property_name",
			"gpml:reconstructionPlateId");
}


void
export_scalar_type()
{
	//
	// ScalarType - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	// In the C++ code it's really a 'ValueObjectType' (the type that goes in the 'gml:valueComponent'
	// slots of a 'gml:DataBlock' property type) but it only really gets used to differentiate the
	// different scalar types (packed in the 'gml:DataBlock' and elsewhere in the code) so we'll call it
	// 'ScalarType' in the Python code (also 'ValueObjectType' is a bit meaningless in a general context).
	//
	bp::class_<GPlatesPropertyValues::ValueObjectType> scalar_type_class(
			"ScalarType",
			"The namespace-qualified type of scalar values.\n"
			"\n"
			"All comparison operators (==, !=, <, <=, >, >=) are supported. ScalarType is "
			"hashable (can be used as a key in a ``dict``).\n",
			bp::no_init/*force usage of create functions*/);
	// Select the create functions appropriate for this QualifiedXmlName type...
	scalar_type_class.def("create_gpml",
			&GPlatesApi::qualified_xml_name_create_gpml<GPlatesPropertyValues::ValueObjectType>,
				"create_gpml(name)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a scalar type qualified with the 'gpml:' prefix ('gpml:' + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"  :rtype: :class:`ScalarType`\n"
				"\n"
				"  ::\n"
				"\n"
				"    gpml_velocity_colat_scalar_type = pygplates.ScalarType.create_gpml('VelocityColat')\n");
	scalar_type_class.staticmethod("create_gpml");
	scalar_type_class.def("create_gml",
			&GPlatesApi::qualified_xml_name_create_gml<GPlatesPropertyValues::ValueObjectType>,
				"create_gml(name)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a scalar type qualified with the 'gml:' prefix ('gml:' + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"  :rtype: :class:`ScalarType`\n");
	scalar_type_class.staticmethod("create_gml");
	scalar_type_class.def("create_xsi",
			&GPlatesApi::qualified_xml_name_create_xsi<GPlatesPropertyValues::ValueObjectType>,
				"create_xsi(name)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a scalar type qualified with the 'xsi:' prefix ('xsi:' + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"  :rtype: :class:`ScalarType`\n");
	scalar_type_class.staticmethod("create_xsi");

	// Add the parts common to each GPlatesModel::QualifiedXmlName template instantiation (code re-use).
	export_qualified_xml_name(
			scalar_type_class,
			"ScalarType",
			"gpml_velocity_colat_scalar_type",
			"gpml:VelocityColat");
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
			"All comparison operators (==, !=, <, <=, >, >=) are supported. StructuralType is "
			"hashable (can be used as a key in a ``dict``).\n",
			bp::no_init/*force usage of create functions*/);
	// Select the create functions appropriate for this QualifiedXmlName type...
	structural_type_class.def("create_gpml",
			&GPlatesApi::qualified_xml_name_create_gpml<GPlatesPropertyValues::StructuralType>,
				"create_gpml(name)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a structural type qualified with the 'gpml:' prefix ('gpml:' + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"  :rtype: :class:`StructuralType`\n"
				"\n"
				"  ::\n"
				"\n"
				"    gpml_finite_rotation_structural_type = pygplates.StructuralType.create_gpml('FiniteRotation')\n");
	structural_type_class.staticmethod("create_gpml");
	structural_type_class.def("create_gml",
			&GPlatesApi::qualified_xml_name_create_gml<GPlatesPropertyValues::StructuralType>,
				"create_gml(name)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a structural type qualified with the 'gml:' prefix ('gml:' + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"  :rtype: :class:`StructuralType`\n"
				"\n"
				"  ::\n"
				"\n"
				"    gml_time_period_structural_type = pygplates.StructuralType.create_gml('TimePeriod')\n");
	structural_type_class.staticmethod("create_gml");
	structural_type_class.def("create_xsi",
			&GPlatesApi::qualified_xml_name_create_xsi<GPlatesPropertyValues::StructuralType>,
				"create_xsi(name)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Create a structural type qualified with the 'xsi:' prefix ('xsi:' + ``name``).\n"
				"\n"
				"  :param name: unqualified name\n"
				"  :type name: string\n"
				"  :rtype: :class:`StructuralType`\n"
				"\n"
				"  ::\n"
				"\n"
				"    xsi_double_structural_type = pygplates.StructuralType.create_xsi('double')\n");
	structural_type_class.staticmethod("create_xsi");

	// Add the parts common to each GPlatesModel::QualifiedXmlName template instantiation (code re-use).
	export_qualified_xml_name(
			structural_type_class,
			"StructuralType",
			"structural_type",
			"gml:TimePeriod");
}


void
export_qualified_xml_names()
{
	export_enumeration_type();
	export_feature_type();
	export_property_name();
	export_scalar_type();
#if 0 // There's no need to expose 'StructuralType' (yet)...
	export_structural_type();
#endif
}

#endif // GPLATES_NO_PYTHON
