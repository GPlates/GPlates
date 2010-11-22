/* $Id: FlowlinesSolver.cc 8461 2010-05-20 14:18:01Z rwatson $ */

/**
* \file 
* File specific comments.
*
* Most recent change:
*   $Date: 2010-05-20 16:18:01 +0200 (to, 20 mai 2010) $
* 
* Copyright (C) 2009, 2010 Geological Survey of Norway.
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
#include <QDebug>

#include <algorithm> // std::find

#include "FlowlineSolver.h"
#include "app-logic/ApplicationState.h"
#include "app-logic/FlowlineUtils.h"
#include "app-logic/ReconstructionTree.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "maths/FiniteRotation.h"
#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "model/ModelUtils.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPoint.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/XsBoolean.h"
#include "utils/GeometryCreationUtils.h"
#include "view-operations/RenderedGeometryFactory.h"

namespace
{

	void
	display_rotation(
		const GPlatesMaths::FiniteRotation &rotation)
	{
		using namespace GPlatesMaths;
		const UnitQuaternion3D old_uq = rotation.unit_quat();
		
		
		const boost::optional<GPlatesMaths::UnitVector3D> &axis_hint = rotation.axis_hint();		
		UnitQuaternion3D::RotationParams params = old_uq.get_rotation_params(axis_hint);
		real_t angle = params.angle;
		
		PointOnSphere point(params.axis);
		LatLonPoint llp = make_lat_lon_point(point);
		
		qDebug() << "Pole: Lat" << llp.latitude() << ", lon: " << llp.longitude() << ", angle: " 
			<< GPlatesMaths::convert_rad_to_deg(params.angle.dval());
		
	}


	void
	display_multipoint(
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multipoint)
	{
		GPlatesMaths::MultiPointOnSphere::const_iterator 
			it = multipoint->begin(),
			end = multipoint->end();
			
		for (; it != end ; ++it)
		{
			GPlatesMaths::LatLonPoint llp = make_lat_lon_point(*it);
			qDebug() << "Lat: " << llp.latitude() << ", lon: " << llp.longitude();
		}	
	}

	void
	add_lines_to_flowline_field_feature(
		GPlatesAppLogic::FlowlineSolver::lines_container_type &lines,
		GPlatesModel::FeatureHandle::weak_ref feature_handle,
		const QString &description)
	{
		GPlatesAppLogic::FlowlineSolver::lines_container_type::iterator 
			it = lines.begin(),
			end = lines.end();
			
		for (; it != end; ++it)
		{
			GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type
				multi_point_on_sphere = GPlatesMaths::MultiPointOnSphere::create_on_heap(
					(*it).begin(),(*it).end());
					
			GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type gml_multi_point = 
				GPlatesPropertyValues::GmlMultiPoint::create(multi_point_on_sphere);
					
			GPlatesModel::PropertyName flowline_prop_name = 
				GPlatesModel::PropertyName::create_gpml(description);
				
			feature_handle->add(
				GPlatesModel::TopLevelPropertyInline::create(
					flowline_prop_name,
					gml_multi_point));				
		}	
	}
	

	void
	add_initial_points_to_lines_container(
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type points,
		GPlatesAppLogic::FlowlineSolver::lines_container_type &lines_container)
	{
		GPlatesMaths::MultiPointOnSphere::const_iterator 
			it_points = points->begin(),
			end_points = points->end();

		// Each element of the vector will contain a list of points corresponding to a 
		// flowline rendered geometry. 
		for (; it_points != end_points ; ++it_points)
		{
			std::list<GPlatesMaths::PointOnSphere> v;
			v.push_back(*it_points);
			lines_container.push_back(v);
		}		
	}
	
	void
	add_subsequent_points_to_lines_container(
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type points,
		GPlatesAppLogic::FlowlineSolver::lines_container_type &lines_container)
	{
		GPlatesMaths::MultiPointOnSphere::const_iterator 
			it_points = points->begin(),
			end_points = points->end();

		GPlatesAppLogic::FlowlineSolver::lines_container_type::iterator 
			it_lines = lines_container.begin();

		for (; it_points != end_points ; ++it_points, ++it_lines)
		{
			it_lines->push_back(*it_points);
		}		
	}	



	// Returns the total pole (i.e. pole w.r.t present day) for moving_plate_id w.r.t. fixed_plate_id.
	GPlatesMaths::FiniteRotation
	get_total_pole(
		GPlatesAppLogic::ReconstructionTree::non_null_ptr_type reconstruction_tree_ptr,
		GPlatesModel::integer_plate_id_type moving_plate_id,
		GPlatesModel::integer_plate_id_type fixed_plate_id)
	{
		// Get the rotation for plate M w.r.t. anchor	
		GPlatesMaths::FiniteRotation rot_M = 
			reconstruction_tree_ptr->get_composed_absolute_rotation(moving_plate_id).first;

		// Get the rotation for plate F w.r.t. anchor	
		GPlatesMaths::FiniteRotation rot_F = 
			reconstruction_tree_ptr->get_composed_absolute_rotation(fixed_plate_id).first;	

		GPlatesMaths::FiniteRotation total_pole = 
			GPlatesMaths::compose(GPlatesMaths::get_reverse(rot_F),rot_M);	

#if 0			
		qDebug() << "Moving: " << moving_plate_id;
		qDebug() << "Fixed: " << fixed_plate_id;
		display_rotation(total_pole);			
#endif						
		return total_pole;	

	}

	// Halves the angle of the provided FiniteRotation.
	void
	get_half_angle_rotation(
		GPlatesMaths::FiniteRotation &rotation)
	{
		if (represents_identity_rotation(rotation.unit_quat()))
		{
			return;
		}
		
		using namespace GPlatesMaths;
		const UnitQuaternion3D old_uq = rotation.unit_quat();
		const boost::optional<GPlatesMaths::UnitVector3D> &axis_hint = rotation.axis_hint();		
		UnitQuaternion3D::RotationParams params = old_uq.get_rotation_params(axis_hint);
		GPlatesMaths::real_t angle = params.angle;

		GPlatesMaths::real_t new_angle = angle/2.;

		UnitQuaternion3D new_uq = UnitQuaternion3D::create_rotation(params.axis,new_angle);
		rotation = FiniteRotation::create(new_uq,axis_hint);
	}

	
	void
	fill_times_vector(
		std::vector<double> &times,
		double reconstruction_time,
		const std::vector<GPlatesPropertyValues::GpmlTimeSample> &time_samples)
	{
		times.push_back(reconstruction_time);

		std::vector<GPlatesPropertyValues::GpmlTimeSample>::const_iterator
			samples_iter = time_samples.begin(),
			samples_end = time_samples.end();

		// Get to the first time which is older than our current reconsruction time. 
		while ((samples_iter != samples_end) && (samples_iter->valid_time()->time_position().value() <= reconstruction_time))
		{
			++samples_iter;
		}	

		// Add the remaining times. 
		while (samples_iter != samples_end)
		{
			times.push_back(samples_iter->valid_time()->time_position().value());
			++samples_iter;
		}	
	}
	
	void
	add_lines_to_layer(
		GPlatesAppLogic::FlowlineSolver::lines_container_type &lines,
		GPlatesAppLogic::ReconstructionGeometryCollection *flowlines_collection,
		bool reverse_arrows
	)
	{
// This should now just add geonetries to the collection; rendering these geometries
// will be done elsewhere. 
#if 0
		// Loop over the vector of lists; for each list, create an arrowed polyline, and
		// add it to the layer.
		GPlatesAppLogic::FlowlineSolver::lines_container_type::iterator 
			it_lines = lines.begin(),
			end_lines = lines.end();



		for (; it_lines != end_lines ; ++it_lines)
		{
			//Make sure we have at least one point in the list before we try to create a polyline.
			if (it_lines->size() > 1)
			{				
				if (reverse_arrows)
				{
					// Reverse the points in the line so the arrows go in the direction of motion.
					it_lines->reverse();
				}

				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline =
					GPlatesMaths::PolylineOnSphere::create_on_heap(*it_lines);

				const GPlatesGui::Colour &colour = GPlatesGui::Colour::get_white();

				// Create a RenderedGeometry using the reconstructed geometry.
				const GPlatesViewOperations::RenderedGeometry rendered_geom =
					GPlatesViewOperations::create_rendered_arrowed_polyline(
					polyline,
					colour,
					0.05f,
					1.5);				
						

				// Add to the rendered layer.
				flowlines_layer->add_rendered_geometry(rendered_geom);

			}
		}	
#endif
	}
	

	void
	generate_symmetric_flowlines(
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type &anchor_plate,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> & 
			reconstruction_feature_collections,
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type seed_points,
		GPlatesModel::integer_plate_id_type &left_plate_id,
		GPlatesModel::integer_plate_id_type &right_plate_id,
		std::vector<GPlatesPropertyValues::GpmlTimeSample> &time_samples,
		GPlatesAppLogic::ReconstructionGeometryCollection *flowlines_collection)
	{
#if 0
		GPlatesModel::integer_plate_id_type reconstruction_anchor_plate_id = 
				reconstruction.reconstruction_tree().get_anchor_plate_id();
				
		// Get our current rotation 
		GPlatesModel::ReconstructionTree::non_null_ptr_type reconstruction_tree_current = 
			GPlatesAppLogic::ReconstructUtils::create_reconstruction_tree(
				reconstruction_time,
				reconstruction_anchor_plate_id,
				//*reconstruction_tree_cache,
				reconstruction_feature_collections);

			
		GPlatesModel::ReconstructionTree::non_null_ptr_type reconstruction_tree_creation = 
			GPlatesAppLogic::ReconstructUtils::create_reconstruction_tree(
				creation_time,
				reconstruction_anchor_plate_id,
				//*reconstruction_tree_cache,
				reconstruction_feature_collections);					

		// multi_point_on_sphere contains the present day coordinates of the seed point, based on its
		// moving plate.
		// For non-present-day creation times, this will not be the correct seed point; we'll need
		// to correct this later. 	
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type starting_points = 
			multi_point_on_sphere;
			
		starting_points = apply_seed_point_correction(starting_points,
			moving_plate_id,
			fixed_plate_id,
			reconstruction_tree_creation,
			reconstruction_tree_current);

		// And a blob in the middle. This blob will be queryable. 
		add_seed_points_to_layer_and_reconstruction(starting_points,flowlines_layer,reconstruction,prop_iter,moving_plate_id);

		// This will hold the times we need to use for stage poles, from the current reconstruction time to
		// the oldest time in the flowline.
		std::vector<double> times;

		fill_times_vector(times,reconstruction_time,time_samples);

		if (times.size () == 1)
		{
			// A size of 1 means we only have the reconstruction time. 
			qDebug() << "No times...";
			return;
		}	

		double earliest_time = times.back();
		double latest_time = times.front();	

		// If the current time is older than the oldest flowline time, or younger than the
		// youngest flowline time, do nothing.
		if ((reconstruction_time > earliest_time) || (reconstruction_time < latest_time))
		{
			qDebug() << "Reconstruction time is outwith range of flowline times. Doing nothing.";
			return;
		}		
		
		// The points which will be added to the list of points, which is used for
		// making the rendered geometry.
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type stored_points = starting_points;

		std::vector<std::list<GPlatesMaths::PointOnSphere> > upstream_lines;

		add_initial_points_to_lines_container(stored_points,upstream_lines);

		// This will store the stage poles which we'll use to form the "mirrored" part of a
		// symmetric flowline.
		std::vector<GPlatesMaths::FiniteRotation> rotations;

		// We'll work from the current time, backwards in time.		
		std::vector<double>::const_iterator 
			iter = times.begin(),
			end = times.end();

		++iter;	

		// current_points will be updated at each iteration.
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type current_points = starting_points;

		for (; iter != end ; ++iter)
		{

			GPlatesModel::ReconstructionTree::non_null_ptr_type reconstruction_tree_ptr = 
				GPlatesAppLogic::ReconstructUtils::create_reconstruction_tree(
				*iter,
				reconstruction_anchor_plate_id,
				reconstruction_feature_collections);
			//	*reconstruction_tree_cache);

			// The stage pole for the moving plate w.r.t. the fixed plate, from current time to t
			GPlatesMaths::FiniteRotation stage_pole =
				GPlatesAppLogic::FlowlineUtils::get_stage_pole(
					*reconstruction_tree_current,
					*reconstruction_tree_ptr,				
					moving_plate_id,
					fixed_plate_id);		

			// Halve the stage pole.
			get_half_angle_rotation(stage_pole);

			// Store this so we can use it on the way back.
			rotations.push_back(stage_pole);

			// Update our current coordinates.
			current_points = stage_pole*starting_points;
			
			stored_points = current_points;

			// Store these points in the lines container.
			add_subsequent_points_to_lines_container(stored_points,upstream_lines);
		}	

		stored_points = starting_points;

		std::vector<std::list<GPlatesMaths::PointOnSphere> > downstream_lines;		
		add_initial_points_to_lines_container(stored_points,downstream_lines);

		// Form the mirrored part of the flowline.
		std::vector<GPlatesMaths::FiniteRotation>::const_iterator 
			rot_iter = rotations.begin(),
			rot_end = rotations.end();			

		for (; rot_iter != rot_end ; ++rot_iter)
		{
			GPlatesMaths::FiniteRotation rot = *rot_iter;

			rot = get_reverse(rot);
			current_points = rot*starting_points;
			stored_points = current_points;

			add_subsequent_points_to_lines_container(stored_points,downstream_lines);
		}	


		// Draw line from centre towards fixed plate.
		add_lines_to_layer(upstream_lines,flowlines_layer,false /*reverse arrows */);
		static const QString upstream_string("upstreamSymmetricalFlowlines");
		add_lines_to_flowline_field_feature(upstream_lines,output_feature_handle,upstream_string);

		// Draw line from centre towards moving plate
		add_lines_to_layer(downstream_lines,flowlines_layer,false /*reverse arrows */);
		static const QString downstream_string("downstreamSymmetricalFlowlines");
		add_lines_to_flowline_field_feature(downstream_lines,output_feature_handle,downstream_string);		
#endif
	}		

	

	void
	display_flowline(
		const std::list<GPlatesMaths::PointOnSphere> line)
	{
		std::list<GPlatesMaths::PointOnSphere>::const_iterator
			iter = line.begin(),
			end = line.end();
			
		for (; iter != end ; ++iter)
		{
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);
			
			qDebug() << "Lat: " << llp.latitude() << ", lon: " << llp.longitude();
		}	
	}

#if 0
	void
	display_gpml_flowline_node(
		const GPlatesPropertyValues::GpmlFlowlineNode &gpml_flowline_node)
	{
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(
			*(gpml_flowline_node.node_point()->point()));
		qDebug() << "Node point: " << llp.latitude() << ", " << llp.longitude();		
		qDebug() << "Fixed plate id: " << gpml_flowline_node.fixed_plate_id()->value();
		qDebug() << "Moving plate id: " << gpml_flowline_node.moving_plate_id()->value();	

		std::vector<double>::const_iterator 
			iter = gpml_flowline_node.times().begin(),	
			end = gpml_flowline_node.times().end();


		for (int count = 0 ; iter != end ; ++iter, ++count)
		{
			qDebug() << "Time " << count << ": " << *iter;
		}		
	}
#endif
}



void
GPlatesAppLogic::FlowlineSolver::visit_feature_handle(
		GPlatesModel::FeatureHandle &feature_handle)
{

	if ( ! initialise_pre_feature_properties(feature_handle)) {
		return;
	}

	static const GPlatesModel::PropertyName flowline_name = 
		GPlatesModel::PropertyName::create_gpml("Flowline");

	// Bail out if we're not a flowline.
	if ( !(feature_handle.feature_type().get_name() == 
		GPlatesUtils::make_icu_string_from_qstring(QString("Flowline"))))
	{
		return;
	}
	
	qDebug() << "Found flowline";
	
	static const GPlatesModel::PropertyName left_plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("leftPlateId");

	const GPlatesPropertyValues::GpmlPlateId *left_plate_id = NULL;
	if (!GPlatesFeatureVisitors::get_property_value(
		feature_handle.reference(), left_plate_id_property_name, left_plate_id))
	{
		qDebug() << "No left plate id found... leaving flowlines calculations.";
		return;
	}
	
	static const GPlatesModel::PropertyName right_plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("rightPlateId");

	const GPlatesPropertyValues::GpmlPlateId *right_plate_id = NULL;
	if (!GPlatesFeatureVisitors::get_property_value(
		feature_handle.reference(), right_plate_id_property_name, right_plate_id))
	{
		qDebug() << "No right plate id found... leaving flowlines calculations.";
		return;
	}
	
	static const GPlatesModel::PropertyName times_property_name =
		GPlatesModel::PropertyName::create_gpml("times");
		
	const GPlatesPropertyValues::GpmlIrregularSampling *times = NULL;
	if (!GPlatesFeatureVisitors::get_property_value(
		feature_handle.reference(),times_property_name,times))
	{
		qDebug() << "No irregular sampling found... leaving flowlines calculations.";	
		return;
	}

		
#if 0	
	static const GPlatesModel::PropertyName node_points_property_name =
		GPlatesModel::PropertyName::create_gpml("nodePoints");
	
	const GPlatesPropertyValues::GmlMultiPoint *multi_point = NULL;
		
	if (!GPlatesFeatureVisitors::get_property_value(
		feature_handle.reference(),node_points_property_name,geometry_ptr))
	{
		qDebug() << "No multipoint found...leaving flowlines calculations.";
		return;
	}
#endif	

	// The following member variables could be set via visitor functions. 

	d_time_samples = times->time_samples();
	
	// Visit each of the properties in turn.
	visit_feature_properties(feature_handle);

	finalise_post_feature_properties(feature_handle);	

}

void
GPlatesAppLogic::FlowlineSolver::visit_gml_point(
	GPlatesPropertyValues::GmlPoint &gml_point)
{

	if (current_top_level_propname())
	{
		static const GPlatesModel::PropertyName flowline_node_property_name = GPlatesModel::PropertyName::create_gpml("seedPoints");
		GPlatesModel::PropertyName property_name = *current_top_level_propname();	
		qDebug() << GPlatesUtils::make_qstring_from_icu_string(property_name.get_name());
		if (property_name != flowline_node_property_name)
		{
			return;
		}
	}

	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point = gml_point.point();
	
	std::vector<GPlatesMaths::PointOnSphere> container_of_points;
	container_of_points.push_back(*point);
	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type seed_points = 
		GPlatesMaths::MultiPointOnSphere::create_on_heap(container_of_points.begin(),container_of_points.end());
		
	try{
#if 1
		generate_symmetric_flowlines(
			d_reconstruction_time,
			d_anchor_plate_id,
			d_reconstruction_feature_collections,
			seed_points,
			d_left_plate_id,
			d_right_plate_id,
			d_time_samples,
			d_flowlines_collection);
#endif
	}	
	catch(...)
	{
		qDebug() << "Caught exception in generate_flowline";
	}		
		
}

void
GPlatesAppLogic::FlowlineSolver::visit_gml_multi_point(
	GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{

	if (current_top_level_propname())
	{
		static const GPlatesModel::PropertyName flowline_node_property_name = GPlatesModel::PropertyName::create_gpml("seedPoints");
		GPlatesModel::PropertyName property_name = *current_top_level_propname();	
		qDebug() << GPlatesUtils::make_qstring_from_icu_string(property_name.get_name());
		if (property_name != flowline_node_property_name)
		{
			return;
		}
	}

	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere = gml_multi_point.multipoint();
	
	try{
#if 0
		generate_flowline(
			d_reconstruction,
			d_reconstruction_feature_collections,
			d_reconstruction_tree_cache,
			multi_point_on_sphere,
			d_reconstruction_plate_id_value,
			d_reference_plate_id_value,
			d_time_samples,
			d_is_symmetrical_flowline,
			d_creation_time,
			d_flowlines_layer,
			*current_top_level_propiter(),			
			d_output_feature_handle);
#endif
	}	
	catch(...)
	{
		qDebug() << "Caught exception in generate_flowline";
	}		
	
	
}

void
GPlatesAppLogic::FlowlineSolver::visit_gpml_constant_value(
	GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}

