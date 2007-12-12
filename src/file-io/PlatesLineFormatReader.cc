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
#include <string>

#include "ReadErrors.h"
#include "LineReader.h"

#include "utils/StringUtils.h"
#include "utils/MathUtils.h"

#include "maths/LatLonPointConversions.h"

#include "model/FeatureRevision.h"
#include "model/InlinePropertyContainer.h"
#include "model/DummyTransactionHandle.h"
#include "model/ModelUtils.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlStrikeSlipEnumeration.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/XsBoolean.h"


namespace
{
	namespace PlotterCodes
	{
		/**
		 * These plotter codes are used to pass and return expected and actual pen codes.
		 * 
		 * Note that pen codes of 2 and 3 do actually occur in PLATES4 line-format files;
		 * the subsequent plotter codes in this enumeration are used purely as result
		 * codes.
		 */
		enum PlotterCode
		{
			PEN_DRAW_TO = 2, PEN_SKIP_TO = 3, PEN_TERMINATING_POINT, PEN_EITHER
		};
	}


	/**
	 * This function assumes that 'create_feature_with_geometry' has ensured that 'points'
	 * contains at least one point.
	 */
	void
	append_appropriate_geometry(
			const std::list<GPlatesMaths::PointOnSphere> &points,
			const char *property_name_string,
			GPlatesModel::FeatureHandle::weak_ref &feature)
	{
		using namespace GPlatesMaths;

		std::list<PointOnSphere>::size_type num_distinct_adj_points =
				count_distinct_adjacent_points(points);
#if 0
		if (num_distinct_adj_points >= 3 && points.front() == points.back()) {
			// It's a polygon.
			// FIXME:  Handle this.
		}
#endif
		if (num_distinct_adj_points >= 2) {
			// It's a polyline.

			// FIXME:  We should evaluate the PolylineOnSphere
			// ConstructionParameterValidity and report any parameter problems as
			// ReadErrors.

			PolylineOnSphere::non_null_ptr_type polyline =
					PolylineOnSphere::create_on_heap(points);
			GPlatesPropertyValues::GmlLineString::non_null_ptr_type gml_line_string =
					GPlatesPropertyValues::GmlLineString::create(polyline);
			GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type gml_orientable_curve =
					GPlatesModel::ModelUtils::create_gml_orientable_curve(gml_line_string);
			GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
					GPlatesModel::ModelUtils::create_gpml_constant_value(
							gml_orientable_curve, "gml:OrientableCurve");
			GPlatesModel::ModelUtils::append_property_value_to_feature(property_value,
					property_name_string, feature);
		} else if (num_distinct_adj_points == 1) {
			// It's a point.
			GPlatesPropertyValues::GmlPoint::non_null_ptr_type gml_point =
					GPlatesPropertyValues::GmlPoint::create(points.front());
			GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
					GPlatesModel::ModelUtils::create_gpml_constant_value(
							gml_point, "gml:Point");
			GPlatesModel::ModelUtils::append_property_value_to_feature(property_value,
					property_name_string, feature);
		} else {
			// FIXME:  A pre-condition of this function has been violated.  We should
			// throw an exception.
		}
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_common(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points,
			const UnicodeString &feature_type_string)
	{
		using namespace GPlatesPropertyValues;
		using namespace GPlatesModel;

		FeatureType feature_type(feature_type_string);
		FeatureHandle::weak_ref feature_handle =
				model.create_feature(feature_type, collection);

		const integer_plate_id_type plate_id = header->plate_id_number();
		const GeoTimeInstant geo_time_instant_begin(header->age_of_appearance());
		const GeoTimeInstant geo_time_instant_end(header->age_of_disappearance());

		// Wrap a "gpml:plateId" in a "gpml:ConstantValue" and append it as the
		// "gpml:reconstructionPlateId" property.
		GpmlPlateId::non_null_ptr_type recon_plate_id = GpmlPlateId::create(plate_id);
		ModelUtils::append_property_value_to_feature(
				ModelUtils::create_gpml_constant_value(recon_plate_id, "gpml:plateId"),
				"gpml:reconstructionPlateId", feature_handle);

		// FIXME:  The property name string for the geometry should be passed as a
		// parameter to this function, since different feature types have different names
		// for their geometries.
		append_appropriate_geometry(points, "gpml:centerLineOf", feature_handle);

		GmlTimePeriod::non_null_ptr_type gml_valid_time =
				ModelUtils::create_gml_time_period(geo_time_instant_begin, geo_time_instant_end);
		ModelUtils::append_property_value_to_feature(
				gml_valid_time, "gml:validTime", feature_handle);

		// Use the PLATES4 geographic description as the "gml:description" property.
		XsString::non_null_ptr_type gml_description = 
				XsString::create(header->geographic_description());
		ModelUtils::append_property_value_to_feature(
				gml_description, "gml:description", feature_handle);

		ModelUtils::append_property_value_to_feature(
				header->clone(), "gpml:oldPlatesHeader", feature_handle);

		return feature_handle;
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_fault(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points, "gpml:Fault");
	}


	GPlatesModel::FeatureHandle::weak_ref
	create_custom_fault(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points,
			const UnicodeString &dip_slip)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
			create_fault(model, collection, header, points);
		
		const GPlatesPropertyValues::GpmlStrikeSlipEnumeration::non_null_ptr_type dip_slip_property_value =
				GPlatesPropertyValues::GpmlStrikeSlipEnumeration::create(dip_slip);
		GPlatesModel::ModelUtils::append_property_value_to_feature(
				dip_slip_property_value, "gpml:dipSlip", feature_handle);

		return feature_handle;
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_normal_fault(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_custom_fault(model, collection, header, points, "Extension");
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_reverse_fault(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_custom_fault(model, collection, header, points, "Compression");
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_thrust_fault(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
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
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points, "gpml:UnclassifiedFeature");
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_mid_ocean_ridge(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points,
			bool is_active)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
			create_common(model, collection, header, points, "gpml:MidOceanRidge");
		
		const GPlatesPropertyValues::XsBoolean::non_null_ptr_type is_active_property_value =
				GPlatesPropertyValues::XsBoolean::create(is_active);
		GPlatesModel::ModelUtils::append_property_value_to_feature(
				is_active_property_value, "gpml:isActive", feature_handle);

		return feature_handle;
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_ridge_segment(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_mid_ocean_ridge(model, collection, header, points, true);
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_extinct_ridge(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_mid_ocean_ridge(model, collection, header, points, false);
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_continental_boundary(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points, "gpml:PassiveContinentalBoundary");
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_orogenic_belt(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points, "gpml:OrogenicBelt");
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_isochron(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		GPlatesModel::FeatureHandle::weak_ref feature =
		   	create_common(model, collection, header, points, "gpml:Isochron");
		const GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type conj_plate_id =
				GPlatesPropertyValues::GpmlPlateId::create(header->conjugate_plate_id_number());
		GPlatesModel::ModelUtils::append_property_value_to_feature(
				conj_plate_id, "gpml:conjugatePlateId", feature);
		return feature;
	}


	typedef GPlatesModel::FeatureHandle::weak_ref (*creation_function_type)(
			GPlatesModel::ModelInterface &,
			GPlatesModel::FeatureCollectionHandle::weak_ref &,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &,
			const std::list<GPlatesMaths::PointOnSphere> &);

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


	GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type
	read_old_plates_header(
			GPlatesFileIO::LineReader &in,
			const std::string &first_line)
	{
		std::string second_line;
		if ( ! in.getline(second_line)) {
			throw GPlatesFileIO::ReadErrors::MissingPlatesHeaderSecondLine;
		}

		typedef GPlatesModel::integer_plate_id_type plate_id_type;
		return GPlatesPropertyValues::GpmlOldPlatesHeader::create(
				GPlatesUtils::slice_string<unsigned int>(first_line, 0, 2, 
					GPlatesFileIO::ReadErrors::InvalidPlatesRegionNumber),
				GPlatesUtils::slice_string<unsigned int>(first_line, 2, 4, 
					GPlatesFileIO::ReadErrors::InvalidPlatesReferenceNumber),
				GPlatesUtils::slice_string<unsigned int>(first_line, 5, 9, 
					GPlatesFileIO::ReadErrors::InvalidPlatesStringNumber),
				GPlatesUtils::slice_string<std::string>(first_line, 10, std::string::npos, 
					GPlatesFileIO::ReadErrors::InvalidPlatesGeographicDescription).c_str(),
				GPlatesUtils::slice_string<plate_id_type>(second_line, 1, 4, 
					GPlatesFileIO::ReadErrors::InvalidPlatesPlateIdNumber),
				GPlatesUtils::slice_string<double>(second_line, 5, 11, 
					GPlatesFileIO::ReadErrors::InvalidPlatesAgeOfAppearance),
				GPlatesUtils::slice_string<double>(second_line, 12, 18, 
					GPlatesFileIO::ReadErrors::InvalidPlatesAgeOfDisappearance),
				GPlatesUtils::slice_string<std::string>(second_line, 19, 21, 
					GPlatesFileIO::ReadErrors::InvalidPlatesDataTypeCode).c_str(),
				GPlatesUtils::slice_string<unsigned int>(second_line, 21, 25, 
					GPlatesFileIO::ReadErrors::InvalidPlatesDataTypeCodeNumber),
				GPlatesUtils::slice_string<std::string>(second_line, 25, 26, 
					GPlatesFileIO::ReadErrors::InvalidPlatesDataTypeCodeNumberAdditional).c_str(),
				GPlatesUtils::slice_string<plate_id_type>(second_line, 26, 29, 
					GPlatesFileIO::ReadErrors::InvalidPlatesConjugatePlateIdNumber),
				GPlatesUtils::slice_string<unsigned int>(second_line, 30, 33, 
					GPlatesFileIO::ReadErrors::InvalidPlatesColourCode),
				GPlatesUtils::slice_string<unsigned int>(second_line, 34, 39, 
					GPlatesFileIO::ReadErrors::InvalidPlatesNumberOfPoints));
	}


	PlotterCodes::PlotterCode
	read_polyline_point(
			GPlatesFileIO::LineReader &in,
			std::list<GPlatesMaths::PointOnSphere> &points,
			PlotterCodes::PlotterCode expected_code)
	{
		std::string line;
		if ( ! in.getline(line)) {
			// Since we're in this function, we're expecting to read a point.  But we
			// couldn't find one.  So, let's complain.
			throw GPlatesFileIO::ReadErrors::MissingPlatesPolylinePoint;
		}

		int plotter;
		double latitude, longitude;

		std::istringstream iss(line);
		iss >> latitude >> longitude >> plotter;
		if ( ! iss) { 
			throw GPlatesFileIO::ReadErrors::InvalidPlatesPolylinePoint;
		}

		// First:  If we've encountered (lat = 99.0; lon = 99.0; plotter code = SKIP TO),
		// that's the end-of-polyline marker.
		if (plotter == PlotterCodes::PEN_SKIP_TO &&
				GPlatesUtils::are_almost_exactly_equal(latitude, 99.0) && 
				GPlatesUtils::are_almost_exactly_equal(longitude, 99.0))
		{
			// Note that we return without appending the point.
			return PlotterCodes::PEN_TERMINATING_POINT;
		}

		// Was it a valid plotter code which we read?
		if (plotter != PlotterCodes::PEN_DRAW_TO && plotter != PlotterCodes::PEN_SKIP_TO) {
			throw GPlatesFileIO::ReadErrors::InvalidPlatesPolylinePlotterCode;
		}

		// Was the plotter code what we expected?  (This is used to ensure that the first
		// plotter code after the two-line header is indeed a "skip to" code rather than a
		// "draw to" code.)
		if (expected_code != PlotterCodes::PEN_EITHER && expected_code != plotter) {
			throw GPlatesFileIO::ReadErrors::MissingPlatesPolylinePoint;
		}

		// Did the point have valid lat and lon?
		if ( ! GPlatesMaths::LatLonPoint::is_valid_latitude(latitude)) {
			throw GPlatesFileIO::ReadErrors::InvalidPlatesPolylineLatitude;
		} else if ( ! GPlatesMaths::LatLonPoint::is_valid_longitude(longitude)) {
			throw GPlatesFileIO::ReadErrors::InvalidPlatesPolylineLongitude;
		}
		GPlatesMaths::LatLonPoint llp(latitude, longitude);
		points.push_back(GPlatesMaths::make_point_on_sphere(llp));

		return static_cast<PlotterCodes::PlotterCode>(plotter);
	}


	void
	create_feature_with_geometry(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesFileIO::LineReader &in,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			creation_function_type creation_function,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type old_plates_header,
			std::list<GPlatesMaths::PointOnSphere> &points,
			GPlatesFileIO::ReadErrorAccumulation &errors)
	{
		if ( ! points.empty()) {
			try {
				creation_function(model, collection, old_plates_header, points);
			} catch (GPlatesGlobal::Exception &e) {
				std::cerr << "Caught exception: " << e << std::endl;
				std::cerr << "FIXME:  We really should report this properly!" << std::endl;
			}
			points.clear();
		} else {
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
					new GPlatesFileIO::LineNumberInFile(in.line_number()));
			errors.d_warnings.push_back(GPlatesFileIO::ReadErrorOccurrence(source, location, 
					GPlatesFileIO::ReadErrors::AdjacentSkipToPlotterCodes,
					GPlatesFileIO::ReadErrors::NoGeometryCreatedByMovement));
		}
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

		GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type 
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

		std::list<GPlatesMaths::PointOnSphere> points;
		read_polyline_point(in, points, PlotterCodes::PEN_SKIP_TO);

		PlotterCodes::PlotterCode code;
		do {
			code = read_polyline_point(in, points, PlotterCodes::PEN_EITHER);
			if (code == PlotterCodes::PEN_TERMINATING_POINT) {
				// When 'read_polyline_point' encounters the terminating point, it
				// doesn't append the point position, so we can create a geometry
				// using all the points in 'points'.
				create_feature_with_geometry(model, collection, in, source,
						creation_function, old_plates_header, points, errors);
			} else if (code == PlotterCodes::PEN_SKIP_TO) {
				// If neither an exception was thrown nor the "terminating point"
				// plotter code was returned, we know that 'read_polyline_point'
				// appended the point.
				//
				// However, since the code was "skip to", we should remove this
				// most recent point temporarily while we're creating a geometry
				// for the previous contiguous geometry.
				GPlatesMaths::PointOnSphere last_point = points.back();
				points.pop_back();

				create_feature_with_geometry(model, collection, in, source,
						creation_function, old_plates_header, points, errors);

				points.push_back(last_point);
			}
		} while (code != PlotterCodes::PEN_TERMINATING_POINT);
	}
}


const GPlatesModel::FeatureCollectionHandle::weak_ref
GPlatesFileIO::PlatesLineFormatReader::read_file(
		FileInfo &fileinfo,
		GPlatesModel::ModelInterface &model,
		ReadErrorAccumulation &read_errors)
{
	QString filename = fileinfo.get_qfileinfo().absoluteFilePath();

	// FIXME: We should replace usage of std::ifstream with the appropriate Qt class.
	std::ifstream input(filename.toAscii().constData());
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

	fileinfo.set_feature_collection(collection);
	return collection;
}
