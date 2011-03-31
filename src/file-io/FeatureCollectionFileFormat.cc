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

#include "FileInfo.h"

#include "global/GPlatesAssert.h"


namespace
{
	//
	// Filename extensions for the various file formats.
	//

	const QString FILE_FORMAT_EXT_GPML = "gpml";
	const QString FILE_FORMAT_EXT_GPMLZ = "gpmlz";
	const QString FILE_FORMAT_EXT_GPMLZ_ALTERNATIVE = "gpml.gz";
	const QString FILE_FORMAT_EXT_PLATES4_LINE = "dat";
	const QString FILE_FORMAT_EXT_PLATES4_LINE_ALTERNATIVE = "pla";
	const QString FILE_FORMAT_EXT_PLATES4_ROTATION = "rot";
	const QString FILE_FORMAT_EXT_SHAPEFILE = "shp";
	const QString FILE_FORMAT_EXT_OGRGMT = "gmt";
	const QString FILE_FORMAT_EXT_GMT = "xy";
	const QString FILE_FORMAT_EXT_GMAP = "vgp";
	const QString FILE_FORMAT_EXT_GSML = "gsml";



	bool
	file_name_ends_with(
			const QFileInfo &file, const QString &suffix)
	{
		return file.completeSuffix().endsWith(
			QString(suffix),
			Qt::CaseInsensitive);
	}

	bool
	is_gmt_format_file(
			const QFileInfo &file)
	{
		return file_name_ends_with(file, FILE_FORMAT_EXT_GMT);
	}


	bool
	is_plates_line_format_file(
			const QFileInfo &file)
	{
		return file_name_ends_with(file, FILE_FORMAT_EXT_PLATES4_LINE) ||
			file_name_ends_with(file, FILE_FORMAT_EXT_PLATES4_LINE_ALTERNATIVE);
	}


	bool
	is_plates_rotation_format_file(
			const QFileInfo &file)
	{
		return file_name_ends_with(file, FILE_FORMAT_EXT_PLATES4_ROTATION);
	}

	bool
	is_shapefile_format_file(
			const QFileInfo &file)
	{
		return file_name_ends_with(file, FILE_FORMAT_EXT_SHAPEFILE);
	}

	bool
	is_ogrgmt_format_file(
			const QFileInfo &file)
	{
		return file_name_ends_with(file, FILE_FORMAT_EXT_OGRGMT);
	}

	bool
	is_gpml_format_file(
			const QFileInfo &file)
	{
		return file_name_ends_with(file, FILE_FORMAT_EXT_GPML);
	}

	bool
	is_gpmlz_format_file(const QFileInfo &file)
	{
		return file_name_ends_with(file, FILE_FORMAT_EXT_GPMLZ) ||
			file_name_ends_with(file, FILE_FORMAT_EXT_GPMLZ_ALTERNATIVE);
	}
	
	bool
	is_gmap_format_file(const QFileInfo &file)
	{
		return file_name_ends_with(file, FILE_FORMAT_EXT_GMAP);
	}

	bool
	is_gsml_format_file(
			const QFileInfo &file)
	{
		return file_name_ends_with(file, FILE_FORMAT_EXT_GSML);
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
	else if (is_gpmlz_format_file(file_info))
	{
		// FIXME: Feed the OutputVisitor a better way of Gzipping things. We might want a Writer base class,
		// which takes a QIODevice, so we could wrap that up in a special GzipWriter. But, that'd mean re-writing
		// the PlatesLineFormat stuff to use a QIODevice.
		return FeatureCollectionFileFormat::GPMLZ;
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
	else if (is_ogrgmt_format_file(file_info))
	{
		return FeatureCollectionFileFormat::OGRGMT;
	}
	else if (is_gmt_format_file(file_info))
	{
		return FeatureCollectionFileFormat::GMT;
	}
	else if (is_gmap_format_file(file_info))
	{
		return FeatureCollectionFileFormat::GMAP;
	}
	else if (is_gsml_format_file(file_info))
	{
		return FeatureCollectionFileFormat::GSML;
	}
	else
	{
		return FeatureCollectionFileFormat::UNKNOWN;
	}
}


QString
GPlatesFileIO::get_filename_extension(
		FeatureCollectionFileFormat::Format format)
{
	switch (format)
	{
	case FeatureCollectionFileFormat::GPML:
		return FILE_FORMAT_EXT_GPML;

	case FeatureCollectionFileFormat::GPMLZ:
		return FILE_FORMAT_EXT_GPMLZ;

	case FeatureCollectionFileFormat::PLATES4_LINE:
		return FILE_FORMAT_EXT_PLATES4_LINE;

	case FeatureCollectionFileFormat::PLATES4_ROTATION:
		return FILE_FORMAT_EXT_PLATES4_ROTATION;

	case FeatureCollectionFileFormat::SHAPEFILE:
		return FILE_FORMAT_EXT_SHAPEFILE;

	case FeatureCollectionFileFormat::OGRGMT:
		return FILE_FORMAT_EXT_OGRGMT;

	case FeatureCollectionFileFormat::GMT:
		return FILE_FORMAT_EXT_GMT;

	case FeatureCollectionFileFormat::GMAP:
		return FILE_FORMAT_EXT_GMAP;

	case FeatureCollectionFileFormat::GSML:
		return FILE_FORMAT_EXT_GSML;

	case FeatureCollectionFileFormat::UNKNOWN:
		return QString();

	default:
		break;
	}

	// Shouldn't get here.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

	// Avoid compiler warning.
	return QString();
}
