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

#include "PlatesPostParseTranslator.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>  /* transform */
#include <memory>  /* std::auto_ptr */
#include <iomanip>
#include <iterator>

#include "util/StringUtils.h"
#include "maths/LatLonPointConversions.h"
#include "fileio/PlatesBoundaryParser.h"
#include "fileio/FileFormatException.h"
#include "fileio/ReadErrors.h"
#include "global/types.h"  /* rid_t */

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

using namespace GPlatesFileIO;

// Some conventions used in this file are inconsistent with the style guide
namespace {

	// List of LatLonPoint Lists. 
	typedef std::list<std::list<PlatesParser::LatLonPoint> > LLLPL_t;

	void
	convert_polyline_to_LLLPL(
			const PlatesParser::Polyline &line,
			LLLPL_t &plate_segments)
	{
		std::list<PlatesParser::LatLonPoint> llpl;

		std::list<PlatesParser::BoundaryLatLonPoint>::const_iterator 
			iter = line.d_points.begin();

		if (iter == line.d_points.end()) {
			throw ReadErrors::MissingPlatesPolylinePoints;
		}
			
		// Handle first point
		llpl.push_back(iter->d_lat_lon_point);
		++iter;

		for ( ; iter != line.d_points.end(); ++iter) {
			if (iter->d_plotter_code == PlatesParser::PlotterCodes::PEN_UP) {
				// We need to split this line, so push_back a copy of the line so far
				plate_segments.push_back(llpl);
				llpl.clear();  // start a new segment
			}
			llpl.push_back(iter->d_lat_lon_point);
		}
		
		// There should always be one segment left to add.
		if (llpl.size()) {
			plate_segments.push_back(llpl);
		}
	}
	
	// The Model requires a vector of double, so we convert the LLLP
	void 
	convert_LLPL_to_vector(
			std::vector< double >& result,
			const std::list< PlatesParser::LatLonPoint >& points)
	{
		for (std::list< PlatesParser::LatLonPoint>::const_iterator iter = 
				points.begin(),	end = points.end();	iter != end; ++iter) 
		{		
			result.push_back(iter->d_lon.dval());	
			result.push_back(iter->d_lat.dval());
		}
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
				header->clone(), "gpml:OldPlatesHeader", feature_handle);

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

		
	typedef GPlatesModel::FeatureHandle::weak_ref	
		(*creation_function_type)(
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
	create_old_plates_header(
			const std::string &first_line,
			const std::string &second_line)
	{
		typedef GPlatesModel::GpmlPlateId::integer_plate_id_type plate_id_type;
		using namespace GPlatesUtil;

		return GPlatesModel::GpmlOldPlatesHeader::create(
			slice_string<unsigned int>(ReadErrors::InvalidPlatesRegionNumber, first_line, 0, 2),
			slice_string<unsigned int>(ReadErrors::InvalidPlatesReferenceNumber, first_line, 2, 4),
			slice_string<unsigned int>(ReadErrors::InvalidPlatesStringNumber, first_line, 5, 9),
			slice_string<std::string>(ReadErrors::InvalidPlatesGeographicDescription, first_line, 10).c_str(),
			slice_string<plate_id_type>(ReadErrors::InvalidPlatesPlateIdNumber, second_line, 1, 4),
			slice_string<double>(ReadErrors::InvalidPlatesAgeOfAppearance, second_line, 5, 11),
			slice_string<double>(ReadErrors::InvalidPlatesAgeOfDisappearance, second_line, 12, 18),
			slice_string<std::string>(ReadErrors::InvalidPlatesDataTypeCode, second_line, 19, 21).c_str(),
			slice_string<unsigned int>(ReadErrors::InvalidPlatesDataTypeCodeNumber, second_line, 21, 25),
			slice_string<std::string>(ReadErrors::InvalidPlatesDataTypeCodeNumberAdditional, second_line, 25, 26).c_str(),
			slice_string<plate_id_type>(ReadErrors::InvalidPlatesConjugatePlateIdNumber, second_line, 26, 29),
			slice_string<unsigned int>(ReadErrors::InvalidPlatesColourCode, second_line, 30, 33),
			slice_string<unsigned int>(ReadErrors::InvalidPlatesNumberOfPoints, second_line, 34, 39));
	}

	void
	add_plate_data(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			const PlatesParser::Plate &plate,
			const boost::shared_ptr<DataSource> &source,
			ReadErrorAccumulation &errors)
	{
		static const creation_map_type &map = build_feature_creation_map();
		const boost::shared_ptr<LocationInDataSource> location(new LineNumberInFile(0));

		typedef std::list<PlatesParser::Polyline>::const_iterator 
				polyline_iterator_type;

		polyline_iterator_type iter;
		for (iter = plate.d_polylines.begin(); iter != plate.d_polylines.end(); ++iter) {
			try {
				GPlatesModel::GpmlOldPlatesHeader::non_null_ptr_type old_plates_header =
						create_old_plates_header(iter->d_header.d_first_line, iter->d_header.d_second_line);

				const boost::shared_ptr<LocationInDataSource> location(
					new LineNumberInFile(iter->d_line_number));

				creation_map_const_iterator result = map.find(old_plates_header->data_type_code());
				creation_function_type creation_function = create_unclassified_feature;
			
				if (result != map.end()) {
					creation_function = result->second;
				} else {
					errors.d_warnings.push_back(ReadErrorOccurrence(source, location, 
							ReadErrors::UnknownPlatesDataTypeCode, ReadErrors::UnclassifiedFeatureCreated));
				}
	
				LLLPL_t plate_segments;
				convert_polyline_to_LLLPL(*iter, plate_segments);
	
				for (LLLPL_t::const_iterator i = plate_segments.begin(), 
						end_segment = plate_segments.end(); i != end_segment; ++i) 
				{			
					std::vector<double> points;
					convert_LLPL_to_vector(points, *i);
			
					creation_function(model, collection, old_plates_header, points);
				}
			} catch (ReadErrors::Description error) {
				errors.d_recoverable_errors.push_back(ReadErrorOccurrence(source, location,
						error, ReadErrors::FeatureDiscarded));
			}
		}

		if (iter == plate.d_polylines.begin()) {
			errors.d_recoverable_errors.push_back(ReadErrorOccurrence(source, location,
					ReadErrors::MissingPlatesPolylines, ReadErrors::FeatureDiscarded));
		}
	}


	void
	add_rotation_data(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			const PlatesParser::RotationSequence &rot,
			ReadErrorAccumulation &errors)
	{
		const unsigned long fixed_plate_id = rot.d_fixed_plate;
		const unsigned long moving_plate_id = rot.d_moving_plate;

		typedef std::vector<GPlatesModel::ModelUtility::TotalReconstructionPoleData> 
				reconstruction_vector_type;

		reconstruction_vector_type five_tuples;

		for (std::list<PlatesParser::FiniteRotation>::const_iterator iter = rot.d_seq.begin(); 
				iter != rot.d_seq.end(); ++iter) 
		{
			GPlatesModel::ModelUtility::TotalReconstructionPoleData total_recon_pole_data = {
				iter->d_time.dval(), 
				iter->d_rot.d_pole.d_lat.dval(), 
				iter->d_rot.d_pole.d_lon.dval(), 
				iter->d_rot.d_angle.dval(), 
				iter->d_comment.c_str()
			};

			five_tuples.push_back(total_recon_pole_data);
		}

		GPlatesModel::ModelUtility::create_total_recon_seq(model, collection,
				fixed_plate_id, moving_plate_id, five_tuples);
	}
	
} // End anonymous namespace


namespace GPlatesFileIO
{

namespace PlatesPostParseTranslator
{
	GPlatesModel::FeatureCollectionHandle::weak_ref
	get_features_from_plates_data(
			GPlatesModel::ModelInterface &model,
			const PlatesParser::PlatesDataMap &map,
			const std::string &filename,
			ReadErrorAccumulation &errors)
	{
		boost::shared_ptr<DataSource> source( 
				new LocalFileDataSource(filename, DataFormats::PlatesLine));

		GPlatesModel::FeatureCollectionHandle::weak_ref collection
			= model.create_feature_collection();

		for (PlatesParser::PlatesDataMap::const_iterator iter = map.begin(),
				end = map.end(); iter != end; ++iter) 
		{ 
			add_plate_data(model, collection, iter->second, source, errors);
		}

		return collection;
	}

	GPlatesModel::FeatureCollectionHandle::weak_ref
	get_rotation_sequences_from_plates_data(
			GPlatesModel::ModelInterface &model,
			const PlatesParser::PlatesRotationData &list,
			const std::string &filename,
			ReadErrorAccumulation &errors)
	{
		GPlatesModel::FeatureCollectionHandle::weak_ref collection
			= model.create_feature_collection();

		for (PlatesParser::PlatesRotationData::const_iterator iter = 
				list.begin(), end = list.end(); iter != end; ++iter) 
		{
			add_rotation_data(model, collection, *iter, errors);		
		}	

		return collection;
	}
	
} // End namespace PlatesPostParseTranslator

} // End namespace GPlatesFileIO
