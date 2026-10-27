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

#include "ReconstructedFlowline.h"
#include "ReconstructionGeometryUtils.h"
#include "ReconstructionTree.h"
#include "RotationUtils.h"

#include "app-logic/GeometryUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"

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

GPlatesAppLogic::FlowlineGeometryPopulator::FlowlineGeometryPopulator(
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time) :
	d_reconstructed_feature_geometries(reconstructed_feature_geometries),
	d_reconstruction_tree_creator(reconstruction_tree_creator),
	d_recon_time(GPlatesPropertyValues::GeoTimeInstant(reconstruction_time)),
	d_flowline_property_finder(
		new GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder(reconstruction_time)),
	d_omit_seed_point_vertex(false)
{  }

bool
GPlatesAppLogic::FlowlineGeometryPopulator::initialise_pre_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
    d_left_rotations.clear();
    d_right_rotations.clear();
    d_left_seed_point_rotations.clear();
    d_right_seed_point_rotations.clear();
    d_left_to_right_plate_frame_rotation = boost::none;
    d_omit_seed_point_vertex = false;

    //Detect Flowline features.
    FlowlineUtils::DetectFlowlineFeatures detector;
    detector.visit_feature(feature_handle.reference());
    if(!detector.has_flowline_features())
    {
		return false;
    }

	d_flowline_property_finder->visit_feature(feature_handle.reference());


	if (!d_flowline_property_finder->can_process_seed_point())
    {
		return false;
    }

	if (d_flowline_property_finder->can_process_flowline())
    {
		// The reconstruction tree for the current reconstruction time.
		ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
				d_reconstruction_tree_creator.get_reconstruction_tree(d_recon_time.value());

		//GPlatesModel::integer_plate_id_type anchor = reconstruction_tree->get_anchor_plate_id();
		double current_time = reconstruction_tree->get_reconstruction_time();

		const std::vector<double> &times = d_flowline_property_finder->get_times();
		const GPlatesModel::integer_plate_id_type left_plate_id =
			d_flowline_property_finder->get_left_plate().get();
		const GPlatesModel::integer_plate_id_type right_plate_id =
			d_flowline_property_finder->get_right_plate().get();

		// The flowline spreads from its geometry import time (or its youngest time if it has none),
		// whatever its times, which only decide which vertices are drawn.
		const double spreading_start_time = d_flowline_property_finder->get_spreading_start_time();

		// The current time need not lie between the flowline's youngest and oldest times.
		// Younger, each half is built out from the seed point as usual, but starts at the youngest
		// time (the seed point vertex is omitted). Older, there are no flowline rotations, so
		// there's only a seed point (where the spreading centre is).
		FlowlineUtils::fill_seed_point_rotations(
			current_time,
			times,
			spreading_start_time,
			left_plate_id,
			right_plate_id,
			d_reconstruction_tree_creator,
			d_left_seed_point_rotations);

		FlowlineUtils::fill_seed_point_rotations(
			current_time,
			times,
			spreading_start_time,
			right_plate_id,
			left_plate_id,
			d_reconstruction_tree_creator,
			d_right_seed_point_rotations);

		// The seed point is stored in the left plate's frame, but the right-hand half of the
		// flowline is built up in the right plate's frame, so we'll need to move the seed point
		// between the two frames.
		d_left_to_right_plate_frame_rotation =
			FlowlineUtils::get_left_to_right_plate_frame_rotation(
				spreading_start_time,
				left_plate_id,
				right_plate_id,
				d_reconstruction_tree_creator);

		// The rotations that build each half out from the seed point, one per vertex older than the
		// current time.
		FlowlineUtils::fill_flowline_rotations(
			current_time,
			times,
			spreading_start_time,
			left_plate_id/*fixed*/,
			right_plate_id/*moving*/,
			d_reconstruction_tree_creator,
			d_left_rotations);

		FlowlineUtils::fill_flowline_rotations(
			current_time,
			times,
			spreading_start_time,
			right_plate_id/*fixed*/,
			left_plate_id/*moving*/,
			d_reconstruction_tree_creator,
			d_right_rotations);

		// Younger than the flowline's youngest time, the first vertex (the seed point) is not at one
		// of the flowline's times.
		d_omit_seed_point_vertex = current_time < times.front();
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

	if (d_flowline_property_finder->can_process_flowline())
	{
		reconstruct_flowline(gml_multi_point.get_multipoint());
	}
	else
	{
		reconstruct_seed_geometry_with_recon_plate_id(gml_multi_point.get_multipoint());
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

	if (d_flowline_property_finder->can_process_flowline())
	{
		reconstruct_flowline(gml_point.get_point().get_geometry_on_sphere());
	}
	else
	{
		reconstruct_seed_geometry_with_recon_plate_id(gml_point.get_point().get_geometry_on_sphere());
	}
		 
}



void
GPlatesAppLogic::FlowlineGeometryPopulator::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesAppLogic::FlowlineGeometryPopulator::reconstruct_seed_geometry_with_recon_plate_id(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &present_day_seed_geometry)
{
	// We end up here if no times are defined in the flowline, or if it has no left or right
	// plate ids. We don't have enough information
	// to do a proper flowline reconstruction, but we want to represent the seed point in some
	// way. We can reconsruct the seedpoint with the plate-id.

	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geom = present_day_seed_geometry;

	if (d_flowline_property_finder->get_reconstruction_plate_id())
	{
		geom = d_reconstruction_tree_creator.get_reconstruction_tree(d_recon_time.value())
				->get_composed_absolute_rotation(
						d_flowline_property_finder->get_reconstruction_plate_id().get()) *
			    geom;
	}

	create_seed_geometry(geom, d_flowline_property_finder->get_reconstruction_plate_id());
}


void
GPlatesAppLogic::FlowlineGeometryPopulator::create_seed_geometry(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &reconstructed_seed_geometry,
		const boost::optional<GPlatesModel::integer_plate_id_type> &plate_id)
{
	// The reconstruction tree for the current reconstruction time.
	ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
			d_reconstruction_tree_creator.get_reconstruction_tree(d_recon_time.value());

	try{

	    GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type seed_point_rfg =
		    ReconstructedFeatureGeometry::create(
				reconstruction_tree,
				d_reconstruction_tree_creator,
				*(current_top_level_propiter()->handle_weak_ref()),
				*(current_top_level_propiter()),
				reconstructed_seed_geometry,
				ReconstructMethod::FLOWLINE,
				plate_id);

	    d_reconstructed_feature_geometries.push_back(seed_point_rfg);

	}
	catch (std::exception &exc)
	{
		qWarning() << exc.what();
	}
	catch (...)
	{
		// We failed creating a seed_point rfg for whatever reason.
		qWarning() << "GPlatesAppLogic::FlowlineGeometryPopulator::create_seed_geometry: Unknown error";
	}

}


void
GPlatesAppLogic::FlowlineGeometryPopulator::reconstruct_flowline(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &present_day_seed_geometry)
{
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type reconstructed_seed_geometry =
		FlowlineUtils::reconstruct_flowline_seed_points(
			present_day_seed_geometry,
			d_recon_time.value(),
			d_reconstruction_tree_creator,
			(current_top_level_propiter()->handle_weak_ref()));

	// At or older than the flowline's oldest time (or with only one time) there are not enough
	// flowline rotations for a flowline (it would have only one vertex), just its seed point(s).
	// They're given the left plate's ID, since they're stored in its frame.
	if (d_left_rotations.size() < (d_omit_seed_point_vertex ? 2u : 1u))
	{
		create_seed_geometry(
				reconstructed_seed_geometry,
				d_flowline_property_finder->get_left_plate());
		return;
	}

	boost::optional<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type>
			present_day_seed_multipoint =
					GPlatesAppLogic::GeometryUtils::get_multi_point_on_sphere(*present_day_seed_geometry);
	if (present_day_seed_multipoint)
	{
		boost::optional<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type>
				reconstructed_seed_multipoint_geometry =
						GPlatesAppLogic::GeometryUtils::get_multi_point_on_sphere(
								*reconstructed_seed_geometry);
		// It should be a multipoint geometry with the same number of points.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				reconstructed_seed_multipoint_geometry &&
					reconstructed_seed_multipoint_geometry.get()->number_of_points() ==
						present_day_seed_multipoint.get()->number_of_points(),
				GPLATES_ASSERTION_SOURCE);

		GPlatesMaths::MultiPointOnSphere::const_iterator
			seed_multipoint_iter = present_day_seed_multipoint.get()->begin(),
			seed_multipoint_end = present_day_seed_multipoint.get()->end();
		GPlatesMaths::MultiPointOnSphere::const_iterator
			reconstructed_seed_multipoint_iter = reconstructed_seed_multipoint_geometry.get()->begin();

		for ( ;
			seed_multipoint_iter != seed_multipoint_end;
			++seed_multipoint_iter, ++reconstructed_seed_multipoint_iter)
		{
			create_flowline_geometry(
					*seed_multipoint_iter,
					*reconstructed_seed_multipoint_iter,
					reconstructed_seed_geometry);
		}
		return;
	}

	boost::optional<const GPlatesMaths::PointOnSphere &> present_day_seed_point =
			GPlatesAppLogic::GeometryUtils::get_point_on_sphere(*present_day_seed_geometry);
	boost::optional<const GPlatesMaths::PointOnSphere &> reconstructed_seed_point =
			GPlatesAppLogic::GeometryUtils::get_point_on_sphere(*reconstructed_seed_geometry);
	// It should be a point geometry.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			present_day_seed_point && reconstructed_seed_point,
			GPLATES_ASSERTION_SOURCE);

	create_flowline_geometry(
			present_day_seed_point.get(),
			reconstructed_seed_point.get(),
			reconstructed_seed_geometry);
}


void
GPlatesAppLogic::FlowlineGeometryPopulator::create_flowline_geometry(
		const GPlatesMaths::PointOnSphere &present_day_seed_point,
		const GPlatesMaths::PointOnSphere &reconstructed_seed_point,
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &reconstructed_seed_geometry)
{

		const GPlatesMaths::PointOnSphere reconstructed_left_seed_point =
			FlowlineUtils::reconstruct_seed_point(
			present_day_seed_point,
			d_left_seed_point_rotations);

		// The seed point is stored in the left plate's frame, so it can be used as-is to seed the
		// left-hand half of the flowline (above), but it must first be moved into the right plate's
		// frame to seed the right-hand half. Without this the two halves of the flowline start from
		// different places whenever the left and right plates do not coincide at the flowline's
		// spreading start time.
		const GPlatesMaths::PointOnSphere right_plate_seed_point =
			d_left_to_right_plate_frame_rotation
			? *d_left_to_right_plate_frame_rotation * present_day_seed_point
			: present_day_seed_point;

		const GPlatesMaths::PointOnSphere reconstructed_right_seed_point =
			FlowlineUtils::reconstruct_seed_point(
			right_plate_seed_point,
			d_right_seed_point_rotations);


		std::vector<GPlatesMaths::PointOnSphere> left_flowline;

		FlowlineUtils::calculate_flowline(
			reconstructed_left_seed_point,
			*d_flowline_property_finder,
			left_flowline,
			d_reconstruction_tree_creator,
			d_left_rotations);

		std::vector<GPlatesMaths::PointOnSphere> right_flowline;

		FlowlineUtils::calculate_flowline(
			reconstructed_right_seed_point,
			*d_flowline_property_finder,
			right_flowline,
			d_reconstruction_tree_creator,
			d_right_rotations);

		if (d_omit_seed_point_vertex)
		{
			left_flowline.erase(left_flowline.begin());
			right_flowline.erase(right_flowline.begin());
		}

		// The reconstruction tree for the current reconstruction time.
		ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
				d_reconstruction_tree_creator.get_reconstruction_tree(d_recon_time.value());

		// We'll calculate the left and right flowlines in the frames of the left and right plates 
		// respectively. Afterwards we correct for the position of the left and right plates at our
		// current time. 
		GPlatesMaths::FiniteRotation left_correction =
			reconstruction_tree->get_composed_absolute_rotation(
			d_flowline_property_finder->get_left_plate().get());

		GPlatesMaths::FiniteRotation right_correction =
			reconstruction_tree->get_composed_absolute_rotation(
			d_flowline_property_finder->get_right_plate().get());


		try{

			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type left_flowline_points =
				GPlatesMaths::PolylineOnSphere::create(left_flowline.begin(),left_flowline.end());

			left_flowline_points = left_correction * left_flowline_points;

			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type right_flowline_points =
				GPlatesMaths::PolylineOnSphere::create(right_flowline.begin(),right_flowline.end());

			right_flowline_points = right_correction * right_flowline_points;

			GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type rf_ptr =
				ReconstructedFlowline::create(
				reconstruction_tree,
				d_reconstruction_tree_creator,
				present_day_seed_point,
				reconstructed_seed_point,
				left_flowline_points,
				right_flowline_points,
				*d_flowline_property_finder->get_left_plate(),
				*d_flowline_property_finder->get_right_plate(),
				d_flowline_property_finder->get_times(),
				d_flowline_property_finder->get_spreading_start_time(),
				*(current_top_level_propiter()->handle_weak_ref()),
				*(current_top_level_propiter()),
				reconstructed_seed_geometry);

		    d_reconstructed_feature_geometries.push_back(rf_ptr);

		}
		catch (std::exception &exc)
		{
			qWarning() << exc.what();
		}
		catch(...)
		{
			// We failed creating a flowline for whatever reason. 
			qWarning() << "GPlatesAppLogic::FlowlineGeometryPopulator::create_flowline_geometry: Unknown error";
		}

}



