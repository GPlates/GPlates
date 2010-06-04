/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-08-15 02:13:48 -0700 (Fri, 15 Aug 2008) $
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
#include <QList>
#include <QString>
#include <boost/none.hpp>

#include "ComputationalMeshSolver.h"

#include "app-logic/PlateVelocityUtils.h"
#include "app-logic/TopologyUtils.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "file-io/GpmlOnePointSixOutputVisitor.h"
#include "file-io/FileInfo.h"

#include "maths/PolylineOnSphere.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/LatLonPoint.h"
#include "maths/ProximityCriteria.h"
#include "maths/PolylineIntersections.h"
#include "maths/CalculateVelocity.h"
#include "maths/CartesianConvMatrix3D.h"

#include "model/FeatureHandle.h"
#include "model/FeatureHandleWeakRefBackInserter.h"
#include "model/ModelUtils.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "model/Reconstruction.h"
#include "model/ReconstructionTree.h"
#include "model/ResolvedTopologicalBoundary.h"
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
			GPlatesModel::Reconstruction &reconstruction,
			const double &recon_time,
			unsigned long root_plate_id,
			//GPlatesModel::Reconstruction &recon,
			GPlatesModel::ReconstructionTree &recon_tree,
			GPlatesModel::ReconstructionTree &recon_tree_2,
			const GPlatesAppLogic::TopologyUtils::resolved_boundaries_for_geometry_partitioning_query_type &
					resolved_boundaries_for_partitioning_geometry_query,
			const GPlatesAppLogic::TopologyUtils::resolved_networks_for_interpolation_query_type &
					resolved_networks_for_velocity_interpolation,
			//reconstruction_geometries_type &reconstructed_geometries,
			GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type point_layer,
			GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type arrow_layer,
			bool should_keep_features_without_recon_plate_id):
	d_recon_time(GPlatesPropertyValues::GeoTimeInstant(recon_time)),
	d_root_plate_id(GPlatesModel::integer_plate_id_type(root_plate_id)),
	d_recon_ptr(&reconstruction),
	d_recon_tree_ptr(&recon_tree),
	d_recon_tree_2_ptr(&recon_tree_2),
	d_resolved_boundaries_for_partitioning_geometry_query(
			resolved_boundaries_for_partitioning_geometry_query),
	d_resolved_networks_for_velocity_interpolation(
			resolved_networks_for_velocity_interpolation),
	//d_reconstruction_geometries_to_populate(&reconstructed_geometries),
	d_rendered_point_layer(point_layer),
	d_rendered_arrow_layer(arrow_layer),
	d_should_keep_features_without_recon_plate_id(should_keep_features_without_recon_plate_id),
	d_colour_palette(GPlatesGui::DefaultPlateIdColourPalette::create())
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


	QString type_name( GPlatesUtils::make_qstring_from_icu_string(
		feature_handle.feature_type().get_name() ) );

	//
	// super short-cut for non-mesh features
	//
	QString type_mesh_node("VelocityField");

	if ( ! (type_name == type_mesh_node) )
	{
		// Quick-out: No need to continue.
		return; 
	}
	d_num_meshes += 1;

	// else process this feature:

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


	// Clear out old data
	d_velocity_colat_values.clear();
	d_velocity_lon_values.clear();

	// output the data
	//

	// Now for the second pass through the properties of the feature:  
	// This time we reconstruct any geometries we find.
	d_accumulator->d_perform_reconstructions = true;

	//
	// visit the props
	//
	visit_feature_properties( feature_handle );


	//
	// Set up GmlDataBlock 
	//
	GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type gml_data_block =
			GPlatesPropertyValues::GmlDataBlock::create();

	GPlatesPropertyValues::ValueObjectType velocity_colat_type =
			GPlatesPropertyValues::ValueObjectType::create_gpml("VelocityColat");
	GPlatesPropertyValues::GmlDataBlockCoordinateList::xml_attributes_type xml_attrs_velocity_colat;
	GPlatesModel::XmlAttributeName uom = GPlatesModel::XmlAttributeName::create_gpml("uom");
	GPlatesModel::XmlAttributeValue cm_per_year("urn:x-si:v1999:uom:cm_per_year");
	xml_attrs_velocity_colat.insert(std::make_pair(uom, cm_per_year));

	GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_type velocity_colat =
			GPlatesPropertyValues::GmlDataBlockCoordinateList::create_copy(
					velocity_colat_type, xml_attrs_velocity_colat,
					d_velocity_colat_values.begin(), d_velocity_colat_values.end() );

	GPlatesPropertyValues::ValueObjectType velocity_lon_type =
			GPlatesPropertyValues::ValueObjectType::create_gpml("VelocityLon");
	GPlatesPropertyValues::GmlDataBlockCoordinateList::xml_attributes_type xml_attrs_velocity_lon;
	xml_attrs_velocity_lon.insert(std::make_pair(uom, cm_per_year));

	GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_type velocity_lon =
			GPlatesPropertyValues::GmlDataBlockCoordinateList::create_copy(
					velocity_lon_type, xml_attrs_velocity_lon,
					d_velocity_lon_values.begin(), d_velocity_lon_values.end() );

	gml_data_block->tuple_list_push_back( velocity_colat );
	gml_data_block->tuple_list_push_back( velocity_lon );

	//
	// append the GmlDataBlock property 
	//

	// find the old prop to remove
	GPlatesModel::PropertyName range_set_prop_name =
		GPlatesModel::PropertyName::create_gml("rangeSet");

	GPlatesModel::FeatureHandle::const_iterator iter = feature_handle.begin();
	GPlatesModel::FeatureHandle::const_iterator end = feature_handle.end();

	// loop over properties
	for ( ; iter != end; ++iter)
	{
		/*
		// double check for validity and nullness
		if(! iter.is_valid() ){ continue; }
		if(! (*iter) )   { continue; }
		*/

		// passed all checks, make the name and test
		GPlatesModel::PropertyName test_name = (*iter)->property_name();

		if ( test_name == range_set_prop_name )
		{
			// Delete the old boundary 
			// GPlatesModel::DummyTransactionHandle transaction(__FILE__, __LINE__);
			feature_handle.remove(iter);
			// transaction.commit();
			// FIXME: this seems to create NULL pointers in the properties collection
			// see FIXME note above to check for NULL? 
			// Or is this to be expected?
			break;
		}
	} // loop over properties

	// create a weak ref to make next function happy:
	GPlatesModel::FeatureHandle::weak_ref feature_ref = feature_handle.reference();

	// add the new gml::rangeSet property
	feature_ref->add(
			GPlatesModel::TopLevelPropertyInline::create(
				range_set_prop_name,
				gml_data_block));

	// disable the accumulator
	d_accumulator = boost::none;
}



void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gml_multi_point(
		GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	if ( ! d_accumulator->d_perform_reconstructions) {
		return;
	}

	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multipoint_ptr =
		gml_multi_point.multipoint();
	
	GPlatesMaths::MultiPointOnSphere::const_iterator iter = multipoint_ptr->begin();
	GPlatesMaths::MultiPointOnSphere::const_iterator end = multipoint_ptr->end();
	for ( ; iter != end; ++iter)
	{
		d_num_points += 1;
		process_point(*iter);
	}
}

void
GPlatesFeatureVisitors::ComputationalMeshSolver::process_point(
		const GPlatesMaths::PointOnSphere &point )
{
	//
	// First see if point is inside any topological networks.
	//

	boost::optional< std::vector<double> > interpolated_velocity_scalars =
			GPlatesAppLogic::TopologyUtils::interpolate_resolved_topology_networks(
					d_resolved_networks_for_velocity_interpolation,
					point);

	if (interpolated_velocity_scalars)
	{
		const GPlatesMaths::VectorColatitudeLongitude velocity_colat_lon =
				GPlatesAppLogic::PlateVelocityUtils::convert_velocity_scalars_to_colatitude_longitude(
						*interpolated_velocity_scalars);

		process_point_in_network(point, velocity_colat_lon);
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
				point, resolved_topological_boundaries_containing_point);
		return;
	}
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::process_point_in_network(
		const GPlatesMaths::PointOnSphere &point, 
		const GPlatesMaths::VectorColatitudeLongitude &velocity_colat_lon)
{
	const double &colat_v = velocity_colat_lon.get_vector_colatitude().dval();
	const double &lon_v = velocity_colat_lon.get_vector_longitude().dval();

	// save the data in the lists 
	d_velocity_colat_values.push_back( colat_v );
	d_velocity_lon_values.push_back( lon_v );

	const GPlatesMaths::Vector3D velocity_vector =
			GPlatesMaths::convert_vector_from_colat_lon_to_xyz(point, velocity_colat_lon);

	render_velocity(point, velocity_vector, GPlatesGui::Colour::get_black());
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::process_point_in_plate_polygon(
		const GPlatesMaths::PointOnSphere &point,
		GPlatesAppLogic::TopologyUtils::resolved_topological_boundary_seq_type
				resolved_topological_boundaries_containing_point)
{

#ifdef DEBUG
GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(point);
std::cout << "ComputationalMeshSolver::process_point: " << llp << std::endl;
#endif

	const boost::optional<GPlatesModel::integer_plate_id_type> recon_plate_id_opt =
			GPlatesAppLogic::TopologyUtils::
					find_reconstruction_plate_id_furthest_from_anchor_in_plate_circuit(
							resolved_topological_boundaries_containing_point);
	if (!recon_plate_id_opt)
	{
		// FIXME: do not paint the point any color ; leave it ?
		// save the data 
		d_velocity_colat_values.push_back( 0 );
		d_velocity_lon_values.push_back( 0 );
		return; 
	}

	const GPlatesModel::integer_plate_id_type recon_plate_id = *recon_plate_id_opt;

	// get the color for the highest numeric id 
	const boost::optional<GPlatesGui::Colour> plate_id_colour_opt =
			d_colour_palette->get_colour(recon_plate_id);
	const GPlatesGui::Colour &plate_id_colour = plate_id_colour_opt
			? *plate_id_colour_opt
			: GPlatesGui::Colour::get_olive();

	// compute the velocity for this point
	const GPlatesMaths::Vector3D vector_xyz =
			GPlatesAppLogic::PlateVelocityUtils::calc_velocity_vector(
					point, *d_recon_tree_ptr, *d_recon_tree_2_ptr, recon_plate_id);

	const GPlatesMaths::VectorColatitudeLongitude velocity_colat_lon = 
			GPlatesMaths::convert_vector_from_xyz_to_colat_lon(point, vector_xyz);

#ifdef DEBUG
	std::cout
			<< "colat_v = " << velocity_colat_lon.get_vector_colatitude()
			<< " ; "
			<< "lon_v = " << velocity_colat_lon.get_vector_longitude() << " ; " << std::endl;
#endif

	// save the data 
	d_velocity_colat_values.push_back( velocity_colat_lon.get_vector_colatitude().dval() );
	d_velocity_lon_values.push_back( velocity_colat_lon.get_vector_longitude().dval() );

	render_velocity(point, vector_xyz, plate_id_colour);
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


#if 0
void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gml_data_block(
		GPlatesPropertyValues::GmlDataBlock &gml_data_block)
{

}
#endif


void
GPlatesFeatureVisitors::ComputationalMeshSolver::render_velocity(
		const GPlatesMaths::PointOnSphere &point,
		const GPlatesMaths::Vector3D &velocity,
		const GPlatesGui::Colour &colour)
{
	// Create a RenderedGeometry using the reconstructed geometry.
	const GPlatesViewOperations::RenderedGeometry rendered_point =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
					point,
					colour,
					GPlatesViewOperations::GeometryOperationParameters::REGULAR_POINT_SIZE_HINT);

	// Add to the rendered layer.
	d_rendered_point_layer->add_rendered_geometry(rendered_point);

	// Create a RenderedGeometry using the vector
	const GPlatesViewOperations::RenderedGeometry rendered_vector =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_direction_arrow(
					point,
					velocity,
					0.05f,
					colour);

	// Add to the rendered layer.
	d_rendered_arrow_layer->add_rendered_geometry( rendered_vector );
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
