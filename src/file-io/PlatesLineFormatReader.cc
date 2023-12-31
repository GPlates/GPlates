/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010 The University of Sydney, Australia
 * Copyright (C) 2008, 2009 California Institute of Technology 
 * Copyright (C) 2010 Geological Survey of Norway
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

#include <list>
#include <vector>
#include <boost/bind/bind.hpp>

#include <QDebug>
#include <QFile>
#include <QString>
#include <QTextStream>

#include "ReadErrors.h"
#include "LineReader.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "model/ChangesetHandle.h"
#include "model/FeatureRevision.h"
#include "model/Model.h"
#include "model/ModelUtils.h"
#include "model/TopLevelPropertyInline.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/Enumeration.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/XsBoolean.h"

#include "property-values/GpmlFeatureReference.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/StructuralType.h"

#include "utils/Profile.h"
#include "utils/StringUtils.h"
#include "utils/UnicodeStringUtils.h"


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
	 * Typedef for a sequence of points.
	 */
	typedef std::vector<GPlatesMaths::PointOnSphere> point_seq_type;

	/**
	 * Typedef for a sequence of geometries (each containing a sequence of points).
	 */
	typedef std::list<point_seq_type> geometry_seq_type;


	/**
	 * Typedef for map from old GP8 id to GP9 feature id 
	*/ 
	typedef std::map<QString, GPlatesModel::FeatureId> old_id_to_new_id_map_type;
	typedef old_id_to_new_id_map_type::const_iterator old_id_to_new_id_map_const_iterator;
	old_id_to_new_id_map_type id_map;

	
	/**
	 * Checks that @a geometry_seq is appropriate for constructing a multipoint.
	 *
	 * Returns true if the @a geometry_seq has at least two geometries, and 
	 * each of these geometries contains only a single point.
	 *
	 * Otherwise returns false.
	 */
	bool
	sequence_is_valid_multipoint(
		const geometry_seq_type &geometry_seq)
	{
		if (geometry_seq.size() < 2)
		{
			return false;
		}
	
		geometry_seq_type::const_iterator 
			it = geometry_seq.begin(),
			end = geometry_seq.end();

		for (; it != end ; ++it)
		{	
			if (GPlatesMaths::count_distinct_adjacent_points(*it) != 1)
			{
				return false;
			}
		}
		return true;
	}	


	//
	//
	//
	GPlatesPropertyValues::GpmlTopologicalPoint::non_null_ptr_type
	create_gpml_topological_point( 
		QString old_fid)
	{
		// FIXME: what to do if the old_fid is not found?
		const GPlatesModel::FeatureId fid = (id_map.find( old_fid ) )->second; 

		const GPlatesModel::PropertyName name =
			GPlatesModel::PropertyName::create_gpml("position");

		const GPlatesPropertyValues::StructuralType value =
			GPlatesPropertyValues::StructuralType::create_gml("Point" );

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
		QString old_fid,
		bool use_reverse)
	{
		// Create a sourceGeometry property delegate
		// FIXME: what to do if the old_fid is not found?
		const GPlatesModel::FeatureId fid = (id_map.find( old_fid ) )->second; 

		const GPlatesModel::PropertyName name =
			GPlatesModel::PropertyName::create_gpml( "centerLineOf" );

		const GPlatesPropertyValues::StructuralType value =
			GPlatesPropertyValues::StructuralType::create_gml("LineString");
		GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type pd_ptr =
			GPlatesPropertyValues::GpmlPropertyDelegate::create(
				fid, name, value);

		// Create a GpmlTopologicalLineSection from the delegate
		GPlatesPropertyValues::GpmlTopologicalLineSection::non_null_ptr_type 
		gtls_ptr =
			GPlatesPropertyValues::GpmlTopologicalLineSection::create(
				pd_ptr,
				use_reverse);

		return gtls_ptr;
	}


	
	//
	//
	//
	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
	create_gpml_topological_sections_vector(
		GPlatesModel::FeatureHandle::weak_ref feature_ref, 
		const std::vector<QString> &boundary_strings)
	{
		// vars to hold working data 
		std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
			topo_section_ptrs_vector;

		// iterator for the list of strings
		std::vector<QString>::const_iterator iter = boundary_strings.begin();

		// loop over the list of strings
		for (int i = 0; iter != boundary_strings.end() ; ++iter, ++i) 
		{
			// the string to parse 
			QString node_str = *iter;

#if 0
// FIXME : remove diagnostic 
qDebug() << "size = " << boundary_strings.size();
qDebug() << "iter = " << node_str;
#endif

			// tmp place to hold parsed sub-strings
			std::vector<QString> tokens;

			// loop over the string finding hash delimiters
			while (node_str.indexOf("#", 0) >= 0)
			{ 
				// store the position of the delimiter
				const int pos = node_str.indexOf("#", 0);

				// get the token
				const QString temp = node_str.mid(0, pos);

				// erase it from the source 
				node_str.remove(0, pos + 1);

				// and put it into the array
				tokens.push_back(temp);
			}

			// the last token is all alone, but has delimiter at end, remove it
			tokens.push_back(node_str);

			//FIXME: synch with file levelerror checking
			// Error checking on number of tokens found
			if ( tokens.size() != 11 )
			{
				qWarning() << "ERROR: Cannot parse boundary feature node line: "
					<< "expected 10 comma delimited tokens, "
					<< "got: " << tokens.size() << " tokens.";
				continue;
			}

			// convert token strings into topology parameters
			QString old_fid = tokens[0];
			int type = tokens[1].toInt();
			//float lat = tokens[2].toFloat();
			//float lon = tokens[3].toFloat();
			//float closeness = tokens[4].toFloat();
			bool use_reverse = tokens[5].toInt();
			//bool use_head_prev = tokens[6].toInt();
			//bool use_tail_prev = tokens[7].toInt();
			//bool use_head_next = tokens[8].toInt();
			//bool use_tail_next = tokens[9].toInt();

#if 0
// FIXME : remove diagnostic 
qDebug() << "old_fid = " << old_fid;
qDebug() << "lat = " << lat;
qDebug() << "lon = " << lon;
qDebug() << "use_reverse = " << use_reverse;
qDebug() << "use_head_prev = " << use_head_prev;
qDebug() << "use_tail_prev = " << use_tail_prev;
qDebug() << "use_head_next = " << use_head_next;
qDebug() << "use_tail_next = " << use_tail_next;
#endif

			// Make sure Feature referenced by old_fid can be located 
			old_id_to_new_id_map_const_iterator find_iter;
			find_iter = id_map.find( old_fid );
			if ( find_iter == id_map.end() ) 
			{
				qWarning() << "WARNING: feature '" << old_fid << "' is missing.";
				qWarning() << "WARNING: a GpmlTopologicalSection will NOT be created";
				// DO NOT create a GpmlTopologicalSection
				continue; // to next item on boundary list; 
			}
			// else, process this boundary node

			// convert type
			//GPlatesGlobal::FeatureTypes feature_type(GPlatesGlobal::UNKNOWN_FEATURE);

			// This was moved from an enumeration in deprecated 'global/types.h'.
			const int POINT_FEATURE = 1;
			const int LINE_FEATURE = 2;

			switch ( type )
			{
				case POINT_FEATURE:
					//feature_type = GPlatesGlobal::POINT_FEATURE;
					// Fill the vector of GpmlTopologicalSection::non_null_ptr_type
					topo_section_ptrs_vector.push_back( 
						create_gpml_topological_point( old_fid ) );
					break;

				case LINE_FEATURE:
					//feature_type = GPlatesGlobal::LINE_FEATURE;
					// Fill the vector of GpmlTopologicalSection::non_null_ptr_type
					topo_section_ptrs_vector.push_back( 
						create_gpml_topological_line_section( old_fid, use_reverse ) );

				default :
					break;
			}

			// ...ignore intersections - they are no longer needed.
		} // end of loop over boundary 

		return topo_section_ptrs_vector;
	}


	//
	//
	//
	GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type
	create_gpml_piecewise_aggregation( 
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		const std::vector<QString> &boundary_strings )
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
				topo_section_ptrs_vector.begin(),
				topo_section_ptrs_vector.end());
	
		const GPlatesPropertyValues::StructuralType topo_poly_type =
			GPlatesPropertyValues::StructuralType::create_gpml("TopologicalPolygon");
	
		// create the ConstantValue
		GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type constant_value =
			GPlatesPropertyValues::GpmlConstantValue::create(
				topo_poly_value, 
				topo_poly_type);
	
		// get the time period for the feature_ref
		// Valid Time (Assuming a gml:TimePeriod, rather than a gml:TimeInstant!)
		static const GPlatesModel::PropertyName valid_time_property_name =
			GPlatesModel::PropertyName::create_gml("validTime");

		boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> time_period =
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GmlTimePeriod>(
				feature_ref, valid_time_property_name);
	
		// Casting time details
		GPlatesPropertyValues::GmlTimePeriod* tp = 
		const_cast<GPlatesPropertyValues::GmlTimePeriod *>( time_period.get().get() );
	
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
	 * Attempts to extract a feature id from PLATES header.
	 * Returns true if successful and result is stored in @a feature_id.
	 */
	bool
	extract_feature_id_from_header(
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			GPlatesUtils::UnicodeString &feature_id)
	{
		static const GPlatesUtils::UnicodeString identity_start_tag(" <identity>");
		static const GPlatesUtils::UnicodeString identity_end_tag("</identity>");
		static const boost::int32_t identity_start_tag_length = identity_start_tag.length();
		static const boost::int32_t identity_end_tag_length = identity_end_tag.length();

		GPlatesUtils::UnicodeString geog_description = header->geographic_description();

		// Search for the identity start tag.
		// For some reason some files have two "<identity>" tags followed by one "</identity>" tag.
		// So we find the last occurrence of the "<identity>" tag.
		const boost::int32_t first_identity_start_index =
				geog_description.qstring().indexOf(identity_start_tag.qstring());
		const boost::int32_t last_identity_start_index =
				geog_description.qstring().lastIndexOf(identity_start_tag.qstring());
		if (first_identity_start_index < 0 ||
			last_identity_start_index < 0)
		{
			return false;
		}

		// Search for the identity end tag (starting at end of the identity start tag).
		const boost::int32_t identity_end_index = geog_description.indexOf(
				identity_end_tag,
				last_identity_start_index + identity_start_tag_length);
		if (identity_end_index < 0)
		{
			return false;
		}

		// The feature id is between end of (last) start tag and start of end tag.
		geog_description.extractBetween(
				last_identity_start_index + identity_start_tag_length,
				identity_end_index,
				feature_id);

		// Remove feature id and start/end id tag (and any extra erroneous start tag) from the
		// geographic description in PLATES header so we don't get two feature ids written out
		// if save to PLATES header later.
		// The PLATES writer will automatically append the feature id to each feature.
		geog_description.removeBetween(
				first_identity_start_index,
				identity_end_index + identity_end_tag_length);

		// Store back to original header.
		header->set_geographic_description(geog_description);

		return true;
	}

	/**
	 * Creates a feature of type @a feature_type.
	 * If feature id can be extracted from the PLATES header then use that
	 * otherwise auto-generate one.
	 */
	GPlatesModel::FeatureHandle::weak_ref
	create_feature(
			const GPlatesModel::FeatureType &feature_type,
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header)
	{
		GPlatesUtils::UnicodeString feature_id;
		if (extract_feature_id_from_header(header, feature_id))
		{
			return GPlatesModel::FeatureHandle::create(
					collection,
					feature_type,
					GPlatesModel::FeatureId(feature_id));
		}

		return GPlatesModel::FeatureHandle::create(
				collection,
				feature_type);
	}

	/**
	 * This function assumes that 'create_feature_with_geometries' has ensured that 'points'
	 * contains at least one point.
	 */
	void
	append_appropriate_geometry(
			const point_seq_type &points,
			const GPlatesModel::PropertyName &property_name,
			GPlatesModel::FeatureHandle::weak_ref &feature)
	{
		using namespace GPlatesMaths;

		point_seq_type::size_type num_distinct_adj_points =
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
					PolylineOnSphere::create(points);
			GPlatesPropertyValues::GmlLineString::non_null_ptr_type gml_line_string =
					GPlatesPropertyValues::GmlLineString::create(polyline);

			GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type gml_orientable_curve =
					GPlatesModel::ModelUtils::create_gml_orientable_curve(gml_line_string);

			GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
					GPlatesModel::ModelUtils::create_gpml_constant_value(gml_orientable_curve);
			feature->add(
					GPlatesModel::TopLevelPropertyInline::create(
						property_name,
						property_value));
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
				feature->add(
						GPlatesModel::TopLevelPropertyInline::create(
							property_name,
							gml_point));
			}
			else
			{
				// Wrap it in a constant value.
				GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
					GPlatesModel::ModelUtils::create_gpml_constant_value(gml_point);
				feature->add(
						GPlatesModel::TopLevelPropertyInline::create(
							property_name,
							property_value));
			}

		} else {
			// FIXME:  A pre-condition of this function has been violated.  We should
			// throw an exception.
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
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq,
			const GPlatesModel::FeatureType &feature_type,
			const GPlatesModel::PropertyName &geometry_property_name)
	{
		using namespace GPlatesPropertyValues;
		using namespace GPlatesModel;

		GPlatesModel::FeatureHandle::weak_ref feature_handle =
				create_feature(feature_type, collection, header);

		const integer_plate_id_type plate_id = header->plate_id_number();
		const integer_plate_id_type conjugate_plate_id = header->conjugate_plate_id_number();
		const GeoTimeInstant geo_time_instant_begin(
				create_geo_time_instant(header->age_of_appearance()));
		const GeoTimeInstant geo_time_instant_end(
				create_geo_time_instant(header->age_of_disappearance()));

		// Ignore a reconstruction plate id of 999 (it's a hard-coded default value for no-plate-id).
		if (plate_id != 999)
		{
			// Attempt to add the reconstruction plate id to the feature.
			// If the feature type does not support it (according to GPGIM) then it won't get added.
			// Pretty much all feature types support a reconstruction plate id so should be no problem.
			GPlatesModel::ModelUtils::add_property(
					feature_handle,
					GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
					GpmlPlateId::create(plate_id),
					true/*check_property_name_allowed_for_feature_type*/,
					false/*check_property_multiplicity*/,
					false/*check_property_value_type*/);
		}

		// Ignore a conjugate plate id of 999 (it's a hard-coded default value for no-plate-id).
		if (conjugate_plate_id != 999)
		{
			// Attempt to add the conjugate plate id to the feature.
			// If the feature type does not support it (according to GPGIM) then it won't get added.
			GPlatesModel::ModelUtils::add_property(
					feature_handle,
					GPlatesModel::PropertyName::create_gpml("conjugatePlateId"),
					GpmlPlateId::create(conjugate_plate_id),
					true/*check_property_name_allowed_for_feature_type*/,
					false/*check_property_multiplicity*/,
					false/*check_property_value_type*/);
		}

		// For each geometry in the feature append the appropriate geometry property value
		// to the current feature.
		std::for_each(geometry_seq.begin(), geometry_seq.end(),
				boost::bind(&append_appropriate_geometry,
						boost::placeholders::_1, boost::cref(geometry_property_name), boost::ref(feature_handle)));

		GmlTimePeriod::non_null_ptr_type gml_valid_time =
				ModelUtils::create_gml_time_period(geo_time_instant_begin, geo_time_instant_end);
		feature_handle->add(
				TopLevelPropertyInline::create(
					PropertyName::create_gml("validTime"),
					gml_valid_time));

		// Use the PLATES4 geographic description as the "gml:name" property.
		XsString::non_null_ptr_type gml_name = 
				XsString::create(header->geographic_description());
		feature_handle->add(
				TopLevelPropertyInline::create(
					PropertyName::create_gml("name"),
					gml_name));

		feature_handle->add(
				TopLevelPropertyInline::create(
					PropertyName::create_gpml("oldPlatesHeader"),
					header->clone()));

		// file the map with id data 
		GPlatesUtils::UnicodeString s = header->old_feature_id(); // GP8 
		const GPlatesModel::FeatureId &fid = feature_handle->feature_id();
		id_map.insert( std::make_pair(s.qstring(), fid) );


		return feature_handle;
	}

	/**
	 * Create a multipoint feature.
	 *
	 * Each geometry in @a geometry_seq should contain only a single point; otherwise
	 * we throw ReadErrors::InvalidMultipointGeometry
	 */
	GPlatesModel::FeatureHandle::weak_ref	
	create_multi_point_feature(
		GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
		GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
		const geometry_seq_type &geometry_seq,
		const GPlatesModel::FeatureType &feature_type,
		const GPlatesModel::PropertyName &geometry_property_name)
	{
		// Check that geometry_seq is appropriate for a multipoint. 
		if (!sequence_is_valid_multipoint(geometry_seq))
		{
			throw GPlatesFileIO::ReadErrors::InvalidMultipointGeometry;
		}
		
		// Assume create_common will do the right thing with append_appropriate_geometry.
		return create_common(collection, header, geometry_seq, feature_type, geometry_property_name);
		
	}

	/**
	 * Creates a GPML feature from PLATES data where the GPML feature accepts a single point only.
	 * Polylines don't make sense here, and should cause a warning.
	 */
	GPlatesModel::FeatureHandle::weak_ref	
	create_single_point_feature(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq,
			const GPlatesModel::FeatureType &feature_type,
			const GPlatesModel::PropertyName &geometry_property_name)
	{
		// Check for invalid data.
		// Invalid if more than one geometry.
		// Invalid if a sole geometry contains more than one distinct point.
		if (geometry_seq.size() > 1 ||
			GPlatesMaths::count_distinct_adjacent_points(geometry_seq.front()) != 1)
		{
			// FIXME: This will be counted as an error, be caught down in read_file, 
			// and nuke the feature. This is a little harsh, but it's the best we can do for now.
			throw GPlatesFileIO::ReadErrors::MoreThanOneDistinctPoint;
		}
		// Assume create_common will do the right thing with append_appropriate_geometry.
		return create_common(collection, header, geometry_seq, feature_type, geometry_property_name);
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_fault(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq, 
				GPlatesModel::FeatureType::create_gpml("Fault"), 
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_normal_fault(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
				create_fault(collection, header, geometry_seq);
		
		const GPlatesPropertyValues::Enumeration::non_null_ptr_type dip_slip_property_value =
				GPlatesPropertyValues::Enumeration::create(
						GPlatesPropertyValues::EnumerationType::create_gpml("DipSlipEnumeration"),
						"Compression");
		feature_handle->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("dipSlip"),
					dip_slip_property_value));

		return feature_handle;
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_reverse_fault(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
				create_fault(collection, header, geometry_seq);
		
		const GPlatesPropertyValues::Enumeration::non_null_ptr_type dip_slip_property_value =
				GPlatesPropertyValues::Enumeration::create(
						GPlatesPropertyValues::EnumerationType::create_gpml("DipSlipEnumeration"),
						"Extension");
		feature_handle->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("dipSlip"),
					dip_slip_property_value));

		return feature_handle;
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_thrust_fault(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
				create_reverse_fault(collection, header, geometry_seq);

		const GPlatesPropertyValues::XsString::non_null_ptr_type subcategory_property_value =
				GPlatesPropertyValues::XsString::create("Thrust");
		feature_handle->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("subcategory"),
					subcategory_property_value));

		return feature_handle;
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_strike_slip_fault(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
				create_fault(collection, header, geometry_seq);
		
		const GPlatesPropertyValues::Enumeration::non_null_ptr_type strike_slip_property_value =
				GPlatesPropertyValues::Enumeration::create(
						GPlatesPropertyValues::EnumerationType::create_gpml("StrikeSlipEnumeration"),
						"Unknown");
		feature_handle->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("strikeSlip"),
					strike_slip_property_value));

		return feature_handle;
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_unclassified_feature(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq, 
				GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature"), 
				GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_aseismic_ridge(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq, 
				GPlatesModel::FeatureType::create_gpml("AseismicRidge"), 
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_bathymetry(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		// FIXME: Set up a method to construct gpml:Contours and use them as the geometry, sourcing
		// the appropriate PLATES header data.
		return create_common(collection, header, geometry_seq, 
				GPlatesModel::FeatureType::create_gpml("Bathymetry"), 
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_basin(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq, 
				GPlatesModel::FeatureType::create_gpml("Basin"), 
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_coastline(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq, 
				GPlatesModel::FeatureType::create_gpml("Coastline"), 
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_continental_boundary(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq, 
				GPlatesModel::FeatureType::create_gpml("PassiveContinentalBoundary"), 
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_continental_fragment(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq, 
				GPlatesModel::FeatureType::create_gpml("ContinentalFragment"), 
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_craton(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq, 
				GPlatesModel::FeatureType::create_gpml("Craton"), 
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_extended_continental_crust(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq, 
				GPlatesModel::FeatureType::create_gpml("ExtendedContinentalCrust"), 
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_fracture_zone(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq, 
				GPlatesModel::FeatureType::create_gpml("FractureZone"), 
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_gravimetry(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		// FIXME: Set up a method to construct gpml:Contours and use them as the geometry, sourcing
		// the appropriate PLATES header data.
		return create_common(collection, header, geometry_seq, 
				GPlatesModel::FeatureType::create_gpml("Gravimetry"), 
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_grid_mark(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		// FIXME: This will create lots of gpml:OldPlatesGridMarks if the source GR feature uses
		// lots of pen up pen down commands. A way to specify use of gml:MultiCurve would be nice.
		return create_common(collection, header, geometry_seq, 
				GPlatesModel::FeatureType::create_gpml("OldPlatesGridMark"), 
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_heat_flow(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		// FIXME: Set up a method to construct gpml:Contours and use them as the geometry, sourcing
		// the appropriate PLATES header data.
		return create_common(collection, header, geometry_seq, 
				GPlatesModel::FeatureType::create_gpml("HeatFlow"), 
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_hot_spot(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		// Check our geometry sequence. If we have only one point, create a single-point feature;
		// if we have more than one, create a multi-point feature.
		if (geometry_seq.size() > 1)
		{
#if 1
			return create_multi_point_feature(collection,header,geometry_seq,
				GPlatesModel::FeatureType::create_gpml("HotSpot"),
				GPlatesModel::PropertyName::create_gpml("multiPosition"));
#else
			// create_common will create a feature with distinct multiple point geometries,
			// as opposed to a multipoint.
			return create_common(collection,header,geometry_seq,
				GPlatesModel::FeatureType::create_gpml("HotSpot"),
				GPlatesModel::PropertyName::create_gpml("position"));
#endif
		}
		else 
		{
			// Zero geometries, or a geometry containing more than one point, 
			// will get caught in the create_single_point_feature function. 
			return create_single_point_feature(collection,header,geometry_seq,
				GPlatesModel::FeatureType::create_gpml("HotSpot"),
				GPlatesModel::PropertyName::create_gpml("position"));
		}	
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_hot_spot_trail(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("HotSpotTrail"),
				GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_inferred_paleo_boundary(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("InferredPaleoBoundary"),
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_island_arc(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq,
			bool is_active)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
			create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("IslandArc"),
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
		
		const GPlatesPropertyValues::XsBoolean::non_null_ptr_type is_active_property_value =
				GPlatesPropertyValues::XsBoolean::create(is_active);
		feature_handle->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("isActive"),
					is_active_property_value));

		return feature_handle;
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_island_arc_active(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_island_arc(collection, header, geometry_seq, true);
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_island_arc_inactive(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_island_arc(collection, header, geometry_seq, false);
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_isochron(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("Isochron"),
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_isopach(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		// FIXME: Set up a method to construct gpml:Contours and use them as the geometry, sourcing
		// the appropriate PLATES header data.
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("SedimentThickness"),
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_geological_lineation(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("GeologicalLineation"),
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_magnetics(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		// FIXME: Set up a method to construct gpml:Contours and use them as the geometry, sourcing
		// the appropriate PLATES header data.
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("Magnetics"),
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_magnetic_pick(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		// FIXME: fill in the rest of MagneticAnomalyIdentification from the appropriate PLATES header data,
		// assuming it is available.
		
		// Check our geometry sequence. If we have only one point, create a single-point feature;
		// if we have more than one, create a multi-point feature.
		if (geometry_seq.size() > 1)
		{
#if 1
			return create_multi_point_feature(collection,header,geometry_seq,
				GPlatesModel::FeatureType::create_gpml("MagneticAnomalyIdentification"),
				GPlatesModel::PropertyName::create_gpml("multiPosition"));
#else
			// create_common will create a feature with distinct multiple point geometries,
			// as opposed to a multipoint.
			return create_common(collection,header,geometry_seq,
				GPlatesModel::FeatureType::create_gpml("MagneticAnomalyIdentification"),
				GPlatesModel::PropertyName::create_gpml("position"));
#endif
		}
		else 
		{
			// Zero geometries, or a geometry containing more than one point, 
			// will get caught in the create_single_point_feature function. 
			return create_single_point_feature(collection,header,geometry_seq,
				GPlatesModel::FeatureType::create_gpml("MagneticAnomalyIdentification"),
				GPlatesModel::PropertyName::create_gpml("position"));
		}
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_mid_ocean_ridge(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq,
			bool is_active)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
			create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("MidOceanRidge"),
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
		
		const GPlatesPropertyValues::XsBoolean::non_null_ptr_type is_active_property_value =
				GPlatesPropertyValues::XsBoolean::create(is_active);

		GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type constant_value_property_value =
				GPlatesModel::ModelUtils::create_gpml_constant_value(is_active_property_value);

		feature_handle->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("isActive"),
					constant_value_property_value));

		return feature_handle;
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_ridge_segment(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_mid_ocean_ridge(collection, header, geometry_seq, true);
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_extinct_ridge(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_mid_ocean_ridge(collection, header, geometry_seq, false);
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_ophiolite_belt(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
				create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("BasicRockUnit"),
				GPlatesModel::PropertyName::create_gpml("outlineOf"));

		const GPlatesPropertyValues::XsString::non_null_ptr_type subcategory_property_value =
				GPlatesPropertyValues::XsString::create("Ophiolite");
		feature_handle->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("subcategory"),
					subcategory_property_value));
		
		return feature_handle;
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_orogenic_belt(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("OrogenicBelt"),
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_seamount(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("Seamount"),
				GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
	}

	GPlatesModel::FeatureHandle::weak_ref	
	create_slab(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("Slab"),
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}

	GPlatesModel::FeatureHandle::weak_ref	
	create_subduction_zone(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq,
			bool is_active,
			const char *subduction_polarity_enumeration_content = "Unknown")
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
				create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("SubductionZone"),
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	
		const GPlatesPropertyValues::StructuralType subduction_polarity_property_type =
			GPlatesPropertyValues::StructuralType::create_gpml("SubductionPolarityEnumeration");

		const GPlatesPropertyValues::Enumeration::non_null_ptr_type subduction_polarity_property_value =
				GPlatesPropertyValues::Enumeration::create(
						GPlatesPropertyValues::EnumerationType::create_gpml("SubductionPolarityEnumeration"),
						subduction_polarity_enumeration_content);
	
		// create the ConstantValue
		GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type subduction_polarity_constant_value =
			GPlatesPropertyValues::GpmlConstantValue::create(
				subduction_polarity_property_value, 
				subduction_polarity_property_type);

		feature_handle->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("subductionPolarity"),
					subduction_polarity_constant_value));

		const GPlatesPropertyValues::XsBoolean::non_null_ptr_type is_active_property_value =
				GPlatesPropertyValues::XsBoolean::create(is_active);
		feature_handle->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("isActive"),
					is_active_property_value));

		return feature_handle;
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_subduction_zone_active(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_subduction_zone(collection, header, geometry_seq, true);
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_subduction_zone_inactive(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_subduction_zone(collection, header, geometry_seq, false);
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_subduction_zone_left(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_subduction_zone(collection, header, geometry_seq, false, "Left");
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_subduction_zone_right(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_subduction_zone(collection, header, geometry_seq, false, "Right");
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_suture(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("Suture"),
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_terrane_boundary(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("TerraneBoundary"),
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_transitional_crust(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("TransitionalCrust"),
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_transform(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("Transform"),
				GPlatesModel::PropertyName::create_gpml("centerLineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_topography(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		// FIXME: Set up a method to construct gpml:Contours and use them as the geometry, sourcing
		// the appropriate PLATES header data.
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("Topography"),
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_volcano(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("Volcano"),
				GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
	}

	GPlatesModel::FeatureHandle::weak_ref	
	create_pluton(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("Pluton"),
				GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
	}

	GPlatesModel::FeatureHandle::weak_ref	
	create_political_boundary(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq, 
				GPlatesModel::FeatureType::create_gpml("PoliticalBoundary"), 
				GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_large_igneous_province(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("LargeIgneousProvince"),
				GPlatesModel::PropertyName::create_gpml("outlineOf"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_navdat_1(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("NavdatSampleMafic"),
				GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
	}

	GPlatesModel::FeatureHandle::weak_ref	
	create_navdat_2(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("NavdatSampleIntermediate"),
				GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_navdat_3(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("NavdatSampleFelsicLow"),
				GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_navdat_4(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		return create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("NavdatSampleFelsicHigh"),
				GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
	}


	GPlatesModel::FeatureHandle::weak_ref	
	create_topological_closed_plate_boundary(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &header,
			const geometry_seq_type &geometry_seq)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_handle = 
				create_common(collection, header, geometry_seq,
				GPlatesModel::FeatureType::create_gpml("TopologicalClosedPlateBoundary"),
				GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));

		return feature_handle;
	}


	typedef GPlatesModel::FeatureHandle::weak_ref (*creation_function_type)(
			GPlatesModel::FeatureCollectionHandle::weak_ref &,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type &,
			const geometry_seq_type &);

	typedef std::map<GPlatesUtils::UnicodeString, creation_function_type> creation_map_type;
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

		// -might- be Ice Shelf, might be Isochron. We don't know.
		// It appears IS covers IC and IM.
		//
		// Update 2012/9/4: Maria Seton requested "IS" result in an isochron instead of unclassified feature.
		map["IS"] = create_isochron;

		map["LI"] = create_geological_lineation;
		map["MA"] = create_magnetics;
		map["NF"] = create_normal_fault;
		map["N1"] = create_navdat_1;
		map["N2"] = create_navdat_2;
		map["N3"] = create_navdat_3;
		map["N4"] = create_navdat_4;
		map["OB"] = create_orogenic_belt;
		map["OP"] = create_ophiolite_belt;
		map["OR"] = create_orogenic_belt;
		map["PB"] = create_inferred_paleo_boundary;
		map["PA"] = create_magnetic_pick;
		map["PC"] = create_magnetic_pick;
		map["PM"] = create_magnetic_pick;
		map["PL"] = create_pluton;
		map["PO"] = create_political_boundary;
		map["RA"] = create_island_arc_inactive;
		map["RF"] = create_reverse_fault;
		map["RI"] = create_ridge_segment;
		map["SL"] = create_slab;
		map["SM"] = create_seamount;
		map["SS"] = create_strike_slip_fault;
		map["SU"] = create_suture;
		map["TB"] = create_terrane_boundary;
		map["TC"] = create_transitional_crust;
		map["TF"] = create_transform;
		map["TH"] = create_thrust_fault;
		map["TO"] = create_topography;
		map["TR"] = create_subduction_zone_active;
		map["UN"] = create_unclassified_feature;
		map["VO"] = create_volcano;
		map["VP"] = create_large_igneous_province;
		map["XR"] = create_extinct_ridge;
		map["XT"] = create_subduction_zone_inactive;
		map["PP"] = create_topological_closed_plate_boundary; // GP8 feature type
		map["ln"] = create_inferred_paleo_boundary; // GP8 feature type
		map["sL"] = create_subduction_zone_left; // GP8 feature type
		map["sR"] = create_subduction_zone_right; // GP8 feature type
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
				new GPlatesFileIO::LineNumber(in.line_number()));
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
				new GPlatesFileIO::LineNumber(in.line_number()));
		errors.d_warnings.push_back(GPlatesFileIO::ReadErrorOccurrence(source, location, 
				GPlatesFileIO::ReadErrors::AmbiguousPlatesIceShelfCode,
				GPlatesFileIO::ReadErrors::NoAction));
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

	typedef std::map<GPlatesUtils::UnicodeString, warning_function_type> warning_map_type;
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
			const QString &first_line)
	{
		QString second_line;
		if ( ! in.peekline(second_line)) {
			throw GPlatesFileIO::ReadErrors::MissingPlatesHeaderSecondLine;
		}

		typedef GPlatesModel::integer_plate_id_type plate_id_type;

		bool ok;

		const unsigned int region_number = first_line.mid(0, 2).toUInt(&ok);
		if (!ok)
		{
			throw GPlatesFileIO::ReadErrors::InvalidPlatesRegionNumber;
		}
		const unsigned int reference_number = first_line.mid(2, 2).toUInt(&ok);
		if (!ok)
		{
			throw GPlatesFileIO::ReadErrors::InvalidPlatesReferenceNumber;
		}
		const unsigned int string_number = first_line.mid(5, 4).toUInt(&ok);
		if (!ok)
		{
			throw GPlatesFileIO::ReadErrors::InvalidPlatesStringNumber;
		}
		const GPlatesUtils::UnicodeString geographic_description(first_line.mid(10));
		if (!ok)
		{
			throw GPlatesFileIO::ReadErrors::InvalidPlatesGeographicDescription;
		}
		// We now read the plate-id from the zeroth column, to accommodate a
		// possible 4-th plate-id digit.
		const plate_id_type plate_id_number = second_line.mid(0, 4).toULong(&ok);
		if (!ok)
		{
			throw GPlatesFileIO::ReadErrors::InvalidPlatesPlateIdNumber;
		}
		const double age_of_appearance = second_line.mid(5, 6).toDouble(&ok);
		if (!ok)
		{
			throw GPlatesFileIO::ReadErrors::InvalidPlatesAgeOfAppearance;
		}
		const double age_of_disappearance = second_line.mid(12, 6).toDouble(&ok);
		if (!ok)
		{
			throw GPlatesFileIO::ReadErrors::InvalidPlatesAgeOfDisappearance;
		}
		const GPlatesUtils::UnicodeString data_type_code(second_line.mid(19, 2));
		if (!ok)
		{
			throw GPlatesFileIO::ReadErrors::InvalidPlatesDataTypeCode;
		}
		const unsigned int data_type_code_number = second_line.mid(21, 4).toUInt(&ok);
		if (!ok)
		{
			throw GPlatesFileIO::ReadErrors::InvalidPlatesDataTypeCodeNumber;
		}
		// We don't read in a "DataTypeCodeNumberAdditional" field now, but we need
		// to pass something to the header constructor, so we'll pass an empty string.
		const GPlatesUtils::UnicodeString data_type_code_number_additional;
		// We now read the conjugate plate-id from column 25, to accommodate a
		// possible 4th conjugate plate-id digit.
		const plate_id_type conjugate_plate_id_number = second_line.mid(25, 4).toULong(&ok);
		if (!ok)
		{
			throw GPlatesFileIO::ReadErrors::InvalidPlatesConjugatePlateIdNumber;
		}
		const unsigned int colour_code = second_line.mid(30, 3).toUInt(&ok);
		if (!ok)
		{
			throw GPlatesFileIO::ReadErrors::InvalidPlatesColourCode;
		}
		const unsigned int number_of_points = second_line.mid(34, 5).toUInt(&ok);
		if (!ok)
		{
			throw GPlatesFileIO::ReadErrors::InvalidPlatesNumberOfPoints;
		}

		GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type
			gpml_old_plates_header = GPlatesPropertyValues::GpmlOldPlatesHeader::create(
				region_number,
				reference_number,
				string_number,
				geographic_description,
				plate_id_number,
				age_of_appearance,
				age_of_disappearance,
				data_type_code,
				data_type_code_number,
				data_type_code_number_additional,
				conjugate_plate_id_number,
				colour_code,
				number_of_points);

		// If we get here then no exception has been thrown parsing first two lines
		// so we can commit to having read the second line.
		// If don't do this then we're effectively only checking every
		// alternate line for a new feature (if the previous feature had a parse error)
		// and could skip a feature even if it's parseable.
		in.getline(second_line);

		return gpml_old_plates_header;
	}


	PlotterCodes::PlotterCode
	read_polyline_point(
			GPlatesFileIO::LineReader &in,
			point_seq_type &points,
			PlotterCodes::PlotterCode expected_code)
	{
		QString line;
		if ( ! in.getline(line)) {
			// Since we're in this function, we're expecting to read a point.  But we
			// couldn't find one.  So, let's complain.
			throw GPlatesFileIO::ReadErrors::MissingPlatesPolylinePoint;
		}

		int plotter;
		double latitude, longitude;

		QTextStream line_stream(&line, QIODevice::ReadOnly);
		// By default QTextStream attempts to detect the base of integers.
		// We turn this off and set base to decimal since we don't want numbers
		// like 012 being interpreted as octal (since has an '0' at front).
		line_stream.setIntegerBase(10);

		line_stream >> latitude >> longitude >> plotter;
		if (line_stream.status() != QTextStream::Ok)
		{
			throw GPlatesFileIO::ReadErrors::InvalidPlatesPolylinePoint;
		}

		// First:  If we've encountered (lat = 99.0; lon = 99.0; plotter code = SKIP TO),
		// that's the end-of-polyline marker.
		if (plotter == PlotterCodes::PEN_SKIP_TO &&
				GPlatesMaths::are_almost_exactly_equal(latitude, 99.0) && 
				GPlatesMaths::are_almost_exactly_equal(longitude, 99.0))
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
	QString
	read_platepolygon_boundary_feature(
			GPlatesFileIO::LineReader &in,
			std::vector<QString> &boundary_strings,
			QString code)
	{
		QString line;

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
	create_feature_with_geometries(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesFileIO::LineReader &in,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			creation_function_type creation_function,
			GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type old_plates_header,
			const geometry_seq_type &geometry_seq,
			GPlatesFileIO::ReadErrorAccumulation &errors)
	{
		if (geometry_seq.empty())
		{
			// We don't have any geometries - this can happen if all points in
			// a feature have "PEN_SKIP_TO" codes (all points get skipped).
			throw GPlatesFileIO::ReadErrors::NoValidGeometriesInPlatesFeature;
		}

		try
		{
			creation_function(collection, old_plates_header, geometry_seq);
		}
		catch (std::exception &exc)
		{
			qWarning() << "PlatesLineFormatReader.cc:create_feature_with_geometries: " << exc.what();
		}
		catch (...)
		{
			qWarning() << "PlatesLineFormatReader.cc:create_feature_with_geometries: Unknown error";
		}
	}


	/**
	 * Checks to make sure valid geometry can be constructed from the points.
	 * Returns true if can create valid geometry from the points.
	 */
	bool
	test_validity_of_points(
			const point_seq_type &point_seq,
			GPlatesFileIO::LineReader &in,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			GPlatesFileIO::ReadErrorAccumulation &errors)
	{
		// See if it's possible to create a polyline.

		// We want to return a different ReadError Description for each possible return
		// value of evaluate_construction_parameter_validity().
		GPlatesMaths::PolylineOnSphere::ConstructionParameterValidity polyline_validity =
				GPlatesMaths::PolylineOnSphere::evaluate_construction_parameter_validity(
						point_seq.begin(), point_seq.end());

		if (polyline_validity == GPlatesMaths::PolylineOnSphere::VALID)
		{
			return true;
		}

		GPlatesFileIO::ReadErrors::Result error_result =
				GPlatesFileIO::ReadErrors::NoGeometryCreatedByMovement;

		GPlatesFileIO::ReadErrors::Description error_description;

		switch (polyline_validity)
		{
		case GPlatesMaths::PolylineOnSphere::INVALID_INSUFFICIENT_DISTINCT_POINTS:
			// Else we have a point which is always valid.
			// Note that the caller ensured we will have at least one point.
			if (GPlatesMaths::count_distinct_adjacent_points(point_seq) == 1)
			{
				return true;
			}

			error_description = GPlatesFileIO::ReadErrors::InsufficientDistinctPointsInPolyline;
			break;

		case GPlatesMaths::PolylineOnSphere::INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:
			error_description = GPlatesFileIO::ReadErrors::AntipodalAdjacentPointsInPolyline;
			break;

		default:
			error_description = GPlatesFileIO::ReadErrors::InvalidPointsInPolyline;
			break;
		}

		// Add error message.
		const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
				new GPlatesFileIO::LineNumber(in.line_number()));
		errors.d_recoverable_errors.push_back(
				GPlatesFileIO::ReadErrorOccurrence(
						source, location, error_description, error_result));

		return false;
	}


	/**
	 * Add points to a list of geometries for a feature.
	 * Checks to make sure valid geometry can be constructed from the points.
	 * Before returning, clears the caller's points passed in.
	 */
	void
	add_points_to_new_geometry(
			geometry_seq_type &geometry_seq,
			point_seq_type &point_seq,
			GPlatesFileIO::LineReader &in,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			GPlatesFileIO::ReadErrorAccumulation &errors)
	{
	
	
// The following check for adjacent skip-to codes has been commented out to allow adjacent
// skip-to codes to be accepted as points. 
// An empty point_seq will still be caught in the test_validity_of_points() function.
#if 0
		// If we have one point then it means we had adjacent skip-to plotter codes.
		if (point_seq.size() < 2)
		{
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
					new GPlatesFileIO::LineNumber(in.line_number()));
			errors.d_warnings.push_back(GPlatesFileIO::ReadErrorOccurrence(source, location, 
					GPlatesFileIO::ReadErrors::AdjacentSkipToPlotterCodes,
					GPlatesFileIO::ReadErrors::NoGeometryCreatedByMovement));

			// We'll stay true to the plotter code, even though it was probably an error,
			// and skip to the next point.
			point_seq.clear();

			return;
		}
#endif
		
		// Test the validity of the points as a geometry.
		// We do this here instead of in 'append_appropriate_geometry()' because
		// we have access to the line number at which the current geometry, of the
		// current feature, ends.
		if (!test_validity_of_points(point_seq, in, source, errors))
		{
			// Clear the points and return without adding to the list of geometries.
			point_seq.clear();

			return;
		}

		// Add a new geometry.
		geometry_seq.push_back( point_seq_type() );

		// Move the geometry from 'point_seq' to the new geometry.
		// This clears the 'point_seq' sequence for use by the next geometry.
		geometry_seq.back().swap(point_seq);
	}

	void
	read_feature(
			GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesFileIO::LineReader &in,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			GPlatesFileIO::ReadErrorAccumulation &errors)
	{
		QString first_line;
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
			geometry_seq_type geometry_seq;

			// a list of boundary feature lines to populate from 
			// read_platepolygon_boundary_feature()
			std::vector<QString> boundary_strings;

			// read the first platepolygon boundary feature and 
			// set the terminator code to CONTINUE
			QString code = "CONTINUE";
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
							collection, old_plates_header, geometry_seq);
		
					// Create the Gpml Piecewise Aggregation from the boundary list
					GPlatesPropertyValues::GpmlPiecewiseAggregation::non_null_ptr_type agg =
						create_gpml_piecewise_aggregation( 
							feature_ref,
							boundary_strings);
		
					// Add a gpml:boundary Property.
					feature_ref->add(
							GPlatesModel::TopLevelPropertyInline::create(
								GPlatesModel::PropertyName::create_gpml("boundary"),
								agg));
				}

			} while (code != "NULL");

			return;
		}
		// else this is a regular feature 


		// List of one or more geometries in the current feature being read.
		// Pen-up plotter codes distinguish the geometries within a feature.
		geometry_seq_type geometry_seq;

		PlotterCodes::PlotterCode code;
		point_seq_type points;
		code = read_polyline_point(in, points, PlotterCodes::PEN_SKIP_TO);

		// FIXME : Rather than create millions of little features for each unbroken
		// section of line, it would be better to create gml:MultiCurve geometry.
		while (code != PlotterCodes::PEN_TERMINATING_POINT) {
			code = read_polyline_point(in, points, PlotterCodes::PEN_EITHER);
			if (code == PlotterCodes::PEN_TERMINATING_POINT) {
				// When 'read_polyline_point' encounters the terminating point, it
				// doesn't append the point position, so we can create a geometry
				// using all the points in 'points'.

				add_points_to_new_geometry(geometry_seq, points, in, source, errors);

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

				add_points_to_new_geometry(geometry_seq, points, in, source, errors);

				points.push_back(last_point);
			}
		}

		// Now that we've read one or more geometries we can create the feature.
		create_feature_with_geometries(collection, in, source,
				creation_function, old_plates_header, geometry_seq, errors);
	}
}


void
GPlatesFileIO::PlatesLineFormatReader::read_file(
		File::Reference &file,
		ReadErrorAccumulation &read_errors,
		bool &contains_unsaved_changes)
{
	PROFILE_FUNC();

	contains_unsaved_changes = false;

	const FileInfo &fileinfo = file.get_file_info();

	QString filename = fileinfo.get_qfileinfo().absoluteFilePath();

	// Open the file for reading.
	QFile input(filename);
	if (!input.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		throw ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
	}

	boost::shared_ptr<DataSource> source( 
			new GPlatesFileIO::LocalFileDataSource(filename, DataFormats::PlatesLine));

	GPlatesModel::FeatureCollectionHandle::weak_ref collection = file.get_feature_collection();
	
	LineReader in(input);
	while (in) {
		try {
			read_feature(collection, in, source, read_errors);
		} catch (GPlatesFileIO::ReadErrors::Description error) {
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
					new GPlatesFileIO::LineNumber(in.line_number()));
			read_errors.d_recoverable_errors.push_back(GPlatesFileIO::ReadErrorOccurrence(
					source, location, error, GPlatesFileIO::ReadErrors::FeatureDiscarded));
		}
	}
}

