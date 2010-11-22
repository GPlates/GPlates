/* $Id$ */

/**
 * \file Exports reconstructed feature geometries to a file.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
 * Copyright (C) 2010 Geological Survey of Norway
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

#include "ReconstructedFeatureGeometryExport.h"

#include "ErrorOpeningFileForWritingException.h"
#include "GMTFormatReconstructedFeatureGeometryExport.h"
#include "FeatureCollectionFileFormat.h"
#include "FileFormatNotSupportedException.h"
#include "ShapefileFormatReconstructedFeatureGeometryExport.h"
#include "ShapefileUtils.h"
#include "ReconstructedFeatureGeometryExportImpl.h"

namespace
{
	QString
	build_flat_structure_filename(
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

	/**
	 * Builds output file name for folder-format output, and creates and subfolders
	 * if they do not already exist. 
	 */
	QString
	build_folder_structure_filename(
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

	void
	export_as_single_file(
		const QString &filename,
		GPlatesFileIO::ReconstructedFeatureGeometryExport::Format export_format,
		const GPlatesFileIO::ReconstructedFeatureGeometryExportImpl::feature_geometry_group_seq_type &grouped_rfgs_seq,
		const GPlatesFileIO::ReconstructedFeatureGeometryExportImpl::referenced_files_collection_type &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time)
	{
		using namespace GPlatesFileIO;

		switch (export_format)
		{
		case ReconstructedFeatureGeometryExport::GMT:
			GMTFormatReconstructedFeatureGeometryExport::export_geometries(
				grouped_rfgs_seq,
				filename,
				referenced_files,
				reconstruction_anchor_plate_id,
				reconstruction_time);
			break;

		case ReconstructedFeatureGeometryExport::SHAPEFILE:
			ShapefileFormatReconstructedFeatureGeometryExport::export_geometries(
				grouped_rfgs_seq,
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


	void
	export_per_collection(
		const QString &filename,
		GPlatesFileIO::ReconstructedFeatureGeometryExport::Format export_format,
		const GPlatesFileIO::ReconstructedFeatureGeometryExportImpl::feature_collection_feature_group_seq_type &grouped_features_seq,
		const GPlatesFileIO::ReconstructedFeatureGeometryExportImpl::referenced_files_collection_type &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time)
	{
		using namespace GPlatesFileIO;

		ReconstructedFeatureGeometryExportImpl::feature_collection_feature_group_seq_type::const_iterator 
			it = grouped_features_seq.begin(),
			end = grouped_features_seq.end();

		QFileInfo export_qfile_info(filename);
		QString export_path = export_qfile_info.absolutePath();
		QString export_filename = export_qfile_info.fileName();

		for (; it != end; ++it)
		{
			const File::Reference *file_ptr = it->file_ptr;	
			FileInfo file_info = file_ptr->get_file_info();
			QFileInfo qfile_info = file_info.get_qfileinfo();
			QString collection_filename = qfile_info.completeBaseName();

#if 1
			// Folder-structure output
			QString output_filename = build_folder_structure_filename(export_path,collection_filename,export_filename);
#else	
			// Flat-structure output.
			QString output_filename = build_flat_structure_filename(export_path,collection_filename,export_filename);
#endif


			boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type> kvd;
			ShapefileUtils::create_default_kvd_from_collection(file_ptr->get_feature_collection(),kvd);

			switch(export_format)
			{
 
			case ReconstructedFeatureGeometryExport::SHAPEFILE:
				ShapefileFormatReconstructedFeatureGeometryExport::export_geometries_per_collection(
					it->feature_geometry_groups,
					output_filename,
					referenced_files,
					reconstruction_anchor_plate_id,
					reconstruction_time);
				break;
			case ReconstructedFeatureGeometryExport::GMT:
				GMTFormatReconstructedFeatureGeometryExport::export_geometries(
					it->feature_geometry_groups,
					output_filename,
					referenced_files,
					reconstruction_anchor_plate_id,
					reconstruction_time);
				break;
			default:
				throw FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
					"Chosen export format is not currently supported.");
			}
		} // iterate over collections
	}
} // namespace

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
GPlatesFileIO::ReconstructedFeatureGeometryExport::export_reconstructed_feature_geometries(
	const QString &filename,
	ReconstructedFeatureGeometryExport::Format export_format,
	const reconstructed_feature_geom_seq_type &reconstructed_feature_geom_seq,
	const files_collection_type &active_files,
	const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
	const double &reconstruction_time)
{
	// Get the list of active reconstructable feature collection files that contain
	// the features referenced by the ReconstructedFeatureGeometry objects.
	ReconstructedFeatureGeometryExportImpl::feature_handle_to_collection_map_type feature_to_collection_map;
	ReconstructedFeatureGeometryExportImpl::referenced_files_collection_type referenced_files;
	ReconstructedFeatureGeometryExportImpl::get_files_referenced_by_geometries(
		referenced_files, reconstructed_feature_geom_seq, active_files,
		feature_to_collection_map);

	// Group the RFGs by their feature.
	ReconstructedFeatureGeometryExportImpl::feature_geometry_group_seq_type grouped_rfgs_seq;
	ReconstructedFeatureGeometryExportImpl::group_rfgs_with_their_feature(
		grouped_rfgs_seq, reconstructed_feature_geom_seq);


	// Group the feature-groups with their collections. 
	ReconstructedFeatureGeometryExportImpl::feature_collection_feature_group_seq_type grouped_features_seq;
	ReconstructedFeatureGeometryExportImpl::group_feature_geom_groups_with_their_collection(
		feature_to_collection_map,
		grouped_features_seq,
		grouped_rfgs_seq);


	export_as_single_file(
		filename,
		export_format,
		grouped_rfgs_seq,
		referenced_files,
		reconstruction_anchor_plate_id,
		reconstruction_time);

	export_per_collection(
		filename,
		export_format,
		grouped_features_seq,
		referenced_files,
		reconstruction_anchor_plate_id,
		reconstruction_time);

}