/* $Id: ReconstructedFeatureGeometryPopulator.cc 9458 2010-08-23 03:33:53Z mchin $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2010-08-23 05:33:53 +0200 (ma, 23 aug 2010) $
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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

#include <boost/none.hpp>  // boost::none

#include <QDebug>

#include "FlowlineGeometryPopulator.h"
#include "FlowlineUtils.h"

#include "Reconstruction.h"
#include "ReconstructedFlowline.h"
#include "ReconstructionGeometryCollection.h"
#include "ReconstructionGeometryUtils.h"
#include "ReconstructionTree.h"
#include "ReconstructUtils.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"


#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlTimeInstant.h"

#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"

#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"

namespace
{
	void
	fill_times_vector(
		std::vector<double> &times,
		const double &reconstruction_time,
		const std::vector<double> &time_samples)
	{


		times.push_back(reconstruction_time);

		std::vector<double>::const_iterator
			samples_iter = time_samples.begin(),
			samples_end = time_samples.end();

		// Get to the first time which is older than our current reconstruction time. 
		while ((samples_iter != samples_end) && (*samples_iter <= reconstruction_time))
		{
			++samples_iter;
		}	

		// Add the remaining times. 
		while (samples_iter != samples_end)
		{
			times.push_back(*samples_iter);
			++samples_iter;
		}	
	}
}

GPlatesAppLogic::FlowlineGeometryPopulator::FlowlineGeometryPopulator(
		ReconstructionGeometryCollection &reconstruction_geometry_collection):
	d_reconstruction_geometry_collection(reconstruction_geometry_collection),
	d_reconstruction_tree(reconstruction_geometry_collection.reconstruction_tree()),
	d_recon_time(GPlatesPropertyValues::GeoTimeInstant(
	    reconstruction_geometry_collection.get_reconstruction_time())),
	d_flowline_property_finder(
	    reconstruction_geometry_collection.get_reconstruction_time())
{  }

bool
GPlatesAppLogic::FlowlineGeometryPopulator::initialise_pre_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
    d_left_rotations.clear();
    d_right_rotations.clear();
    d_left_seed_point_rotations.clear();
    d_right_seed_point_rotations.clear();

    //Detect Flowline features.
    FlowlineUtils::DetectFlowlineFeatures detector;
    detector.visit_feature(feature_handle.reference());
    if(!detector.has_flowline_features())
    {
		return false;
    }

    d_flowline_property_finder.visit_feature(feature_handle.reference());


    if (!d_flowline_property_finder.can_process_seed_point())
    {
		return false;
    }

    if (d_flowline_property_finder.can_process_flowline())
    {
		GPlatesModel::integer_plate_id_type anchor = d_reconstruction_tree->get_anchor_plate_id();
		double current_time = d_reconstruction_tree->get_reconstruction_time();
		std::vector<double> times = d_flowline_property_finder.get_times();

		std::vector<double>::const_iterator
			t_iter = times.begin(),
			t_prev_iter = times.begin();

		++t_iter;

		if ((current_time > times.back()) ||
			(current_time < times.front()))
		{
			return true;
		}

		FlowlineUtils::fill_seed_point_rotations(
			current_time,
			times,
			*d_flowline_property_finder.get_left_plate(),
			*d_flowline_property_finder.get_right_plate(),
			d_reconstruction_tree,
			d_left_seed_point_rotations);

		FlowlineUtils::fill_seed_point_rotations(
			current_time,
			times,
			*d_flowline_property_finder.get_right_plate(),
			*d_flowline_property_finder.get_left_plate(),
			d_reconstruction_tree,
			d_right_seed_point_rotations);

		// This will now hold the times we need to use for flowline rotations, from the current reconstruction time to
		// the oldest time in the flowline.
		times.clear();

		FlowlineUtils::fill_times_vector(
			times,
			current_time,
			d_flowline_property_finder.get_times());

		// We'll work from the current time, backwards in time.
		std::vector<double>::const_iterator
			iter = times.begin(),
			end = times.end();

		// Save the "previous" tree for use in the loop.
		GPlatesAppLogic::ReconstructionTree::non_null_ptr_type tree_at_prev_time_ptr =
			GPlatesAppLogic::ReconstructUtils::create_reconstruction_tree(
			*iter,
			anchor,
			d_reconstruction_tree->get_reconstruction_features());

		// Step forward beyond the current time
		++iter;

		for (; iter != end ; ++iter)
		{

			GPlatesAppLogic::ReconstructionTree::non_null_ptr_type tree_at_time_t_ptr =
				GPlatesAppLogic::ReconstructUtils::create_reconstruction_tree(
				*iter,
				anchor,
				d_reconstruction_tree->get_reconstruction_features());


			// The stage pole for the right plate w.r.t. the left plate
			GPlatesMaths::FiniteRotation stage_pole_left =
				GPlatesAppLogic::ReconstructUtils::get_stage_pole(
				*tree_at_prev_time_ptr,
				*tree_at_time_t_ptr,
				*d_flowline_property_finder.get_right_plate(),
				*d_flowline_property_finder.get_left_plate());


			GPlatesMaths::FiniteRotation stage_pole_right =
				GPlatesAppLogic::ReconstructUtils::get_stage_pole(
				*tree_at_prev_time_ptr,
				*tree_at_time_t_ptr,
				*d_flowline_property_finder.get_left_plate(),
				*d_flowline_property_finder.get_right_plate());

			// Halve the stage poles, and store them.
			FlowlineUtils::get_half_angle_rotation(stage_pole_left);
			FlowlineUtils::get_half_angle_rotation(stage_pole_right);

			d_left_rotations.push_back(stage_pole_left);
			d_right_rotations.push_back(stage_pole_right);

			tree_at_prev_time_ptr = tree_at_time_t_ptr;

		}
    }
    return true;

}



void
GPlatesAppLogic::FlowlineGeometryPopulator::visit_gml_multi_point(
		GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	if (current_top_level_propname())
	{
		static const GPlatesModel::PropertyName flowline_node_property_name = 
			GPlatesModel::PropertyName::create_gpml("seedPoints");
		GPlatesModel::PropertyName property_name = *current_top_level_propname();	

		if (property_name != flowline_node_property_name)
		{
			return;
		}
	}	

	GPlatesMaths::MultiPointOnSphere::const_iterator 
		iter = gml_multi_point.multipoint()->begin(),
		end = gml_multi_point.multipoint()->end();

	for (; iter != end ; ++iter)
	{
		process_point(*iter);
	}
}




void
GPlatesAppLogic::FlowlineGeometryPopulator::visit_gml_point(
		GPlatesPropertyValues::GmlPoint &gml_point)
{
	if (current_top_level_propname())
	{
		static const GPlatesModel::PropertyName flowline_node_property_name = 
			GPlatesModel::PropertyName::create_gpml("seedPoints");
		GPlatesModel::PropertyName property_name = *current_top_level_propname();	
		if (property_name != flowline_node_property_name)
		{
			return;
		}
	}	

	process_point(*gml_point.point());
}



void
GPlatesAppLogic::FlowlineGeometryPopulator::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}



void
GPlatesAppLogic::FlowlineGeometryPopulator::finalise_post_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{

	
}


void
GPlatesAppLogic::FlowlineGeometryPopulator::process_point(
	const GPlatesMaths::PointOnSphere &point)
{
    if (d_flowline_property_finder.can_process_flowline())
    {
		process_seed_point_and_flowline(point);
    }
    else
    {
		process_seed_point_with_recon_plate_id(point);
    }
}

void
GPlatesAppLogic::FlowlineGeometryPopulator::process_seed_point_and_flowline(
	const GPlatesMaths::PointOnSphere &point)
{
	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type reconstructed_left_seed_point =
		FlowlineUtils::reconstruct_seed_point(
		    point.get_non_null_pointer(),
		    d_left_seed_point_rotations);

	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type reconstructed_right_seed_point =
		FlowlineUtils::reconstruct_seed_point(
		    point.get_non_null_pointer(),
		    d_right_seed_point_rotations);


	std::vector<GPlatesMaths::PointOnSphere> left_flowline;

	FlowlineUtils::calculate_flowline(
		reconstructed_left_seed_point,
		d_flowline_property_finder,
		left_flowline,
		d_reconstruction_tree,
		d_left_rotations);

	std::vector<GPlatesMaths::PointOnSphere> right_flowline;

	FlowlineUtils::calculate_flowline(
		reconstructed_right_seed_point,
		d_flowline_property_finder,
		right_flowline,
		d_reconstruction_tree,
		d_right_rotations);

	// We'll calculate the left and right flowlines in the frames of the left and right plates 
	// respectively. Afterwards we correct for the position of the left and right plates at our
	// current time. 
	GPlatesMaths::FiniteRotation left_correction =
		d_reconstruction_tree->get_composed_absolute_rotation(
		    d_flowline_property_finder.get_left_plate().get()).first;

	GPlatesMaths::FiniteRotation right_correction =
		d_reconstruction_tree->get_composed_absolute_rotation(
		    d_flowline_property_finder.get_right_plate().get()).first;

	GPlatesMaths::FiniteRotation seed_point_correction = left_correction;


	try{
	    // This seed_point could equally be calculated from the right-rotations, by the way.
	    GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type seed_point =
		    left_correction * reconstructed_left_seed_point;

	    GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type seed_point_rfg =
		    ReconstructedFeatureGeometry::create(
			d_reconstruction_tree,
			seed_point,
			*(current_top_level_propiter()->handle_weak_ref()),
			*(current_top_level_propiter()),
			d_flowline_property_finder.get_reconstruction_plate_id());

	    d_reconstruction_geometry_collection.add_reconstruction_geometry(seed_point_rfg);

	    GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type left_flowline_points =
		    GPlatesMaths::PolylineOnSphere::create_on_heap(left_flowline.begin(),left_flowline.end());

	    left_flowline_points = left_correction * left_flowline_points;

	    GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type right_flowline_points =
		    GPlatesMaths::PolylineOnSphere::create_on_heap(right_flowline.begin(),right_flowline.end());

	    right_flowline_points = right_correction * right_flowline_points;

	    GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type rf_ptr =
		    ReconstructedFlowline::create(
			d_reconstruction_tree,
			point.get_non_null_pointer(),
			left_flowline_points,
			right_flowline_points,
			*(current_top_level_propiter()->handle_weak_ref()),
			*(current_top_level_propiter()));

	    d_reconstruction_geometry_collection.add_reconstruction_geometry(rf_ptr);

	}
	catch(...)
	{
		// We failed creating a flowline for whatever reason. 
	}

}

void
GPlatesAppLogic::FlowlineGeometryPopulator::process_seed_point_with_recon_plate_id(
	const GPlatesMaths::PointOnSphere &point)
{
	// We end up here if no times are defined in the flowline, or if the 
	// recon-time is outwith the flowline time periods. 

	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type seed_point = point.get_non_null_pointer();

	if (d_flowline_property_finder.get_reconstruction_plate_id())
	{
		seed_point = d_reconstruction_tree->get_composed_absolute_rotation(
			    d_flowline_property_finder.get_reconstruction_plate_id().get()).first *
			    seed_point;
	}



	try{

	    GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type seed_point_rfg =
		    ReconstructedFeatureGeometry::create(
			d_reconstruction_tree,
			seed_point,
			*(current_top_level_propiter()->handle_weak_ref()),
			*(current_top_level_propiter()),
			d_flowline_property_finder.get_reconstruction_plate_id());

	    d_reconstruction_geometry_collection.add_reconstruction_geometry(seed_point_rfg);

	}
	catch(...)
	{
		// We failed creating a seed_point rfg for whatever reason.
	}

}
