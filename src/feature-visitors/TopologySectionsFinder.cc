/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-08-15 02:13:48 -0700 (Fri, 15 Aug 2008) $
 * 
 * Copyright (C) 2008, 2009 California Institute of Technology
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include "app-logic/TopologyInternalUtils.h"

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
#include "maths/LatLonPoint.h"
#include "maths/ProximityCriteria.h"
#include "maths/PolylineIntersections.h"

#include "utils/UnicodeStringUtils.h"


GPlatesFeatureVisitors::TopologySectionsFinder::TopologySectionsFinder()
{
	// clear the working vector
	d_table_rows.clear();
}



bool
GPlatesFeatureVisitors::TopologySectionsFinder::initialise_pre_feature_properties(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	// super short-cut for features without boundary list properties
	//
	// FIXME: Do this check based on feature properties rather than feature type.
	// So if something looks like a TCPB (because it has a topology polygon property)
	// then treat it like one. For this to happen we first need TopologicalNetwork to
	// use a property type different than TopologicalPolygon.
	//
	static const QString topology_boundary_type_name("TopologicalClosedPlateBoundary");
	static const QString topology_network_type_name("TopologicalNetwork");
	const QString feature_type = GPlatesUtils::make_qstring_from_icu_string( feature_handle.feature_type().get_name() );

	// Quick-out: No need to continue.
	if ( ( feature_type != topology_boundary_type_name ) &&
		( feature_type != topology_network_type_name ) )
	{
		return false; 
	}

	// clear the working vector
	d_table_rows.clear();

	// else process this feature's properties:
	return true;
}


void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}



void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_gpml_piecewise_aggregation(
		const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
{
	std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator iter =
			gpml_piecewise_aggregation.time_windows().begin();

	std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator end =
			gpml_piecewise_aggregation.time_windows().end();

	for ( ; iter != end; ++iter) 
	{
		process_gpml_time_window(*iter);
	}
}


void
GPlatesFeatureVisitors::TopologySectionsFinder::process_gpml_time_window(
		const GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
{
#ifdef DEBUG
std::cout << "TopologySectionsFinder::process_gpml_time_window()" << std::endl;
#endif
	gpml_time_window.time_dependent_value()->accept_visitor(*this);
	gpml_time_window.valid_time()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_gpml_topological_polygon(
		const GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_toplogical_polygon)
{
#ifdef DEBUG
std::cout << "TopologySectionsFinder::visit_gpml_topological_polygon" << std::endl;
#endif
	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>::const_iterator 
		iter, end;
	iter = gpml_toplogical_polygon.sections().begin();
	end = gpml_toplogical_polygon.sections().end();
	// loop over all the sections
	for ( ; iter != end; ++iter) 
	{
		// visit the rest of the gpml 
		(*iter)->accept_visitor(*this);
	}
}

void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_gpml_topological_line_section(
		const GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_toplogical_line_section)
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

	// check for click point
	boost::optional<GPlatesMaths::PointOnSphere> click_point = boost::none;

	//
	// The present day click point is the same for the start and end intersection.
	// It represents where the user clicked on the current section.
	//
	// The reference plate ids can be different though (for the start and end intersection)
	// but currently they are not used - instead the plate id of the current section is
	// used - it was like that before to allow testing different algorithms for click points
	// but it was decided that it makes most sense to rotate the click point with
	// the feature that was clicked.
	//

	// Just look for the first click point we can find.
	// If we don't find one then it means either both adjacent sections are point sections
	// and hence cannot intersect or that one or more of the adjacent sections are
	// line sections but that they were not intersecting when the topology was built.
	if ( gpml_toplogical_line_section.get_start_intersection() )
	{
		click_point = *gpml_toplogical_line_section.get_start_intersection()
				->reference_point()->point(); 
	} 
	else if ( gpml_toplogical_line_section.get_end_intersection() )
	{
		click_point = *gpml_toplogical_line_section.get_end_intersection()
				->reference_point()->point(); 
	} 

	// use reverse 
	const bool use_reverse = gpml_toplogical_line_section.get_reverse_order();

	const GPlatesGui::TopologySectionsContainer::TableRow table_row(
			src_geom_id, src_prop_name, use_reverse, click_point);

	// append the working row to the vector
	d_table_rows.push_back( table_row );

#ifdef DEBUG
qDebug() << "  src_geom_id = " << GPlatesUtils::make_qstring_from_icu_string(src_geom_id.get());
qDebug() << "  use_reverse = " << use_reverse;
#endif
}


void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_gpml_topological_point(
		const GPlatesPropertyValues::GpmlTopologicalPoint &gpml_toplogical_point)
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

	// No click point and no reverse for point sections.
	const GPlatesGui::TopologySectionsContainer::TableRow table_row(src_geom_id, src_prop_name);

	// append the working row to the vector
	d_table_rows.push_back( table_row );

#ifdef DEBUG
qDebug() << "  src_geom_id = " << GPlatesUtils::make_qstring_from_icu_string(src_geom_id.get());
#endif
}
		


void
GPlatesFeatureVisitors::TopologySectionsFinder::report()
{
	std::cout << "-------------------------------------------------------------" << std::endl;
	std::cout << "TopologySectionsFinder::report()" << std::endl;
	std::cout << "number sections visited = " << d_table_rows.size() << std::endl;

	GPlatesGui::TopologySectionsContainer::const_iterator section_iter = d_table_rows.begin();
	GPlatesGui::TopologySectionsContainer::const_iterator section_end = d_table_rows.end();

	for ( ; section_iter != section_end ; ++section_iter)
	{
		qDebug()
			<< "id ="
			<< GPlatesUtils::make_qstring_from_icu_string(section_iter->get_feature_id().get());
		qDebug()
			<< "reverse? = "
			<< section_iter->get_reverse();

		if (section_iter->get_present_day_click_point())
		{
			qDebug()
				<< "click_point lat = "
				<< GPlatesMaths::make_lat_lon_point(*section_iter->get_present_day_click_point()).latitude()
				<< "; lon = "
				<< GPlatesMaths::make_lat_lon_point(*section_iter->get_present_day_click_point()).longitude();
		}
	}
	std::cout << "-------------------------------------------------------------" << std::endl;
}

