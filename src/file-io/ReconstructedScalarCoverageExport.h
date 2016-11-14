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

#ifndef GPLATES_FILE_IO_RECONSTRUCTEDSCALARCOVERAGEEXPORT_H
#define GPLATES_FILE_IO_RECONSTRUCTEDSCALARCOVERAGEEXPORT_H

#include <vector>
#include <QFileInfo>
#include <QString>

#include "file-io/File.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class ReconstructedScalarCoverage;
}

namespace GPlatesModel
{
	class ModelInterface;
}

namespace GPlatesFileIO
{
	namespace ReconstructedScalarCoverageExport
	{
		//
		// Export reconstructed scalar coverages.
		//


		/**
		 * Exports @a ReconstructedScalarCoverage objects containing *scalar coverages* to the GPML file format.
		 *
		 * If @a include_dilatation_rate is true then an extra set of per-point scalars,
		 * under 'gpml:DilatationRate', is exported as per-point dilatation rates (in units of 1/second).
		 *
		 * If @a include_dilatation  is true then an extra set of per-point scalars,
		 * under 'gpml:Dilatation', is exported as per-point accumulated dilatation (unit-less).
		 *
		 * @param export_single_output_file specifies whether to write all reconstructed scalar coverages to a single file.
		 * @param export_per_input_file specifies whether to group reconstructed scalar coverages according
		 *        to the input files their features came from and write to corresponding output files.
		 * @param export_separate_output_directory_per_input_file
		 *        Save each exported file to a different directory based on the file basename.
		 *        Only applies if @a export_per_input_file is 'true'.
		 * @param include_dilatation_rate if true then an extra set of per-point scalars,
		 *        under 'gpml:DilatationRate', is exported as per-point dilatation rates (in units of 1/second).
		 *
		 * Note that both @a export_single_output_file and @a export_per_input_file can be true
		 * in which case both a single output file is exported as well as grouped output files.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 */
		void
		export_reconstructed_scalar_coverages_to_gpml_format(
				const QString &filename,
				const std::vector<const GPlatesAppLogic::ReconstructedScalarCoverage *> &reconstructed_scalar_coverage_seq,
				GPlatesModel::ModelInterface &model,
				const std::vector<const File::Reference *> &active_files,
				bool include_dilatation_rate,
				bool include_dilatation,
				bool export_single_output_file,
				bool export_per_input_file,
				bool export_separate_output_directory_per_input_file);


		/**
		 * Exports @a ReconstructedScalarCoverage objects containing *scalar coverages* to the GMT file format.
		 *
		 * Note that GMT format provides a choice of how to output each reconstructed scalar coverage.
		 *
		 * Each line in the GMT file contains:
		 * 
		 *    domain_point [dilatation_rate] [dilatation] scalar
		 * 
		 * ...where 'domain_point' is position associated with the dilatation rate.
		 * If @a include_dilatation_rate is true then dilatation rate is output (in units of 1/second).
		 * If @a include_dilatation is true then accumulated dilatation is output (unit-less).
		 *
		 * If @a domain_point_lon_lat_format is true then the domain points are output as the
		 * GMT default of (longitude latitude), otherwise they're output as (latitude longitude).
		 *
		 * @param export_single_output_file specifies whether to write all reconstructed scalar coverages to a single file.
		 * @param export_per_input_file specifies whether to group reconstructed scalar coverages according
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
		export_reconstructed_scalar_coverages_to_gmt_format(
				const QString &filename,
				const std::vector<const GPlatesAppLogic::ReconstructedScalarCoverage *> &reconstructed_scalar_coverage_seq,
				const std::vector<const File::Reference *> &active_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool domain_point_lon_lat_format,
				bool include_dilatation_rate,
				bool include_dilatation,
				bool export_single_output_file,
				bool export_per_input_file,
				bool export_separate_output_directory_per_input_file);
	}
}

#endif // GPLATES_FILE_IO_RECONSTRUCTEDSCALARCOVERAGEEXPORT_H
