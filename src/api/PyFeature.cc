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
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <QString>

#include "PyInformationModel.h"
#include "PythonConverterUtils.h"

#include "global/GPlatesAssert.h"
#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/stl_iterator.hpp>

#include "model/FeatureHandle.h"
#include "model/FeatureId.h"
#include "model/FeatureType.h"
#include "model/Gpgim.h"
#include "model/ModelUtils.h"
#include "model/RevisionId.h"
#include "model/TopLevelProperty.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	const GPlatesModel::FeatureHandle::non_null_ptr_type
	feature_handle_create(
			boost::optional<GPlatesModel::FeatureType> feature_type,
			boost::optional<GPlatesModel::FeatureId> feature_id,
			VerifyInformationModel::Value verify_information_model)
	{
		// Default to unclassified feature - since that supports any combination of properties.
		if (!feature_type)
		{
			feature_type = GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature");
		}
		else if (verify_information_model == VerifyInformationModel::YES)
		{
			// This exception will get converted to python 'InformationModelError'.
			GPlatesGlobal::Assert<InformationModelException>(
					GPlatesModel::Gpgim::instance().get_feature_class(feature_type.get()),
					GPLATES_EXCEPTION_SOURCE,
					QString("The feature type was not recognised as a valid type by the GPGIM"));
		}

		// Create a unique feature id if none specified.
		if (!feature_id)
		{
			feature_id = GPlatesModel::FeatureId();
		}

		return GPlatesModel::FeatureHandle::create(feature_type.get(), feature_id.get());
	}

	GPlatesModel::TopLevelProperty::non_null_ptr_type
	feature_handle_add_property(
			GPlatesModel::FeatureHandle &feature_handle,
			const GPlatesModel::PropertyName &property_name,
			GPlatesModel::PropertyValue::non_null_ptr_type property_value,
			VerifyInformationModel::Value verify_information_model)
	{
		if (verify_information_model == VerifyInformationModel::NO)
		{
			// Just create a top-level property without checking information model.
			GPlatesModel::TopLevelProperty::non_null_ptr_type property =
					GPlatesModel::TopLevelPropertyInline::create(property_name, property_value);
			// Return the newly added property.
			return *feature_handle.add(property);
		}

		// Only add property if valid property name for the feature's type.
		GPlatesModel::ModelUtils::TopLevelPropertyError::Type add_property_error_code;
		boost::optional<GPlatesModel::FeatureHandle::iterator> feature_property_iter =
				GPlatesModel::ModelUtils::add_property(
						feature_handle.reference(),
						property_name,
						property_value,
						true/*check_property_name_allowed_for_feature_type*/,
						&add_property_error_code);
		if (!feature_property_iter)
		{
			throw InformationModelException(
					GPLATES_EXCEPTION_SOURCE,
					QString(GPlatesModel::ModelUtils::get_error_message(add_property_error_code)));
		}

		// Return the newly added property.
		return *feature_property_iter.get();
	}

	bp::list
	feature_handle_add_properties(
			GPlatesModel::FeatureHandle &feature_handle,
			bp::object properties_object,
			VerifyInformationModel::Value verify_information_model)
	{
		bp::list properties;

		// Begin/end iterators over the python property name/value pair sequence.
		bp::stl_input_iterator<bp::object>
				properties_iter(properties_object),
				properties_end;

		for ( ; properties_iter != properties_end; ++properties_iter)
		{
			std::vector<bp::object> name_value_vector;
			// Attempt to extract the property name and value.
			try
			{
				// A two-element sequence containing property name and property value.
				bp::stl_input_iterator<bp::object>
						name_value_seq_begin(*properties_iter),
						name_value_seq_end;

				// Copy into a vector.
				std::copy(name_value_seq_begin, name_value_seq_end, std::back_inserter(name_value_vector));
			}
			catch (const boost::python::error_already_set &)
			{
				PyErr_Clear();

				PyErr_SetString(PyExc_TypeError, "Expected a sequence of (PropertyName, PropertyValue)");
				bp::throw_error_already_set();
			}

			if (name_value_vector.size() != 2)   // (PropertyName, PropertyValue)
			{
				PyErr_SetString(PyExc_TypeError, "Expected a sequence of (PropertyName, PropertyValue)");
				bp::throw_error_already_set();
			}

			bp::extract<GPlatesModel::PropertyName> extract_property_name(name_value_vector[0]);
			bp::extract<GPlatesModel::PropertyValue::non_null_ptr_type> extract_property_value(name_value_vector[1]);
			if (!extract_property_name.check() ||
				!extract_property_value.check())
			{
				PyErr_SetString(PyExc_TypeError, "Expected a sequence of (PropertyName, PropertyValue)");
				bp::throw_error_already_set();
			}

			GPlatesModel::TopLevelProperty::non_null_ptr_type property =
					feature_handle_add_property(
							feature_handle,
							extract_property_name(),
							extract_property_value(),
							verify_information_model);
			properties.append(property);
		}

		return properties;
	}

	void
	feature_handle_remove(
			GPlatesModel::FeatureHandle &feature_handle,
			bp::object properties_object)
	{
		// See if a single property name.
		bp::extract<GPlatesModel::PropertyName> extract_property_name(properties_object);
		if (extract_property_name.check())
		{
			const GPlatesModel::PropertyName property_name = extract_property_name();

			// Search for the property name.
			GPlatesModel::FeatureHandle::iterator properties_iter = feature_handle.begin();
			GPlatesModel::FeatureHandle::iterator properties_end = feature_handle.end();
			for ( ; properties_iter != properties_end; ++properties_iter)
			{
				GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

				if (property_name == feature_property->get_property_name())
				{
					// Note that removing a property does not prevent us from incrementing to the next property.
					feature_handle.remove(properties_iter);
				}
			}

			return;
		}

		// See if a single property.
		bp::extract<GPlatesModel::TopLevelProperty::non_null_ptr_type> extract_property(properties_object);
		if (extract_property.check())
		{
			GPlatesModel::TopLevelProperty::non_null_ptr_type property = extract_property();

			// Search for the property.
			// Note: This searches for the same property *instance* - it does not compare values of
			// two different property instances.
			GPlatesModel::FeatureHandle::iterator properties_iter = feature_handle.begin();
			GPlatesModel::FeatureHandle::iterator properties_end = feature_handle.end();
			for ( ; properties_iter != properties_end; ++properties_iter)
			{
				GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

				// Compare pointers not pointed-to-objects.
				if (property == feature_property)
				{
					feature_handle.remove(properties_iter);
					return;
				}
			}

			// Raise the 'ValueError' python exception if the property was not found.
			PyErr_SetString(PyExc_ValueError, "Property instance not found");
			bp::throw_error_already_set();
		}

		// Try a sequence of property names next.
		typedef std::vector<GPlatesModel::PropertyName> property_names_seq_type;
		property_names_seq_type property_names_seq;
		try
		{
			// Begin/end iterators over the python property names sequence.
			bp::stl_input_iterator<GPlatesModel::PropertyName>
					property_names_begin(properties_object),
					property_names_end;

			// Copy into the vector.
			std::copy(property_names_begin, property_names_end, std::back_inserter(property_names_seq));

			// Remove duplicates.
			property_names_seq.erase(
					std::unique(property_names_seq.begin(), property_names_seq.end()),
					property_names_seq.end());

			// Search for the property names.
			GPlatesModel::FeatureHandle::iterator properties_iter = feature_handle.begin();
			GPlatesModel::FeatureHandle::iterator properties_end = feature_handle.end();
			for ( ; properties_iter != properties_end; ++properties_iter)
			{
				GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

				property_names_seq_type::iterator property_names_seq_iter = std::find(
						property_names_seq.begin(),
						property_names_seq.end(),
						feature_property->get_property_name());
				if (property_names_seq_iter != property_names_seq.end())
				{
					// Remove the property from the feature.
					// Note that removing a property does not prevent us from incrementing to the next property.
					feature_handle.remove(properties_iter);
				}
			}

			return;
		}
		catch (const boost::python::error_already_set &)
		{
			PyErr_Clear();
		}

		// Try a sequence of properties next.
		typedef std::vector<GPlatesModel::TopLevelProperty::non_null_ptr_type> properties_seq_type;
		properties_seq_type properties_seq;
		try
		{
			// Begin/end iterators over the python properties sequence.
			bp::stl_input_iterator<GPlatesModel::TopLevelProperty::non_null_ptr_type>
					properties_begin(properties_object),
					properties_end;

			// Copy into the vector.
			std::copy(properties_begin, properties_end, std::back_inserter(properties_seq));

			// Remove duplicate property pointers.
			properties_seq.erase(
					std::unique(properties_seq.begin(), properties_seq.end()),
					properties_seq.end());
		}
		catch (const boost::python::error_already_set &)
		{
			PyErr_Clear();

			PyErr_SetString(PyExc_TypeError,
					"Expected PropertyName or Property or sequence of PropertyName's or sequence of Property's");
			bp::throw_error_already_set();
		}

		// Search for the properties.
		GPlatesModel::FeatureHandle::iterator properties_iter = feature_handle.begin();
		GPlatesModel::FeatureHandle::iterator properties_end = feature_handle.end();
		for ( ; properties_iter != properties_end; ++properties_iter)
		{
			GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

			// Compare pointers not pointed-to-objects.
			properties_seq_type::iterator properties_seq_iter =
					std::find(properties_seq.begin(), properties_seq.end(), feature_property);
			if (properties_seq_iter != properties_seq.end())
			{
				// Remove the property from the feature.
				// Note that removing a property does not prevent us from incrementing to the next property.
				feature_handle.remove(properties_iter);
				// Record that we have removed this property.
				properties_seq.erase(properties_seq_iter);
			}
		}

		// Raise the 'ValueError' python exception if not all properties were found.
		if (!properties_seq.empty())
		{
			PyErr_SetString(PyExc_ValueError, "Not all property instances were found");
			bp::throw_error_already_set();
		}
	}
}


void
export_feature()
{
	//
	// Feature - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesModel::FeatureHandle,
			GPlatesModel::FeatureHandle::non_null_ptr_type,
			boost::noncopyable>(
					"Feature",
					"The feature is an abstract model of some geological or plate-tectonic object or "
					"concept of interest. It consists of a collection of properties, a feature type "
					"and a feature id.\n"
					"\n"
					"The following operations for accessing the properties are supported:\n"
					"\n"
					"=========================== ==========================================================\n"
					"Operation                   Result\n"
					"=========================== ==========================================================\n"
					"``len(f)``                  number of properties in feature *f*\n"
					"``for p in f``              iterates over the properties *p* in feature *f*\n"
					"=========================== ==========================================================\n"
					"\n"
					"For example:\n"
					"::\n"
					"\n"
					"  num_properties = len(feature)\n"
					"  properties_in_feature = [property for property in feature]\n"
					"  assert(num_properties == len(properties_in_feature))\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::feature_handle_create,
						bp::default_call_policies(),
						(bp::arg("feature_type") = boost::optional<GPlatesModel::FeatureType>(),
							bp::arg("feature_id") = boost::optional<GPlatesModel::FeatureId>(),
							bp::arg("verify_information_model") = GPlatesApi::VerifyInformationModel::YES)),
				"__init__([feature_type], [feature_id], [verify_information_model=VerifyInformationModel.yes])\n"
				"  Create a new feature instance that is (initially) empty (has no properties).\n"
				"\n"
				"  :param feature_type: the type of feature\n"
				"  :type feature_type: :class:`FeatureType`\n"
				"  :param feature_id: the feature identifier\n"
				"  :type feature_id: :class:`FeatureId`\n"
				"  :param verify_information_model: whether to check *feature_type* with the information model (default) or not\n"
				"  :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*\n"
				"  :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* "
				"and *feature_type* is not a recognised feature type\n"
				"\n"
				"  *feature_type* defaults to *gpml:UnclassifiedFeature* if not specified. "
				"There are no restrictions on the types and number of properties that can be added "
				"to features of type *gpml:UnclassifiedFeature* provided their property names are recognised by the "
				"GPlates Geological Information Model (GPGIM) (see http://www.gplates.org/gpml.html). "
				"However all other feature types are restricted to a subset of recognised properties. "
				"The restriction is apparent when the features are created explicitly (see :meth:`add`) "
				"and when features are *read* from a GPML format file (there are no restrictions "
				"when the features are *written* to a GPML format file).\n"
				"\n"
				"  If *feature_id* is not specified then a unique feature identifier is created. In most cases "
				"a specific *feature_id* should not be specified because it avoids the possibility of "
				"accidentally having two feature instances with the same identifier which can cause "
				"problems with *topological* geometries.\n"
				"  ::\n"
				"\n"
				"    unclassified_feature = pygplates.Feature()\n"
				"\n"
				"    # This does the same thing as the code above.\n"
				"    unclassified_feature = pygplates.Feature(\n"
				"        pygplates.FeatureType.create_gpml('UnclassifiedFeature'))\n")
		.def("__iter__", bp::iterator<GPlatesModel::FeatureHandle>())
		.def("__len__", &GPlatesModel::FeatureHandle::size)
#if 0 // TODO: Add once clone does a proper deep-copy...
		.def("clone",
				&GPlatesApi::feature_handle_clone,
				"clone() -> Feature\n"
				"  Create a duplicate of this feature instance, including a recursive copy "
				"of its property values.\n"
				"\n"
				"  :rtype: :class:`Feature`\n")
#endif
		.def("add",
				&GPlatesApi::feature_handle_add_property,
				(bp::arg("property_name"),
						bp::arg("property_value"),
						bp::arg("verify_information_model") = GPlatesApi::VerifyInformationModel::YES),
				"add(property_name, property_value, [verify_information_model=VerifyInformationModel.yes]) "
				"-> Property\n"
				"  Adds a property to this feature.\n"
				"\n"
				"  :param property_name: the name of the property\n"
				"  :type property_name: :class:`PropertyName`\n"
				"  :param property_value: the value of the property\n"
				"  :type property_value: :class:`PropertyValue`\n"
				"  :param verify_information_model: whether to check the information model before adding (default) or not\n"
				"  :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*\n"
				"  :returns: the property added to the feature\n"
				"  :rtype: :class:`Property`\n"
				"  :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* "
				"and *property_name* is not a recognised property name or is not supported by the feature type\n"
				"\n"
				"  ::\n"
				"\n"
				"    property_added = feature.add(property_name, property_value)\n"
				"\n"
				"  A feature is an *unordered* collection of properties so there is no concept of "
				"where a property is inserted in the sequence of properties.\n"
				"\n"
				"  Note that even a feature of type *gpml:UnclassifiedFeature* will raise *InformationModelError* "
				"if *verify_information_model* is *VerifyInformationModel.yes* and *property_name* is not "
				"recognised by the GPlates Geological Information Model (GPGIM).\n")
		.def("add",
				&GPlatesApi::feature_handle_add_properties,
				(bp::arg("properties"),
						bp::arg("verify_information_model") = GPlatesApi::VerifyInformationModel::YES),
				"add(properties, [verify_information_model=VerifyInformationModel.yes]) "
				"-> list\n"
				"  Adds properties to this feature.\n"
				"\n"
				"  :param properties: the property name/value pairs to add\n"
				"  :type properties: a sequence (eg, ``list`` or ``tuple``) of (:class:`PropertyName`, :class:`PropertyValue`)\n"
				"  :param verify_information_model: whether to check the information model before adding (default) or not\n"
				"  :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*\n"
				"  :returns: the list of properties added to the feature\n"
				"  :rtype: ``list`` of :class:`Property`\n"
				"  :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* "
				"and any of the property names are not recognised property names or not supported by the feature type\n"
				"\n"
				"  ::\n"
				"\n"
				"    properties_added = feature.add([(property_name1, property_value1), (property_name2, property_value2)])\n"
				"\n"
				"  A feature is an *unordered* collection of properties so there is no concept of where "
				"a property is inserted in the sequence of properties.\n"
				"\n"
				"  Note that even a feature of type *gpml:UnclassifiedFeature* will raise *InformationModelError* "
				"if *verify_information_model* is *VerifyInformationModel.yes* and a property name is not "
				"recognised by the GPlates Geological Information Model (GPGIM).\n")
		.def("remove",
				&GPlatesApi::feature_handle_remove,
				(bp::arg("properties")),
				"remove(properties)\n"
				"  Removes one or more properties from this feature.\n"
				"\n"
				"  :param properties: one or more property names or property instances\n"
				"  :type properties: :class:`PropertyName` or :class:`Property` or a sequence "
				"(eg, ``list`` or ``tuple``) of :class:`PropertyName` or a sequence of :class:`Property`\n"
				"  :raises: ValueError if any specified :class:`Property` is not currently a property in this feature\n"
				"\n"
				"  All feature properties matching each :class:`PropertyName` (if any specified) will "
				"be removed. Any specified :class:`PropertyName` that does not exist in this feature is ignored. "
				"However if any specified :class:`Property` is not currently a property in this feature "
				"then the ``ValueError`` exception is raised - note that the same property *instance* must "
				"have previously been added (in other words the property values are not compared - "
				"it actually looks for the same property *instance*).\n"
				"\n"
				"  ::\n"
				"\n"
				"    feature.remove(property_name)\n"
				"    feature.remove([property_name1, property_name2])\n"
				"    feature.remove(property)\n"
				"    feature.remove([property1, property2])\n")
		.def("get_feature_type",
				&GPlatesModel::FeatureHandle::feature_type,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_feature_type() -> FeatureType\n"
				"  Returns the feature type.\n"
				"\n"
				"  :rtype: :class:`FeatureType`\n")
		.def("get_feature_id",
				&GPlatesModel::FeatureHandle::feature_id,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_feature_id() -> FeatureId\n"
				"  Returns the feature identifier.\n"
				"\n"
				"  :rtype: :class:`FeatureId`\n")
	;

	// Enable boost::optional<FeatureHandle::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesModel::FeatureHandle::non_null_ptr_type>();

	// Registers 'non-const' to 'const' conversions.
	boost::python::implicitly_convertible<
			GPlatesModel::FeatureHandle::non_null_ptr_type,
			GPlatesModel::FeatureHandle::non_null_ptr_to_const_type>();
	boost::python::implicitly_convertible<
			boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type>,
			boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_to_const_type> >();
}

// This is here at the end of the layer because the problem appears to reside
// in a template being instantiated at the end of the compilation unit.
DISABLE_GCC_WARNING("-Wshadow")

#endif // GPLATES_NO_PYTHON
