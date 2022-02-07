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

#ifndef GPLATES_FILE_IO_DEFORMATIONEXPORT_H
#define GPLATES_FILE_IO_DEFORMATIONEXPORT_H

#include <vector>
#include <boost/optional.hpp>
#include <QFileInfo>
#include <QString>

#include "app-logic/DeformationStrain.h"

#include "file-io/File.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class TopologyReconstructedFeatureGeometry;
}

namespace GPlatesModel
{
	class ModelInterface;
}

namespace GPlatesFileIO
{
	namespace DeformationExport
	{
		//
		// Export deformation.
		//


		/**
		 * Options for exporting principal strain/stretch.
		 */
		struct PrincipalStrainOptions
		{
			enum OutputType
			{
				STRAIN,  //!< Extension is positive; compression is negative.
				STRETCH  //!< Is positive (1.0 + strain); can plot as ellipse.
			};

			enum FormatType
			{
				//! MajorAngle (-180 to +180 degrees anti-clockwise from West; 0 is East) / MajorAxis / MinorAxis.
				ANGLE_MAJOR_MINOR,
				//! MajorAzimuth (0 to 360 degrees clockwise from North; 0 is North) / MajorAxis / MinorAxis.
				AZIMUTH_MAJOR_MINOR
			};


			PrincipalStrainOptions(
					OutputType output_,
					FormatType format_);

			/**
			 * Returns the angle or azimuth from the specified principal strain.
			 */
			double
			get_principal_angle_or_azimuth_in_degrees(
					const GPlatesAppLogic::DeformationStrain::StrainPrincipal &principal_strain) const;


			OutputType output;
			FormatType format;
		};


		/**
		 * Exports @a TopologyReconstructedFeatureGeometry objects containing deformation information to the GPML file format.
		 *
		 * If @a include_principal_strain is specified then 3 extra sets of per-point scalars are exported:
		 * - 'gpml:PrincipalStrainMajorAngle/Azimuth' or 'PrincipalStretchMajorAngle/Azimuth' is the angle or azimuth (in degrees) of major principal axis.
		 * - 'gpml:PrincipalStrainMajorAxis' or 'PrincipalStretchMajorAxis' is largest principal strain or stretch (1+strain), both unitless.
		 * - 'gpml:PrincipalStrainMinorAxis' or 'PrincipalStretchMinorAxis' is smallest principal strain or stretch (1+strain), both unitless.
		 *
		 * If @a include_dilatation_strain is true then an extra set of per-point scalars,
		 * under 'gpml:DilatationStrain', is exported as per-point dilatation strain (unitless).
		 *
		 * If @a include_dilatation_strain_rate is true then an extra set of per-point scalars,
		 * under 'gpml:DilatationStrainRate', is exported as per-point dilatation strain rates (in units of 1/second).
		 *
		 * If @a include_second_invariant_strain_rate is true then an extra set of per-point scalars,
		 * under 'gpml:TotalStrainRate', is exported as per-point second invariant strain rates (in units of 1/second).
		 *
		 * @param export_single_output_file specifies whether to write all deformed feature geometries to a single file.
		 * @param export_per_input_file specifies whether to group deformed feature geometries according
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
		export_deformation_to_gpml_format(
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
				bool export_separate_output_directory_per_input_file);


		/**
		 * Exports @a TopologyReconstructedFeatureGeometry objects containing deformation information to the GMT file format.
		 *
		 * Note that GMT format provides a choice of how to output each topology reconstructed feature geometry.
		 *
		 * Each line in the GMT file contains:
		 * 
		 *    domain_point [principal_<strain|stretch>_major_<angle|azimuth> principal_<strain|stretch>_major_axis principal_<strain|stretch>_minor_axis]
		 *                 [dilatation_strain] [dilatation_strain_rate] [second_invariant_strain_rate]
		 * 
		 * ...where 'domain_point' is position associated with the deformation information.
		 * If @a include_principal_strain is specified then principal strain/stretch (angle/azimuth, major axis, minor axis)
		 * are output (where strain/stretch is unitless and angle/azimuth is in degrees).
		 * If @a include_dilatation_strain is true then dilatation strain is output (unitless).
		 * If @a include_dilatation_strain_rate is true then dilatation strain rate is output (in units of 1/second).
		 * If @a include_second_invariant_strain_rate is true second invariant strain rate is output (in units of 1/second).
		 *
		 * If @a domain_point_lon_lat_format is true then the domain points are output as the
		 * GMT default of (longitude latitude), otherwise they're output as (latitude longitude).
		 *
		 * @param export_single_output_file specifies whether to write all deformed feature geometries to a single file.
		 * @param export_per_input_file specifies whether to group deformed feature geometries according
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
		export_deformation_to_gmt_format(
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
				bool export_separate_output_directory_per_input_file);
	}
}

#endif // GPLATES_FILE_IO_DEFORMATIONEXPORT_H
