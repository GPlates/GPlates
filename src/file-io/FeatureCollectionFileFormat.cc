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

#include "FeatureCollectionFileFormat.h"
#include "FeatureWriter.h"
#include "FileInfo.h"

#include "GpmlOnePointSixOutputVisitor.h"
#include "PlatesLineFormatWriter.h"
#include "PlatesRotationFormatWriter.h"
#include "GMTFormatWriter.h"
#include "ShapefileWriter.h"

#include "GpmlOnePointSixReader.h"
#include "PlatesLineFormatReader.h"
#include "PlatesRotationFormatReader.h"
#include "ShapefileReader.h"

#include "FileFormatNotSupportedException.h"
#include "ErrorOpeningFileForWritingException.h"
#include "ErrorOpeningFileForReadingException.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"
#include "qt-widgets/ShapefilePropertyMapper.h"


namespace
{
	bool
		file_name_ends_with(const QFileInfo &file, const QString &suffix)
	{
		return file.completeSuffix().endsWith(
			QString(suffix),
			Qt::CaseInsensitive);
	}

	bool
		is_gmt_format_file(const QFileInfo &file)
	{
		return file_name_ends_with(file, "xy");
	}


	bool
		is_plates_line_format_file(const QFileInfo &file)
	{
		return file_name_ends_with(file, "dat") ||
			file_name_ends_with(file, "pla");
	}


	bool
		is_plates_rotation_format_file(const QFileInfo &file)
	{
		return file_name_ends_with(file, "rot");
	}

	bool
		is_shapefile_format_file(const QFileInfo &file)
	{
		return file_name_ends_with(file, "shp");

	// RJW: Testing ESRI Geodatabases.
	//	return file_name_ends_with(file, "mdb");
	}

	bool
		is_gpml_format_file(const QFileInfo &file)
	{
		return file_name_ends_with(file, "gpml");
	}

	bool
		is_gpml_gz_format_file(const QFileInfo &file)
	{
		return file_name_ends_with(file, "gpml.gz");
	}

	/**
	 * Make sure @a write_format is compatible with @a file_format.
	 *
	 * @param write_format format specified by user.
	 * @param file_format format determined by file extension.
	 */
	bool
		is_write_format_compatible_with_file_format(
		GPlatesFileIO::FeatureCollectionWriteFormat::Format write_format,
		GPlatesFileIO::FeatureCollectionFileFormat::Format file_format)
	{
		switch (write_format)
		{
		case GPlatesFileIO::FeatureCollectionWriteFormat::USE_FILE_EXTENSION:
			// Using file extension to determine write format so always ok.
			return true;

		case GPlatesFileIO::FeatureCollectionWriteFormat::GMT_WITH_PLATES4_STYLE_HEADER:
		case GPlatesFileIO::FeatureCollectionWriteFormat::GMT_VERBOSE_HEADER:
		case GPlatesFileIO::FeatureCollectionWriteFormat::GMT_PREFER_PLATES4_STYLE_HEADER:
			// Writing using GMT so file extension must match.
			return  file_format == GPlatesFileIO::FeatureCollectionFileFormat::GMT;
		}

		// Forgot to add a case statement for new enumeration ?
		return false;
	}

	/**
	 * Create a derived @a FeatureWriter determined by file extension.
	 */
	boost::shared_ptr<GPlatesFileIO::FeatureWriter>
		get_feature_collection_writer_from_file_extension(
		const GPlatesFileIO::FileInfo& file_info)
	{
		// Determine file format from file extension.
		const GPlatesFileIO::FeatureCollectionFileFormat::Format file_format =
			get_feature_collection_file_format(file_info);

		switch (file_format)
		{
		case GPlatesFileIO::FeatureCollectionFileFormat::GPML:
			return boost::shared_ptr<GPlatesFileIO::FeatureWriter>(
				new GPlatesFileIO::GpmlOnePointSixOutputVisitor(file_info, false));

		case GPlatesFileIO::FeatureCollectionFileFormat::GPML_GZ:
			return boost::shared_ptr<GPlatesFileIO::FeatureWriter>(
				new GPlatesFileIO::GpmlOnePointSixOutputVisitor(file_info, true));

		case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_LINE:
			return boost::shared_ptr<GPlatesFileIO::FeatureWriter>(
				new GPlatesFileIO::PlatesLineFormatWriter(file_info));

		case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_ROTATION:
			return boost::shared_ptr<GPlatesFileIO::FeatureWriter>(
				new GPlatesFileIO::PlatesRotationFormatWriter(file_info));

		case GPlatesFileIO::FeatureCollectionFileFormat::GMT:
			return boost::shared_ptr<GPlatesFileIO::FeatureWriter>(
				new GPlatesFileIO::GMTFormatWriter(file_info));

		case GPlatesFileIO::FeatureCollectionFileFormat::SHAPEFILE:
			return boost::shared_ptr<GPlatesFileIO::FeatureWriter>(
				new GPlatesFileIO::ShapefileWriter(file_info));
		default:
			throw GPlatesFileIO::FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
				"Chosen file format is not currently supported.");
		}
	}

	enum FileMagic
	{
		UNKNOWN, XML, GZIP
	};

	FileMagic
		identify_by_magic_number(const GPlatesFileIO::FileInfo& file_info)
	{
		static const QByteArray MAGIC_UTF8 = QByteArray::fromHex("efbbbf");
		static const QByteArray MAGIC_UTF16_BIG_ENDIAN = QByteArray::fromHex("feff");
		static const QByteArray MAGIC_UTF16_LITTLE_ENDIAN = QByteArray::fromHex("fffe");
		static const QByteArray MAGIC_GZIP = QByteArray::fromHex("1f8b");
		static const QByteArray MAGIC_XML = QByteArray("<?xml");

		QString filename(file_info.get_qfileinfo().filePath());
		QFile f(filename);

		if ( ! f.open(QIODevice::ReadOnly)) 
		{
			throw GPlatesFileIO::ErrorOpeningFileForReadingException(filename);
		}

		FileMagic magic = UNKNOWN;

		// Skip over any Unicode byte-order-marks.
		if (f.peek(MAGIC_UTF8.size()) == MAGIC_UTF8) {
			f.read(MAGIC_UTF8.size());
		} else if (f.peek(MAGIC_UTF16_BIG_ENDIAN.size()) == MAGIC_UTF16_BIG_ENDIAN) {
			f.read(MAGIC_UTF16_BIG_ENDIAN.size());
		} else if (f.peek(MAGIC_UTF16_LITTLE_ENDIAN.size()) == MAGIC_UTF16_LITTLE_ENDIAN) {
			f.read(MAGIC_UTF16_LITTLE_ENDIAN.size());
		}

		// Look for magic.
		if (f.peek(MAGIC_GZIP.size()) == MAGIC_GZIP) {
			magic = GZIP;
		} else if (f.peek(MAGIC_XML.size()) == MAGIC_XML) {
			magic = XML;
		}

		f.close();
		return magic;
	}
}


GPlatesFileIO::FeatureCollectionFileFormat::Format
GPlatesFileIO::get_feature_collection_file_format(
	const FileInfo& file_info)
{
	return get_feature_collection_file_format( file_info.get_qfileinfo() );
}

GPlatesFileIO::FeatureCollectionFileFormat::Format
GPlatesFileIO::get_feature_collection_file_format(
	const QFileInfo& file_info)
{
	if (is_gpml_format_file(file_info))
	{
		return FeatureCollectionFileFormat::GPML;
	}
	else if (is_gpml_gz_format_file(file_info))
	{
		// FIXME: Feed the OutputVisitor a better way of Gzipping things. We might want a Writer base class,
		// which takes a QIODevice, so we could wrap that up in a special GzipWriter. But, that'd mean re-writing
		// the PlatesLineFormat stuff to use a QIODevice.
		return FeatureCollectionFileFormat::GPML_GZ;
	}
	else if (is_plates_line_format_file(file_info))
	{
		return FeatureCollectionFileFormat::PLATES4_LINE;
	}
	else if (is_plates_rotation_format_file(file_info))
	{
		return FeatureCollectionFileFormat::PLATES4_ROTATION;
	}
	else if (is_shapefile_format_file(file_info))
	{
		return FeatureCollectionFileFormat::SHAPEFILE;
	}
	else if (is_gmt_format_file(file_info))
	{
		return FeatureCollectionFileFormat::GMT;
	}
	else
	{
		return FeatureCollectionFileFormat::UNKNOWN;
	}
}

boost::shared_ptr<GPlatesFileIO::FeatureWriter>
GPlatesFileIO::get_feature_collection_writer(
	const FileInfo& file_info,
	FeatureCollectionWriteFormat::Format write_format)
{
	if ( ! is_writable(file_info) )
	{
		throw ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
				file_info.get_qfileinfo().filePath());
	}

	// Assert GMT format compatilibity.
	switch (write_format)
	{
	case FeatureCollectionWriteFormat::GMT_WITH_PLATES4_STYLE_HEADER:
	case FeatureCollectionWriteFormat::GMT_VERBOSE_HEADER:
	case FeatureCollectionWriteFormat::GMT_PREFER_PLATES4_STYLE_HEADER:
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				get_feature_collection_file_format(file_info) == FeatureCollectionFileFormat::GMT,
				GPLATES_ASSERTION_SOURCE);
		break;
	default:
		break;
	}

	// Create and return feature writer.
	switch (write_format)
	{
	case FeatureCollectionWriteFormat::USE_FILE_EXTENSION:
		return get_feature_collection_writer_from_file_extension(file_info);

	case FeatureCollectionWriteFormat::GMT_WITH_PLATES4_STYLE_HEADER:
		return boost::shared_ptr<FeatureWriter>(
			new GMTFormatWriter(file_info, GMTFormatWriter::PLATES4_STYLE_HEADER));

	case FeatureCollectionWriteFormat::GMT_VERBOSE_HEADER:
		return boost::shared_ptr<FeatureWriter>(
			new GMTFormatWriter(file_info, GMTFormatWriter::VERBOSE_HEADER));

	case FeatureCollectionWriteFormat::GMT_PREFER_PLATES4_STYLE_HEADER:
		return boost::shared_ptr<FeatureWriter>(
			new GMTFormatWriter(file_info, GMTFormatWriter::PREFER_PLATES4_STYLE_HEADER));

	default:
		throw FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
				"Chosen write format is not currently supported.");
	}
}

void
GPlatesFileIO::read_feature_collection_file(
	FileInfo &file_info,
	GPlatesModel::ModelInterface &model,
	ReadErrorAccumulation &read_errors)
{
	switch ( get_feature_collection_file_format(file_info) )
	{
	case FeatureCollectionFileFormat::GPML:
	case FeatureCollectionFileFormat::GPML_GZ:
		// Both '.gpml' and '.gpml.gz' use the same reader.
		// Instead of trusting the file extension 'GPML' or 'GPML_GZ'
		// we'll check the first few bytes of the file to make sure it's
		// a gzip compressed file or xml file.
		switch ( identify_by_magic_number(file_info) )
		{
		case XML:
		case UNKNOWN:
			// Not gzipped, probably XML (although if the magic number doesn't match
			// XML, we should attempt to read it as XML anyway because to reach this
			// point, the user has specified a filename matching a GPML extension)
			GpmlOnePointSixReader::read_file(file_info, model, read_errors, false);
			break;

		case GZIP:
			GpmlOnePointSixReader::read_file(file_info, model, read_errors, true);
			break;

		default:
			throw FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
				"File extension is '.gpml' or '.gpml.gz' but file format is neither.");
		}
		break;

	case FeatureCollectionFileFormat::PLATES4_LINE:
		PlatesLineFormatReader::read_file(file_info, model, read_errors);
		break;

	case FeatureCollectionFileFormat::PLATES4_ROTATION:
		PlatesRotationFormatReader::read_file(file_info, model, read_errors);
		break;

	case FeatureCollectionFileFormat::SHAPEFILE:
		ShapefileReader::read_file(file_info, model, read_errors);
		break;

	case FeatureCollectionFileFormat::GMT:
	case FeatureCollectionFileFormat::UNKNOWN:
	default:
		throw FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
				"Chosen file format is not currently supported.");
	}
}
