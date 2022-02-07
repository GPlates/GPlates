/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#include <QLocale>
#include <QRegExp>
#include <QTextStream>

#include "DeformationExport.h"

#include "FileFormatNotSupportedException.h"
#include "GMTFormatDeformationExport.h"
#include "GpmlFormatDeformationExport.h"
#include "ReconstructionGeometryExportImpl.h"

#include "app-logic/TopologyReconstructedFeatureGeometry.h"

#include "maths/MathsUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/LogException.h"


using namespace GPlatesFileIO::ReconstructionGeometryExportImpl;


namespace GPlatesFileIO
{
	namespace DeformationExport
	{
		namespace
		{
			//! Typedef for a sequence of @a TopologyReconstructedFeatureGeometry objects.
			typedef std::list< FeatureGeometryGroup<GPlatesAppLogic::TopologyReconstructedFeatureGeometry> >
					deformed_feature_geometry_seq_type;

			//! Typedef for a sequence of @a FeatureCollectionFeatureGroup objects.
			typedef std::list< FeatureCollectionFeatureGroup<GPlatesAppLogic::TopologyReconstructedFeatureGeometry> >
					grouped_features_seq_type;
		}
	}
}


GPlatesFileIO::DeformationExport::PrincipalStrainOptions::PrincipalStrainOptions(
		OutputType output_,
		FormatType format_) :
	output(output_),
	format(format_)
{
}


double
GPlatesFileIO::DeformationExport::PrincipalStrainOptions::get_principal_angle_or_azimuth_in_degrees(
		const GPlatesAppLogic::DeformationStrain::StrainPrincipal &principal_strain) const
{
	// The angle in 'DeformationStrain::StrainPrincipal' is counter-clockwise and zero when axis points South
	// (actually the major/minor axes are each more like a line than a direction, so flipping by
	// 180 degrees doesn't matter and hence, for example, South and North are really the same line).
	double principal_angle_or_azimuth_in_degrees = GPlatesMaths::convert_rad_to_deg(principal_strain.angle);

	if (format == DeformationExport::PrincipalStrainOptions::ANGLE_MAJOR_MINOR)
	{
		// Convert angle such that -180 to +180 degrees is counter-clockwise from West and 0 is East.
		principal_angle_or_azimuth_in_degrees -= 90.0;
		// Make sure in range [-180, 180].
		if (principal_angle_or_azimuth_in_degrees > 180.0)
		{
			principal_angle_or_azimuth_in_degrees -= 360.0;
		}
		else if (principal_angle_or_azimuth_in_degrees < -180.0)
		{
			principal_angle_or_azimuth_in_degrees += 360.0;
		}
	}
	else // PrincipalStrainOptions::AZIMUTH_MAJOR_MINOR...
	{
		// Convert angle such that 0 to 360 degrees clockwise from North and 0 is North.
		principal_angle_or_azimuth_in_degrees = 180.0 - principal_angle_or_azimuth_in_degrees;
		// Make sure in range [0, 360].
		if (principal_angle_or_azimuth_in_degrees > 360.0)
		{
			principal_angle_or_azimuth_in_degrees -= 360.0;
		}
		else if (principal_angle_or_azimuth_in_degrees < 0.0)
		{
			principal_angle_or_azimuth_in_degrees += 360.0;
		}
	}

	return principal_angle_or_azimuth_in_degrees;
}


void
GPlatesFileIO::DeformationExport::export_deformation_to_gpml_format(
		const QString &filename,
		const std::vector<const GPlatesAppLogic::TopologyReconstructedFeatureGeometry *> &deformed_feature_geometry_seq,
		GPlatesModel::ModelInterface &model,
		const std::vector<const File::Reference *> &active_files,
		boost::optional<PrincipalStrainOptions> include_principal_strain,
		bool include_dilatation_strain,
		bool include_dilatation_strain_rate,
		bool include_second_invariant_strain_rate,
		bool export_single_output_file,
		bool export_per_input_file,
		bool export_separate_output_directory_per_input_file)
{
	// Get the list of active scalar coverage feature collection files that contain
	// the features referenced by the TopologyReconstructedFeatureGeometry objects.
	feature_handle_to_collection_map_type feature_to_collection_map;
	std::vector<const File::Reference *> referenced_files;
	get_files_referenced_by_geometries(
			referenced_files,
			deformed_feature_geometry_seq,
			active_files,
			feature_to_collection_map);

	// Group the TopologyReconstructedFeatureGeometry objects by their feature.
	deformed_feature_geometry_seq_type grouped_deformed_feature_geometry_seq;
	group_reconstruction_geometries_with_their_feature(
			grouped_deformed_feature_geometry_seq,
			deformed_feature_geometry_seq,
			feature_to_collection_map);

	if (export_single_output_file)
	{
		GpmlFormatDeformationExport::export_deformation(
				grouped_deformed_feature_geometry_seq,
				filename,
				model,
				include_principal_strain,
				include_dilatation_strain,
				include_dilatation_strain_rate,
				include_second_invariant_strain_rate);
	}

	if (export_per_input_file)
	{
		// Group the feature-groups with their collections. 
		grouped_features_seq_type grouped_features_seq;
		group_feature_geom_groups_with_their_collection(
				feature_to_collection_map,
				grouped_features_seq,
				grouped_deformed_feature_geometry_seq);

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
			GpmlFormatDeformationExport::export_deformation(
					grouped_features_iter->feature_geometry_groups,
					*output_filename_iter,
					model,
					include_principal_strain,
					include_dilatation_strain,
					include_dilatation_strain_rate,
					include_second_invariant_strain_rate);
		}
	}
}


void
GPlatesFileIO::DeformationExport::export_deformation_to_gmt_format(
		const QString &filename,
		const std::vector<const GPlatesAppLogic::TopologyReconstructedFeatureGeometry *> &deformed_feature_geometry_seq,
		const std::vector<const File::Reference *> &active_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool domain_point_lon_lat_format,
		boost::optional<PrincipalStrainOptions> include_principal_strain,
		bool include_dilatation_strain,
		bool include_dilatation_strain_rate,
		bool include_second_invariant_strain_rate,
		bool export_single_output_file,
		bool export_per_input_file,
		bool export_separate_output_directory_per_input_file)
{
	// Get the list of active scalar coverage feature collection files that contain
	// the features referenced by the TopologyReconstructedFeatureGeometry objects.
	feature_handle_to_collection_map_type feature_to_collection_map;
	std::vector<const File::Reference *> referenced_files;
	get_files_referenced_by_geometries(
			referenced_files,
			deformed_feature_geometry_seq,
			active_files,
			feature_to_collection_map);

	// Group the TopologyReconstructedFeatureGeometry objects by their feature.
	deformed_feature_geometry_seq_type grouped_deformed_feature_geometry_seq;
	group_reconstruction_geometries_with_their_feature(
			grouped_deformed_feature_geometry_seq,
			deformed_feature_geometry_seq,
			feature_to_collection_map);

	if (export_single_output_file)
	{
		GMTFormatDeformationExport::export_deformation(
				grouped_deformed_feature_geometry_seq,
				filename,
				referenced_files,
				reconstruction_anchor_plate_id,
				reconstruction_time,
				domain_point_lon_lat_format,
				include_principal_strain,
				include_dilatation_strain,
				include_dilatation_strain_rate,
				include_second_invariant_strain_rate);
	}

	if (export_per_input_file)
	{
		// Group the feature-groups with their collections. 
		grouped_features_seq_type grouped_features_seq;
		group_feature_geom_groups_with_their_collection(
				feature_to_collection_map,
				grouped_features_seq,
				grouped_deformed_feature_geometry_seq);

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
			GMTFormatDeformationExport::export_deformation(
					grouped_features_iter->feature_geometry_groups,
					*output_filename_iter,
					referenced_files,
					reconstruction_anchor_plate_id,
					reconstruction_time,
					domain_point_lon_lat_format,
					include_principal_strain,
					include_dilatation_strain,
					include_dilatation_strain_rate,
					include_second_invariant_strain_rate);
		}
	}
}
