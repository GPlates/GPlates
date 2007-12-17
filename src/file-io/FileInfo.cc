/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#include <fstream>
#include "FileInfo.h"
#include "PlatesLineFormatWriter.h"
#include "PlatesRotationFormatWriter.h"
#include "FileFormatNotSupportedException.h"
#include "ErrorOpeningFileForWritingException.h"


// FIXME: This was copied verbatim from "qt-widgets/ViewportWindow.cc".  It is thus a prime
// candidate for REFACTORISATION.
namespace
{
	bool
	file_name_ends_with(const GPlatesFileIO::FileInfo &file, const QString &suffix)
	{
		return file.get_qfileinfo().suffix().endsWith(QString(suffix), Qt::CaseInsensitive);
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
	} else {
		throw FileFormatNotSupportedException("Chosen file format is not currently supported.");
	}
	return writer;
}

