/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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
#include <QFile>
#include <QStringList>
#include <QString>
#include <QTextStream>

#include "GMTFormatMultiPointVectorFieldExport.h"

#include "ErrorOpeningFileForWritingException.h"
#include "GMTFormatHeader.h"

#include "app-logic/MultiPointVectorField.h"

#include "file-io/FileInfo.h"

#include "global/GPlatesAssert.h"

#include "maths/CalculateVelocity.h"
#include "maths/MathsUtils.h"

#include "utils/StringFormattingUtils.h"


namespace GPlatesFileIO
{
	namespace GMTFormatMultiPointVectorFieldExport
	{
		namespace
		{
			//! Convenience typedef for a sequence of MPVFs.
			typedef std::vector<const GPlatesAppLogic::MultiPointVectorField *>
					multi_point_vector_field_seq_type;


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
			 * Outputs a velocity line to the GMT output consisting of velocity and optionally position and plate id.
			 */
			void
			print_gmt_velocity_line(
					QTextStream &output_stream,
					const GPlatesMaths::PointOnSphere &domain_point,
					const GPlatesMaths::Vector3D &velocity_vector,
					GPlatesModel::integer_plate_id_type plate_id,
					MultiPointVectorFieldExport::VelocityVectorFormatType velocity_vector_format,
					bool domain_point_lon_lat_format,
					bool include_plate_id,
					bool include_domain_point)
			{
				/*
				 * Write the complete line to a string stream first, so that in case an exception
				 * is thrown, the output stream is not modified.
				 */
				std::ostringstream gmt_line;

				const GPlatesMaths::LatLonPoint domain_point_lat_lon =
						GPlatesMaths::make_lat_lon_point(domain_point);

				//
				// Output domain point.
				//

				if (include_domain_point)
				{
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
						gmt_line << "  " << domain_point_lon_str << "      " << domain_point_lat_str;
					}
					else
					{
						gmt_line << "  " << domain_point_lat_str << "      " << domain_point_lon_str;
					}
				}

				//
				// Output plate id.
				//

				if (include_plate_id)
				{
					// Use a minimum width of 5 since 5-digit plate ids are currently in use.
					static const unsigned PLATE_ID_FIELDWIDTH = 5;

					const std::string plate_id_str = GPlatesUtils::formatted_int_to_string(
							plate_id,
							PLATE_ID_FIELDWIDTH);

					gmt_line << "      " << plate_id_str;
				}

				//
				// Output velocity.
				//

				// Output velocities as double precision.
				static const unsigned VELOCITY_PRECISION = 16;
				static const unsigned VELOCITY_FIELDWIDTH = VELOCITY_PRECISION + 3;

				switch (velocity_vector_format)
				{
				case MultiPointVectorFieldExport::VELOCITY_VECTOR_3D:
					{
						const std::string velocity_x_str = GPlatesUtils::formatted_double_to_string(
								velocity_vector.x().dval(),
								VELOCITY_FIELDWIDTH,
								VELOCITY_PRECISION);
						const std::string velocity_y_str = GPlatesUtils::formatted_double_to_string(
								velocity_vector.y().dval(),
								VELOCITY_FIELDWIDTH,
								VELOCITY_PRECISION);
						const std::string velocity_z_str = GPlatesUtils::formatted_double_to_string(
								velocity_vector.z().dval(),
								VELOCITY_FIELDWIDTH,
								VELOCITY_PRECISION);

						gmt_line << "      " << velocity_x_str << "      " << velocity_y_str << "      " << velocity_z_str;
					}
					break;

				case MultiPointVectorFieldExport::VELOCITY_VECTOR_COLAT_LON:
					{
						GPlatesMaths::VectorColatitudeLongitude velocity_colat_lon =
								GPlatesMaths::convert_vector_from_xyz_to_colat_lon(domain_point, velocity_vector);

						const std::string velocity_colat_str = GPlatesUtils::formatted_double_to_string(
								velocity_colat_lon.get_vector_colatitude().dval(),
								VELOCITY_FIELDWIDTH,
								VELOCITY_PRECISION);
						const std::string velocity_lon_str = GPlatesUtils::formatted_double_to_string(
								velocity_colat_lon.get_vector_longitude().dval(),
								VELOCITY_FIELDWIDTH,
								VELOCITY_PRECISION);

						gmt_line << "      " << velocity_colat_str << "      " << velocity_lon_str;
					}
					break;

				case MultiPointVectorFieldExport::VELOCITY_VECTOR_MAGNITUDE_ANGLE:
					{
						std::pair<GPlatesMaths::real_t, GPlatesMaths::real_t> velocity_magnitude_angle =
								GPlatesMaths::calculate_vector_components_magnitude_angle(domain_point, velocity_vector);

						const std::string velocity_magnitude_str = GPlatesUtils::formatted_double_to_string(
								velocity_magnitude_angle.first.dval(),
								VELOCITY_FIELDWIDTH,
								VELOCITY_PRECISION);
						const std::string velocity_angle_degrees_str = GPlatesUtils::formatted_double_to_string(
							GPlatesMaths::convert_rad_to_deg(velocity_magnitude_angle.second.dval()),
								VELOCITY_FIELDWIDTH,
								VELOCITY_PRECISION);

						gmt_line << "      " << velocity_magnitude_str << "      " << velocity_angle_degrees_str;
					}
					break;

				case MultiPointVectorFieldExport::VELOCITY_VECTOR_MAGNITUDE_AZIMUTH:
					{
						std::pair<GPlatesMaths::real_t, GPlatesMaths::real_t> velocity_magnitude_azimuth =
								GPlatesMaths::calculate_vector_components_magnitude_and_azimuth(domain_point, velocity_vector);

						const std::string velocity_magnitude_str = GPlatesUtils::formatted_double_to_string(
								velocity_magnitude_azimuth.first.dval(),
								VELOCITY_FIELDWIDTH,
								VELOCITY_PRECISION);
						const std::string velocity_azimuth_degrees_str = GPlatesUtils::formatted_double_to_string(
							GPlatesMaths::convert_rad_to_deg(velocity_magnitude_azimuth.second.dval()),
								VELOCITY_FIELDWIDTH,
								VELOCITY_PRECISION);

						gmt_line << "      " << velocity_magnitude_str << "      " << velocity_azimuth_degrees_str;
					}
					break;

				default:
					// Shouldn't get here.
					GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
					break;
				}

				//
				// Output the final line.
				//

				const std::string gmt_line_string = gmt_line.str();
				output_stream << gmt_line_string.c_str() << endl;
			}


			/**
			 * Write the velocity vector field and optionally its domain positions and plate ids.
			 */
			void
			print_gmt_velocity_vector_field(
					QTextStream &output_stream,
					const GPlatesAppLogic::MultiPointVectorField &velocity_vector_field,
					MultiPointVectorFieldExport::VelocityVectorFormatType velocity_vector_format,
					double velocity_scale,
					unsigned int &velocity_vector_index,
					unsigned int velocity_stride,
					bool domain_point_lon_lat_format,
					bool include_plate_id,
					bool include_domain_point)
			{
				GPlatesMaths::MultiPointOnSphere::const_iterator domain_iter =
						velocity_vector_field.multi_point()->begin();
				GPlatesMaths::MultiPointOnSphere::const_iterator domain_end =
						velocity_vector_field.multi_point()->end();
				GPlatesAppLogic::MultiPointVectorField::codomain_type::const_iterator codomain_iter =
						velocity_vector_field.begin();
				for ( ; domain_iter != domain_end; ++domain_iter, ++codomain_iter)
				{
					// Only output every 'n'th velocity vector.
					if ((velocity_vector_index++ % velocity_stride) != 0)
					{
						continue;
					}

					const GPlatesMaths::PointOnSphere &domain_point = *domain_iter;

					// If the current codomain is invalid/null then default to zero velocity and plate id.
					GPlatesMaths::Vector3D velocity_vector(0, 0, 0);
					GPlatesModel::integer_plate_id_type plate_id = 0;

					// Set the velocity and plate id if we have a valid/non-null codomain.
					if (*codomain_iter)
					{
						const GPlatesAppLogic::MultiPointVectorField::CodomainElement &codomain = codomain_iter->get();

						velocity_vector = codomain.d_vector;
						if (codomain.d_plate_id)
						{
							plate_id = codomain.d_plate_id.get();
						}
					}

					print_gmt_velocity_line(
							output_stream,
							domain_point,
							velocity_scale * velocity_vector,
							plate_id,
							velocity_vector_format,
							domain_point_lon_lat_format,
							include_plate_id,
							include_domain_point);
				}
			}
		}
	}
}


void
GPlatesFileIO::GMTFormatMultiPointVectorFieldExport::export_velocity_vector_fields(
		const std::list<multi_point_vector_field_group_type> &velocity_vector_field_group_seq,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		MultiPointVectorFieldExport::VelocityVectorFormatType velocity_vector_format,
		double velocity_scale,
		unsigned int velocity_stride,
		bool domain_point_lon_lat_format,
		bool include_plate_id,
		bool include_domain_point,
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

	// Even though we're printing out vector fields rather than present day geometry we still
	// write out the verbose properties of the feature.
	GMTFormatVerboseHeader gmt_header;

	// Keep track of the number of velocity vectors encountered.
	// This is needed for the velocity stride so we only output every 'n'th velocity vector.
	unsigned int velocity_vector_index = 0;

	// Iterate through the vector fields and write to output.
	std::list<multi_point_vector_field_group_type>::const_iterator feature_iter;
	for (feature_iter = velocity_vector_field_group_seq.begin();
		feature_iter != velocity_vector_field_group_seq.end();
		++feature_iter)
	{
		const multi_point_vector_field_group_type &feature_vector_field_group = *feature_iter;

		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref =
				feature_vector_field_group.feature_ref;
		if (!feature_ref.is_valid())
		{
			continue;
		}

		// Get the header lines.
		std::vector<QString> header_lines;
		gmt_header.get_feature_header_lines(feature_ref, header_lines);

		// Iterate through the vector fields of the current feature and write to output.
		multi_point_vector_field_seq_type::const_iterator mpvf_iter;
		for (mpvf_iter = feature_vector_field_group.recon_geoms.begin();
			mpvf_iter != feature_vector_field_group.recon_geoms.end();
			++mpvf_iter)
		{
			const GPlatesAppLogic::MultiPointVectorField *mpvf = *mpvf_iter;

			if (include_domain_meta_data)
			{
				// Print the header lines.
				gmt_header_printer.print_feature_header_lines(output_stream, header_lines);
			}

			// Write the velocity vector field and its domain positions and plate ids.
			print_gmt_velocity_vector_field(
					output_stream,
					*mpvf,
					velocity_vector_format,
					velocity_scale,
					velocity_vector_index,
					velocity_stride,
					domain_point_lon_lat_format,
					include_plate_id,
					include_domain_point);

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
