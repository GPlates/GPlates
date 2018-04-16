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

#include <QLocale>
#include <QRegExp>
#include <QTextStream>

#include "MultiPointVectorFieldExport.h"

#include "CitcomsFormatVelocityVectorFieldExport.h"
#include "FileFormatNotSupportedException.h"
#include "GMTFormatMultiPointVectorFieldExport.h"
#include "GpmlFormatMultiPointVectorFieldExport.h"
#include "ReconstructionGeometryExportImpl.h"
#include "TerraFormatVelocityVectorFieldExport.h"

#include "app-logic/MultiPointVectorField.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/LogException.h"


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
		}
	}
}


void
GPlatesFileIO::MultiPointVectorFieldExport::export_velocity_vector_fields_to_gpml_format(
		const QString &filename,
		const std::vector<const GPlatesAppLogic::MultiPointVectorField *> &velocity_vector_field_seq,
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
			velocity_vector_field_seq,
			feature_to_collection_map);

	if (export_single_output_file)
	{
		GpmlFormatMultiPointVectorFieldExport::export_velocity_vector_fields(
				grouped_velocity_vector_field_seq,
				filename,
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
			GpmlFormatMultiPointVectorFieldExport::export_velocity_vector_fields(
					grouped_features_iter->feature_geometry_groups,
					*output_filename_iter,
					model,
					referenced_files,
					reconstruction_anchor_plate_id,
					reconstruction_time);
		}
	}
}


void
GPlatesFileIO::MultiPointVectorFieldExport::export_velocity_vector_fields_to_gmt_format(
		const QString &filename,
		const std::vector<const GPlatesAppLogic::MultiPointVectorField *> &velocity_vector_field_seq,
		const std::vector<const File::Reference *> &active_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		GMTVelocityVectorFormatType velocity_vector_format,
		double velocity_scale,
		unsigned int velocity_stride,
		bool domain_point_lon_lat_format,
		bool include_plate_id,
		bool include_domain_point,
		bool include_domain_meta_data,
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
			velocity_vector_field_seq,
			feature_to_collection_map);

	if (export_single_output_file)
	{
		GMTFormatMultiPointVectorFieldExport::export_velocity_vector_fields(
				grouped_velocity_vector_field_seq,
				filename,
				referenced_files,
				reconstruction_anchor_plate_id,
				reconstruction_time,
				velocity_vector_format,
				velocity_scale,
				velocity_stride,
				domain_point_lon_lat_format,
				include_plate_id,
				include_domain_point,
				include_domain_meta_data);
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
			GMTFormatMultiPointVectorFieldExport::export_velocity_vector_fields(
					grouped_features_iter->feature_geometry_groups,
					*output_filename_iter,
					referenced_files,
					reconstruction_anchor_plate_id,
					reconstruction_time,
					velocity_vector_format,
					velocity_scale,
					velocity_stride,
					domain_point_lon_lat_format,
					include_plate_id,
					include_domain_point,
					include_domain_meta_data);
		}
	}
}


void
GPlatesFileIO::MultiPointVectorFieldExport::export_velocity_vector_fields_to_terra_text_format(
		const QString &velocity_domain_file_name_template,
		const QString &velocity_export_file_name_template,
		const QString &velocity_domain_mt_place_holder,
		const QString &velocity_domain_nt_place_holder,
		const QString &velocity_domain_nd_place_holder,
		const QString &velocity_domain_processor_place_holder,
		const QString &velocity_export_processor_place_holder,
		const std::vector<const GPlatesAppLogic::MultiPointVectorField *> &velocity_vector_field_seq,
		const std::vector<const File::Reference *> &active_files,
		int age)
{
	// Get the list of active multi-point vector field feature collection files that contain
	// the features referenced by the MultiPointVectorField objects.
	feature_handle_to_collection_map_type feature_to_collection_map;
	populate_feature_handle_to_collection_map(
			feature_to_collection_map,
			active_files);

	// Group the MultiPointVectorField objects by their feature.
	multi_point_vector_field_seq_type grouped_velocity_vector_field_seq;
	group_reconstruction_geometries_with_their_feature(
			grouped_velocity_vector_field_seq,
			velocity_vector_field_seq,
			feature_to_collection_map);

	// Group the feature-groups with their collections.
	grouped_features_seq_type grouped_features_seq;
	group_feature_geom_groups_with_their_collection(
			feature_to_collection_map,
			grouped_features_seq,
			grouped_velocity_vector_field_seq);

	// Convert the velocity domain file name template to a regular expression by replacing the
	// placeholders with regular expressions that match the Terra integer parameters.
	const QString unsigned_integer_reg_exp_string("(\\d+)");
	QString velocity_domain_file_name_reg_exp_string(velocity_domain_file_name_template);
	velocity_domain_file_name_reg_exp_string.replace(
			velocity_domain_mt_place_holder,
			unsigned_integer_reg_exp_string);
	velocity_domain_file_name_reg_exp_string.replace(
			velocity_domain_nt_place_holder,
			unsigned_integer_reg_exp_string);
	velocity_domain_file_name_reg_exp_string.replace(
			velocity_domain_nd_place_holder,
			unsigned_integer_reg_exp_string);
	velocity_domain_file_name_reg_exp_string.replace(
			velocity_domain_processor_place_holder,
			unsigned_integer_reg_exp_string);
	const QRegExp velocity_domain_file_name_reg_exp(velocity_domain_file_name_reg_exp_string);

	// Determine the order of the template parameter placeholders.
	const int index_of_mt = velocity_domain_file_name_template.indexOf(velocity_domain_mt_place_holder);
	const int index_of_nt = velocity_domain_file_name_template.indexOf(velocity_domain_nt_place_holder);
	const int index_of_nd = velocity_domain_file_name_template.indexOf(velocity_domain_nd_place_holder);
	const int index_of_np = velocity_domain_file_name_template.indexOf(velocity_domain_processor_place_holder);
	// Throw exception if cannot find all placeholders.
	// This exception will get caught by the velocity export animation.
	GPlatesGlobal::Assert<GPlatesGlobal::LogException>(
			index_of_mt >= 0 && index_of_nt >= 0 && index_of_nd >= 0 && index_of_np >= 0,
			GPLATES_ASSERTION_SOURCE,
			"Error finding parameters from velocity domain file name when exporting velocities to Terra format.");

	// The regular expression group ordering depends on the order of placeholders in the template.
	// The values will be in the range [0,3].
	const int reg_exp_group_order_of_mt =
			(index_of_mt > index_of_nt) + (index_of_mt > index_of_nd) + (index_of_mt > index_of_np);
	const int reg_exp_group_order_of_nt =
			(index_of_nt > index_of_mt) + (index_of_nt > index_of_nd) + (index_of_nt > index_of_np);
	const int reg_exp_group_order_of_nd =
			(index_of_nd > index_of_mt) + (index_of_nd > index_of_nt) + (index_of_nd > index_of_np);
	const int reg_exp_group_order_of_np =
			(index_of_np > index_of_mt) + (index_of_np > index_of_nt) + (index_of_np > index_of_nd);

	grouped_features_seq_type::const_iterator grouped_features_iter = grouped_features_seq.begin();
	grouped_features_seq_type::const_iterator grouped_features_end = grouped_features_seq.end();
	for ( ; grouped_features_iter != grouped_features_end; ++grouped_features_iter)
	{
		// Get the current velocity *domain* filename.
		const File::Reference *file_ptr = grouped_features_iter->file_ptr;	
		const QFileInfo qfile_info = file_ptr->get_file_info().get_qfileinfo();
		const QString velocity_domain_filename = qfile_info.completeBaseName();

		// See if the current velocity domain filename matches the template.
		if (velocity_domain_file_name_reg_exp.indexIn(velocity_domain_filename) < 0)
		{
			continue;
		}

		const QStringList template_parameters = velocity_domain_file_name_reg_exp.capturedTexts();

		// All template parameters must have matched to get here.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				template_parameters.size() == 5, // 4 Terra parameters plus 1 for the entire match.
				GPLATES_ASSERTION_SOURCE);

		// The individual template parameter strings.
		const QString mt_string = template_parameters.at(reg_exp_group_order_of_mt + 1);
		const QString nt_string = template_parameters.at(reg_exp_group_order_of_nt + 1);
		const QString nd_string = template_parameters.at(reg_exp_group_order_of_nd + 1);
		const QString np_string = template_parameters.at(reg_exp_group_order_of_np + 1);

		// Use the "C" locale to convert sub-strings (in the file names) to integers.
		static const QLocale C_LOCALE = QLocale::c();

		// The regular expression has ensured the parameter strings contain only unsigned integers.
		bool mt_ok, nt_ok, nd_ok, np_ok;
		const uint mt = C_LOCALE.toUInt(mt_string, &mt_ok);
		const uint nt = C_LOCALE.toUInt(nt_string, &nt_ok);
		const uint nd = C_LOCALE.toUInt(nd_string, &nd_ok);
		const uint np = C_LOCALE.toUInt(np_string, &np_ok);
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				mt_ok && nt_ok && nd_ok && np_ok,
				GPLATES_ASSERTION_SOURCE);

		// Note that the Terra export velocity *filename* expects four digits for the local processor number.
		QString np_string_four_digits;
		QTextStream np_string_four_digits_stream(&np_string_four_digits);
		np_string_four_digits_stream << qSetFieldWidth(4) << qSetPadChar('0') << np;

		// Form the current export file name from the template by replacing the processor
		// placeholder with the current processor number.
		QString velocity_export_file_name(velocity_export_file_name_template);
		velocity_export_file_name.replace(velocity_export_processor_place_holder, np_string_four_digits);

		// Finally we can export to the current velocity file.
		TerraFormatVelocityVectorFieldExport::export_velocity_vector_fields(
				grouped_features_iter->feature_geometry_groups,
				velocity_export_file_name,
				mt,
				nt,
				nd,
				np,
				age);
	}
}


void
GPlatesFileIO::MultiPointVectorFieldExport::export_velocity_vector_fields_to_citcoms_global_format(
		const QString &velocity_domain_file_name_template,
		const QString &velocity_export_file_name_template,
		const QString &velocity_domain_density_place_holder,
		const QString &velocity_domain_cap_number_place_holder,
		const QString &velocity_export_cap_number_place_holder,
		const std::vector<const GPlatesAppLogic::MultiPointVectorField *> &velocity_vector_field_seq,
		const std::vector<const File::Reference *> &active_files,
		int age,
		bool include_gmt_export,
		double gmt_velocity_scale,
		unsigned int gmt_velocity_stride)
{
	// Get the list of active multi-point vector field feature collection files that contain
	// the features referenced by the MultiPointVectorField objects.
	feature_handle_to_collection_map_type feature_to_collection_map;
	populate_feature_handle_to_collection_map(
			feature_to_collection_map,
			active_files);

	// Group the MultiPointVectorField objects by their feature.
	multi_point_vector_field_seq_type grouped_velocity_vector_field_seq;
	group_reconstruction_geometries_with_their_feature(
			grouped_velocity_vector_field_seq,
			velocity_vector_field_seq,
			feature_to_collection_map);

	// Group the feature-groups with their collections.
	grouped_features_seq_type grouped_features_seq;
	group_feature_geom_groups_with_their_collection(
			feature_to_collection_map,
			grouped_features_seq,
			grouped_velocity_vector_field_seq);

	// Convert the velocity domain file name template to a regular expression by replacing the
	// placeholders with regular expressions that match the CitcomS integer parameters.
	const QString unsigned_integer_reg_exp_string("(\\d+)");
	QString velocity_domain_file_name_reg_exp_string(velocity_domain_file_name_template);
	velocity_domain_file_name_reg_exp_string.replace(
			velocity_domain_density_place_holder,
			unsigned_integer_reg_exp_string);
	velocity_domain_file_name_reg_exp_string.replace(
			velocity_domain_cap_number_place_holder,
			unsigned_integer_reg_exp_string);
	const QRegExp velocity_domain_file_name_reg_exp(velocity_domain_file_name_reg_exp_string);

	// Determine the order of the template parameter placeholders.
	const int index_of_density = velocity_domain_file_name_template.indexOf(velocity_domain_density_place_holder);
	const int index_of_cap_number = velocity_domain_file_name_template.indexOf(velocity_domain_cap_number_place_holder);
	// Throw exception if cannot find all placeholders.
	// This exception will get caught by the velocity export animation.
	GPlatesGlobal::Assert<GPlatesGlobal::LogException>(
			index_of_density >= 0 && index_of_cap_number >= 0,
			GPLATES_ASSERTION_SOURCE,
			"Error finding parameters from velocity domain file name when exporting velocities to CitcomS global format.");

	// The regular expression group ordering depends on the order of placeholders in the template.
	// The values will be in the range [0,3].
	const int reg_exp_group_order_of_density = (index_of_density > index_of_cap_number);
	const int reg_exp_group_order_of_cap_number = (index_of_cap_number > index_of_density);

	grouped_features_seq_type::const_iterator grouped_features_iter = grouped_features_seq.begin();
	grouped_features_seq_type::const_iterator grouped_features_end = grouped_features_seq.end();
	for ( ; grouped_features_iter != grouped_features_end; ++grouped_features_iter)
	{
		// Get the current velocity *domain* filename.
		const File::Reference *file_ptr = grouped_features_iter->file_ptr;	
		const QFileInfo qfile_info = file_ptr->get_file_info().get_qfileinfo();
		const QString velocity_domain_filename = qfile_info.completeBaseName();

		// See if the current velocity domain filename matches the template.
		if (velocity_domain_file_name_reg_exp.indexIn(velocity_domain_filename) < 0)
		{
			continue;
		}

		const QStringList template_parameters = velocity_domain_file_name_reg_exp.capturedTexts();

		// All template parameters must have matched to get here.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				template_parameters.size() == 3, // 2 CitcomS parameters plus 1 for the entire match.
				GPLATES_ASSERTION_SOURCE);

		// The individual template parameter strings.
		const QString density_string = template_parameters.at(reg_exp_group_order_of_density + 1);
		const QString cap_number_string = template_parameters.at(reg_exp_group_order_of_cap_number + 1);

#if 0
		// Use the "C" locale to convert sub-strings (in the file names) to integers.
		static const QLocale C_LOCALE = QLocale::c();

		// The regular expression has ensured the parameter strings contain only unsigned integers.
		bool density_ok, cap_number_ok;
		const uint density = C_LOCALE.toUInt(density_string, &density_ok);
		const uint cap_number = C_LOCALE.toUInt(cap_number_string, &cap_number_ok);
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				density_ok && cap_number_ok,
				GPLATES_ASSERTION_SOURCE);
#endif

		// Form the current export file name from the template by replacing the cap number
		// placeholder with the current cap number.
		QString velocity_export_file_name(velocity_export_file_name_template);
		velocity_export_file_name.replace(velocity_export_cap_number_place_holder, cap_number_string);

		// Finally we can export to the current velocity file.
		CitcomsFormatVelocityVectorFieldExport::export_global_velocity_vector_fields(
				grouped_features_iter->feature_geometry_groups,
				velocity_export_file_name,
				age,
				include_gmt_export,
				gmt_velocity_scale,
				gmt_velocity_stride);
	}
}
