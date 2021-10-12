/* $Id$ */

/**
 * \file Exports reconstructed feature geometries to a file.
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

#include "ReconstructedFeatureGeometryExport.h"

#include "GMTFormatReconstructedFeatureGeometryExport.h"
#include "FeatureCollectionFileFormat.h"
#include "FileFormatNotSupportedException.h"
#include "ShapefileFormatReconstructedFeatureGeometryExport.h"


GPlatesFileIO::ReconstructedFeatureGeometryExport::Format
GPlatesFileIO::ReconstructedFeatureGeometryExport::get_export_file_format(
		const QFileInfo& file_info)
{
	// Since we're using a feature collection file format to export
	// our RFGs we'll use the feature collection file format code.
	const FeatureCollectionFileFormat::Format feature_collection_file_format =
			get_feature_collection_file_format(file_info);

	// Only some feature collection file formats are used for exporting
	// reconstructed feature geometries because most file formats only
	// make sense for unreconstructed geometry (since they provide the
	// information required to do the reconstructions).
	switch (feature_collection_file_format)
	{
	case FeatureCollectionFileFormat::GMT:
		return GMT;
	case FeatureCollectionFileFormat::SHAPEFILE:
		return SHAPEFILE;
	default:
		break;
	}

	return UNKNOWN;
}


void
GPlatesFileIO::ReconstructedFeatureGeometryExport::export_geometries(
		const feature_geometry_group_seq_type &feature_geometry_group_seq,
		ReconstructedFeatureGeometryExport::Format export_format,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time)
{
	switch (export_format)
	{
	case GMT:
		GMTFormatReconstructedFeatureGeometryExport::export_geometries(
				feature_geometry_group_seq,
				file_info,
				referenced_files,
				reconstruction_anchor_plate_id,
				reconstruction_time);
		break;
		
	case SHAPEFILE:
		ShapefileFormatReconstructedFeatureGeometryExport::export_geometries(
			feature_geometry_group_seq,
			file_info,
			referenced_files,
			reconstruction_anchor_plate_id,
			reconstruction_time);		
		break;

	default:
		throw FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
				"Chosen export format is not currently supported.");
	}
}

