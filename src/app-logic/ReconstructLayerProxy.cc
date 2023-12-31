/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include <boost/bind/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/utility/in_place_factory.hpp>

#include <QDebug>

#include "ReconstructionTreeCreator.h"
#include "ReconstructLayerProxy.h"
#include "ResolvedTopologicalNetwork.h"
#include "TopologyGeometryResolverLayerProxy.h"
#include "TopologyNetworkResolverLayerProxy.h"
#include "TopologyReconstruct.h"
#include "TopologyUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/NotYetImplementedException.h"

#include "maths/CubeQuadTreePartitionUtils.h"
#include "maths/MathsUtils.h"
#include "maths/types.h"

#include "model/FeatureHandle.h"

#include "utils/Profile.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * Helper function for 'GPlatesMaths::CubeQuadTreePartitionUtils::mirror' when mirroring
		 * elements at the root of a cube quad tree.
		 */
		inline
		void
		add_reconstruction_to_root_element_of_rfg_spatial_partition(
				ReconstructLayerProxy::reconstructed_feature_geometries_spatial_partition_type &rfg_spatial_partition,
				const ReconstructContext::Reconstruction &reconstruction)
		{
			rfg_spatial_partition.add_unpartitioned(
					reconstruction.get_reconstructed_feature_geometry());
		}

		/**
		 * Helper function for 'GPlatesMaths::CubeQuadTreePartitionUtils::mirror' when mirroring
		 * elements at a quad node of a cube quad tree.
		 */
		inline
		void
		add_reconstruction_to_node_element_of_rfg_spatial_partition(
				ReconstructLayerProxy::reconstructed_feature_geometries_spatial_partition_type &rfg_spatial_partition,
				ReconstructLayerProxy::reconstructed_feature_geometries_spatial_partition_type::node_reference_type rfg_node,
				const ReconstructContext::Reconstruction &reconstruction)
		{
			rfg_spatial_partition.add(
					reconstruction.get_reconstructed_feature_geometry(),
					rfg_node);
		}
	}
}


const double GPlatesAppLogic::ReconstructLayerProxy::POLYGON_MESH_EDGE_LENGTH_THRESHOLD_RADIANS =
		GPlatesMaths::convert_deg_to_rad(5);


GPlatesAppLogic::ReconstructLayerProxy::ReconstructLayerProxy(
		const ReconstructMethodRegistry &reconstruct_method_registry,
		const ReconstructParams &reconstruct_params,
		unsigned int max_num_reconstructions_in_cache) :
	d_reconstruct_method_registry(reconstruct_method_registry),
	d_reconstruct_context(reconstruct_method_registry),
	// Start off with a reconstruction layer proxy that creates identity rotations.
	d_current_reconstruction_layer_proxy(ReconstructionLayerProxy::create()),
	d_current_reconstruction_time(0),
	d_current_reconstruct_params(reconstruct_params),
	d_cached_reconstructions(
			boost::bind(&ReconstructLayerProxy::create_reconstruction_info, this, boost::placeholders::_1),
			max_num_reconstructions_in_cache),
	d_cached_reconstructions_default_maximum_size(max_num_reconstructions_in_cache)
{
}


GPlatesAppLogic::ReconstructLayerProxy::~ReconstructLayerProxy()
{
	// Defined in ".cc" file because...
	// non_null_ptr destructors require complete type of class they're referring to.
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructLayerProxy::get_reconstructed_feature_geometries(
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
		const ReconstructParams &reconstruct_params,
		const double &reconstruction_time)
{
	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	// Lookup the cached ReconstructionInfo associated with the reconstruction time and reconstruct params.
	const reconstruction_cache_key_type reconstruction_cache_key(reconstruction_time, reconstruct_params);
	ReconstructionInfo &reconstruction_info = d_cached_reconstructions.get_value(reconstruction_cache_key);

	// If the cached reconstruction info has not been initialised or has been evicted from the cache...
	if (!reconstruction_info.cached_reconstructed_feature_geometries)
	{
		// Reconstruct our features into a sequence of ReconstructContext::ReconstructedFeature's.
		// It takes only slightly longer to generate a sequence of ReconstructContext::ReconstructedFeature's
		// versus a sequence of RFGs but it means if another client then requests the
		// ReconstructContext::ReconstructedFeature's then they will be cached and won't have to be
		// calculated - doing it the other way around doesn't work.
		std::vector<ReconstructContext::ReconstructedFeature> &reconstructed_features =
				cache_reconstructed_features(reconstruction_info, reconstruction_time);

		// Create empty vector of RFGs.
		reconstruction_info.cached_reconstructed_feature_geometries =
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type>();

		// Copy the RFGs already cached in the ReconstructContext::ReconstructedFeature's into this cached format.
		// The ReconstructContext::ReconstructedFeature's store RFGs and geometry property handles.
		// This format only needs the RFG.
		//
		// Actually this reserve will fall short for features with multiple geometry properties
		// but it should optimise most cases where there's only one per feature.
		reconstruction_info.cached_reconstructed_feature_geometries->reserve(reconstructed_features.size());
		BOOST_FOREACH(const ReconstructContext::ReconstructedFeature &reconstructed_feature, reconstructed_features)
		{
			const ReconstructContext::ReconstructedFeature::reconstruction_seq_type &reconstructions =
					reconstructed_feature.get_reconstructions();
			BOOST_FOREACH(const ReconstructContext::Reconstruction &reconstruction, reconstructions)
			{
				reconstruction_info.cached_reconstructed_feature_geometries->push_back(
						reconstruction.get_reconstructed_feature_geometry());
			}
		}
	}

	// Append our cached RFGs to the caller's sequence.
	reconstructed_feature_geometries.insert(
			reconstructed_feature_geometries.end(),
			reconstruction_info.cached_reconstructed_feature_geometries->begin(),
			reconstruction_info.cached_reconstructed_feature_geometries->end());

	return reconstruction_info.cached_reconstructed_feature_geometries_handle.get();
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructLayerProxy::get_reconstructions(
		std::vector<ReconstructContext::Reconstruction> &reconstructions,
		const ReconstructParams &reconstruct_params,
		const double &reconstruction_time)
{
	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	// Lookup the cached ReconstructionInfo associated with the reconstruction time and reconstruct params.
	const reconstruction_cache_key_type reconstruction_cache_key(reconstruction_time, reconstruct_params);
	ReconstructionInfo &reconstruction_info = d_cached_reconstructions.get_value(reconstruction_cache_key);

	// If the cached reconstruction info has not been initialised or has been evicted from the cache...
	if (!reconstruction_info.cached_reconstructions)
	{
		// Reconstruct our features into a sequence of ReconstructContext::ReconstructedFeature's.
		std::vector<ReconstructContext::ReconstructedFeature> &reconstructed_features =
				cache_reconstructed_features(reconstruction_info, reconstruction_time);

		// Create empty vector of ReconstructContext::Reconstruction's.
		reconstruction_info.cached_reconstructions = std::vector<ReconstructContext::Reconstruction>();

		// Copy the ReconstructContext::Reconstruction's already cached in the
		// ReconstructContext::ReconstructedFeature's into this cached format.
		//
		// Actually this reserve will fall short for features with multiple geometry properties
		// but it should optimise most cases where there's only one per feature.
		reconstruction_info.cached_reconstructions->reserve(reconstructed_features.size());
		BOOST_FOREACH(const ReconstructContext::ReconstructedFeature &reconstructed_feature, reconstructed_features)
		{
			const ReconstructContext::ReconstructedFeature::reconstruction_seq_type &feature_reconstructions =
					reconstructed_feature.get_reconstructions();
			BOOST_FOREACH(const ReconstructContext::Reconstruction &feature_reconstruction, feature_reconstructions)
			{
				reconstruction_info.cached_reconstructions->push_back(feature_reconstruction);
			}
		}
	}

	// Append our cached RFGs to the caller's sequence.
	reconstructions.insert(
			reconstructions.end(),
			reconstruction_info.cached_reconstructions->begin(),
			reconstruction_info.cached_reconstructions->end());

	return reconstruction_info.cached_reconstructed_feature_geometries_handle.get();
}



GPlatesAppLogic::ReconstructLayerProxy::reconstructed_feature_geometries_spatial_partition_type::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructLayerProxy::get_reconstructed_feature_geometries_spatial_partition(
		const ReconstructParams &reconstruct_params,
		const double &reconstruction_time,
		ReconstructHandle::type *reconstruct_handle)
{
	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	// Lookup the cached ReconstructionInfo associated with the reconstruction time and reconstruct params.
	const reconstruction_cache_key_type reconstruction_cache_key(reconstruction_time, reconstruct_params);
	ReconstructionInfo &reconstruction_info = d_cached_reconstructions.get_value(reconstruction_cache_key);

	// If the cached reconstruction info has not been initialised or has been evicted from the cache...
	if (!reconstruction_info.cached_reconstructed_feature_geometries_spatial_partition)
	{
		// Reconstruct our features into a spatial partition of ReconstructContext::Reconstruction's.
		// It takes only slightly longer to generate a spatial partition of
		// ReconstructContext::Reconstruction's versus a spatial partition of RFGs but it means if
		// another client then requests the ReconstructContext::Reconstruction's then they will
		// be cached and won't have to be calculated - doing it the other way around doesn't work.
		reconstructions_spatial_partition_type::non_null_ptr_to_const_type reconstructions_spatial_partition =
				cache_reconstructions_spatial_partition(reconstruction_info, reconstruction_time);

		// Add the RFGs to a new spatial partition to return to the caller.
		reconstruction_info.cached_reconstructed_feature_geometries_spatial_partition =
				reconstructed_feature_geometries_spatial_partition_type::create(
						DEFAULT_SPATIAL_PARTITION_DEPTH);

		// For each ReconstructContext::Reconstruction in the spatial partition generate an RFG
		// in the RFG spatial partition using methods
		// 'render_to_spatial_partition_root' and 'render_to_spatial_partition_quad_tree_node'
		// to do the transformations.
		GPlatesMaths::CubeQuadTreePartitionUtils::mirror(
				*reconstruction_info.cached_reconstructed_feature_geometries_spatial_partition.get(),
				*reconstructions_spatial_partition,
				&add_reconstruction_to_root_element_of_rfg_spatial_partition,
				&add_reconstruction_to_node_element_of_rfg_spatial_partition);
	}

	// If the caller wants the reconstruct handle then return it.
	if (reconstruct_handle)
	{
		*reconstruct_handle = reconstruction_info.cached_reconstructed_feature_geometries_handle.get();
	}

	// Return the cached spatial partition.
	return reconstruction_info.cached_reconstructed_feature_geometries_spatial_partition.get();
}


GPlatesAppLogic::ReconstructLayerProxy::reconstructions_spatial_partition_type::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructLayerProxy::get_reconstructions_spatial_partition(
		const ReconstructParams &reconstruct_params,
		const double &reconstruction_time,
		ReconstructHandle::type *reconstruct_handle)
{
	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	// Lookup the cached ReconstructionInfo associated with the reconstruction time and reconstruct params.
	const reconstruction_cache_key_type reconstruction_cache_key(reconstruction_time, reconstruct_params);
	ReconstructionInfo &reconstruction_info = d_cached_reconstructions.get_value(reconstruction_cache_key);

	// If the cached reconstruction info has not been initialised or has been evicted from the cache...
	if (!reconstruction_info.cached_reconstructions_spatial_partition)
	{
		cache_reconstructions_spatial_partition(
				reconstruction_info,
				reconstruction_time);
	}

	// If the caller wants the reconstruct handle then return it.
	if (reconstruct_handle)
	{
		*reconstruct_handle = reconstruction_info.cached_reconstructed_feature_geometries_handle.get();
	}

	// Return the cached spatial partition.
	return reconstruction_info.cached_reconstructions_spatial_partition.get();
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructLayerProxy::get_reconstructed_features(
		std::vector<ReconstructContext::ReconstructedFeature> &reconstructed_features,
		const ReconstructParams &reconstruct_params,
		const double &reconstruction_time)
{
	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	// Lookup the cached ReconstructionInfo associated with the reconstruction time and reconstruct params.
	const reconstruction_cache_key_type reconstruction_cache_key(reconstruction_time, reconstruct_params);
	ReconstructionInfo &reconstruction_info = d_cached_reconstructions.get_value(reconstruction_cache_key);

	// If the cached reconstruction info has not been initialised or has been evicted from the cache...
	if (!reconstruction_info.cached_reconstructed_features)
	{
		cache_reconstructed_features(reconstruction_info, reconstruction_time);
	}

	// Append our cached RFGs to the caller's sequence.
	reconstructed_features.insert(
			reconstructed_features.end(),
			reconstruction_info.cached_reconstructed_features->begin(),
			reconstruction_info.cached_reconstructed_features->end());

	return reconstruction_info.cached_reconstructed_feature_geometries_handle.get();
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructLayerProxy::get_reconstructed_feature_time_spans(
		std::vector<ReconstructContext::ReconstructedFeatureTimeSpan> &reconstructed_feature_time_spans,
		const TimeSpanUtils::TimeRange &time_range,
		const ReconstructParams &reconstruct_params)
{
	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	// Use a trick to keep the context state cached after we return from the current function.
	// TODO: Implement a cache specifically for time spans.
#if 0
	// Find an existing context state or create a new one.
	const ReconstructContext::context_state_reference_type context_state_ref =
			get_or_create_reconstruct_context(reconstruct_params);
#else
	// Similar to calling 'get_or_create_reconstruct_context()' but also caches the context state
	// in a ReconstructionInfo at an arbitrary reconstruction time (present day) so that it can get
	// re-used (rather than just having it get destroyed as soon as we leave the current function.
	//
	// This caching works even if the cache size is only 1 (eg, when reconstructing using topologies)
	// because if a different reconstruction time is later requested with the same ReconstructParams
	// (both of which form the cache key) then even though the only one reconstruction time can be cached
	// at a time the context state is re-used since we use a map of ReconstructParams to context states
	// in 'get_or_create_reconstruct_context()'.
	//
	// Note that 'get_value()' calls 'create_reconstruction_info()' which calls 'get_or_create_reconstruct_context()'.
	const ReconstructContext::context_state_reference_type context_state_ref =
			d_cached_reconstructions.get_value(
					reconstruction_cache_key_type(0.0/*reconstruction_time*/, reconstruct_params))
							.context_state;
#endif

	return d_reconstruct_context.get_reconstructed_feature_time_spans(
			reconstructed_feature_time_spans,
			context_state_ref,
			time_range);
}


void
GPlatesAppLogic::ReconstructLayerProxy::get_topology_reconstructed_feature_time_spans(
		std::vector<ReconstructContext::TopologyReconstructedFeatureTimeSpan> &topology_reconstructed_feature_time_spans,
		const ReconstructParams &reconstruct_params)
{
	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	// Use a trick to keep the context state cached after we return from the current function.
	// TODO: Implement a cache specifically for time spans.
#if 0
	// Find an existing context state or create a new one.
	const ReconstructContext::context_state_reference_type context_state_ref =
			get_or_create_reconstruct_context(reconstruct_params);
#else
	// Similar to calling 'get_or_create_reconstruct_context()' but also caches the context state
	// in a ReconstructionInfo at an arbitrary reconstruction time (present day) so that it can get
	// re-used (rather than just having it get destroyed as soon as we leave the current function.
	//
	// This caching works even if the cache size is only 1 (eg, when reconstructing using topologies)
	// because if a different reconstruction time is later requested with the same ReconstructParams
	// (both of which form the cache key) then even though the only one reconstruction time can be cached
	// at a time the context state is re-used since we use a map of ReconstructParams to context states
	// in 'get_or_create_reconstruct_context()'.
	//
	// Note that 'get_value()' calls 'create_reconstruction_info()' which calls 'get_or_create_reconstruct_context()'.
	const ReconstructContext::context_state_reference_type context_state_ref =
			d_cached_reconstructions.get_value(
					reconstruction_cache_key_type(0.0/*reconstruction_time*/, reconstruct_params))
							.context_state;
#endif

	return d_reconstruct_context.get_topology_reconstructed_feature_time_spans(
				topology_reconstructed_feature_time_spans,
				context_state_ref);
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructLayerProxy::get_reconstructed_topological_sections(
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_topological_sections,
		const std::set<GPlatesModel::FeatureId> &topological_sections_referenced,
		const ReconstructParams &reconstruct_params,
		const double &reconstruction_time)
{
	PROFILE_FUNC();

	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	// Lookup the cached ReconstructionInfo associated with the reconstruction time and reconstruct params.
	//
	// If a new 'ReconstructionInfo' is returned it won't be expensive to create because we won't be using
	// topologies to reconstruct (because a topology layer is asking us for topological sections and it
	// won't ask layers, that reconstruct using topologies, to do that).
	const reconstruction_cache_key_type reconstruction_cache_key(reconstruction_time, reconstruct_params);
	const ReconstructionInfo &reconstruction_info = d_cached_reconstructions.get_value(reconstruction_cache_key);

	//
	// We don't want to re-generate the cache - we only want to re-use the cache if it's there.
	// We also want the context state to remain (in the cached ReconstructionInfo) so it can be re-used
	// (this is really just re-using anything that happens to be cached inside the context's ReconstructMethod instances).
	//

	// If we have cached RFGs then just return them.
	if (reconstruction_info.cached_reconstructed_feature_geometries)
	{
		// Append, to the caller's sequence, those cached RFGs that match the topological section feature IDs.
		const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries =
				reconstruction_info.cached_reconstructed_feature_geometries.get();
		BOOST_FOREACH(
				const ReconstructedFeatureGeometry::non_null_ptr_type &reconstructed_feature_geometry,
				reconstructed_feature_geometries)
		{
			const GPlatesModel::FeatureHandle::weak_ref feature_ref = reconstructed_feature_geometry->get_feature_ref();
			if (feature_ref.is_valid() &&
				topological_sections_referenced.find(feature_ref->feature_id()) != topological_sections_referenced.end())
			{
				reconstructed_topological_sections.push_back(reconstructed_feature_geometry);
			}
		}

		return reconstruction_info.cached_reconstructed_feature_geometries_handle.get();
	}

	// If we have cached 'ReconstructContext::ReconstructedFeature's then convert them to RFGs.
	if (reconstruction_info.cached_reconstructed_features)
	{
		// Copy the RFGs already cached in the ReconstructContext::ReconstructedFeature's into RFG format.
		// The ReconstructContext::ReconstructedFeature's store RFGs and geometry property handles.
		// We only need RFGs.
		const std::vector<ReconstructContext::ReconstructedFeature> &reconstructed_features =
				reconstruction_info.cached_reconstructed_features.get();
		BOOST_FOREACH(const ReconstructContext::ReconstructedFeature &reconstructed_feature, reconstructed_features)
		{
			// Append, to the caller's sequence, those cached RFGs that match the topological section feature IDs.
			const GPlatesModel::FeatureHandle::weak_ref feature_ref = reconstructed_feature.get_feature();
			if (feature_ref.is_valid() &&
				topological_sections_referenced.find(feature_ref->feature_id()) != topological_sections_referenced.end())
			{
				const ReconstructContext::ReconstructedFeature::reconstruction_seq_type &reconstructions =
						reconstructed_feature.get_reconstructions();
				BOOST_FOREACH(const ReconstructContext::Reconstruction &reconstruction, reconstructions)
				{
					reconstructed_topological_sections.push_back(
							reconstruction.get_reconstructed_feature_geometry());
				}
			}
		}

		return reconstruction_info.cached_reconstructed_feature_geometries_handle.get();
	}

	// Generate RFGs only for the requested topological sections.
	// Note that we don't cache these results because we'd then have to keep track of which
	// feature IDs we've cached for (we could do that though, but currently it's not really necessary).
	return d_reconstruct_context.get_reconstructed_topological_sections( 
			reconstructed_topological_sections,
			topological_sections_referenced,
			reconstruction_info.context_state,
			reconstruction_time);
}


GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::non_null_ptr_type
GPlatesAppLogic::ReconstructLayerProxy::get_reconstructed_static_polygon_meshes(
		GPlatesOpenGL::GLRenderer &renderer,
		bool reconstructing_with_age_grid,
		const double &reconstruction_time)
{
	PROFILE_FUNC();

	bool need_to_update = false;

	// Rebuild the GLReconstructedStaticPolygonMeshes object if necessary.
	// If it doesn't exist then either it has never been requested or it was invalidated
	// because the reconstructable feature collections have changed in some way causing the
	// present day polygon meshes to (possibly) change.
	if (!d_cached_reconstructed_polygon_meshes.cached_reconstructed_static_polygon_meshes)
	{
		GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::non_null_ptr_type reconstructed_polygon_meshes =
				GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::create(
						renderer,
						get_present_day_polygon_meshes(),
						get_present_day_geometries(),
						reconstruction_time,
						get_reconstructions_spatial_partition(reconstruction_time));
		d_cached_reconstructed_polygon_meshes.cached_reconstructed_static_polygon_meshes = reconstructed_polygon_meshes;

		// Even though we just created the 'GLReconstructedStaticPolygonMeshes' we still need to
		// update it in case we're using age grids which need a reconstructions spatial partition
		// that ignores the active time periods of features.
		need_to_update = true;

		// We have taken measures to be up-to-date with respect to the reconstructed polygon geometries.
		get_subject_token().update_observer(
				d_cached_reconstructed_polygon_meshes.cached_reconstructed_polygons_observer_token);
	}

	// Update if the reconstruction time has changed...
	if (d_cached_reconstructed_polygon_meshes.cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time))
	{
		need_to_update = true;

		d_cached_reconstructed_polygon_meshes.cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
	}

	// Update if we're changing decision to reconstruct with an age grid...
	if (d_cached_reconstructed_polygon_meshes.cached_reconstructing_with_age_grid != reconstructing_with_age_grid)
	{
		need_to_update = true;

		d_cached_reconstructed_polygon_meshes.cached_reconstructing_with_age_grid = reconstructing_with_age_grid;
	}

	// Update if we're not up-to-date with respect to the reconstructed polygon geometries...
	if (!get_subject_token().is_observer_up_to_date(
			d_cached_reconstructed_polygon_meshes.cached_reconstructed_polygons_observer_token))
	{
		need_to_update = true;

		// We have taken measures to be up-to-date with respect to the reconstructed polygon geometries.
		get_subject_token().update_observer(
				d_cached_reconstructed_polygon_meshes.cached_reconstructed_polygons_observer_token);
	}

	if (need_to_update)
	{
		//
		// Update our cached reconstructed polygon meshes.
		//

		// The reconstructions spatial partition for *active* features.
		const GPlatesAppLogic::ReconstructLayerProxy::reconstructions_spatial_partition_type::non_null_ptr_to_const_type
				reconstructions_spatial_partition =
						get_reconstructions_spatial_partition(reconstruction_time);

		// The reconstructions spatial partition for *active* or *inactive* features.
		boost::optional<GPlatesAppLogic::ReconstructLayerProxy::reconstructions_spatial_partition_type::non_null_ptr_to_const_type>
						active_or_inactive_reconstructions_spatial_partition;
		// It's only needed if we've been asked to help reconstruct a raster with the aid of an age grid.
		if (reconstructing_with_age_grid)
		{
			// Use the same reconstruct params but specify that reconstructions should include *inactive* features also.
			GPlatesAppLogic::ReconstructParams reconstruct_params = get_current_reconstruct_params();
			reconstruct_params.set_reconstruct_by_plate_id_outside_active_time_period(true);

			// Get a new reconstructions spatial partition that includes *inactive* reconstructions.
			active_or_inactive_reconstructions_spatial_partition =
					get_reconstructions_spatial_partition(reconstruct_params, reconstruction_time);
		}

		d_cached_reconstructed_polygon_meshes.cached_reconstructed_static_polygon_meshes.get()->update(
				reconstruction_time,
				reconstructions_spatial_partition,
				active_or_inactive_reconstructions_spatial_partition);
	}

	return d_cached_reconstructed_polygon_meshes.cached_reconstructed_static_polygon_meshes.get();
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructLayerProxy::get_reconstructed_feature_velocities(
		std::vector<MultiPointVectorField::non_null_ptr_type> &reconstructed_feature_velocities,
		const ReconstructParams &reconstruct_params,
		const double &reconstruction_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		const double &velocity_delta_time)
{
	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	// Lookup the cached ReconstructionInfo associated with the reconstruction time and reconstruct params.
	const reconstruction_cache_key_type reconstruction_cache_key(reconstruction_time, reconstruct_params);
	ReconstructionInfo &reconstruction_info = d_cached_reconstructions.get_value(reconstruction_cache_key);

	// If the velocity delta time parameters have changed then remove the velocities from the cache.
	if (reconstruction_info.cached_velocity_delta_time_params !=
		std::make_pair(velocity_delta_time_type, GPlatesMaths::real_t(velocity_delta_time)))
	{
		reconstruction_info.cached_reconstructed_feature_velocities = boost::none;

		reconstruction_info.cached_velocity_delta_time_params =
				std::make_pair(velocity_delta_time_type, GPlatesMaths::real_t(velocity_delta_time));
	}

	// If the cached velocities have not been initialised or has been evicted from the cache...
	if (!reconstruction_info.cached_reconstructed_feature_velocities)
	{
		// Calculate velocities at the reconstructed feature geometry positions.
		cache_reconstructed_feature_velocities(
				reconstruction_info,
				reconstruction_time,
				velocity_delta_time_type,
				velocity_delta_time);
	}

	// Append our cached velocities to the caller's sequence.
	reconstructed_feature_velocities.insert(
			reconstructed_feature_velocities.end(),
			reconstruction_info.cached_reconstructed_feature_velocities->begin(),
			reconstruction_info.cached_reconstructed_feature_velocities->end());

	return reconstruction_info.cached_reconstructed_feature_velocities_handle.get();
}


GPlatesAppLogic::ReconstructMethodInterface::Context
GPlatesAppLogic::ReconstructLayerProxy::get_reconstruct_method_context(
		const ReconstructParams &reconstruct_params) const
{
	// If the ReconstructParams says not to reconstruct using topologies.
	if (!reconstruct_params.get_reconstruct_using_topologies())
	{
		return ReconstructMethodInterface::Context(
				reconstruct_params,
				// The reconstruction tree creator...
				d_current_reconstruction_layer_proxy.get_input_layer_proxy()->get_reconstruction_tree_creator());
	}

	const TimeSpanUtils::TimeRange time_range(
			reconstruct_params.get_topology_reconstruction_begin_time(),
			reconstruct_params.get_topology_reconstruction_end_time(),
			reconstruct_params.get_topology_reconstruction_time_increment(),
			TimeSpanUtils::TimeRange::ADJUST_BEGIN_TIME);
	const unsigned int num_time_slots = time_range.get_num_time_slots();

	// As a performance optimisation, request a reconstruction tree creator with a cache size
	// the same as the time range.
	// This ensures we don't get a noticeable slowdown when the time span range exceeds the
	// size of the cache in the reconstruction layer proxy.
	ReconstructionTreeCreator reconstruction_tree_creator =
			d_current_reconstruction_layer_proxy.get_input_layer_proxy()->get_reconstruction_tree_creator(
					// +1 accounts for the extra time step used to generate deformed geometries...
					num_time_slots + 1);

	// Create our resolved network time span that combines resolved networks from
	// *all* topological network layers.
	TopologyReconstruct::resolved_network_time_span_type::non_null_ptr_type combined_resolved_network_time_span =
			TopologyReconstruct::resolved_network_time_span_type::create(time_range);

	// Get a resolved network time span from each topological network layer.
	std::vector<TopologyReconstruct::resolved_network_time_span_type::non_null_ptr_to_const_type>
			resolved_network_time_spans;
	resolved_network_time_spans.reserve(
			d_current_topological_network_resolver_layer_proxies.size());
	BOOST_FOREACH(
			const LayerProxyUtils::InputLayerProxy<TopologyNetworkResolverLayerProxy> &
					topological_network_resolver_layer_proxy,
			d_current_topological_network_resolver_layer_proxies)
	{
		TopologyReconstruct::resolved_network_time_span_type::non_null_ptr_to_const_type
				resolved_network_time_span =
						topological_network_resolver_layer_proxy.get_input_layer_proxy()
								->get_resolved_network_time_span(time_range);

		resolved_network_time_spans.push_back(resolved_network_time_span);
	}

	// Iterate over the time slots of the time span and fill in the resolved topological networks.
	for (unsigned int time_slot = 0; time_slot < num_time_slots; ++time_slot)
	{
		TopologyReconstruct::rtn_seq_type rtns_in_time_slot;

		// Get the resolved topological networks for the current time slot.
		std::vector<TopologyReconstruct::resolved_network_time_span_type::non_null_ptr_to_const_type>::const_iterator
				resolved_network_time_spans_iter = resolved_network_time_spans.begin();
		std::vector<TopologyReconstruct::resolved_network_time_span_type::non_null_ptr_to_const_type>::const_iterator
				resolved_network_time_spans_end = resolved_network_time_spans.end();
		for ( ;
			resolved_network_time_spans_iter != resolved_network_time_spans_end;
			++resolved_network_time_spans_iter)
		{
			TopologyReconstruct::resolved_network_time_span_type::non_null_ptr_to_const_type
					resolved_network_time_span = *resolved_network_time_spans_iter;

			boost::optional<const TopologyReconstruct::rtn_seq_type &> rtns =
					resolved_network_time_span->get_sample_in_time_slot(time_slot);
			if (rtns)
			{
				rtns_in_time_slot.insert(rtns_in_time_slot.end(), rtns->begin(), rtns->end());
			}
		}

		if (!rtns_in_time_slot.empty())
		{
			combined_resolved_network_time_span->set_sample_in_time_slot(rtns_in_time_slot, time_slot);
		}
	}

	// Create our resolved boundary time span that combines resolved boundaries from
	// *all* topological boundary layers.
	TopologyReconstruct::resolved_boundary_time_span_type::non_null_ptr_type combined_resolved_boundary_time_span =
			TopologyReconstruct::resolved_boundary_time_span_type::create(time_range);

	// Get a resolved boundary time span from each topological boundary layer.
	std::vector<TopologyReconstruct::resolved_boundary_time_span_type::non_null_ptr_to_const_type>
			resolved_boundary_time_spans;
	resolved_boundary_time_spans.reserve(
			d_current_topological_boundary_resolver_layer_proxies.size());
	BOOST_FOREACH(
			const LayerProxyUtils::InputLayerProxy<TopologyGeometryResolverLayerProxy> &
					topological_boundary_resolver_layer_proxy,
			d_current_topological_boundary_resolver_layer_proxies)
	{
		TopologyReconstruct::resolved_boundary_time_span_type::non_null_ptr_to_const_type
				resolved_boundary_time_span =
						topological_boundary_resolver_layer_proxy.get_input_layer_proxy()
								->get_resolved_boundary_time_span(time_range);

		resolved_boundary_time_spans.push_back(resolved_boundary_time_span);
	}

	// Iterate over the time slots of the time span and fill in the resolved topological boundaries.
	for (unsigned int time_slot = 0; time_slot < num_time_slots; ++time_slot)
	{
		TopologyReconstruct::rtb_seq_type rtbs_in_time_slot;

		// Get the resolved topological boundaries for the current time slot.
		std::vector<TopologyReconstruct::resolved_boundary_time_span_type::non_null_ptr_to_const_type>::const_iterator
				resolved_boundary_time_spans_iter = resolved_boundary_time_spans.begin();
		std::vector<TopologyReconstruct::resolved_boundary_time_span_type::non_null_ptr_to_const_type>::const_iterator
				resolved_boundary_time_spans_end = resolved_boundary_time_spans.end();
		for ( ;
			resolved_boundary_time_spans_iter != resolved_boundary_time_spans_end;
			++resolved_boundary_time_spans_iter)
		{
			TopologyReconstruct::resolved_boundary_time_span_type::non_null_ptr_to_const_type
					resolved_boundary_time_span = *resolved_boundary_time_spans_iter;

			boost::optional<const TopologyReconstruct::rtb_seq_type &> rtbs =
					resolved_boundary_time_span->get_sample_in_time_slot(time_slot);
			if (rtbs)
			{
				rtbs_in_time_slot.insert(rtbs_in_time_slot.end(), rtbs->begin(), rtbs->end());
			}
		}

		if (!rtbs_in_time_slot.empty())
		{
			combined_resolved_boundary_time_span->set_sample_in_time_slot(rtbs_in_time_slot, time_slot);
		}
	}

	// Create our topology reconstruct object that combines resolved boundaries and networks from
	// *all* topological boundary/network layers.
	TopologyReconstruct::non_null_ptr_to_const_type topology_reconstruct =
			TopologyReconstruct::create(
					time_range,
					combined_resolved_boundary_time_span,
					combined_resolved_network_time_span,
					reconstruction_tree_creator);

	return ReconstructMethodInterface::Context(
			reconstruct_params,
			reconstruction_tree_creator,
			topology_reconstruct);
}


const std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> &
GPlatesAppLogic::ReconstructLayerProxy::get_present_day_geometries()
{
	if (!d_cached_present_day_info.cached_present_day_geometries)
	{
		d_cached_present_day_info.cached_present_day_geometries =
				d_reconstruct_context.get_present_day_feature_geometries();
	}

	return d_cached_present_day_info.cached_present_day_geometries.get();
}


const std::vector<boost::optional<GPlatesMaths::PolygonMesh::non_null_ptr_to_const_type> > &
GPlatesAppLogic::ReconstructLayerProxy::get_present_day_polygon_meshes()
{
	if (!d_cached_present_day_info.cached_present_day_polygon_meshes)
	{
		// First get the present day geometries.
		const std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> &
				present_day_geometries = get_present_day_geometries();

		// Start off with an empty sequence of polygon meshes.
		d_cached_present_day_info.cached_present_day_polygon_meshes =
				std::vector<boost::optional<GPlatesMaths::PolygonMesh::non_null_ptr_to_const_type> >();
		std::vector<boost::optional<GPlatesMaths::PolygonMesh::non_null_ptr_to_const_type> > &
				present_day_polygon_meshes = d_cached_present_day_info.cached_present_day_polygon_meshes.get();
		present_day_polygon_meshes.reserve(present_day_geometries.size());

		BOOST_FOREACH(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &present_day_geometry,
				present_day_geometries)
		{
			// Create the polygon mesh from the present day geometry.
			// Note that the returned polygon mesh could be boost::none (but we add it anyway).
			present_day_polygon_meshes.push_back(
					GPlatesMaths::PolygonMesh::create(
							present_day_geometry,
							POLYGON_MESH_EDGE_LENGTH_THRESHOLD_RADIANS));
		}
	}

	return d_cached_present_day_info.cached_present_day_polygon_meshes.get();
}


GPlatesAppLogic::ReconstructLayerProxy::geometries_spatial_partition_type::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructLayerProxy::get_present_day_geometries_spatial_partition()
{
	if (!d_cached_present_day_info.cached_present_day_geometries_spatial_partition)
	{
		// Get the present day geometries.
		const std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> &
				present_day_geometries = get_present_day_geometries();

		// Generate the location of each present day geometry in the spatial partition.
		// This is in case it's later requested by the client.
		d_cached_present_day_info.cached_present_day_geometries_spatial_partition_locations =
				std::vector<GPlatesMaths::CubeQuadTreeLocation>();
		std::vector<GPlatesMaths::CubeQuadTreeLocation> &spatial_partition_locations =
				d_cached_present_day_info.cached_present_day_geometries_spatial_partition_locations.get();

		// Start out creating a default-constructed location for each present day geometry.
		spatial_partition_locations.resize(present_day_geometries.size());

		// Add the present day geometries to a new spatial partition to return to the caller.
		d_cached_present_day_info.cached_present_day_geometries_spatial_partition =
				geometries_spatial_partition_type::create(DEFAULT_SPATIAL_PARTITION_DEPTH);

		unsigned int present_day_geometry_index = 0;
		BOOST_FOREACH(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &present_day_geometry,
				present_day_geometries)
		{
			// The first argument is what's inserted into the partition.
			// The second argument is what determines the location at which to insert.
			d_cached_present_day_info.cached_present_day_geometries_spatial_partition.get()->add(
					present_day_geometry,
					*present_day_geometry,
					// Write the location, at which the geometry is added, to our sequence...
					&spatial_partition_locations[present_day_geometry_index]);

			++present_day_geometry_index;
		}
	}

	return d_cached_present_day_info.cached_present_day_geometries_spatial_partition.get();
}


const std::vector<GPlatesMaths::CubeQuadTreeLocation> &
GPlatesAppLogic::ReconstructLayerProxy::get_present_day_geometries_spatial_partition_locations()
{
	if (!d_cached_present_day_info.cached_present_day_geometries_spatial_partition_locations)
	{
		// The locations are generated when the spatial partition is built.
		get_present_day_geometries_spatial_partition();
	}

	return d_cached_present_day_info.cached_present_day_geometries_spatial_partition_locations.get();
}


GPlatesAppLogic::ReconstructionLayerProxy::non_null_ptr_type
GPlatesAppLogic::ReconstructLayerProxy::get_current_reconstruction_layer_proxy()
{
	return d_current_reconstruction_layer_proxy.get_input_layer_proxy();
}


void
GPlatesAppLogic::ReconstructLayerProxy::get_current_reconstructable_features(
		std::vector<GPlatesModel::FeatureHandle::weak_ref> &features) const
{
	features.insert(
			features.end(),
			d_current_reconstructable_features.begin(),
			d_current_reconstructable_features.end());
}


void
GPlatesAppLogic::ReconstructLayerProxy::get_current_features(
		std::vector<GPlatesModel::FeatureHandle::weak_ref> &features) const
{
	// Iterate over the current feature collections.
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>::const_iterator feature_collections_iter =
			d_current_feature_collections.begin();
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>::const_iterator feature_collections_end =
			d_current_feature_collections.end();
	for ( ; feature_collections_iter != feature_collections_end; ++feature_collections_iter)
	{
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection = *feature_collections_iter;
		if (feature_collection.is_valid())
		{
			GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection->begin();
			GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection->end();
			for ( ; features_iter != features_end; ++features_iter)
			{
				const GPlatesModel::FeatureHandle::weak_ref feature = (*features_iter)->reference();

				if (feature.is_valid())
				{
					features.push_back(feature);
				}
			}
		}
	}
}


const GPlatesUtils::SubjectToken &
GPlatesAppLogic::ReconstructLayerProxy::get_subject_token()
{
	// We've checked to see if any inputs have changed *except* the reconstruction and
	// topological network layer proxy inputs.
	// This is because we get notified of all changes to input except input layer proxies which
	// we have to poll to see if they changed since we last accessed them - so we do that now.
	check_input_layer_proxies();

	return d_subject_token;
}


const GPlatesUtils::SubjectToken &
GPlatesAppLogic::ReconstructLayerProxy::get_reconstructable_feature_collections_subject_token()
{
	return d_reconstructable_feature_collections_subject_token;
}


void
GPlatesAppLogic::ReconstructLayerProxy::set_current_reconstruction_time(
		const double &reconstruction_time)
{
	if (d_current_reconstruction_time == GPlatesMaths::real_t(reconstruction_time))
	{
		// The current reconstruction time hasn't changed so avoid updating any observers unnecessarily.
		return;
	}
	d_current_reconstruction_time = reconstruction_time;

	// Note that we don't invalidate our reconstruction cache because if a reconstruction is
	// not cached for a requested reconstruction time then a new reconstruction is created.
	//

	// UPDATE: Don't need to notify observers of change in reconstruction time because all layers
	// can easily find this out. We want to avoid observer updates here in case any of them
	// cache calculations based on the reconstruction time - if we told them we had changed they
	// would have no way of knowing that only the reconstruction time changed and hence they
	// would be forced to flush their caches losing any benefit of caching over reconstruction times.
#if 0
	d_subject_token.invalidate();
#endif
}


void
GPlatesAppLogic::ReconstructLayerProxy::set_current_reconstruct_params(
		const ReconstructParams &reconstruct_params)
{
	if (d_current_reconstruct_params == reconstruct_params)
	{
		// The current reconstruct params haven't changed so avoid updating any observers unnecessarily.
		return;
	}

	// If we are now using topologies but were not previously (or vice versa) then
	// invalidate our reconstruction cache.
	if (d_current_reconstruct_params.get_reconstruct_using_topologies() !=
		reconstruct_params.get_reconstruct_using_topologies())
	{
		// The cached reconstruction info is now invalid.
		reset_reconstruction_cache();
	}

	d_current_reconstruct_params = reconstruct_params;

	if (d_current_reconstruct_params.get_reconstruct_using_topologies())
	{
		// Reduce the reconstructions cache size to one while using topologies to reconstruct since
		// a time span of resolved topologies is needed for this and they can consume a lot of memory.
		// So we limit the reconstructions cache size to one so that there's only one resolved topologies
		// time span in existence at a time.
		d_cached_reconstructions.set_maximum_num_values_in_cache(1);
	}
	else
	{
		// We are no longer reconstructing geometries using topologies and hence no longer need to
		// limit memory consumption as much.
		// So we restore the default maximum size of the reconstructions cache.
		d_cached_reconstructions.set_maximum_num_values_in_cache(
				d_cached_reconstructions_default_maximum_size);
	}

	// Note that we don't invalidate our reconstruction cache because if a reconstruction is
	// not cached for a requested reconstruct params then a new reconstruction is created.
	// Observers need to be aware that the default reconstruct params have changed though.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::ReconstructLayerProxy::set_current_reconstruction_layer_proxy(
		const ReconstructionLayerProxy::non_null_ptr_type &reconstruction_layer_proxy)
{
	d_current_reconstruction_layer_proxy.set_input_layer_proxy(reconstruction_layer_proxy);

	// The cached reconstruction info is now invalid.
	reset_reconstruction_cache();

	// Polling observers need to update themselves.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::ReconstructLayerProxy::set_current_topology_surface_layer_proxies(
		const std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> &resolved_boundary_topology_surface_layer_proxies,
		const std::vector<TopologyNetworkResolverLayerProxy::non_null_ptr_type> &resolved_network_topology_surface_layer_proxies)
{
	bool changed_topology_surface_layer_proxies = false;

	if (d_current_topological_boundary_resolver_layer_proxies.set_input_layer_proxies(
			resolved_boundary_topology_surface_layer_proxies))
	{
		// The topology *boundary* surface layers are different than last time.
		changed_topology_surface_layer_proxies = true;
	}

	if (d_current_topological_network_resolver_layer_proxies.set_input_layer_proxies(
			resolved_network_topology_surface_layer_proxies))
	{
		// The topology *network* surface layers are different than last time.
		changed_topology_surface_layer_proxies = true;
	}

	if (changed_topology_surface_layer_proxies &&
		// Note: We only need to invalidate if we're actually using the topologies...
		using_topologies_to_reconstruct())
	{
		// The cached reconstruction info is now invalid.
		reset_reconstruction_cache();

		// Polling observers need to update themselves with respect to us.
		d_subject_token.invalidate();
	}
}


void
GPlatesAppLogic::ReconstructLayerProxy::add_reconstructable_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	d_current_feature_collections.push_back(feature_collection);

	// Notify the reconstruct context of the new features.
	d_current_reconstructable_features.clear();
	d_reconstruct_context.set_features(
			d_current_feature_collections,
			d_current_reconstructable_features);

	// The cached reconstruction info is now invalid.
	reset_reconstruction_cache();

	// Polling observers need to update themselves.
	d_subject_token.invalidate();

	// Anything dependent on the reconstructable feature collections is now invalid.
	reset_reconstructable_feature_collection_caches();

	// Polling observers need to update themselves if they depend on present day geometries, for example.
	d_reconstructable_feature_collections_subject_token.invalidate();
}


void
GPlatesAppLogic::ReconstructLayerProxy::remove_reconstructable_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// Erase the feature collection from our list.
	d_current_feature_collections.erase(
			std::find(
					d_current_feature_collections.begin(),
					d_current_feature_collections.end(),
					feature_collection));

	// Notify the reconstruct context of the new features.
	d_current_reconstructable_features.clear();
	d_reconstruct_context.set_features(
			d_current_feature_collections,
			d_current_reconstructable_features);

	// The cached reconstruction info is now invalid.
	reset_reconstruction_cache();

	// Polling observers need to update themselves.
	d_subject_token.invalidate();

	// Anything dependent on the reconstructable feature collections is now invalid.
	reset_reconstructable_feature_collection_caches();

	// Polling observers need to update themselves if they depend on present day geometries, for example.
	d_reconstructable_feature_collections_subject_token.invalidate();
}


void
GPlatesAppLogic::ReconstructLayerProxy::modified_reconstructable_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// Notify the reconstruct context of the new features.
	d_current_reconstructable_features.clear();
	d_reconstruct_context.set_features(
			d_current_feature_collections,
			d_current_reconstructable_features);

	// The cached reconstruction info is now invalid.
	reset_reconstruction_cache();

	// Polling observers need to update themselves.
	d_subject_token.invalidate();

	// Anything dependent on the reconstructable feature collections is now invalid.
	reset_reconstructable_feature_collection_caches();

	// Polling observers need to update themselves if they depend on present day geometries, for example.
	d_reconstructable_feature_collections_subject_token.invalidate();
}


bool
GPlatesAppLogic::ReconstructLayerProxy::using_topologies_to_reconstruct() const
{
	return d_current_reconstruct_params.get_reconstruct_using_topologies();
}


void
GPlatesAppLogic::ReconstructLayerProxy::reset_reconstruction_cache()
{
	// Clear any cached reconstruction info for any reconstruction times and reconstruct params.
	// Note that this also destroys any context states created from ReconstructContext.
	d_cached_reconstructions.clear();
}


void
GPlatesAppLogic::ReconstructLayerProxy::reset_reconstructable_feature_collection_caches()
{
	// Clear anything that depends on the reconstructable feature collections.
	d_cached_present_day_info.invalidate();

	// These are *reconstructed* polygon meshes but they depend on the *present day* polygon meshes
	// which in turn depend on the reconstructable feature collections.
	d_cached_reconstructed_polygon_meshes.invalidate();
}


template <class InputLayerProxyWrapperType>
void
GPlatesAppLogic::ReconstructLayerProxy::check_input_layer_proxy(
		InputLayerProxyWrapperType &input_layer_proxy_wrapper)
{
	// See if the input layer proxy has changed.
	if (!input_layer_proxy_wrapper.is_up_to_date())
	{
		// The cached reconstruction info is now invalid.
		reset_reconstruction_cache();

		// We're now up-to-date with respect to the input layer proxy.
		input_layer_proxy_wrapper.set_up_to_date();

		// Polling observers need to update themselves with respect to us.
		d_subject_token.invalidate();
	}
}


void
GPlatesAppLogic::ReconstructLayerProxy::check_input_layer_proxies()
{
	// See if the reconstruction layer proxy has changed.
	check_input_layer_proxy(d_current_reconstruction_layer_proxy);

	// Only check input topology layers if we're actually using them.
	//
	// NOTE: This also avoids infinite recursion due to these topology layers checking us.
	// They also won't check us if we're using topologies.
	if (using_topologies_to_reconstruct())
	{
		// See if the resolved boundary layer proxies have changed.
		BOOST_FOREACH(
				LayerProxyUtils::InputLayerProxy<TopologyGeometryResolverLayerProxy> &topological_boundary_resolver_layer_proxy,
				d_current_topological_boundary_resolver_layer_proxies)
		{
			check_input_layer_proxy(topological_boundary_resolver_layer_proxy);
		}

		// See if the resolved network layer proxies have changed.
		BOOST_FOREACH(
				LayerProxyUtils::InputLayerProxy<TopologyNetworkResolverLayerProxy> &topological_network_resolver_layer_proxy,
				d_current_topological_network_resolver_layer_proxies)
		{
			check_input_layer_proxy(topological_network_resolver_layer_proxy);
		}
	}
}


std::vector<GPlatesAppLogic::ReconstructContext::ReconstructedFeature> &
GPlatesAppLogic::ReconstructLayerProxy::cache_reconstructed_features(
		ReconstructionInfo &reconstruction_info,
		const double &reconstruction_time)
{
	// If they're already cached then nothing to do.
	if (reconstruction_info.cached_reconstructed_features)
	{
		return reconstruction_info.cached_reconstructed_features.get();
	}

	// Create empty vector of reconstructed features.
	reconstruction_info.cached_reconstructed_features = std::vector<ReconstructContext::ReconstructedFeature>();

	// Reconstruct our features into our cache.
	reconstruction_info.cached_reconstructed_feature_geometries_handle =
			d_reconstruct_context.get_reconstructed_features( 
					reconstruction_info.cached_reconstructed_features.get(),
					reconstruction_info.context_state,
					reconstruction_time);

	return reconstruction_info.cached_reconstructed_features.get();
}


GPlatesAppLogic::ReconstructLayerProxy::reconstructions_spatial_partition_type::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructLayerProxy::cache_reconstructions_spatial_partition(
		ReconstructionInfo &reconstruction_info,
		const double &reconstruction_time)
{
	// If they're already cached then nothing to do.
	if (reconstruction_info.cached_reconstructions_spatial_partition)
	{
		return reconstruction_info.cached_reconstructions_spatial_partition.get();
	}

	// Reconstruct our features into a sequence of ReconstructContext::ReconstructedFeature's.
	std::vector<ReconstructContext::ReconstructedFeature> &reconstructed_features =
			cache_reconstructed_features(reconstruction_info, reconstruction_time);

	//PROFILE_BLOCK("ReconstructLayerProxy::get_reconstructions_spatial_partition: ");

	// Add the RFGs to a new spatial partition to return to the caller.
	reconstruction_info.cached_reconstructions_spatial_partition =
			reconstructions_spatial_partition_type::create(
					DEFAULT_SPATIAL_PARTITION_DEPTH);

	BOOST_FOREACH(const ReconstructContext::ReconstructedFeature &reconstructed_feature, reconstructed_features)
	{
		const ReconstructContext::ReconstructedFeature::reconstruction_seq_type &reconstructions =
				reconstructed_feature.get_reconstructions();
		BOOST_FOREACH(const ReconstructContext::Reconstruction &reconstruction, reconstructions)
		{
			// NOTE: To avoid reconstructing geometries when it might not be needed we add the
			// *unreconstructed* geometry (and a finite rotation) to the spatial partition.
			// The spatial partition will rotate only the centroid of the *unreconstructed*
			// geometry (instead of reconstructing the entire geometry) and then use that as the
			// insertion location (along with the *unreconstructed* geometry's bounding circle extents).
			// An example where transforming might not be needed is data mining co-registration
			// where might not need to transform all geometries to determine if seed and target
			// features are close enough within a region of interest.

			const ReconstructedFeatureGeometry::non_null_ptr_type &rfg =
					reconstruction.get_reconstructed_feature_geometry();

			// See if the reconstruction can be represented as a finite rotation.
			const boost::optional<ReconstructedFeatureGeometry::FiniteRotationReconstruction> &
					finite_rotation_reconstruction = rfg->finite_rotation_reconstruction();
			if (finite_rotation_reconstruction)
			{
				// The resolved geometry is the *unreconstructed* geometry (but still possibly
				// the result of a look up of a time-dependent geometry property).
				const GPlatesMaths::GeometryOnSphere &resolved_geometry =
						*finite_rotation_reconstruction->get_resolved_geometry();
				const GPlatesMaths::FiniteRotation &finite_rotation =
						finite_rotation_reconstruction->get_reconstruct_method_finite_rotation()
								->get_finite_rotation();

				reconstruction_info.cached_reconstructions_spatial_partition.get()->add(
						reconstruction, resolved_geometry, finite_rotation);
			}
			else
			{
				// It's not a finite rotation so we can't assume the geometry has rigidly rotated.
				// Hence we can't assume its shape is the same and hence can't assume the
				// small circle bounding radius is the same.
				// So just get the reconstructed geometry and insert it into the spatial partition.
				// The appropriate bounding small circle will be generated for it when it's added.
				reconstruction_info.cached_reconstructions_spatial_partition.get()->add(
						reconstruction, *rfg->reconstructed_geometry());
			}
		}
	}

	return reconstruction_info.cached_reconstructions_spatial_partition.get();
}


std::vector<GPlatesAppLogic::MultiPointVectorField::non_null_ptr_type> &
GPlatesAppLogic::ReconstructLayerProxy::cache_reconstructed_feature_velocities(
		ReconstructionInfo &reconstruction_info,
		const double &reconstruction_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		const double &velocity_delta_time)
{
	// If they're already cached then nothing to do.
	if (reconstruction_info.cached_reconstructed_feature_velocities)
	{
		return reconstruction_info.cached_reconstructed_feature_velocities.get();
	}

	// Create empty vector of reconstructed feature velocities.
	reconstruction_info.cached_reconstructed_feature_velocities =
			std::vector<MultiPointVectorField::non_null_ptr_type>();

	// Calculate our velocities into our cache.
	reconstruction_info.cached_reconstructed_feature_velocities_handle =
			d_reconstruct_context.reconstruct_feature_velocities(
					reconstruction_info.cached_reconstructed_feature_velocities.get(),
					reconstruction_info.context_state,
					reconstruction_time,
					velocity_delta_time,
					velocity_delta_time_type);

	return reconstruction_info.cached_reconstructed_feature_velocities.get();
}


GPlatesAppLogic::ReconstructLayerProxy::ReconstructionInfo
GPlatesAppLogic::ReconstructLayerProxy::create_reconstruction_info(
		const reconstruction_cache_key_type &reconstruction_cache_key)
{
	const ReconstructParams &reconstruct_params = reconstruction_cache_key.second;

	//
	// Make sure the *specified* ReconstructParams matches the *current* ReconstructParams in regard to
	// whether it's reconstructing using topologies or not.
	//
	// This is because if we're *currently* using topologies then topology layers will not attempt to use us
	// (this layer) to find its topological sections, and we'll be potentially using topology layers to
	// help us reconstruct/deform features. That logic also helps avoid infinite cycles where a topology
	// layer checks it's up-to-date wrt to us which causes us to check them, and so on.
	// If we allow the caller to then reconstruct *without* using topologies (ie, by specifying a
	// ReconstructParams with a different 'get_reconstruct_using_topologies()' than our current ReconstructParams)
	// then that logic gets quite tricky. Currently we don't need it, so we won't implement it.
	//
	// However, if this assertion gets triggered then we will need to think about implementing it.
	//
	// Note: we test here since all ReconstructParams go through this function as part of caching reconstructions.
	//
	GPlatesGlobal::Assert<GPlatesGlobal::NotYetImplementedException>(
			reconstruct_params.get_reconstruct_using_topologies() ==
				d_current_reconstruct_params.get_reconstruct_using_topologies(),
			GPLATES_EXCEPTION_SOURCE);

	// See if we've already got a reconstruct context state for the specified reconstruct params
	// part of the key, ignoring the reconstruction time part.
	//
	// This is important because we don't want to create a new context for each new
	// reconstruction time (when the reconstruct parameters haven't changed) since, when geometries
	// are reconstructed using topologies, this results in excessive generation of expensive
	// reconstruction lookup tables in the reconstruct methods (inside the reconstruct context).
	const ReconstructContext::context_state_reference_type context_state_ref =
			get_or_create_reconstruct_context(reconstruct_params);

	return ReconstructionInfo(context_state_ref);
}


GPlatesAppLogic::ReconstructContext::context_state_reference_type
GPlatesAppLogic::ReconstructLayerProxy::get_or_create_reconstruct_context(
		const ReconstructParams &reconstruct_params)
{
	// First go through the sequence of mapped reconstruct context states and remove any expired entries.
	// This is to prevent the accumulation of expired entries over time.
	reconstruct_context_state_map_type::iterator context_state_iter = d_reconstruct_context_state_map.begin();
	while (context_state_iter != d_reconstruct_context_state_map.end())
	{
		reconstruct_context_state_map_type::iterator current_context_state_iter = context_state_iter;
		++context_state_iter;

		if (current_context_state_iter->second.expired())
		{
			d_reconstruct_context_state_map.erase(current_context_state_iter);
		}
	}

	// See if we've already got a reconstruct context state for the specified reconstruct params.
	ReconstructContext::context_state_weak_reference_type &context_state_weak_ref =
			d_reconstruct_context_state_map[reconstruct_params];

	ReconstructContext::context_state_reference_type context_state_ref;
	if (context_state_weak_ref.expired())
	{
		// Create a new reconstruct context state...
		//
		// The context state has either been released from all clients holding/referencing it
		// (eg, all cached ReconstructionInfo objects referencing it have been flushed)
		// or a context state was never created for the specified reconstruct parameters.
		// In both cases we create a new context state and map it to the reconstruct parameters.
		context_state_ref = d_reconstruct_context.create_context_state(
				get_reconstruct_method_context(reconstruct_params));

		// Associate the new context state with the ReconstructParams so we can find it again.
		context_state_weak_ref = context_state_ref;
	}
	else
	{
		// Use existing reconstruct context state...
		context_state_ref = context_state_weak_ref.lock();
	}

	return context_state_ref;
}
