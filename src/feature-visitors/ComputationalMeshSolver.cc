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
#include <sstream>
#include <iostream>

#include <QLocale>
#include <QDebug>
#include <QList>
#include <QString>
#include <boost/none.hpp>

#include "ComputationalMeshSolver.h"

#include "feature-visitors/ValueFinder.h"
#include "feature-visitors/ReconstructedFeatureGeometryFinder.h"

#include "model/ReconstructedFeatureGeometry.h"
#include "model/Reconstruction.h"
#include "model/ReconstructionTree.h"
#include "model/FeatureHandle.h"
#include "model/InlinePropertyContainer.h"
#include "model/FeatureRevision.h"

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

#include "maths/PolylineOnSphere.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/LatLonPointConversions.h"
#include "maths/ProximityCriteria.h"
#include "maths/PolylineIntersections.h"

#include "gui/ProximityTests.h"

#include "utils/UnicodeStringUtils.h"
#include "utils/FeatureHandleToOldId.h"

GPlatesFeatureVisitors::ComputationalMeshSolver::ComputationalMeshSolver(
			const double &recon_time,
			unsigned long root_plate_id,
			GPlatesModel::Reconstruction &recon,
			GPlatesModel::ReconstructionTree &recon_tree,
			GPlatesFeatureVisitors::ReconstructedFeatureGeometryFinder &finder,
			reconstruction_geometries_type &reconstructed_geometries,
			bool should_keep_features_without_recon_plate_id):
	d_recon_time(GPlatesPropertyValues::GeoTimeInstant(recon_time)),
	d_root_plate_id(GPlatesModel::integer_plate_id_type(root_plate_id)),
	d_recon_ptr(&recon),
	d_recon_tree_ptr(&recon_tree),
	d_recon_finder_ptr(&finder),
	d_reconstruction_geometries_to_populate(&reconstructed_geometries),
	d_should_keep_features_without_recon_plate_id(should_keep_features_without_recon_plate_id)
{  
	d_num_features = 0;
	d_num_meshes = 0;
}



void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_feature_handle(
		GPlatesModel::FeatureHandle &feature_handle)
{
	d_num_features += 1;

qDebug() << "qDebug: " << GPlatesUtils::make_qstring_from_icu_string(feature_handle.feature_type().get_name() );


	// super short-cut for features without boundary list properties
	QString type("ComputationalMesh");
	if ( type != GPlatesUtils::make_qstring_from_icu_string(feature_handle.feature_type().get_name() ) ) { 
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


	// Now for the second pass through the properties of the feature:  
	// This time we reconstruct any geometries we find.
	d_accumulator->d_perform_reconstructions = true;

	visit_feature_properties(feature_handle);

	d_accumulator = boost::none;
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
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
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_inline_property_container(
		GPlatesModel::InlinePropertyContainer &inline_property_container)
{
std::cout << "GPlatesFeatureVisitors::ComputationalMeshSolver::visit_inline_property_container() " << std::endl;
	visit_property_values(inline_property_container);
}

void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gml_multi_point(
	GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	if ( ! d_accumulator->d_perform_reconstructions) {
		return;
	}
std::cout << "GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gml_multi_point() " << std::endl;

	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multipoint_ptr =
		gml_multi_point.multipoint();
	
	GPlatesMaths::MultiPointOnSphere::const_iterator iter = multipoint_ptr->begin();
	GPlatesMaths::MultiPointOnSphere::const_iterator end = multipoint_ptr->end();
	for ( ; iter != end; ++iter)
	{
		process_point(*iter);
	}
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::process_point(
	const GPlatesMaths::PointOnSphere &point )
{
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(point);
std::cout << "llp =" << llp << std::endl;

}




void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gml_time_period(
		GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
std::cout << "GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gml_time_period() " << std::endl;
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
std::cout << "GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gpml_constant_value() " << std::endl;
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gpml_plate_id(
		GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
std::cout << "GPlatesFeatureVisitors::ComputationalMeshSolver::visit_gpml_plate_id() " << std::endl;
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
GPlatesFeatureVisitors::ComputationalMeshSolver::report()
{
	std::cout << "GPlatesFeatureVisitors::ComputationalMeshSolver::report() " << std::endl;
	std::cout << "number features visited = " << d_num_features << std::endl;
	std::cout << "number meshes visited = " << d_num_meshes << std::endl;
}


////
