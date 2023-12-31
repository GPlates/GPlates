/* $Id$ */

/**
 * \file Exports multi-point vector fields to a file.
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

#ifndef GPLATES_FILEIO_MULTIPOINTVECTORFIELDEXPORT_H
#define GPLATES_FILEIO_MULTIPOINTVECTORFIELDEXPORT_H

#include <vector>
#include <QFileInfo>
#include <QString>

#include "file-io/File.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class MultiPointVectorField;
}

namespace GPlatesModel
{
	class ModelInterface;
}

namespace GPlatesFileIO
{
	namespace MultiPointVectorFieldExport
	{
		//
		// Export *velocity* multi-point vector fields.
		//
		// 


		/**
		 * How to write out each velocity vector to GMT format.
		 */
		enum GMTVelocityVectorFormatType
		{
			GMT_VELOCITY_VECTOR_3D,
			GMT_VELOCITY_VECTOR_COLAT_LON,
			GMT_VELOCITY_VECTOR_ANGLE_MAGNITUDE,
			GMT_VELOCITY_VECTOR_AZIMUTH_MAGNITUDE
		};


		/**
		 * Exports @a MultiPointVectorField objects containing *velocities* to the GPML file format.
		 *
		 * NOTE: The GPML format stores velocities only in colat/lon format.
		 *
		 * @param export_single_output_file specifies whether to write all velocity vector fields to a single file.
		 * @param export_per_input_file specifies whether to group velocity vector fields according
		 *        to the input files their features came from and write to corresponding output files.
		 * @param export_separate_output_directory_per_input_file
		 *        Save each exported file to a different directory based on the file basename.
		 *        Only applies if @a export_per_input_file is 'true'.
		 *
		 * Note that both @a export_single_output_file and @a export_per_input_file can be true
		 * in which case both a single output file is exported as well as grouped output files.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 */
		void
		export_velocity_vector_fields_to_gpml_format(
				const QString &filename,
				const std::vector<const GPlatesAppLogic::MultiPointVectorField *> &velocity_vector_field_seq,
				GPlatesModel::ModelInterface &model,
				const std::vector<const File::Reference *> &active_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool export_single_output_file,
				bool export_per_input_file,
				bool export_separate_output_directory_per_input_file);


		/**
		 * Exports @a MultiPointVectorField objects containing *velocities* to the GMT file format.
		 *
		 * Note that GMT format provides a choice of how to output each velocity vector.
		 *
		 * Each line in the GMT file contains:
		 *    [domain_point] velocity [plate_id]
		 * ...where 'domain_point' is position at which the velocity was calculated and 'plate_id'
		 * is the plate id used to calculate the velocity (for topological networks the plate id
		 * only identifies the network used to calculate the velocity).
		 *
		 * The plate ID is only included if @a include_plate_id is true.
		 * The domain point is only included if @a include_domain_point is true.
		 * If @a domain_point_lon_lat_format is true then the domain points are output as the
		 * GMT default of (longitude latitude), otherwise they're output as (latitude longitude).
		 *
		 * Velocity magnitudes are scaled by @a velocity_scale.
		 * Only every 'velocity_stride'th velocity vector is output.
		 *
		 * @param export_single_output_file specifies whether to write all velocity vector fields to a single file.
		 * @param export_per_input_file specifies whether to group velocity vector fields according
		 *        to the input files their features came from and write to corresponding output files.
		 * @param export_separate_output_directory_per_input_file
		 *        Save each exported file to a different directory based on the file basename.
		 *        Only applies if @a export_per_input_file is 'true'.
		 *
		 * Note that both @a export_single_output_file and @a export_per_input_file can be true
		 * in which case both a single output file is exported as well as grouped output files.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 */
		void
		export_velocity_vector_fields_to_gmt_format(
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
				bool export_separate_output_directory_per_input_file);


		/**
		 * Exports @a MultiPointVectorField objects containing *velocities* to the Terra text file format.
		 *
		 * NOTE: Only velocity vector fields associated with velocity domain (grid) files whose
		 * file names match the template @a velocity_domain_file_name_template are exported.
		 *
		 * For example, a *domain* template of "TerraMesh.%MT.%NT.%ND.%NP" will match a velocity field whose
		 * domain came from a file called "TerraMesh.32.16.5.1" where '1' is the Terra local processor number.
		 *
		 * The velocity *domain* template parameters are:
		 *   @a velocity_domain_mt_place_holder - used to match Terra 'mt' parameter,
		 *   @a velocity_domain_nt_place_holder - used to match Terra 'nt' parameter,
		 *   @a velocity_domain_nd_place_holder - used to match Terra 'nd' parameter,
		 *   @a velocity_domain_processor_place_holder - used to match the Terra local processor number.
		 * And there must be one, and only one, occurrence of each of these parameter placeholders
		 * in the *domain* file name template @a velocity_domain_file_name_template.
		 *
		 * For each matching velocity *domain* file, a velocity export file is created using the file name
		 * template @a velocity_export_file_name_template where the local processor number, obtained
		 * from matching @a velocity_domain_processor_place_holder, is used in the exported filename
		 * by replacing @a velocity_export_processor_place_holder with that local processor number.
		 * For example, the *domain* file name "TerraMesh.32.16.5.1" is converted to the *export*
		 * file name "gpt.0001.025" when the domain template is "TerraMesh.%MT.%NT.%ND.%NP" and the
		 * export template is "gpt.%P.025" - note that 4 digits are always used for the
		 * processor number in the *export* file name.
		 *
		 * Each velocity line in the Terra text file, after the header lines, contains:
		 *    velocity_x velocity_y velocity_z
		 *
		 * @a age is the reconstruction time rounded to an integer.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 */
		void
		export_velocity_vector_fields_to_terra_text_format(
				const QString &velocity_domain_file_name_template,
				const QString &velocity_export_file_name_template,
				const QString &velocity_domain_mt_place_holder,
				const QString &velocity_domain_nt_place_holder,
				const QString &velocity_domain_nd_place_holder,
				const QString &velocity_domain_processor_place_holder,
				const QString &velocity_export_processor_place_holder,
				const std::vector<const GPlatesAppLogic::MultiPointVectorField *> &velocity_vector_field_seq,
				const std::vector<const File::Reference *> &active_files,
				int age);




		/**
		 * Exports @a MultiPointVectorField objects containing *velocities* to the CitcomS global file format.
		 *
		 * NOTE: Only velocity vector fields associated with velocity domain (grid) files whose
		 * file names match the template @a velocity_domain_file_name_template are exported.
		 *
		 * For example, a *domain* template of "%D.mesh.%C" will match a velocity field whose
		 * domain came from a file called "33.mesh.9" where '9' is the CitcomS diamond cap number.
		 *
		 * The velocity *domain* template parameters are:
		 *   @a velocity_domain_density_place_holder - used to match the CitcomS diamond resolution,
		 *   @a velocity_domain_cap_number_place_holder - used to match the CitcomS diamond cap number.
		 * And there must be one, and only one, occurrence of @a velocity_domain_cap_number_place_holder
		 * in the *domain* file name template @a velocity_domain_file_name_template.
		 *
		 * For each matching velocity *domain* file, a velocity export file is created using the file name
		 * template @a velocity_export_file_name_template where the cap number, obtained
		 * from matching @a velocity_domain_cap_number_place_holder, is used in the exported filename
		 * by replacing @a velocity_export_cap_number_place_holder with that cap number.
		 * For example, the *domain* file name "33.mesh.9" is converted to the *export* file name
		 * "bvel25.9" when the domain template is "%D.mesh.%C" and the export template is "bvel25.%P".
		 *
		 * Each velocity line in the Terra text file, after the header lines, contains:
		 *    velocity_colat velocity_lon
		 *
		 * @a age is the reconstruction time rounded to an integer.
		 *
		 * If @a include_gmt_export is true then, for each CitcomS velocity file exported, a
		 * CitcomS-compatible GMT format velocity file is exported with the same filename but
		 * with the GMT ".xy" filename extension added.
		 * If @a include_gmt_export is true then, only for the GMT exported files, the
		 * velocity magnitudes are scaled by @a gmt_velocity_scale and only every
		 * 'gmt_velocity_stride'th velocity vector is output.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 */
		void
		export_velocity_vector_fields_to_citcoms_global_format(
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
				unsigned int gmt_velocity_stride);
	}
}

#endif // GPLATES_FILEIO_MULTIPOINTVECTORFIELDEXPORT_H

