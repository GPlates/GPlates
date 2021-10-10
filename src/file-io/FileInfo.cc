/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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

#include <QFile>
#include <QByteArray>
#include <fstream>
#include "FileInfo.h"
#include "PlatesLineFormatWriter.h"
#include "PlatesRotationFormatWriter.h"
#include "GpmlOnePointSixOutputVisitor.h"
#include "FileFormatNotSupportedException.h"
#include "ErrorOpeningFileForWritingException.h"
#include "ErrorOpeningFileForReadingException.h"


// FIXME: This was copied verbatim from "qt-widgets/ViewportWindow.cc".  It is thus a prime
// candidate for REFACTORISATION.
namespace
{
	bool
	file_name_ends_with(const GPlatesFileIO::FileInfo &file, const QString &suffix)
	{
		return file.get_qfileinfo().completeSuffix().endsWith(QString(suffix), Qt::CaseInsensitive);
	}


	bool
	is_plates_line_format_file(const GPlatesFileIO::FileInfo &file)
	{
		return file_name_ends_with(file, "dat") || file_name_ends_with(file, "pla");
	}

	
	bool
	is_plates_rotation_format_file(const GPlatesFileIO::FileInfo &file)
	{
		return file_name_ends_with(file, "rot");
	}

	bool
	is_shapefile_format_file(
			const GPlatesFileIO::FileInfo &file)
	{
		return file_name_ends_with(file, "shp");
	}

	bool
	is_gpml_format_file(
			const GPlatesFileIO::FileInfo &file)
	{
		return file_name_ends_with(file, "gpml");
	}

	bool
	is_gpml_gz_format_file(
			const GPlatesFileIO::FileInfo &file)
	{
		return file_name_ends_with(file, "gpml.gz");
	}
}


boost::shared_ptr< GPlatesModel::ConstFeatureVisitor >
GPlatesFileIO::FileInfo::get_writer() const
{
	if ( ! is_writable()) {
		throw ErrorOpeningFileForWritingException(d_file_info.filePath());
	}

	boost::shared_ptr< GPlatesModel::ConstFeatureVisitor > writer;

	if (is_plates_line_format_file(*this)) {
		writer = boost::shared_ptr< GPlatesModel::ConstFeatureVisitor >(new PlatesLineFormatWriter(*this));
	} else if (is_plates_rotation_format_file(*this)) {
		writer = boost::shared_ptr< GPlatesModel::ConstFeatureVisitor >(new PlatesRotationFormatWriter(*this));
	} else if (is_gpml_format_file(*this)) {
		writer = boost::shared_ptr< GPlatesModel::ConstFeatureVisitor >(new GpmlOnePointSixOutputVisitor(*this, false));
	} else if (is_gpml_gz_format_file(*this)) {
		// FIXME: Feed the OutputVisitor a better way of Gzipping things. We might want a Writer base class,
		// which takes a QIODevice, so we could wrap that up in a special GzipWriter. But, that'd mean re-writing
		// the PlatesLineFormat stuff to use a QIODevice.
		writer = boost::shared_ptr< GPlatesModel::ConstFeatureVisitor >(new GpmlOnePointSixOutputVisitor(*this, true));
	} else {
		throw FileFormatNotSupportedException("Chosen file format is not currently supported.");
	}
	return writer;
}



GPlatesFileIO::FileInfo::FileMagic
GPlatesFileIO::FileInfo::identify_by_magic_number() const
{
	static const QByteArray MAGIC_UTF8 = QByteArray::fromHex("efbbbf");
	static const QByteArray MAGIC_UTF16_BIG_ENDIAN = QByteArray::fromHex("feff");
	static const QByteArray MAGIC_UTF16_LITTLE_ENDIAN = QByteArray::fromHex("fffe");
	static const QByteArray MAGIC_GZIP = QByteArray::fromHex("1f8b");
	static const QByteArray MAGIC_XML = QByteArray("<?xml");

	QString filename(d_file_info.filePath());
	QFile f(filename);

	if ( ! f.open(QIODevice::ReadOnly)) 
	{
		throw ErrorOpeningFileForReadingException(filename);
	}
	
	FileMagic magic = FileInfo::UNKNOWN;
	
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
		magic = FileInfo::GZIP;
	} else if (f.peek(MAGIC_XML.size()) == MAGIC_XML) {
		magic = FileInfo::XML;
	}
	
	f.close();
	return magic;
}

