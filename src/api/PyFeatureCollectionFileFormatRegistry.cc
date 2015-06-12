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
#include <boost/shared_ptr.hpp>

#include "PyFeatureCollectionFileFormatRegistry.h"

#include "global/python.h"

#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/File.h"
#include "file-io/FileInfo.h"
#include "file-io/ReadErrorAccumulation.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	boost::shared_ptr<GPlatesFileIO::FeatureCollectionFileFormat::Registry>
	feature_collection_file_format_registry_create()
	{
		return boost::shared_ptr<GPlatesFileIO::FeatureCollectionFileFormat::Registry>(
				new GPlatesFileIO::FeatureCollectionFileFormat::Registry());
	}

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
}


void
export_feature_collection_file_format_registry()
{
	//
	// FeatureCollectionFileFormatRegistry - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesFileIO::FeatureCollectionFileFormat::Registry,
			boost::shared_ptr<GPlatesFileIO::FeatureCollectionFileFormat::Registry>,
			boost::noncopyable>(
					"FeatureCollectionFileFormatRegistry",
					"Reads and writes *feature collections* from/to various *feature collection* file formats.\n"
					"\n"
					"The following default file formats are currently supported by GPlates:\n"
					"\n"
					"=============================== ======================= ============== =================\n"
					"File Format                     Filename Extension      Supports Read  Supports Write\n"
					"=============================== ======================= ============== =================\n"
					"GPlates Markup Language         '.gpml'                 Yes            Yes\n"
					"Compressed GPML                 '.gpmlz' or '.gpml.gz'  Yes            Yes\n"
					"PLATES4 line                    '.dat' or '.pla'        Yes            Yes\n"
					"PLATES4 rotation                '.rot'                  Yes            Yes\n"
					"GPlates rotation                '.grot'                 Yes            Yes\n"
					"ESRI Shapefile                  '.shp'                  Yes            Yes\n"
					"OGR GMT                         '.gmt'                  Yes            Yes\n"
					"GMT xy                          '.xy'                   No             Yes\n"
					"GMAP Virtual Geomagnetic Poles  '.vgp'                  Yes            No\n"
					"=============================== ======================= ============== =================\n"
					"\n"
					"In the future, support will be added to enable users to implement and register "
					"readers/writers for other file formats (or their own non-standard file formats).\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::feature_collection_file_format_registry_create),
				"__init__()\n"
				"  Create a new registry of feature collection readers/writers and registers the "
				"default file formats supported by GPlates.\n"
				"  ::\n"
				"\n"
				"    feature_collection_file_format_registry = "
				"pygplates.FeatureCollectionFileFormatRegistry()\n")
		.def("read",
				&GPlatesApi::read_feature_collection,
				(bp::arg("filename")),
				"read(filename)\n"
				"  Reads a feature collection from the file with name *filename*.\n"
				"\n"
				"  :param filename: the name of the file to read\n"
				"  :type filename: string\n"
				"  :rtype: :class:`FeatureCollection`\n"
				"  :raises: OpenFileForReadingError if the file is not readable\n"
				"  :raises: FileFormatNotSupportedError if the file format (identified by the filename "
				"extension) does not support reading\n"
				"\n"
				"  For example:\n"
				"  ::\n"
				"\n"
				"    try:\n"
				"        feature_collection = feature_collection_file_format_registry.read(filename)\n"
				"    except pygplates.OpenFileForReadingError:\n"
				"        # Handle inability to read from file.\n"
				"        ...\n"
				"    except pygplates.FileFormatNotSupportedError:\n"
				"        # Handle unsupported file format (for reading).\n"
				"        ...\n"
				"\n"
				"  .. note:: The returned *feature collection* may contain fewer features than are "
				"stored in the file if there were read errors. *TODO:* return read errors.\n")
		.def("write",
				&GPlatesApi::write_feature_collection,
				(bp::arg("feature_collection"), bp::arg("filename")),
				"write(feature_collection, filename)\n"
				"  Writes a feature collection to the file with name *filename*.\n"
				"\n"
				"  :param feature_collection: the feature collection to write\n"
				"  :type feature_collection: :class:`FeatureCollection`\n"
				"  :param filename: the name of the file to write\n"
				"  :type filename: string\n"
				"  :raises: OpenFileForWritingError if the file is not writable\n"
				"  :raises: FileFormatNotSupportedError if the file format (identified by the filename "
				"extension) does not support writing\n"
				"\n"
				"  For example:\n"
				"  ::\n"
				"\n"
				"    try:\n"
				"        feature_collection_file_format_registry.write(feature_collection, filename)\n"
				"    except pygplates.OpenFileForWritingError:\n"
				"        # Handle inability to write to file.\n"
				"        ...\n"
				"    except pygplates.FileFormatNotSupportedError:\n"
				"        # Handle unsupported file format (for writing).\n"
				"        ...\n")
	;
}

#endif // GPLATES_NO_PYTHON
