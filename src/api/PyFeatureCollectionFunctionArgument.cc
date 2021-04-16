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

#include "PyFeatureCollectionFunctionArgument.h"

#include "PythonConverterUtils.h"
#include "PythonExtractUtils.h"
#include "PythonHashDefVisitor.h"

#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/ReadErrorAccumulation.h"

#include "global/python.h"

#include "model/FeatureHandle.h"
#include "model/FeatureCollectionHandle.h"

#include "utils/ReferenceCount.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * A from-python converter from a feature collection or a string filename to a
	 * @a FeatureCollectionFunctionArgument.
	 */
	struct ConversionFeatureCollectionFunctionArgument :
			private boost::noncopyable
	{
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
	register_conversion_feature_collection_function_argument()
	{
		// Register function argument types variant.
		PythonConverterUtils::register_variant_conversion<
				FeatureCollectionFunctionArgument::function_argument_type>();

		// NOTE: We don't define a to-python conversion.

		// From python conversion.
		bp::converter::registry::push_back(
				&ConversionFeatureCollectionFunctionArgument::convertible,
				&ConversionFeatureCollectionFunctionArgument::construct,
				bp::type_id<FeatureCollectionFunctionArgument>());
	}


	/**
	 * A from-python converter from a feature collection or a string filename or a sequence of feature
	 * collections and/or string filenames to a @a FeatureCollectionSequenceFunctionArgument.
	 */
	struct ConversionFeatureCollectionSequenceFunctionArgument :
			private boost::noncopyable
	{
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
	register_conversion_feature_collection_sequence_function_argument()
	{
		// Register function argument types variant.
		PythonConverterUtils::register_variant_conversion<
				FeatureCollectionSequenceFunctionArgument::function_argument_type>();

		// NOTE: We don't define a to-python conversion.

		// From python conversion.
		bp::converter::registry::push_back(
				&ConversionFeatureCollectionSequenceFunctionArgument::convertible,
				&ConversionFeatureCollectionSequenceFunctionArgument::construct,
				bp::type_id<FeatureCollectionSequenceFunctionArgument>());
	}
}


bool
GPlatesApi::FeatureCollectionFunctionArgument::is_convertible(
		bp::object python_function_argument)
{
	// Test all supported types (in function_argument_type) except the bp::object (since that's a sequence).
	if (bp::extract<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>(python_function_argument).check() ||
		bp::extract<QString>(python_function_argument).check() ||
		bp::extract<GPlatesModel::FeatureHandle::non_null_ptr_type>(python_function_argument).check())
	{
		return true;
	}

	// Else it's a boost::python::object so we're expecting it to be a sequence of
	// GPlatesModel::FeatureHandle::non_null_ptr_type's which requires further checking.
	return static_cast<bool>(
			PythonExtractUtils::check_sequence<GPlatesModel::FeatureHandle::non_null_ptr_type>(python_function_argument));
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
	if (const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type *feature_collection_function_argument =
		boost::get<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>(&function_argument))
	{
		// Create a file with an empty filename - since we don't know if feature collection
		// came from a file or not.
		return GPlatesFileIO::File::create_file(GPlatesFileIO::FileInfo(), *feature_collection_function_argument);
	}
	else if (const QString *filename_function_argument =
		boost::get<QString>(&function_argument))
	{
		// Create a file with an empty feature collection.
		GPlatesFileIO::File::non_null_ptr_type file =
				GPlatesFileIO::File::create_file(GPlatesFileIO::FileInfo(*filename_function_argument));

		// Read new features from the file into the feature collection.
		GPlatesFileIO::FeatureCollectionFileFormat::Registry file_registry;
		GPlatesFileIO::ReadErrorAccumulation read_errors;
		file_registry.read_feature_collection(file->get_reference(), read_errors);

		return file;
	}
	else if (const GPlatesModel::FeatureHandle::non_null_ptr_type *feature_function_argument =
		boost::get<GPlatesModel::FeatureHandle::non_null_ptr_type>(&function_argument))
	{
		// Create a feature collection with a single feature.
		GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection =
				GPlatesModel::FeatureCollectionHandle::create();
		feature_collection->add(*feature_function_argument);

		// Create a file with an empty filename - since feature collection didn't come from a file.
		return GPlatesFileIO::File::create_file(GPlatesFileIO::FileInfo(), feature_collection);
	}
	else
	{
		//
		// A sequence of features.
		//

		// Create a feature collection to add the features to.
		GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection =
				GPlatesModel::FeatureCollectionHandle::create();

		const bp::object sequence = boost::get<bp::object>(function_argument);
		std::vector<GPlatesModel::FeatureHandle::non_null_ptr_type> features;
		PythonExtractUtils::extract_sequence(features, sequence);

		for (GPlatesModel::FeatureHandle::non_null_ptr_type feature : features)
		{
			feature_collection->add(feature);
		}

		// Create a file with an empty filename - since feature collection didn't come from a file.
		return GPlatesFileIO::File::create_file(GPlatesFileIO::FileInfo(), feature_collection);
	}
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
	// Test all supported types (in function_argument_type) except the bp::object (since that's a sequence).
	if (bp::extract<FeatureCollectionFunctionArgument>(python_function_argument).check())
	{
		return true;
	}

	// Else it's a boost::python::object so we're expecting it to be a sequence of
	// FeatureCollectionFunctionArgument's which requires further checking.
	return static_cast<bool>(
			PythonExtractUtils::check_sequence<FeatureCollectionFunctionArgument>(python_function_argument));
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


void
GPlatesApi::FeatureCollectionSequenceFunctionArgument::initialise_feature_collections(
		std::vector<FeatureCollectionFunctionArgument> &feature_collections,
		const function_argument_type &function_argument)
{
	if (const FeatureCollectionFunctionArgument *feature_collection_function_argument =
		boost::get<FeatureCollectionFunctionArgument>(&function_argument))
	{
		feature_collections.push_back(*feature_collection_function_argument);
	}
	else
	{
		//
		// A sequence of feature collections and/or filenames.
		//

		const bp::object sequence = boost::get<bp::object>(function_argument);

		// Use convenience class 'FeatureCollectionFunctionArgument' to access the feature collections.
		PythonExtractUtils::extract_sequence(feature_collections, sequence);
	}
}


void
GPlatesApi::FeatureCollectionSequenceFunctionArgument::get_feature_collections(
		std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> &feature_collections) const
{
	for (const FeatureCollectionFunctionArgument &feature_collection : d_feature_collections)
	{
		feature_collections.push_back(feature_collection.get_feature_collection());
	}
}


void
GPlatesApi::FeatureCollectionSequenceFunctionArgument::get_files(
		std::vector<GPlatesFileIO::File::non_null_ptr_type> &feature_collection_files) const
{
	for (const FeatureCollectionFunctionArgument &feature_collection : d_feature_collections)
	{
		feature_collection_files.push_back(feature_collection.get_file());
	}
}


namespace GPlatesApi
{
	/**
	 * This is a convenience wrapper class for python users to access the functionality provided
	 * by 'FeatureCollectionSequenceFunctionArgument' (which is otherwise only available to C++ code).
	 */
	class FeaturesFunctionArgument :
			public GPlatesUtils::ReferenceCount<FeaturesFunctionArgument>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<FeaturesFunctionArgument> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const FeaturesFunctionArgument> non_null_ptr_to_const_type;


		/**
		 * Returns true if can extract features from python object (function argument).
		 */
		static
		bool
		contains_features(
				bp::object function_argument)
		{
			return FeatureCollectionSequenceFunctionArgument::is_convertible(function_argument);
		}

		static
		non_null_ptr_type
		create(
				const FeatureCollectionSequenceFunctionArgument &features_function_argument)
		{
			return non_null_ptr_type(new FeaturesFunctionArgument(features_function_argument));
		}


		/**
		 * Extract a list of features from the function argument.
		 */
		bp::list
		get_features() const
		{
			std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> feature_collections;
			d_features_function_argument.get_feature_collections(feature_collections);

			// Add the features in the feature collections to a python list.
			bp::list features_list_object;

			for (GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection : feature_collections)
			{
				// Iterate over the features in the collection.
				GPlatesModel::FeatureCollectionHandle::iterator feature_collection_iter = feature_collection->begin();
				GPlatesModel::FeatureCollectionHandle::iterator feature_collection_end = feature_collection->end();
				for ( ; feature_collection_iter != feature_collection_end; ++feature_collection_iter)
				{
					GPlatesModel::FeatureHandle::non_null_ptr_type feature = *feature_collection_iter;

					features_list_object.append(feature);
				}
			}

			return features_list_object;
		}

		/**
		 * Extract a list of (feature collection, filename) tuples that came from existing files.
		 *
		 * Feature collections that did not come from files are not included in the returned list.
		 */
		bp::list
		get_files() const
		{
			const std::vector<FeatureCollectionFunctionArgument> &feature_collection_function_arguments =
					d_features_function_argument.get_feature_collection_function_arguments();

			bp::list feature_collection_files_list_object;

			// Add (feature collection, filename) tuples to a python list.
			for (const FeatureCollectionFunctionArgument &feature_collection_function_argument : feature_collection_function_arguments)
			{
				// Get the file.
				GPlatesFileIO::File::non_null_ptr_type feature_collection_file =
						feature_collection_function_argument.get_file();

				// Skip feature collections that didn't come from (existing) files.
				if (!feature_collection_file->get_reference().get_file_info().get_qfileinfo().exists())
				{
					continue;
				}

				const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection =
						GPlatesUtils::get_non_null_pointer(
								feature_collection_file->get_reference().get_feature_collection().handle_ptr());

				QString feature_collection_filename =
						feature_collection_file->get_reference().get_file_info().get_qfileinfo().absoluteFilePath();

				feature_collection_files_list_object.append(
						bp::make_tuple(feature_collection, feature_collection_filename));
			}

			return feature_collection_files_list_object;
		}

	private:

		explicit
		FeaturesFunctionArgument(
				const FeatureCollectionSequenceFunctionArgument &features_function_argument) :
			d_features_function_argument(features_function_argument)
		{  }

		FeatureCollectionSequenceFunctionArgument d_features_function_argument;

	};
}


void
export_feature_collection_function_argument()
{
	// Register converter from a feature collection or a string filename to a @a FeatureCollectionFunctionArgument.
	GPlatesApi::register_conversion_feature_collection_function_argument();

	// Register converter from a feature collection or a string filename to a @a FeatureCollectionSequenceFunctionArgument.
	GPlatesApi::register_conversion_feature_collection_sequence_function_argument();


	//
	// FeaturesFunctionArgument - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	// This is a convenience wrapper class for python users to access the functionality provided
	// by 'FeatureCollectionSequenceFunctionArgument' (which is otherwise only available to C++ code).
	//
	bp::class_<
			GPlatesApi::FeaturesFunctionArgument,
			GPlatesApi::FeaturesFunctionArgument::non_null_ptr_type,
			boost::noncopyable>(
					"FeaturesFunctionArgument",
					"A utility class for extracting features from collections and files.\n"
					"\n"
					"This is useful when defining your own function that accepts features from a variety "
					"of sources. It avoids the hassle of having to explicitly test for each source type.\n"
					"\n"
					"The currently supported source types are:\n"
					"\n"
					"* :class:`FeatureCollection`\n"
					"* filename (string)\n"
					"* :class:`Feature`\n"
					"* sequence of :class:`Feature`\n"
					"* sequence of any combination of the above four types\n"
					"\n"
					"The following is an example of a user-defined function that accepts features "
					"in any of the above forms:\n"
					"::\n"
					"\n"
					"  def my_function(features):\n"
					"      # Turn function argument into something more convenient for extracting features.\n"
					"      features = pygplates.FeaturesFunctionArgument(features)\n"
					"      \n"
					"      # Iterate over features from the function argument.\n"
					"      for feature in features.get_features()\n"
					"          ...\n"
					"  \n"
					"  # Some examples of calling the above function:\n"
					"  my_function('file.gpml')\n"
					"  my_function(['file1.gpml', 'file2.gpml'])\n"
					"  my_function(['file.gpml', feature_collection])\n"
					"  my_function([feature1, feature2])\n"
					"  my_function([feature_collection,  feature1, feature2 ])\n"
					"  my_function([feature_collection, [feature1, feature2]])\n"
					"  my_function(feature)\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::FeaturesFunctionArgument::create,
						bp::default_call_policies(),
						(bp::arg("function_argument"))),
				"__init__(function_argument)\n"
				"  Extract features from files and/or collections of features.\n"
				"\n"
				"  :param function_argument: A feature collection, or filename, or feature, or "
				"sequence of features, or a sequence (eg, ``list`` or ``tuple``) of any combination "
				"of those four types\n"
				"  :type function_argument: :class:`FeatureCollection`, or string, or :class:`Feature`, "
				"or sequence of :class:`Feature`, or sequence of any combination of those four types\n"
				"  :raises: OpenFileForReadingError if any file is not readable (when filenames specified)\n"
				"  :raises: FileFormatNotSupportedError if any file format (identified by the filename "
				"extensions) does not support reading (when filenames specified)\n"
				"\n"
				"  The features are extracted from *function_argument*.\n"
				"\n"
				"  If any filenames are specified (in *function_argument*) then this method uses "
				":class:`FeatureCollection` internally to read those files. "
				"Those files contain the subset of features returned by :meth:`get_files`.\n"
				"\n"
				"  To turn an argument of your function into a list of features:\n"
				"  ::\n"
				"\n"
				"    def my_function(features):\n"
				"        # Turn function argument into something more convenient for extracting features.\n"
				"        features = pygplates.FeaturesFunctionArgument(features)\n"
				"        \n"
				"        # Iterate over features from the function argument.\n"
				"        for feature in features.get_features()\n"
				"            ...\n"
				"    \n"
				"    my_function(['file1.gpml', 'file2.gpml'])\n")
		.def("contains_features",
				&GPlatesApi::FeaturesFunctionArgument::contains_features,
				(bp::arg("function_argument")),
				"contains_features(function_argument)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Return whether *function_argument* contains features.\n"
				"\n"
				"  :param function_argument: the function argument to test for features\n"
				"\n"
				"  This method returns ``True`` if *function_argument* is a "
				":class:`feature collection<FeatureCollection>`, or filename, or :class:`feature<Feature>`, "
				"or sequence of :class:`features<Feature>`, or a sequence (eg, ``list`` or ``tuple``) "
				"of any combination of those four types.\n"
				"\n"
				"  To define a function that raises ``TypeError`` if its function argument does not contain features:\n"
				"  ::\n"
				"\n"
				"    def my_function(features):\n"
				"        if not pygplates.FeaturesFunctionArgument.contains_features(features):\n"
				"            raise TypeError(\"Unexpected type for argument 'features' in function 'my_function'.\")\n"
				"        \n"
				"        # Turn function argument into something more convenient for extracting features.\n"
				"        features = pygplates.FeaturesFunctionArgument(features)\n"
				"        ...\n"
				"\n"
				"  Note that it is not necessary to call :meth:`contains_features` before constructing "
				"a :class:`FeaturesFunctionArgument` because the :meth:`constructor<__init__>` will "
				"raise an error if the function argument does not contain features. However raising "
				"your own error (as in the example above) helps to clarify the source of the error "
				"for the user (caller) of your function.\n")
		.staticmethod("contains_features")
		.def("get_features",
				&GPlatesApi::FeaturesFunctionArgument::get_features,
				"get_features()\n"
				"  Returns a list of all features specified in the :meth:`constructor<__init__>`.\n"
				"\n"
				"  :rtype: list of :class:`Feature`\n"
				"\n"
				"  Note that any features coming from files are loaded only once in the "
				":meth:`constructor<__init__>`. They are not loaded each time this method is called.\n"
				"\n"
				"  Define a function that extract features and processes them:\n"
				"  ::\n"
				"\n"
				"    def my_function(features):\n"
				"        # Turn function argument into something more convenient for extracting features.\n"
				"        features = pygplates.FeaturesFunctionArgument(features)\n"
				"        \n"
				"        # Iterate over features from the function argument.\n"
				"        for feature in features.get_features():\n"
				"            ...\n"
				"    \n"
				"    # Process features in 'file.gpml', 'feature_collection' and 'feature'.\n"
				"    my_function(['file.gpml', feature_collection, feature])\n")
		.def("get_files",
				&GPlatesApi::FeaturesFunctionArgument::get_files,
				"get_files()\n"
				"  Returns a list of feature collections that were loaded from files specified in "
				"the :meth:`constructor<__init__>`.\n"
				"\n"
				"  :returns: a list of (feature collection, filename) tuples\n"
				"  :rtype: list of (:class:`FeatureCollection`, string) tuples\n"
				"\n"
				"  Only those feature collections associated with filenames (specified in the function "
				"argument in :meth:`constructor<__init__>`) are returned. :class:`Features<Feature>` "
				"and :class:`feature collections<FeatureCollection>` that were directly specified "
				"(in the function argument in :meth:`constructor<__init__>`) are not returned here.\n"
				"\n"
				"  Note that the returned features (coming from files) are loaded only once in the "
				":meth:`constructor<__init__>`. They are not loaded each time this method is called.\n"
				"\n"
				"  Define a function that extract features, modifies them and writes those features "
				"that came from files back out to those same files:\n"
				"  ::\n"
				"\n"
				"    def my_function(features):\n"
				"        # Turn function argument into something more convenient for extracting features.\n"
				"        features = pygplates.FeaturesFunctionArgument(features)\n"
				"        \n"
				"        # Modify features in some way.\n"
				"        for feature in features.get_features():\n"
				"            ...\n"
				"        \n"
				"        # Write those (modified) feature collections that came from files (if any) back to file.\n"
				"        files = features.get_files()\n"
				"        if files:\n"
				"            for feature_collection, filename in files:\n"
				"                # This can raise pygplates.OpenFileForWritingError if file is not writable.\n"
				"                feature_collection.write(filename)\n"
				"    \n"
				"    # Modify features in 'file.gpml' and 'feature_collection'.\n"
				"    # Modified features from 'file.gpml' will get written back out to 'file.gpml'.\n"
				"    my_function(['file.gpml', feature_collection])\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;
}
