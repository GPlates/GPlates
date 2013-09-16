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

#include "global/python.h"

#include "model/FeatureHandle.h"
#include "model/FeatureCollectionHandle.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	void
	feature_collection_handle_add(
			GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection_handle,
			GPlatesModel::FeatureHandle::non_null_ptr_type feature)
	{
		// Ignore the returned iterator.
		feature_collection_handle->add(feature);
	}

	void
	feature_collection_handle_remove(
			GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection_handle,
			GPlatesModel::FeatureHandle::non_null_ptr_type feature)
	{
		// Search for the feature.
		// Note: This searches for the same feature *instance* - it does not compare values of
		// two different feature instances.
		GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection_handle->begin();
		GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection_handle->end();
		for ( ; features_iter != features_end; ++features_iter)
		{
			GPlatesModel::FeatureHandle::non_null_ptr_type collection_feature = *features_iter;

			// Compare pointers not pointed-to-objects.
			if (feature == collection_feature)
			{
				feature_collection_handle->remove(features_iter);
				return;
			}
		}

		// Raise the 'ValueError' python exception if the feature was not found.
		PyErr_SetString(PyExc_ValueError, "Feature instance not found");
		bp::throw_error_already_set();
	}
}


void
export_feature_collection()
{
	// Select the desired overload...
	const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type (*create)() =
					&GPlatesModel::FeatureCollectionHandle::create;

	//
	// FeatureCollection - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesModel::FeatureCollectionHandle,
			GPlatesModel::FeatureCollectionHandle::non_null_ptr_type,
			boost::noncopyable>(
					"FeatureCollection",
					"The feature collection aggregates a set of features into a collection. This is traditionally "
					"so that a group of features can be loaded, saved or unloaded in a single operation.\n"
					"\n"
					"A feature collection instance is iterable over its features (and supports the ``len()`` function):\n"
					"  ::\n"
					"\n"
					"    num_features = len(feature_collection)\n"
					"    features_in_collection = [feature for feature in feature_collection]\n"
					"    assert(num_features == len(features_in_collection))\n"
					"\n",
					bp::no_init)
		.def("create",
				create,
				"create() -> FeatureCollection\n"
				"  Create a new feature collection instance that is (initially) empty (has no features).\n"
				"  ::\n"
				"\n"
				"    feature_collection = pygplates.FeatureCollection.create()\n")
 		.staticmethod("create")
		.def("__iter__", bp::iterator<GPlatesModel::FeatureCollectionHandle>())
		.def("__len__", &GPlatesModel::FeatureCollectionHandle::size)
		.def("add",
				&GPlatesApi::feature_collection_handle_add,
				(bp::arg("feature")),
				"add(feature)\n"
				"  Adds a feature to this collection. A feature collection is an *unordered* collection of features "
				"so there is no concept of where a feature is inserted in the sequence of features.\n"
				"\n"
				"  :param feature: the feature\n"
				"  :type feature: :class:`Feature`\n")
		.def("remove",
				&GPlatesApi::feature_collection_handle_remove,
				(bp::arg("feature")),
				"remove(feature)\n"
				"  Removes *feature* from this collection. Raises the ``ValueError`` exception if *feature* is not "
				"currently a feature in this collection. Note that the same feature *instance* must "
				"have previously been added. In other words, *remove* does not compare the value of "
				"*feature* with the values of the features of this collection - it actually looks for "
				"the same feature *instance*.\n"
				"\n"
				"  :param feature: a feature instance that currently exists inside this collection\n"
				"  :type feature: :class:`Feature`\n")
	;

	// Enable boost::optional<FeatureCollectionHandle::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>();

	// Registers 'non-const' to 'const' conversions.
	boost::python::implicitly_convertible<
			GPlatesModel::FeatureCollectionHandle::non_null_ptr_type,
			GPlatesModel::FeatureCollectionHandle::non_null_ptr_to_const_type>();
	boost::python::implicitly_convertible<
			boost::optional<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>,
			boost::optional<GPlatesModel::FeatureCollectionHandle::non_null_ptr_to_const_type> >();
}

#endif // GPLATES_NO_PYTHON
