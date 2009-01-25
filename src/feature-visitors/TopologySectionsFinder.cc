/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-08-15 02:13:48 -0700 (Fri, 15 Aug 2008) $
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include "TopologySectionsFinder.h"

#include "feature-visitors/ValueFinder.h"
#include "feature-visitors/PlateIdFinder.h"
#include "feature-visitors/ReconstructedFeatureGeometryFinder.h"

#include "model/ReconstructedFeatureGeometry.h"
#include "model/Reconstruction.h"
#include "model/ReconstructionTree.h"
#include "model/FeatureHandle.h"
#include "model/InlinePropertyContainer.h"
#include "model/FeatureRevision.h"
#include "model/FeatureIdRegistry.h"

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

GPlatesFeatureVisitors::TopologySectionsFinder::TopologySectionsFinder( 
		std::vector<GPlatesModel::FeatureId> &section_ids,
		std::vector<bool> &reverse_flags):
	d_section_ids( &section_ids ),
	d_reverse_flags( &reverse_flags )
{
	d_section_ids->clear();
	d_reverse_flags->clear();
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
		if (*iter != NULL) {
			// FIXME: This d_current_property could go in the {Const,}FeatureVisitor base.
			(*iter)->accept_visitor(*this);
		}
	}
}


void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_inline_property_container(
		GPlatesModel::InlinePropertyContainer &inline_property_container)
{
	visit_property_values(inline_property_container);
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

#if 0
// FIXME: remove?
void
GPlatesFeatureVisitors::TopologySectionsFinder::visit_gpml_property_delegate(
	GPlatesPropertyValues::GpmlPropertyDelegate &gpml_property_delegate)
{
#ifdef DEBUG
std::cout << "TopologySectionsFinder::visit_gpml_property_delegate()" << std::endl;
#endif
}
#endif


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
		(*iter)->accept_visitor(*this);
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
	GPlatesModel::FeatureId src_geom_id = 
		( gpml_toplogical_line_section.get_source_geometry() )->feature_id();

	bool use_reverse = gpml_toplogical_line_section.get_reverse_order();

	// fill the vectors
	d_section_ids->push_back( src_geom_id );
	d_reverse_flags->push_back( use_reverse );

#ifdef DEBUG
qDebug() << "  src_geom_id = " << GPlatesUtils::make_qstring_from_icu_string(src_geom_id.get());
qDebug() << "  use_reverse = " << use_reverse;
#endif
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
	GPlatesModel::FeatureId	src_geom_id = 
		( gpml_toplogical_point.get_source_geometry() )->feature_id();

	// fill the vectors
	d_section_ids->push_back( src_geom_id );
	d_reverse_flags->push_back( false );

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

	std::vector<GPlatesModel::FeatureId>::iterator itr;
	itr = d_section_ids->begin();
	for ( ; itr != d_section_ids->end() ; ++itr)
	{
		qDebug() << "id =" 
			<< GPlatesUtils::make_qstring_from_icu_string( itr->get() );
	}
	std::cout << "-------------------------------------------------------------" << std::endl;
}

