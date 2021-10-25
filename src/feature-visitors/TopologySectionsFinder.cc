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

#include <boost/optional.hpp>

#include "TopologySectionsFinder.h"

#include "app-logic/Reconstruction.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/TopologyInternalUtils.h"

#include "model/FeatureHandle.h"
#include "model/FeatureHandleWeakRefBackInserter.h"
#include "model/FeatureRevision.h"
#include "model/TopLevelPropertyInline.h"

#include "maths/LatLonPoint.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PolylineIntersections.h"
#include "maths/PolylineOnSphere.h"
#include "maths/ProximityCriteria.h"

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
#include "property-values/GpmlTopologicalLine.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalNetwork.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/UninterpretedPropertyValue.h"
#include "property-values/StructuralType.h"
#include "property-values/XsString.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"

#include "utils/UnicodeStringUtils.h"


GPlatesFeatureVisitors::TopologySectionsFinder::TopologySectionsFinder()
{
	// clear the working vector
	d_boundary_sections.clear();
	d_interior_sections.clear();
}



bool
GPlatesFeatureVisitors::TopologySectionsFinder::initialise_pre_feature_properties(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	// NOTE: We don't test for topological feature types anymore.
	// If a feature has a topological polygon, line or network property then it will
	// get resolved, otherwise no reconstruction geometries will be generated.
	// We're not testing feature type because we're introducing the ability for any feature type
	// to allow a topological (or static) geometry property (at least for topological polygons/lines).

	// clear the working vectors
	d_boundary_sections.clear();
	d_interior_sections.clear();

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
		// FIXME: We really should be checking the time period of each time window against the
		// current reconstruction time - which means we should also have a reconstruction time.
		// It currently works because all topologies are currently created with a *single* time window
		// (ie, we don't actually have any real time-dependent topologies even though they're wrapped
		// in time-dependent wrappers).
		// However we won't fix this just yet because GPML files created with old versions of GPlates
		// set the time period, of the sole time window, to match that of the 'feature's time period
		// (in the topology build/edit tools) - newer versions set it to *all* time (distant past/future).
		// If the user expands the 'feature's time period *after* building/editing the topology then
		// the *un-adjusted* time window time period will be incorrect and hence we need to ignore it.
		// By the way, the time window is a *sole* time window because the topology tools cannot yet
		// create time-dependent topology (section) lists.
		process_gpml_time_window(*iter);
	}
}


void
GPlatesFeatureVisitors::TopologySectionsFinder::process_gpml_time_window(
		const GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
{
	gpml_time_window.time_dependent_value()->accept_visitor(*this);
	gpml_time_window.valid_time()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_gpml_topological_line(
 		const GPlatesPropertyValues::GpmlTopologicalLine &gpml_toplogical_line)
{
	// Set the sequence number
	d_seq_num = 0;

	// loop over all the sections
	GPlatesPropertyValues::GpmlTopologicalLine::sections_const_iterator iter =
			gpml_toplogical_line.sections_begin();
	GPlatesPropertyValues::GpmlTopologicalLine::sections_const_iterator end =
			gpml_toplogical_line.sections_end();
	for ( ; iter != end; ++iter) 
	{
		// visit the rest of the gpml 
		(*iter)->accept_visitor(*this);
	}
}


void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_gpml_topological_network(
		const GPlatesPropertyValues::GpmlTopologicalNetwork &gpml_toplogical_network)
{
	// Set the sequence number for the boundary sections.
	d_seq_num = 0;

	GPlatesPropertyValues::GpmlTopologicalNetwork::boundary_sections_const_iterator boundary_sections_iter =
			gpml_toplogical_network.boundary_sections_begin();
	GPlatesPropertyValues::GpmlTopologicalNetwork::boundary_sections_const_iterator boundary_sections_iter_end =
			gpml_toplogical_network.boundary_sections_end();
	// Loop over the boundary sections.
	for ( ; boundary_sections_iter != boundary_sections_iter_end; ++boundary_sections_iter) 
	{
		// Visit the current topological section.
		(*boundary_sections_iter)->accept_visitor(*this);
	}

	// Set the sequence number for the interior geometries.
	d_seq_num = 1;

	GPlatesPropertyValues::GpmlTopologicalNetwork::interior_geometries_const_iterator interior_geometries_iter =
			gpml_toplogical_network.interior_geometries_begin();
	GPlatesPropertyValues::GpmlTopologicalNetwork::interior_geometries_const_iterator interior_geometries_iter_end =
			gpml_toplogical_network.interior_geometries_end();
	// Loop over the interior geometries.
	for ( ; interior_geometries_iter != interior_geometries_iter_end; ++interior_geometries_iter) 
	{
		// Visit the current topological network interior.
		visit_gpml_topological_network_interior(*interior_geometries_iter);
	}
}

void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_gpml_topological_network_interior(
		const GPlatesPropertyValues::GpmlTopologicalNetwork::Interior &gpml_toplogical_network_interior)
{
	// source geom.'s value is a delegate 
	// DO NOT visit the delegate with:
	// ( gpml_toplogical_line_section.get_source_geometry() )->accept_visitor(*this); 

	// Rather, access directly
	GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type property_delegate_ptr =
			gpml_toplogical_network_interior.get_source_geometry();
	const GPlatesModel::FeatureId &src_geom_id = property_delegate_ptr->feature_id();
	const GPlatesModel::PropertyName &src_prop_name = property_delegate_ptr->target_property();

	// NOTE: A topological interior is *not* a topological section.
	// But for the meantime we treat it like one because the topology tools currently access
	// interior geometries via the topology *sections* table.
	//
	// FIXME: Move topology interiors out of the topology sections table and hence from the
	// topological sections container and finder.
	const GPlatesGui::TopologySectionsContainer::TableRow table_row(src_geom_id, src_prop_name);

	d_interior_sections.push_back(table_row);
}


void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_gpml_topological_polygon(
		const GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_toplogical_polygon)
{
	// Set the sequence number
	d_seq_num = 0;

	// loop over all the sections
	GPlatesPropertyValues::GpmlTopologicalPolygon::sections_const_iterator iter =
			gpml_toplogical_polygon.exterior_sections_begin();
	GPlatesPropertyValues::GpmlTopologicalPolygon::sections_const_iterator end =
			gpml_toplogical_polygon.exterior_sections_end();
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
	// source geom.'s value is a delegate 
	// DO NOT visit the delegate with:
	// ( gpml_toplogical_line_section.get_source_geometry() )->accept_visitor(*this); 

	// Rather, access directly
	GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type property_delegate_ptr =
			gpml_toplogical_line_section.get_source_geometry();
	GPlatesModel::FeatureId src_geom_id = property_delegate_ptr->feature_id();
	const GPlatesModel::PropertyName src_prop_name = property_delegate_ptr->target_property();

	// use reverse 
	const bool use_reverse = gpml_toplogical_line_section.get_reverse_order();

	const GPlatesGui::TopologySectionsContainer::TableRow table_row(
			src_geom_id, src_prop_name, use_reverse);

	// append the working row to the appropriate vector
	// Note: Currently topological sections can only form part of the *boundary* but we'll
	// test both in case that changes.
	if ( d_seq_num == 0 )
	{
		d_boundary_sections.push_back( table_row );
	}	
	else if ( d_seq_num == 1)
	{
		d_interior_sections.push_back( table_row );
	}
}


void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_gpml_topological_point(
		const GPlatesPropertyValues::GpmlTopologicalPoint &gpml_toplogical_point)
{  
	// DO NOT visit the delegate with:
	// ( gpml_toplogical_line_section.get_source_geometry() )->accept_visitor(*this); 

	// Access directly the data
	GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type property_delegate_ptr =
			gpml_toplogical_point.get_source_geometry();
	GPlatesModel::FeatureId	src_geom_id = property_delegate_ptr->feature_id();
	const GPlatesModel::PropertyName src_prop_name = property_delegate_ptr->target_property();

	// No click point and no reverse for point sections.
	const GPlatesGui::TopologySectionsContainer::TableRow table_row(src_geom_id, src_prop_name);

	// append the working row to the appropriate vector
	// Note: Currently topological sections can only form part of the *boundary* but we'll
	// test both in case that changes.
	if ( d_seq_num == 0 )
	{
		d_boundary_sections.push_back( table_row );
	}	
	else if ( d_seq_num == 1)
	{
		d_interior_sections.push_back( table_row );
	}
}
		

void
GPlatesFeatureVisitors::TopologySectionsFinder::report()
{
	qDebug() << "-------------------------------------------------------------";
	qDebug() << "TopologySectionsFinder::report()";
	qDebug() << " ";
	qDebug() << "Boundary sections found = " << d_boundary_sections.size();
	GPlatesGui::TopologySectionsContainer::const_iterator section_itr = d_boundary_sections.begin();
	GPlatesGui::TopologySectionsContainer::const_iterator section_end = d_boundary_sections.end();
	for ( ; section_itr != section_end ; ++section_itr)
	{
	qDebug() << "id =" << GPlatesUtils::make_qstring_from_icu_string(section_itr->get_feature_id().get());
	}
	qDebug() << "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -";
	qDebug() << "Interior sections found = " << d_interior_sections.size();
	section_itr = d_interior_sections.begin();
	section_end = d_interior_sections.end();
	for ( ; section_itr != section_end ; ++section_itr)
	{
	qDebug() << "id =" << GPlatesUtils::make_qstring_from_icu_string(section_itr->get_feature_id().get());
	}
	qDebug() << "-------------------------------------------------------------";
}

