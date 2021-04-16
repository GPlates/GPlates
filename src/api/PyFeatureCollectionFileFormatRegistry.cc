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
#include <boost/foreach.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <QString>

#include "PyFeatureCollectionFileFormatRegistry.h"

#include "PythonExtractUtils.h"
#include "PythonHashDefVisitor.h"

#include "global/python.h"

#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/File.h"
#include "file-io/FileInfo.h"
#include "file-io/ReadErrorAccumulation.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	GPlatesModel::FeatureCollectionHandle::non_null_ptr_type
	read_feature_collection(
			const GPlatesFileIO::FeatureCollectionFileFormat::Registry &registry,
			const QString &filename)
	{
		const GPlatesFileIO::FileInfo file_info(filename);

		// Create a file with an empty feature collection.
		GPlatesFileIO::File::non_null_ptr_type file = GPlatesFileIO::File::create_file(file_info);

		// TODO: Return the read errors to the python caller.
		GPlatesFileIO::ReadErrorAccumulation read_errors;
		// Read new features from the file into the feature collection.
		registry.read_feature_collection(file->get_reference(), read_errors);

		return GPlatesModel::FeatureCollectionHandle::non_null_ptr_type(
				file->get_reference().get_feature_collection().handle_ptr());
	}

	void
	read_feature_collections(
			std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> &feature_collections,
			const GPlatesFileIO::FeatureCollectionFileFormat::Registry &registry,
			const std::vector<QString> &filenames)
	{
		BOOST_FOREACH(const QString &filename, filenames)
		{
			feature_collections.push_back(
					read_feature_collection(registry, filename));
		}
	}

	bp::object
	read_feature_collections(
			const GPlatesFileIO::FeatureCollectionFileFormat::Registry &registry,
			bp::object filename_object)
	{
		// See if a single filename.
		bp::extract<QString> extract_filename(filename_object);
		if (extract_filename.check())
		{
			const QString filename = extract_filename();

			return bp::object(read_feature_collection(registry, filename));
		}

		// Try a sequence of filenames next.
		std::vector<QString> filenames;
		PythonExtractUtils::extract_iterable(
				filenames,
				filename_object,
				"Expected a filename or sequence of filenames");

		std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> feature_collections;
		read_feature_collections(feature_collections, registry, filenames);

		bp::list feature_collections_list;

		BOOST_FOREACH(
				GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection,
				feature_collections)
		{
			feature_collections_list.append(feature_collection);
		}

		return feature_collections_list;
	}

	void
	write_feature_collection(
			const GPlatesFileIO::FeatureCollectionFileFormat::Registry &registry,
			GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection,
			const QString &filename)
	{
		const GPlatesFileIO::FileInfo file_info(filename);

		// Create an output file to write out the feature collection.
		GPlatesFileIO::File::non_null_ptr_type file =
				GPlatesFileIO::File::create_file(file_info, feature_collection);

		// Write the features from the feature collection to the file.
		registry.write_feature_collection(file->get_reference());
	}

	boost::shared_ptr<GPlatesFileIO::FeatureCollectionFileFormat::Registry>
	feature_collection_file_format_registry_create()
	{
		return boost::shared_ptr<GPlatesFileIO::FeatureCollectionFileFormat::Registry>(
				new GPlatesFileIO::FeatureCollectionFileFormat::Registry());
	}

	bp::object
	feature_collection_file_format_registry_read(
			const GPlatesFileIO::FeatureCollectionFileFormat::Registry &registry,
			bp::object filename_object)
	{
		return read_feature_collections(registry, filename_object);
	}
}


void
export_feature_collection_file_format_registry()
{
	//
	// FeatureCollectionFileFormatRegistry - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	// NOTE: We don't document for now since we've hidden this class because it's a little confusing for the user
	//       (better that they just use pygplates.FeatureCollection). We still enable this class in case some
	//       Python users are still using this class in their scripts.
	//
	bp::class_<
			GPlatesFileIO::FeatureCollectionFileFormat::Registry,
			boost::shared_ptr<GPlatesFileIO::FeatureCollectionFileFormat::Registry>,
			boost::noncopyable>(
					"FeatureCollectionFileFormatRegistry",
					// We use our own "__init__" (defined below)...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::feature_collection_file_format_registry_create)
				// ,
				// "__init__()\n"
				// "  Create a new registry of feature collection readers/writers and registers the "
				// "default file formats supported by GPlates.\n"
				// "  ::\n"
				// "\n"
				// "    feature_collection_file_format_registry = pygplates.FeatureCollectionFileFormatRegistry()\n"
		)
		.def("read",
				&GPlatesApi::feature_collection_file_format_registry_read,
				(bp::arg("filename"))
				// ,
				// "read(filename)\n"
				// "  Reads one or more feature collections (from one or more files).\n"
				// "\n"
				// "  :param filename: the name of the file (or files) to read\n"
				// "  :type filename: string, or sequence of strings\n"
				// "  :rtype: :class:`FeatureCollection`, list of :class:`FeatureCollection`\n"
				// "  :raises: OpenFileForReadingError if any file is not readable\n"
				// "  :raises: FileFormatNotSupportedError if any file format (identified by a filename "
				// "extension) does not support reading\n"
				// "\n"
				// "  For example:\n"
				// "  ::\n"
				// "\n"
				// "    try:\n"
				// "        feature_collection = feature_collection_file_format_registry.read(filename)\n"
				// "    except pygplates.OpenFileForReadingError:\n"
				// "        # Handle inability to read from file.\n"
				// "        ...\n"
				// "    except pygplates.FileFormatNotSupportedError:\n"
				// "        # Handle unsupported file format (for reading).\n"
				// "        ...\n"
				// "\n"
				// "  .. note:: The returned *feature collection* may contain fewer features than are "
				// "stored in the file if there were read errors. *TODO:* return read errors.\n"
				// "\n"
				// "  .. seealso:: :meth:`FeatureCollection.read`\n"
		)
		.def("write",
				&GPlatesApi::write_feature_collection,
				(bp::arg("feature_collection"), bp::arg("filename"))
				// ,
				// "write(feature_collection, filename)\n"
				// "  Writes a feature collection to the file with name *filename*.\n"
				// "\n"
				// "  :param feature_collection: the feature collection to write\n"
				// "  :type feature_collection: :class:`FeatureCollection`\n"
				// "  :param filename: the name of the file to write\n"
				// "  :type filename: string\n"
				// "  :raises: OpenFileForWritingError if the file is not writable\n"
				// "  :raises: FileFormatNotSupportedError if the file format (identified by the filename "
				// "extension) does not support writing\n"
				// "\n"
				// "  For example:\n"
				// "  ::\n"
				// "\n"
				// "    try:\n"
				// "        feature_collection_file_format_registry.write(feature_collection, filename)\n"
				// "    except pygplates.OpenFileForWritingError:\n"
				// "        # Handle inability to write to file.\n"
				// "        ...\n"
				// "    except pygplates.FileFormatNotSupportedError:\n"
				// "        # Handle unsupported file format (for writing).\n"
				// "        ...\n"
				// "\n"
				// "  .. seealso:: :meth:`FeatureCollection.write`\n"
		)
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;
}
