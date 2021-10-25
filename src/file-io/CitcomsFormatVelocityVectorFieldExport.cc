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

#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <QFile>
#include <QStringList>
#include <QString>
#include <QTextStream>

#include "CitcomsFormatVelocityVectorFieldExport.h"

#include "ErrorOpeningFileForWritingException.h"

#include "app-logic/MultiPointVectorField.h"

#include "file-io/FileInfo.h"

#include "global/GPlatesAssert.h"

#include "maths/CalculateVelocity.h"
#include "maths/MathsUtils.h"

#include "utils/StringFormattingUtils.h"


namespace GPlatesFileIO
{
	namespace CitcomsFormatVelocityVectorFieldExport
	{
		namespace
		{
			//! Convenience typedef for a sequence of MPVFs.
			typedef std::vector<const GPlatesAppLogic::MultiPointVectorField *>
					multi_point_vector_field_seq_type;


			/**
			 * Outputs a velocity line to the CitcomS output consisting of velocity vector.
			 */
			void
			print_citcoms_velocity_line(
					QTextStream &output_stream,
					const GPlatesMaths::PointOnSphere &domain_point,
					const GPlatesMaths::Vector3D &velocity_vector)
			{
				const GPlatesMaths::VectorColatitudeLongitude velocity_colat_lon =
						GPlatesMaths::convert_vector_from_xyz_to_colat_lon(domain_point, velocity_vector);

				// Output velocities as double precision.
				static const unsigned VELOCITY_PRECISION = 16;
				static const unsigned VELOCITY_FIELDWIDTH = VELOCITY_PRECISION + 3;

				/*
				 * Convert the velocity components to strings first, so that in case an exception
				 * is thrown, the output stream is not modified.
				 */
				const std::string velocity_colat_str = GPlatesUtils::formatted_double_to_string(
						velocity_colat_lon.get_vector_colatitude().dval(),
						VELOCITY_FIELDWIDTH,
						VELOCITY_PRECISION);
				const std::string velocity_lon_str = GPlatesUtils::formatted_double_to_string(
						velocity_colat_lon.get_vector_longitude().dval(),
						VELOCITY_FIELDWIDTH,
						VELOCITY_PRECISION);

				output_stream
						<< velocity_colat_str.c_str()
						<< "  "
						<< velocity_lon_str.c_str()
						<< endl;
			}


			/**
			 * Outputs a velocity line to the CitcomS-compatible GMT output consisting of
			 * a domain point as lat/lon and velocity azimuth and magnitude.
			 *
			 * Note the that domain point is lat/lon and not the default GMT format lon/lat.
			 */
			void
			print_citcoms_gmt_velocity_line(
					QTextStream &gmt_output_stream,
					const GPlatesMaths::PointOnSphere &domain_point,
					const GPlatesMaths::Vector3D &velocity_vector)
			{
				/*
				 * A coordinate in the GMT xy format is written as decimal number that
				 * takes up 8 characters excluding sign.
				 */
				static const unsigned GMT_COORDINATE_FIELDWIDTH = 9;

				const GPlatesMaths::LatLonPoint domain_point_lat_lon =
						GPlatesMaths::make_lat_lon_point(domain_point);

				const std::string domain_point_lat_str = GPlatesUtils::formatted_double_to_string(
						domain_point_lat_lon.latitude(),
						GMT_COORDINATE_FIELDWIDTH);
				const std::string domain_point_lon_str = GPlatesUtils::formatted_double_to_string(
						domain_point_lat_lon.longitude(),
						GMT_COORDINATE_FIELDWIDTH);

				// Output velocities as double precision.
				static const unsigned VELOCITY_PRECISION = 16;
				static const unsigned VELOCITY_FIELDWIDTH = VELOCITY_PRECISION + 3;

				const std::pair<GPlatesMaths::real_t/*magnitude*/, GPlatesMaths::real_t/*azimuth*/>
						velocity_magnitude_azimuth =
								GPlatesMaths::calculate_vector_components_magnitude_and_azimuth(
										domain_point,
										velocity_vector);

				/*
				 * Convert the velocity components to strings first, so that in case an exception
				 * is thrown, the output stream is not modified.
				 */
				const std::string velocity_magnitude_str = GPlatesUtils::formatted_double_to_string(
						velocity_magnitude_azimuth.first.dval(),
						VELOCITY_FIELDWIDTH,
						VELOCITY_PRECISION);
				const std::string velocity_azimuth_str = GPlatesUtils::formatted_double_to_string(
						GPlatesMaths::convert_rad_to_deg(velocity_magnitude_azimuth.second.dval()),
						VELOCITY_FIELDWIDTH,
						VELOCITY_PRECISION);

				gmt_output_stream
						<< domain_point_lat_str.c_str()
						<< "  "
						<< domain_point_lon_str.c_str()
						<< "  "
						<< velocity_azimuth_str.c_str()
						<< "  "
						<< velocity_magnitude_str.c_str()
						<< endl;
			}


			/**
			 * Write the velocity vector field.
			 */
			void
			print_citcoms_velocity_vector_field(
					QTextStream &output_stream,
					boost::optional<QTextStream> &gmt_output_stream,
					const GPlatesAppLogic::MultiPointVectorField &velocity_vector_field,
					double gmt_velocity_scale,
					unsigned int &velocity_vector_index,
					unsigned int gmt_velocity_stride)
			{
				GPlatesMaths::MultiPointOnSphere::const_iterator domain_iter =
						velocity_vector_field.multi_point()->begin();
				GPlatesMaths::MultiPointOnSphere::const_iterator domain_end =
						velocity_vector_field.multi_point()->end();
				GPlatesAppLogic::MultiPointVectorField::codomain_type::const_iterator codomain_iter =
						velocity_vector_field.begin();
				for ( ; domain_iter != domain_end; ++domain_iter, ++codomain_iter)
				{
					const GPlatesMaths::PointOnSphere &domain_point = *domain_iter;

					// If the current codomain is invalid/null then default to zero velocity.
					GPlatesMaths::Vector3D velocity_vector(0, 0, 0);

					// Set the velocity if we have a valid/non-null codomain.
					if (*codomain_iter)
					{
						const GPlatesAppLogic::MultiPointVectorField::CodomainElement &codomain = codomain_iter->get();
						velocity_vector = codomain.d_vector;
					}

					// Print to the CitcomS file.
					print_citcoms_velocity_line(
							output_stream,
							domain_point,
							velocity_vector);

					// Print to the CitcomS-compatible GMT file, if requested.
					if (gmt_output_stream)
					{
						// Only output every 'n'th velocity vector.
						if ((velocity_vector_index % gmt_velocity_stride) == 0)
						{
							print_citcoms_gmt_velocity_line(
									gmt_output_stream.get(),
									domain_point,
									gmt_velocity_scale * velocity_vector);
						}
					}

					++velocity_vector_index;
				}
			}
		}
	}
}


void
GPlatesFileIO::CitcomsFormatVelocityVectorFieldExport::export_global_velocity_vector_fields(
		const std::list<velocity_vector_field_group_type> &velocity_vector_field_group_seq,
		const QFileInfo& file_info,
		int age,
		bool include_gmt_export,
		double gmt_velocity_scale,
		unsigned int gmt_velocity_stride)
{
	// Open the CitcomS file.
	QFile output_file(file_info.filePath());
	if ( ! output_file.open(QIODevice::WriteOnly | QIODevice::Text) )
	{
		throw ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE, file_info.filePath());
	}
	QTextStream output_stream(&output_file);

	// Open the CitcomS-compatible GMT file, if requested.
	boost::optional<QFile> gmt_output_file;
	boost::optional<QTextStream> gmt_output_stream;
	if (include_gmt_export)
	{
		const QString gmt_filename = file_info.filePath() + ".xy";
		// Constructs using single-argument QFile constructor...
		gmt_output_file = boost::in_place(gmt_filename);
		if ( ! gmt_output_file->open(QIODevice::WriteOnly | QIODevice::Text) )
		{
			throw ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE, gmt_filename);
		}
		// Constructs using single-argument QTextStream constructor...
		gmt_output_stream = boost::in_place(&gmt_output_file.get());
	}

	//
	// Note that there's no header for CitcomS velocity files.
	//

	// Keep track of the number of velocity vectors encountered.
	// This is needed for the velocity stride so we only output every 'n'th velocity vector.
	unsigned int velocity_vector_index = 0;

	// Iterate through the vector fields and write to output.
	std::list<velocity_vector_field_group_type>::const_iterator feature_iter;
	for (feature_iter = velocity_vector_field_group_seq.begin();
		feature_iter != velocity_vector_field_group_seq.end();
		++feature_iter)
	{
		const velocity_vector_field_group_type &feature_vector_field_group = *feature_iter;

		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref =
				feature_vector_field_group.feature_ref;
		if (!feature_ref.is_valid())
		{
			qWarning() << "Invalid feature reference during CitcomS global velocity export - ignoring feature.";
			continue;
		}

		// Iterate through the vector fields of the current feature and write to output.
		multi_point_vector_field_seq_type::const_iterator mpvf_iter;
		for (mpvf_iter = feature_vector_field_group.recon_geoms.begin();
			mpvf_iter != feature_vector_field_group.recon_geoms.end();
			++mpvf_iter)
		{
			const GPlatesAppLogic::MultiPointVectorField *mpvf = *mpvf_iter;

			// Write the velocity vector field.
			print_citcoms_velocity_vector_field(
					output_stream,
					gmt_output_stream,
					*mpvf,
					gmt_velocity_scale,
					velocity_vector_index,
					gmt_velocity_stride);
		}
	}
}
