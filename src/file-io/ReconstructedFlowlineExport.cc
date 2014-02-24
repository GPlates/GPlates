/* $Id$ */

/**
 * \file Exports reconstructed feature geometries to a file.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
 * Copyright (C) 2010, 2011 Geological Survey of Norway
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

#include "ReconstructedFlowlineExport.h"

#include "GMTFormatFlowlineExport.h"
#include "FeatureCollectionFileFormat.h"
#include "FeatureCollectionFileFormatRegistry.h"
#include "FileFormatNotSupportedException.h"
#include "ReconstructionGeometryExportImpl.h"
#include "OgrFormatFlowlineExport.h"

using namespace GPlatesFileIO::ReconstructionGeometryExportImpl;


namespace GPlatesFileIO
{
	namespace ReconstructedFlowlineExport
	{
		namespace
		{
			//! Typedef for a sequence of @a FeatureGeometryGroup objects.
			typedef std::list< FeatureGeometryGroup<GPlatesAppLogic::ReconstructedFlowline> >
					feature_geometry_group_seq_type;

			//! Typedef for a sequence of @a FeatureCollectionFeatureGroup objects.
			typedef std::list< FeatureCollectionFeatureGroup<GPlatesAppLogic::ReconstructedFlowline> >
					grouped_features_seq_type;


			void
			export_as_single_file(
					const QString &filename,
					Format export_format,
					const feature_geometry_group_seq_type &grouped_recon_geoms_seq,
					const std::vector<const File::Reference *> &referenced_files,
					const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
					const double &reconstruction_time,
					bool wrap_to_dateline)
			{
				switch (export_format)
				{
				case GMT:
					GMTFormatFlowlinesExport::export_flowlines(
						grouped_recon_geoms_seq,
						filename,
						referenced_files,
						reconstruction_anchor_plate_id,
						reconstruction_time);
					break;

				case SHAPEFILE:
					OgrFormatFlowlineExport::export_flowlines(
						grouped_recon_geoms_seq,
						filename,
						referenced_files,
						reconstruction_anchor_plate_id,
						reconstruction_time,
						wrap_to_dateline);		
					break;

				case OGRGMT:
					OgrFormatFlowlineExport::export_flowlines(
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

			void
			export_per_collection(
					const QString &filename,
					Format export_format,
					const feature_geometry_group_seq_type &grouped_recon_geoms_seq,
					const std::vector<const File::Reference *> &referenced_files,
					const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
					const double &reconstruction_time,
					bool wrap_to_dateline)
			{
				switch(export_format)
				{
				case SHAPEFILE:
					OgrFormatFlowlineExport::export_flowlines(
						grouped_recon_geoms_seq,
						filename,
						referenced_files,
						reconstruction_anchor_plate_id,
						reconstruction_time,
						wrap_to_dateline);
					break;
				case OGRGMT:
					OgrFormatFlowlineExport::export_flowlines(
						grouped_recon_geoms_seq,
						filename,
						referenced_files,
						reconstruction_anchor_plate_id,
						reconstruction_time);
					break;
				case GMT:
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
		}
	}
}


GPlatesFileIO::ReconstructedFlowlineExport::Format
GPlatesFileIO::ReconstructedFlowlineExport::get_export_file_format(
		const QFileInfo& file_info,
		const FeatureCollectionFileFormat::Registry &file_format_registry)
{
	// Since we're using a feature collection file format to export
	// our RFGs we'll use the feature collection file format code.
	const boost::optional<FeatureCollectionFileFormat::Format> feature_collection_file_format =
			file_format_registry.get_file_format(file_info);
	if (!feature_collection_file_format ||
		!file_format_registry.does_file_format_support_writing(feature_collection_file_format.get()))
	{
		return UNKNOWN;
	}

	// Only some feature collection file formats are used for exporting
	// reconstructed feature geometries because most file formats only
	// make sense for unreconstructed geometry (since they provide the
	// information required to do the reconstructions).
	switch (feature_collection_file_format.get())
	{
	case FeatureCollectionFileFormat::WRITE_ONLY_XY_GMT:
		return GMT;
	case FeatureCollectionFileFormat::SHAPEFILE:
		return SHAPEFILE;
	case FeatureCollectionFileFormat::OGRGMT:
		return OGRGMT;
	default:
		break;
	}

	return UNKNOWN;
}


void
GPlatesFileIO::ReconstructedFlowlineExport::export_reconstructed_flowlines(
		const QString &filename,
		Format export_format,
		const std::vector<const GPlatesAppLogic::ReconstructedFlowline *> &reconstructed_flowline_seq,
		const std::vector<const File::Reference *> &active_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool export_single_output_file,
		bool export_per_input_file,
		bool export_separate_output_directory_per_input_file,
		bool wrap_to_dateline)
{
	// Get the list of active reconstructable feature collection files that contain
	// the features referenced by the ReconstructionGeometry objects.
	feature_handle_to_collection_map_type feature_to_collection_map;
	std::vector<const File::Reference *> referenced_files;
	get_files_referenced_by_geometries(
			referenced_files, reconstructed_flowline_seq, active_files,
			feature_to_collection_map);

	// Group the ReconstructionGeometry objects by their feature.
	feature_geometry_group_seq_type grouped_recon_geom_seq;
	group_reconstruction_geometries_with_their_feature(
			grouped_recon_geom_seq, reconstructed_flowline_seq);

	if (export_single_output_file)
	{
		export_as_single_file(
				filename,
				export_format,
				grouped_recon_geom_seq,
				referenced_files,
				reconstruction_anchor_plate_id,
				reconstruction_time,
				wrap_to_dateline);
	}

	if (export_per_input_file)
	{
		// Group the feature-groups with their collections. 
		grouped_features_seq_type grouped_features_seq;
		group_feature_geom_groups_with_their_collection(
				feature_to_collection_map,
				grouped_features_seq,
				grouped_recon_geom_seq);

		std::vector<QString> output_filenames;
		get_output_filenames(
				output_filenames,
				filename,
				grouped_features_seq,
				export_separate_output_directory_per_input_file);

		grouped_features_seq_type::const_iterator grouped_features_iter = grouped_features_seq.begin();
		grouped_features_seq_type::const_iterator grouped_features_end = grouped_features_seq.end();
		for (std::vector<QString>::const_iterator output_filename_iter = output_filenames.begin();
			grouped_features_iter != grouped_features_end;
			++grouped_features_iter, ++output_filename_iter)
		{
			export_per_collection(
					*output_filename_iter,
					export_format,
					grouped_features_iter->feature_geometry_groups,
					referenced_files,
					reconstruction_anchor_plate_id,
					reconstruction_time,
					wrap_to_dateline);
		}
	}
}
