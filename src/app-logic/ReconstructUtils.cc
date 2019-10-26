/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <map>
#include <QDebug>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include "ReconstructUtils.h"

#include "AppLogicUtils.h"
#include "ReconstructContext.h"
#include "ReconstructionTreePopulator.h"
#include "SmallCircleGeometryPopulator.h"

#include "maths/ConstGeometryOnSphereVisitor.h"

#include "model/types.h"


bool
GPlatesAppLogic::ReconstructUtils::is_reconstruction_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref)
{
	return ReconstructionTreePopulator::can_process(feature_ref);
}


bool
GPlatesAppLogic::ReconstructUtils::has_reconstruction_features(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_iter = feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator feature_collection_end = feature_collection->end();
	for ( ; feature_collection_iter != feature_collection_end; ++feature_collection_iter)
	{
		const GPlatesModel::FeatureHandle::const_weak_ref feature_ref =
				(*feature_collection_iter)->reference();

		if (is_reconstruction_feature(feature_ref))
		{
			return true; 
		}
	}

	return false;
}


bool
GPlatesAppLogic::ReconstructUtils::is_reconstructable_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref,
		const ReconstructMethodRegistry &reconstruct_method_registry)
{
	// See if any reconstruct methods can reconstruct the current feature.
	return reconstruct_method_registry.can_reconstruct_feature(feature_ref);
}


bool
GPlatesAppLogic::ReconstructUtils::has_reconstructable_features(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
		const ReconstructMethodRegistry &reconstruct_method_registry)
{
	// Iterate over the features in the feature collection.
	GPlatesModel::FeatureCollectionHandle::const_iterator features_iter = feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator features_end = feature_collection->end();
	for ( ; features_iter != features_end; ++features_iter)
	{
		const GPlatesModel::FeatureHandle::const_weak_ref feature_ref = (*features_iter)->reference();

		// See if any reconstruct methods can reconstruct the current feature.
		if (reconstruct_method_registry.can_reconstruct_feature(feature_ref))
		{
			// Only need to be able to process one feature to be able to process the entire collection.
			return true;
		}
	}

	return false;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructUtils::reconstruct(
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
		const double &reconstruction_time,
		const ReconstructMethodRegistry &reconstruct_method_registry,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const ReconstructParams &reconstruct_params)
{
	std::vector<ReconstructContext::ReconstructedFeature> reconstructed_features;
	const ReconstructHandle::type reconstruct_handle = reconstruct(
			reconstructed_features,
			reconstruction_time,
			reconstruct_method_registry,
			reconstructable_features_collection,
			reconstruction_tree_creator,
			reconstruct_params);

	// Copy the RFGs in the ReconstructContext::ReconstructedFeature's.
	// The ReconstructContext::ReconstructedFeature's store RFGs and geometry property handles.
	// This format only needs the RFG.
	BOOST_FOREACH(const ReconstructContext::ReconstructedFeature &reconstructed_feature, reconstructed_features)
	{
		const ReconstructContext::ReconstructedFeature::reconstruction_seq_type &reconstructed_feature_reconstructions =
				reconstructed_feature.get_reconstructions();
		BOOST_FOREACH(const ReconstructContext::Reconstruction &reconstruction, reconstructed_feature_reconstructions)
		{
			reconstructed_feature_geometries.push_back(
					reconstruction.get_reconstructed_feature_geometry());
		}
	}

	return reconstruct_handle;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructUtils::reconstruct(
		std::vector<ReconstructContext::Reconstruction> &reconstructions,
		const double &reconstruction_time,
		const ReconstructMethodRegistry &reconstruct_method_registry,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const ReconstructParams &reconstruct_params)
{
	std::vector<ReconstructContext::ReconstructedFeature> reconstructed_features;
	const ReconstructHandle::type reconstruct_handle = reconstruct(
			reconstructed_features,
			reconstruction_time,
			reconstruct_method_registry,
			reconstructable_features_collection,
			reconstruction_tree_creator,
			reconstruct_params);

	// Copy the ReconstructContext::Reconstruction's in the ReconstructContext::ReconstructedFeature's.
	BOOST_FOREACH(const ReconstructContext::ReconstructedFeature &reconstructed_feature, reconstructed_features)
	{
		const ReconstructContext::ReconstructedFeature::reconstruction_seq_type &reconstructed_feature_reconstructions =
				reconstructed_feature.get_reconstructions();
		BOOST_FOREACH(const ReconstructContext::Reconstruction &reconstruction, reconstructed_feature_reconstructions)
		{
			reconstructions.push_back(reconstruction);
		}
	}

	return reconstruct_handle;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructUtils::reconstruct(
		std::vector<ReconstructContext::ReconstructedFeature> &reconstructed_features,
		const double &reconstruction_time,
		const ReconstructMethodRegistry &reconstruct_method_registry,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const ReconstructParams &reconstruct_params)
{
	// Create a reconstruct context - it will determine which reconstruct method each
	// reconstructable feature requires.
	ReconstructContext reconstruct_context(reconstruct_method_registry);
	reconstruct_context.set_features(reconstructable_features_collection);

	// Create the context state in which to reconstruct.
	const ReconstructMethodInterface::Context reconstruct_method_context(
			reconstruct_params,
			reconstruction_tree_creator);
	const ReconstructContext::context_state_reference_type context_state =
			reconstruct_context.create_context_state(reconstruct_method_context);

	// Reconstruct the reconstructable features.
	return reconstruct_context.get_reconstructed_features(
			reconstructed_features,
			context_state,
			reconstruction_time);
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructUtils::reconstruct(
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
		const ReconstructParams &reconstruct_params,
		unsigned int reconstruction_tree_cache_size)
{
	std::vector<ReconstructContext::ReconstructedFeature> reconstructed_features;
	const ReconstructHandle::type reconstruct_handle = reconstruct(
			reconstructed_features,
			reconstruction_time,
			anchor_plate_id,
			reconstructable_features_collection,
			reconstruction_features_collection,
			reconstruct_params,
			reconstruction_tree_cache_size);

	// Copy the RFGs in the ReconstructContext::ReconstructedFeature's.
	// The ReconstructContext::ReconstructedFeature's store RFGs and geometry property handles.
	// This format only needs the RFG.
	BOOST_FOREACH(const ReconstructContext::ReconstructedFeature &reconstructed_feature, reconstructed_features)
	{
		const ReconstructContext::ReconstructedFeature::reconstruction_seq_type &reconstructed_feature_reconstructions =
				reconstructed_feature.get_reconstructions();
		BOOST_FOREACH(const ReconstructContext::Reconstruction &reconstruction, reconstructed_feature_reconstructions)
		{
			reconstructed_feature_geometries.push_back(
					reconstruction.get_reconstructed_feature_geometry());
		}
	}

	return reconstruct_handle;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructUtils::reconstruct(
		std::vector<ReconstructContext::Reconstruction> &reconstructions,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
		const ReconstructParams &reconstruct_params,
		unsigned int reconstruction_tree_cache_size)
{
	std::vector<ReconstructContext::ReconstructedFeature> reconstructed_features;
	const ReconstructHandle::type reconstruct_handle = reconstruct(
			reconstructed_features,
			reconstruction_time,
			anchor_plate_id,
			reconstructable_features_collection,
			reconstruction_features_collection,
			reconstruct_params,
			reconstruction_tree_cache_size);

	// Copy the ReconstructContext::Reconstruction's in the ReconstructContext::ReconstructedFeature's.
	BOOST_FOREACH(const ReconstructContext::ReconstructedFeature &reconstructed_feature, reconstructed_features)
	{
		const ReconstructContext::ReconstructedFeature::reconstruction_seq_type &reconstructed_feature_reconstructions =
				reconstructed_feature.get_reconstructions();
		BOOST_FOREACH(const ReconstructContext::Reconstruction &reconstruction, reconstructed_feature_reconstructions)
		{
			reconstructions.push_back(reconstruction);
		}
	}

	return reconstruct_handle;
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructUtils::reconstruct(
		std::vector<ReconstructContext::ReconstructedFeature> &reconstructed_features,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
		const ReconstructParams &reconstruct_params,
		unsigned int reconstruction_tree_cache_size)
{
	ReconstructMethodRegistry reconstruct_method_registry;

	ReconstructionTreeCreator reconstruction_tree_creator =
			create_cached_reconstruction_tree_creator(
					reconstruction_features_collection,
					anchor_plate_id,
					reconstruction_tree_cache_size,
					// We're not going to modify the reconstruction features so no need to clone...
					false/*clone_reconstruction_features*/);

	return reconstruct(
			reconstructed_features,
			reconstruction_time,
			reconstruct_method_registry,
			reconstructable_features_collection,
			reconstruction_tree_creator,
			reconstruct_params);
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructUtils::reconstruct_geometry(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		const ReconstructMethodRegistry &reconstruct_method_registry,
		const GPlatesModel::FeatureHandle::weak_ref &reconstruction_properties,
		const double &reconstruction_time,
		const ReconstructMethodInterface::Context &reconstruct_method_context,
		bool reverse_reconstruct)
{
	// Create a context without topology reconstruction for creating a reconstruct method.
	//
	// TODO: A bit hacky - there's probably a better way to do this.
	// Problem is a reconstruct method instance might topology-reconstruct its feature's geometry
	// whereas we only want to reconstruct based on the feature's properties (eg, plate ID).
	ReconstructMethodInterface::Context reconstruct_method_context_without_topology_reconstruction(reconstruct_method_context);
	reconstruct_method_context_without_topology_reconstruction.topology_reconstruct = boost::none;

	// Find out how to reconstruct the geometry based on the feature containing the reconstruction properties.
	ReconstructMethodInterface::non_null_ptr_type reconstruct_method =
			reconstruct_method_registry.create_reconstruct_method_or_default(
					reconstruction_properties,
					reconstruct_method_context_without_topology_reconstruction);

	return reconstruct_method->reconstruct_geometry(
			geometry,
			reconstruct_method_context_without_topology_reconstruction,
			reconstruction_time,
			reverse_reconstruct);
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructUtils::reconstruct_geometry(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		const ReconstructMethodRegistry &reconstruct_method_registry,
		const GPlatesModel::FeatureHandle::weak_ref &reconstruction_properties,
		const double &reconstruction_time,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const ReconstructParams &reconstruct_params,
		bool reverse_reconstruct)
{
	// Create the context in which to reconstruct.
	const ReconstructMethodInterface::Context reconstruct_method_context(
			reconstruct_params,
			reconstruction_tree_creator);

	return reconstruct_geometry(
			geometry,
			reconstruct_method_registry,
			reconstruction_properties,
			reconstruction_time,
			reconstruct_method_context,
			reverse_reconstruct);
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructUtils::reconstruct_geometry(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		const GPlatesModel::FeatureHandle::weak_ref &reconstruction_properties,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
		const ReconstructParams &reconstruct_params,
		bool reverse_reconstruct,
		unsigned int reconstruction_tree_cache_size)
{
	ReconstructionTreeCreator reconstruction_tree_creator =
			create_cached_reconstruction_tree_creator(
					reconstruction_features_collection,
					anchor_plate_id,
					reconstruction_tree_cache_size,
					// We're not going to modify the reconstruction features so no need to clone...
					false/*clone_reconstruction_features*/);

	ReconstructMethodRegistry reconstruct_method_registry;

	return reconstruct_geometry(
			geometry,
			reconstruct_method_registry,
			reconstruction_properties,
			reconstruction_time,
			reconstruction_tree_creator,
			reconstruct_params,
			reverse_reconstruct);
}
