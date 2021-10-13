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


void
GPlatesAppLogic::ReconstructLayerProxy::get_reconstructed_feature_geometries(
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
		const double &reconstruction_time)
{
	// See if the reconstruction time has changed.
	if (d_cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time))
	{
		// The reconstructed feature geometries are now invalid.
		reset_reconstructed_feature_geometry_caches();

		// Note that observers don't need to be updated when the time changes - if they
		// have reconstructed feature geometries for a different time they don't need
		// to be updated just because some other client requested a different time.
		d_cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
	}

	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	if (!d_cached_reconstructed_feature_geometries)
	{
		// Create empty vector of RFGs.
		d_cached_reconstructed_feature_geometries =
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type>();

		// Optimisation is to use the cached RFGs in the other cached format if they are available.
		if (d_cached_reconstructions)
		{
			// Copy the RFGs already cached into the this cached format.
			// The other format stores an RFG and a geometry property handle.
			// This format only needs the RFG.
			d_cached_reconstructed_feature_geometries->reserve(d_cached_reconstructions->size());
			BOOST_FOREACH(
					const ReconstructContext::Reconstruction &reconstruction,
					d_cached_reconstructions.get())
			{
				d_cached_reconstructed_feature_geometries->push_back(
						reconstruction.get_reconstructed_feature_geometry());
			}
		}
		else
		{
			// Reconstruct our features into our sequence of RFGs.
			d_reconstruct_context.reconstruct(
					d_cached_reconstructed_feature_geometries.get(),
					// Used to call when a reconstruction tree is needed for any time/anchor.
					d_current_reconstruction_layer_proxy.get_input_layer_proxy()->get_reconstruction_tree_creator(),
					// Also pass in the current time.
					reconstruction_time);
		}
	}

	// Append our cached RFGs to the caller's sequence.
	reconstructed_feature_geometries.insert(
			reconstructed_feature_geometries.end(),
			d_cached_reconstructed_feature_geometries->begin(),
			d_cached_reconstructed_feature_geometries->end());
}


void
GPlatesAppLogic::ReconstructLayerProxy::get_reconstructed_feature_geometries(
		std::vector<ReconstructContext::Reconstruction> &reconstructed_feature_geometries,
		const double &reconstruction_time)
{
	// See if the reconstruction time has changed.
	if (d_cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time))
	{
		// The reconstructed feature geometries are now invalid.
		reset_reconstructed_feature_geometry_caches();

		// Note that observers don't need to be updated when the time changes - if they
		// have reconstructed feature geometries for a different time they don't need
		// to be updated just because some other client requested a different time.
		d_cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
	}

	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	if (!d_cached_reconstructions)
	{
		// Create empty vector of RFGs.
		d_cached_reconstructions = std::vector<ReconstructContext::Reconstruction>();

		// Reconstruct our features into our sequence of RFGs.
		d_reconstruct_context.reconstruct(
				d_cached_reconstructions.get(),
				// Used to call when a reconstruction tree is needed for any time/anchor.
				d_current_reconstruction_layer_proxy.get_input_layer_proxy()->get_reconstruction_tree_creator(),
				// Also pass in the current time.
				reconstruction_time);
	}

	// Append our cached RFGs to the caller's sequence.
	reconstructed_feature_geometries.insert(
			reconstructed_feature_geometries.end(),
			d_cached_reconstructions->begin(),
			d_cached_reconstructions->end());
}


GPlatesAppLogic::ReconstructLayerProxy::reconstructed_feature_geometries_spatial_partition_type::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructLayerProxy::get_reconstructed_feature_geometries_spatial_partition(
		const double &reconstruction_time)
{
	// See if the reconstruction time has changed.
	if (d_cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time))
	{
		// The reconstructed feature geometries are now invalid.
		reset_reconstructed_feature_geometry_caches();

		// Note that observers don't need to be updated when the time changes - if they
		// have reconstructed feature geometries for a different time they don't need
		// to be updated just because some other client requested a different time.
		d_cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
	}

	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	if (!d_cached_reconstructed_feature_geometries_spatial_partition)
	{
		// Reconstruct our features into a sequence of RFGs.
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_feature_geometries;
		get_reconstructed_feature_geometries(reconstructed_feature_geometries, reconstruction_time);

		// Add the RFGs to a new spatial partition to return to the caller.
		d_cached_reconstructed_feature_geometries_spatial_partition =
				reconstructed_feature_geometries_spatial_partition_type::create(
						DEFAULT_SPATIAL_PARTITION_DEPTH);
		BOOST_FOREACH(
				const ReconstructedFeatureGeometry::non_null_ptr_type &rfg,
				reconstructed_feature_geometries)
		{
			// NOTE: To avoid reconstructing geometries on the CPU when it might not be needed
			// (graphics card can transform much faster so when only visualising it's not needed)
			// we add the *unreconstructed* geometry (and a finite rotation) to the spatial partition.
			// The spatial partition will rotate only the centroid of the *unreconstructed*
			// geometry (instead of reconstructing the entire geometry) and then use that as the
			// insertion location (along with the *unreconstructed* geometry's bounding circle extents).

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

				d_cached_reconstructed_feature_geometries_spatial_partition.get()->add(
						rfg, resolved_geometry, finite_rotation);
			}
			else
			{
				// It's not a finite rotation so we can't assume the geometry has rigidly rotated.
				// Hence we can't assume it's shape is the same and hence can't assume the
				// small circle bounding radius is the same.
				// So just get the reconstructed geometry and insert it into the spatial partition.
				// The appropriate bounding small circle will be generated for it when it's added.
				d_cached_reconstructed_feature_geometries_spatial_partition.get()->add(
						rfg, *rfg->reconstructed_geometry());
			}
		}
	}

	return d_cached_reconstructed_feature_geometries_spatial_partition.get();
}


GPlatesAppLogic::ReconstructLayerProxy::reconstructions_spatial_partition_type::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructLayerProxy::get_reconstructions_spatial_partition(
		const double &reconstruction_time)
{
	// See if the reconstruction time has changed.
	if (d_cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time))
	{
		// The reconstructed feature geometries are now invalid.
		reset_reconstructed_feature_geometry_caches();

		// Note that observers don't need to be updated when the time changes - if they
		// have reconstructed feature geometries for a different time they don't need
		// to be updated just because some other client requested a different time.
		d_cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
	}

	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	if (!d_cached_reconstructions_spatial_partition)
	{
		// Reconstruct our features into a sequence of RFGs.
		std::vector<ReconstructContext::Reconstruction> reconstructions;
		get_reconstructed_feature_geometries(reconstructions, reconstruction_time);

		// Add the RFGs to a new spatial partition to return to the caller.
		d_cached_reconstructions_spatial_partition =
				reconstructions_spatial_partition_type::create(
						DEFAULT_SPATIAL_PARTITION_DEPTH);
		BOOST_FOREACH(
				const ReconstructContext::Reconstruction &reconstruction,
				reconstructions)
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

				d_cached_reconstructions_spatial_partition.get()->add(
						reconstruction, resolved_geometry, finite_rotation);
			}
			else
			{
				// It's not a finite rotation so we can't assume the geometry has rigidly rotated.
				// Hence we can't assume it's shape is the same and hence can't assume the
				// small circle bounding radius is the same.
				// So just get the reconstructed geometry and insert it into the spatial partition.
				// The appropriate bounding small circle will be generated for it when it's added.
				d_cached_reconstructions_spatial_partition.get()->add(
						reconstruction, *rfg->reconstructed_geometry());
			}
		}
	}

	return d_cached_reconstructions_spatial_partition.get();
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
	d_current_reconstruction_time = reconstruction_time;

	// Note that we don't reset our caches because we only do that when the client
	// requests a reconstruction time that differs from the cached reconstruction time.
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
GPlatesAppLogic::ReconstructLayerProxy::set_current_reconstruct_params(
		const ReconstructParams &reconstruct_params)
{
	// Update the parameters to use for reconstructing.
	d_reconstruct_context.set_reconstruct_params(reconstruct_params);

	// The reconstructed feature geometries are now invalid.
	reset_reconstructed_feature_geometry_caches();

	// Polling observers need to update themselves.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::ReconstructLayerProxy::reset_reconstructed_feature_geometry_caches()
{
	// Clear any cached reconstructed feature geometries.
	d_cached_reconstructed_feature_geometries = boost::none;
	d_cached_reconstructions = boost::none;
	d_cached_reconstructed_feature_geometries_spatial_partition = boost::none;
	d_cached_reconstructions_spatial_partition = boost::none;
	d_cached_reconstruction_time = boost::none;
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
		// The velocities are now invalid.
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
