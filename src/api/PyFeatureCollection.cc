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

#include <boost/foreach.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <QString>

#include "PyFeatureCollection.h"

#include "PythonConverterUtils.h"

#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/ReadErrorAccumulation.h"

#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/stl_iterator.hpp>

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
	 * Python converter from a feature collection or a string filename to a
	 * @a FeatureCollectionFunctionArgument (and vice versa).
	 */
	struct python_FeatureCollectionFunctionArgument :
			private boost::noncopyable
	{
		struct Conversion
		{
			static
			PyObject *
			convert(
					const FeatureCollectionFunctionArgument &function_arg)
			{
				namespace bp = boost::python;

				return bp::incref(function_arg.to_python().ptr());
			}
		};

		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			if (FeatureCollectionFunctionArgument::is_convertible(
				bp::object(bp::handle<>(bp::borrowed(obj)))))
			{
				return obj;
			}

			return NULL;
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
							FeatureCollectionFunctionArgument> *>(
									data)->storage.bytes;

			new (storage) FeatureCollectionFunctionArgument(
					bp::object(bp::handle<>(bp::borrowed(obj))));

			data->convertible = storage;
		}
	};


	/**
	 * Registers converter from a feature collection or a string filename to a @a FeatureCollectionFunctionArgument.
	 */
	void
	register_feature_collection_function_argument_conversion()
	{
		// Register function argument types variant.
		PythonConverterUtils::register_variant_conversion<
				FeatureCollectionFunctionArgument::function_argument_type>();

		// To python conversion.
		bp::to_python_converter<
				FeatureCollectionFunctionArgument,
				typename python_FeatureCollectionFunctionArgument::Conversion>();

		// From python conversion.
		bp::converter::registry::push_back(
				&python_FeatureCollectionFunctionArgument::convertible,
				&python_FeatureCollectionFunctionArgument::construct,
				bp::type_id<FeatureCollectionFunctionArgument>());
	}


	/**
	 * Python converter from a feature collection or a string filename or a sequence of feature
	 * collections and/or string filenames to a @a FeatureCollectionSequenceFunctionArgument (and vice versa).
	 */
	struct python_FeatureCollectionSequenceFunctionArgument :
			private boost::noncopyable
	{
		struct Conversion
		{
			static
			PyObject *
			convert(
					const FeatureCollectionSequenceFunctionArgument &function_arg)
			{
				namespace bp = boost::python;

				return bp::incref(function_arg.to_python().ptr());
			}
		};

		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			if (FeatureCollectionSequenceFunctionArgument::is_convertible(
				bp::object(bp::handle<>(bp::borrowed(obj)))))
			{
				return obj;
			}

			return NULL;
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
							FeatureCollectionSequenceFunctionArgument> *>(
									data)->storage.bytes;

			new (storage) FeatureCollectionSequenceFunctionArgument(
					bp::object(bp::handle<>(bp::borrowed(obj))));

			data->convertible = storage;
		}
	};


	/**
	 * Registers converter from a feature collection or a string filename to a @a FeatureCollectionSequenceFunctionArgument.
	 */
	void
	register_feature_collection_sequence_function_argument_conversion()
	{
		// Register function argument types variant.
		PythonConverterUtils::register_variant_conversion<
				FeatureCollectionSequenceFunctionArgument::function_argument_type>();

		// To python conversion.
		bp::to_python_converter<
				FeatureCollectionSequenceFunctionArgument,
				typename python_FeatureCollectionSequenceFunctionArgument::Conversion>();

		// From python conversion.
		bp::converter::registry::push_back(
				&python_FeatureCollectionSequenceFunctionArgument::convertible,
				&python_FeatureCollectionSequenceFunctionArgument::construct,
				bp::type_id<FeatureCollectionSequenceFunctionArgument>());
	}
}


bool
GPlatesApi::FeatureCollectionFunctionArgument::is_convertible(
		bp::object python_function_argument)
{
	return bp::extract<function_argument_type>(python_function_argument).check();
}


GPlatesApi::FeatureCollectionFunctionArgument::FeatureCollectionFunctionArgument(
		bp::object python_function_argument) :
	d_feature_collection(
			initialise_feature_collection(
					bp::extract<function_argument_type>(python_function_argument)))
{
}


GPlatesApi::FeatureCollectionFunctionArgument::FeatureCollectionFunctionArgument(
		const function_argument_type &function_argument) :
	d_feature_collection(initialise_feature_collection(function_argument))
{
}


GPlatesFileIO::File::non_null_ptr_type
GPlatesApi::FeatureCollectionFunctionArgument::initialise_feature_collection(
		const function_argument_type &function_argument)
{
	if (const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type *feature_collection =
		boost::get<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>(&function_argument))
	{
		// Create a file with an empty filename - since we don't know if feature collection
		// came from a file or not.
		return GPlatesFileIO::File::create_file(GPlatesFileIO::FileInfo(), *feature_collection);
	}

	const QString filename = boost::get<QString>(function_argument);

	// Create a file with an empty feature collection.
	GPlatesFileIO::File::non_null_ptr_type file =
			GPlatesFileIO::File::create_file(GPlatesFileIO::FileInfo(filename));

	// Read new features from the file into the feature collection.
	GPlatesFileIO::FeatureCollectionFileFormat::Registry file_registry;
	GPlatesFileIO::ReadErrorAccumulation read_errors;
	file_registry.read_feature_collection(file->get_reference(), read_errors);

	return file;
}


bp::object
GPlatesApi::FeatureCollectionFunctionArgument::to_python() const
{
	// Wrap feature collection in a python object.
	return bp::object(get_feature_collection());
}


GPlatesModel::FeatureCollectionHandle::non_null_ptr_type
GPlatesApi::FeatureCollectionFunctionArgument::get_feature_collection() const
{
	// Extract the feature collection contained within the file.
	return GPlatesUtils::get_non_null_pointer(
			d_feature_collection->get_reference().get_feature_collection().handle_ptr());
}


GPlatesFileIO::File::non_null_ptr_type
GPlatesApi::FeatureCollectionFunctionArgument::get_file() const
{
	return d_feature_collection;
}


bool
GPlatesApi::FeatureCollectionSequenceFunctionArgument::is_convertible(
		bp::object python_function_argument)
{
	// If we fail to extract or iterate over the supported types then catch exception and return NULL.
	try
	{
		if (bp::extract<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>(python_function_argument).check() ||
			bp::extract<QString>(python_function_argument).check())
		{
			return true;
		}

		// Else it's a boost::python::object so we're expecting it to be a sequence of
		// FeatureCollectionFunctionArgument's which requires further checking.

		const bp::object sequence = python_function_argument;

		// Iterate over the sequence.
		//
		// NOTE: We avoid iterating using 'bp::stl_input_iterator<FeatureCollectionFunctionArgument>'
		// because we want to avoid actually reading a feature collection from a file.
		// We're just checking if there's a feature collection or a string here.
		bp::object iter = sequence.attr("__iter__")();
		while (bp::handle<> item = bp::handle<>(bp::allow_null(PyIter_Next(iter.ptr()))))
		{
			if (!bp::extract<FeatureCollectionFunctionArgument>(bp::object(item)).check())
			{
				return false;
			}
		}

		if (PyErr_Occurred())
		{
			PyErr_Clear();
			return false;
		}

		return true;
	}
	catch (const bp::error_already_set &)
	{
		PyErr_Clear();
	}

	return false;
}


GPlatesApi::FeatureCollectionSequenceFunctionArgument::FeatureCollectionSequenceFunctionArgument(
		bp::object python_function_argument)
{
	initialise_feature_collections(
			d_feature_collections,
			bp::extract<function_argument_type>(python_function_argument));
}


GPlatesApi::FeatureCollectionSequenceFunctionArgument::FeatureCollectionSequenceFunctionArgument(
		const function_argument_type &function_argument)
{
	initialise_feature_collections(d_feature_collections, function_argument);
}


GPlatesApi::FeatureCollectionSequenceFunctionArgument::FeatureCollectionSequenceFunctionArgument(
		const std::vector<FeatureCollectionFunctionArgument> &feature_collections) :
	d_feature_collections(feature_collections)
{
}


void
GPlatesApi::FeatureCollectionSequenceFunctionArgument::initialise_feature_collections(
		std::vector<FeatureCollectionFunctionArgument> &feature_collections,
		const function_argument_type &function_argument)
{
	if (const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type *feature_collection =
		boost::get<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>(&function_argument))
	{
		feature_collections.push_back(FeatureCollectionFunctionArgument(*feature_collection));
	}
	else if (const QString *filename = boost::get<QString>(&function_argument))
	{
		feature_collections.push_back(FeatureCollectionFunctionArgument(*filename));
	}
	else
	{
		//
		// A sequence of feature collections and/or filenames.
		//

		const bp::object sequence = boost::get<bp::object>(function_argument);

		// Use convenience class 'FeatureCollectionFunctionArgument' to access the feature collections.
		bp::stl_input_iterator<FeatureCollectionFunctionArgument> feature_collections_iter(sequence);
		bp::stl_input_iterator<FeatureCollectionFunctionArgument> feature_collections_end;
		for ( ; feature_collections_iter != feature_collections_end; ++feature_collections_iter)
		{
			feature_collections.push_back(*feature_collections_iter);
		}
	}
}


bp::object
GPlatesApi::FeatureCollectionSequenceFunctionArgument::to_python() const
{
	// Add feature collections to a python list.
	bp::list python_feature_collections;
	BOOST_FOREACH(const FeatureCollectionFunctionArgument &feature_collection, d_feature_collections)
	{
		python_feature_collections.append(feature_collection.get_feature_collection());
	}

	return python_feature_collections;
}


void
GPlatesApi::FeatureCollectionSequenceFunctionArgument::get_feature_collections(
		std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> &feature_collections) const
{
	BOOST_FOREACH(const FeatureCollectionFunctionArgument &feature_collection, d_feature_collections)
	{
		feature_collections.push_back(feature_collection.get_feature_collection());
	}
}


void
GPlatesApi::FeatureCollectionSequenceFunctionArgument::get_files(
		std::vector<GPlatesFileIO::File::non_null_ptr_type> &feature_collection_files) const
{
	BOOST_FOREACH(const FeatureCollectionFunctionArgument &feature_collection, d_feature_collections)
	{
		feature_collection_files.push_back(feature_collection.get_file());
	}
}


void
export_feature_collection()
{
	// Select the desired overload...
	const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type (*feature_collection_create)() =
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
				bp::make_constructor(feature_collection_create),
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

	// Register converter from a feature collection or a string filename to a @a FeatureCollectionFunctionArgument.
	GPlatesApi::register_feature_collection_function_argument_conversion();

	// Register converter from a feature collection or a string filename to a @a FeatureCollectionSequenceFunctionArgument.
	GPlatesApi::register_feature_collection_sequence_function_argument_conversion();
}

#endif // GPLATES_NO_PYTHON
