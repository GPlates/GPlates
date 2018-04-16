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

#include "GMTFormatDeformationExport.h"

#include "ErrorOpeningFileForWritingException.h"
#include "GMTFormatHeader.h"

#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/TopologyReconstructedFeatureGeometry.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "file-io/FileInfo.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/MathsUtils.h"

#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsString.h"

#include "utils/StringFormattingUtils.h"


namespace GPlatesFileIO
{
	namespace GMTFormatDeformationExport
	{
		namespace
		{
			//! Convenience typedef for a sequence of deformed feature geometries.
			typedef std::vector<const GPlatesAppLogic::TopologyReconstructedFeatureGeometry *> deformed_feature_geometry_seq_type;


			/**
			 * Outputs a deformation line to the GMT output consisting of position and optional dilatation strain rate and strain.
			 */
			void
			print_gmt_deformation_line(
					QTextStream &output_stream,
					const GPlatesMaths::PointOnSphere &domain_point,
					bool domain_point_lon_lat_format,
					boost::optional<const double &> dilatation_rate,
					boost::optional<const double &> dilatation)
			{
				/*
				 * Write the complete line to a string stream first, so that in case an exception
				 * is thrown, the output stream is not modified.
				 */
				std::ostringstream gmt_line;

				//
				// Output domain point.
				//

				const GPlatesMaths::LatLonPoint domain_point_lat_lon =
						GPlatesMaths::make_lat_lon_point(domain_point);

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
				// Output accumulated dilatation.
				//

				if (dilatation)
				{
					// Don't format as fixed notation.
					gmt_line << " "
							<< std::setw(SCALAR_FIELDWIDTH) << std::scientific << std::setprecision(SCALAR_PRECISION)
							<< dilatation.get();
				}

				//
				// Output the final line.
				//

				const std::string gmt_line_string = gmt_line.str();
				output_stream << gmt_line_string.c_str() << endl;
			}


			/**
			 * Write the deformed feature geometry (positions and dilatation strain rates).
			 */
			void
			print_gmt_deformed_feature_geometry(
					QTextStream &output_stream,
					const GPlatesAppLogic::TopologyReconstructedFeatureGeometry &deformed_feature_geometry,
					bool domain_point_lon_lat_format,
					bool include_dilatation_rate,
					bool include_dilatation)
			{
				//
				// Print the feature header.
				//

				// Print the feature name.
				boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type> gml_name =
						GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::XsString>(
								deformed_feature_geometry.get_feature_ref(),
								GPlatesModel::PropertyName::create_gml("name"));
				if (gml_name)
				{
					output_stream << "> Name=" << gml_name.get()->get_value().get().qstring() << "\n";
				}
				else
				{
					output_stream << "> Name=\n";
				}

				// Print the feature's time period.
				boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> gml_valid_time =
						GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GmlTimePeriod>(
								deformed_feature_geometry.get_feature_ref(),
								GPlatesModel::PropertyName::create_gml("validTime"));
				if (gml_valid_time)
				{
					const GPlatesPropertyValues::GeoTimeInstant &begin_time = gml_valid_time.get()->begin()->get_time_position();
					const GPlatesPropertyValues::GeoTimeInstant &end_time = gml_valid_time.get()->end()->get_time_position();

					output_stream << "> ValidTime=(";
					if (begin_time.is_real())
					{
						output_stream << begin_time.value();
					}
					else if (begin_time.is_distant_past())
					{
						output_stream << "distant past";
					}
					else if (begin_time.is_distant_future())
					{
						output_stream << "distant future";
					}
					output_stream << ", ";
					if (end_time.is_real())
					{
						output_stream << end_time.value();
					}
					else if (end_time.is_distant_past())
					{
						output_stream << "distant past";
					}
					else if (end_time.is_distant_future())
					{
						output_stream << "distant future";
					}
					output_stream << ")\n";
				}
				else
				{
					output_stream << "> ValidTime=(distant past, distant future)\n";
				}

				// Print the feature's reconstruction plate ID.
				boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> gpml_reconstruction_plate_id =
						GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
								deformed_feature_geometry.get_feature_ref(),
								GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"));
				if (gpml_reconstruction_plate_id)
				{
					output_stream << "> ReconstructionPlateId=" << gpml_reconstruction_plate_id.get()->get_value() << "\n";
				}
				else
				{
					output_stream << "> ReconstructionPlateId=0\n";
				}

				// Print feature ID.
				output_stream
						<< "> FeatureID="
						<< deformed_feature_geometry.get_feature_ref()->feature_id().get().qstring() << "\n";

				// Print the time range over which the reconstructed feature was reconstructed using topologies.
				const GPlatesAppLogic::TimeSpanUtils::TimeRange time_range = deformed_feature_geometry.get_time_range();
				output_stream << "> Topology reconstruction time range:"
						<< " BeginTime=" << time_range.get_begin_time()
						<< " EndTime=" << time_range.get_end_time()
						<< " TimeIncrement=" << time_range.get_time_increment();
				output_stream << "\n";

				//
				// Print the feature data.
				//

				typedef GPlatesAppLogic::TopologyReconstructedFeatureGeometry::point_seq_type point_seq_type;
				typedef GPlatesAppLogic::TopologyReconstructedFeatureGeometry::point_deformation_strain_rate_seq_type
						point_deformation_strain_rate_seq_type;
				typedef GPlatesAppLogic::TopologyReconstructedFeatureGeometry::point_deformation_total_strain_seq_type
						point_deformation_total_strain_seq_type;

				point_seq_type deformed_domain_points;
				point_deformation_strain_rate_seq_type deformation_strain_rates;
				point_deformation_total_strain_seq_type deformation_strains;

				// Only retrieve strain rates and total strains if needed.
				boost::optional<point_deformation_strain_rate_seq_type &> deformation_strain_rates_option;
				boost::optional<point_deformation_total_strain_seq_type &> deformation_strains_option;
				if (include_dilatation_rate)
				{
					deformation_strain_rates_option = deformation_strain_rates;
				}
				if (include_dilatation)
				{
					deformation_strains_option = deformation_strains;
				}

				// Get the current (per-point) geometry data.
				deformed_feature_geometry.get_geometry_data(
						deformed_domain_points,
						deformation_strain_rates_option,
						deformation_strains_option);

				boost::optional< std::vector<double> > dilatation_rates;
				if (include_dilatation_rate)
				{
					dilatation_rates = std::vector<double>();

					// The number of domain points should match the number of deformation strain rates.
					GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
							deformed_domain_points.size() == deformation_strain_rates.size(),
							GPLATES_ASSERTION_SOURCE);

					dilatation_rates->reserve(deformation_strain_rates.size());
					for (unsigned int n = 0; n < deformation_strain_rates.size(); ++n)
					{
						dilatation_rates->push_back(deformation_strain_rates[n].get_strain_rate_dilatation());
					}
				}

				boost::optional< std::vector<double> > dilatations;
				if (include_dilatation)
				{
					dilatations = std::vector<double>();

					// The number of domain points should match the number of deformation strains.
					GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
							deformed_domain_points.size() == deformation_strains.size(),
							GPLATES_ASSERTION_SOURCE);

					dilatations->reserve(deformation_strains.size());
					for (unsigned int n = 0; n < deformation_strains.size(); ++n)
					{
						dilatations->push_back(deformation_strains[n].get_strain_dilatation());
					}
				}

				point_seq_type::const_iterator domain_iter = deformed_domain_points.begin();
				point_seq_type::const_iterator domain_end = deformed_domain_points.end();
				for (unsigned int index = 0; domain_iter != domain_end; ++domain_iter, ++index)
				{
					const GPlatesMaths::PointOnSphere &domain_point = *domain_iter;

					boost::optional<const double &> dilatation_rate;
					if (include_dilatation_rate)
					{
						dilatation_rate = dilatation_rates.get()[index];
					}

					boost::optional<const double &> dilatation;
					if (include_dilatation)
					{
						dilatation = dilatations.get()[index];
					}

					print_gmt_deformation_line(
							output_stream,
							domain_point,
							domain_point_lon_lat_format,
							dilatation_rate,
							dilatation);
				}
			}
		}
	}
}


void
GPlatesFileIO::GMTFormatDeformationExport::export_deformation(
		const std::list<deformed_feature_geometry_group_type> &deformed_feature_geometry_group_seq,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool domain_point_lon_lat_format,
		bool include_dilatation_rate,
		bool include_dilatation)
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
	// Print global header.
	//

	// Print the reconstruction time and anchored plate ID.
	output_stream << "> ReconstructionTime=" << reconstruction_time << "\n";
	output_stream << "> AnchoredPlateID=" << reconstruction_anchor_plate_id << "\n";

	// Print the data column headers.
	output_stream << "> Columns:";
	if (domain_point_lon_lat_format)
	{
		output_stream << " Longitude Latitude";
	}
	else
	{
		output_stream << " Latitude Longitude";
	}
	if (include_dilatation_rate)
	{
		output_stream << " DilatationRate";
	}
	if (include_dilatation)
	{
		output_stream << " Dilatation";
	}
	output_stream << "\n";

	output_stream << ">\n";

	// Iterate through the deformed feature geometries and write to output.
	std::list<deformed_feature_geometry_group_type>::const_iterator feature_iter;
	for (feature_iter = deformed_feature_geometry_group_seq.begin();
		feature_iter != deformed_feature_geometry_group_seq.end();
		++feature_iter)
	{
		const deformed_feature_geometry_group_type &deformed_feature_geometry_group = *feature_iter;

		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref = deformed_feature_geometry_group.feature_ref;
		if (!feature_ref.is_valid())
		{
			continue;
		}

		// Iterate through the deformed feature geometries of the current feature and write to output.
		deformed_feature_geometry_seq_type::const_iterator dfg_iter;
		for (dfg_iter = deformed_feature_geometry_group.recon_geoms.begin();
			dfg_iter != deformed_feature_geometry_group.recon_geoms.end();
			++dfg_iter)
		{
			const GPlatesAppLogic::TopologyReconstructedFeatureGeometry *dfg = *dfg_iter;

			// Write the topology reconstructed feature geometry (positions and dilatation rates).
			print_gmt_deformed_feature_geometry(
					output_stream,
					*dfg,
					domain_point_lon_lat_format,
					include_dilatation_rate,
					include_dilatation);
		}
	}
}
