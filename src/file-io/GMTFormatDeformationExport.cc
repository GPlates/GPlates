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
			 * Outputs a deformation line to the GMT output consisting of position and optional strain rates.
			 */
			void
			print_gmt_deformation_line(
					QTextStream &output_stream,
					const GPlatesMaths::PointOnSphere &domain_point,
					bool domain_point_lon_lat_format,
					boost::optional<const GPlatesAppLogic::DeformationStrain::StrainPrincipal &> principal_strain,
					boost::optional<const GPlatesFileIO::DeformationExport::PrincipalStrainOptions &> principal_strain_options,
					boost::optional<const double &> dilatation_strain,
					boost::optional<const double &> dilatation_strain_rate,
					boost::optional<const double &> second_invariant_strain_rate)
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
				// Output principal strain.
				//

				if (principal_strain &&
					principal_strain_options) // Either both are true or both are false.
				{
					const double principal_angle_or_azimuth_in_degrees =
							principal_strain_options->get_principal_angle_or_azimuth_in_degrees(principal_strain.get());

					double principal_major;
					double principal_minor;
					if (principal_strain_options->output == DeformationExport::PrincipalStrainOptions::STRAIN)
					{
						// Output strain.
						principal_major = principal_strain->principal1;
						principal_minor = principal_strain->principal2;
					}
					else // PrincipalStrainOptions::STRETCH...
					{
						// Output stretch (1.0 + strain).
						principal_major = 1.0 + principal_strain->principal1;
						principal_minor = 1.0 + principal_strain->principal2;
					}

					// Don't format as fixed notation.
					gmt_line << " "
							<< std::scientific << std::setprecision(SCALAR_PRECISION)
							<< std::setw(SCALAR_FIELDWIDTH) << principal_angle_or_azimuth_in_degrees << " "
							<< std::setw(SCALAR_FIELDWIDTH) << principal_major << " "
							<< std::setw(SCALAR_FIELDWIDTH) << principal_minor;
				}

				//
				// Output dilatation strain.
				//

				if (dilatation_strain)
				{
					// Don't format as fixed notation.
					gmt_line << " "
							<< std::setw(SCALAR_FIELDWIDTH) << std::scientific << std::setprecision(SCALAR_PRECISION)
							<< dilatation_strain.get();
				}

				//
				// Output dilatation strain rate.
				//

				if (dilatation_strain_rate)
				{
					// Don't format as fixed notation.
					gmt_line << " "
							<< std::setw(SCALAR_FIELDWIDTH) << std::scientific << std::setprecision(SCALAR_PRECISION)
							<< dilatation_strain_rate.get();
				}

				//
				// Output second invariant strain rate.
				//

				if (second_invariant_strain_rate)
				{
					// Don't format as fixed notation.
					gmt_line << " "
							<< std::setw(SCALAR_FIELDWIDTH) << std::scientific << std::setprecision(SCALAR_PRECISION)
							<< second_invariant_strain_rate.get();
				}

				//
				// Output the final line.
				//

				const std::string gmt_line_string = gmt_line.str();
				output_stream << gmt_line_string.c_str() << endl;
			}


			/**
			 * Write the deformed feature geometry (positions and strain rates).
			 */
			void
			print_gmt_deformed_feature_geometry(
					QTextStream &output_stream,
					const GPlatesAppLogic::TopologyReconstructedFeatureGeometry &deformed_feature_geometry,
					bool domain_point_lon_lat_format,
					boost::optional<DeformationExport::PrincipalStrainOptions> include_principal_strain,
					bool include_dilatation_strain,
					bool include_dilatation_strain_rate,
					bool include_second_invariant_strain_rate)
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
					output_stream << "> Name=" << gml_name.get()->value().get().qstring() << "\n";
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
					const GPlatesPropertyValues::GeoTimeInstant &begin_time = gml_valid_time.get()->begin()->time_position();
					const GPlatesPropertyValues::GeoTimeInstant &end_time = gml_valid_time.get()->end()->time_position();

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
					output_stream << "> ReconstructionPlateId=" << gpml_reconstruction_plate_id.get()->value() << "\n";
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

				// Only retrieve strain rates if needed.
				boost::optional<point_deformation_strain_rate_seq_type &> deformation_strain_rates_option;
				if (include_dilatation_strain_rate ||
					include_second_invariant_strain_rate)
				{
					deformation_strain_rates_option = deformation_strain_rates;
				}

				// Only retrieve strain if needed.
				boost::optional<point_deformation_total_strain_seq_type &> deformation_strains_option;
				if (include_principal_strain ||
					include_dilatation_strain)
				{
					deformation_strains_option = deformation_strains;
				}

				// Get the current (per-point) geometry data.
				deformed_feature_geometry.get_geometry_data(
						deformed_domain_points,
						deformation_strain_rates_option,
						deformation_strains_option);

				if (include_principal_strain ||
					include_dilatation_strain)
				{
					// The number of domain points should match the number of deformation strains.
					GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
							deformed_domain_points.size() == deformation_strains.size(),
							GPLATES_ASSERTION_SOURCE);
				}

				if (include_dilatation_strain_rate ||
					include_second_invariant_strain_rate)
				{
					// The number of domain points should match the number of deformation strain rates.
					GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
							deformed_domain_points.size() == deformation_strain_rates.size(),
							GPLATES_ASSERTION_SOURCE);
				}

				boost::optional< std::vector<GPlatesAppLogic::DeformationStrain::StrainPrincipal> > principal_strains;
				if (include_principal_strain)
				{
					principal_strains = std::vector<GPlatesAppLogic::DeformationStrain::StrainPrincipal>();

					principal_strains->reserve(deformation_strains.size());
					for (unsigned int n = 0; n < deformation_strains.size(); ++n)
					{
						principal_strains->push_back(deformation_strains[n].get_strain_principal());
					}
				}

				boost::optional< std::vector<double> > dilatation_strains;
				if (include_dilatation_strain)
				{
					dilatation_strains = std::vector<double>();

					dilatation_strains->reserve(deformation_strains.size());
					for (unsigned int n = 0; n < deformation_strains.size(); ++n)
					{
						dilatation_strains->push_back(deformation_strains[n].get_strain_dilatation());
					}
				}

				boost::optional< std::vector<double> > dilatation_strain_rates;
				if (include_dilatation_strain_rate)
				{
					dilatation_strain_rates = std::vector<double>();

					dilatation_strain_rates->reserve(deformation_strain_rates.size());
					for (unsigned int n = 0; n < deformation_strain_rates.size(); ++n)
					{
						dilatation_strain_rates->push_back(deformation_strain_rates[n].get_strain_rate_dilatation());
					}
				}

				boost::optional< std::vector<double> > second_invariant_strain_rates;
				if (include_second_invariant_strain_rate)
				{
					second_invariant_strain_rates = std::vector<double>();

					second_invariant_strain_rates->reserve(deformation_strain_rates.size());
					for (unsigned int n = 0; n < deformation_strain_rates.size(); ++n)
					{
						second_invariant_strain_rates->push_back(deformation_strain_rates[n].get_strain_rate_second_invariant());
					}
				}

				point_seq_type::const_iterator domain_iter = deformed_domain_points.begin();
				point_seq_type::const_iterator domain_end = deformed_domain_points.end();
				for (unsigned int index = 0; domain_iter != domain_end; ++domain_iter, ++index)
				{
					const GPlatesMaths::PointOnSphere &domain_point = *domain_iter;

					boost::optional<const GPlatesAppLogic::DeformationStrain::StrainPrincipal &> principal_strain;
					boost::optional<const DeformationExport::PrincipalStrainOptions &> principal_strain_options;
					if (include_principal_strain)
					{
						principal_strain = principal_strains.get()[index];
						principal_strain_options = include_principal_strain.get();
					}

					boost::optional<const double &> dilatation_strain;
					if (include_dilatation_strain)
					{
						dilatation_strain = dilatation_strains.get()[index];
					}

					boost::optional<const double &> dilatation_strain_rate;
					if (include_dilatation_strain_rate)
					{
						dilatation_strain_rate = dilatation_strain_rates.get()[index];
					}

					boost::optional<const double &> second_invariant_strain_rate;
					if (include_second_invariant_strain_rate)
					{
						second_invariant_strain_rate = second_invariant_strain_rates.get()[index];
					}

					print_gmt_deformation_line(
							output_stream,
							domain_point,
							domain_point_lon_lat_format,
							principal_strain,
							principal_strain_options,
							dilatation_strain,
							dilatation_strain_rate,
							second_invariant_strain_rate);
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
		boost::optional<DeformationExport::PrincipalStrainOptions> include_principal_strain,
		bool include_dilatation_strain,
		bool include_dilatation_strain_rate,
		bool include_second_invariant_strain_rate)
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
	if (include_principal_strain)
	{
		if (include_principal_strain->output == DeformationExport::PrincipalStrainOptions::STRAIN)
		{
			if (include_principal_strain->format == DeformationExport::PrincipalStrainOptions::ANGLE_MAJOR_MINOR)
			{
				output_stream << " PrincipalStrainMajorAngle";
			}
			else
			{
				output_stream << " PrincipalStrainMajorAzimuth";
			}

			output_stream << " PrincipalStrainMajorAxis";
			output_stream << " PrincipalStrainMinorAxis";
		}
		else
		{
			if (include_principal_strain->format == DeformationExport::PrincipalStrainOptions::ANGLE_MAJOR_MINOR)
			{
				output_stream << " PrincipalStretchMajorAngle";
			}
			else
			{
				output_stream << " PrincipalStretchMajorAzimuth";
			}

			output_stream << " PrincipalStretchMajorAxis";
			output_stream << " PrincipalStretchMinorAxis";
		}
	}
	if (include_dilatation_strain)
	{
		output_stream << " DilatationStrain";
	}
	if (include_dilatation_strain_rate)
	{
		output_stream << " DilatationStrainRate";
	}
	if (include_second_invariant_strain_rate)
	{
		output_stream << " TotalStrainRate";
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

			// Write the topology reconstructed feature geometry (positions and strain rates).
			print_gmt_deformed_feature_geometry(
					output_stream,
					*dfg,
					domain_point_lon_lat_format,
					include_principal_strain,
					include_dilatation_strain,
					include_dilatation_strain_rate,
					include_second_invariant_strain_rate);
		}
	}
}
