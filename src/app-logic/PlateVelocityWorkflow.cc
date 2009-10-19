/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <algorithm>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "PlateVelocityUtils.h"
#include "PlateVelocityWorkflow.h"
#include "ReconstructUtils.h"

#include "model/ModelInterface.h"
#include "model/ReconstructionTree.h"


bool
GPlatesAppLogic::PlateVelocityWorkflow::add_file(
		FeatureCollectionFileState::file_iterator file_iter,
		const ClassifyFeatureCollection::classifications_type &classification,
		bool /*used_by_higher_priority_workflow*/)
{
	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
			file_iter->get_feature_collection();

	// Only interested in feature collections with velocity mesh nodes.
	if (!PlateVelocityUtils::detect_velocity_mesh_nodes(feature_collection))
	{
		return false;
	}

	// Create a new feature collection with velocity field features that the
	// velocity solver can use for its calculations.
	const GPlatesModel::FeatureCollectionHandleUnloader::shared_ref velocity_field_feature_collection =
			PlateVelocityUtils::create_velocity_field_feature_collection(
					feature_collection, d_model);

	// Add to our list of velocity field feature collections.
	d_velocity_field_feature_collection_infos.push_back(
			VelocityFieldFeatureCollectionInfo(
					file_iter,
					velocity_field_feature_collection));

	return true;
}


void
GPlatesAppLogic::PlateVelocityWorkflow::remove_file(
		FeatureCollectionFileState::file_iterator file_iter)
{
	using boost::lambda::_1;

	// Try removing it from the velocity feature collections.
	d_velocity_field_feature_collection_infos.erase(
			std::remove_if(
					d_velocity_field_feature_collection_infos.begin(),
					d_velocity_field_feature_collection_infos.end(),
					boost::lambda::bind(&VelocityFieldFeatureCollectionInfo::d_file_iterator, _1)
							== file_iter),
			d_velocity_field_feature_collection_infos.end());
}


bool
GPlatesAppLogic::PlateVelocityWorkflow::changed_file(
		FeatureCollectionFileState::file_iterator file_iter,
		GPlatesFileIO::File &old_file,
		const ClassifyFeatureCollection::classifications_type &new_classification)
{
	// Only interested in feature collections with velocity mesh nodes.
	return PlateVelocityUtils::detect_velocity_mesh_nodes(file_iter->get_feature_collection());
}


void
GPlatesAppLogic::PlateVelocityWorkflow::set_file_active(
		FeatureCollectionFileState::file_iterator file_iter,
		bool activate)
{
	using boost::lambda::_1;

	velocity_field_feature_collection_info_seq_type::iterator iter = std::find_if(
			d_velocity_field_feature_collection_infos.begin(),
			d_velocity_field_feature_collection_infos.end(),
			boost::lambda::bind(&VelocityFieldFeatureCollectionInfo::d_file_iterator, _1)
					== file_iter);

	if (iter != d_velocity_field_feature_collection_infos.end())
	{
		iter->d_active = activate;
	}
}


void
GPlatesAppLogic::PlateVelocityWorkflow::solve_velocities(
		GPlatesModel::Reconstruction &reconstruction,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type reconstruction_anchored_plate_id,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstruction_features_collection,
		GPlatesFeatureVisitors::TopologyResolver &topology_resolver)
{
	/*
	 * FIXME: Presentation code should not be in here (this is app logic code).
	 * Remove any rendered geometry code to the presentation tier.
	 */
	// Activate the comp_mesh_point_layer.
	d_comp_mesh_point_layer->set_active();
	// Clear all RenderedGeometry's before adding new ones.
	d_comp_mesh_point_layer->clear_rendered_geometries();
	// Activate the comp_mesh_arrow_layer.
	d_comp_mesh_arrow_layer->set_active();
	// Clear all RenderedGeometry's before adding new ones.
	d_comp_mesh_arrow_layer->clear_rendered_geometries();

	// Return if there's no velocity feature collections to solve.
	if (d_velocity_field_feature_collection_infos.empty())
	{
		return;
	}

	// FIXME: should this '1' should be user controllable?
	const double &reconstruction_time_1 = reconstruction_time;
	const double reconstruction_time_2 = reconstruction_time_1 + 1;

	//
	// Create a second reconstruction tree for velocity calculations.
	//
	GPlatesModel::ReconstructionTree::non_null_ptr_type reconstruction_tree_2_ptr = 
			GPlatesAppLogic::ReconstructUtils::create_reconstruction_tree(
					reconstruction_features_collection,
					reconstruction_time_2,
					reconstruction_anchored_plate_id);

	// Our two reconstruction trees.
	GPlatesModel::ReconstructionTree &reconstruction_tree_1 =
			reconstruction.reconstruction_tree();
	GPlatesModel::ReconstructionTree &reconstruction_tree_2 =
			*reconstruction_tree_2_ptr;


	// Iterate over all our velocity field feature collections and solve velocities.
	velocity_field_feature_collection_info_seq_type::iterator velocity_field_feature_collection_iter;
	for (velocity_field_feature_collection_iter = d_velocity_field_feature_collection_infos.begin();
		velocity_field_feature_collection_iter != d_velocity_field_feature_collection_infos.end();
		++velocity_field_feature_collection_iter)
	{
		// Only interested in active files.
		if (!velocity_field_feature_collection_iter->d_active)
		{
			continue;
		}

		const GPlatesModel::FeatureCollectionHandle::weak_ref velocity_field_feature_collection =
				velocity_field_feature_collection_iter->d_velocity_field_feature_collection
						->get_feature_collection();


		PlateVelocityUtils::solve_velocities(
			velocity_field_feature_collection,
			reconstruction_tree_1,
			reconstruction_tree_2,
			reconstruction_time_1,
			reconstruction_time_2,
			reconstruction_anchored_plate_id,
			topology_resolver,
			d_comp_mesh_point_layer,
			d_comp_mesh_arrow_layer);
	}
}
