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

#include "FeatureCollectionReaderWriter.h"

#include "ErrorOpeningFileForReadingException.h"
#include "ErrorOpeningFileForWritingException.h"
#include "FileFormatNotSupportedException.h"
#include "GmapReader.h"
#include "GMTFormatWriter.h"
#include "GpmlOnePointSixOutputVisitor.h"
#include "GpmlOnePointSixReader.h"
#include "PlatesLineFormatReader.h"
#include "PlatesLineFormatWriter.h"
#include "PlatesRotationFormatReader.h"
#include "PlatesRotationFormatWriter.h"
#include "ShapefileReader.h"
#include "ShapefileWriter.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"


namespace
{
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
	 * Create a derived @a GPlatesModel::ConstFeatureVisitor determined by file extension.
	 */
	boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>
	get_feature_collection_writer_from_file_extension(
			const GPlatesFileIO::FileInfo& file_info,
			const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
	{
		// Determine file format from file extension.
		const GPlatesFileIO::FeatureCollectionFileFormat::Format file_format =
			get_feature_collection_file_format(file_info);

		switch (file_format)
		{
		case GPlatesFileIO::FeatureCollectionFileFormat::GPML:
			return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
				new GPlatesFileIO::GpmlOnePointSixOutputVisitor(file_info, false));

		case GPlatesFileIO::FeatureCollectionFileFormat::GPML_GZ:
			return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
				new GPlatesFileIO::GpmlOnePointSixOutputVisitor(file_info, true));

		case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_LINE:
			return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
				new GPlatesFileIO::PlatesLineFormatWriter(file_info));

		case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_ROTATION:
			return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
				new GPlatesFileIO::PlatesRotationFormatWriter(file_info));

		case GPlatesFileIO::FeatureCollectionFileFormat::GMT:
			return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
				new GPlatesFileIO::GMTFormatWriter(file_info));

		case GPlatesFileIO::FeatureCollectionFileFormat::SHAPEFILE:
			return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
				new GPlatesFileIO::ShapefileWriter(file_info, feature_collection));

		case GPlatesFileIO::FeatureCollectionFileFormat::UNKNOWN:
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
	identify_by_magic_number(
			const GPlatesFileIO::FileInfo& file_info)
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
			throw GPlatesFileIO::ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
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


boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>
GPlatesFileIO::get_feature_collection_writer(
		const FileInfo& file_info,
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
		FeatureCollectionWriteFormat::Format write_format)
{
	// The following check is commented out because	it fails in certain circumstances
	// on newer versions of Windows. We'll just try and open the file for writing
	// and throw an exception if it fails.
#if 0
	if ( ! is_writable(file_info) )
	{
		throw ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
				file_info.get_qfileinfo().filePath());
	}
#endif

	// Make sure write format is compatible with file format.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			is_write_format_compatible_with_file_format(
					write_format,
					get_feature_collection_file_format(file_info)),
			GPLATES_ASSERTION_SOURCE);

	// Create and return feature writer.
	switch (write_format)
	{
	case FeatureCollectionWriteFormat::USE_FILE_EXTENSION:
		return get_feature_collection_writer_from_file_extension(file_info, feature_collection);

	case FeatureCollectionWriteFormat::GMT_WITH_PLATES4_STYLE_HEADER:
		return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
			new GMTFormatWriter(file_info, GMTFormatWriter::PLATES4_STYLE_HEADER));

	case FeatureCollectionWriteFormat::GMT_VERBOSE_HEADER:
		return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
			new GMTFormatWriter(file_info, GMTFormatWriter::VERBOSE_HEADER));

	case FeatureCollectionWriteFormat::GMT_PREFER_PLATES4_STYLE_HEADER:
		return boost::shared_ptr<GPlatesModel::ConstFeatureVisitor>(
			new GMTFormatWriter(file_info, GMTFormatWriter::PREFER_PLATES4_STYLE_HEADER));

	default:
		throw FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
				"Chosen write format is not currently supported.");
	}
}

const GPlatesFileIO::File::shared_ref
GPlatesFileIO::read_feature_collection(
		const FileInfo &file_info,
		GPlatesModel::ModelInterface &model,
		ReadErrorAccumulation &read_errors)
{
	try
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
				return GpmlOnePointSixReader::read_file(file_info, model, read_errors, false);

			case GZIP:
				return GpmlOnePointSixReader::read_file(file_info, model, read_errors, true);

			default:
				throw FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
					"File extension is '.gpml' or '.gpml.gz' but file format is neither.");
			}
			break;

		case FeatureCollectionFileFormat::PLATES4_LINE:
			return PlatesLineFormatReader::read_file(file_info, model, read_errors);

		case FeatureCollectionFileFormat::PLATES4_ROTATION:
			return PlatesRotationFormatReader::read_file(file_info, model, read_errors);

		case FeatureCollectionFileFormat::SHAPEFILE:
			return ShapefileReader::read_file(file_info, model, read_errors);

		case FeatureCollectionFileFormat::GMAP:
			return GmapReader::read_file(file_info,model,read_errors);

		case FeatureCollectionFileFormat::GMT:
		
		case FeatureCollectionFileFormat::UNKNOWN:
		default:
			throw FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
					"Chosen file format is not currently supported.");
		}
	}
	catch (GPlatesFileIO::ErrorOpeningFileForReadingException &e)
	{
		// FIXME: A bit of a sucky conversion from ErrorOpeningFileForReadingException to
		// ReadErrorOccurrence, but hey, this whole function will be rewritten when we add
		// QFileDialog support.
		// FIXME: I suspect I'm Missing The Point with these shared_ptrs.
		boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
				new GPlatesFileIO::LocalFileDataSource(
						e.filename(), GPlatesFileIO::DataFormats::Unspecified));
		boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
				new GPlatesFileIO::LineNumber(0));
		read_errors.d_failures_to_begin.push_back(GPlatesFileIO::ReadErrorOccurrence(
				e_source,
				e_location,
				GPlatesFileIO::ReadErrors::ErrorOpeningFileForReading,
				GPlatesFileIO::ReadErrors::FileNotLoaded));

		// Rethrow the exception to let caller know that an error occurred.
		// This is important because the caller is expecting a valid feature collection
		// unless an exception is thrown so if we don't throw one then the caller
		// will try to dereference the feature collection and crash.
		throw;
	}
}
