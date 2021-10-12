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


namespace
{
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
		return file_name_ends_with(file, "xy");
	}


	bool
	is_plates_line_format_file(
			const QFileInfo &file)
	{
		return file_name_ends_with(file, "dat") ||
			file_name_ends_with(file, "pla");
	}


	bool
	is_plates_rotation_format_file(
			const QFileInfo &file)
	{
		return file_name_ends_with(file, "rot");
	}

	bool
	is_shapefile_format_file(
			const QFileInfo &file)
	{
		return file_name_ends_with(file, "shp");

	// RJW: Testing ESRI Geodatabases.
	//	return file_name_ends_with(file, "mdb");
	}

	bool
	is_gpml_format_file(
			const QFileInfo &file)
	{
		return file_name_ends_with(file, "gpml");
	}

	bool
	is_gpml_gz_format_file(const QFileInfo &file)
	{
		return file_name_ends_with(file, "gpml.gz");
	}
	
	bool
	is_gmap_format_file(const QFileInfo &file)
	{
		return file_name_ends_with(file,"vgp");
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
	else if (is_gmap_format_file(file_info))
	{
		return FeatureCollectionFileFormat::GMAP;
	}
	else
	{
		return FeatureCollectionFileFormat::UNKNOWN;
	}
}
