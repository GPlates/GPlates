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
				// Output velocities as double precision.
				static const unsigned VELOCITY_PRECISION = 16;
				static const unsigned VELOCITY_FIELDWIDTH = VELOCITY_PRECISION + 3;

				GPlatesMaths::VectorColatitudeLongitude velocity_colat_lon =
						GPlatesMaths::convert_vector_from_xyz_to_colat_lon(domain_point, velocity_vector);

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
			 * Write the velocity vector field.
			 */
			void
			print_citcoms_velocity_vector_field(
					QTextStream &output_stream,
					const GPlatesAppLogic::MultiPointVectorField &velocity_vector_field)
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

					print_citcoms_velocity_line(
							output_stream,
							domain_point,
							velocity_vector);
				}
			}
		}
	}
}


void
GPlatesFileIO::CitcomsFormatVelocityVectorFieldExport::export_global_velocity_vector_fields(
		const std::list<velocity_vector_field_group_type> &velocity_vector_field_group_seq,
		const QFileInfo& file_info,
		int age)
{
	// Open the file.
	QFile output_file(file_info.filePath());
	if ( ! output_file.open(QIODevice::WriteOnly | QIODevice::Text) )
	{
		throw ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
			file_info.filePath());
	}

	QTextStream output_stream(&output_file);

	//
	// Note that there's no header for CitcomS velocity files.
	//

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
			print_citcoms_velocity_vector_field(output_stream, *mpvf);
		}
	}
}
