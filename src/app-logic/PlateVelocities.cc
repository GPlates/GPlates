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

#include <boost/optional.hpp>

#include "PlateVelocities.h"

#include "AppLogicUtils.h"
#include "Reconstruct.h"

#include "feature-visitors/ComputationalMeshSolver.h"

#include "model/FeatureType.h"
#include "model/ModelUtils.h"
#include "model/PropertyName.h"

#include "property-values/GmlMultiPoint.h"


namespace
{
	/**
	 * Determines if any mesh node features that can be used by plate velocity calculations.
	 */
	class DetectVelocityMeshNodes:
			public GPlatesModel::FeatureVisitor
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
				GPlatesModel::FeatureHandle &feature_handle)
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
		}

	private:
		bool d_found_velocity_mesh_nodes;
	};


	/**
	 * For each feature of type "gpml:MeshNode" creates a new feature
	 * of type "gpml:VelocityField" and adds to a new feature collection.
	 */
	class AddVelocityFieldFeatures:
			public GPlatesModel::FeatureVisitor
	{
	public:
		AddVelocityFieldFeatures(
				GPlatesModel::FeatureCollectionHandle::weak_ref &velocity_field_feature_collection,
				GPlatesModel::ModelInterface &model) :
			d_velocity_field_feature_collection(velocity_field_feature_collection),
			d_model(model)
		{  }


		virtual
		void
		visit_feature_handle(
				GPlatesModel::FeatureHandle &feature_handle)
		{
			static const GPlatesModel::FeatureType mesh_node_feature_type = 
					GPlatesModel::FeatureType::create_gpml("MeshNode");

			if (feature_handle.feature_type() != mesh_node_feature_type)
			{
				return;
			}

			d_velocity_field_feature = create_velocity_field_feature();

			visit_feature_properties(feature_handle);
		}


		virtual
		void
		visit_feature_properties(
				GPlatesModel::FeatureHandle &feature_handle)
		{
			GPlatesModel::FeatureHandle::properties_iterator iter = feature_handle.properties_begin();
			GPlatesModel::FeatureHandle::properties_iterator end = feature_handle.properties_end();
			for ( ; iter != end; ++iter)
			{
				// Elements of this properties vector can be NULL pointers.  (See the comment in
				// "model/FeatureRevision.h" for more details.)
				if (*iter != NULL)
				{
					d_current_property_name = (*iter)->property_name();

					(*iter)->accept_visitor(*this);
				}
			}
		}


		virtual
		void
		visit_gml_multi_point(
				GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
		{
			static const GPlatesModel::PropertyName mesh_points_prop_name =
					GPlatesModel::PropertyName::create_gml("meshPoints");

			// Note: we can't get here without a valid property name but check
			// anyway in case visiting a property value directly (ie, not via a feature).
			if (d_current_property_name &&
				*d_current_property_name == mesh_points_prop_name)
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
		boost::optional<GPlatesModel::PropertyName> d_current_property_name;


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
				GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
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
GPlatesAppLogic::PlateVelocities::detect_velocity_mesh_nodes(
		GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
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


GPlatesModel::FeatureCollectionHandle::weak_ref
GPlatesAppLogic::PlateVelocities::create_velocity_field_feature_collection(
		GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_with_mesh_nodes,
		GPlatesModel::ModelInterface &model)
{
	if (!feature_collection_with_mesh_nodes.is_valid())
	{
		return GPlatesModel::FeatureCollectionHandle::weak_ref();
	}

	// Create a new feature collection to store our velocity field features.
	GPlatesModel::FeatureCollectionHandle::weak_ref velocity_field_feature_collection =
			model->create_feature_collection();

	// A visitor to look for mesh node features in the original feature collection
	// and create corresponding velocity field features in the new feature collection.
	AddVelocityFieldFeatures add_velocity_field_features(
			velocity_field_feature_collection, model);

	GPlatesAppLogic::AppLogicUtils::visit_feature_collection(
			feature_collection_with_mesh_nodes, add_velocity_field_features);

	// Return the newly created feature collection.
	return velocity_field_feature_collection;
}


void
GPlatesAppLogic::PlateVelocities::solve_velocities(
		GPlatesModel::FeatureCollectionHandle::weak_ref &velocity_field_feature_collection,
		GPlatesModel::ReconstructionTree &reconstruction_tree_1,
		GPlatesModel::ReconstructionTree &reconstruction_tree_2,
		const double &reconstruction_time_1,
		const double &reconstruction_time_2,
		GPlatesModel::integer_plate_id_type reconstruction_root,
		GPlatesFeatureVisitors::TopologyResolver &topology_resolver,
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type comp_mesh_layer)
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
			comp_mesh_layer,
			true); // keep features without recon plate id

	GPlatesAppLogic::AppLogicUtils::visit_feature_collection(
			velocity_field_feature_collection,
			velocity_solver);

	// velocity_solver.report();
}
