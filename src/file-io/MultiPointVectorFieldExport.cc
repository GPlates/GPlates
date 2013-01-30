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

#include "MultiPointVectorFieldExport.h"

#include "FileFormatNotSupportedException.h"
#include "GpmlFormatMultiPointVectorFieldExport.h"
#include "ReconstructionGeometryExportImpl.h"

#include "app-logic/MultiPointVectorField.h"

using namespace GPlatesFileIO::ReconstructionGeometryExportImpl;


namespace GPlatesFileIO
{
	namespace MultiPointVectorFieldExport
	{
		namespace
		{
			//! Typedef for a sequence of @a MultiPointVectorField objects.
			typedef std::list< FeatureGeometryGroup<GPlatesAppLogic::MultiPointVectorField> >
					multi_point_vector_field_seq_type;

			//! Typedef for a sequence of @a FeatureCollectionFeatureGroup objects.
			typedef std::list< FeatureCollectionFeatureGroup<GPlatesAppLogic::MultiPointVectorField> >
					grouped_features_seq_type;


			void
			export_velocity_vector_fields_as_single_file(
					const QString &filename,
					VelocityFormat export_format,
					const multi_point_vector_field_seq_type &grouped_velocity_vector_fields_seq,
					const GPlatesModel::Gpgim &gpgim,
					GPlatesModel::ModelInterface &model,
					const std::vector<const File::Reference *> &referenced_files,
					const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
					const double &reconstruction_time)
			{
				switch (export_format)
				{
				case GPML:
					GpmlFormatMultiPointVectorFieldExport::export_velocity_vector_fields(
						grouped_velocity_vector_fields_seq,
						filename,
						gpgim,
						model,
						referenced_files,
						reconstruction_anchor_plate_id,
						reconstruction_time);
					break;

#if 0
				case GMT:
					GMTFormatMultiPointVectorFieldExport::export_geometries(
						grouped_velocity_vector_fields_seq,
						filename,
						referenced_files,
						reconstruction_anchor_plate_id,
						reconstruction_time);		
					break;
#endif

				default:
					throw FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
						"Chosen export format is not currently supported.");
				}
			}

			void
			export_velocity_vector_fields_per_collection(
					const QString &filename,
					VelocityFormat export_format,
					const multi_point_vector_field_seq_type &grouped_velocity_vector_fields_seq,
					const GPlatesModel::Gpgim &gpgim,
					GPlatesModel::ModelInterface &model,
					const std::vector<const File::Reference *> &referenced_files,
					const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
					const double &reconstruction_time)
			{
				switch(export_format)
				{
				case GPML:
					GpmlFormatMultiPointVectorFieldExport::export_velocity_vector_fields(
						grouped_velocity_vector_fields_seq,
						filename,
						gpgim,
						model,
						referenced_files,
						reconstruction_anchor_plate_id,
						reconstruction_time);
					break;

#if 0
				case GMT:
					GMTFormatMultiPointVectorFieldExport::export_geometries(
						grouped_velocity_vector_fields_seq,
						filename,
						referenced_files,
						reconstruction_anchor_plate_id,
						reconstruction_time);		
					break;
#endif

				default:
					throw FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
						"Chosen export format is not currently supported.");
				}
			}
		}
	}
}


void
GPlatesFileIO::MultiPointVectorFieldExport::export_velocity_vector_fields(
		const QString &filename,
		VelocityFormat export_format,
		const std::vector<const GPlatesAppLogic::MultiPointVectorField *> &velocity_vector_field_seq,
		const GPlatesModel::Gpgim &gpgim,
		GPlatesModel::ModelInterface &model,
		const std::vector<const File::Reference *> &active_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool export_single_output_file,
		bool export_per_input_file,
		bool export_separate_output_directory_per_input_file)
{
	// Get the list of active multi-point vector field feature collection files that contain
	// the features referenced by the MultiPointVectorField objects.
	feature_handle_to_collection_map_type feature_to_collection_map;
	std::vector<const File::Reference *> referenced_files;
	get_files_referenced_by_geometries(
			referenced_files,
			velocity_vector_field_seq,
			active_files,
			feature_to_collection_map);

	// Group the MultiPointVectorField objects by their feature.
	multi_point_vector_field_seq_type grouped_velocity_vector_field_seq;
	group_reconstruction_geometries_with_their_feature(
			grouped_velocity_vector_field_seq,
			velocity_vector_field_seq);

	if (export_single_output_file)
	{
		export_velocity_vector_fields_as_single_file(
				filename,
				export_format,
				grouped_velocity_vector_field_seq,
				gpgim,
				model,
				referenced_files,
				reconstruction_anchor_plate_id,
				reconstruction_time);
	}

	if (export_per_input_file)
	{
		// Group the feature-groups with their collections. 
		grouped_features_seq_type grouped_features_seq;
		group_feature_geom_groups_with_their_collection(
				feature_to_collection_map,
				grouped_features_seq,
				grouped_velocity_vector_field_seq);

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
			export_velocity_vector_fields_per_collection(
					*output_filename_iter,
					export_format,
					grouped_features_iter->feature_geometry_groups,
					gpgim,
					model,
					referenced_files,
					reconstruction_anchor_plate_id,
					reconstruction_time);
		}
	}
}
