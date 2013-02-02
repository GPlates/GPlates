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
	class Gpgim;
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
		 * How to write out each velocity vector.
		 *
		 * Note that this is not used by all velocity formats.
		 */
		enum VelocityVectorFormatType
		{
			VELOCITY_VECTOR_3D,
			VELOCITY_VECTOR_COLAT_LON,
			VELOCITY_VECTOR_MAGNITUDE_ANGLE
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
				const GPlatesModel::Gpgim &gpgim,
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
		 *    domain_point plate_id velocity
		 * ...where 'domain_point' is position at which the velocity was calculated and 'plate_id'
		 * is the plate id used to calculate the velocity (for topological networks the plate id
		 * only identifies the network used to calculate the velocity).
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
				VelocityVectorFormatType velocity_vector_format,
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
	}
}

#endif // GPLATES_FILEIO_MULTIPOINTVECTORFIELDEXPORT_H

