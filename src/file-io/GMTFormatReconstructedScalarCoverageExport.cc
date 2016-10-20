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

#include <iomanip>
#include <sstream>
#include <boost/optional.hpp>
#include <QFile>
#include <QStringList>
#include <QString>
#include <QTextStream>

#include "GMTFormatReconstructedScalarCoverageExport.h"

#include "ErrorOpeningFileForWritingException.h"
#include "GMTFormatHeader.h"

#include "app-logic/DeformedFeatureGeometry.h"
#include "app-logic/ReconstructedScalarCoverage.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include "file-io/FileInfo.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/MathsUtils.h"

#include "utils/StringFormattingUtils.h"


namespace GPlatesFileIO
{
	namespace GMTFormatReconstructedScalarCoverageExport
	{
		namespace
		{
			//! Convenience typedef for a sequence of reconstructed scalar coverages.
			typedef std::vector<const GPlatesAppLogic::ReconstructedScalarCoverage *>
					reconstructed_scalar_coverage_seq_type;


			/**
			 * Prints GMT format header at top of the exported file containing information
			 * about the reconstruction that is not per-feature information.
			 */
			void
			get_global_header_lines(
					std::vector<QString>& header_lines,
					const referenced_files_collection_type &referenced_files,
					const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
					const double &reconstruction_time)
			{
				// Print the anchor plate id.
				header_lines.push_back(
						QString("anchorPlateId ") + QString::number(reconstruction_anchor_plate_id));

				// Print the reconstruction time.
				header_lines.push_back(
						QString("reconstructionTime ") + QString::number(reconstruction_time));

				GMTFormatHeader::add_filenames_to_header(header_lines, referenced_files);
			}


			/**
			 * Outputs a scalar coverage line to the GMT output consisting of scalar and optionally position.
			 */
			void
			print_gmt_scalar_coverage_line(
					QTextStream &output_stream,
					boost::optional<const GPlatesMaths::PointOnSphere &> domain_point,
					boost::optional<const double &> dilatation_rate,
					const double &scalar_value,
					bool domain_point_lon_lat_format)
			{
				/*
				 * Write the complete line to a string stream first, so that in case an exception
				 * is thrown, the output stream is not modified.
				 */
				std::ostringstream gmt_line;

				//
				// Output domain point.
				//

				if (domain_point)
				{
					const GPlatesMaths::LatLonPoint domain_point_lat_lon =
							GPlatesMaths::make_lat_lon_point(domain_point.get());

					/*
					 * A coordinate in the GMT xy format is written as decimal number that
					 * takes up 8 characters excluding sign.
					 */
					static const unsigned GMT_COORDINATE_FIELDWIDTH = 9;

					const std::string domain_point_lat_str = GPlatesUtils::formatted_double_to_string(
							domain_point_lat_lon.latitude(),
							GMT_COORDINATE_FIELDWIDTH);
					const std::string domain_point_lon_str = GPlatesUtils::formatted_double_to_string(
							domain_point_lat_lon.longitude(),
							GMT_COORDINATE_FIELDWIDTH);

					// GMT format is by default (lon,lat) which is opposite of PLATES4 line format.
					if (domain_point_lon_lat_format)
					{
						gmt_line << " " << domain_point_lon_str << " " << domain_point_lat_str;
					}
					else
					{
						gmt_line << " " << domain_point_lat_str << " " << domain_point_lon_str;
					}
				}

				// Output scalars as double precision.
				static const unsigned SCALAR_PRECISION = 16;
				static const unsigned SCALAR_FIELDWIDTH = SCALAR_PRECISION + 3;

				//
				// Output dilatation rate.
				//

				if (dilatation_rate)
				{
					// Don't format as fixed notation.
					gmt_line << " "
							<< std::setw(SCALAR_FIELDWIDTH) << std::scientific << std::setprecision(SCALAR_PRECISION)
							<< dilatation_rate.get();
				}

				//
				// Output scalar value.
				//

				// Don't format as fixed notation.
				gmt_line << " "
						<< std::setw(SCALAR_FIELDWIDTH) << std::scientific << std::setprecision(SCALAR_PRECISION)
						<< scalar_value;

				//
				// Output the final line.
				//

				const std::string gmt_line_string = gmt_line.str();
				output_stream << gmt_line_string.c_str() << endl;
			}


			/**
			 * Write the scalar coverage and optionally its domain positions.
			 */
			void
			print_gmt_scalar_coverage(
					QTextStream &output_stream,
					const GPlatesAppLogic::ReconstructedScalarCoverage &reconstructed_scalar_coverage,
					bool domain_point_lon_lat_format,
					bool include_domain_point,
					bool include_dilatation_rate)
			{
				GPlatesAppLogic::ReconstructedScalarCoverage::point_seq_type reconstructed_domain_points;
				reconstructed_scalar_coverage.get_reconstructed_points(reconstructed_domain_points);

				const GPlatesAppLogic::ReconstructedScalarCoverage::point_scalar_value_seq_type &scalar_values =
						reconstructed_scalar_coverage.get_reconstructed_point_scalar_values();

				// The number of domain points should match the number of scalars.
				// The ReconstructedScalarCoverage interface guarantees this.
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						reconstructed_domain_points.size() == scalar_values.size(),
						GPLATES_ASSERTION_SOURCE);

				boost::optional< std::vector<double> > dilatation_rates;
				if (include_dilatation_rate)
				{
					dilatation_rates = std::vector<double>();

					boost::optional<const GPlatesAppLogic::DeformedFeatureGeometry *> dfg =
							GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
									const GPlatesAppLogic::DeformedFeatureGeometry *>(
											reconstructed_scalar_coverage.get_reconstructed_domain_geometry().get());
					if (dfg)
					{
						// Get the current (per-point) deformation strain rates.
						const GPlatesAppLogic::DeformedFeatureGeometry::point_deformation_strain_rate_seq_type &
								deformation_strain_rates = dfg.get()->get_point_deformation_strain_rates();

						// The number of strain rates should match the number of scalars.
						GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
								deformation_strain_rates.size() == scalar_values.size(),
								GPLATES_ASSERTION_SOURCE);
						dilatation_rates->reserve(deformation_strain_rates.size());
						for (unsigned int n = 0; n < deformation_strain_rates.size(); ++n)
						{
							dilatation_rates->push_back(deformation_strain_rates[n].get_dilatation());
						}
					}
					else
					{
						// The RFG is not a DeformedFeatureGeometry so we have no deformation strain information.
						// Default to zero strain.
						dilatation_rates->resize(scalar_values.size(), 0.0);
					}
				}

				GPlatesAppLogic::ReconstructedScalarCoverage::point_seq_type::const_iterator
						domain_iter = reconstructed_domain_points.begin();
				GPlatesAppLogic::ReconstructedScalarCoverage::point_seq_type::const_iterator
						domain_end = reconstructed_domain_points.end();
				GPlatesAppLogic::ReconstructedScalarCoverage::point_scalar_value_seq_type::const_iterator
						range_iter = scalar_values.begin();
				for (unsigned int index = 0; domain_iter != domain_end; ++domain_iter, ++range_iter, ++index)
				{
					boost::optional<const GPlatesMaths::PointOnSphere &> domain_point;
					if (include_domain_point)
					{
						domain_point = *domain_iter;
					}

					boost::optional<const double &> dilatation_rate;
					if (include_dilatation_rate)
					{
						dilatation_rate = dilatation_rates.get()[index];
					}

					const double &scalar_value = *range_iter;

					print_gmt_scalar_coverage_line(
							output_stream,
							domain_point,
							dilatation_rate,
							scalar_value,
							domain_point_lon_lat_format);
				}
			}
		}
	}
}


void
GPlatesFileIO::GMTFormatReconstructedScalarCoverageExport::export_reconstructed_scalar_coverages(
		const std::list<reconstructed_scalar_coverage_group_type> &reconstructed_scalar_coverage_group_seq,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool domain_point_lon_lat_format,
		bool include_domain_point,
		bool include_dilatation_rate,
		bool include_domain_meta_data)
{
	// Open the file.
	QFile output_file(file_info.filePath());
	if ( ! output_file.open(QIODevice::WriteOnly | QIODevice::Text) )
	{
		throw ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
			file_info.filePath());
	}

	QTextStream output_stream(&output_file);

	// Does the actual printing of GMT header to the output stream.
	GMTHeaderPrinter gmt_header_printer;

	if (include_domain_meta_data)
	{
		// Write out the global header (at the top of the exported file).
		std::vector<QString> global_header_lines;
		get_global_header_lines(global_header_lines,
				referenced_files, reconstruction_anchor_plate_id, reconstruction_time);
		gmt_header_printer.print_global_header_lines(output_stream, global_header_lines);
	}

	// Even though we're printing out reconstructed scalar coverages rather than present day geometry
	// we still write out the verbose properties of the feature.
	GMTFormatVerboseHeader gmt_header;

	// Iterate through the vector fields and write to output.
	std::list<reconstructed_scalar_coverage_group_type>::const_iterator feature_iter;
	for (feature_iter = reconstructed_scalar_coverage_group_seq.begin();
		feature_iter != reconstructed_scalar_coverage_group_seq.end();
		++feature_iter)
	{
		const reconstructed_scalar_coverage_group_type &feature_scalar_coverage_group = *feature_iter;

		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref = feature_scalar_coverage_group.feature_ref;
		if (!feature_ref.is_valid())
		{
			continue;
		}

		// Get the header lines.
		std::vector<QString> header_lines;
		gmt_header.get_feature_header_lines(feature_ref, header_lines);

		// Iterate through the reconstructed scalar coverages of the current feature and write to output.
		reconstructed_scalar_coverage_seq_type::const_iterator rsc_iter;
		for (rsc_iter = feature_scalar_coverage_group.recon_geoms.begin();
			rsc_iter != feature_scalar_coverage_group.recon_geoms.end();
			++rsc_iter)
		{
			const GPlatesAppLogic::ReconstructedScalarCoverage *rsc = *rsc_iter;

			if (include_domain_meta_data)
			{
				// Print the header lines.
				gmt_header_printer.print_feature_header_lines(output_stream, header_lines);
			}

			// Write the scalar coverage and its domain positions.
			print_gmt_scalar_coverage(
					output_stream,
					*rsc,
					domain_point_lon_lat_format,
					include_domain_point,
					include_dilatation_rate);

			if (include_domain_meta_data)
			{
				// Write the final terminating symbol for the current feature.
				//
				// No newline is output since a GMT header may follow (due to the next feature) in which
				// case it will use the same line.
				output_stream << ">";
			}
		}
	}
}
