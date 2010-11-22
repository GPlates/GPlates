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
	d_recon_time(
			GPlatesPropertyValues::GeoTimeInstant(
					reconstruction_geometry_collection.get_reconstruction_time())),
	d_flowline_property_finder()
{  }

bool
GPlatesAppLogic::FlowlineGeometryPopulator::initialise_pre_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{


	//Detect Flowline features and set the flag.
	DetectFlowlineFeatures detector;
	detector.visit_feature_handle(feature_handle);
	if(!detector.has_flowline_features())
	{
		return false;
	}

	d_flowline_property_finder.visit_feature(feature_handle.reference());


	if (!d_flowline_property_finder.can_process_flowline())
	{
		return false;
	}
	// This will hold the times we need to use for stage poles, from the current reconstruction time to
	// the oldest time in the flowline.
	std::vector<double> times;

	FlowlineUtils::fill_times_vector(
		times,
		d_reconstruction_tree->get_reconstruction_time(),
		d_flowline_property_finder.get_times());

	// We'll work from the current time, backwards in time.		
	std::vector<double>::const_iterator 
		iter = times.begin(),
		end = times.end();

	// Step forward beyond the current time
	++iter;	

	GPlatesModel::integer_plate_id_type anchor = d_reconstruction_tree->get_anchor_plate_id();


	for (; iter != end ; ++iter)
	{

		GPlatesAppLogic::ReconstructionTree::non_null_ptr_type tree_at_time_t_ptr = 
			GPlatesAppLogic::ReconstructUtils::create_reconstruction_tree(
				*iter,
				anchor,
				d_reconstruction_tree->get_reconstruction_features());

		// The stage pole for the moving plate w.r.t. the fixed plate, from current time to t
		GPlatesMaths::FiniteRotation stage_pole =
			GPlatesAppLogic::ReconstructUtils::get_stage_pole(
				*d_reconstruction_tree,
				*tree_at_time_t_ptr,				
				*d_flowline_property_finder.get_left_plate(),
				*d_flowline_property_finder.get_right_plate());		

		// Halve the stage pole, and store it.
		FlowlineUtils::get_half_angle_rotation(stage_pole);

		//FlowlineUtils::display_rotation(stage_pole);

		d_rotations.push_back(stage_pole);

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
		qDebug() << GPlatesUtils::make_qstring_from_icu_string(property_name.get_name());
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
	// Get the reconstructed seed point location.
	// FIXME: We're already doing this in the flowline calculation.
	boost::optional<GPlatesMaths::FiniteRotation> rotation =
		ReconstructUtils::get_half_stage_rotation(
		*d_reconstruction_tree,
		d_flowline_property_finder.get_left_plate().get(),
		d_flowline_property_finder.get_right_plate().get());

	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type reconstructed_seed_point = point.get_non_null_pointer();

	if (rotation)
	{
		reconstructed_seed_point = (*rotation) * reconstructed_seed_point;
	}

#if 0
	GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type seed_point_rfg =
		ReconstructedFeatureGeometry::create(
			d_reconstruction_tree,
			reconstructed_seed_point,
			*(current_top_level_propiter()->handle_weak_ref()),
			*(current_top_level_propiter()));

	d_reconstruction_geometry_collection.add_reconstruction_geometry(seed_point_rfg);
#endif

	std::vector<GPlatesMaths::PointOnSphere> upstream_flowline;

	FlowlineUtils::calculate_upstream_symmetric_flowline(
		point,
		d_flowline_property_finder,
		upstream_flowline,
		d_reconstruction_tree,
		d_rotations);

	std::vector<GPlatesMaths::PointOnSphere> downstream_flowline;

	FlowlineUtils::calculate_downstream_symmetric_flowline(
		point,
		d_flowline_property_finder,
		downstream_flowline,
		d_reconstruction_tree,
		d_rotations);

	try{
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type upstream_flowline_points = 
			GPlatesMaths::PolylineOnSphere::create_on_heap(upstream_flowline.begin(),upstream_flowline.end());

		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type downstream_flowline_points = 
			GPlatesMaths::PolylineOnSphere::create_on_heap(downstream_flowline.begin(),downstream_flowline.end());

		GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type rf_ptr =
			ReconstructedFlowline::create(
			d_reconstruction_tree,
			point.get_non_null_pointer(),
			upstream_flowline_points,
			downstream_flowline_points,
			*(current_top_level_propiter()->handle_weak_ref()),
			*(current_top_level_propiter()));

		d_reconstruction_geometry_collection.add_reconstruction_geometry(rf_ptr);

	}
	catch(...)
	{
		// We failed creating a flowline for whatever reason. 
	}

}
