/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include <QDebug>
#include <QDir> // static "separator" function.

#include "ReconstructionGeometryExport.h"

#include "GMTFormatFlowlineExport.h"
#include "GMTFormatMotionPathExport.h"
#include "GMTFormatReconstructedFeatureGeometryExport.h"
#include "ShapefileFormatFlowlineExport.h"
#include "ShapefileFormatMotionPathExport.h"
#include "ShapefileFormatReconstructedFeatureGeometryExport.h"

using namespace GPlatesFileIO::ReconstructionGeometryExportImpl;


QString
GPlatesFileIO::ReconstructionGeometryExport::build_flat_structure_filename(
	const QString &export_path,
	const QString &collection_filename,
	const QString &export_filename)
{
	QString output_filename =  export_path + 
							QDir::separator() + 
							collection_filename + 
							"_" + 
							export_filename;
	return output_filename;

}


QString
GPlatesFileIO::ReconstructionGeometryExport::build_folder_structure_filename(
	const QString &export_path,
	const QString &collection_filename,
	const QString &export_filename)
{
	QString output_folder_name= export_path + 
		QDir::separator() + 
		collection_filename;
	
	QDir folder(export_path);
	QDir sub_folder(output_folder_name);

	if (!sub_folder.exists())
	{
		bool success = folder.mkdir(collection_filename);
		if (!success)
		{
			throw GPlatesFileIO::ErrorOpeningFileForWritingException(
				GPLATES_EXCEPTION_SOURCE,"Unable to create output directory.");
		}
	}

	QString output_filename = output_folder_name + 
							QDir::separator() + 
							export_filename;
	return output_filename;
}


GPlatesFileIO::ReconstructionGeometryExport::Format
GPlatesFileIO::ReconstructionGeometryExport::get_export_file_format(
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


template<>
void
GPlatesFileIO::ReconstructionGeometryExport::export_as_single_file<GPlatesAppLogic::ReconstructedFeatureGeometry>(
		const QString &filename,
		Format export_format,
		const std::list< FeatureGeometryGroup<GPlatesAppLogic::ReconstructedFeatureGeometry> > &grouped_recon_geoms_seq,
		const std::vector<const File::Reference *> &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		const Options<GPlatesAppLogic::ReconstructedFeatureGeometry> &export_options)
{
	switch (export_format)
	{
	case ReconstructionGeometryExport::GMT:
		GMTFormatReconstructedFeatureGeometryExport::export_geometries(
			grouped_recon_geoms_seq,
			filename,
			referenced_files,
			reconstruction_anchor_plate_id,
			reconstruction_time);
		break;

	case ReconstructionGeometryExport::SHAPEFILE:
		ShapefileFormatReconstructedFeatureGeometryExport::export_geometries(
			grouped_recon_geoms_seq,
			filename,
			referenced_files,
			reconstruction_anchor_plate_id,
			reconstruction_time);		
		break;

	default:
		throw FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
			"Chosen export format is not currently supported.");
	}
}


template<>
void
GPlatesFileIO::ReconstructionGeometryExport::export_as_single_file<GPlatesAppLogic::ReconstructedFlowline>(
		const QString &filename,
		Format export_format,
		const std::list< FeatureGeometryGroup<GPlatesAppLogic::ReconstructedFlowline> > &grouped_recon_geoms_seq,
		const std::vector<const File::Reference *> &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		const Options<GPlatesAppLogic::ReconstructedFlowline> &export_options)
{
	switch (export_format)
	{
	case ReconstructionGeometryExport::GMT:
		GMTFormatFlowlinesExport::export_flowlines(
			grouped_recon_geoms_seq,
			filename,
			referenced_files,
			reconstruction_anchor_plate_id,
			reconstruction_time);
		break;

	case ReconstructionGeometryExport::SHAPEFILE:
		ShapefileFormatFlowlineExport::export_flowlines(
			grouped_recon_geoms_seq,
			filename,
			referenced_files,
			reconstruction_anchor_plate_id,
			reconstruction_time);		
		break;

	default:
		throw FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
			"Chosen export format is not currently supported.");
	}
}


template<>
void
GPlatesFileIO::ReconstructionGeometryExport::export_as_single_file<GPlatesAppLogic::ReconstructedMotionPath>(
		const QString &filename,
		Format export_format,
		const std::list< FeatureGeometryGroup<GPlatesAppLogic::ReconstructedMotionPath> > &grouped_recon_geoms_seq,
		const std::vector<const File::Reference *> &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		const Options<GPlatesAppLogic::ReconstructedMotionPath> &export_options)
{
	switch (export_format)
	{
	case ReconstructionGeometryExport::GMT:
		GMTFormatMotionPathsExport::export_motion_paths(
			grouped_recon_geoms_seq,
			filename,
			referenced_files,
			reconstruction_anchor_plate_id,
			reconstruction_time);
		break;

	case ReconstructionGeometryExport::SHAPEFILE:
		ShapefileFormatMotionPathExport::export_motion_paths(
			grouped_recon_geoms_seq,
			filename,
			referenced_files,
			reconstruction_anchor_plate_id,
			reconstruction_time);		
		break;

	default:
		throw FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
			"Chosen export format is not currently supported.");
	}
}


template<>
void
GPlatesFileIO::ReconstructionGeometryExport::export_per_collection<GPlatesAppLogic::ReconstructedFeatureGeometry>(
		Format export_format,
		const std::list< FeatureGeometryGroup<GPlatesAppLogic::ReconstructedFeatureGeometry> > &grouped_recon_geoms_seq,
		const QString &filename,
		const std::vector<const File::Reference *> &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		const Options<GPlatesAppLogic::ReconstructedFeatureGeometry> &export_options)
{
	switch(export_format)
	{
	case ReconstructionGeometryExport::SHAPEFILE:
		ShapefileFormatReconstructedFeatureGeometryExport::export_geometries_per_collection(
			grouped_recon_geoms_seq,
			filename,
			referenced_files,
			reconstruction_anchor_plate_id,
			reconstruction_time);
		break;
	case ReconstructionGeometryExport::GMT:
		GMTFormatReconstructedFeatureGeometryExport::export_geometries(
			grouped_recon_geoms_seq,
			filename,
			referenced_files,
			reconstruction_anchor_plate_id,
			reconstruction_time);
		break;
	default:
		throw FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
			"Chosen export format is not currently supported.");
	}
}


template<>
void
GPlatesFileIO::ReconstructionGeometryExport::export_per_collection<GPlatesAppLogic::ReconstructedFlowline>(
		Format export_format,
		const std::list< FeatureGeometryGroup<GPlatesAppLogic::ReconstructedFlowline> > &grouped_recon_geoms_seq,
		const QString &filename,
		const std::vector<const File::Reference *> &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		const Options<GPlatesAppLogic::ReconstructedFlowline> &export_options)
{
	switch(export_format)
	{
		// I should only be exporting the reconstruction files here rather than all the active / referenced
		// files.  
	case ReconstructionGeometryExport::SHAPEFILE:
		ShapefileFormatFlowlineExport::export_flowlines(
			grouped_recon_geoms_seq,
			filename,
			referenced_files,
			reconstruction_anchor_plate_id,
			reconstruction_time,
			false /* export referenced files */);

		break;

	case ReconstructionGeometryExport::GMT:
		GMTFormatFlowlinesExport::export_flowlines(
			grouped_recon_geoms_seq,
			filename,
			referenced_files,
			reconstruction_anchor_plate_id,
			reconstruction_time);
		break;

	default:
		throw FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
			"Chosen export format is not currently supported.");
	}
}


template<>
void
GPlatesFileIO::ReconstructionGeometryExport::export_per_collection<GPlatesAppLogic::ReconstructedMotionPath>(
		Format export_format,
		const std::list< FeatureGeometryGroup<GPlatesAppLogic::ReconstructedMotionPath> > &grouped_recon_geoms_seq,
		const QString &filename,
		const std::vector<const File::Reference *> &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		const Options<GPlatesAppLogic::ReconstructedMotionPath> &export_options)
{
	switch(export_format)
	{
		// I should only be exporting the reconstruction files here rather than all the active / referenced
		// files.  
	case ReconstructionGeometryExport::SHAPEFILE:
		ShapefileFormatMotionPathExport::export_motion_paths(
			grouped_recon_geoms_seq,
			filename,
			referenced_files,
			reconstruction_anchor_plate_id,
			reconstruction_time,
			false /* export source files */);

		break;

	case ReconstructionGeometryExport::GMT:
		GMTFormatMotionPathsExport::export_motion_paths(
			grouped_recon_geoms_seq,
			filename,
			referenced_files,
			reconstruction_anchor_plate_id,
			reconstruction_time);
		break;

	default:
		throw FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
			"Chosen export format is not currently supported.");
	}
}
