/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-08-15 02:13:48 -0700 (Fri, 15 Aug 2008) $
 * 
 * Copyright (C) 2008, 2009 California Institute of Technology
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

// #define DEBUG

#ifdef _MSC_VER
#define copysign _copysign
#endif

#include <boost/optional.hpp>
#include <boost/none.hpp>

#include "TopologySectionsFinder.h"


#include "model/FeatureHandle.h"
#include "model/FeatureHandleWeakRefBackInserter.h"
#include "model/FeatureRevision.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "model/Reconstruction.h"
#include "model/ReconstructionTree.h"
#include "model/TopLevelPropertyInline.h"

#include "property-values/Enumeration.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GpmlPolarityChronId.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFeatureReference.h"
#include "property-values/GpmlFeatureSnapshotReference.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlHotSpotTrailMark.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlMeasure.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlRevisionId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalIntersection.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/UninterpretedPropertyValue.h"
#include "property-values/TemplateTypeParameterType.h"
#include "property-values/XsString.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"

#include "maths/PolylineOnSphere.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/LatLonPointConversions.h"
#include "maths/ProximityCriteria.h"
#include "maths/PolylineIntersections.h"

#include "gui/ProximityTests.h"

#include "utils/UnicodeStringUtils.h"


namespace
{
	/**
	 * Resolves a FeatureId reference in a TableRow (or doesn't,
	 * if it can't be resolved.)
	 *
	 * Copied from TopologySectionsTable.cc.
	 */
	void
	resolve_feature_id(
			GPlatesGui::TopologySectionsContainer::TableRow &entry)
	{
		std::vector<GPlatesModel::FeatureHandle::weak_ref> back_ref_targets;
		entry.d_feature_id.find_back_ref_targets(
				GPlatesModel::append_as_weak_refs(back_ref_targets));
		
		if (back_ref_targets.size() == 1) {
			GPlatesModel::FeatureHandle::weak_ref weakref = *back_ref_targets.begin();
			entry.d_feature_ref = weakref;
		} else {
			static const GPlatesModel::FeatureHandle::weak_ref null_ref;
			entry.d_feature_ref = null_ref;
		}
	}

	/**
	 * "Resolves" the target of a PropertyDelegate to a FeatureHandle::properties_iterator.
	 * Ideally, a PropertyDelegate would be able to uniquely identify a particular property,
	 * regardless of how many times that property appears inside a Feature or how many
	 * in-line properties (an idea which is now deprecated) that property might have.
	 *
	 * In reality, we need a way to go from FeatureId+PropertyName to a properties_iterator,
	 * and we need one now. This function exists to grab the first properties_iterator belonging
	 * to the FeatureHandle (which in turn can be resolved with the @a resolve_feature_id
	 * function above) which matches the supplied PropertyName.
	 *
	 * It returns a boost::optional because there is naturally no guarantee that we will
	 * find a match.
	 */
	boost::optional<GPlatesModel::FeatureHandle::properties_iterator>
	find_properties_iterator(
			const GPlatesModel::FeatureHandle::weak_ref feature_ref,
			const GPlatesModel::PropertyName property_name)
	{
		if ( ! feature_ref.is_valid()) {
			return boost::none;
		}
		
		// Iterate through the top level properties; look for the first name that matches.
		GPlatesModel::FeatureHandle::properties_iterator it = feature_ref->properties_begin();
		GPlatesModel::FeatureHandle::properties_iterator end = feature_ref->properties_end();
		for ( ; it != end; ++it) {
			// Elements of this properties vector can be NULL pointers.  (See the comment in
			// "model/FeatureRevision.h" for more details.)
			if (*it != NULL && (*it)->property_name() == property_name) {
				return it;
			}
		}
		
		// No match.
		return boost::none;
	}
}



GPlatesFeatureVisitors::TopologySectionsFinder::TopologySectionsFinder( 
		std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> &section_ptrs,
		std::vector<GPlatesModel::FeatureId> &section_ids,
		std::vector< std::pair<double, double> > &click_points,
		std::vector<bool> &reverse_flags):
	d_section_ptrs( &section_ptrs ),
	d_section_ids( &section_ids ),
	d_click_points( &click_points ),
	d_reverse_flags( &reverse_flags )
{
	d_section_ptrs->clear();
	d_section_ids->clear();
	d_click_points->clear();
	d_reverse_flags->clear();

	// clear the working vector
	d_table_rows.clear();
}



void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_feature_handle(
		GPlatesModel::FeatureHandle &feature_handle)
{
	// super short-cut for features without boundary list properties
	QString type("TopologicalClosedPlateBoundary");
	if ( type != GPlatesUtils::make_qstring_from_icu_string(
			feature_handle.feature_type().get_name() ) ) { 
		// Quick-out: No need to continue.
		return; 
	}

	// clear the working vector
	d_table_rows.clear();

	// else process this feature's properties:
	visit_feature_properties(feature_handle);
}



void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
	GPlatesModel::FeatureHandle::properties_iterator iter = feature_handle.properties_begin();
	GPlatesModel::FeatureHandle::properties_iterator end = feature_handle.properties_end();
	for ( ; iter != end; ++iter) {
		// Elements of this properties vector can be NULL pointers.  (See the comment in
		// "model/FeatureRevision.h" for more details.)
		if (*iter != NULL) 
		{
			(*iter)->accept_visitor(*this);
		}
	}
}


void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_top_level_property_inline(
		GPlatesModel::TopLevelPropertyInline &top_level_property_inline)
{
	visit_property_values(top_level_property_inline);
}


void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}



void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_gpml_piecewise_aggregation(
		GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
{
	std::vector<GPlatesPropertyValues::GpmlTimeWindow>::iterator iter =
			gpml_piecewise_aggregation.time_windows().begin();

	std::vector<GPlatesPropertyValues::GpmlTimeWindow>::iterator end =
			gpml_piecewise_aggregation.time_windows().end();

	for ( ; iter != end; ++iter) 
	{
		process_gpml_time_window(*iter);
	}
}


void
GPlatesFeatureVisitors::TopologySectionsFinder::process_gpml_time_window(
	GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
{
#ifdef DEBUG
std::cout << "TopologySectionsFinder::process_gpml_time_window()" << std::endl;
#endif
	gpml_time_window.time_dependent_value()->accept_visitor(*this);
	gpml_time_window.valid_time()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_gpml_topological_polygon(
	GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_toplogical_polygon)
{
#ifdef DEBUG
std::cout << "TopologySectionsFinder::visit_gpml_topological_polygon" << std::endl;
#endif
	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>::iterator 
		iter, end;
	iter = gpml_toplogical_polygon.sections().begin();
	end = gpml_toplogical_polygon.sections().end();
	// loop over all the sections
	for ( ; iter != end; ++iter) 
	{
		// save raw data
		d_section_ptrs->push_back( *iter );

		// set the GpmlTopologicalSection::non_null_ptr of the working row
		d_table_row.d_section_ptr = *iter;

		// visit the rest of the gpml 
		(*iter)->accept_visitor(*this);

		// append the working row to the vector
		d_table_rows.push_back( d_table_row );
	}
}

void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_gpml_topological_line_section(
	GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_toplogical_line_section)
{  
#ifdef DEBUG
std::cout << "TopologySectionsFinder::visit_gpml_topological_line_section" << std::endl;
#endif
	// source geom.'s value is a delegate 
	// DO NOT visit the delegate with:
	// ( gpml_toplogical_line_section.get_source_geometry() )->accept_visitor(*this); 

	// Rather, access directly
	GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type property_delegate_ptr =
			gpml_toplogical_line_section.get_source_geometry();
	GPlatesModel::FeatureId src_geom_id = property_delegate_ptr->feature_id();
	const GPlatesModel::PropertyName src_prop_name = property_delegate_ptr->target_property();

	// feature id
	d_section_ids->push_back( src_geom_id );

	d_table_row.d_feature_id = src_geom_id;

	// Set d_table_row.d_feature_ref from d_table_row.d_feature_id if we can.
	resolve_feature_id(d_table_row);
	// Also set d_table_row.d_geometry_property_opt from a suitable-looking property
	// that looks like it matches the PropertyDelegate. Assuming everything else
	// resolved ok.
	d_table_row.d_geometry_property_opt = find_properties_iterator(
			d_table_row.d_feature_ref, src_prop_name);

	// check for intersection and click points
	if ( gpml_toplogical_line_section.get_start_intersection() )
	{
		gpml_toplogical_line_section.get_start_intersection()->accept_visitor(*this);
	} 
	else if ( gpml_toplogical_line_section.get_end_intersection() )
	{
		gpml_toplogical_line_section.get_end_intersection()->accept_visitor(*this);
	} 
	else 
	{
		// FIXME: what to put here?
		// fill in an 'empty' point 	
		d_click_points->push_back( std::make_pair( 0, 0 ) );
		d_table_row.d_click_point = boost::none;
	}

	
	// use reverse 
	bool use_reverse = gpml_toplogical_line_section.get_reverse_order();
	d_reverse_flags->push_back( use_reverse );

	d_table_row.d_reverse = use_reverse;

#ifdef DEBUG
qDebug() << "  src_geom_id = " << GPlatesUtils::make_qstring_from_icu_string(src_geom_id.get());
qDebug() << "  use_reverse = " << use_reverse;
#endif
}


void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_gpml_topological_intersection(
	GPlatesPropertyValues::GpmlTopologicalIntersection &gpml_toplogical_intersection)
{  
#ifdef DEBUG
std::cout << "TopologySectionsFinder::visit_gpml_topological_intersection" << std::endl;
#endif
	// reference_point property value is a gml_point:
	GPlatesMaths::PointOnSphere pos =
		*( ( gpml_toplogical_intersection.reference_point() )->point() ); 

// std::cout << "llp=" << GPlatesMaths::make_lat_lon_point( pos ) << std::endl;

	double click_point_lat = (GPlatesMaths::make_lat_lon_point( pos )).latitude();
	double click_point_lon = (GPlatesMaths::make_lat_lon_point( pos )).longitude();
	
	d_click_points->push_back( std::make_pair( click_point_lat, click_point_lon ) );

	d_table_row.d_click_point = GPlatesMaths::make_lat_lon_point( pos );
}

void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_gpml_topological_point(
	GPlatesPropertyValues::GpmlTopologicalPoint &gpml_toplogical_point)
{  
#ifdef DEBUG
std::cout << "TopologySectionsFinder::visit_gpml_topological_point" << std::endl;
#endif
	// DO NOT visit the delegate with:
	// ( gpml_toplogical_line_section.get_source_geometry() )->accept_visitor(*this); 

	// Access directly the data
	GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type property_delegate_ptr =
			gpml_toplogical_point.get_source_geometry();
	GPlatesModel::FeatureId	src_geom_id = property_delegate_ptr->feature_id();
	const GPlatesModel::PropertyName src_prop_name = property_delegate_ptr->target_property();

	// fill the vectors
	d_section_ids->push_back( src_geom_id );

	d_table_row.d_feature_id = src_geom_id;
	
	// Set d_table_row.d_feature_ref from d_table_row.d_feature_id if we can.
	resolve_feature_id(d_table_row);
	// Also set d_table_row.d_geometry_property_opt from a suitable-looking property
	// that looks like it matches the PropertyDelegate. Assuming everything else
	// resolved ok.
	d_table_row.d_geometry_property_opt = find_properties_iterator(
			d_table_row.d_feature_ref, src_prop_name);

	// fill in an 'empty' flag 	
	d_reverse_flags->push_back( false );
	d_table_row.d_reverse = false;

	// fill in an 'empty' point 	
	d_click_points->push_back( std::make_pair( 0, 0 ) );
	d_table_row.d_click_point = boost::none;

#ifdef DEBUG
qDebug() << "  src_geom_id = " << GPlatesUtils::make_qstring_from_icu_string(src_geom_id.get());
qDebug() << "  use_reverse = " << use_reverse;
#endif
}
		


void
GPlatesFeatureVisitors::TopologySectionsFinder::report()
{
	std::cout << "-------------------------------------------------------------" << std::endl;
	std::cout << "TopologySectionsFinder::report()" << std::endl;
	std::cout << "number sections visited = " << d_section_ids->size() << std::endl;

	std::vector<GPlatesModel::FeatureId>::iterator f_itr = d_section_ids->begin();
	std::vector<std::pair<double, double> >::iterator c_itr = d_click_points->begin();
	std::vector<bool>::iterator r_itr = d_reverse_flags->begin();

	for ( ; f_itr != d_section_ids->end() ; ++f_itr, ++r_itr, ++c_itr)
	{
		qDebug() << "id =" << GPlatesUtils::make_qstring_from_icu_string( f_itr->get() );
		qDebug() << "reverse? = " << *r_itr;
		qDebug() << "click_point_lat = " << c_itr->first << "; lon = " << c_itr->second;
	}
	std::cout << "--                              --                         --" << std::endl;

	std::vector<GPlatesGui::TopologySectionsContainer::TableRow>::iterator tr_itr;
	tr_itr = d_table_rows.begin();
	
	// loop over rows
	for ( ; tr_itr != d_table_rows.end() ; ++tr_itr)
	{
		qDebug() << "id =" 
			<< GPlatesUtils::make_qstring_from_icu_string( (*tr_itr).d_feature_id.get() );
		qDebug() << "reverse? = " << (*tr_itr).d_reverse;
	}
	std::cout << "-------------------------------------------------------------" << std::endl;

}

