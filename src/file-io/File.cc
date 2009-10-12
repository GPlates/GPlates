/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "File.h"

#include "FileInfo.h"


GPlatesFileIO::File::shared_ref
GPlatesFileIO::File::create_empty_file(
		const GPlatesModel::FeatureCollectionHandleUnloader::shared_ref &feature_collection)
{
	// Feature collection was not created from a file so use default FileInfo and
	// unknown file format.
	const FileInfo file_info;
	const FeatureCollectionFileFormat::Format file_format = FeatureCollectionFileFormat::UNKNOWN;

	return shared_ref(new File(feature_collection, file_info, file_format));
}


GPlatesFileIO::File::shared_ref
GPlatesFileIO::File::create_loaded_file(
		const GPlatesModel::FeatureCollectionHandleUnloader::shared_ref &feature_collection,
		const FileInfo &file_info)
{
	// Determine the file format from the filename extension.
	const FeatureCollectionFileFormat::Format file_format =
			get_feature_collection_file_format(file_info);

	return shared_ref(new File(feature_collection, file_info, file_format));
}


GPlatesFileIO::File::shared_ref
GPlatesFileIO::File::create_save_file(
		const File &file,
		const FileInfo &file_info)
{
	// NOTE: We use the file format of the original file and *not* the file format
	// obtained by looking at the new filename extension.
	const FeatureCollectionFileFormat::Format file_format = file.get_loaded_file_format();

	// Share the feature collection from the existing file.
	return shared_ref(new File(file.d_feature_collection, file_info, file_format));
}


GPlatesFileIO::File::File(
		const GPlatesModel::FeatureCollectionHandleUnloader::shared_ref &feature_collection,
		const FileInfo &file_info,
		const FeatureCollectionFileFormat::Format file_format) :
	d_feature_collection(feature_collection),
	d_file_info(file_info),
	d_loaded_file_format(file_format)
{
}
