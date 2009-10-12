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
#include <boost/optional.hpp>

#include "PlateVelocities.h"

#include "AppLogicUtils.h"
#include "Reconstruct.h"

#include "feature-visitors/ComputationalMeshSolver.h"

#include "model/FeatureType.h"
#include "model/ConstFeatureVisitor.h"
#include "model/Model.h"
#include "model/ModelInterface.h"
#include "model/ModelUtils.h"
#include "model/PropertyName.h"
#include "model/ReconstructionTree.h"

#include "property-values/GmlMultiPoint.h"


namespace
{
	/**
	 * Determines if any mesh node features that can be used by plate velocity calculations.
	 */
	class DetectVelocityMeshNodes:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		DetectVelocityMeshNodes() :
			d_found_velocity_mesh_nodes(false)
		{  }


		bool
		has_velocity_mesh_node_features() const
		{
			return d_found_velocity_mesh_nodes;
		}


		virtual
		void
		visit_feature_handle(
				const GPlatesModel::FeatureHandle &feature_handle)
		{
			if (d_found_velocity_mesh_nodes)
			{
				// We've already found a mesh node feature so just return.
				// We're trying to see if any features in a feature collection have
				// a velocity mesh node.
				return;
			}

			static const GPlatesModel::FeatureType mesh_node_feature_type = 
					GPlatesModel::FeatureType::create_gpml("MeshNode");

			if (feature_handle.feature_type() == mesh_node_feature_type)
			{
				d_found_velocity_mesh_nodes = true;
			}

			// NOTE: We don't actually want to visit the feature's properties
			// so we're not calling 'visit_feature_properties()'.
		}

	private:
		bool d_found_velocity_mesh_nodes;
	};


	/**
	 * For each feature of type "gpml:MeshNode" creates a new feature
	 * of type "gpml:VelocityField" and adds to a new feature collection.
	 */
	class AddVelocityFieldFeatures:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		AddVelocityFieldFeatures(
				GPlatesModel::FeatureCollectionHandle::weak_ref &velocity_field_feature_collection,
				GPlatesModel::ModelInterface &model) :
			d_velocity_field_feature_collection(velocity_field_feature_collection),
			d_model(model)
		{  }


		virtual
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle)
		{
			static const GPlatesModel::FeatureType mesh_node_feature_type = 
					GPlatesModel::FeatureType::create_gpml("MeshNode");

			if (feature_handle.feature_type() != mesh_node_feature_type)
			{
				// Don't visit this feature.
				return false;
			}

			d_velocity_field_feature = create_velocity_field_feature();

			return true;
		}


		virtual
		void
		visit_gml_multi_point(
				const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
		{
			static const GPlatesModel::PropertyName mesh_points_prop_name =
					GPlatesModel::PropertyName::create_gpml("meshPoints");

			// Note: we can't get here without a valid property name but check
			// anyway in case visiting a property value directly (ie, not via a feature).
			if (current_top_level_propname() &&
				*current_top_level_propname() == mesh_points_prop_name)
			{
				// We only expect one "meshPoints" property per mesh node feature.
				// If there are multiple then we'll create multiple "domainSet" properties
				// and velocity solver will aggregate them all into a single "rangeSet"
				// property. Which means we'll have one large "rangeSet" property mapping
				// into multiple smaller "domainSet" properties and the mapping order
				// will be implementation defined.
				create_and_append_domain_set_property_to_velocity_field_feature(
						gml_multi_point);
			}
		}

	private:
		GPlatesModel::FeatureCollectionHandle::weak_ref &d_velocity_field_feature_collection;
		GPlatesModel::ModelInterface &d_model;
		GPlatesModel::FeatureHandle::weak_ref d_velocity_field_feature;


		const GPlatesModel::FeatureHandle::weak_ref
		create_velocity_field_feature()
		{
			static const GPlatesModel::FeatureType velocity_field_feature_type = 
					GPlatesModel::FeatureType::create_gpml("VelocityField");

			// Create a new velocity field feature adding it to the new collection.
			return d_model->create_feature(
					velocity_field_feature_type, d_velocity_field_feature_collection);
		}


		void
		create_and_append_domain_set_property_to_velocity_field_feature(
				const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
		{
			//
			// Create the "gml:domainSet" property of type GmlMultiPoint -
			// basically references "meshPoints" property in mesh node feature which
			// should be a GmlMultiPoint.
			//
			static const GPlatesModel::PropertyName domain_set_prop_name =
					GPlatesModel::PropertyName::create_gml("domainSet");

			GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type domain_set_gml_multi_point =
					GPlatesPropertyValues::GmlMultiPoint::create(
							gml_multi_point.multipoint());

			GPlatesModel::ModelUtils::append_property_value_to_feature(
					domain_set_gml_multi_point,
					domain_set_prop_name,
					d_velocity_field_feature);
		}
	};
}


bool
GPlatesAppLogic::PlateVelocityUtils::detect_velocity_mesh_nodes(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	if (!feature_collection.is_valid())
	{
		return false;
	}

	// Visitor to detect mesh node features in the feature collection.
	DetectVelocityMeshNodes detect_velocity_mesh_nodes;

	GPlatesAppLogic::AppLogicUtils::visit_feature_collection(
			feature_collection, detect_velocity_mesh_nodes);

	return detect_velocity_mesh_nodes.has_velocity_mesh_node_features();
}


GPlatesModel::FeatureCollectionHandleUnloader::shared_ref
GPlatesAppLogic::PlateVelocityUtils::create_velocity_field_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_with_mesh_nodes,
		GPlatesModel::ModelInterface &model)
{
	if (!feature_collection_with_mesh_nodes.is_valid())
	{
		return GPlatesModel::FeatureCollectionHandleUnloader::create(
				GPlatesModel::FeatureCollectionHandle::weak_ref());
	}

	// Create a new feature collection to store our velocity field features.
	GPlatesModel::FeatureCollectionHandle::weak_ref velocity_field_feature_collection =
			model->create_feature_collection();

	const GPlatesModel::FeatureCollectionHandleUnloader::shared_ref
			velocity_field_feature_collection_unloader =
					GPlatesModel::FeatureCollectionHandleUnloader::create(
							velocity_field_feature_collection);

	// A visitor to look for mesh node features in the original feature collection
	// and create corresponding velocity field features in the new feature collection.
	AddVelocityFieldFeatures add_velocity_field_features(
			velocity_field_feature_collection, model);

	GPlatesAppLogic::AppLogicUtils::visit_feature_collection(
			feature_collection_with_mesh_nodes, add_velocity_field_features);

	// Return the newly created feature collection.
	return velocity_field_feature_collection_unloader;
}


void
GPlatesAppLogic::PlateVelocityUtils::solve_velocities(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &velocity_field_feature_collection,
		GPlatesModel::ReconstructionTree &reconstruction_tree_1,
		GPlatesModel::ReconstructionTree &reconstruction_tree_2,
		const double &reconstruction_time_1,
		const double &reconstruction_time_2,
		GPlatesModel::integer_plate_id_type reconstruction_root,
		GPlatesFeatureVisitors::TopologyResolver &topology_resolver,
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type comp_mesh_point_layer,
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type comp_mesh_arrow_layer)
{
	// Visit the feature collections and fill computational meshes with 
	// nice juicy velocity data
	GPlatesFeatureVisitors::ComputationalMeshSolver velocity_solver( 
			reconstruction_time_1,
			reconstruction_time_2,
			reconstruction_root,
			reconstruction_tree_1,
			reconstruction_tree_2,
			topology_resolver,
			comp_mesh_point_layer,
			comp_mesh_arrow_layer,
			true); // keep features without recon plate id

	GPlatesAppLogic::AppLogicUtils::visit_feature_collection(
			velocity_field_feature_collection,
			velocity_solver);
}


bool
GPlatesAppLogic::PlateVelocities::add_file(
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
	d_velocity_field_feature_collection_infos.back().d_active = true;

	return true;
}


void
GPlatesAppLogic::PlateVelocities::remove_file(
		FeatureCollectionFileState::file_iterator file_iter)
{
	using boost::lambda::_1;
	using boost::lambda::bind;

	// Try removing it from the velocity feature collections.
	d_velocity_field_feature_collection_infos.erase(
			std::remove_if(
					d_velocity_field_feature_collection_infos.begin(),
					d_velocity_field_feature_collection_infos.end(),
					bind(&VelocityFieldFeatureCollectionInfo::d_file_iterator, _1) == file_iter),
			d_velocity_field_feature_collection_infos.end());
}


bool
GPlatesAppLogic::PlateVelocities::changed_file(
		FeatureCollectionFileState::file_iterator file_iter,
		GPlatesFileIO::File &old_file,
		const ClassifyFeatureCollection::classifications_type &new_classification)
{
	// Only interested in feature collections with velocity mesh nodes.
	return PlateVelocityUtils::detect_velocity_mesh_nodes(file_iter->get_feature_collection());
}


void
GPlatesAppLogic::PlateVelocities::set_file_active(
		FeatureCollectionFileState::file_iterator file_iter,
		bool activate)
{
	using boost::lambda::_1;
	using boost::lambda::bind;

	velocity_field_feature_collection_info_seq_type::iterator iter = std::find_if(
			d_velocity_field_feature_collection_infos.begin(),
			d_velocity_field_feature_collection_infos.end(),
			bind(&VelocityFieldFeatureCollectionInfo::d_file_iterator, _1) == file_iter);

	if (iter != d_velocity_field_feature_collection_infos.end())
	{
		iter->d_active = activate;
	}
}


void
GPlatesAppLogic::PlateVelocities::solve_velocities(
		GPlatesModel::Reconstruction &reconstruction,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type reconstruction_anchored_plate_id,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstruction_features_collection,
		GPlatesFeatureVisitors::TopologyResolver &topology_resolver)
{
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
