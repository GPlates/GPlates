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


#if 0
#define DEBUG
#define DEBUG_VISIT
#define DEBUG_RESOLVE_INTERSECTION
#define DEBUG_RESOLVE_BOUNDARY
#define DEBUG_GET_VERTEX_LIST
#define DEBUG_BOUNDS
#define DEBUG_POINT_IN_POLYGON
#endif


#ifdef _MSC_VER
#define copysign _copysign
#endif

#include "TopologyResolver.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/FeatureHandleWeakRefBackInserter.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "model/ReconstructedFeatureGeometryFinder.h"
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

GPlatesFeatureVisitors::TopologyResolver::TopologyResolver(
			const double &recon_time,
			unsigned long root_plate_id,
			GPlatesModel::Reconstruction &recon,
			GPlatesModel::ReconstructionTree &recon_tree,
			reconstruction_geometries_type &reconstructed_geometries,
			bool should_keep_features_without_recon_plate_id):
	d_recon_time(GPlatesPropertyValues::GeoTimeInstant(recon_time)),
	d_root_plate_id(GPlatesModel::integer_plate_id_type(root_plate_id)),
	d_recon_ptr(&recon),
	d_recon_tree_ptr(&recon_tree),
	d_reconstruction_geometries_to_populate(&reconstructed_geometries),
	d_should_keep_features_without_recon_plate_id(should_keep_features_without_recon_plate_id)
{  
	d_num_features = 0;
	d_num_topologies = 0;
}



void
GPlatesFeatureVisitors::TopologyResolver::visit_feature_handle(
		GPlatesModel::FeatureHandle &feature_handle)
{
	d_num_features += 1;

	// super short-cut for features without boundary list properties
	QString type("TopologicalClosedPlateBoundary");
	if ( type != GPlatesUtils::make_qstring_from_icu_string(
			feature_handle.feature_type().get_name() ) ) 
	{ 
		// Quick-out: No need to continue.
		return; 
	}

	// else process this feature:
	// create an accumulator struct 
	// visit the properties once to check times and rot ids
	// visit the properties once to reconstruct 

	// resolve the boundary vertex_list

	d_accumulator = ReconstructedFeatureGeometryAccumulator();

	// Now visit each of the properties in turn, twice -- firstly, to find a reconstruction
	// plate ID and to determine whether the feature is defined at this reconstruction time;
	// after that, to perform the reconstructions (if appropriate) using the plate ID.

	// The first time through, we're not reconstructing, just gathering information.
	d_accumulator->d_perform_reconstructions = false;

	visit_feature_properties(feature_handle);


	// So now we've visited the properties of this feature.  Let's find out if we were able
	// to obtain all the information we need.
	if ( ! d_accumulator->d_feature_is_defined_at_recon_time) {
		// Quick-out: No need to continue.
		d_accumulator = boost::none;
		return;
	}


	if ( ! d_accumulator->d_recon_plate_id) 
	{
		// We couldn't obtain the reconstruction plate ID.
		// So, how do we react to this situation?  Do we ignore features which don't have a
		// reconsruction plate ID, or do we "reconstruct" their geometries using the
		// identity rotation, so the features simply sit still on the globe?  Fortunately,
		// the client code has already told us how it wants us to behave...
		if ( ! d_should_keep_features_without_recon_plate_id) {
			d_accumulator = boost::none;
			return;
		}
	}
	else
	{
		// We obtained the reconstruction plate ID.  We now have all the information we
		// need to reconstruct according to the reconstruction plate ID.
		d_accumulator->d_recon_rotation =
			d_recon_tree_ptr->get_composed_absolute_rotation(*(d_accumulator->d_recon_plate_id)).first;
	}

	// Now for the second pass through the properties of the feature:  
	// This time we reconstruct any geometries we find.
	d_accumulator->d_perform_reconstructions = true;


	// Create a PlatePolygon struct to hold the results of resolving topology props.
	PlatePolygon plate;

	// Clear the boundary list befor the visit to properties
	d_boundary_list.clear();

	visit_feature_properties(feature_handle);

	d_num_topologies += 1;

	// fill the PlatePolygon struct:
	// Iterate over d_boundary_list to generate a list of vertices and 
	// via compute_bounds( plate), set the other plate.d_FOO variables
	resolve_boundary( plate );

	// insert the plate into the map
	d_fid_polygon_pair_list.push_back( 
						std::make_pair( feature_handle.feature_id().get(), plate ) );

	d_accumulator = boost::none;
}



void
GPlatesFeatureVisitors::TopologyResolver::visit_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
	GPlatesModel::FeatureHandle::properties_iterator iter = feature_handle.properties_begin();
	GPlatesModel::FeatureHandle::properties_iterator end = feature_handle.properties_end();
	for ( ; iter != end; ++iter) {
		// Elements of this properties vector can be NULL pointers.  (See the comment in
		// "model/FeatureRevision.h" for more details.)
		if (*iter != NULL) {
			// FIXME: This d_current_property could go in the {Const,}FeatureVisitor base.
			d_accumulator->d_current_property = iter;
			(*iter)->accept_visitor(*this);
		}
	}
}


void
GPlatesFeatureVisitors::TopologyResolver::visit_top_level_property_inline(
		GPlatesModel::TopLevelPropertyInline &top_level_property_inline)
{
	visit_property_values( top_level_property_inline );
}


void
GPlatesFeatureVisitors::TopologyResolver::visit_gml_time_period(
		GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
	static const GPlatesModel::PropertyName valid_time_property_name =
		GPlatesModel::PropertyName::create_gml("validTime");

	if ( ! d_accumulator->d_perform_reconstructions) {
		// We're gathering information, not performing reconstructions.

		// Note that we're going to assume that we're in a property...
		if (d_accumulator->current_property_name() == valid_time_property_name) {
			// This time period is the "valid time" time period.
			if ( ! gml_time_period.contains(d_recon_time)) {
				// Oh no!  This feature instance is not defined at the recon time!
				d_accumulator->d_feature_is_defined_at_recon_time = false;
			}
		}
	}
}


void
GPlatesFeatureVisitors::TopologyResolver::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::TopologyResolver::visit_gpml_plate_id(
		GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	static GPlatesModel::PropertyName reconstruction_plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

	if ( ! d_accumulator->d_perform_reconstructions) {
		// We're gathering information, not performing reconstructions.

		// Note that we're going to assume that we're in a property...
		if (d_accumulator->current_property_name() == reconstruction_plate_id_property_name) {
			// This plate ID is the reconstruction plate ID.
			d_accumulator->d_recon_plate_id = gpml_plate_id.value();
		}
	}
}





void
GPlatesFeatureVisitors::TopologyResolver::visit_gpml_piecewise_aggregation(
		GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
{
	if ( ! d_accumulator->d_perform_reconstructions ) {
		return;
	}

	//d_output.writeStartGpmlElement("PiecewiseAggregation");
	//d_output.writeStartGpmlElement("valueType");
	//writeTemplateTypeParameterType(d_output, gpml_piecewise_aggregation.value_type());
	//d_output.writeEndElement();

	std::vector<GPlatesPropertyValues::GpmlTimeWindow>::iterator iter =
			gpml_piecewise_aggregation.time_windows().begin();

	std::vector<GPlatesPropertyValues::GpmlTimeWindow>::iterator end =
			gpml_piecewise_aggregation.time_windows().end();

	for ( ; iter != end; ++iter) 
	{
		//d_output.writeStartGpmlElement("timeWindow");
		write_gpml_time_window(*iter);
	}
}


void
GPlatesFeatureVisitors::TopologyResolver::write_gpml_time_window(
	GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
{
	gpml_time_window.time_dependent_value()->accept_visitor(*this);
	gpml_time_window.valid_time()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::TopologyResolver::visit_gpml_topological_polygon(
	GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_toplogical_polygon)
{
	if ( ! d_accumulator->d_perform_reconstructions ) {
		return;
	}

	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>::iterator 
		iter, end;

	iter = gpml_toplogical_polygon.sections().begin();
	end = gpml_toplogical_polygon.sections().end();

	// set the default d_FOO vars
	d_ref_point_lat = 0.0;
	d_ref_point_lon = 0.0;

	// loop over all the sections
	for ( ; iter != end; ++iter) 
	{
		// visit the properties, set the local d_FOO vars
		(*iter)->accept_visitor(*this);

		// create a boundary feature node for this section
		create_boundary_node();
	}

}

void
GPlatesFeatureVisitors::TopologyResolver::visit_gpml_topological_line_section(
	GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_toplogical_line_section)
{  
	if ( ! d_accumulator->d_perform_reconstructions ) {
		return;
	}
#ifdef DEBUG_VISIT
std::cout << "TopologyResolver::visit_gpml_topological_line_section" << std::endl;
#endif

	// This is a line type feature
	d_type = GPlatesGlobal::LINE_FEATURE;

	// source geom.'s value is a delegate 
	// DO NOT visit the delegate with:
	// ( gpml_toplogical_line_section.get_source_geometry() )->accept_visitor(*this); 

	// Rather, access directly
	GPlatesModel::FeatureId src_geom_fid = 
		( gpml_toplogical_line_section.get_source_geometry() )->feature_id();

#ifdef DEBUG_VISIT
qDebug() << "TopologyResolver::visit_gpml_topological_line_section: src_geom_fid=" 
	<< GPlatesUtils::make_qstring_from_icu_string( src_geom_fid.get() );
#endif


	// set the member data for create_boundary() call
	d_src_geom_fid = ( gpml_toplogical_line_section.get_source_geometry() )->feature_id();

	// clear the working vertex list
	d_working_vertex_list.clear();

	// fill the working vertex list
	get_vertex_list_from_feature_id( d_working_vertex_list, d_src_geom_fid);
	
	// Set reverse flag 
	d_use_reverse = gpml_toplogical_line_section.get_reverse_order();
#ifdef DEBUG_VISIT
qDebug() << "TopologyResolver::visit_gpml_topological_line_section: use_reverse=" << d_use_reverse;
#endif


	// Check for, and process intersections
	if ( gpml_toplogical_line_section.get_start_intersection() )
	{
#ifdef DEBUG_VISIT
qDebug() << "TopologyResolver::visit_gpml_topological_line_section: FOUND: start_intersection";
#endif

		// DO NOT VISIT with this call:
		// gpml_toplogical_line_section.get_start_intersection()->accept_visitor(*this);
		// Rather, access the start_intersection directly:
		GPlatesPropertyValues::GpmlTopologicalIntersection &start =
			gpml_toplogical_line_section.get_start_intersection().get();

		// intersection geometry property value is a PropertyDelegate
		// DO NOT visit the delegate with:
		// gpml_toplogical_intersection.intersection_geometry()->accept_visitor(*this); 
		// Rather, access the data directly:
		// first, get the feature id:
		GPlatesModel::FeatureId intersection_geom_fid = 
			( start.intersection_geometry() )->feature_id();
#ifdef DEBUG_VISIT
qDebug() << "TopologyResolver::visit_gpml_topological_line_section: intersection_geom_fid=" 
<< GPlatesUtils::make_qstring_from_icu_string(intersection_geom_fid.get());
#endif

		// next, get the vertices for this intersection_geometry

		// Fill the working vertex list
		d_node2_vertex_list.clear();
		get_vertex_list_from_feature_id(
			d_node2_vertex_list,
			intersection_geom_fid);

		// reference_point property value is a gml_point: 
		GPlatesMaths::PointOnSphere pos = 
			*( ( start.reference_point() )->point() );
#ifdef DEBUG_VISIT
std::cout << "TopologyResolver::visit_gpml_topological_line_section: reference_point= " << GPlatesMaths::make_lat_lon_point( pos ) << std::endl;
#endif

		// Fill the working data
		d_ref_point_ptr = ( ( start.reference_point() )->point() ).get();

		// reference_point_plate_id property value is a PropertyDelegate
		// DO NOT visit the delegate with:
		// gpml_toplogical_intersection.intersection_geometry()->accept_visitor(*this); 
		// Rather, access the data directly
		// first, get the feature id:
		GPlatesModel::FeatureId ref_point_plate_id_fid = 
			( start.reference_point_plate_id() )->feature_id();
#ifdef DEBUG_VISIT
qDebug() << "TopologyResolver::visit_gpml_topological_line_section: ref_point_plate_id_fid=" 
<< GPlatesUtils::make_qstring_from_icu_string( ref_point_plate_id_fid.get() );
#endif
		// Fill the working data
		d_ref_point_plate_id_fid = ref_point_plate_id_fid;

#ifdef DEBUG_VISIT
		// next, get the target_property name
		const GPlatesModel::PropertyName &prop_name =
			( start.reference_point_plate_id() )->target_property();
qDebug() << "TopologyResolver::visit_gpml_topological_line_section: ref_point_plate_id_prop_name =" 
<< GPlatesUtils::make_qstring_from_icu_string( prop_name.get_name() );
#endif

		// resolve the intersection and fill more of the d_ variables
		resolve_intersection(
			src_geom_fid,
			intersection_geom_fid,
			GPlatesFeatureVisitors::TopologyResolver::INTERSECT_PREV );
	}

	if ( gpml_toplogical_line_section.get_end_intersection() )
	{
#ifdef DEBUG_VISIT
qDebug() << "TopologyResolver::visit_gpml_topological_line_section: FOUND: end_intersection";
#endif

		// DO NOT VISIT with this call:
		// gpml_toplogical_line_section.get_end_intersection()->accept_visitor(*this);
		// Rather, access the end_intersection directly:
		GPlatesPropertyValues::GpmlTopologicalIntersection &end =
			gpml_toplogical_line_section.get_end_intersection().get();

		// intersection geometry property value is a PropertyDelegate
		// DO NOT visit the delegate with:
		// gpml_toplogical_intersection.intersection_geometry()->accept_visitor(*this); 
		// Rather, access the data directly:
		// first, get the feature id:
		GPlatesModel::FeatureId intersection_geom_fid = 
			( end.intersection_geometry() )->feature_id();

#ifdef DEBUG_VISIT
qDebug() << "TopologyResolver::visit_gpml_topological_line_section: intersection_geom_fid=" 
<< GPlatesUtils::make_qstring_from_icu_string(intersection_geom_fid.get());
#endif

		// next, get the vertices for this intersection_geometry

		// Fill the working vertex list
		d_node2_vertex_list.clear();
		get_vertex_list_from_feature_id(
			d_node2_vertex_list,
			intersection_geom_fid);

		// reference_point property value is a gml_point: 
		GPlatesMaths::PointOnSphere pos = 
			*( ( end.reference_point() )->point() );
#ifdef DEBUG_VISIT
std::cout << "TopologyResolver::visit_gpml_topological_line_section: reference_point= " << GPlatesMaths::make_lat_lon_point( pos ) << std::endl;
#endif

		// Fill the working data
		d_ref_point_ptr = ( ( end.reference_point() )->point() ).get();

		// reference_point_plate_id property value is a PropertyDelegate
		// DO NOT visit the delegate with:
		// gpml_toplogical_intersection.intersection_geometry()->accept_visitor(*this); 
		// Rather, access the data directly
		// first, get the feature id:
		GPlatesModel::FeatureId ref_point_plate_id_fid = 
			( end.reference_point_plate_id() )->feature_id();
#ifdef DEBUG_VISIT
qDebug() << "TopologyResolver::visit_gpml_topological_line_section: ref_point_plate_id_fid=" 
<< GPlatesUtils::make_qstring_from_icu_string( ref_point_plate_id_fid.get() );
#endif

		// Fill the working data
		d_ref_point_plate_id_fid = ref_point_plate_id_fid;

#ifdef DEBUG_VISIT
		// next, get the target_property name
		const GPlatesModel::PropertyName &prop_name =
			( end.reference_point_plate_id() )->target_property();
qDebug() << "TopologyResolver::visit_gpml_topological_line_section: ref_point_plate_id_prop_name =" 
<< GPlatesUtils::make_qstring_from_icu_string( prop_name.get_name() );
#endif

		// resolve the intersection and fill more of the d_ variables
		resolve_intersection(
			src_geom_fid,
			intersection_geom_fid,
			GPlatesFeatureVisitors::TopologyResolver::INTERSECT_NEXT );

	}

}



void
GPlatesFeatureVisitors::TopologyResolver::resolve_intersection( 
	GPlatesModel::FeatureId source_geometry_feature_id,
	GPlatesModel::FeatureId intersection_geometry_feature_id,
	GPlatesFeatureVisitors::TopologyResolver::NeighborRelation relation)
{
#ifdef DEBUG
std::cout << "TopologyResolver::resolve_intersection: " << std::endl;
//std::cout << "TopologyResolver::resolve_intersection: src_geometry_id= " << src_geometry_id << std::endl;
//std::cout << "TopologyResolver::resolve_intersection: intersection_id= " << intersection_id << std::endl;
std::cout << "TopologyResolver::resolve_intersection: d_working_vertex_list.size=" << d_working_vertex_list.size() << std::endl;
std::cout << "TopologyResolver::resolve_intersection: node2_vertex_list.size=" << d_node2_vertex_list.size() << std::endl;
#endif

	// Double check working lists: 
	if ( d_working_vertex_list.size() < 2 )
	{
		// FIXME : freak out!
		std::cerr << "TopologyResolver::resolve_intersection: " << std::endl
		<< "WARN: d_working_vertex_list < 2 ; Unable to create polyline." << std::endl
		// << "WARN: src_geometry_id= " << src_geometry_id 
		<< std::endl;
		std::cerr << "d_working_vertex_list.size=" << d_working_vertex_list.size() << std::endl;
		return;
	}

	if ( d_node2_vertex_list.size() < 2 )
	{
		// FIXME : freak out!
		std::cerr << "TopologyResolver::resolve_intersection: " << std::endl
		<< "WARN: d_node2_vertex_list < 2 ; Unable to create polyline." << std::endl
		// << "WARN: src_geometry_id= " << src_geometry_id << std::endl
		// << "WARN: intersection_id= " << intersection_id 
		<< std::endl;
		std::cerr << "node2_vertex_list.size=" << d_node2_vertex_list.size() << std::endl;
		return;
	}

	// test for intersection and set node relation:

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type node1_polyline =
		GPlatesMaths::PolylineOnSphere::create_on_heap( d_working_vertex_list );

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type node2_polyline =
		GPlatesMaths::PolylineOnSphere::create_on_heap( d_node2_vertex_list );



	// variables to save results of intersection
	std::list<GPlatesMaths::PointOnSphere> intersection_points;
	std::list<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> partitioned_lines;

	int num_intersect = 0;
	num_intersect = GPlatesMaths::PolylineIntersections::partition_intersecting_polylines(
		*node1_polyline,
		*node2_polyline,
		intersection_points,
		partitioned_lines);

#ifdef DEBUG_RESOLVE_INTERSECTION
std::cout << "TopologyResolver::resolve_intersection: " 
<< "num_intersect = " << num_intersect << std::endl;
#endif

	// switch on relation enum to set node1's member data
	switch ( relation )
	{
		case GPlatesFeatureVisitors::TopologyResolver::INTERSECT_PREV :
#ifdef DEBUG_RESOLVE_INTERSECTION
std::cout << "TopologyResolver::resolve_intersection: INTERSECT_PREV: " << std::endl;
#endif
			d_num_intersections_with_prev = num_intersect;
			break;

		case GPlatesFeatureVisitors::TopologyResolver::INTERSECT_NEXT:
#ifdef DEBUG_RESOLVE_INTERSECTION
std::cout << "TopologyResolver::resolve_intersection: INTERSECT_NEXT: " << std::endl;
#endif
			d_num_intersections_with_next = num_intersect;
			break;

		case GPlatesFeatureVisitors::TopologyResolver::NONE :
		case GPlatesFeatureVisitors::TopologyResolver::OTHER :
		default :
			// somthing bad happened freak out
			break;
	}


	if ( num_intersect == 0 )
	{
		// no change to node1
		// clean up
		//delete node1_polyline.get();
		//delete node2_polyline.get();
		return;
	}
	else if ( num_intersect == 1)
	{
		// pair of polyline lists from intersection
		std::pair<
			std::list< GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
			std::list< GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
		> parts;

		// unambiguously identify partitioned lines:
		// parts.first.front is the head of node1_polyline
		// parts.first.back is the tail of node1_polyline
		// parts.second.front is the head of node2_polyline
		// parts.second.back is the tail of node2_polyline
		parts = GPlatesMaths::PolylineIntersections::identify_partitioned_polylines(
			*node1_polyline,
			*node2_polyline,
			intersection_points,
			partitioned_lines);

		// clean up
		//delete node1_polyline.get();
		//delete node2_polyline.get();

#ifdef DEBUG_RESOLVE_INTERSECTION
GPlatesMaths::PolylineOnSphere::vertex_const_iterator iter, end;
iter = parts.first.front()->vertex_begin();
end = parts.first.front()->vertex_end();
std::cout << "TopologyResolver::resolve_intersection: HEAD: verts:" << std::endl;
for ( ; iter != end; ++iter) {
std::cout << "TopologyResolver::resolve_intersection: llp=" << GPlatesMaths::make_lat_lon_point(*iter) << std::endl;
}

iter = parts.first.back()->vertex_begin();
end = parts.first.back()->vertex_end();
std::cout << "TopologyResolver::resolve_intersection: TAIL: verts:" << std::endl;
for ( ; iter != end; ++iter) {
std::cout << "TopologyResolver::resolve_intersection: llp=" << GPlatesMaths::make_lat_lon_point(*iter) << std::endl;
}
#endif

		// now check which element of parts.first
		// is closest to ref_point:
		const GPlatesMaths::PointOnSphere *proximity_test_point_ptr;

		// rotate the click point with the plate id set in d_ref_point_plate_id_fid
		// before calling is_close_to()

		// set the pointer by default
		proximity_test_point_ptr = d_ref_point_ptr;

		// save the un-rotated click points
		d_ref_point_list.push_back( *d_ref_point_ptr );

		// Get a vector of FeatureHandle weak_refs for this FeatureId
		std::vector<GPlatesModel::FeatureHandle::weak_ref> back_refs;
		d_ref_point_plate_id_fid.find_back_ref_targets( append_as_weak_refs( back_refs ) );

		// Double check refs
		if ( back_refs.size() == 0 )
		{
			qDebug() << "ERROR: resolve_intersection()";
			qDebug() << "ERROR: No Feature found for feature id =";
			qDebug() << 
				GPlatesUtils::make_qstring_from_icu_string( d_ref_point_plate_id_fid.get() );
			qDebug() << "ERROR: Unable to rotate proximity test point for intersection test.";
			qDebug() << " ";
			// FIXME: what else to do?
		}

		// get a feature handle for the d_ref_point_plate_id_fid
		GPlatesModel::FeatureHandle::weak_ref ref_point_feature_ref = back_refs.front();

		// get the plate id for that feature
		static const GPlatesModel::PropertyName plate_id_property_name =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		const GPlatesPropertyValues::GpmlPlateId *recon_plate_id;

		if ( GPlatesFeatureVisitors::get_property_value(
			*ref_point_feature_ref, plate_id_property_name, recon_plate_id ) )
		{
			// The feature has a reconstruction plate ID.
#ifdef DEBUG_RESOLVE_INTERSECTION
std::cout << "TopologyResolver::resolve_intersection: MOVE the click point with plate id " << recon_plate_id << std::endl;
#endif
			const GPlatesMaths::FiniteRotation &r = 
				d_recon_tree_ptr->get_composed_absolute_rotation(
					recon_plate_id->value() 
				).first;

			// reconstruct the point 
			const GPlatesMaths::PointOnSphere recon_point = 
				GPlatesMaths::operator*(r, *d_ref_point_ptr);

			// reset the pointer
			proximity_test_point_ptr = &recon_point;

			// save the rotated click points
			d_proximity_point_list.push_back( *proximity_test_point_ptr );
		}

		// test PROXIMITY
		GPlatesMaths::real_t closeness_inclusion_threshold = 0.9;
		const GPlatesMaths::real_t cit_sqrd =
			closeness_inclusion_threshold * closeness_inclusion_threshold;
		const GPlatesMaths::real_t latitude_exclusion_threshold = sqrt(1.0 - cit_sqrd);

		// these get filled by calls to is_close_to()
		bool click_close_to_head;
		bool click_close_to_tail;
		GPlatesMaths::real_t closeness_head;
		GPlatesMaths::real_t closeness_tail;

		// set head closeness
		click_close_to_head = parts.first.front()->is_close_to(
			*proximity_test_point_ptr,
			closeness_inclusion_threshold,
			latitude_exclusion_threshold,
			closeness_head);

		// set tail closeness
		click_close_to_tail = parts.first.back()->is_close_to(
			*proximity_test_point_ptr,
			closeness_inclusion_threshold,
			latitude_exclusion_threshold,
			closeness_tail);


		// Make sure that the click point is close to /something/ !!!
		if ( !click_close_to_head && !click_close_to_tail ) 
		{
			std::cerr << "TopologyResolver::resolve_intersection: " << std::endl
				<< "WARN: click point not close to anything!" << std::endl
				<< "WARN: Unable to set boundary feature intersection flags!" 
				// << "WARN: src_geometry_id= " << src_geometry_id << std::endl
				// << "WARN: intersection_id= " << intersection_id << std::endl;
				<< std::endl;
			qDebug() << "WARN: source_geometry_feature_id=" 
			<< GPlatesUtils::make_qstring_from_icu_string(source_geometry_feature_id.get() );
			qDebug() << "WARN: intersection_geometry_feature_id=" 
			<< GPlatesUtils::make_qstring_from_icu_string(intersection_geometry_feature_id.get());
			// FIXME : freak out!
		}

//#ifdef DEBUG_RESOLVE_INTERSECTION
#ifdef DEBUG
std::cout << "TopologyResolver::resolve_intersection: "
<< "closeness_head=" << closeness_head << " and " 
<< "closeness_tail=" << closeness_tail << std::endl;
#endif

		// now compare the closeness values to set relation
		if ( closeness_head > closeness_tail )
		{

//#ifdef DEBUG_RESOLVE_INTERSECTION
#ifdef DEBUG
qDebug() << "TopologyResolver::resolve_intersection: use HEAD of: " 
<< GPlatesUtils::make_qstring_from_icu_string( source_geometry_feature_id.get() );
#endif

			d_closeness = closeness_head;

			// switch on the relation to be set
			switch ( relation )
			{
				case GPlatesFeatureVisitors::TopologyResolver::INTERSECT_PREV :
					d_use_head_from_intersect_prev = true;
					d_use_tail_from_intersect_prev = false;
					break;
	
				case GPlatesFeatureVisitors::TopologyResolver::INTERSECT_NEXT:
					d_use_head_from_intersect_next = true;
					d_use_tail_from_intersect_next = false;
					break;

				default:
					break;
			}
			return; // node1's relation has been set
		} 
		else if ( closeness_tail > closeness_head )
		{
			d_closeness = closeness_tail;
//#ifdef DEBUG_RESOLVE_INTERSECTION
#ifdef DEBUG
qDebug() << "TopologyResolver::resolve_intersection: use TAIL of: " 
<< GPlatesUtils::make_qstring_from_icu_string( source_geometry_feature_id.get() );
#endif

			// switch on the relation to be set
			switch ( relation )
			{
				case GPlatesFeatureVisitors::TopologyResolver::INTERSECT_PREV :
					d_use_tail_from_intersect_prev = true;
					d_use_head_from_intersect_prev = false;
					break;
	
				case GPlatesFeatureVisitors::TopologyResolver::INTERSECT_NEXT:
					d_use_tail_from_intersect_next = true;
					d_use_head_from_intersect_next = false;
					break;

				default:
					break;
			}
			return; // node1's relation has been set
		} 

	} // end of else if ( num_intersect == 1 )
	else 
	{
		// num_intersect must be 2 or greater
		std::cerr << "TopologyResolver::resolve_intersection: " << std::endl
			<< "WARN: num_intersect=" << num_intersect << std::endl 
			<< "WARN: Unable to set boundary feature intersection relations!" << std::endl
			<< "WARN: Make sure boundary feature's only intersect once." << std::endl
			// << "WARN: src geometry id= " << src_geometry_id << std::endl
			// << "WARN: intersection id= " << intersection_id << std::endl;
			<< std::endl;
		qDebug() << "WARN: source_geometry_feature_id=" 
		<< GPlatesUtils::make_qstring_from_icu_string( source_geometry_feature_id.get() );
		qDebug() << "WARN: intersection_geometry_feature_id=" 
		<< GPlatesUtils::make_qstring_from_icu_string(intersection_geometry_feature_id.get());
		std::cerr << std::endl;
		// FIXME : freak out!
		return;
	}
}


void
GPlatesFeatureVisitors::TopologyResolver::visit_gpml_topological_point(
	GPlatesPropertyValues::GpmlTopologicalPoint &gpml_toplogical_point)
{  
#ifdef DEBUG_VISIT
std::cout << "TopologyResolver::visit_gpml_topological_point" << std::endl;
#endif
	if ( ! d_accumulator->d_perform_reconstructions ) {
		return;
	}

	// FIXME: should we visit the delegate?
	// ( gpml_toplogical_line_section.get_source_geometry() )->accept_visitor(*this); 

	// This is a line type feature
	d_type = GPlatesGlobal::POINT_FEATURE;

	// Access  directly the data
	d_src_geom_fid = ( gpml_toplogical_point.get_source_geometry() )->feature_id();

#ifdef DEBUG_VISIT
qDebug() << "fid=" << GPlatesUtils::make_qstring_from_icu_string( d_src_geom_fid.get() );
#endif
}
		


void
GPlatesFeatureVisitors::TopologyResolver::create_boundary_node()
{
	GPlatesGlobal::FeatureTypes feature_type(GPlatesGlobal::UNKNOWN_FEATURE);

	switch ( d_type )
	{
		case GPlatesGlobal::POINT_FEATURE:
			feature_type = GPlatesGlobal::POINT_FEATURE;
			break;

		case GPlatesGlobal::LINE_FEATURE:
			feature_type = GPlatesGlobal::LINE_FEATURE;
			break;

		default :
			// FIXME error
			break;
	}

	// convert coordinates
	GPlatesMaths::LatLonPoint llp( d_ref_point_lat, d_ref_point_lon);
	GPlatesMaths::PointOnSphere click_point = GPlatesMaths::make_point_on_sphere(llp);

	// empty list place holder
 	std::list<GPlatesMaths::PointOnSphere> empty_vert_list;
 	empty_vert_list.clear();

	// create a boundary feature struct
	BoundaryFeature bf(
				d_src_geom_fid,
				feature_type,
				empty_vert_list,
				click_point,
				d_closeness,
				d_use_reverse,
				d_num_intersections_with_prev,
				d_num_intersections_with_next,
				d_use_head_from_intersect_prev,
				d_use_tail_from_intersect_prev,
				d_use_head_from_intersect_next,
				d_use_tail_from_intersect_next
			);

	// add this node to the list
	d_boundary_list.push_back( bf );
}

void
GPlatesFeatureVisitors::TopologyResolver::resolve_boundary( PlatePolygon &plate )
{
#ifdef DEBUG_RESOLVE_BOUNDARY
std::cout << "TopologyResolver::resolve_boundary() " << std::endl;
#endif


	// clear the working list
	plate.d_vertex_list.clear();

	// Iterate over the list of boundary features to get the list of vertices 
	plate.d_vertex_list = get_vertex_list( d_boundary_list.begin(), d_boundary_list.end() );

// FIXME: diagnostic output
#ifdef DEBUG_RESOLVE_BOUNDARY
BoundaryFeatureList_type::iterator iter = d_boundary_list.begin();
for ( ; iter != d_boundary_list.end() ; ++iter) 
{
qDebug() << "node id = " 
<< GPlatesUtils::make_qstring_from_icu_string( (iter->m_feature_id).get() );
}
std::cout << "TopologyResolver::resolve_boundary() plate.d_vertex_list.size() " 
	<< plate.d_vertex_list.size() << std::endl;
#endif


	if (plate.d_vertex_list.size() == 0 ) {
		return;
	}

	// Create an RFG for the polygon 

	// create a polygon on sphere
	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type reconstructed_geom = 
		GPlatesMaths::PolygonOnSphere::create_on_heap( plate.d_vertex_list );
		
	// create an RFG
	GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
		GPlatesModel::ReconstructedFeatureGeometry::create(
			reconstructed_geom,
			*d_accumulator->d_current_property->collection_handle_ptr(),
			*d_accumulator->d_current_property,
			d_accumulator->d_recon_plate_id,
			d_accumulator->d_time_of_appearance);

	d_reconstruction_geometries_to_populate->push_back(rfg_ptr);
 	d_reconstruction_geometries_to_populate->back()->set_reconstruction_ptr(d_recon_ptr);


	//
	// RFG for the proximity rotated reference points 
	//
	if ( d_proximity_point_list.size() != 0) 
	{
#ifdef DEBUG_RESOLVE_BOUNDARY
std::cout << "TopologyResolver::resolve_boundary(): d_proximity_point_list.size()=" << d_proximity_point_list.size() << std::endl;
#endif

	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type ref_points = 
			GPlatesMaths::MultiPointOnSphere::create_on_heap( d_proximity_point_list );

	// create an RFG
	GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type b_rfg_ptr =
		GPlatesModel::ReconstructedFeatureGeometry::create(
			ref_points,
			*d_accumulator->d_current_property->collection_handle_ptr(),
			*d_accumulator->d_current_property);

		d_reconstruction_geometries_to_populate->push_back(b_rfg_ptr);
 		d_reconstruction_geometries_to_populate->back()->set_reconstruction_ptr(d_recon_ptr);
	}

	// compute_bounds 
	compute_bounds( plate );
}



// Traverse the boundary feature list and return the list of vertices
// found from processing each node and it's relation to previous and next neighbors
std::list<GPlatesMaths::PointOnSphere>
GPlatesFeatureVisitors::TopologyResolver::get_vertex_list(
	BoundaryFeatureList_type::iterator pos1,
	BoundaryFeatureList_type::iterator pos2)
{
	// Clear working variable
	std::list<GPlatesMaths::PointOnSphere> work_vertex_list;
	work_vertex_list.clear();

#if 0 // TYPE
	// Clear subduction boundary component lists
	m_subduction_list.clear();
	m_subduction_sR_list.clear();
	m_subduction_sL_list.clear();
	m_ridge_transform_list.clear();
#endif

#ifdef DEBUG_GET_VERTEX_LIST
std::cout << "TopologyResolver::g_v_l: d_boundary_list.size()=" 
<< d_boundary_list.size() << std::endl;
#endif

	// 
	// super short cut for empty list:
	//
	if ( d_boundary_list.empty() )
	{
		return work_vertex_list;
	}

	//
	// super short cut for single feature on list
	//
	if ( d_boundary_list.size() == 1 )
	{
		BoundaryFeature node = d_boundary_list.front();

		GPlatesModel::FeatureId fid = node.m_feature_id;
		
#ifdef DEBUG_GET_VERTEX_LIST
qDebug() << "TopologyResolver::g_v_l: " << "d_boundary_list.size()==1; ";
qDebug() << "TopologyResolver::g_v_l: " << "fid=" 
	<< GPlatesUtils::make_qstring_from_icu_string(fid.get());
#endif

		if (node.m_feature_type == GPlatesGlobal::POINT_FEATURE)
		{
			// only one boundary feature and it is a point
			// find vertex of rotated point in layout
			// put direcly into work list
			get_vertex_list_from_feature_id( 
				work_vertex_list, 
				fid );

			// no boundary feature list neighbor processing needed
		}
		else if (node.m_feature_type == GPlatesGlobal::LINE_FEATURE)
		{
			// only one boundary feature and it is a line
			// find vertex list from rotated polyline in layout
			// put direcly into work list
			get_vertex_list_from_feature_id( 
				work_vertex_list, 
				fid );

			// no boundary feature list neighbor processing needed
		}
		else
		{
			// FIX : boundary features must be POINT_FEATURE or LINE only
			// FIX : for now, send back an empty list 
			work_vertex_list.clear();
		}

		return work_vertex_list;
	}

	//
	// else the list size is > 1
	//

	// Iterators to the boundary feature list
	BoundaryFeatureList_type::iterator prev;
	BoundaryFeatureList_type::iterator iter; 
	BoundaryFeatureList_type::iterator next;

	//
	// iterate over the boundary list from pos1 to --pos2
	//
	for (iter = pos1; iter != pos2; ++iter)
	{
#ifdef DEBUG_GET_VERTEX_LIST
std::cout << "TopologyResolver::g_v_l() step 1: iterator math" << std::endl;
#endif
		// 
		// Step 1: Iterator math 
		//

		// iterator math for prev
		if (iter == pos1) 
		{
			prev = pos2;
			--prev;
		}
		else
		{
			prev = iter;
			--prev;
		}

		// iterator math for next 
		next = iter;
		++next;
		if ( next == pos2 )
		{
			next = pos1;
		}

		//
		// Step 2: get feature ids for iters
		//
		GPlatesModel::FeatureId prev_fid = prev->m_feature_id;
		GPlatesModel::FeatureId iter_fid = iter->m_feature_id;
		GPlatesModel::FeatureId next_fid = next->m_feature_id;

#ifdef DEBUG_GET_VERTEX_LIST
qDebug() << "TopologyResolver::g_v_l step 2: PREV_fid=" 
	<< GPlatesUtils::make_qstring_from_icu_string( prev_fid.get());
qDebug() << "TopologyResolver::g_v_l step 2: ITER_fid=" 
	<< GPlatesUtils::make_qstring_from_icu_string( iter_fid.get());
qDebug() << "TopologyResolver::g_v_l step 2: NEXT_fid=" 
	<< GPlatesUtils::make_qstring_from_icu_string( next_fid.get());
#endif

		// short cut for iter type == POINT_FEATURE
		if ( iter->m_feature_type == GPlatesGlobal::POINT_FEATURE )
		{
			// find verts for iter
			// put directly into work_vertex_list
			get_vertex_list_from_feature_id( 
				work_vertex_list, 
				iter_fid );

			continue; // to next iter to boundary feature in list
		}

		//
		// Double check that iter is a LINE
		//
		if ( iter->m_feature_type != GPlatesGlobal::LINE_FEATURE )
		{
			// FIXME: post error ?
			continue; // to next iter 
		}

		//
		// Step 3 : Get iter vertex list from feature in the Layout
		//
		std::list<GPlatesMaths::PointOnSphere> iter_vertex_list;

		get_vertex_list_from_feature_id(
			iter_vertex_list,
			iter_fid );

#ifdef DEBUG_GET_VERTEX_LIST
std::cout << "TopologyResolver::g_v_l step 3 gvl_fid: ";
std::cout << "iter_vertex_list.size()=" << iter_vertex_list.size() << "; ";
std::cout << std::endl;
#endif


		//
		// Step 4: Process test vertex list with neighbor relations
		//

		//
		// test with NEXT ; modify iter_vertex_list as needed
		//
		int num_next = get_vertex_list_from_node_relation(
			GPlatesFeatureVisitors::TopologyResolver::INTERSECT_NEXT,
			*iter,
			*next,
			iter_vertex_list);

		if (num_next > 1) 
		{
			qDebug() << "WARNING:";
			qDebug() << "WARNING:";
			qDebug() << "TopologyResolver::g_v_l step 4: " << "num_NEXT=" << num_next;
			qDebug() << "TopologyResolver::g_v_l step 4: " 
				<< "PREV_fid=" << GPlatesUtils::make_qstring_from_icu_string( prev_fid.get());
			qDebug() << "TopologyResolver::g_v_l step 4: " 
				<< "ITER_fid=" << GPlatesUtils::make_qstring_from_icu_string( iter_fid.get());
			qDebug() << "TopologyResolver::g_v_l step 4: " 
				<< "NEXT_fid=" << GPlatesUtils::make_qstring_from_icu_string( next_fid.get());
		}

#ifdef DEBUG_GET_VERTEX_LIST
std::cout << "TopologyResolver::g_v_l step 4 post-NEXT: "
<< "iter_vertex_list.size()=" << iter_vertex_list.size() << "; "
<< std::endl;
#endif

		//
		// test with PREV ; modify iter_vertex_list as needed
		//
		int num_prev = get_vertex_list_from_node_relation(
			GPlatesFeatureVisitors::TopologyResolver::INTERSECT_PREV,
			*iter,
			*prev,
			iter_vertex_list);

		if (num_prev > 1) 
		{
			qDebug() << "WARNING:";
			qDebug() << "WARNING:";
			qDebug() << "TopologyResolver::g_v_l step 4: " << "num_PREV=" << num_prev;
			qDebug() << "TopologyResolver::g_v_l step 4: " 
				<< "PREV_fid=" << GPlatesUtils::make_qstring_from_icu_string( prev_fid.get());
			qDebug() << "TopologyResolver::g_v_l step 4: " 
				<< "ITER_fid=" << GPlatesUtils::make_qstring_from_icu_string( iter_fid.get());
			qDebug() << "TopologyResolver::g_v_l step 4: " 
				<< "NEXT_fid=" << GPlatesUtils::make_qstring_from_icu_string( next_fid.get());
		}

#ifdef DEBUG_GET_VERTEX_LIST
std::cout << "TopologyResolver::g_v_l step 4 post-PREV: "
<< "iter_vertex_list.size()=" << iter_vertex_list.size() << "; "
<< std::endl;
#endif

		// 
		// Step 5: test for reverse flag on node 
		//
		// check for reverse flag of boundary feature node 
		// NOT on original feature
		//
		if ( iter->m_use_reverse )
		{
#ifdef DEBUG_GET_VERTEX_LIST
std::cout << "TopologyResolver::g_v_l step 5 rev?: " 
<< "iter->m_use_reverse=true" << std::endl;
#endif
			iter_vertex_list.reverse();
		}


		//
		// Step 6: copy final processed vertex list to working list
		//
		copy(
			iter_vertex_list.begin(),
			iter_vertex_list.end(),
			back_inserter( work_vertex_list )
		);

#if 0 // TYPE 
		//
		// Step 6.1: Splice out specific boundary types:
		// copy final processed vertex list to sub_boundary 
		//
		std::string type_code = iter->m_feature->GetTypeCode();

		if ( ( type_code == "sR" ) || ( type_code == "sL" ) ) 
		{
			std::string use_reverse = "no";

			// Adjust the type code to match the orientation 
			// of how the vertices are used in the polygon boundary
			if ( iter->m_use_reverse )
			{
				use_reverse = "yes"; 

				if (type_code == "sR") 
				{ 
					type_code = "sL";  
//std::cout << "TopologyResolver::g_v_l step 6: sR -> sL" << std::endl; 
				}
				else if (type_code == "sL") 
				{ 
					type_code = "sR";  
//std::cout << "TopologyResolver::g_v_l step 6: sL -> sR" << std::endl; 
				}
			}

			std::string feature_tag = 
				">" + type_code + 
				"    # name: " + iter->m_feature->GetName() +
				"    # polygon: " + GetName() +
				"    # use_reverse: " + use_reverse +
				"\n";

// std::cout << "TopologyResolver::g_v_l step 6: tag=" << feature_tag;


			// Get the vertices from this segment
			std::list<GPlatesMaths::PointOnSphere> tmp_vertex_list;
			tmp_vertex_list.clear();

			copy(
				iter_vertex_list.begin(),
				iter_vertex_list.end(),
				back_inserter( tmp_vertex_list )
			);

			// Append this subduction boundary to the list
			m_subduction_list.push_back( 
				SubductionBoundaryFeature(
					GPlatesUtils::get_old_id( iter->m_feature),
					feature_tag,
					tmp_vertex_list)
			);

			if (type_code == "sL") 
			{
//std::cout << "TopologyResolver::g_v_l step 6: sL_list.push_back: "
//<< "tmp_vertex_list.size()=" << tmp_vertex_list.size() << "; "
//<< std::endl;
				m_subduction_sL_list.push_back( 
					SubductionBoundaryFeature(
						GPlatesUtils::get_old_id( iter->m_feature),
						feature_tag,
						tmp_vertex_list)
				);
			}

			if (type_code == "sR") 
			{
//std::cout << "TopologyResolver::g_v_l step 6: sR_list.push_back: "
//<< "tmp_vertex_list.size()=" << tmp_vertex_list.size() << "; "
//<< std::endl;
				m_subduction_sR_list.push_back( 
					SubductionBoundaryFeature(
						GPlatesUtils::get_old_id( iter->m_feature),
						feature_tag,
						tmp_vertex_list)
				);
			}
		}
		else
		{
			std::string use_reverse = "no";
			if ( iter->m_use_reverse ) { use_reverse = "yes"; }

			std::string feature_tag = 
				">" + type_code + 
				"    # name: " + iter->m_feature->GetName() +
				"    # polygon: " + GetName() +
				"    # use_reverse: " + use_reverse +
				"\n";
			// Get the vertices from this segment
			std::list<GPlatesMaths::PointOnSphere> tmp_vertex_list;
			tmp_vertex_list.clear();

			copy(
				iter_vertex_list.begin(),
				iter_vertex_list.end(),
				back_inserter( tmp_vertex_list )
			);

// std::cout << "TopologyResolver::g_v_l step 6: tag=" << feature_tag;
// std::cout << "TopologyResolver::g_v_l step 6: r_t_list.push_back: "
// << "tmp_vertex_list.size()=" << tmp_vertex_list.size() << "; "
// << std::endl;
			// Append this boundary to the list
			m_ridge_transform_list.push_back( 
				SubductionBoundaryFeature(
					GPlatesUtils::get_old_id( iter->m_feature),
					feature_tag,
					tmp_vertex_list)
			);
		}

#endif // TYPE

#ifdef DEBUG_GET_VERTEX_LIST
std::cout << "TopologyResolver::g_v_l step 6 copy: "
<< "iter_vertex_list.size()=" << iter_vertex_list.size() << "; "
<< std::endl;
#endif

	} // End of interation over boundary feature list

	//
	// Step 7: return working vertex list
	//
#ifdef DEBUG_GET_VERTEX_LIST
std::cout << "TopologyResolver::g_v_l step 7: "
<< "work_vertex_list.size()=" << work_vertex_list.size() << "; "
<< std::endl;
#endif

	return work_vertex_list;
}





//
// get_vertex_list_from_node_relation
//
int
GPlatesFeatureVisitors::TopologyResolver::get_vertex_list_from_node_relation( 
	GPlatesFeatureVisitors::TopologyResolver::NeighborRelation relation,
	BoundaryFeature &node1,
	BoundaryFeature &node2,
	std::list<GPlatesMaths::PointOnSphere> &vertex_list)
{

	// double check on empty vertex_list
	if ( vertex_list.empty() )
	{
		// no change to node1 or vertex_list
		return 0;
	}

	GPlatesModel::FeatureId node1_fid = node1.m_feature_id;
	GPlatesModel::FeatureId node2_fid = node2.m_feature_id;

// FIX : remove this diagnostic output
#ifdef DEBUG_GET_VERTEX_LIST
std::cout << std::endl;
std::cout << "TopologyResolver::g_v_l_f_n_r: " << "relation=" << relation << std::endl;
qDebug() << "TopologyResolver::g_v_l_f_n_r: " << "node1_fid=" 
<< GPlatesUtils::make_qstring_from_icu_string(node1_fid.get());
qDebug() << "TopologyResolver::g_v_l_f_n_r: " << "node2_fid=" 
<< GPlatesUtils::make_qstring_from_icu_string(node2_fid.get());
std::cout << std::endl;
#endif

	// short cut for node2 is a POINT_FEATURE
	if ( node2.m_feature_type == GPlatesGlobal::POINT_FEATURE)
	{
		// no change to node1 or vertex_list
		return 0;
	}

	//
	// node2 must be a GPlatesGlobal::LINE_FEATURE , so test for intersection
	//
	
	//
	// tmp variables to hold results of processing node1 vs node2
	//
	std::list<GPlatesMaths::PointOnSphere> node1_vertex_list;
	std::list<GPlatesMaths::PointOnSphere> node2_vertex_list;

	//
	// short cut for empty node2
	//

	// get verts for node2 from Layout 
	get_vertex_list_from_feature_id(
		node2_vertex_list,
		node2_fid );

	//
	// skip features not found, or missing from Layout
	//
	if ( node2_vertex_list.empty() )
	{
		// no change to node1 or vertex_list
		return 0;
	}

	//
	// create polylines for each boundary feature node
	//

	// use the argument vertex_list
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type node1_polyline = 
		GPlatesMaths::PolylineOnSphere::create_on_heap( vertex_list );

#ifdef DEBUG_GET_VERTEX_LIST
const GPlatesMaths::PolylineOnSphere& pl1 = *node1_polyline;
GPlatesMaths::PolylineOnSphere::VertexConstIterator itr1 = pl1.vertex_begin();
GPlatesMaths::PolylineOnSphere::VertexConstIterator end1 = pl1.vertex_end();
for ( ; itr1 != end1 ; ++itr1) {
std::cout << "TEST: llp( *itr1 ) = " << GPlatesMaths::make_lat_lon_point(*itr1) << std::endl;
}
std::cout << std::endl;
#endif
	

	// use the feature's vertex list
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type node2_polyline = 
		GPlatesMaths::PolylineOnSphere::create_on_heap( node2_vertex_list );

#ifdef DEBUG_GET_VERTEX_LIST
const GPlatesMaths::PolylineOnSphere& pl2 = *node2_polyline;
GPlatesMaths::PolylineOnSphere::VertexConstIterator itr2 = pl2.vertex_begin();
GPlatesMaths::PolylineOnSphere::VertexConstIterator end2 = pl2.vertex_end();
for ( ; itr2 != end2 ; ++itr2) {
std::cout << "TEST: llp( *itr2 ) = " << GPlatesMaths::make_lat_lon_point(*itr2) << std::endl;
}
std::cout << std::endl;
#endif
	

	//
	// variables to save results of intersection tests
	//
	std::list< GPlatesMaths::PointOnSphere > intersection_points;
	std::list< GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type > partitioned_lines;

	//
	// test for intersection between node1 and node2
	//
	int num_intersect = 0;

	num_intersect = GPlatesMaths::PolylineIntersections::partition_intersecting_polylines(
		*node1_polyline,
		*node2_polyline,
		intersection_points,
		partitioned_lines);

#ifdef DEBUG_GET_VERTEX_LIST
std::cout << "TopologyResolver::g_v_l_f_n_r: "
<< "num_intersect = " << num_intersect << std::endl;
#endif


	//
	// switch on relation enum to update node1's member data
	//
	switch ( relation )
	{
		case GPlatesFeatureVisitors::TopologyResolver::INTERSECT_PREV:
			node1.m_num_intersections_with_prev = num_intersect;
			break;

		case GPlatesFeatureVisitors::TopologyResolver::INTERSECT_NEXT:
			node1.m_num_intersections_with_next = num_intersect;
			break;

		case GPlatesFeatureVisitors::TopologyResolver::NONE :
		case GPlatesFeatureVisitors::TopologyResolver::OTHER :
		default :
			// somthing bad happened , freak out
			break;
	}


	// 

	if ( num_intersect == 0 )
	{
		// no further change to node1 or vertex_list
		// clean up
		//delete node1_polyline.get();
		//delete node2_polyline.get();
		return 0;
	}
	else if ( num_intersect == 1 )
	{
		// pair of polyline lists from intersection
		std::pair<
			std::list< GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type >,
			std::list< GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type >
		> parts;

		// unambiguously identify partitioned lines:
		//
		// parts.first.front() is a ref to the head of node1_polyline
		// parts.first.back() is a ref to the tail of node1_polyline
		// parts.second.front() is a ref to the head of node2_polyline
		// parts.second.back() is a ref to the tail of node2_polyline
		//
		parts = GPlatesMaths::PolylineIntersections::identify_partitioned_polylines(
			*node1_polyline,
			*node2_polyline,
			intersection_points,
			partitioned_lines);

		// clean up
		// delete node1_polyline.get();
		// delete node2_polyline.get();

		// now check which element of parts.first
		// to use based upon node1's neighbor relations
		//
		// parts.first holds the sub-parts of node1's polyline

		//
		// switch on the relation to check
		//
		switch ( relation )
		{
			case GPlatesFeatureVisitors::TopologyResolver::INTERSECT_PREV :

				if ( node1.m_use_head_from_intersect_prev )
				{
					vertex_list.clear();
					copy( 
						parts.first.front()->vertex_begin(),
						parts.first.front()->vertex_end(),
						back_inserter( vertex_list )
					);
					return 1;
				}
				
				if ( node1.m_use_tail_from_intersect_prev )
				{
					vertex_list.clear();
					copy( 
						parts.first.back()->vertex_begin(),
						parts.first.back()->vertex_end(),
						back_inserter( vertex_list )
					);
					return 1;
				}

				break;

			case GPlatesFeatureVisitors::TopologyResolver::INTERSECT_NEXT:

				if ( node1.m_use_head_from_intersect_next )
				{
					vertex_list.clear();
					copy( 
						parts.first.front()->vertex_begin(),
						parts.first.front()->vertex_end(),
						back_inserter( vertex_list )
					);
					return 1;
				}
				
				if ( node1.m_use_tail_from_intersect_next )
				{
					vertex_list.clear();
					copy( 
						parts.first.back()->vertex_begin(),
						parts.first.back()->vertex_end(),
						back_inserter( vertex_list )
					);
					return 1;
				}

				break;

			default:
				break;
		}
	} 
	// end of else if ( num_intersect == 1 )
	else 
	{
		// num_intersect must be 2 or greater oh no!_GET_VERTEX_LIST
		std::cerr << "TopologyResolver::g_v_l_f_n_r: "
		<< "WARN: num_intersect=" << num_intersect
		<< std::endl;
		return num_intersect;
	}
	return 0;
}


void
GPlatesFeatureVisitors::TopologyResolver::get_vertex_list_from_feature_id(
	std::list<GPlatesMaths::PointOnSphere> &vertex_list,
	GPlatesModel::FeatureId feature_id)
{
#ifdef DEBUG_GET_VERTEX_LIST
std::cout << "TopologyResolver::get_vertex_list_from_feature_id:"  << std::endl;
#endif

	// Get a vector of FeatureHandle weak_refs for this FeatureId
	std::vector<GPlatesModel::FeatureHandle::weak_ref> back_refs;
	feature_id.find_back_ref_targets( append_as_weak_refs( back_refs ) );

	// Double check back_refs
	if ( back_refs.size() == 0 )
	{
		// FIXME: feak out? 
		qDebug() << "ERROR: get_vertex_list_from_feature_id():";
		qDebug() << "ERROR: No feature found for feature_id =";
		qDebug() <<
			GPlatesUtils::make_qstring_from_icu_string( feature_id.get() );
		qDebug() << "ERROR: Unable to obtain feature (and its geometry, or vertices)";
		qDebug() << " ";
		// FIXME: what else to do?
		// no change to vertex_list
		return;
	}

	if ( back_refs.size() > 1)
	{
		qDebug() << "ERROR: TopologyResolver::get_vertex_list_from_feature_id():";
		qDebug() << "ERROR: More than one feature found for feature_id =";
		qDebug() <<
			GPlatesUtils::make_qstring_from_icu_string( feature_id.get() );
		qDebug() << "ERROR: Unable to determine feature";
		// FIXME: what else to do?
		// no change to vertex_list
		return;
	}

	// else , get the first ref on the list
	GPlatesModel::FeatureHandle::weak_ref feature_ref = back_refs.front();

	// find the RFGs for this feature ref
	GPlatesModel::ReconstructedFeatureGeometryFinder finder( d_recon_ptr ); 
	finder.find_rfgs_of_feature( feature_ref );

	// Get a list of RFGs 
	GPlatesModel::ReconstructedFeatureGeometryFinder::rfg_container_type::const_iterator 
		find_iter = finder.found_rfgs_begin();

	// Double check RFGs
	if ( find_iter != finder.found_rfgs_end() )
	{
		// Get the geometry on sphere from the RFG
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type gos_ptr = 
			(*find_iter)->geometry();	
	
		if (gos_ptr) 
		{
			// visit the geom on sphere ( calls functions defined in TopologyResolver.h )
			// fill the d_rfg_vertex_list
			gos_ptr->accept_visitor(*this);

			// Copy the vertices from the d_rfg_vertex_list to the argument list
			copy(
				d_rfg_vertex_list.begin(),
				d_rfg_vertex_list.end(),
				back_inserter( vertex_list )
			);
		} 
		// else FIXME: do we need to report this ?
	}
	// else FIXME: do we need to report this?

#ifdef DEBUG_GET_VERTEX_LIST
std::cout << "TopologyResolver::get_vertex_list_from_feature_id: vertex_list.size() =" 
<< vertex_list.size() << std::endl;
#endif
}



void
GPlatesFeatureVisitors::TopologyResolver::compute_bounds( PlatePolygon &plate )
{
	// tmp vars
	double dlon = 0;
	double lon_sum = 0.0;

	// re-set initial default values to opposite extreame value
	plate.d_max_lat = -91;
	plate.d_min_lat = 91;
	plate.d_max_lon = -181;
	plate.d_min_lon = 181;

	// re-set polar value to default : 
	// 0 = no pole contained in polygon
	plate.d_pole = 0;

	// loop over rotated vertices
	std::list<GPlatesMaths::PointOnSphere>::const_iterator iter;
	std::list<GPlatesMaths::PointOnSphere>::const_iterator next;

	for ( iter = plate.d_vertex_list.begin(); iter != plate.d_vertex_list.end(); ++iter )
	{
		// get coords for iter vertex
		GPlatesMaths::LatLonPoint v1 = GPlatesMaths::make_lat_lon_point(*iter);

		// coords for next vertex in list
		next = iter;
		++next;
		if ( next == plate.d_vertex_list.end() ) { next = plate.d_vertex_list.begin(); }

		// coords for next vextex
		GPlatesMaths::LatLonPoint v2 = GPlatesMaths::make_lat_lon_point(*next);

		double v1lat = v1.latitude();
		double v1lon = v1.longitude();

		// double v2lat = v2.latitude(); // not used
		double v2lon = v2.longitude();

		plate.d_min_lon = std::min( v1lon, plate.d_min_lon );
		plate.d_max_lon = std::max( v1lon, plate.d_max_lon );

		plate.d_min_lat = std::min( v1lat, plate.d_min_lat );
		plate.d_max_lat = std::max( v1lat, plate.d_max_lat );

		dlon = v1lon - v2lon;

		if (fabs (dlon) > 180.0) 
		{
			dlon = copysign (360.0 - fabs (dlon), -dlon);
		}

		lon_sum += dlon;

	}

#ifdef DEBUG_BOUNDS
std::cout << "TopologyResolver::compute_bounds: max_lat = " << plate.d_max_lat << std::endl;
std::cout << "TopologyResolver::compute_bounds: min_lat = " << plate.d_min_lat << std::endl;
std::cout << "TopologyResolver::compute_bounds: max_lon = " << plate.d_max_lon << std::endl;
std::cout << "TopologyResolver::compute_bounds: min_lon = " << plate.d_min_lon << std::endl;
std::cout << "TopologyResolver::compute_bounds: lon_sum = " << lon_sum << std::endl;
#endif

	//
	// dermine if the platepolygon contains the pole
	//
	if ( fabs(fabs(lon_sum) - 360.0) < 1.0e-8 )
	{
		if ( fabs(plate.d_max_lat) > fabs(plate.d_min_lat) ) 
		{
			plate.d_pole = static_cast<int>(copysign(1.0, plate.d_max_lat));
		}
		else 
		{
			plate.d_pole = static_cast<int>(copysign(1.0, plate.d_min_lat));
		}
	}
#ifdef DEBUG_BOUNDS
std::cout << "TopologyResolver::compute_bounds: plate.d_pole = " << plate.d_pole << std::endl;
#endif
}





/* This function is used to see if some point p is located 
 * inside, outside, or on the boundary of the plate polygon
 * Returns the following values:
 *	0:	p is outside of S
 *	1:	p is inside of S
 *	2:	p is on boundary of S
 */
int 
GPlatesFeatureVisitors::TopologyResolver::is_point_in_on_out ( 
	const GPlatesMaths::PointOnSphere &test_point,
	PlatePolygon &plate) const
{
	/* Algorithm:
	 *
	 * Case 1: The polygon S contains a geographical pole
	 *	   a) if P is beyond the far latitude then P is outside
	 *	   b) Compute meridian through P and count intersections:
	 *		odd: P is outside; even: P is inside
	 *
	 * Case 2: S does not contain a pole
	 *	   a) If P is outside range of latitudes then P is outside
	 *	   c) Compute meridian through P and count intersections:
	 *		odd: P is inside; even: P is outside
	 *
	 * In all cases, we check if P is on the outline of S
	 *
	 */
	
	// counters for the number of crossings of a meridian 
	// through p and and the segments of this polygon
	int count_north = 0;
	int count_south = 0;


	// coords of test point p
	GPlatesMaths::LatLonPoint p = GPlatesMaths::make_lat_lon_point(test_point);

	// test point's plat
	double plat = p.latitude();
		
#ifdef DEBUG_POINT_IN_POLYGON
std::cout << "TopologyResolver::is_point_in_on_out: " << std::endl;
std::cout << "llp" << p << std::endl;
std::cout << "pos" << test_point << std::endl;
#endif

	if (plate.d_pole) 
	{	
#ifdef DEBUG_POINT_IN_POLYGON
std::cout << "TopologyResolver::is_point_in_on_out: plate contains N or S pole." << std::endl;
#endif
		/* Case 1 of an enclosed polar cap */

		/* N polar cap */
		if (plate.d_pole == 1) 
		{	
			/* South of a N polar cap */
			if (plat < plate.d_min_lat) return (POINT_OUTSIDE_POLYGON);	

			/* Clearly inside of a N polar cap */
			if (plat > plate.d_max_lat) return (POINT_INSIDE_POLYGON);	
		}

		/* S polar cap */
		if (plate.d_pole == -1) 
		{	
			/* North of a S polar cap */
			if (plat > plate.d_max_lat) return (POINT_OUTSIDE_POLYGON);

			/* North of a S polar cap */
			if (plat < plate.d_min_lat) return (POINT_INSIDE_POLYGON);	
		}
	
		// Tally up number of intersections between polygon 
		// and meridian through p 
		
		if ( is_point_in_on_out_counter(test_point, plate, count_north, count_south) ) 
		{
			/* Found P is on S */
			return (POINT_ON_POLYGON);	
		}
	
		if (plate.d_pole == 1 && count_north % 2 == 0) 
		{
			return (POINT_INSIDE_POLYGON);
		}

		if (plate.d_pole == -1 && count_south % 2 == 0) 
		{
			return (POINT_INSIDE_POLYGON);
		}
	
		return (POINT_OUTSIDE_POLYGON);
	}
	
	/* Here is Case 2.  */

	// First check latitude range 
	if (plat < plate.d_min_lat || plat > plate.d_max_lat) 
	{
		return (POINT_OUTSIDE_POLYGON);
	}
	
	// Longitudes are tricker and are tested with the 
	// tallying of intersections 
	
	if ( is_point_in_on_out_counter( test_point, plate, count_north, count_south ) ) 
	{
		/* Found P is on S */
		return (POINT_ON_POLYGON);	
	}

	if (count_north % 2) return (POINT_INSIDE_POLYGON);
	
	/* Nothing triggered the tests; we are outside */
	return (POINT_OUTSIDE_POLYGON);	
}


int 
GPlatesFeatureVisitors::TopologyResolver::is_point_in_on_out_counter ( 
	const GPlatesMaths::PointOnSphere &test_point,
	PlatePolygon &plate,
	int &count_north,
	int &count_south) 
const
{
#ifdef DEBUG_POINT_IN_POLYGON
std::cout << "TopologyResolver::is_point_in_on_out_counter: " << std::endl;
#endif

	// local tmp vars
	// using GPlatesMaths::Real lets us use operator== 
	GPlatesMaths::Real W, E, S, N, x_lat;
	GPlatesMaths::Real dlon;
	GPlatesMaths::Real lon, lon1, lon2;

	// coords of test test_point p
	GPlatesMaths::LatLonPoint p = GPlatesMaths::make_lat_lon_point(test_point);

	// test_point's coords are plon, plat
	double plon = p.longitude();
	double plat = p.latitude();

	// re-set number of crossings
	count_south = 0;
	count_north = 0;

	// Compute meridian through P and count all the crossings with 
	// segments of polygon boundary 

	// loop over vertices
	// form segments for each vertex pair
	std::list<GPlatesMaths::PointOnSphere>::const_iterator iter;
	std::list<GPlatesMaths::PointOnSphere>::const_iterator next;

	// loop over rotated vertices using access iterators
	for ( iter = plate.d_vertex_list.begin(); iter != plate.d_vertex_list.end(); ++iter )
	{
		// get coords for iter vertex
		GPlatesMaths::LatLonPoint v1 = GPlatesMaths::make_lat_lon_point(*iter);

		// identifiy next vertex 
		next = iter; 
		++next;
		if ( next == plate.d_vertex_list.end() ) { next = plate.d_vertex_list.begin(); }

		// coords for next vextex
		GPlatesMaths::LatLonPoint v2 = GPlatesMaths::make_lat_lon_point(*next);

		double v1lat = v1.latitude();
		double v1lon = v1.longitude();

		double v2lat = v2.latitude();
		double v2lon = v2.longitude();

#ifdef DEBUG_POINT_IN_POLYGON
std::cout << "is_point_in_on_out_counter: v1 = " << v1lat << ", " << v1lon << std::endl;
std::cout << "is_point_in_on_out_counter: v2 = " << v2lat << ", " << v2lon << std::endl; 
#endif
		// Copy the two vertex longitudes 
		// since we need to mess with them
		lon1 = v1lon;
		lon2 = v2lon;

		// delta in lon
		dlon = lon2 - lon1;

		if ( dlon > 180.0 )
		{
			// Jumped across Greenwhich going westward
			lon2 -= 360.0;
		} 
		else if ( dlon < -180.0 )
		{
			// Jumped across Greenwhich going eastward
			lon1 -= 360.0;
		}

		// set lon limits for this segment
		if (lon1 <= lon2) 
		{	
			/* segment goes W to E (or N-S) */
			W = lon1;
			E = lon2;
		}
		else 
		{			
			/* segment goes E to W */
			W = lon2;
			E = lon1;
		}
		
		/* Local copy of plon, adjusted given the segment lon range */
		lon = plon;	

		/* Make sure we rewind way west for starters */
		while (lon > W) lon -= 360.0;	

		/* Then make sure we wind to inside the lon range or way east */
		while (lon < W) lon += 360.0;	

		/* Not crossing this segment */
		if (lon > E) continue; // to next vertex

		/* Special case of N-S segment: does P lie on it? */
		if (dlon == 0.0) 
		{	
			if ( v2lat < v1lat ) 
			{	
				/* Get N and S limits for segment */
				S = v2lat;
				N = v1lat;
			}
			else 
			{
				N = v2lat;
				S = v1lat;
			}

			/* P is not on this segment */
			if ( plat < S || plat > N ) continue; // to next vertex

			/* P is on segment boundary; we are done */
			return (1);	
		}

		/* Calculate latitude at intersection */
		x_lat = v1lat + (( v2lat - v1lat) / (lon2 - lon1)) * (lon - lon1);

		/* P is on S boundary */
		if ( x_lat == plat ) return (1);	

		// Only allow cutting a vertex at end of a segment 
		// to avoid duplicates 
		if (lon == lon1) continue;	

		if (x_lat > plat)	
		{
			/* Cut is north of P */
			++count_north;
		}
		else
		{
			/* Cut is south of P */
			++count_south;
		}

	} // end of loop over vertices

	return (0);	
}

std::vector<GPlatesModel::FeatureId>
GPlatesFeatureVisitors::TopologyResolver::locate_point(
	const GPlatesMaths::PointOnSphere &point )
{
	std::vector<GPlatesModel::FeatureId> found_ids;
	
	// loop over the map of plates, as represented by pair<FeatureId,PlatePolygon>
	fid_polygon_pair_list_iterator iter = d_fid_polygon_pair_list.begin();
	fid_polygon_pair_list_iterator end = d_fid_polygon_pair_list.end();
	for ( ; iter != end ; ++iter )
	{
		GPlatesModel::FeatureId fid = iter->first;
		PlatePolygon plate_polygon = iter->second;

		// Get a vector of FeatureHandle weak_refs for this FeatureId
		std::vector<GPlatesModel::FeatureHandle::weak_ref> back_refs;
		( iter->first ).find_back_ref_targets( append_as_weak_refs( back_refs ) );

		// Double check refs
		if ( back_refs.size() == 0 )
		{
			// FIXME: Error reporting?
			qDebug() << "ERROR: locate_point():";
			qDebug() << "ERROR: No feature found for feature_id =";
			qDebug() << "ERROR:" << GPlatesUtils::make_qstring_from_icu_string( fid.get() );
			qDebug() << "ERROR: Unable test this feature for point location";
			qDebug() << " ";
			// return empty vector
			return found_ids;
		}

		if ( back_refs.size() != 1 )
		{
			// FIXME: Error reporting?
			qDebug() << "ERROR: locate_point():";
			qDebug() << "ERROR: No feature found for feature_id =";
			qDebug() << "ERROR:" << GPlatesUtils::make_qstring_from_icu_string( fid.get() );
			qDebug() << "ERROR: Unable test this feature for point location";
			qDebug() << " ";
			// return empty vector
			return found_ids;
		}

		// else, get the first ref on the vector
		GPlatesModel::FeatureHandle::weak_ref feature_ref = back_refs.front();

#ifdef DEBUG_POINT_IN_POLYGON
		// Get reconstructionPlateId property value
		static const GPlatesModel::PropertyName property_name =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		const GPlatesPropertyValues::GpmlPlateId *recon_plate_id;

		if ( GPlatesFeatureVisitors::get_property_value(
			*feature_ref, property_name, recon_plate_id ) )
		{
			std::cout 
				<< " TopologyResolver::locate_point(): " 
				<< " reconstructionPlateId = " << recon_plate_id->value() 
				<< " has " << plate_polygon.d_vertex_list.size() << " vertices." 
				<< std::endl;
		}
#endif

		// Apply the point in polygon test to the point: 
		/*
 		* 0: test_point is outside the plate
 		* 1: test_point is inside the plate
 		* 2: test_point is on boundary of the plate
 		*/
		int state = is_point_in_on_out( point, plate_polygon );
		if (state > 0) 
		{
			found_ids.push_back( fid );
		}
	}
	return found_ids;
}



void
GPlatesFeatureVisitors::TopologyResolver::report()
{
	std::cout << "-------------------------------------------------------------" << std::endl;
	std::cout << "TopologyResolver::report()" << std::endl;
	std::cout << "number features visited = " << d_num_features << std::endl;
	std::cout << "number topologies visited = " << d_num_topologies << std::endl;

	fid_polygon_pair_list_iterator iter = d_fid_polygon_pair_list.begin();
	fid_polygon_pair_list_iterator end = d_fid_polygon_pair_list.end();
	for ( ; iter != end ; ++iter )
	{
		GPlatesModel::FeatureId fid = iter->first;
		PlatePolygon plate_polygon = iter->second;

		// report on this topology
		std::cout << std::endl;
		std::cout << "feature_id = " 
			<< (GPlatesUtils::make_qstring_from_icu_string(fid.get()) ).toStdString() 
			<< std::endl;

		// Get a vector of FeatureHandle weak_refs for this FeatureId
		std::vector<GPlatesModel::FeatureHandle::weak_ref> back_refs;
		fid.find_back_ref_targets( append_as_weak_refs( back_refs ) );

		// Double check refs
		if ( back_refs.size() == 0 )
		{
			qDebug() << "ERROR: report():";
			qDebug() << "ERROR: No feature found for feature_id =";
			qDebug() << "ERROR:" << GPlatesUtils::make_qstring_from_icu_string( fid.get() );
			qDebug() << "ERROR: Unable to report on feature.";
			qDebug() << " ";
			// FIXME: what else to do?
			continue; // to next pair<FeatureId, PlatePolygon> on the list
		}

		if ( back_refs.size() != 1 )
		{
			// FIXME: Error reporting?
			qDebug() << "ERROR: report():";
			qDebug() << "ERROR: More than one feature found for feature_id =";
			qDebug() << "ERROR:" << GPlatesUtils::make_qstring_from_icu_string( fid.get() );
			qDebug() << "ERROR: Unable to report on feature.";
			qDebug() << " ";
			// FIXME: what else to do?
			continue; // to next pair<FeatureId, PlatePolygon> on the list
		}

		// else, get the first ref on the vector
		GPlatesModel::FeatureHandle::weak_ref feature_ref = back_refs.front();

		// Get the name property value
		static const GPlatesModel::PropertyName name_property_name =
			GPlatesModel::PropertyName::create_gml("name");
		const GPlatesPropertyValues::XsString *name;
		if ( GPlatesFeatureVisitors::get_property_value(
			*feature_ref, name_property_name, name ) ) 
		{
			std::cout << " name = \"" 
				<< GPlatesUtils::make_qstring(name->value()).toStdString() 
				<< "\"" 
				<< std::endl;
		}

		// Get the reconstructionPlateId property value
		static const GPlatesModel::PropertyName property_name =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
		const GPlatesPropertyValues::GpmlPlateId *recon_plate_id;
		if ( GPlatesFeatureVisitors::get_property_value(
			*feature_ref, property_name, recon_plate_id ) ) 
		{
			std::cout << " reconstructionPlateId = " << recon_plate_id->value() << std::endl;
		}

		// report on polygon stats
		std::cout 
			<< " # of vertices = " << plate_polygon.d_vertex_list.size() << std::endl
			<< " max_lat = " << plate_polygon.d_max_lat
			<< " max_lat = " << plate_polygon.d_max_lat
			<< " max_lon = " << plate_polygon.d_max_lon
			<< " max_lon = " << plate_polygon.d_max_lon
			<< "; encloses a pole? = " << ( plate_polygon.d_pole ? "yes":"no" ) 
			<< std::endl;
	}
	std::cout << "-------------------------------------------------------------" << std::endl;
}





