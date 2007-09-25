/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 The University of Sydney, Australia
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

#include "PlatesLineFormatReader.h"

#include <fstream>
#include <sstream>

#include "util/StringUtils.h"
#include "util/MathUtils.h"
#include "fileio/ReadErrors.h"
#include "fileio/LineReader.h"

#include "model/FeatureRevision.h"
#include "model/GmlLineString.h"
#include "model/GmlOrientableCurve.h"
#include "model/GmlTimePeriod.h"
#include "model/GpmlConstantValue.h"
#include "model/GpmlFiniteRotation.h"
#include "model/GpmlFiniteRotationSlerp.h"
#include "model/GpmlIrregularSampling.h"
#include "model/GpmlPlateId.h"
#include "model/GpmlTimeSample.h"
#include "model/GpmlOldPlatesHeader.h"
#include "model/InlinePropertyContainer.h"
#include "model/DummyTransactionHandle.h"
#include "model/ModelUtility.h"

namespace {

	namespace PlotterCodes {
		enum PlotterCode {
			PEN_EITHER, PEN_TERMINATING_POINT, PEN_DOWN, PEN_UP
		};
	}

	GPlatesModel::FeatureHandle::weak_ref	
	create_common(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesModel::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::vector<double> &points,
			const UnicodeString &feature_type_string)
	{
		GPlatesModel::FeatureType feature_type(feature_type_string);
		GPlatesModel::FeatureHandle::weak_ref feature_handle =
				model.create_feature(feature_type, collection);

		const GPlatesModel::GpmlPlateId::integer_plate_id_type plate_id = header->plate_id_number();
		const GPlatesModel::GeoTimeInstant geo_time_instant_begin(header->age_of_appearance());
		const GPlatesModel::GeoTimeInstant geo_time_instant_end(header->age_of_disappearance());

		const GPlatesModel::PropertyContainer::non_null_ptr_type reconstruction_plate_id_container =
				GPlatesModel::ModelUtility::create_reconstruction_plate_id(plate_id);
		const GPlatesModel::PropertyContainer::non_null_ptr_type centre_line_of_container =
				GPlatesModel::ModelUtility::create_centre_line_of(points);
		const GPlatesModel::PropertyContainer::non_null_ptr_type valid_time_container =
				GPlatesModel::ModelUtility::create_valid_time(geo_time_instant_begin, geo_time_instant_end);
		
		GPlatesModel::DummyTransactionHandle pc1(__FILE__, __LINE__);
		feature_handle->append_property_container(reconstruction_plate_id_container, pc1);
		pc1.commit();
	
		GPlatesModel::DummyTransactionHandle pc2(__FILE__, __LINE__);
		feature_handle->append_property_container(centre_line_of_container, pc2);
		pc2.commit();
	
		GPlatesModel::DummyTransactionHandle pc3(__FILE__, __LINE__);
		feature_handle->append_property_container(valid_time_container, pc3);
		pc3.commit();
	
		GPlatesModel::XsString::non_null_ptr_type description = 
				GPlatesModel::XsString::create(header->geographic_description());
		
		GPlatesModel::ModelUtility::append_property_value_to_feature(
				description, "gml:description", feature_handle);

		GPlatesModel::ModelUtility::append_property_value_to_feature(
				header->clone(), "gpml:oldPlatesHeader", feature_handle);

		return feature_handle;
	}

	GPlatesModel::FeatureHandle::weak_ref	
	create_fault(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesModel::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::vector<double> &points)
	{
		return create_common(model, collection, header, points, "gpml:Fault");
	}

	GPlatesModel::FeatureHandle::weak_ref
	create_custom_fault(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesModel::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::vector<double> &points,
			const std::string &dip_slip)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
			create_fault(model, collection, header, points);
		
		const GPlatesModel::GpmlStrikeSlipEnumeration::non_null_ptr_type dip_slip_property_value =
				GPlatesModel::ModelUtility::create_gpml_strike_slip_enumeration(dip_slip);
		GPlatesModel::ModelUtility::append_property_value_to_feature(
				dip_slip_property_value, "gpml:dipSlip", feature_handle);

		return feature_handle;
	}

	GPlatesModel::FeatureHandle::weak_ref	
	create_normal_fault(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesModel::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::vector<double> &points)
	{
		return create_custom_fault(model, collection, header, points, "Extension");
	}

	GPlatesModel::FeatureHandle::weak_ref	
	create_reverse_fault(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesModel::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::vector<double> &points)
	{
		return create_custom_fault(model, collection, header, points, "Compression");
	}

	GPlatesModel::FeatureHandle::weak_ref	
	create_thrust_fault(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesModel::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::vector<double> &points)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
			create_reverse_fault(model, collection, header, points);

		// FIXME: Set .subcategory to "Thrust"
		return feature_handle;
	}

	GPlatesModel::FeatureHandle::weak_ref	
	create_unclassified_feature(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesModel::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::vector<double> &points)
	{
		return create_common(model, collection, header, points, "gpml:UnclassifiedFeature");
	}

	GPlatesModel::FeatureHandle::weak_ref	
	create_mid_ocean_ridge(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesModel::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::vector<double> &points,
			bool is_active)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
			create_common(model, collection, header, points, "gpml:MidOceanRidge");
		
		const GPlatesModel::XsBoolean::non_null_ptr_type is_active_property_value =
				GPlatesModel::ModelUtility::create_xs_boolean(is_active);
		GPlatesModel::ModelUtility::append_property_value_to_feature(
				is_active_property_value, "gpml:isActive", feature_handle);

		return feature_handle;
	}

	GPlatesModel::FeatureHandle::weak_ref	
	create_ridge_segment(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesModel::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::vector<double> &points)
	{
		return create_mid_ocean_ridge(model, collection, header, points, true);
	}
	
	GPlatesModel::FeatureHandle::weak_ref	
	create_extinct_ridge(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesModel::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::vector<double> &points)
	{
		return create_mid_ocean_ridge(model, collection, header, points, false);
	}

	GPlatesModel::FeatureHandle::weak_ref	
	create_continental_boundary(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesModel::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::vector<double> &points)
	{
		return create_common(model, collection, header, points, "gpml:PassiveContinentalBoundary");
	}

	GPlatesModel::FeatureHandle::weak_ref	
	create_orogenic_belt(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesModel::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::vector<double> &points)
	{
		return create_common(model, collection, header, points, "gpml:OrogenicBelt");
	}

	GPlatesModel::FeatureHandle::weak_ref	
	create_isochron(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesModel::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::vector<double> &points)
	{
		return create_common(model, collection, header, points, "gpml:Isochron");
	}

	typedef GPlatesModel::FeatureHandle::weak_ref (*creation_function_type)(
			GPlatesModel::ModelInterface &,
			GPlatesModel::FeatureCollectionHandle::weak_ref &,
			GPlatesModel::GpmlOldPlatesHeader::non_null_ptr_type &,
			const std::vector<double> &);

	typedef std::map<UnicodeString, creation_function_type> creation_map_type;
	typedef creation_map_type::const_iterator creation_map_const_iterator;

	const creation_map_type &
	build_feature_creation_map()
	{
		static creation_map_type map;
		map["CB"] = create_continental_boundary;
		map["CM"] = create_continental_boundary;
		map["CO"] = create_continental_boundary;
		map["IS"] = create_isochron;
		map["IM"] = create_isochron;
		map["NF"] = create_normal_fault;
		map["OB"] = create_orogenic_belt;
		map["OR"] = create_orogenic_belt;
		map["RF"] = create_reverse_fault;
		map["RI"] = create_ridge_segment;
		map["SS"] = create_fault;
		map["TH"] = create_thrust_fault;
		map["XR"] = create_extinct_ridge;
		return map;
	}

	GPlatesModel::GpmlOldPlatesHeader::non_null_ptr_type
	read_old_plates_header(
			GPlatesFileIO::LineReader &in,
			const std::string &first_line)
	{
		std::string second_line;
		if ( ! in.getline(second_line)) {
			throw GPlatesFileIO::ReadErrors::MissingPlatesHeaderSecondLine;
		}

		typedef GPlatesModel::GpmlPlateId::integer_plate_id_type plate_id_type;
		return GPlatesModel::GpmlOldPlatesHeader::create(
				GPlatesUtil::slice_string<unsigned int>(first_line, 0, 2, 
					GPlatesFileIO::ReadErrors::InvalidPlatesRegionNumber),
				GPlatesUtil::slice_string<unsigned int>(first_line, 2, 4, 
					GPlatesFileIO::ReadErrors::InvalidPlatesReferenceNumber),
				GPlatesUtil::slice_string<unsigned int>(first_line, 5, 9, 
					GPlatesFileIO::ReadErrors::InvalidPlatesStringNumber),
				GPlatesUtil::slice_string<std::string>(first_line, 10, std::string::npos, 
					GPlatesFileIO::ReadErrors::InvalidPlatesGeographicDescription).c_str(),
				GPlatesUtil::slice_string<plate_id_type>(second_line, 1, 4, 
					GPlatesFileIO::ReadErrors::InvalidPlatesPlateIdNumber),
				GPlatesUtil::slice_string<double>(second_line, 5, 11, 
					GPlatesFileIO::ReadErrors::InvalidPlatesAgeOfAppearance),
				GPlatesUtil::slice_string<double>(second_line, 12, 18, 
					GPlatesFileIO::ReadErrors::InvalidPlatesAgeOfDisappearance),
				GPlatesUtil::slice_string<std::string>(second_line, 19, 21, 
					GPlatesFileIO::ReadErrors::InvalidPlatesDataTypeCode).c_str(),
				GPlatesUtil::slice_string<unsigned int>(second_line, 21, 25, 
					GPlatesFileIO::ReadErrors::InvalidPlatesDataTypeCodeNumber),
				GPlatesUtil::slice_string<std::string>(second_line, 25, 26, 
					GPlatesFileIO::ReadErrors::InvalidPlatesDataTypeCodeNumberAdditional).c_str(),
				GPlatesUtil::slice_string<plate_id_type>(second_line, 26, 29, 
					GPlatesFileIO::ReadErrors::InvalidPlatesConjugatePlateIdNumber),
				GPlatesUtil::slice_string<unsigned int>(second_line, 30, 33, 
					GPlatesFileIO::ReadErrors::InvalidPlatesColourCode),
				GPlatesUtil::slice_string<unsigned int>(second_line, 34, 39, 
					GPlatesFileIO::ReadErrors::InvalidPlatesNumberOfPoints));
	}
	
	PlotterCodes::PlotterCode
	read_polyline_point(
			GPlatesFileIO::LineReader &in,
			std::vector<double> &points,
			PlotterCodes::PlotterCode expected_code)
	{
		std::string line;
		if ( ! in.getline(line)) {
			throw GPlatesFileIO::ReadErrors::MissingPlatesPolylinePoint;
		}

		int plotter;
		double latitude, longitude;

		std::istringstream iss(line);
		iss >> latitude >> longitude >> plotter;
		if ( ! iss) { 
			throw GPlatesFileIO::ReadErrors::InvalidPlatesPolylinePoint;
		}

		if (expected_code != PlotterCodes::PEN_EITHER && expected_code != plotter) {
			throw GPlatesFileIO::ReadErrors::MissingPlatesPolylinePoint;
		} else if (plotter == PlotterCodes::PEN_UP && 
				GPlatesUtil::are_values_approx_equal(latitude, 99.0) && 
				GPlatesUtil::are_values_approx_equal(longitude, 99.0))
		{
			return PlotterCodes::PEN_TERMINATING_POINT;
		}

		if (plotter != PlotterCodes::PEN_UP && plotter != PlotterCodes::PEN_DOWN) {
			throw GPlatesFileIO::ReadErrors::BadPlatesPolylinePlotterCode;
		} else if ( ! GPlatesUtil::is_value_in_range(latitude, -90.0, +90.0)) {
			throw GPlatesFileIO::ReadErrors::BadPlatesPolylineLatitude;
		} else if ( ! GPlatesUtil::is_value_in_range(longitude, -360.0, +360.0)) {
			throw GPlatesFileIO::ReadErrors::BadPlatesPolylineLongitude;
		}

		points.push_back(longitude);
		points.push_back(latitude);
		return static_cast<PlotterCodes::PlotterCode>(plotter);
	}

	void
	read_features(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesFileIO::LineReader &in,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			GPlatesFileIO::ReadErrorAccumulation &errors)
	{
		static const creation_map_type &map = build_feature_creation_map();
		creation_function_type creation_function = create_unclassified_feature;

		std::string first_line;
		if ( ! in.getline(first_line)) {
			return; // Do not want to throw here: end of file reached
		}

		GPlatesModel::GpmlOldPlatesHeader::non_null_ptr_type 
				old_plates_header = read_old_plates_header(in, first_line);

		creation_map_const_iterator result = map.find(old_plates_header->data_type_code());	
		if (result != map.end()) {
			creation_function = result->second;
		} else {
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
					new GPlatesFileIO::LineNumberInFile(in.line_number()));
			errors.d_warnings.push_back(GPlatesFileIO::ReadErrorOccurrence(source, location, 
					GPlatesFileIO::ReadErrors::UnknownPlatesDataTypeCode,
					GPlatesFileIO::ReadErrors::UnclassifiedFeatureCreated));
		}

		std::vector<double> points;
		read_polyline_point(in, points, PlotterCodes::PEN_UP);

		PlotterCodes::PlotterCode code;
		do {
			code = read_polyline_point(in, points, PlotterCodes::PEN_EITHER);
			if (code == PlotterCodes::PEN_UP || code == PlotterCodes::PEN_TERMINATING_POINT) {
				creation_function(model, collection, old_plates_header, points);
				points.clear();
			}
		} while (code != PlotterCodes::PEN_TERMINATING_POINT);
	}
}

const GPlatesModel::FeatureCollectionHandle::weak_ref
GPlatesFileIO::PlatesLineFormatReader::read_file(
		const std::string &filename,
		GPlatesModel::ModelInterface &model,
		ReadErrorAccumulation &read_errors)
{
	std::ifstream input(filename.c_str());
	if ( ! input) {
		throw ErrorOpeningFileForReadingException(filename);
	}

	boost::shared_ptr<DataSource> source( 
			new GPlatesFileIO::LocalFileDataSource(filename, DataFormats::PlatesLine));
	GPlatesModel::FeatureCollectionHandle::weak_ref collection
			= model.create_feature_collection();
	
	LineReader in(input);
	while (in) {
		try {
			read_features(model, collection, in, source, read_errors);
		} catch (GPlatesFileIO::ReadErrors::Description error) {
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
					new GPlatesFileIO::LineNumberInFile(in.line_number()));
			read_errors.d_recoverable_errors.push_back(GPlatesFileIO::ReadErrorOccurrence(
					source, location, error, GPlatesFileIO::ReadErrors::FeatureDiscarded));
		}
	}

	return collection;
}
