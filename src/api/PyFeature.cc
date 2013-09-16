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

#include "global/CompilerWarnings.h"
#include "global/python.h"

#include "model/FeatureHandle.h"
#include "model/FeatureId.h"
#include "model/FeatureType.h"
#include "model/RevisionId.h"
#include "model/TopLevelProperty.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	const GPlatesModel::FeatureHandle::non_null_ptr_type
	feature_handle_create(
			boost::optional<GPlatesModel::FeatureType> feature_type = boost::none,
			boost::optional<GPlatesModel::FeatureId> feature_id = boost::none,
			boost::optional<GPlatesModel::RevisionId> revision_id = boost::none)
	{
		// Default to unclassified feature - since that supports any combination of properties.
		if (!feature_type)
		{
			feature_type = GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature");
		}

		// Create a unique feature id if none specified.
		if (!feature_id)
		{
			feature_id = GPlatesModel::FeatureId();
		}

		// Create a unique revision id if none specified.
		if (!revision_id)
		{
			revision_id = GPlatesModel::RevisionId();
		}

		return GPlatesModel::FeatureHandle::create(feature_type.get(), feature_id.get(), revision_id.get());
	}

DISABLE_GCC_WARNING("-Wshadow")
	// Default argument overloads of 'GPlatesModel::FeatureHandle::create'.
	BOOST_PYTHON_FUNCTION_OVERLOADS(
			feature_handle_create_overloads,
			feature_handle_create, 0, 3)
ENABLE_GCC_WARNING("-Wshadow")

	void
	feature_handle_add(
			GPlatesModel::FeatureHandle::non_null_ptr_type feature_handle,
			GPlatesModel::TopLevelProperty::non_null_ptr_type property)
	{
		// Ignore the returned iterator.
		feature_handle->add(property);
	}

	void
	feature_handle_remove(
			GPlatesModel::FeatureHandle::non_null_ptr_type feature_handle,
			GPlatesModel::TopLevelProperty::non_null_ptr_type property)
	{
		// Search for the property.
		// Note: This searches for the same property *instance* - it does not compare values of
		// two different property instances.
		GPlatesModel::FeatureHandle::iterator properties_iter = feature_handle->begin();
		GPlatesModel::FeatureHandle::iterator properties_end = feature_handle->end();
		for ( ; properties_iter != properties_end; ++properties_iter)
		{
			GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

			// Compare pointers not pointed-to-objects.
			if (property == feature_property)
			{
				feature_handle->remove(properties_iter);
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
					"Since a feature contains revisioned content it also has a :class:`revision id<RevisionId>`. "
					"Each time a property of a feature is modified, a new revision id is generated for the feature. "
					"The concept of revisioning comes into play with model undo/redo and can generally be ignored.\n"
					"\n"
					"A feature instance is iterable over its properties (and supports the ``len()`` function):\n"
					"  ::\n"
					"\n"
					"    num_properties = len(feature)\n"
					"    properties_in_feature = [property for property in feature]\n"
					"    assert(num_properties == len(properties_in_feature))\n"
					"\n",
					bp::no_init)
		.def("create",
				&GPlatesApi::feature_handle_create,
				GPlatesApi::feature_handle_create_overloads(
					(bp::arg("feature_type") = boost::optional<GPlatesModel::FeatureType>(),
						bp::arg("feature_id") = boost::optional<GPlatesModel::FeatureId>(),
						bp::arg("revision_id") = boost::optional<GPlatesModel::RevisionId>()),
					"create([feature_type=None[, feature_id=None[, revision_id=None]]]) -> Feature\n"
					"  Create a new feature instance that is (initially) empty (has no properties).\n"
					"\n"
					"  *feature_type* defaults to *gpml:UnclassifiedFeature* if not specified. "
					"There are no restrictions on the types and number of properties that can be added "
					"to features of type *gpml:UnclassifiedFeature*. However all other feature types "
					"are restricted according to the GPlates Geological Information Model (GPGIM) "
					"(see http://www.gplates.org/gpml.html). The restriction is only apparent "
					"when the features are *read* from a GPML format file (there are no restrictions "
					"when the features are *written* to a GPML format file). *TODO: Support GPGIM validation*.\n"
					"\n"
					"  If *feature_id* is not specified then a unique feature identifier is created. In most cases "
					"a specific *feature_id* should not be specified because it avoids the possibility of "
					"accidentally having two feature instances with the same identifier which can cause "
					"problems with *topological* geometries.\n"
					"\n"
					"  If *revision_id* is not specified then a unique revision identifier is created. "
					"In most cases a specific *revision_id* does not need to be specified.\n"
					"  ::\n"
					"\n"
					"    unclassified_feature = pygplates.Feature.create()\n"
					"\n"
					"    # This does the same thing as the code above.\n"
					"    unclassified_feature = pygplates.Feature.create(\n"
					"        pygplates.FeatureType.create_gpml('UnclassifiedFeature'))\n"
					"\n"
					"  :param feature_type: the type of feature\n"
					"  :type feature_type: :class:`FeatureType`\n"
					"  :param feature_id: the feature identifier\n"
					"  :type feature_id: :class:`FeatureId`\n"
					"  :param revision_id: the current revision identifier\n"
					"  :type revision_id: :class:`RevisionId`\n"))
 		.staticmethod("create")
		.def("__iter__", bp::iterator<GPlatesModel::FeatureHandle>())
#if 0 // TODO: Add once clone does a proper deep-copy...
		.def("clone",
				&GPlatesApi::feature_handle_clone,
				"clone() -> Feature\n"
				"  Create a duplicate of this feature instance, including a recursive copy "
				"of its property values.\n"
				"\n"
				"  :rtype: :class:`Feature`\n")
#endif
		.def("__len__", &GPlatesModel::FeatureHandle::size)
		.def("add",
				&GPlatesApi::feature_handle_add,
				(bp::arg("property")),
				"add(property)\n"
				"  Adds a property to this feature. A feature is an *unordered* collection of properties "
				"so there is no concept of where a property is inserted in the sequence of properties.\n"
				"\n"
				"  :param property: the property\n"
				"  :type property: :class:`Property`\n")
		.def("remove",
				&GPlatesApi::feature_handle_remove,
				(bp::arg("property")),
				"remove(property)\n"
				"  Removes *property* from this feature. Raises the ``ValueError`` exception if *property* is not "
				"currently a property in this feature. Note that the same property *instance* must "
				"have previously been added. In other words, *remove* does not compare the value of "
				"*property* with the values of the properties of this feature - it actually looks for "
				"the same property *instance*.\n"
				"\n"
				"  :param property: a property instance that currently exists inside this feature\n"
				"  :type property: :class:`Property`\n")
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
		.def("get_revision_id",
				&GPlatesModel::FeatureHandle::revision_id,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_revision_id() -> RevisionId\n"
				"  Returns the current revision identifier. The revision identifier changes each time "
				"the feature instance is modified (such as adding, removing or modifying a property).\n"
				"\n"
				"  :rtype: :class:`RevisionId`\n")
	;

	// Enable boost::optional<FeatureHandle::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesModel::FeatureHandle::non_null_ptr_type>();

	// Registers 'non-const' to 'const' conversions.
	boost::python::implicitly_convertible<
			GPlatesModel::FeatureHandle::non_null_ptr_type,
			GPlatesModel::FeatureHandle::non_null_ptr_to_const_type>();
	boost::python::implicitly_convertible<
			boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type>,
			boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_to_const_type> >();
}

#endif // GPLATES_NO_PYTHON
