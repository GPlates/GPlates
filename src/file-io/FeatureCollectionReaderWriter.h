/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_FILE_IO_FEATURECOLLECTIONREADERWRITER_H
#define GPLATES_FILE_IO_FEATURECOLLECTIONREADERWRITER_H

#include <boost/shared_ptr.hpp>

#include "FeatureCollectionFileFormat.h"
#include "File.h"

#include "model/FeatureCollectionHandle.h"


namespace GPlatesModel
{
	class FeatureHandle;
	template<class FeatureHandleType> class FeatureVisitorBase;
	typedef FeatureVisitorBase<const FeatureHandle> ConstFeatureVisitor;
	class ModelInterface;
}

namespace GPlatesFileIO
{
	class FileInfo;
	struct ReadErrorAccumulation;


	/**
	* Creates and returns a feature collection writer.
	*
	* If @a write_format is not @a USE_FILE_EXTENSION then it must
	* be compatible with the format obtained from the file extension of @a file_info.
	* For example, @a GMT and @a GMT_VERBOSE_HEADER should only be specified for
	* a file that has the GMT '.xy' extension.
	*
	* @param file_info feature collection and output file
	* @param feature_collection the feature collection that will be written (this is
	*        currently required by the shapefile writer).
	* @param write_format specifies which format to write.
	*
	* @throws ErrorOpeningFileForWritingException if file is not writable.
	* @throws FileFormatNotSupportedException if file format has no writer.
	*/
	boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>
	get_feature_collection_writer(
			const FileInfo& file_info,
			// FIXME: remove feature collection parameter (the features to be written are
			// visited by the writer returned by this function).
			const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
			FeatureCollectionWriteFormat::Format write_format =
				FeatureCollectionWriteFormat::USE_FILE_EXTENSION);


	/**
	* Reads a feature collection from a file.
	*
	* @param fileinfo file to read from and store feature collection in.
	* @param model to create feature collection.
	* @param read_errors to contain errors reading file.
	*
	* @throws FileFormatNotSupportedException if file format not recognised or has no reader.
	*/
	const GPlatesFileIO::File::shared_ref
	read_feature_collection(
			const FileInfo &fileinfo,
			GPlatesModel::ModelInterface &model,
			ReadErrorAccumulation &read_errors);
}

#endif // GPLATES_FILE_IO_FEATURECOLLECTIONREADERWRITER_H
