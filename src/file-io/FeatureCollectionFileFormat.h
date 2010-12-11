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

#include <QFileInfo>
#include <QString>


namespace GPlatesFileIO
{
	class FileInfo;


	namespace FeatureCollectionFileFormat
	{
		//! Formats of files that can contain feature collections.
		enum Format
		{
			UNKNOWN,           //!< Format, or file extension, is unknown.

			GPML,              //!< '.gpml' extension.
			GPMLZ,			   //!< '.gpmlz' or '.gpml.gz' extension.
			PLATES4_LINE,      //!< '.dat' or '.pla' extension.
			PLATES4_ROTATION,  //!< '.rot' extension.
			SHAPEFILE,         //!< '.shp' extension.
			GMT,               //!< '.xy' extension.
			GMAP               //!< '.vgp' extension.
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

	/**
	 * Determine feature collection file type based on file extension.
	 *
	 * @param file_info file whose extension used to determine file format.
	 */
	FeatureCollectionFileFormat::Format
	get_feature_collection_file_format(
			const FileInfo& file_info);

	/**
	 * Determine feature collection file type based on file extension.
	 *
	 * @param file_info file whose extension used to determine file format.
	 */
	FeatureCollectionFileFormat::Format
	get_feature_collection_file_format(
			const QFileInfo& file_info);

	/**
	 * Returns the filename extension for @a format.
	 *
	 * If @a format is @a UNKNOWN then returns empty string.
	 */
	QString
	get_filename_extension(
			FeatureCollectionFileFormat::Format format);
}

#endif // GPLATES_FILEIO_FEATURECOLLECTIONFILEFORMAT_H
