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
#include <QString>

#include "PyInformationModel.h"
#include "PythonConverterUtils.h"

#include "global/GPlatesAssert.h"
#include "global/python.h"

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

	void
	feature_handle_remove_property(
			GPlatesModel::FeatureHandle &feature_handle,
			const GPlatesModel::PropertyName &property_name)
	{
		// Search for the property name.
		GPlatesModel::FeatureHandle::iterator properties_iter = feature_handle.begin();
		GPlatesModel::FeatureHandle::iterator properties_end = feature_handle.end();
		for ( ; properties_iter != properties_end; ++properties_iter)
		{
			GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

			// Compare pointers not pointed-to-objects.
			if (property_name == feature_property->get_property_name())
			{
				// Note that removing a property does not prevent us from incrementing to the next property.
				feature_handle.remove(properties_iter);
			}
		}
	}

	void
	feature_handle_remove(
			GPlatesModel::FeatureHandle &feature_handle,
			GPlatesModel::TopLevelProperty::non_null_ptr_type property)
	{
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
				"The restriction is apparent when the features are created explicitly (see :meth:`add_property`) "
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
		.def("add_property",
				&GPlatesApi::feature_handle_add_property,
				(bp::arg("property_name"),
						bp::arg("property_value"),
						bp::arg("verify_information_model") = GPlatesApi::VerifyInformationModel::YES),
				"add_property(property_name, property_value, [verify_information_model=VerifyInformationModel.yes])\n"
				"  Adds a property to this feature.\n"
				"\n"
				"  :param property_name: the name of the property\n"
				"  :type property_name: :class:`PropertyName`\n"
				"  :param property_value: the value of the property\n"
				"  :type property_value: :class:`PropertyValue`\n"
				"  :param verify_information_model: whether to check the information model before adding (default) or not\n"
				"  :type verify_information_model: *VerifyInformationModel.yes* or *VerifyInformationModel.no*\n"
				"  :raises: InformationModelError if *verify_information_model* is *VerifyInformationModel.yes* "
				"and *property_name* is not a recognised property name or is not supported by the feature type\n"
				"\n"
				"  A feature is an *unordered* collection of properties "
				"so there is no concept of where a property is inserted in the sequence of properties.\n"
				"\n"
				"  Note that even a feature of type *gpml:UnclassifiedFeature* will raise *InformationModelError* "
				"if *verify_information_model* is *VerifyInformationModel.yes* and *property_name* is not "
				"recognised by the GPlates Geological Information Model (GPGIM).\n")
		.def("remove_property",
				&GPlatesApi::feature_handle_remove_property,
				(bp::arg("property_name")),
				"remove_property(property_name)\n"
				"  Removes all properties named *property_name* from this feature.\n"
				"\n"
				"  :param property: the name of the property(s) to remove\n"
				"  :type property: :class:`PropertyName`\n"
				"\n"
				"  This method does nothing if there are no properties named *property_name*.\n")
		.def("remove",
				&GPlatesApi::feature_handle_remove,
				(bp::arg("property")),
				"remove(property)\n"
				"  Removes *property* from this feature.\n"
				"\n"
				"  :param property: a property instance that currently exists inside this feature\n"
				"  :type property: :class:`Property`\n"
				"\n"
				"  Raises the ``ValueError`` exception if *property* is not "
				"currently a property in this feature. Note that the same property *instance* must "
				"have previously been added. In other words, *remove* does not compare the value of "
				"*property* with the values of the properties of this feature - it actually looks for "
				"the same property *instance*.\n")
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
