/* $Id: ReconstructedFeatureGeometryPopulator.cc 9458 2010-08-23 03:33:53Z mchin $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010, 2011 Geological Survey of Norway
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


#include "MotionPathGeometryPopulator.h"

#include "MotionPathUtils.h"

#include "Reconstruction.h"
#include "ReconstructedFlowline.h"
#include "ReconstructionGeometryUtils.h"
#include "ReconstructionTree.h"
#include "ReconstructUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"

#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"

#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlTimeInstant.h"

#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"


GPlatesAppLogic::MotionPathGeometryPopulator::MotionPathGeometryPopulator(
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time) :
	d_reconstructed_feature_geometries(reconstructed_feature_geometries),
	d_reconstruction_tree_creator(reconstruction_tree_creator),
	d_recon_time(GPlatesPropertyValues::GeoTimeInstant(reconstruction_time)),
	d_motion_track_property_finder(
		new GPlatesAppLogic::MotionPathUtils::MotionPathPropertyFinder(reconstruction_time))
{  }

bool
GPlatesAppLogic::MotionPathGeometryPopulator::initialise_pre_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
	d_rotations.clear();

	//Detect MotionPath features and set the flag.
	MotionPathUtils::DetectMotionPathFeatures detector;
	detector.visit_feature_handle(feature_handle);
	if(!detector.has_motion_track_features())
	{
		return false;
	}

	d_motion_track_property_finder->visit_feature(feature_handle.reference());

	if (!d_motion_track_property_finder->can_process_seed_point())
	{
		return false;
	}

	if (d_motion_track_property_finder->can_process_motion_path())
	{


		// This will hold the times we need to use for stage poles, from the current reconstruction time to
		// the oldest time in the motion track.
		std::vector<double> times;

		MotionPathUtils::fill_times_vector(
			times,
			d_recon_time.value(),
			d_motion_track_property_finder->get_times());

		// We'll work from the current time, backwards in time.		
		std::vector<double>::const_iterator 
			iter = times.begin(),
			end = times.end();

		for (; iter != end ; ++iter)
		{

			GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type tree_at_time_t_ptr =
				d_reconstruction_tree_creator.get_reconstruction_tree(
						*iter,
						*d_motion_track_property_finder->get_relative_plate_id());

			GPlatesMaths::FiniteRotation rot = tree_at_time_t_ptr->get_composed_absolute_rotation(
				*d_motion_track_property_finder->get_reconstruction_plate_id());

			d_rotations.push_back(rot);
		}
	}
	return true;
}




void
GPlatesAppLogic::MotionPathGeometryPopulator::visit_gml_multi_point(
		GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	if (current_top_level_propname())
	{
		static const GPlatesModel::PropertyName motion_track_node_property_name = 
			GPlatesModel::PropertyName::create_gpml("seedPoints");
		GPlatesModel::PropertyName property_name = *current_top_level_propname();	

		if (property_name != motion_track_node_property_name)
		{
			return;
		}
	}	

	// The reconstruction tree for the current reconstruction time.
	ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
			d_reconstruction_tree_creator.get_reconstruction_tree(d_recon_time.value());

	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type reconstructed_seed_multipoint_geometry =
				reconstruction_tree->get_composed_absolute_rotation(
					d_motion_track_property_finder->get_reconstruction_plate_id().get()) *
					gml_multi_point.multipoint();

	try{

		GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type seed_point_rfg =
			ReconstructedFeatureGeometry::create(
				reconstruction_tree,
				d_reconstruction_tree_creator,
				*(current_top_level_propiter()->handle_weak_ref()),
				*(current_top_level_propiter()),
				reconstructed_seed_multipoint_geometry,
				ReconstructMethod::MOTION_PATH,
				d_motion_track_property_finder->get_reconstruction_plate_id());

		d_reconstructed_feature_geometries.push_back(seed_point_rfg);

	}
	catch (std::exception &exc)
	{
		qWarning() << exc.what();
	}
	catch(...)
	{
		// We failed creating a seed_point rfg for whatever reason.
		qWarning() << "GPlatesAppLogic::MotionPathGeometryPopulator::visit_gml_multi_point: Unknown error";
	}


	if (d_motion_track_property_finder->can_process_motion_path())
	{
		// Present day and reconstructed seed multipoints should have the same number of points.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				reconstructed_seed_multipoint_geometry->number_of_points() ==
					gml_multi_point.multipoint()->number_of_points(),
				GPLATES_ASSERTION_SOURCE);

		GPlatesMaths::MultiPointOnSphere::const_iterator 
			seed_multipoint_iter = gml_multi_point.multipoint()->begin(),
			seed_multipoint_end = gml_multi_point.multipoint()->end();
		GPlatesMaths::MultiPointOnSphere::const_iterator 
			reconstructed_seed_multipoint_iter = reconstructed_seed_multipoint_geometry->begin();

		for ( ;
			seed_multipoint_iter != seed_multipoint_end;
			++seed_multipoint_iter, ++reconstructed_seed_multipoint_iter)
		{
			create_motion_path_geometry(
					seed_multipoint_iter->get_non_null_pointer(),
					reconstructed_seed_multipoint_iter->get_non_null_pointer(),
					reconstructed_seed_multipoint_geometry);
		}
	}
}




void
GPlatesAppLogic::MotionPathGeometryPopulator::visit_gml_point(
		GPlatesPropertyValues::GmlPoint &gml_point)
{
	if (current_top_level_propname())
	{
		static const GPlatesModel::PropertyName motion_track_node_property_name = 
			GPlatesModel::PropertyName::create_gpml("seedPoints");
		GPlatesModel::PropertyName property_name = *current_top_level_propname();	

		if (property_name != motion_track_node_property_name)
		{
			return;
		}
	}	

	// The reconstruction tree for the current reconstruction time.
	ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
			d_reconstruction_tree_creator.get_reconstruction_tree(d_recon_time.value());

	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type reconstructed_seed_point_geometry =
		reconstruction_tree->get_composed_absolute_rotation(
			d_motion_track_property_finder->get_reconstruction_plate_id().get()) *
			gml_point.point();


	try{

		GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type seed_point_rfg =
			ReconstructedFeatureGeometry::create(
				reconstruction_tree,
				d_reconstruction_tree_creator,
				*(current_top_level_propiter()->handle_weak_ref()),
				*(current_top_level_propiter()),
				reconstructed_seed_point_geometry,
				ReconstructMethod::MOTION_PATH,
				d_motion_track_property_finder->get_reconstruction_plate_id());

		d_reconstructed_feature_geometries.push_back(seed_point_rfg);

	}
	catch (std::exception &exc)
	{
		qWarning() << exc.what();
	}
	catch(...)
	{
		// We failed creating a seed_point rfg for whatever reason.
		qWarning() << "GPlatesAppLogic::MotionPathGeometryPopulator::visit_gml_point: Unknown error";
	}

	if (d_motion_track_property_finder->can_process_motion_path())
	{
		create_motion_path_geometry(
				gml_point.point(),
				reconstructed_seed_point_geometry,
				reconstructed_seed_point_geometry);
	}

}



void
GPlatesAppLogic::MotionPathGeometryPopulator::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesAppLogic::MotionPathGeometryPopulator::create_motion_path_geometry(
	const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type &present_day_seed_point_geometry,
	const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type &reconstructed_seed_point_geometry,
	const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &reconstructed_seed_geometry)
{

	std::vector<GPlatesMaths::PointOnSphere> motion_track;

	MotionPathUtils::calculate_motion_track(
		present_day_seed_point_geometry,
		*d_motion_track_property_finder,
		motion_track,
		d_rotations);

	// NOTE: We no longer require the reconstruction time to be between the end points of the times vector.
	// This enables us to display/export at, for example, present day when the time vector does
	// not include present day (such as a motion path representing part of a hotspot trail).
	//
	// This also means it's possible to not have enough points to form a polyline.
	if (motion_track.size() < 2)
	{
		return;
	}

	// The reconstruction tree for the current reconstruction time.
	ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
			d_reconstruction_tree_creator.get_reconstruction_tree(d_recon_time.value());

	GPlatesMaths::FiniteRotation relative_plate_correction =
		reconstruction_tree->get_composed_absolute_rotation(
		d_motion_track_property_finder->get_relative_plate_id().get());

	try
	{
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type motion_track_points = 
			GPlatesMaths::PolylineOnSphere::create_on_heap(motion_track.begin(),motion_track.end());

		// Everything has been calculated in the frame of the relative_plate_id; now
		// we just correct for that plate's motion. 
		motion_track_points = relative_plate_correction * motion_track_points;

		GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type mtrg_ptr =
			ReconstructedMotionPath::create(
			reconstruction_tree,
			d_reconstruction_tree_creator,
			present_day_seed_point_geometry,
			reconstructed_seed_point_geometry,
			motion_track_points,
			*d_motion_track_property_finder->get_reconstruction_plate_id(),
			*(current_top_level_propiter()->handle_weak_ref()),
			*(current_top_level_propiter()),
			reconstructed_seed_geometry);

		d_reconstructed_feature_geometries.push_back(mtrg_ptr);

	}
	catch (const std::exception &exc)
	{
		qWarning() << exc.what();
	}
	catch(...)
	{
		// We failed creating a motion track for whatever reason. 
		qWarning() << "GPlatesAppLogic::MotionPathGeometryPopulator::create_motion_path_geometry: Unknown error";
	}

}
