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

#include "PyFeatureCollectionFileFormatRegistry.h"
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
			GPlatesModel::FeatureCollectionHandle &feature_collection_handle,
			GPlatesModel::FeatureHandle::non_null_ptr_type feature)
	{
		// Search for the feature.
		// Note: This searches for the same feature *instance* - it does not compare values of
		// two different feature instances.
		GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection_handle.begin();
		GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection_handle.end();
		for ( ; features_iter != features_end; ++features_iter)
		{
			GPlatesModel::FeatureHandle::non_null_ptr_type collection_feature = *features_iter;

			// Compare pointers not pointed-to-objects.
			if (feature == collection_feature)
			{
				feature_collection_handle.remove(features_iter);
				return;
			}
		}

		// Raise the 'ValueError' python exception if the feature was not found.
		PyErr_SetString(PyExc_ValueError, "Feature instance not found");
		bp::throw_error_already_set();
	}


	/**
	 * From-python converter from a string filename to a FeatureCollection.
	 */
	struct python_FeatureCollection :
			private boost::noncopyable
	{
		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			// If it's a string then it can be used as a filename to read a feature collection.
			return bp::extract<QString>(obj).check() ? obj : NULL;
		}

		static
		void
		construct(
				PyObject *obj,
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			namespace bp = boost::python;

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<
							GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> *>(
									data)->storage.bytes;

			const QString filename = bp::extract<QString>(obj);
			GPlatesFileIO::FeatureCollectionFileFormat::Registry file_registry;

			// Use the API function in "PyFeatureCollectionFileFormatRegistry.h" to read the file.
			new (storage) GPlatesModel::FeatureCollectionHandle::non_null_ptr_type(
					GPlatesApi::read_feature_collection(file_registry, filename));

			data->convertible = storage;
		}
	};


	/**
	 * Registers from-python converter from a string filename to a FeatureCollection.
	 */
	void
	register_feature_collection_from_python_conversion()
	{
		// From python conversion.
		bp::converter::registry::push_back(
				&python_FeatureCollection::convertible,
				&python_FeatureCollection::construct,
				bp::type_id<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>());
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
					"The following operations for accessing the features are supported:\n"
					"\n"
					"=========================== ==========================================================\n"
					"Operation                   Result\n"
					"=========================== ==========================================================\n"
					"``len(fc)``                 number of features in feature collection *fc*\n"
					"``for f in fc``             iterates over the features *f* in feature collection *fc*\n"
					"=========================== ==========================================================\n"
					"\n"
					"For example:\n"
					"::\n"
					"\n"
					"  num_features = len(feature_collection)\n"
					"  features_in_collection = [feature for feature in feature_collection]\n"
					"  assert(num_features == len(features_in_collection))\n"
					"\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(create),
				"__init__()\n"
				"  Create a new feature collection instance that is (initially) empty (has no features).\n"
				"  ::\n"
				"\n"
				"    feature_collection = pygplates.FeatureCollection()\n")
		.def("__iter__", bp::iterator<GPlatesModel::FeatureCollectionHandle>())
		.def("__len__", &GPlatesModel::FeatureCollectionHandle::size)
		.def("add",
				&GPlatesApi::feature_collection_handle_add,
				(bp::arg("feature")),
				"add(feature)\n"
				"  Adds a feature to this collection.\n"
				"\n"
				"  :param feature: the feature\n"
				"  :type feature: :class:`Feature`\n"
				"\n"
				"  A feature collection is an *unordered* collection of features "
				"so there is no concept of where a feature is inserted in the sequence of features.\n")
		.def("remove",
				&GPlatesApi::feature_collection_handle_remove,
				(bp::arg("feature")),
				"remove(feature)\n"
				"  Removes *feature* from this collection.\n"
				"\n"
				"  :param feature: a feature instance that currently exists inside this collection\n"
				"  :type feature: :class:`Feature`\n"
				"\n"
				"  Raises the ``ValueError`` exception if *feature* is not "
				"currently a feature in this collection. Note that the same feature *instance* must "
				"have previously been added. In other words, *remove* does not compare the value of "
				"*feature* with the values of the features of this collection - it actually looks for "
				"the same feature *instance*.\n")
	;

	// Enable boost::optional<FeatureCollectionHandle::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>();

	// Registers 'non-const' to 'const' conversions.
	boost::python::implicitly_convertible<
			GPlatesModel::FeatureCollectionHandle::non_null_ptr_type,
			GPlatesModel::FeatureCollectionHandle::non_null_ptr_to_const_type>();
	boost::python::implicitly_convertible<
			boost::optional<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>,
			boost::optional<GPlatesModel::FeatureCollectionHandle::non_null_ptr_to_const_type> >();

	// A string can be converted to a feature collection by using it as a feature collection filename.
	GPlatesApi::register_feature_collection_from_python_conversion();
}

#endif // GPLATES_NO_PYTHON
