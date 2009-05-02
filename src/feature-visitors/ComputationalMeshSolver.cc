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


#include <fstream>
#include <sstream>
#include <iostream>

#include <QLocale>
#include <QDebug>
#include <QList>
#include <QString>
#include <boost/none.hpp>

#include "ComputationalMeshSolver.h"

#include "feature-visitors/PropertyValueFinder.h"


#include "model/FeatureHandleWeakRefBackInserter.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "model/Reconstruction.h"
#include "model/ReconstructionTree.h"
#include "model/FeatureHandle.h"
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
#include "property-values/GmlDomainSet.h"

#include "maths/PolylineOnSphere.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/LatLonPointConversions.h"
#include "maths/ProximityCriteria.h"
#include "maths/PolylineIntersections.h"
#include "maths/CalculateVelocityOfPoint.h"

#include "gui/ProximityTests.h"
#include "gui/PlatesColourTable.h"

#include "utils/UnicodeStringUtils.h"

#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryParameters.h"


GPlatesFeatureVisitors::ComputationalMeshSolver::ComputationalMeshSolver(
			const double &recon_time,
			const double &recon_time_2,
			unsigned long root_plate_id,
			GPlatesModel::Reconstruction &recon,
			GPlatesModel::ReconstructionTree &recon_tree,
			GPlatesModel::ReconstructionTree &recon_tree_2,
			GPlatesFeatureVisitors::TopologyResolver &topo_resolver,
			reconstruction_geometries_type &reconstructed_geometries,
			GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type layer,
			bool should_keep_features_without_recon_plate_id):
	d_recon_time(GPlatesPropertyValues::GeoTimeInstant(recon_time)),
	d_root_plate_id(GPlatesModel::integer_plate_id_type(root_plate_id)),
	d_recon_ptr(&recon),
	d_recon_tree_ptr(&recon_tree),
	d_recon_tree_2_ptr(&recon_tree_2),
	d_topology_resolver_ptr(&topo_resolver),
	d_reconstruction_geometries_to_populate(&reconstructed_geometries),
	d_rendered_layer(layer),
	d_should_keep_features_without_recon_plate_id(should_keep_features_without_recon_plate_id)
{  
	d_num_features = 0;
	d_num_meshes = 0;
	d_num_points = 0;
	d_num_points_on_multiple_plates = 0;

	d_colour_table_ptr = GPlatesGui::PlatesColourTable::Instance();
}



void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_feature_handle(
		GPlatesModel::FeatureHandle &feature_handle)
{
	d_num_features += 1;


	QString type_name( GPlatesUtils::make_qstring_from_icu_string(
		feature_handle.feature_type().get_name() ) );

	// super short-cut for non-mesh features
	QString type_unclass("UnclassifiedFeature");
	QString type_coverage("Coverage");

	if ( (type_name == type_unclass) || (type_name == type_coverage) )
	{
		// ?
	} 
	else {
		// Quick-out: No need to continue.
		return; 
	}
	d_num_meshes += 1;

	// else process this feature:
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


	//
	// output the data
	//
	std::ostringstream oss;

	oss << "velocity_colat+lon_at_";
	oss << d_recon_time.value();
	oss << "Ma_on_mesh-";

	
	// Get the name property value
	static const GPlatesModel::PropertyName name_property_name =
		GPlatesModel::PropertyName::create_gml("name");
	const GPlatesPropertyValues::XsString *name;
	if ( GPlatesFeatureVisitors::get_property_value(
		feature_handle, name_property_name, name ) ) 
	{
		oss << GPlatesUtils::make_qstring(name->value()).toStdString();
	}
	else
	{
		oss << "unknown_mesh_name";
	}

	oss << "-for_1Ma_inc";
	oss << ".dat";

	// report progress
	qDebug() << " processing mesh: " << GPlatesUtils::make_qstring(name->value());

	// Now for the second pass through the properties of the feature:  
	// This time we reconstruct any geometries we find.
	d_accumulator->d_perform_reconstructions = true;

	// clear the output string for this feature
	d_output_string.clear();

	visit_feature_properties(feature_handle);

	d_accumulator = boost::none;

	// build an output file stream
	std::ofstream outfile( oss.str().c_str() );
	if ( !outfile ) {
		std::cout << "WARNING: cannot open file for writing" << std::endl;
		return;
	}
	// write out data
	outfile << d_output_string;
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
//std::cout << "ComputationalMeshSolver::visit_feature_props: " << std::endl;
	GPlatesModel::FeatureHandle::properties_iterator iter = feature_handle.properties_begin();
	GPlatesModel::FeatureHandle::properties_iterator end = feature_handle.properties_end();
	for ( ; iter != end; ++iter) {
		// Elements of this properties vector can be NULL pointers.  (See the comment in
		// "model/FeatureRevision.h" for more details.)
		if (*iter != NULL) {
			// FIXME: This d_current_property thing could go in {Const,}FeatureVisitor base.
			d_accumulator->d_current_property = iter;
			(*iter)->accept_visitor(*this);
		}
	}
}



void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_top_level_property_inline(
		GPlatesModel::TopLevelPropertyInline &top_level_property_inline)
{
//std::cout << "ComputationalMeshSolver::visit_top_level_property_inline: " << std::endl;
	visit_property_values( top_level_property_inline );
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_property_values(
		GPlatesModel::TopLevelPropertyInline &top_level_property_inline)
{
//std::cout << "ComputationalMeshSolver::visit_property_values: " << std::endl;
	GPlatesModel::TopLevelPropertyInline::const_iterator iter = top_level_property_inline.begin();
	GPlatesModel::TopLevelPropertyInline::const_iterator end = top_level_property_inline.end();
	for ( ; iter != end; ++iter) {
		(*iter)->accept_visitor(*this);
	}
}



void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gml_multi_point(
	GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
//std::cout << "ComputationalMeshSolver::visit_gml_multi_point: " << std::endl;
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

#ifdef DEBUG
GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(point);
//std::cout << "ComputationalMeshSolver::process_point: " << llp << std::endl;
#endif

	std::vector<GPlatesModel::FeatureId> feature_ids;
	std::vector<GPlatesModel::FeatureId>::iterator iter;

	std::list<GPlatesModel::integer_plate_id_type> recon_plate_ids;
	std::list<GPlatesModel::integer_plate_id_type>::iterator p_iter;

	// Locate the point
	feature_ids = d_topology_resolver_ptr->locate_point( point );

#ifdef DEBUG
// FIXME
//std::cout << "ComputationalMeshSolver::process_point: found in " << feature_ids.size() << " plates." << std::endl;
#endif

	// loop over feature ids 
	for (iter = feature_ids.begin(); iter != feature_ids.end(); ++iter)
	{
		GPlatesModel::FeatureId feature_id = *iter;

		// Get a vector of FeatureHandle weak_refs for this Feature ID
		std::vector<GPlatesModel::FeatureHandle::weak_ref> back_refs;	
		feature_id.find_back_ref_targets( append_as_weak_refs( back_refs ) );

		// Double check vector
		if ( back_refs.size() == 0 )
		{
			continue; // to next feature id 
		}

	 	// else , get the first ref on the list		
	 	GPlatesModel::FeatureHandle::weak_ref feature_ref = back_refs.front();

		// Get all the reconstructionPlateId for this point 
		const GPlatesModel::PropertyName property_name =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		const GPlatesPropertyValues::GpmlPlateId *recon_plate_id;

		if ( GPlatesFeatureVisitors::get_property_value(
				*feature_ref, property_name, recon_plate_id ) )
		{
			// The feature has a reconstruction plate ID.
			recon_plate_ids.push_back( recon_plate_id->value() );
		}
	}

	if ( recon_plate_ids.empty() ) {
		// FIXME: do not paint the point any color ; leave it ?
		return; 
	}

	// update the stats
	if ( recon_plate_ids.size() > 1) { 
		d_num_points_on_multiple_plates += 1;
	}

	// sort the plate ids
	recon_plate_ids.sort();
	recon_plate_ids.reverse();

#ifdef DEBUG
// FIXME: remove this diagnostic 
std::cout << "plate ids: (";
for ( p_iter = recon_plate_ids.begin(); p_iter != recon_plate_ids.end(); ++p_iter)
{
	std::cout << *p_iter << ", ";
}
std::cout << ")" << std::endl;
#endif

	// get the highest numeric id 
	GPlatesModel::integer_plate_id_type plate_id = recon_plate_ids.front();
		

	// get the color for the highest numeric id 
	GPlatesGui::ColourTable::const_iterator colour = d_colour_table_ptr->end();
	colour = d_colour_table_ptr->lookup_by_plate_id( plate_id ); 
	if (colour == d_colour_table_ptr->end()) { 
		colour = &GPlatesGui::Colour::get_olive(); 
	}


	// Create a RenderedGeometry using the reconstructed geometry.
	const GPlatesViewOperations::RenderedGeometry rendered_geom =
		GPlatesViewOperations::create_rendered_geometry_on_sphere(
			point.clone_as_geometry(),
			*colour,
			GPlatesViewOperations::GeometryOperationParameters::REGULAR_POINT_SIZE_HINT,
			GPlatesViewOperations::GeometryOperationParameters::LINE_WIDTH_HINT);

	// Add to the rendered layer.
	d_rendered_layer->add_rendered_geometry(rendered_geom);

	// get the finite rotation for this palte id
	GPlatesMaths::FiniteRotation fr_t1 = 
		d_recon_tree_ptr->get_composed_absolute_rotation( plate_id ).first;

	GPlatesMaths::FiniteRotation fr_t2 = 
		d_recon_tree_2_ptr->get_composed_absolute_rotation( plate_id ).first;


	std::pair< GPlatesMaths::real_t, GPlatesMaths::real_t > velocity_pair = 
		GPlatesMaths::CalculateVelocityOfPoint( point, fr_t1, fr_t2 );

	// compute the velocity for this point
	GPlatesMaths::real_t colat_v = velocity_pair.first;	
	GPlatesMaths::real_t lon_v   = velocity_pair.second;
#ifdef DEBUG
	std::cout << "colat_v = " << colat_v << " ; " << "lon_v = " << lon_v << " ; " << std::endl;
#endif

	// OUTPUT
	std::ostringstream oss;
	oss << colat_v << '\t' << lon_v << '\n';
	d_output_string.append( oss.str() );
}




void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gml_time_period(
		GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
//std::cout << "ComputationalMeshSolver::visit_gml_time_period: " << std::endl;
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
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
//std::cout << "ComputationalMeshSolver::visit_gpml_constant_value: " << std::endl;
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gpml_plate_id(
		GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
//std::cout << "ComputationalMeshSolver::visit_gpml_plate_id: " << std::endl;
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
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gml_data_block(
		GPlatesPropertyValues::GmlDataBlock &gml_data_block)
{
//std::cout << "ComputationalMeshSolver::visit_gml_data_block: " << std::endl;
}



void
GPlatesFeatureVisitors::ComputationalMeshSolver::report()
{
	std::cout << "-------------------------------------------------------------" << std::endl;
	std::cout << "GPlatesFeatureVisitors::ComputationalMeshSolver::report() " << std::endl;
	std::cout << "number features visited = " << d_num_features << std::endl;
	std::cout << "number meshes visited = " << d_num_meshes << std::endl;
	std::cout << "number points visited = " << d_num_points << std::endl;
	std::cout << "number points on multiple plates = " << d_num_points_on_multiple_plates << std::endl;
	std::cout << "-------------------------------------------------------------" << std::endl;

}


////
