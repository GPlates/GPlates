/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 The University of Sydney, Australia
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

#include <QDebug>
#include <QString>

#include "ReadErrors.h"
#include "LineReader.h"

#include "global/types.h"

#include "utils/StringUtils.h"
#include "utils/MathUtils.h"

#include "maths/LatLonPointConversions.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "model/FeatureRevision.h"
#include "model/InlinePropertyContainer.h"
#include "model/DummyTransactionHandle.h"
#include "model/ModelUtils.h"

#include "feature-visitors/PlateIdFinder.h"
#include "feature-visitors/GmlTimePeriodFinder.h"
#include "feature-visitors/XsStringFinder.h"
#include "feature-visitors/ViewFeatureGeometriesWidgetPopulator.h"

#include "property-values/Enumeration.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GmlTimeInstant.h"

#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlFeatureReference.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlRevisionId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/TemplateTypeParameterType.h"
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


	typedef std::map<std::string, GPlatesModel::FeatureId> old_id_to_new_id_map_type;
	typedef old_id_to_new_id_map_type::const_iterator old_id_to_new_id_map_const_iterator;

	old_id_to_new_id_map_type id_map;



	//
	//
	//
	GPlatesPropertyValues::GpmlTopologicalPoint::non_null_ptr_type
	create_gpml_topological_point( 
		std::string old_fid)
	{
		// FIXME: what to do if the old_fid is not found?
		const GPlatesModel::FeatureId fid = (id_map.find( old_fid ) )->second; 

		const GPlatesModel::PropertyName name =
			GPlatesModel::PropertyName::create_gpml("position");

		const GPlatesPropertyValues::TemplateTypeParameterType value =
			GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Point" );

		GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type pd_ptr =
			GPlatesPropertyValues::GpmlPropertyDelegate::create(
				fid, name, value);

		// Create a GpmlTopologicalLineSection from the delegate
		GPlatesPropertyValues::GpmlTopologicalPoint::non_null_ptr_type gtp_ptr = 
			GPlatesPropertyValues::GpmlTopologicalPoint::create(pd_ptr);

		return gtp_ptr;
	}
	
	//
	//
	//
	GPlatesPropertyValues::GpmlTopologicalLineSection::non_null_ptr_type
	create_gpml_topological_line_section(
		std::string old_fid,
		bool use_reverse)
	{
		// Create a sourceGeometry property delegate
		// FIXME: what to do if the old_fid is not found?
		const GPlatesModel::FeatureId fid = (id_map.find( old_fid ) )->second; 

		const GPlatesModel::PropertyName name =
			GPlatesModel::PropertyName::create_gpml( "centerLineOf" );

		const GPlatesPropertyValues::TemplateTypeParameterType value =
			GPlatesPropertyValues::TemplateTypeParameterType::create_gml("LineString");
		GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type pd_ptr =
			GPlatesPropertyValues::GpmlPropertyDelegate::create(
				fid, name, value);

		// Create a GpmlTopologicalLineSection from the delegate
		GPlatesPropertyValues::GpmlTopologicalLineSection::non_null_ptr_type 
		gtls_ptr =
			GPlatesPropertyValues::GpmlTopologicalLineSection::create(
				pd_ptr,
				boost::none,
				boost::none,
				use_reverse);

		return gtls_ptr;
	}


	
	//
	//
	//
	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
	create_gpml_topological_sections_vector(
		GPlatesModel::FeatureHandle::weak_ref feature_ref, 
		std::vector<std::string> boundary_strings)
	{
		// vars to hold working data 
		std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
			topo_section_ptrs_vector;

		// iterator for the list of strings
		std::vector<std::string>::iterator iter = boundary_strings.begin(); 

		// numeric indices to the boundary list:
		// i is the current item, a.k.a iter
		// p and n, are the previous and next items on the list 
		int i = 0;
		int p = 0;
		int n = 0;

		// loop over the list of strings
		for ( ; iter != boundary_strings.end() ; ++iter, ++i) 
		{
			// the string to parse 
			std::string node_str = *iter;

			// index math to close the loop
			if ( iter == --(boundary_strings.end()) ) {
				n = 0;
				p = i - 1;
			} else if ( iter == boundary_strings.begin() ) {
				n = i + 1;
				p = boundary_strings.size() - 1;
			} else {
				n = i + 1;
				p = i - 1;
			}
#if 0
// FIXME : remove diagnostic 
std::cout << "p=" << p << " ; i=" << i << " ; n=" << n  << std::endl; 
std::cout << "iter = " << node_str << std::endl; 
#endif

			// tmp place to hold parsed sub-strings
			std::vector<std::string> tokens;

			// loop over the string finding hash delimiters
			while (node_str.find("#", 0) != std::string::npos)
			{ 
				// store the position of the delimiter
				size_t pos = node_str.find("#", 0);

				// get the token
				std::string temp = node_str.substr(0, pos);

				// erase it from the source 
				node_str.erase(0, pos + 1);

				// and put it into the array
				tokens.push_back(temp);
			}

			// the last token is all alone, but has delimiter at end, remove it
			tokens.push_back(node_str);

//FIXME: synch with file levelerror checking
			// Error checking on number of tokens found
			if ( tokens.size() != 11 )
			{
				std::ostringstream oss;
				oss << "Cannot parse boundary feature node line: "
					<< "expected 10 comma delimited tokens, "
					<< "got: " << tokens.size() << " tokens."
					<< std::endl;
				std::cout << "ERROR: " << oss.str() << std::endl;
				continue;
			}

			// convert token strings into topology parameters
			int type;
			float lat;
			float lon;
			float closeness;
			bool use_reverse;
			bool use_head_prev;
			bool use_tail_prev;
			bool use_head_next;
			bool use_tail_next;

			std::string old_fid 	  = tokens[0];
			std::istringstream iss_type(tokens[1]) ; iss_type >> type;
			std::istringstream iss_lat( tokens[2] ); iss_lat >> lat;
			std::istringstream iss_lon( tokens[3] ); iss_lon >> lon;
			std::istringstream iss_clo( tokens[4] ); iss_clo >> closeness; 
			std::istringstream iss_rev( tokens[5] ); iss_rev >> use_reverse; 
			std::istringstream iss_uhp( tokens[6] ); iss_uhp >> use_head_prev; 
			std::istringstream iss_utp( tokens[7] ); iss_utp >> use_tail_prev; 
			std::istringstream iss_uhn( tokens[8] ); iss_uhn >> use_head_next; 
			std::istringstream iss_utn( tokens[9] ); iss_utn >> use_tail_next;

#if 0
// FIXME : remove diagnostic 
std::cout << "old_fid = " << old_fid << std::endl;
std::cout << "lat = " << lat << std::endl;
std::cout << "lon = " << lon << std::endl;
std::cout << "use_reverse = " << use_reverse << std::endl;
std::cout << "use_head_prev = " << use_head_prev << std::endl;
std::cout << "use_tail_prev = " << use_tail_prev << std::endl;
std::cout << "use_head_next = " << use_head_next << std::endl;
std::cout << "use_tail_next = " << use_tail_next << std::endl;
#endif
			// convert type
			GPlatesGlobal::FeatureTypes feature_type(GPlatesGlobal::UNKNOWN_FEATURE);

			switch ( type )
			{
				case GPlatesGlobal::POINT_FEATURE:
					feature_type = GPlatesGlobal::POINT_FEATURE;
					// Fill the vector of GpmlTopologicalSection::non_null_ptr_type
					topo_section_ptrs_vector.push_back( 
						create_gpml_topological_point( old_fid ) );
					break;

				case GPlatesGlobal::LINE_FEATURE:
					feature_type = GPlatesGlobal::LINE_FEATURE;
					// Fill the vector of GpmlTopologicalSection::non_null_ptr_type
					topo_section_ptrs_vector.push_back( 
						create_gpml_topological_line_section( old_fid, use_reverse ) );

				default :
					break;
			}

			// create intersections if needed....

			// convert coordinates
			GPlatesMaths::LatLonPoint llp( lat, lon);
			GPlatesMaths::PointOnSphere pos = GPlatesMaths::make_point_on_sphere(llp);
			const GPlatesMaths::PointOnSphere const_pos(pos);
			GPlatesPropertyValues::GmlPoint::non_null_ptr_type ref_point =
				GPlatesPropertyValues::GmlPoint::create( const_pos );

			// Check for intersections with prev item on the list
			if (use_head_prev or use_tail_prev)
			{
				// get the PREV feature id
				std::string str = boundary_strings.at( p );
				size_t pos = str.find("#", 0);
				std::string prev_old_fid = str.substr(0, pos);
				// FIXME: what to do if the prev_old_fid is not found?
				const GPlatesModel::FeatureId prev_fid = (id_map.find(prev_old_fid))->second; 

				const GPlatesModel::PropertyName prop_name1 =
					GPlatesModel::PropertyName::create_gpml("centerLineOf");
				
				const GPlatesPropertyValues::TemplateTypeParameterType value_type1 =
					GPlatesPropertyValues::TemplateTypeParameterType::create_gml("LineString");
		
				// create the intersectionGeometry property delegate
				GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type geom_delegte = 
					GPlatesPropertyValues::GpmlPropertyDelegate::create( 
						prev_fid,
						prop_name1,
						value_type1
					);
		
				// reference_point
				 GPlatesPropertyValues::GmlPoint::non_null_ptr_type ref_point =
					GPlatesPropertyValues::GmlPoint::create( const_pos );
		
				// reference_point_plate_id
				// FIXME: what to do if the old_fid is not found?
				const GPlatesModel::FeatureId index_fid = (id_map.find( old_fid ) )->second; 

				const GPlatesModel::PropertyName prop_name2 =
					GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
		
				const GPlatesPropertyValues::TemplateTypeParameterType value_type2 =
					GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("PlateId" );
		
				GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type plate_id_delegate = 
					GPlatesPropertyValues::GpmlPropertyDelegate::create( 
						index_fid,
						prop_name2,
						value_type2
					);
		
				// Create the start GpmlTopologicalIntersection
				GPlatesPropertyValues::GpmlTopologicalIntersection start_ti(
					geom_delegte,
					ref_point,
					plate_id_delegate);
					
				// Set the start instersection
				GPlatesPropertyValues::GpmlTopologicalLineSection* gtls_ptr =
					dynamic_cast<GPlatesPropertyValues::GpmlTopologicalLineSection*>(
						topo_section_ptrs_vector.at( i ).get() 
					);
		
				gtls_ptr->set_start_intersection( start_ti );
			}

			// check for endIntersection
			if (use_head_next or use_tail_next)
			{
				// get the next feature id
				std::string str = boundary_strings.at( n );
				size_t pos = str.find("#", 0);
				std::string next_old_fid = str.substr(0, pos);
				// FIXME: what to do if the next_old_fid is not found?
				const GPlatesModel::FeatureId next_fid = (id_map.find(next_old_fid))->second; 

				const GPlatesModel::PropertyName prop_name1 =
					GPlatesModel::PropertyName::create_gpml("centerLineOf");
				
				const GPlatesPropertyValues::TemplateTypeParameterType value_type1 =
					GPlatesPropertyValues::TemplateTypeParameterType::create_gml("LineString");
		
				// create the intersectionGeometry property delegate
				GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type geom_delegte = 
					GPlatesPropertyValues::GpmlPropertyDelegate::create( 
						next_fid,
						prop_name1,
						value_type1
					);
		
				// reference_point
				 GPlatesPropertyValues::GmlPoint::non_null_ptr_type ref_point =
					GPlatesPropertyValues::GmlPoint::create( const_pos );
		
				// reference_point_plate_id
				// FIXME: what to do if the old_fid is not found?
				const GPlatesModel::FeatureId index_fid = (id_map.find( old_fid ) )->second; 

				const GPlatesModel::PropertyName prop_name2 =
					GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
		
				const GPlatesPropertyValues::TemplateTypeParameterType value_type2 =
					GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("PlateId" );
		
				GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type plate_id_delegate = 
					GPlatesPropertyValues::GpmlPropertyDelegate::create( 
						index_fid,
						prop_name2,
						value_type2
					);
		
				// Create the start GpmlTopologicalIntersection
				GPlatesPropertyValues::GpmlTopologicalIntersection end_ti(
					geom_delegte,
					ref_point,
					plate_id_delegate);
					
				// Set the start instersection
				GPlatesPropertyValues::GpmlTopologicalLineSection* gtls_ptr =
					dynamic_cast<GPlatesPropertyValues::GpmlTopologicalLineSection*>(
						topo_section_ptrs_vector.at( i ).get() 
					);
		
				gtls_ptr->set_end_intersection( end_ti );
			}

		} // end of loop over boundary 

		return topo_section_ptrs_vector;
	}


	//
	//
	//
	GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type
	create_gpml_piecewise_aggregation( 
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		std::vector<std::string> boundary_strings )
	{
		// vars to hold working data 
		std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
			topo_section_ptrs_vector =
				create_gpml_topological_sections_vector(
					feature_ref,
					boundary_strings);

		// create the TopologicalPolygon
		GPlatesModel::PropertyValue::non_null_ptr_type topo_poly_value =
			GPlatesPropertyValues::GpmlTopologicalPolygon::create(
				topo_section_ptrs_vector);
	
		const GPlatesPropertyValues::TemplateTypeParameterType topo_poly_type =
			GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("TopologicalPolygon");
	
		// create the ConstantValue
		GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type constant_value =
			GPlatesPropertyValues::GpmlConstantValue::create(
				topo_poly_value, 
				topo_poly_type);
	
		// get the time period for the feature_ref
		// Valid Time (Assuming a gml:TimePeriod, rather than a gml:TimeInstant!)
		static const GPlatesModel::PropertyName name =
			GPlatesModel::PropertyName::create_gml("validTime");
	
		GPlatesFeatureVisitors::GmlTimePeriodFinder time_period_finder(name);
		time_period_finder.visit_feature_handle( *feature_ref );
	
		GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type time_period = 
			*time_period_finder.found_time_periods_begin();
	
		//GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type *tp = 
		GPlatesPropertyValues::GmlTimePeriod* tp = 
		const_cast<GPlatesPropertyValues::GmlTimePeriod *>( time_period.get() );
	
		// GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type ttpp =
		GPlatesUtils::non_null_intrusive_ptr<
			GPlatesPropertyValues::GmlTimePeriod, 
			GPlatesUtils::NullIntrusivePointerHandler> ttpp(
					tp,
					GPlatesUtils::NullIntrusivePointerHandler()
			);
	
		// create the TimeWindow
		GPlatesPropertyValues::GpmlTimeWindow tw = GPlatesPropertyValues::GpmlTimeWindow(
				constant_value, 
				ttpp,
				topo_poly_type);
	
		// use the time window
		std::vector<GPlatesPropertyValues::GpmlTimeWindow> time_windows;
	
		time_windows.push_back(tw);
	
		// create and return the PiecewiseAggregation
		GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type aggregation =
			GPlatesPropertyValues::GpmlPiecewiseAggregation::create(
				time_windows, 
				topo_poly_type);
		
		return aggregation;
	}

		
	/**
	 * This function assumes that 'create_feature_with_geometry' has ensured that 'points'
	 * contains at least one point.
	 */
	void
	append_appropriate_geometry(
			const std::list<GPlatesMaths::PointOnSphere> &points,
			const GPlatesModel::PropertyName &property_name,
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

			PolylineOnSphere::non_null_ptr_to_const_type polyline =
					PolylineOnSphere::create_on_heap(points);
			GPlatesPropertyValues::GmlLineString::non_null_ptr_type gml_line_string =
					GPlatesPropertyValues::GmlLineString::create(polyline);
			GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type gml_orientable_curve =
					GPlatesModel::ModelUtils::create_gml_orientable_curve(gml_line_string);
			GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
					GPlatesModel::ModelUtils::create_gpml_constant_value(
							gml_orientable_curve, 
							GPlatesPropertyValues::TemplateTypeParameterType::create_gml("OrientableCurve"));
			GPlatesModel::ModelUtils::append_property_value_to_feature(property_value,
					property_name, feature);
		} else if (num_distinct_adj_points == 1) {
			// It's a point.
			GPlatesPropertyValues::GmlPoint::non_null_ptr_type gml_point =
					GPlatesPropertyValues::GmlPoint::create(points.front());

			// If it's a point, then we're going to check if the passed property type is gpml:position.
			// If it *is* gpml:position, then we don't want to wrap it in a constant value.
			// If it's *not* gpml:position, then we can go ahead and wrap it in a constant value.
			static const GPlatesModel::PropertyName gpml_position = GPlatesModel::PropertyName::create_gpml("position");

			if (property_name == gpml_position)
			{
				// Don't wrap it in a constant value, just add it directly.
				GPlatesModel::ModelUtils::append_property_value_to_feature(gml_point,
					property_name, feature);
			}
			else
			{
				// Wrap it in a constant value.
				GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
					GPlatesModel::ModelUtils::create_gpml_constant_value(
							gml_point, 
							GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Point"));
				GPlatesModel::ModelUtils::append_property_value_to_feature(property_value,
					property_name, feature);
			}

		} else {
			// FIXME:  A pre-condition of this function has been violated.  We should
			// throw an exception.

			//
			// NOTE: this is actually fine, since topology type features don't have a geom.
			//
		}
	}


	const GPlatesPropertyValues::GeoTimeInstant
	create_geo_time_instant(
			const double &time)
	{
		if (time < -998.9 && time > -1000.0) {
			// It's in the distant future, which is denoted in PLATES4 line-format
			// files using times like -999.0 or -999.9.
			return GPlatesPropertyValues::GeoTimeInstant::create_distant_future();
		}
		if (time > 998.9 && time < 1000.0) {
			// It's in the distant past, which is denoted in PLATES4 line-format files
			// using times like 999.0 or 999.9.
			return GPlatesPropertyValues::GeoTimeInstant::create_distant_past();
		}
		return GPlatesPropertyValues::GeoTimeInstant(time);
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_common(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points,
			const GPlatesModel::FeatureType &feature_type,
			const GPlatesModel::PropertyName &geometry_property_name)
	{
		using namespace GPlatesPropertyValues;
		using namespace GPlatesModel;

		FeatureHandle::weak_ref feature_handle =
				model.create_feature(feature_type, collection);

		const integer_plate_id_type plate_id = header->plate_id_number();
		const GeoTimeInstant geo_time_instant_begin(
				create_geo_time_instant(header->age_of_appearance()));
		const GeoTimeInstant geo_time_instant_end(
				create_geo_time_instant(header->age_of_disappearance()));

		// Wrap a "gpml:plateId" in a "gpml:ConstantValue" and append it as the
		// "gpml:reconstructionPlateId" property.
		GpmlPlateId::non_null_ptr_type recon_plate_id = GpmlPlateId::create(plate_id);
		ModelUtils::append_property_value_to_feature(
				ModelUtils::create_gpml_constant_value(recon_plate_id, 
					TemplateTypeParameterType::create_gpml("plateId")),
				PropertyName::create_gpml("reconstructionPlateId"),
				feature_handle);

		append_appropriate_geometry(points, geometry_property_name, feature_handle);

		GmlTimePeriod::non_null_ptr_type gml_valid_time =
				ModelUtils::create_gml_time_period(geo_time_instant_begin, geo_time_instant_end);
		ModelUtils::append_property_value_to_feature(
				gml_valid_time, 
				PropertyName::create_gml("validTime"), 
				feature_handle);

		// Use the PLATES4 geographic description as the "gml:name" property.
		XsString::non_null_ptr_type gml_name = 
				XsString::create(header->geographic_description());
		ModelUtils::append_property_value_to_feature(
				gml_name, 
				PropertyName::create_gml("name"), 
				feature_handle);

		ModelUtils::append_property_value_to_feature(
				header->clone(), 
				PropertyName::create_gpml("oldPlatesHeader"), 
				feature_handle);


		// file the map with id data 
		std::string s = header->old_feature_id();
		const GPlatesModel::FeatureId &fid = feature_handle->feature_id();
		id_map.insert( std::make_pair(s, fid) );

		return feature_handle;
	}


	/**
	 * Creates a GPML feature from PLATES data where the GPML feature accepts a single point only.
	 * Polylines don't make sense here, and should cause a warning.
	 */
	GPlatesModel::FeatureHandle::weak_ref	
	create_single_point_feature(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points,
			const GPlatesModel::FeatureType &feature_type,
			const GPlatesModel::PropertyName &geometry_property_name)
	{
		// Check for invalid data
		std::list<GPlatesMaths::PointOnSphere>::size_type num_distinct_adj_points =
				GPlatesMaths::count_distinct_adjacent_points(points);
		if (num_distinct_adj_points > 1) {
			// FIXME: This will be counted as an error, be caught down in read_file, 
			// and nuke the feature. This is a little harsh, but it's the best we can do for now.
			throw GPlatesFileIO::ReadErrors::MoreThanOneDistinctPoint;
		}
		// Assume create_common will do the right thing with append_appropriate_geometry.
		return create_common(model, collection, header, points, feature_type, geometry_property_name);
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_fault(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points, 
				GPlatesModel::FeatureType::create_gpml("Fault"), 
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_normal_fault(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
				create_fault(model, collection, header, points);
		
		const GPlatesPropertyValues::Enumeration::non_null_ptr_type dip_slip_property_value =
				GPlatesPropertyValues::Enumeration::create("gpml:DipSlipEnumeration", "Compression");
		GPlatesModel::ModelUtils::append_property_value_to_feature(
				dip_slip_property_value, 
				GPlatesModel::PropertyName::create_gpml("dipSlip"), 
				feature_handle);

		return feature_handle;
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_reverse_fault(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
				create_fault(model, collection, header, points);
		
		const GPlatesPropertyValues::Enumeration::non_null_ptr_type dip_slip_property_value =
				GPlatesPropertyValues::Enumeration::create("gpml:DipSlipEnumeration", "Extension");
		GPlatesModel::ModelUtils::append_property_value_to_feature(
				dip_slip_property_value, 
				GPlatesModel::PropertyName::create_gpml("dipSlip"), 
				feature_handle);

		return feature_handle;
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

		const GPlatesPropertyValues::XsString::non_null_ptr_type subcategory_property_value =
				GPlatesPropertyValues::XsString::create("Thrust");
		GPlatesModel::ModelUtils::append_property_value_to_feature(
				subcategory_property_value, 
				GPlatesModel::PropertyName::create_gpml("subcategory"), 
				feature_handle);

		return feature_handle;
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_strike_slip_fault(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
				create_fault(model, collection, header, points);
		
		const GPlatesPropertyValues::Enumeration::non_null_ptr_type strike_slip_property_value =
				GPlatesPropertyValues::Enumeration::create("gpml:StrikeSlipEnumeration", "Unknown");
		GPlatesModel::ModelUtils::append_property_value_to_feature(
				strike_slip_property_value, 
				GPlatesModel::PropertyName::create_gpml("strikeSlip"), 
				feature_handle);

		return feature_handle;
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_unclassified_feature(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points, 
				GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature"), 
				GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_aseismic_ridge(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points, 
				GPlatesModel::FeatureType::create_gpml("AseismicRidge"), 
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_bathymetry(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		// FIXME: Set up a method to construct gpml:Contours and use them as the geometry, sourcing
		// the appropriate PLATES header data.
		return create_common(model, collection, header, points, 
				GPlatesModel::FeatureType::create_gpml("Bathymetry"), 
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_basin(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points, 
				GPlatesModel::FeatureType::create_gpml("Basin"), 
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_coastline(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points, 
				GPlatesModel::FeatureType::create_gpml("Coastline"), 
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_continental_boundary(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points, 
				GPlatesModel::FeatureType::create_gpml("PassiveContinentalBoundary"), 
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_continental_fragment(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points, 
				GPlatesModel::FeatureType::create_gpml("ContinentalFragment"), 
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_craton(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points, 
				GPlatesModel::FeatureType::create_gpml("Craton"), 
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_extended_continental_crust(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points, 
				GPlatesModel::FeatureType::create_gpml("ExtendedContinentalCrust"), 
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_fracture_zone(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points, 
				GPlatesModel::FeatureType::create_gpml("FractureZone"), 
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_gravimetry(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		// FIXME: Set up a method to construct gpml:Contours and use them as the geometry, sourcing
		// the appropriate PLATES header data.
		return create_common(model, collection, header, points, 
				GPlatesModel::FeatureType::create_gpml("Gravimetry"), 
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_grid_mark(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		// FIXME: This will create lots of gpml:OldPlatesGridMarks if the source GR feature uses
		// lots of pen up pen down commands. A way to specify use of gml:MultiCurve would be nice.
		return create_common(model, collection, header, points, 
				GPlatesModel::FeatureType::create_gpml("OldPlatesGridMark"), 
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_heat_flow(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		// FIXME: Set up a method to construct gpml:Contours and use them as the geometry, sourcing
		// the appropriate PLATES header data.
		return create_common(model, collection, header, points, 
				GPlatesModel::FeatureType::create_gpml("HeatFlow"), 
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_hot_spot(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_single_point_feature(model, collection, header, points, 
				GPlatesModel::FeatureType::create_gpml("HotSpot"), 
				GPlatesModel::PropertyName::create_gpml("position"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_hot_spot_trail(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("HotSpotTrail"),
				GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_inferred_paleo_boundary(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("InferredPaleoBoundary"),
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_island_arc(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points,
			bool is_active)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
			create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("IslandArc"),
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
		
#if 0
		const GPlatesPropertyValues::XsBoolean::non_null_ptr_type is_active_property_value =
				GPlatesPropertyValues::XsBoolean::create(is_active);
		GPlatesModel::ModelUtils::append_property_value_to_feature(
				is_active_property_value, 
				GPlatesModel::PropertyName::create_gpml("isActive"), 
				feature_handle);
#endif

		return feature_handle;
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_island_arc_active(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_island_arc(model, collection, header, points, true);
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_island_arc_inactive(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_island_arc(model, collection, header, points, false);
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_isochron(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		GPlatesModel::FeatureHandle::weak_ref feature =
		   	create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("Isochron"),
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
		const GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type conj_plate_id =
				GPlatesPropertyValues::GpmlPlateId::create(header->conjugate_plate_id_number());
		GPlatesModel::ModelUtils::append_property_value_to_feature(
				conj_plate_id, 
				GPlatesModel::PropertyName::create_gpml("conjugatePlateId"), 
				feature);
		return feature;
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_isopach(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		// FIXME: Set up a method to construct gpml:Contours and use them as the geometry, sourcing
		// the appropriate PLATES header data.
		return create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("SedimentThickness"),
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_geological_lineation(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("GeologicalLineation"),
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_magnetics(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		// FIXME: Set up a method to construct gpml:Contours and use them as the geometry, sourcing
		// the appropriate PLATES header data.
		return create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("Magnetics"),
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_magnetic_pick(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		// FIXME: fill in the rest of MagneticAnomalyIdentification from the appropriate PLATES header data,
		// assuming it is available.
		return create_single_point_feature(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("MagneticAnomalyIdentification"),
				GPlatesModel::PropertyName::create_gpml("position"));
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
			create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("MidOceanRidge"),
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
#if 0
		const GPlatesPropertyValues::XsBoolean::non_null_ptr_type is_active_property_value =
				GPlatesPropertyValues::XsBoolean::create(is_active);
		GPlatesModel::ModelUtils::append_property_value_to_feature(
				is_active_property_value, 
				GPlatesModel::PropertyName::create_gpml("isActive"), 
				feature_handle);
#endif

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
	create_ophiolite_belt(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
				create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("BasicRockUnit"),
				GPlatesModel::PropertyName::create_gpml("outlineOf"));

		const GPlatesPropertyValues::XsString::non_null_ptr_type subcategory_property_value =
				GPlatesPropertyValues::XsString::create("Ophiolite");
		GPlatesModel::ModelUtils::append_property_value_to_feature(
				subcategory_property_value, 
				GPlatesModel::PropertyName::create_gpml("subcategory"), 
				feature_handle);
		
		return feature_handle;
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_orogenic_belt(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("OrogenicBelt"),
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_seamount(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("Seamount"),
				GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_subduction_zone(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points,
			bool is_active)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
				create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("SubductionZone"),
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
		
#if 0
		const GPlatesPropertyValues::XsBoolean::non_null_ptr_type is_active_property_value =
				GPlatesPropertyValues::XsBoolean::create(is_active);
		GPlatesModel::ModelUtils::append_property_value_to_feature(
				is_active_property_value, 
				GPlatesModel::PropertyName::create_gpml("isActive"), 
				feature_handle);
#endif

		return feature_handle;
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_subduction_zone_active(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_subduction_zone(model, collection, header, points, true);
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_subduction_zone_inactive(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_subduction_zone(model, collection, header, points, false);
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_suture(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("Suture"),
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_terrane_boundary(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("TerraneBoundary"),
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_transitional_crust(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("TransitionalCrust"),
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_transform(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("Transform"),
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_topography(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		// FIXME: Set up a method to construct gpml:Contours and use them as the geometry, sourcing
		// the appropriate PLATES header data.
		return create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("Topography"),
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_volcano(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("Volcano"),
				GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_large_igneous_province(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		return create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("LargeIgneousProvince"),
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_topological_closed_plate_boundary(
			GPlatesModel::ModelInterface &model, 
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const std::list<GPlatesMaths::PointOnSphere> &points)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
				create_common(model, collection, header, points,
				GPlatesModel::FeatureType::create_gpml("TopologicalClosedPlateBoundary"),
				GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));

		return feature_handle;
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
		map["AR"] = create_aseismic_ridge;
		map["BA"] = create_bathymetry;
		map["BS"] = create_basin;
		map["CB"] = create_continental_boundary;
		map["CF"] = create_continental_fragment;
		map["CM"] = create_continental_boundary;
		map["CO"] = create_continental_boundary;
		map["CR"] = create_craton;
		map["CS"] = create_coastline;
		map["EC"] = create_extended_continental_crust;
		map["FT"] = create_fault;
		map["FZ"] = create_fracture_zone;
		map["GR"] = create_grid_mark;
		map["GV"] = create_gravimetry;
		map["HF"] = create_heat_flow;
		map["HS"] = create_hot_spot;
		map["HT"] = create_hot_spot_trail;
		map["IA"] = create_island_arc_active;
		map["IC"] = create_isochron;
		map["IM"] = create_isochron;
		map["IP"] = create_isopach;
		map["IR"] = create_island_arc_inactive;
		map["IS"] = create_unclassified_feature; // -might- be Ice Shelf, might be Isochron. We don't know.
		map["LI"] = create_geological_lineation;
		map["MA"] = create_magnetics;
		map["NF"] = create_normal_fault;
		map["OB"] = create_orogenic_belt;
		map["OP"] = create_ophiolite_belt;
		map["OR"] = create_orogenic_belt;
		map["PB"] = create_inferred_paleo_boundary;
		map["PC"] = create_magnetic_pick;
		map["PM"] = create_magnetic_pick;
		map["PP"] = create_topological_closed_plate_boundary;
		map["RA"] = create_island_arc_inactive;
		map["RF"] = create_reverse_fault;
		map["RI"] = create_ridge_segment;
		map["SM"] = create_seamount;
		map["SS"] = create_strike_slip_fault;
		map["SU"] = create_suture;
		map["TB"] = create_terrane_boundary;
		map["TC"] = create_transitional_crust;
		map["TF"] = create_transform;
		map["TH"] = create_thrust_fault;
		map["TO"] = create_topography;
		map["TR"] = create_subduction_zone_active;
		map["sL"] = create_subduction_zone_active;
		map["sR"] = create_subduction_zone_active;
		map["UN"] = create_unclassified_feature;
		map["VO"] = create_volcano;
		map["VP"] = create_large_igneous_province;
		map["XR"] = create_extinct_ridge;
		map["XT"] = create_subduction_zone_inactive;
		return map;
	}


	void
	null_warning_function(
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			GPlatesFileIO::LineReader &in,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			GPlatesFileIO::ReadErrorAccumulation &errors)
	{  }

	void
	warning_unknown_data_type_code(
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			GPlatesFileIO::LineReader &in,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			GPlatesFileIO::ReadErrorAccumulation &errors)
	{
		const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
				new GPlatesFileIO::LineNumberInFile(in.line_number()));
		errors.d_warnings.push_back(GPlatesFileIO::ReadErrorOccurrence(source, location, 
				GPlatesFileIO::ReadErrors::UnknownPlatesDataTypeCode,
				GPlatesFileIO::ReadErrors::UnclassifiedFeatureCreated));
	}


	void
	warning_ice_shelf_ambiguity(
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			GPlatesFileIO::LineReader &in,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			GPlatesFileIO::ReadErrorAccumulation &errors)
	{
		const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
				new GPlatesFileIO::LineNumberInFile(in.line_number()));
		errors.d_warnings.push_back(GPlatesFileIO::ReadErrorOccurrence(source, location, 
				GPlatesFileIO::ReadErrors::AmbiguousPlatesIceShelfCode,
				GPlatesFileIO::ReadErrors::UnclassifiedFeatureCreated));
	}


	/**
	 * Warning functions are defined for any cases where additional ReadErrorOccurrences
	 * need to be added for specific PLATES data types.
	 */
	typedef void (*warning_function_type)(
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &,
			GPlatesFileIO::LineReader &,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &,
			GPlatesFileIO::ReadErrorAccumulation &);

	typedef std::map<UnicodeString, warning_function_type> warning_map_type;
	typedef warning_map_type::const_iterator warning_map_const_iterator;


	const warning_map_type &
	build_feature_specific_warning_map()
	{
		static warning_map_type map;
		map["IS"] = warning_ice_shelf_ambiguity;
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


	// This function reads a series of lines from the input file
	// and copies the data to a list of strings
	std::string 
	read_platepolygon_boundary_feature(
			GPlatesFileIO::LineReader &in,
			std::vector<std::string> &boundary_strings,
			std::string code)
	{
		std::string line;

		if ( ! in.getline(line)) 
		{
			// Since we're in this function, we're expecting to read a string.  
			// But we couldn't find one.  So, let's complain.
			throw GPlatesFileIO::ReadErrors::MissingPlatepolygonBoundaryFeature;
		}

		// 'NULL' is the end of platepolygon boundary list marker.
		if (line == "NULL") 
		{
			return "NULL";
		}

		// append the line to the list
		boundary_strings.push_back( line );

		return code;
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
		std::string first_line;
		if ( ! in.getline(first_line)) {
			return; // Do not want to throw here: end of file reached
		}

		GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type 
				old_plates_header = read_old_plates_header(in, first_line);
		
		// Look up any specific notes or warnings associated with this type code.
		static const warning_map_type &warning_map = build_feature_specific_warning_map();
		warning_function_type warning_function = null_warning_function;

		warning_map_const_iterator warning_result = warning_map.find(old_plates_header->data_type_code());
		if (warning_result != warning_map.end()) {
			warning_function = warning_result->second;
		}
		warning_function(old_plates_header, in, source, errors);
		
		// Locate the creation function for this type code.
		static const creation_map_type &creation_map = build_feature_creation_map();
		creation_function_type creation_function = create_unclassified_feature;

		creation_map_const_iterator creation_result = creation_map.find(old_plates_header->data_type_code());	
		if (creation_result != creation_map.end()) {
			creation_function = creation_result->second;
		} else {
			warning_unknown_data_type_code(old_plates_header, in, source, errors);
		}

		// Short-cut for Platepolygons (geometry to be resolved each reconstruction)
		if (old_plates_header->data_type_code() == "PP") 
		{
			// Empty list of points to make create_common a happy litte function 
			const std::list<GPlatesMaths::PointOnSphere> points;

			// a list of boundary feature lines to populate from 
			// read_platepolygon_boundary_feature()
			std::vector<std::string> boundary_strings;

			// read the first platepolygon boundary feature and 
			// set the terminator code to CONTINUE
			std::string code = "CONTINUE";
			read_platepolygon_boundary_feature(in, boundary_strings, code);

			// Loop over the platepolygon's boundary features, and build a list of strings
			do 
			{
				code = read_platepolygon_boundary_feature(in, boundary_strings, code);

				if (code == "NULL")
				{
					// We have reached end the boundary list, 

					// First create the the feature 
					// NOTE: this will call 'create_common'
					GPlatesModel::FeatureHandle::weak_ref feature_ref = 
						create_topological_closed_plate_boundary(
							model, collection, old_plates_header, points);
		
#if 0
// FIXME: remove this diagnostic 
GPlatesFeatureVisitors::XsStringFinder string_finder(
	GPlatesModel::PropertyName::create_gml("name") );
string_finder.visit_feature_handle( *feature_ref );
if (string_finder.found_strings_begin() != string_finder.found_strings_end()) 
{
	GPlatesPropertyValues::XsString::non_null_ptr_to_const_type name =
		 *string_finder.found_strings_begin();
qDebug() << "::ppend_boundary_topology: name=" << GPlatesUtils::make_qstring(name->value());
}

std::vector<std::string>::iterator iter = boundary_strings.begin(); 
for ( ; iter != boundary_strings.end() ; ++iter) 
{
	std::cout << "iter = " << *iter << std::endl;
}
#endif

					// Create the Gpml Piecewise Aggregation from the boundary list
					GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type agg =
						create_gpml_piecewise_aggregation( 
							feature_ref,
							boundary_strings);
		
					// Add a gpml:boundary Property.
					GPlatesModel::ModelUtils::append_property_value_to_feature(
						agg,
						GPlatesModel::PropertyName::create_gpml("boundary"),
						feature_ref);
				}

			} while (code != "NULL");

			return;
		}

		std::list<GPlatesMaths::PointOnSphere> points;
		read_polyline_point(in, points, PlotterCodes::PEN_SKIP_TO);

		// FIXME : Rather than create millions of little features for each unbroken
		// section of line, it would be better to create gml:MultiCurve geometry.
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
// end of anonymous name space


void
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
}
