/* $Id$ */

/**
 * @file 
 * Contains the definitions of member functions of class ComputationalMeshSolver.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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


#include <fstream>
#include <sstream>
#include <iostream>

#include <QLocale>
#include <QDebug>
#include <boost/none.hpp>

#include "ComputationalMeshSolver.h"

#include "app-logic/GeometryCookieCutter.h"
#include "app-logic/PlateVelocityUtils.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/ResolvedTopologicalBoundary.h"
#include "app-logic/TopologyUtils.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "file-io/GpmlOnePointSixOutputVisitor.h"
#include "file-io/FileInfo.h"

#include "maths/CalculateVelocity.h"
#include "maths/CartesianConvMatrix3D.h"
#include "maths/LatLonPoint.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineIntersections.h"
#include "maths/PolylineOnSphere.h"
#include "maths/ProximityCriteria.h"

#include "model/FeatureHandle.h"
#include "model/FeatureHandleWeakRefBackInserter.h"
#include "model/ModelUtils.h"
#include "model/TopLevelPropertyInline.h"

#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFeatureReference.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlRevisionId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/GmlDataBlock.h"
#include "property-values/GmlDataBlockCoordinateList.h"

#include "utils/UnicodeStringUtils.h"

#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryParameters.h"


GPlatesFeatureVisitors::ComputationalMeshSolver::ComputationalMeshSolver(
		std::vector<GPlatesAppLogic::multi_point_vector_field_non_null_ptr_type> &velocity_fields_to_populate,
		const double &recon_time,
		const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type &recon_tree,
		const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type &recon_tree_2,
		const GPlatesAppLogic::GeometryCookieCutter &reconstructed_static_polygons_query,
		const GPlatesAppLogic::TopologyUtils::resolved_boundaries_for_geometry_partitioning_query_type &
				resolved_boundaries_for_partitioning_geometry_query,
		const GPlatesAppLogic::TopologyUtils::resolved_networks_for_interpolation_query_type &
				resolved_networks_for_velocity_interpolation,
		bool should_keep_features_without_recon_plate_id):
	d_velocity_fields_to_populate(velocity_fields_to_populate),
	d_recon_time(GPlatesPropertyValues::GeoTimeInstant(recon_time)),
	d_recon_tree_ptr(recon_tree),
	d_recon_tree_2_ptr(recon_tree_2),
	d_reconstructed_static_polygons_query(reconstructed_static_polygons_query),
	d_resolved_boundaries_for_partitioning_geometry_query(resolved_boundaries_for_partitioning_geometry_query),
	d_resolved_networks_for_velocity_interpolation(resolved_networks_for_velocity_interpolation),
	d_should_keep_features_without_recon_plate_id(should_keep_features_without_recon_plate_id),
	d_feature_handle_ptr(NULL)
{  
	d_num_features = 0;
	d_num_meshes = 0;
	d_num_points = 0;
}



void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_feature_handle(
		GPlatesModel::FeatureHandle &feature_handle)
{
	d_num_features += 1;

	// FIXME:  We should use property names rather than feature types to trigger behaviour.
	// What property name would serve this purpose?

	// Update: We now allow any feature type (not just "MeshNode"s) - if it contains multi-points
	// then it will have velocities calculated.
#if 0
	// The following statement is an O(L log N) map-insertion, where N is the number of feature
	// types currently loaded in GPlates and L is the average length of a feature-type string. 
	// Because the local variable is static, the statement will be executed at most once in the
	// run-time of the program.
	static const GPlatesModel::FeatureType mesh_node_feature_type =
			GPlatesModel::FeatureType::create_gpml("MeshNode");
	// The following is an O(1) iterator comparison.
	if (feature_handle.feature_type() != mesh_node_feature_type) {
		// Not a velocity field feature, so nothing to do here.
		return;
	}
#endif
	d_num_meshes += 1;

	// else process this feature:

	d_feature_handle_ptr = &feature_handle;

#if 0
	// this holds the name of the output file 
	std::ostringstream oss;

	oss << "velocity_colat+lon_at_";
	oss << d_recon_time.value();
	oss << "Ma_on_mesh-";

	// Get the name property value for this feature 
	static const GPlatesModel::PropertyName name_property_name =
		GPlatesModel::PropertyName::create_gml("name");

	const GPlatesPropertyValues::XsString *feature_name;

	if ( GPlatesFeatureVisitors::get_property_value(
		feature_handle.reference(), name_property_name, feature_name ) ) 
	{
		oss << GPlatesUtils::make_qstring(feature_name->value()).toStdString();
		// report progress
		qDebug() << " processing mesh: " << GPlatesUtils::make_qstring( feature_name->value() );
	}
	else
	{
		oss << "unknown_mesh_name";
		// report progress
		qDebug() << " processing mesh: " << oss.str().c_str();
	}
#endif


	// create an accumulator struct 
	// visit the properties once to check times and rot ids
	// visit the properties once to reconstruct ?
	// parse the boundary string
	// resolve the boundary vertex_list

	d_accumulator = ReconstructedFeatureGeometryAccumulator();

	// Now visit each of the properties in turn, twice -- firstly, to find a reconstruction
	// plate ID and to determine whether the feature is defined at this reconstruction time;
	// after that, to perform the reconstructions (if appropriate) using the plate ID.

	// The first time through, we're not reconstructing, just gathering information.
	d_accumulator->d_perform_reconstructions = false;

	visit_feature_properties( feature_handle );

	// So now we've visited the properties of this feature.  Let's find out if we were able
	// to obtain all the information we need.
	if ( ! d_accumulator->d_feature_is_defined_at_recon_time) {
		// Quick-out: No need to continue.
		d_accumulator = boost::none;
		d_feature_handle_ptr = NULL;
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
			d_feature_handle_ptr = NULL;
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

	//
	// visit the props
	//
	visit_feature_properties( feature_handle );

	// disable the accumulator
	d_accumulator = boost::none;
	d_feature_handle_ptr = NULL;
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gml_line_string(
		GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	if ( ! d_accumulator->d_perform_reconstructions) {
		return;
	}

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_ptr =
			gml_line_string.polyline();

	// Since the domain is always stored as a multipoint create a new multipoint using
	// the vertices of the polyline.
	//
	// NOTE: This is slightly dodgy because we will end up creating a MultiPointVectorField
	// that stores a multi-point domain and a corresponding velocity field but the
	// geometry property iterator (referenced by the MultiPointVectorField) will be
	// a polyline geometry and not a multi-point geometry.
	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type velocity_domain =
			GPlatesMaths::MultiPointOnSphere::create_on_heap(
					polyline_ptr->vertex_begin(), polyline_ptr->vertex_end());

	generate_velocities_in_multipoint_domain(velocity_domain);
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gml_multi_point(
		GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	if ( ! d_accumulator->d_perform_reconstructions) {
		return;
	}

	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type velocity_domain =
			gml_multi_point.multipoint();

	generate_velocities_in_multipoint_domain(velocity_domain);
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gml_orientable_curve(
		GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	gml_orientable_curve.base_curve()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gml_point(
		GPlatesPropertyValues::GmlPoint &gml_point)
{
	if ( ! d_accumulator->d_perform_reconstructions) {
		return;
	}

	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_ptr = gml_point.point();

	// Since the domain is always stored as a multipoint create a new multipoint using the point.
	//
	// NOTE: This is slightly dodgy because we will end up creating a MultiPointVectorField
	// that stores a multi-point domain and a corresponding velocity field but the
	// geometry property iterator (referenced by the MultiPointVectorField) will be
	// a point geometry and not a multi-point geometry.
	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type velocity_domain =
			GPlatesMaths::MultiPointOnSphere::create_on_heap(
					point_ptr.get(), point_ptr.get() + 1);

	generate_velocities_in_multipoint_domain(velocity_domain);
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gml_polygon(
		GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	if ( ! d_accumulator->d_perform_reconstructions) {
		return;
	}

	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_ptr = gml_polygon.exterior();

	// Since the domain is always stored as a multipoint create a new multipoint using
	// the vertices of the polygon.
	//
	// NOTE: This is slightly dodgy because we will end up creating a MultiPointVectorField
	// that stores a multi-point domain and a corresponding velocity field but the
	// geometry property iterator (referenced by the MultiPointVectorField) will be
	// a polygon geometry and not a multi-point geometry.
	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type velocity_domain =
			GPlatesMaths::MultiPointOnSphere::create_on_heap(
					polygon_ptr->vertex_begin(), polygon_ptr->vertex_end());

	generate_velocities_in_multipoint_domain(velocity_domain);
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::generate_velocities_in_multipoint_domain(
		const GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type &velocity_domain)
{
	GPlatesMaths::MultiPointOnSphere::const_iterator iter = velocity_domain->begin();
	GPlatesMaths::MultiPointOnSphere::const_iterator end = velocity_domain->end();

	GPlatesAppLogic::MultiPointVectorField::non_null_ptr_type vector_field_ptr =
			GPlatesAppLogic::MultiPointVectorField::create_empty(
					d_recon_tree_ptr,
					velocity_domain,
					*d_feature_handle_ptr,
					*current_top_level_propiter());
	GPlatesAppLogic::MultiPointVectorField::codomain_type::iterator field_iter =
			vector_field_ptr->begin();

	for ( ; iter != end; ++iter, ++field_iter)
	{
		d_num_points += 1;
		process_point(*iter, *field_iter);
	}

	// Having created and populated the MultiPointVectorField, let's store it in the collection
	// of velocity fields.
	d_velocity_fields_to_populate.push_back(vector_field_ptr);
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::process_point(
		const GPlatesMaths::PointOnSphere &point,
		boost::optional<GPlatesAppLogic::MultiPointVectorField::CodomainElement> &range_element)
{
	// First check whether point is inside any topological networks.

	boost::optional< std::vector<double> > interpolated_velocity_scalars =
			GPlatesAppLogic::TopologyUtils::interpolate_resolved_topology_networks(
					d_resolved_networks_for_velocity_interpolation,
					point);

	if (interpolated_velocity_scalars)
	{
		const GPlatesMaths::VectorColatitudeLongitude velocity_colat_lon =
				GPlatesAppLogic::PlateVelocityUtils::convert_velocity_scalars_to_colatitude_longitude(
						*interpolated_velocity_scalars);

		process_point_in_network(point, range_element, velocity_colat_lon);
		return;
	}


	//
	// Next see if point is inside any topological boundaries.
	//

	// Get a list of all resolved topological boundaries that contain 'point'.
	GPlatesAppLogic::TopologyUtils::resolved_topological_boundary_seq_type
			resolved_topological_boundaries_containing_point;
	GPlatesAppLogic::TopologyUtils::find_resolved_topology_boundaries_containing_point(
			resolved_topological_boundaries_containing_point,
			point,
			d_resolved_boundaries_for_partitioning_geometry_query);

	if (!resolved_topological_boundaries_containing_point.empty())
	{
		process_point_in_plate_polygon(
				point, range_element, resolved_topological_boundaries_containing_point);
		return;
	}


	//
	// Next see if point is inside any static (reconstructed) polygons.
	//

	const boost::optional<const GPlatesAppLogic::ReconstructionGeometry *>
			reconstructed_static_polygon_containing_point = 
					d_reconstructed_static_polygons_query.partition_point(point);

	if (reconstructed_static_polygon_containing_point)
	{
		process_point_in_static_polygon(
				point, range_element, reconstructed_static_polygon_containing_point.get());
		return;
	}

    // else, point not found in any topology or static polygons so set the velocity values to 0 

	GPlatesMaths::Vector3D zero_velocity(0, 0, 0);
	GPlatesAppLogic::MultiPointVectorField::CodomainElement::Reason reason =
			GPlatesAppLogic::MultiPointVectorField::CodomainElement::NotInAnyBoundaryOrNetwork;
	range_element = GPlatesAppLogic::MultiPointVectorField::CodomainElement(zero_velocity, reason);

	// In the previous code, the point wasn't rendered if it wasn't in any boundary or network.

#ifdef DEBUG
    // report a warning 
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(point);
	std::cout << "WARNING: mesh point not in any Topology! point = " << llp << std::endl;
#endif

}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::process_point_in_network(
		const GPlatesMaths::PointOnSphere &point, 
		boost::optional<GPlatesAppLogic::MultiPointVectorField::CodomainElement> &range_element,
		const GPlatesMaths::VectorColatitudeLongitude &velocity_colat_lon)
{
	const GPlatesMaths::Vector3D velocity_vector =
			GPlatesMaths::convert_vector_from_colat_lon_to_xyz(point, velocity_colat_lon);

	GPlatesAppLogic::MultiPointVectorField::CodomainElement::Reason reason =
			GPlatesAppLogic::MultiPointVectorField::CodomainElement::InDeformationNetwork;
	range_element = GPlatesAppLogic::MultiPointVectorField::CodomainElement(velocity_vector, reason);
	// In the previous code, the point was rendered black if it was in a deformation network.
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::process_point_in_plate_polygon(
		const GPlatesMaths::PointOnSphere &point,
		boost::optional<GPlatesAppLogic::MultiPointVectorField::CodomainElement> &range_element,
		const GPlatesAppLogic::TopologyUtils::resolved_topological_boundary_seq_type &
				resolved_topological_boundaries_containing_point)
{

#ifdef DEBUG
GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(point);
std::cout << "ComputationalMeshSolver::process_point_in_plate_polygon: " << llp << std::endl;
#endif

	boost::optional< std::pair<
			GPlatesModel::integer_plate_id_type,
			const GPlatesAppLogic::ResolvedTopologicalBoundary * > > recon_plate_id_opt =
			GPlatesAppLogic::TopologyUtils::
					find_reconstruction_plate_id_furthest_from_anchor_in_plate_circuit(
							resolved_topological_boundaries_containing_point);
	if (!recon_plate_id_opt)
	{
		// FIXME: do not paint the point any color ; leave it ?

		GPlatesMaths::Vector3D zero_velocity(0, 0, 0);
		GPlatesAppLogic::MultiPointVectorField::CodomainElement::Reason reason =
				GPlatesAppLogic::MultiPointVectorField::CodomainElement::NotInAnyBoundaryOrNetwork;
		range_element = GPlatesAppLogic::MultiPointVectorField::CodomainElement(zero_velocity, reason);

		// In the previous code, the point wasn't rendered if it wasn't in any boundary or
		// network.

		return; 
	}

	GPlatesModel::integer_plate_id_type recon_plate_id = recon_plate_id_opt->first;
	const GPlatesAppLogic::ResolvedTopologicalBoundary *resolved_topo_boundary = recon_plate_id_opt->second;

	// compute the velocity for this point
	const GPlatesMaths::Vector3D vector_xyz =
			GPlatesAppLogic::PlateVelocityUtils::calc_velocity_vector(
					point, *d_recon_tree_ptr, *d_recon_tree_2_ptr, recon_plate_id);

	GPlatesAppLogic::MultiPointVectorField::CodomainElement::Reason reason =
			GPlatesAppLogic::MultiPointVectorField::CodomainElement::InPlateBoundary;
	range_element = GPlatesAppLogic::MultiPointVectorField::CodomainElement(vector_xyz, reason,
			recon_plate_id, resolved_topo_boundary);

	// In the previous code, the point was rendered according to the plate ID if it was in a
	// plate boundary.
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::process_point_in_static_polygon(
		const GPlatesMaths::PointOnSphere &point,
		boost::optional<GPlatesAppLogic::MultiPointVectorField::CodomainElement> &range_element,
		const GPlatesAppLogic::ReconstructionGeometry *reconstructed_static_polygon_containing_point)
{

#ifdef DEBUG
GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(point);
std::cout << "ComputationalMeshSolver::process_point_in_static_polygon: " << llp << std::endl;
#endif

	const boost::optional<GPlatesModel::integer_plate_id_type> recon_plate_id_opt =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_plate_id(reconstructed_static_polygon_containing_point);
	if (!recon_plate_id_opt)
	{
		// FIXME: do not paint the point any color ; leave it ?

		GPlatesMaths::Vector3D zero_velocity(0, 0, 0);
		GPlatesAppLogic::MultiPointVectorField::CodomainElement::Reason reason =
				GPlatesAppLogic::MultiPointVectorField::CodomainElement::NotInAnyBoundaryOrNetwork;
		range_element = GPlatesAppLogic::MultiPointVectorField::CodomainElement(zero_velocity, reason);

		// In the previous code, the point wasn't rendered if it wasn't in any boundary or
		// network.

		return; 
	}

	GPlatesModel::integer_plate_id_type recon_plate_id = recon_plate_id_opt.get();

	// compute the velocity for this point
	const GPlatesMaths::Vector3D vector_xyz =
			GPlatesAppLogic::PlateVelocityUtils::calc_velocity_vector(
					point, *d_recon_tree_ptr, *d_recon_tree_2_ptr, recon_plate_id);

	GPlatesAppLogic::MultiPointVectorField::CodomainElement::Reason reason =
			GPlatesAppLogic::MultiPointVectorField::CodomainElement::InStaticPolygon;
	range_element = GPlatesAppLogic::MultiPointVectorField::CodomainElement(vector_xyz, reason,
			recon_plate_id, reconstructed_static_polygon_containing_point);
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gml_time_period(
		GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
	static const GPlatesModel::PropertyName valid_time_property_name =
		GPlatesModel::PropertyName::create_gml("validTime");

	if ( ! d_accumulator->d_perform_reconstructions) {
		// We're gathering information, not performing reconstructions.

		// Note that we're going to assume that we're in a property...
		if (current_top_level_propname() == valid_time_property_name) {
			// This time period is the "valid time" time period.
			if ( ! gml_time_period.contains(d_recon_time)) {
				// Oh no!  This feature instance is not defined at the recon time!
				d_accumulator->d_feature_is_defined_at_recon_time = false;
			}
		}
	}
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gpml_plate_id(
		GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	static GPlatesModel::PropertyName reconstruction_plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

	if ( ! d_accumulator->d_perform_reconstructions) {
		// We're gathering information, not performing reconstructions.

		// Note that we're going to assume that we're in a property...
		if (current_top_level_propname() == reconstruction_plate_id_property_name) {
			// This plate ID is the reconstruction plate ID.
			d_accumulator->d_recon_plate_id = gpml_plate_id.value();
		}
	}
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::report()
{
	std::cout << "-------------------------------------------------------------" << std::endl;
	std::cout << "GPlatesFeatureVisitors::ComputationalMeshSolver::report() " << std::endl;
	std::cout << "number features visited = " << d_num_features << std::endl;
	std::cout << "number meshes visited = " << d_num_meshes << std::endl;
	std::cout << "number points visited = " << d_num_points << std::endl;
	std::cout << "-------------------------------------------------------------" << std::endl;

}


////
