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

#include "app-logic/ReconstructedScalarCoverage.h"
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
	namespace GMTFormatReconstructedScalarCoverageExport
	{
		namespace
		{
			//! Convenience typedef for a sequence of reconstructed scalar coverages.
			typedef std::vector<const GPlatesAppLogic::ReconstructedScalarCoverage *>
					reconstructed_scalar_coverage_seq_type;


			/**
			 * Outputs a scalar coverage line to the GMT output.
			 */
			void
			print_gmt_scalar_coverage_line(
					QTextStream &output_stream,
					const GPlatesMaths::PointOnSphere &domain_point,
					bool domain_point_lon_lat_format,
					boost::optional<const double &> dilatation_strain,
					boost::optional<const double &> dilatation_strain_rate,
					boost::optional<const double &> second_invariant_strain_rate,
					const double &scalar_value)
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
			 * Write the scalar coverage and optionally strain rates.
			 */
			void
			print_gmt_scalar_coverage(
					QTextStream &output_stream,
					const GPlatesAppLogic::ReconstructedScalarCoverage &reconstructed_scalar_coverage,
					bool domain_point_lon_lat_format,
					bool include_dilatation_strain,
					bool include_dilatation_strain_rate,
					bool include_second_invariant_strain_rate)
			{
				// See if RFG was reconstructed using topologies.
				boost::optional<const GPlatesAppLogic::TopologyReconstructedFeatureGeometry *> dfg =
						GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
								const GPlatesAppLogic::TopologyReconstructedFeatureGeometry *>(
										reconstructed_scalar_coverage.get_reconstructed_feature_geometry().get());

				//
				// Print the feature header.
				//

				// Print the feature's name.
				boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type> gml_name =
						GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::XsString>(
								reconstructed_scalar_coverage.get_feature_ref(),
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
								reconstructed_scalar_coverage.get_feature_ref(),
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
								reconstructed_scalar_coverage.get_feature_ref(),
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
						<< reconstructed_scalar_coverage.get_feature_ref()->feature_id().get().qstring() << "\n";

				if (dfg)
				{
					// Print the time range over which the reconstructed feature was reconstructed using topologies.
					const GPlatesAppLogic::TimeSpanUtils::TimeRange time_range = dfg.get()->get_time_range();
					output_stream << "> Topology reconstruction time range:"
							<< " BeginTime=" << time_range.get_begin_time()
							<< " EndTime=" << time_range.get_end_time()
							<< " TimeIncrement=" << time_range.get_time_increment();
					output_stream << "\n";
				}

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
				// The scalar type.
				output_stream << " " << reconstructed_scalar_coverage.get_scalar_type().get_name().qstring();
				output_stream << "\n";

				//
				// Print the feature data.
				//

				GPlatesAppLogic::ReconstructedScalarCoverage::point_seq_type reconstructed_domain_points;
				reconstructed_scalar_coverage.get_reconstructed_points(reconstructed_domain_points);

				GPlatesAppLogic::ReconstructedScalarCoverage::point_scalar_value_seq_type scalar_values;
				reconstructed_scalar_coverage.get_reconstructed_point_scalar_values(scalar_values);

				// The number of domain points should match the number of scalars.
				// The ReconstructedScalarCoverage interface guarantees this.
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						reconstructed_domain_points.size() == scalar_values.size(),
						GPLATES_ASSERTION_SOURCE);

				boost::optional< std::vector<double> > dilatation_strains;
				boost::optional< std::vector<double> > dilatation_strain_rates;
				boost::optional< std::vector<double> > second_invariant_strain_rates;
				if (include_dilatation_strain ||
					include_dilatation_strain_rate ||
					include_second_invariant_strain_rate)
				{
					if (dfg)
					{
						typedef GPlatesAppLogic::TopologyReconstructedFeatureGeometry::point_deformation_strain_rate_seq_type
								point_deformation_strain_rate_seq_type;
						typedef GPlatesAppLogic::TopologyReconstructedFeatureGeometry::point_deformation_total_strain_seq_type
								point_deformation_total_strain_seq_type;

						// Get the current (per-point) geometry data.
						point_deformation_strain_rate_seq_type deformation_strain_rates;
						point_deformation_total_strain_seq_type deformation_strains;
						dfg.get()->get_geometry_data(
								boost::none/*points*/,
								deformation_strain_rates,
								deformation_strains);
						// The number of strain rates should match the number of scalars.
						GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
								deformation_strain_rates.size() == scalar_values.size(),
								GPLATES_ASSERTION_SOURCE);

						if (include_dilatation_strain)
						{
							dilatation_strains = std::vector<double>();

							dilatation_strains->reserve(deformation_strains.size());
							for (unsigned int n = 0; n < deformation_strains.size(); ++n)
							{
								dilatation_strains->push_back(deformation_strains[n].get_strain_dilatation());
							}
						}
						if (include_dilatation_strain_rate)
						{
							dilatation_strain_rates = std::vector<double>();

							dilatation_strain_rates->reserve(deformation_strain_rates.size());
							for (unsigned int n = 0; n < deformation_strain_rates.size(); ++n)
							{
								dilatation_strain_rates->push_back(deformation_strain_rates[n].get_strain_rate_dilatation());
							}
						}
						if (include_second_invariant_strain_rate)
						{
							second_invariant_strain_rates = std::vector<double>();

							second_invariant_strain_rates->reserve(deformation_strain_rates.size());
							for (unsigned int n = 0; n < deformation_strain_rates.size(); ++n)
							{
								second_invariant_strain_rates->push_back(deformation_strain_rates[n].get_strain_rate_second_invariant());
							}
						}
					}
					else
					{
						// The RFG is not a TopologyReconstructedFeatureGeometry so we have no deformation strain information.
						// Default to zero strain and strain rate.
						if (include_dilatation_strain)
						{
							dilatation_strains = std::vector<double>();
							dilatation_strains->resize(scalar_values.size(), 0.0);
						}
						if (include_dilatation_strain_rate)
						{
							dilatation_strain_rates = std::vector<double>();
							dilatation_strain_rates->resize(scalar_values.size(), 0.0);
						}
						if (include_second_invariant_strain_rate)
						{
							second_invariant_strain_rates = std::vector<double>();
							second_invariant_strain_rates->resize(scalar_values.size(), 0.0);
						}
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
					const GPlatesMaths::PointOnSphere &domain_point = *domain_iter;

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

					const double &scalar_value = *range_iter;

					print_gmt_scalar_coverage_line(
							output_stream,
							domain_point,
							domain_point_lon_lat_format,
							dilatation_strain,
							dilatation_strain_rate,
							second_invariant_strain_rate,
							scalar_value);
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
	output_stream << ">\n";

	// Iterate through the reconstructed scalar coverages and write to output.
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

		// Iterate through the reconstructed scalar coverages of the current feature and write to output.
		reconstructed_scalar_coverage_seq_type::const_iterator rsc_iter;
		for (rsc_iter = feature_scalar_coverage_group.recon_geoms.begin();
			rsc_iter != feature_scalar_coverage_group.recon_geoms.end();
			++rsc_iter)
		{
			const GPlatesAppLogic::ReconstructedScalarCoverage *rsc = *rsc_iter;

			// Write the scalar coverage and its domain positions.
			print_gmt_scalar_coverage(
					output_stream,
					*rsc,
					domain_point_lon_lat_format,
					include_dilatation_strain,
					include_dilatation_strain_rate,
					include_second_invariant_strain_rate);
		}
	}
}
