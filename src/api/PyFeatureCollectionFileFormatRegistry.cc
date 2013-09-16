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

#include "global/python.h"

#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/File.h"
#include "file-io/FileInfo.h"
#include "file-io/ReadErrorAccumulation.h"

#include "model/Gpgim.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	boost::shared_ptr<GPlatesFileIO::FeatureCollectionFileFormat::Registry>
	feature_collection_file_format_registry_create()
	{
		return boost::shared_ptr<GPlatesFileIO::FeatureCollectionFileFormat::Registry>(
				new GPlatesFileIO::FeatureCollectionFileFormat::Registry(
						// Create a GPGIM from its XML file...
						GPlatesModel::Gpgim::create()));
	}

	/**
	 * Read a feature collection from the specified file.
	 */
	GPlatesModel::FeatureCollectionHandle::non_null_ptr_type
	feature_collection_file_format_registry_read(
			boost::shared_ptr<GPlatesFileIO::FeatureCollectionFileFormat::Registry> registry,
			const QString &filename)
	{
		const GPlatesFileIO::FileInfo file_info(filename);

		// Create a file with an empty feature collection.
		GPlatesFileIO::File::non_null_ptr_type file = GPlatesFileIO::File::create_file(file_info);

		// TODO: Return the read errors to the python caller.
		GPlatesFileIO::ReadErrorAccumulation read_errors;
		// Read new features from the file into the feature collection.
		// Both the filename and target feature collection are in 'file_ref'.
		registry->read_feature_collection(file->get_reference(), read_errors);

		return GPlatesModel::FeatureCollectionHandle::non_null_ptr_type(
				file->get_reference().get_feature_collection().handle_ptr());
	}
}


void
export_feature_collection_file_format_registry()
{
	//
	// FeatureCollectionFileFormatRegistry
	//
	bp::class_<
			GPlatesFileIO::FeatureCollectionFileFormat::Registry,
			boost::shared_ptr<GPlatesFileIO::FeatureCollectionFileFormat::Registry>,
			boost::noncopyable>(
					"FeatureCollectionFileFormatRegistry",
					// Seems we need this (even though later define "__init__") since
					// FeatureCollectionFileFormat::Registry has no default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						GPlatesApi::feature_collection_file_format_registry_create))
		.def("read", &GPlatesApi::feature_collection_file_format_registry_read)
	;
}

#endif // GPLATES_NO_PYTHON
