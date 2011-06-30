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

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include "ReconstructLayerProxy.h"

#include "maths/CubeQuadTreePartitionUtils.h"


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


GPlatesAppLogic::ReconstructLayerProxy::ReconstructLayerProxy(
		const ReconstructMethodRegistry &reconstruct_method_registry,
		const ReconstructParams &reconstruct_params,
		unsigned int max_num_reconstructions_in_cache) :
	d_reconstruct_method_registry(reconstruct_method_registry),
	d_reconstruct_context(reconstruct_method_registry),
	// Start off with a reconstruction layer proxy that does identity rotations.
	d_current_reconstruction_layer_proxy(ReconstructionLayerProxy::create()),
	d_current_reconstruction_time(0),
	d_current_reconstruct_params(reconstruct_params),
	d_reconstruction_cache(&create_empty_reconstruction_info, max_num_reconstructions_in_cache)
{
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
	ReconstructionInfo &reconstruction_info = d_reconstruction_cache.get_value(reconstruction_cache_key);

	// If the cached reconstruction info has not been initialised or has been evicted from the cache...
	if (!reconstruction_info.cached_reconstructed_feature_geometries)
	{
		// Reconstruct our features into a sequence of ReconstructContext::Reconstruction's.
		// It takes only slightly longer (eg, 7.4msec versus 7.22msec) to generate a sequence
		// of ReconstructContext::Reconstruction's versus a sequence of RFGs but it means if
		// another client then requests the ReconstructContext::Reconstruction's then they will
		// be cached and won't have to be calculated - doing it the other way around doesn't work.
		std::vector<ReconstructContext::Reconstruction> &reconstructions =
				cache_reconstructions(reconstruction_info, reconstruct_params, reconstruction_time);

		// Create empty vector of RFGs.
		reconstruction_info.cached_reconstructed_feature_geometries =
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type>();

		// Copy the RFGs already cached in the ReconstructContext::Reconstruction's into this cached format.
		// The ReconstructContext::Reconstruction's store an RFG and a geometry property handle.
		// This format only needs the RFG.
		reconstruction_info.cached_reconstructed_feature_geometries->reserve(reconstructions.size());
		BOOST_FOREACH(const ReconstructContext::Reconstruction &reconstruction, reconstructions)
		{
			reconstruction_info.cached_reconstructed_feature_geometries->push_back(
					reconstruction.get_reconstructed_feature_geometry());
		}
	}

	// Append our cached RFGs to the caller's sequence.
	reconstructed_feature_geometries.insert(
			reconstructed_feature_geometries.end(),
			reconstruction_info.cached_reconstructed_feature_geometries->begin(),
			reconstruction_info.cached_reconstructed_feature_geometries->end());

	return reconstruction_info.cached_reconstruct_handle.get();
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
	ReconstructionInfo &reconstruction_info = d_reconstruction_cache.get_value(reconstruction_cache_key);

	// If the cached reconstruction info has not been initialised or has been evicted from the cache...
	if (!reconstruction_info.cached_reconstructions)
	{
		cache_reconstructions(reconstruction_info, reconstruct_params, reconstruction_time);
	}

	// Append our cached RFGs to the caller's sequence.
	reconstructions.insert(
			reconstructions.end(),
			reconstruction_info.cached_reconstructions->begin(),
			reconstruction_info.cached_reconstructions->end());

	return reconstruction_info.cached_reconstruct_handle.get();
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
	ReconstructionInfo &reconstruction_info = d_reconstruction_cache.get_value(reconstruction_cache_key);

	// If the cached reconstruction info has not been initialised or has been evicted from the cache...
	if (!reconstruction_info.cached_reconstructed_feature_geometries_spatial_partition)
	{
		// Reconstruct our features into a spatial partition of ReconstructContext::Reconstruction's.
		// It takes only slightly longer to generate a spatial partition of
		// ReconstructContext::Reconstruction's versus a spatial partition of RFGs but it means if
		// another client then requests the ReconstructContext::Reconstruction's then they will
		// be cached and won't have to be calculated - doing it the other way around doesn't work.
		reconstructions_spatial_partition_type::non_null_ptr_to_const_type reconstructions_spatial_partition =
				cache_reconstructions_spatial_partition(
						reconstruction_info, reconstruct_params, reconstruction_time);

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
		*reconstruct_handle = reconstruction_info.cached_reconstruct_handle.get();
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
	ReconstructionInfo &reconstruction_info = d_reconstruction_cache.get_value(reconstruction_cache_key);

	// If the cached reconstruction info has not been initialised or has been evicted from the cache...
	if (!reconstruction_info.cached_reconstructions_spatial_partition)
	{
		cache_reconstructions_spatial_partition(
				reconstruction_info, reconstruct_params, reconstruction_time);
	}

	// If the caller wants the reconstruct handle then return it.
	if (reconstruct_handle)
	{
		*reconstruct_handle = reconstruction_info.cached_reconstruct_handle.get();
	}

	// Return the cached spatial partition.
	return reconstruction_info.cached_reconstructions_spatial_partition.get();
}


const std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> &
GPlatesAppLogic::ReconstructLayerProxy::get_present_day_geometries()
{
	if (!d_cached_present_day_geometries)
	{
		d_cached_present_day_geometries = d_reconstruct_context.get_present_day_geometries();
	}

	return d_cached_present_day_geometries.get();
}


const std::vector<boost::optional<GPlatesMaths::PolygonMesh::non_null_ptr_to_const_type> > &
GPlatesAppLogic::ReconstructLayerProxy::get_present_day_polygon_meshes()
{
	if (!d_cached_present_day_polygon_meshes)
	{
		// First get the present day geometries.
		const std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> &
				present_day_geometries = get_present_day_geometries();

		// Start off with an empty sequence of polygon meshes.
		d_cached_present_day_polygon_meshes =
				std::vector<boost::optional<GPlatesMaths::PolygonMesh::non_null_ptr_to_const_type> >();
		std::vector<boost::optional<GPlatesMaths::PolygonMesh::non_null_ptr_to_const_type> > &
				present_day_polygon_meshes = d_cached_present_day_polygon_meshes.get();
		present_day_polygon_meshes.reserve(present_day_geometries.size());

		BOOST_FOREACH(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &present_day_geometry,
				present_day_geometries)
		{
			// Create the polygon mesh from the present day geometry.
			// Note that the returned polygon mesh could be boost::none (but we add it anyway).
			present_day_polygon_meshes.push_back(
					GPlatesMaths::PolygonMesh::create(present_day_geometry));
		}
	}

	return d_cached_present_day_polygon_meshes.get();
}


GPlatesAppLogic::ReconstructLayerProxy::geometries_spatial_partition_type::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructLayerProxy::get_present_day_geometries_spatial_partition()
{
	if (!d_cached_present_day_geometries_spatial_partition)
	{
		// Get the present day geometries.
		const std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> &
				present_day_geometries = get_present_day_geometries();

		// Generate the location of each present day geometry in the spatial partition.
		// This is in case it's later requested by the client.
		d_cached_present_day_geometries_spatial_partition_locations =
				std::vector<GPlatesMaths::CubeQuadTreeLocation>();
		std::vector<GPlatesMaths::CubeQuadTreeLocation> &spatial_partition_locations =
				d_cached_present_day_geometries_spatial_partition_locations.get();

		// Start out creating a default-constructed location for each present day geometry.
		spatial_partition_locations.resize(present_day_geometries.size());

		// Add the present day geometries to a new spatial partition to return to the caller.
		d_cached_present_day_geometries_spatial_partition =
				geometries_spatial_partition_type::create(DEFAULT_SPATIAL_PARTITION_DEPTH);

		unsigned int present_day_geometry_index = 0;
		BOOST_FOREACH(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &present_day_geometry,
				present_day_geometries)
		{
			// The first argument is what's inserted into the partition.
			// The second argument is what determines the location at which to insert.
			d_cached_present_day_geometries_spatial_partition.get()->add(
					present_day_geometry,
					*present_day_geometry,
					// Write the location, at which the geometry is added, to our sequence...
					&spatial_partition_locations[present_day_geometry_index]);

			++present_day_geometry_index;
		}
	}

	return d_cached_present_day_geometries_spatial_partition.get();
}


const std::vector<GPlatesMaths::CubeQuadTreeLocation> &
GPlatesAppLogic::ReconstructLayerProxy::get_present_day_geometries_spatial_partition_locations()
{
	if (!d_cached_present_day_geometries_spatial_partition_locations)
	{
		// The locations are generated when the spatial partition is built.
		get_present_day_geometries_spatial_partition();
	}

	return d_cached_present_day_geometries_spatial_partition_locations.get();
}


GPlatesAppLogic::ReconstructionLayerProxy::non_null_ptr_type
GPlatesAppLogic::ReconstructLayerProxy::get_reconstruction_layer_proxy()
{
	return d_current_reconstruction_layer_proxy.get_input_layer_proxy();
}


const GPlatesUtils::SubjectToken &
GPlatesAppLogic::ReconstructLayerProxy::get_subject_token()
{
	// We've checked to see if any inputs have changed *except* the reconstruction layer proxy input.
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

	// Note that we don't invalidate our cache because if a reconstruction is
	// not cached for a requested reconstruction time then a new reconstruction is created.
	// But observers need to be aware that the default reconstruction time has changed.
	d_subject_token.invalidate();
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
	d_current_reconstruct_params = reconstruct_params;

	// Note that we don't invalidate our cache because if a reconstruction is
	// not cached for a requested reconstruct params then a new reconstruction is created.
	// But observers need to be aware that the default reconstruct params have changed.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::ReconstructLayerProxy::set_current_reconstruction_layer_proxy(
		const ReconstructionLayerProxy::non_null_ptr_type &reconstruction_layer_proxy)
{
	d_current_reconstruction_layer_proxy.set_input_layer_proxy(reconstruction_layer_proxy);

	// The reconstructed feature geometries are now invalid.
	reset_reconstructed_feature_geometry_caches();

	// Polling observers need to update themselves.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::ReconstructLayerProxy::add_reconstructable_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	d_current_reconstructable_feature_collections.push_back(feature_collection);

	// Get the reconstruct context to reassign reconstruct methods for each feature.
	d_reconstruct_context.reassign_reconstruct_methods_to_features(
			d_reconstruct_method_registry,
			d_current_reconstructable_feature_collections);

	// The reconstructed feature geometries are now invalid.
	reset_reconstructed_feature_geometry_caches();

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
	d_current_reconstructable_feature_collections.erase(
			std::find(
					d_current_reconstructable_feature_collections.begin(),
					d_current_reconstructable_feature_collections.end(),
					feature_collection));

	// Get the reconstruct context to reassign reconstruct methods for each feature.
	d_reconstruct_context.reassign_reconstruct_methods_to_features(
			d_reconstruct_method_registry,
			d_current_reconstructable_feature_collections);

	// The reconstructed feature geometries are now invalid.
	reset_reconstructed_feature_geometry_caches();

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
	// Get the reconstruct context to reassign reconstruct methods for each feature.
	d_reconstruct_context.reassign_reconstruct_methods_to_features(
			d_reconstruct_method_registry,
			d_current_reconstructable_feature_collections);

	// The reconstructed feature geometries are now invalid.
	reset_reconstructed_feature_geometry_caches();

	// Polling observers need to update themselves.
	d_subject_token.invalidate();

	// Anything dependent on the reconstructable feature collections is now invalid.
	reset_reconstructable_feature_collection_caches();

	// Polling observers need to update themselves if they depend on present day geometries, for example.
	d_reconstructable_feature_collections_subject_token.invalidate();
}


void
GPlatesAppLogic::ReconstructLayerProxy::reset_reconstructed_feature_geometry_caches()
{
	// Clear any cached reconstructed feature geometries for any reconstruction times and reconstruct params.
	d_reconstruction_cache.clear();
}


void
GPlatesAppLogic::ReconstructLayerProxy::reset_reconstructable_feature_collection_caches()
{
	// Clear anything that depends on the reconstructable feature collections.
	d_cached_present_day_geometries = boost::none;
	d_cached_present_day_polygon_meshes = boost::none;
	d_cached_present_day_geometries_spatial_partition = boost::none;
	d_cached_present_day_geometries_spatial_partition_locations = boost::none;
}


template <class InputLayerProxyWrapperType>
void
GPlatesAppLogic::ReconstructLayerProxy::check_input_layer_proxy(
		InputLayerProxyWrapperType &input_layer_proxy_wrapper)
{
	// See if the input layer proxy has changed.
	if (!input_layer_proxy_wrapper.is_up_to_date())
	{
		// The reconstructed feature geometries are now invalid.
		reset_reconstructed_feature_geometry_caches();

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
}


std::vector<GPlatesAppLogic::ReconstructContext::Reconstruction> &
GPlatesAppLogic::ReconstructLayerProxy::cache_reconstructions(
		ReconstructionInfo &reconstruction_info,
		const ReconstructParams &reconstruct_params,
		const double &reconstruction_time)
{
	// If they're already cached then nothing to do.
	if (reconstruction_info.cached_reconstructions)
	{
		return reconstruction_info.cached_reconstructions.get();
	}

	// Create empty vector of reconstructions.
	reconstruction_info.cached_reconstructions = std::vector<ReconstructContext::Reconstruction>();

	// Reconstruct our features into our sequence of cached RFGs.
	reconstruction_info.cached_reconstruct_handle = d_reconstruct_context.reconstruct(
			reconstruction_info.cached_reconstructions.get(),
			reconstruct_params,
			// Used to call when a reconstruction tree is needed for any time/anchor.
			d_current_reconstruction_layer_proxy.get_input_layer_proxy()->get_reconstruction_tree_creator(),
			// Also pass in the current time.
			reconstruction_time);

	return reconstruction_info.cached_reconstructions.get();
}


GPlatesAppLogic::ReconstructLayerProxy::reconstructions_spatial_partition_type::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructLayerProxy::cache_reconstructions_spatial_partition(
		ReconstructionInfo &reconstruction_info,
		const ReconstructParams &reconstruct_params,
		const double &reconstruction_time)
{
	// If they're already cached then nothing to do.
	if (reconstruction_info.cached_reconstructions_spatial_partition)
	{
		return reconstruction_info.cached_reconstructions_spatial_partition.get();
	}

	// Reconstruct our features into a sequence of ReconstructContext::Reconstruction's.
	std::vector<ReconstructContext::Reconstruction> &reconstructions =
			cache_reconstructions(
					reconstruction_info, reconstruct_params, reconstruction_time);

	//PROFILE_BLOCK("ReconstructLayerProxy::get_reconstructions_spatial_partition: ");

	// Add the RFGs to a new spatial partition to return to the caller.
	reconstruction_info.cached_reconstructions_spatial_partition =
			reconstructions_spatial_partition_type::create(
					DEFAULT_SPATIAL_PARTITION_DEPTH);
	BOOST_FOREACH(const ReconstructContext::Reconstruction &reconstruction, reconstructions)
	{
		// NOTE: To avoid reconstructing geometries on the CPU when it might not be needed
		// (graphics card can transform much faster so when only visualising it's not needed)
		// we add the *unreconstructed* geometry (and a finite rotation) to the spatial partition.
		// The spatial partition will rotate only the centroid of the *unreconstructed*
		// geometry (instead of reconstructing the entire geometry) and then use that as the
		// insertion location (along with the *unreconstructed* geometry's bounding circle extents).

		const ReconstructedFeatureGeometry::non_null_ptr_type &rfg =
				reconstruction.get_reconstructed_feature_geometry();

		// See if the reconstruction can be represented as a finite rotation.
		const boost::optional<ReconstructedFeatureGeometry::FiniteRotationReconstruction> &
				finite_rotation_reconstruction = rfg->finite_rotation_reconstruction();
		if (finite_rotation_reconstruction)
		{
			// The resolved geometry is the *unreconstructed* geometry (but still possibly
			// the result of a lookup of a time-dependent geometry property).
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
			// Hence we can't assume it's shape is the same and hence can't assume the
			// small circle bounding radius is the same.
			// So just get the reconstructed geometry and insert it into the spatial partition.
			// The appropriate bounding small circle will be generated for it when it's added.
			reconstruction_info.cached_reconstructions_spatial_partition.get()->add(
					reconstruction, *rfg->reconstructed_geometry());
		}
	}

	return reconstruction_info.cached_reconstructions_spatial_partition.get();
}
