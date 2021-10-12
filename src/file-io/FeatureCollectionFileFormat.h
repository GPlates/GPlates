/* $Id$ */

/**
 * \file 
 * File formats for feature collection file readers and writers and
 * functions relating to these file formats.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_FEATURECOLLECTIONFILEFORMAT_H
#define GPLATES_FILEIO_FEATURECOLLECTIONFILEFORMAT_H

#include <boost/shared_ptr.hpp>
#include <QFileInfo>


namespace GPlatesModel
{
	class ModelInterface;
}

namespace GPlatesFileIO
{
	class FileInfo;
	class FeatureWriter;
	class FeatureCollectionReader;
	struct ReadErrorAccumulation;
}

namespace GPlatesFileIO
{
	namespace FeatureCollectionFileFormat
	{
		//! Formats of files that can contain feature collections.
		enum Format
		{
			UNKNOWN,           //!< Format, or file extension, is unknown.

			GPML,              //!< '.gpml' extension.
			GPML_GZ,           //!< '.gpml.gz' extension.
			PLATES4_LINE,      //!< '.dat' or '.pla' extension.
			PLATES4_ROTATION,  //!< '.rot' extension.
			SHAPEFILE,         //!< '.shp' extension.
			GMT                //!< '.xy' extension.
		};
	}

	namespace FeatureCollectionWriteFormat
	{
		/**
		 * Formats to write feature collections.
		 * Doesn't necessarily uniquely determine the format of the file.
		 * For example, 'GMT' and 'GMT_VERBOSE_HEADER' are both that same format
		 * when reading - but here they are used to determine what type of information
		 * is written to those parts of the format that are flexible (such as the header).
		 */
		enum Format
		{
			USE_FILE_EXTENSION,             //!< Determine format to write using file's extension.
			GMT_WITH_PLATES4_STYLE_HEADER,  //!< GMT format with PLATES4 style header (otherwise) short unhelpful header.
			GMT_VERBOSE_HEADER,             //!< GMT format with header containing string values of all feature properties.
			GMT_PREFER_PLATES4_STYLE_HEADER //!< GMT format with PLATES4 style header preferred over verbose header.
		};
	}

	//@{
	/**
	* Determine feature collection file type based on file extension.
	*
	* @param file_info file whose extension used to determine file format.
	*/
	FeatureCollectionFileFormat::Format
		get_feature_collection_file_format(
		const FileInfo& file_info);

	FeatureCollectionFileFormat::Format
		get_feature_collection_file_format(
		const QFileInfo& file_info);
	//@}

	/**
	* Creates and returns a feature collection writer.
	*
	* If @a write_format is not @a USE_FILE_EXTENSION then it must
	* be compatible with the format obtained from the file extension of @a file_info.
	* For example, @a GMT and @a GMT_VERBOSE_HEADER should only be specified for
	* a file that has the GMT '.xy' extension.
	*
	* @param file_info feature collection and output file
	* @param write_format specifies which format to write.
	*
	* @throws ErrorOpeningFileForWritingException if file is not writable.
	* @throws FileFormatNotSupportedException if file format has no writer.
	*/
	boost::shared_ptr<FeatureWriter>
		get_feature_collection_writer(
		const FileInfo& file_info,
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
	void
		read_feature_collection_file(
		FileInfo &fileinfo,
		GPlatesModel::ModelInterface &model,
		ReadErrorAccumulation &read_errors);
}

#endif // GPLATES_FILEIO_FEATURECOLLECTIONFILEFORMAT_H
