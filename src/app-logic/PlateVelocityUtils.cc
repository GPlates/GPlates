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

#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>
#include <boost/lambda/lambda.hpp>

#include "PlateVelocityUtils.h"

#include "AppLogicUtils.h"
#include "Reconstruct.h"
#include "TopologyUtils.h"

#include "feature-visitors/ComputationalMeshSolver.h"
#include "feature-visitors/PropertyValueFinder.h"

#include "maths/CalculateVelocity.h"

#include "model/FeatureType.h"
#include "model/FeatureVisitor.h"
#include "model/Model.h"
#include "model/ModelUtils.h"
#include "model/PropertyName.h"
#include "model/ReconstructionTree.h"
#include "model/ResolvedTopologicalNetwork.h"

#include "property-values/GmlMultiPoint.h"
#include "property-values/GpmlPlateId.h"


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
			return GPlatesModel::FeatureHandle::create(
					d_velocity_field_feature_collection,
					velocity_field_feature_type);
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

			d_velocity_field_feature->add(
					GPlatesModel::TopLevelPropertyInline::create(
						domain_set_prop_name,
						domain_set_gml_multi_point));
		}
	};


	/**
	 * Information for calculating velocities - information that is not
	 * on a per-feature basis.
	 */
	class InfoForVelocityCalculations
	{
	public:
		InfoForVelocityCalculations(
				const GPlatesModel::ReconstructionTree &reconstruction_tree_1_,
				const GPlatesModel::ReconstructionTree &reconstruction_tree_2_) :
			reconstruction_tree_1(&reconstruction_tree_1_),
			reconstruction_tree_2(&reconstruction_tree_2_)
		{  }

		const GPlatesModel::ReconstructionTree *reconstruction_tree_1;
		const GPlatesModel::ReconstructionTree *reconstruction_tree_2;
	};


	/**
	 * A function to calculate velocity colat/lon - the signature of this function
	 * matches that required by TopologyUtils::interpolate_resolved_topology_networks().
	 *
	 * FIXME: This needs to be optimised - currently there's alot of common calculations
	 * within a feature that are repeated for each point - these calculations can be cached
	 * and looked up.
	 */
	std::vector<double>
	calc_velocity_vector_for_interpolation(
			const GPlatesMaths::PointOnSphere &point,
			const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref,
			const boost::any &user_data)
	{
		const InfoForVelocityCalculations &velocity_calc_info =
				boost::any_cast<const InfoForVelocityCalculations &>(user_data);

		// Get the reconstructionPlateId property value for the feature.
		static const GPlatesModel::PropertyName recon_plate_id_property_name =
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		GPlatesModel::integer_plate_id_type recon_plate_id_value = 0;
		const GPlatesPropertyValues::GpmlPlateId *recon_plate_id = NULL;
		if ( GPlatesFeatureVisitors::get_property_value(
				feature_ref, recon_plate_id_property_name, recon_plate_id ) )
		{
			// The feature has a reconstruction plate ID.
			recon_plate_id_value = recon_plate_id->value();
		}

		const GPlatesMaths::VectorColatitudeLongitude velocity_colat_lon = 
				GPlatesAppLogic::PlateVelocityUtils::calc_velocity_colat_lon(
						point,
						*velocity_calc_info.reconstruction_tree_1,
						*velocity_calc_info.reconstruction_tree_2,
						recon_plate_id_value);

		return GPlatesAppLogic::PlateVelocityUtils::convert_velocity_colatitude_longitude_to_scalars(
				velocity_colat_lon);
	}
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
			GPlatesModel::FeatureCollectionHandle::create(model->root());

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
		GPlatesModel::Reconstruction &reconstruction,
		GPlatesModel::ReconstructionTree &reconstruction_tree_1,
		GPlatesModel::ReconstructionTree &reconstruction_tree_2,
		const double &reconstruction_time_1,
		const double &reconstruction_time_2,
		GPlatesModel::integer_plate_id_type reconstruction_root,
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type comp_mesh_point_layer,
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type comp_mesh_arrow_layer)
{
	// Get the resolved topological boundaries in the reconstruction and optimise them
	// for point-in-polygon tests.
	// Later the velocity solver will query the boundaries at various points and
	// then calculate velocities using the best matching boundary.
	const TopologyUtils::resolved_boundaries_for_geometry_partitioning_query_type resolved_boundaries_query =
			TopologyUtils::query_resolved_topologies_for_geometry_partitioning(reconstruction);

	// Get the resolved topological networks in the reconstruction.
	// This effectively goes through all the node points in all topological networks
	// and generates velocity data for them.
	// Later the velocity solver will query the networks for the interpolated velocity
	// at various points.

	const InfoForVelocityCalculations velocity_calc_info(
			reconstruction_tree_1, reconstruction_tree_2);

	/*
	 * The following callback function will take a ResolvedTopologicalNetworkImpl pointer
	 * as its first argument and a TopologyUtils::network_interpolation_query_callback_type
	 * as its second argument. It will first construct a TopologicalNetworkVelocities object
	 * using the second argument and then pass that as an argument to
	 * ResolvedTopologicalNetworkImpl::set_network_velocities().
	 */
	const TopologyUtils::network_interpolation_query_callback_type callback_function =
			boost::lambda::bind(
					// The member function that this callback will delegate to
					&GPlatesModel::ResolvedTopologicalNetworkImpl::set_network_velocities,
					boost::lambda::_1/*the ResolvedTopologicalNetworkImpl object to set it on*/,
					boost::lambda::bind(
							// Construct a TopologicalNetworkVelocities object from the query
							boost::lambda::constructor<TopologicalNetworkVelocities>(),
							boost::lambda::_2/*the query to store in the network*/));

	// Fill the topological network with velocities data at all the network points.
	const TopologyUtils::resolved_networks_for_interpolation_query_type resolved_networks_query =
			TopologyUtils::query_resolved_topology_networks_for_interpolation(
					reconstruction,
					&calc_velocity_vector_for_interpolation,
					// Two scalars per velocity...
					NUM_TOPOLOGICAL_NETWORK_VELOCITY_COMPONENTS,
					// Info used by "calc_velocity_vector_for_interpolation()"...
					boost::any(velocity_calc_info),
					callback_function);


	// Visit the feature collections and fill computational meshes with 
	// nice juicy velocity data
	GPlatesFeatureVisitors::ComputationalMeshSolver velocity_solver( 
			reconstruction,
			reconstruction_time_1,
			reconstruction_root,
			reconstruction_tree_1,
			reconstruction_tree_2,
			resolved_boundaries_query,
			resolved_networks_query,
			comp_mesh_point_layer,
			comp_mesh_arrow_layer,
			true); // keep features without recon plate id

	GPlatesAppLogic::AppLogicUtils::visit_feature_collection(
			velocity_field_feature_collection,
			static_cast<GPlatesModel::FeatureVisitor &>(velocity_solver));
}


GPlatesMaths::Vector3D
GPlatesAppLogic::PlateVelocityUtils::calc_velocity_vector(
		const GPlatesMaths::PointOnSphere &point,
		const GPlatesModel::ReconstructionTree &reconstruction_tree1,
		const GPlatesModel::ReconstructionTree &reconstruction_tree2,
		const GPlatesModel::integer_plate_id_type &reconstruction_plate_id)
{
	// Get the finite rotation for this plate id.
	const GPlatesMaths::FiniteRotation &fr_t1 =
			reconstruction_tree1.get_composed_absolute_rotation(reconstruction_plate_id).first;

	const GPlatesMaths::FiniteRotation &fr_t2 =
			reconstruction_tree2.get_composed_absolute_rotation(reconstruction_plate_id).first;

	return GPlatesMaths::calculate_velocity_vector(point, fr_t1, fr_t2);
}


GPlatesMaths::VectorColatitudeLongitude
GPlatesAppLogic::PlateVelocityUtils::calc_velocity_colat_lon(
		const GPlatesMaths::PointOnSphere &point,
		const GPlatesModel::ReconstructionTree &reconstruction_tree1,
		const GPlatesModel::ReconstructionTree &reconstruction_tree2,
		const GPlatesModel::integer_plate_id_type &reconstruction_plate_id)
{
	const GPlatesMaths::Vector3D vector_xyz = calc_velocity_vector(
			point, reconstruction_tree1, reconstruction_tree2, reconstruction_plate_id);

	return GPlatesMaths::convert_vector_from_xyz_to_colat_lon(point, vector_xyz);
}


boost::optional<GPlatesMaths::VectorColatitudeLongitude>
GPlatesAppLogic::PlateVelocityUtils::TopologicalNetworkVelocities::interpolate_velocity(
		const GPlatesMaths::PointOnSphere &point)
{
	const boost::optional< std::vector<double> > interpolated_velocity =
			TopologyUtils::interpolate_resolved_topology_network(
					d_network_velocities_query, point);

	if (!interpolated_velocity)
	{
		return boost::none;
	}

	return convert_velocity_scalars_to_colatitude_longitude(*interpolated_velocity);
}


void
GPlatesAppLogic::PlateVelocityUtils::TopologicalNetworkVelocities::get_network_velocities(
		std::vector<GPlatesMaths::PointOnSphere> &network_points,
		std::vector<GPlatesMaths::VectorColatitudeLongitude> &network_velocities) const
{
	// Get the network velocities as scalar tuples.
	std::vector< std::vector<double> > network_scalar_tuple_sequence;
	TopologyUtils::get_resolved_topology_network_scalars(
			d_network_velocities_query,
			network_points,
			network_scalar_tuple_sequence);

	// Assemble the scalar tuples as velocity vectors.

	network_velocities.reserve(network_scalar_tuple_sequence.size());

	// Iterate through the scalar tuple sequence and convert to velocity colat/lon.
	typedef std::vector< std::vector<double> > scalar_tuple_seq_type;
	scalar_tuple_seq_type::const_iterator scalar_tuple_iter = network_scalar_tuple_sequence.begin();
	scalar_tuple_seq_type::const_iterator scalar_tuple_end = network_scalar_tuple_sequence.end();
	for ( ; scalar_tuple_iter != scalar_tuple_end; ++scalar_tuple_iter)
	{
		const std::vector<double> &scalar_tuple = *scalar_tuple_iter;

		network_velocities.push_back(
				convert_velocity_scalars_to_colatitude_longitude(scalar_tuple));
	}
}
