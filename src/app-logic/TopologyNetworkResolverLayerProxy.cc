/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
 * Copyright (C) 2013 California Institute of Technology
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
#include <boost/utility/in_place_factory.hpp>

#include "TopologyNetworkResolverLayerProxy.h"

#include "GeometryUtils.h"
#include "ReconstructionGeometryUtils.h"
#include "ResolvedTopologicalLine.h"
#include "ResolvedTopologicalNetwork.h"
#include "ResolvedVertexSourceInfo.h"
#include "TopologyInternalUtils.h"
#include "TopologyUtils.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/MathsUtils.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * Filter out features that are topological networks.
		 *
		 * This function is actually reasonably expensive, so it's best to only call this when
		 * the layer input feature collections change.
		 */
		void
		find_topological_network_features(
				std::vector<GPlatesModel::FeatureHandle::weak_ref> &topological_network_features,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &feature_collections)
		{
			PROFILE_FUNC();

			// Iterate over the current feature collections.
			std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>::const_iterator feature_collections_iter =
					feature_collections.begin();
			std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>::const_iterator feature_collections_end =
					feature_collections.end();
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

						if (!feature.is_valid())
						{
							continue;
						}

						if (TopologyUtils::is_topological_network_feature(feature))
						{
							topological_network_features.push_back(feature);
						}
					}
				}
			}
		}
	}
}


GPlatesAppLogic::TopologyNetworkResolverLayerProxy::TopologyNetworkResolverLayerProxy(
		const TopologyNetworkParams &topology_network_params) :
	d_current_reconstruction_time(0),
	d_current_topology_network_params(topology_network_params)
{
	// Defined in ".cc" file because...
	// non_null_ptr destructors require complete type of class they're referring to.
	// Compiler will call destructors of already-constructed members if constructor throws exception.
}


GPlatesAppLogic::TopologyNetworkResolverLayerProxy::~TopologyNetworkResolverLayerProxy()
{
	// Defined in ".cc" file because...
	// non_null_ptr destructors require complete type of class they're referring to.
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::get_resolved_topological_networks(
		std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks,
		const TopologyNetworkParams &topology_network_params,
		const double &reconstruction_time)
{
	// See if the reconstruction time has changed.
	if (d_cached_resolved_networks.cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time) ||
		d_cached_resolved_networks.cached_topology_network_params != topology_network_params)
	{
		// The resolved networks are now invalid.
		d_cached_resolved_networks.invalidate();

		// The new time and params that our cache will correspond to.
		d_cached_resolved_networks.cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
		d_cached_resolved_networks.cached_topology_network_params = topology_network_params;
	}

	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	if (!d_cached_resolved_networks.cached_resolved_topological_networks)
	{
		cache_resolved_topological_networks(topology_network_params, reconstruction_time);
	}

	// Append our cached resolved topological networks to the caller's sequence.
	resolved_topological_networks.insert(
			resolved_topological_networks.end(),
			d_cached_resolved_networks.cached_resolved_topological_networks->begin(),
			d_cached_resolved_networks.cached_resolved_topological_networks->end());

	return d_cached_resolved_networks.cached_reconstruct_handle.get();
}


GPlatesAppLogic::TopologyReconstruct::resolved_network_time_span_type::non_null_ptr_to_const_type
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::get_resolved_network_time_span(
		const TimeSpanUtils::TimeRange &time_range,
		const TopologyNetworkParams &topology_network_params)
{
	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	// See if the topology network params have changed.
	if (d_cached_time_span.cached_topology_network_params != topology_network_params)
	{
		// The time span is now invalid.
		d_cached_time_span.invalidate();

		// The new params that our cache will correspond to.
		d_cached_time_span.cached_topology_network_params = topology_network_params;
	}

	// If the resolved network time span did not get invalidated (due to updated inputs or changed params)
	// then see if the time range has changed (or the topology network params have changed).
	if (d_cached_time_span.cached_resolved_network_time_span)
	{
		const TimeSpanUtils::TimeRange &cached_time_range =
				d_cached_time_span.cached_resolved_network_time_span.get()->get_time_range();

		if (!GPlatesMaths::are_geo_times_approximately_equal(
				time_range.get_begin_time(),
				cached_time_range.get_begin_time()) ||
			!GPlatesMaths::are_geo_times_approximately_equal(
				time_range.get_end_time(),
				cached_time_range.get_end_time()) ||
			!GPlatesMaths::are_geo_times_approximately_equal(
				time_range.get_time_increment(),
				cached_time_range.get_time_increment()))
		{
			// The resolved network time span has a different time range.
			// Instead of invalidating the current resolved network time span we will attempt
			// to build a new one from the existing one since they may have time slots in common.
			// Note that we've already checked our input proxies so we know that the current
			// resolved network time span still contains valid resolved networks.
			cache_resolved_network_time_span(time_range, topology_network_params);

			return d_cached_time_span.cached_resolved_network_time_span.get();
		}
	}

	if (!d_cached_time_span.cached_resolved_network_time_span)
	{
		cache_resolved_network_time_span(time_range, topology_network_params);
	}

	return d_cached_time_span.cached_resolved_network_time_span.get();
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::get_resolved_topological_network_velocities(
		std::vector<MultiPointVectorField::non_null_ptr_type> &resolved_topological_network_velocities,
		const TopologyNetworkParams &topology_network_params,
		const double &reconstruction_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		const double &velocity_delta_time)
{
	// See if the reconstruction time has changed.
	if (d_cached_resolved_networks.cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time) ||
		d_cached_resolved_networks.cached_topology_network_params != topology_network_params)
	{
		// The resolved networks are now invalid.
		d_cached_resolved_networks.invalidate();

		// The new time and params that our cache will correspond to.
		d_cached_resolved_networks.cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
		d_cached_resolved_networks.cached_topology_network_params = topology_network_params;
	}

	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	// If the velocity delta time parameters have changed then remove the velocities from the cache.
	if (d_cached_resolved_networks.cached_velocity_delta_time_params !=
		std::make_pair(velocity_delta_time_type, GPlatesMaths::real_t(velocity_delta_time)))
	{
		d_cached_resolved_networks.cached_resolved_topological_network_velocities = boost::none;

		d_cached_resolved_networks.cached_velocity_delta_time_params =
				std::make_pair(velocity_delta_time_type, GPlatesMaths::real_t(velocity_delta_time));
	}

	if (!d_cached_resolved_networks.cached_resolved_topological_network_velocities)
	{
		// First get/create the resolved topological networks.
		cache_resolved_topological_networks(topology_network_params, reconstruction_time);

		// Create empty vector of resolved topological network velocities.
		d_cached_resolved_networks.cached_resolved_topological_network_velocities =
				std::vector<MultiPointVectorField::non_null_ptr_type>();

		// Create our topological network velocities.
		d_cached_resolved_networks.cached_velocities_handle =
				create_resolved_topological_network_velocities(
						d_cached_resolved_networks.cached_resolved_topological_network_velocities.get(),
						d_cached_resolved_networks.cached_resolved_topological_networks.get(),
						reconstruction_time,
						velocity_delta_time_type,
						velocity_delta_time);
	}

	// Append our cached resolved topological network velocities to the caller's sequence.
	resolved_topological_network_velocities.insert(
			resolved_topological_network_velocities.end(),
			d_cached_resolved_networks.cached_resolved_topological_network_velocities->begin(),
			d_cached_resolved_networks.cached_resolved_topological_network_velocities->end());

	return d_cached_resolved_networks.cached_velocities_handle.get();
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::get_current_topological_network_features(
		std::vector<GPlatesModel::FeatureHandle::weak_ref> &topological_network_features) const
{
	topological_network_features.insert(
			topological_network_features.end(),
			d_current_topological_network_features.begin(),
			d_current_topological_network_features.end());
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::get_current_features(
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


void
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::get_current_dependent_topological_sections(
		std::set<GPlatesModel::FeatureId> &dependent_topological_sections) const
{
	// NOTE: We don't need to call 'check_input_layer_proxies()' because the feature IDs come from
	// our topological features (not the dependent topological section layers).

	dependent_topological_sections.insert(
			d_dependent_topological_sections.get_topological_section_feature_ids().begin(),
			d_dependent_topological_sections.get_topological_section_feature_ids().end());
}


const GPlatesUtils::SubjectToken &
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::get_subject_token()
{
	// We've checked to see if any inputs have changed except the reconstruction and
	// reconstruct layer proxy inputs.
	// This is because we get notified of all changes to input except input layer proxies which
	// we have to poll to see if they changed since we last accessed them - so we do that now.
	check_input_layer_proxies();

	return d_subject_token;
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::set_current_reconstruction_time(
		const double &reconstruction_time)
{
	d_current_reconstruction_time = reconstruction_time;

	// Note that we don't reset our caches because we only do that when the client
	// requests a reconstruction time that differs from the cached reconstruction time.
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::set_current_topology_network_params(
		const TopologyNetworkParams &topology_network_params)
{
	if (d_current_topology_network_params == topology_network_params)
	{
		// The current params haven't changed so avoid updating any observers unnecessarily.
		return;
	}
	d_current_topology_network_params = topology_network_params;

	// Note that we don't invalidate our resolved topological networks cache because they aren't
	// cached for a requested params then a new set is created.
	// Observers need to be aware that the default params have changed.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::set_current_topological_sections_layer_proxies(
		const std::vector<ReconstructLayerProxy::non_null_ptr_type> &reconstructed_geometry_topological_sections_layer_proxies,
		const std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> &resolved_line_topological_sections_layer_proxies)
{
	bool invalidate_cache = false;

	// Filter out layers that use topologies to reconstruct. These layers cannot supply
	// topological sections because they use topology layers thus creating a cyclic dependency.
	std::vector<ReconstructLayerProxy::non_null_ptr_type> valid_reconstructed_geometry_topological_sections_layer_proxies;
	BOOST_FOREACH(
			const ReconstructLayerProxy::non_null_ptr_type &reconstructed_geometry_topological_sections_layer_proxy,
			reconstructed_geometry_topological_sections_layer_proxies)
	{
		if (!reconstructed_geometry_topological_sections_layer_proxy->using_topologies_to_reconstruct())
		{
			valid_reconstructed_geometry_topological_sections_layer_proxies.push_back(
					reconstructed_geometry_topological_sections_layer_proxy);
		}
	}

	if (d_current_reconstructed_geometry_topological_sections_layer_proxies.set_input_layer_proxies(
			valid_reconstructed_geometry_topological_sections_layer_proxies))
	{
		// The topological section layers are different than last time.
		// If the *dependent* layers are different then cache invalidation is necessary.
		// Dependent means the currently cached resolved networks (and time spans) use topological sections from the specified layers.
		if (d_dependent_topological_sections.set_topological_section_layers(
				valid_reconstructed_geometry_topological_sections_layer_proxies))
		{
			invalidate_cache = true;
		}
	}

	if (d_current_resolved_line_topological_sections_layer_proxies.set_input_layer_proxies(
			resolved_line_topological_sections_layer_proxies))
	{
		// The topological section layers are different than last time.
		// If the *dependent* layers are different then cache invalidation is necessary.
		// Dependent means the currently cached resolved networks (and time spans) use topological sections from the specified layers.
		if (d_dependent_topological_sections.set_topological_section_layers(
				resolved_line_topological_sections_layer_proxies))
		{
			invalidate_cache = true;
		}
	}

	if (invalidate_cache)
	{
		// All resolved topological networks are now invalid.
		reset_cache();

		// Polling observers need to update themselves with respect to us.
		d_subject_token.invalidate();
	}
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::add_topological_network_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	d_current_feature_collections.push_back(feature_collection);

	// Not all features will necessarily be topological, and those that are topological will not
	// necessarily all be topological networks.
	d_current_topological_network_features.clear();
	find_topological_network_features(
			d_current_topological_network_features,
			d_current_feature_collections);

	// Set the feature IDs of topological sections referenced by our resolved networks for *all* times.
	d_dependent_topological_sections.set_topological_section_feature_ids(
			d_current_topological_network_features,
			TopologyGeometry::NETWORK);

	// The resolved topological networks are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::remove_topological_network_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// Erase the feature collection from our list.
	d_current_feature_collections.erase(
			std::find(
					d_current_feature_collections.begin(),
					d_current_feature_collections.end(),
					feature_collection));

	// Not all features will necessarily be topological, and those that are topological will not
	// necessarily all be topological networks.
	d_current_topological_network_features.clear();
	find_topological_network_features(
			d_current_topological_network_features,
			d_current_feature_collections);

	// Set the feature IDs of topological sections referenced by our resolved networks for *all* times.
	d_dependent_topological_sections.set_topological_section_feature_ids(
			d_current_topological_network_features,
			TopologyGeometry::NETWORK);

	// The resolved topological networks are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::modified_topological_network_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// Not all features will necessarily be topological, and those that are topological will not
	// necessarily all be topological networks.
	d_current_topological_network_features.clear();
	find_topological_network_features(
			d_current_topological_network_features,
			d_current_feature_collections);

	// Set the feature IDs of topological sections referenced by our resolved networks for *all* times.
	d_dependent_topological_sections.set_topological_section_feature_ids(
			d_current_topological_network_features,
			TopologyGeometry::NETWORK);

	// The resolved topological networks are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::reset_cache()
{
	// Clear any cached resolved topological networks.
	d_cached_resolved_networks.invalidate();
	d_cached_time_span.invalidate();
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::check_input_layer_proxies()
{
	// See if any reconstructed geometry topological section layer proxies have changed.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &rfg_topological_sections_layer_proxy,
			d_current_reconstructed_geometry_topological_sections_layer_proxies)
	{
		// Filter out layers that use topologies to reconstruct. These layers cannot supply
		// topological sections because they use topology layers thus creating a cyclic dependency.
		//
		// This also avoids infinite recursion by not checking if the layer is up-to-date (which might then check us, etc).
		//
		// Normally this layer would get excluded when the topological section layers are set, but that only happens when
		// a new reconstruction is performed and we might get called just before that happens, so we need to exclude here also.
		if (rfg_topological_sections_layer_proxy.get_input_layer_proxy()->using_topologies_to_reconstruct())
		{
			continue;
		}

		if (rfg_topological_sections_layer_proxy.is_up_to_date())
		{
			continue;
		}

		// If any cached resolved networks (including time spans) depend on these topological sections
		// then we need to invalidate our cache.
		//
		// Typically our dependency layers include all reconstruct/resolved-geometry layers
		// due to the usual global search for topological section features. However this means
		// layers that don't contribute topological sections will trigger unnecessary cache flushes
		// which is especially noticeable in the case of rebuilding network time spans.
		// To avoid this we check if any topological sections from a layer can actually contribute.
		if (d_dependent_topological_sections.update_topological_section_layer(
				rfg_topological_sections_layer_proxy.get_input_layer_proxy()))
		{
			// The networks are now invalid.
			reset_cache();

			// Polling observers need to update themselves with respect to us.
			d_subject_token.invalidate();
		}

		// We're now up-to-date with respect to the input layer proxy.
		rfg_topological_sections_layer_proxy.set_up_to_date();
	}

	// See if any resolved geometry topological section layer proxies have changed.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<TopologyGeometryResolverLayerProxy> &rtl_topological_sections_layer_proxy,
			d_current_resolved_line_topological_sections_layer_proxies)
	{
		if (rtl_topological_sections_layer_proxy.is_up_to_date())
		{
			continue;
		}

		// If any cached resolved networks (including time spans) depend on these topological sections
		// then we need to invalidate our cache.
		//
		// Typically our dependency layers include all reconstruct/resolved-geometry layers
		// due to the usual global search for topological section features. However this means
		// layers that don't contribute topological sections will trigger unnecessary cache flushes
		// which is especially noticeable in the case of rebuilding network time spans.
		// To avoid this we check if any topological sections from a layer can actually contribute.
		if (d_dependent_topological_sections.update_topological_section_layer(
				rtl_topological_sections_layer_proxy.get_input_layer_proxy()))
		{
			// The networks are now invalid.
			reset_cache();

			// Polling observers need to update themselves with respect to us.
			d_subject_token.invalidate();
		}

		// We're now up-to-date with respect to the input layer proxy.
		rtl_topological_sections_layer_proxy.set_up_to_date();
	}
}


std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type> &
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::cache_resolved_topological_networks(
		const TopologyNetworkParams &topology_network_params,
		const double &reconstruction_time)
{
	// If they're already cached then nothing to do.
	if (d_cached_resolved_networks.cached_resolved_topological_networks)
	{
		return d_cached_resolved_networks.cached_resolved_topological_networks.get();
	}

	// Create empty vector of resolved topological networks.
	d_cached_resolved_networks.cached_resolved_topological_networks =
			std::vector<ResolvedTopologicalNetwork::non_null_ptr_type>();

	// First see if we've already cached the current reconstruction time (and topology network params)
	// in the resolved network time span.
	if (d_cached_time_span.cached_resolved_network_time_span &&
		d_cached_time_span.cached_topology_network_params == topology_network_params)
	{
		// If there's a time slot in the time span that matches the reconstruction time
		// then we can re-use the resolved networks in that time slot.
		const boost::optional<unsigned int> time_slot =
				d_cached_time_span.cached_resolved_network_time_span.get()->get_time_range().get_time_slot(reconstruction_time);
		if (time_slot)
		{
			// Extract the resolved topological networks for the reconstruction time.
			boost::optional<TopologyReconstruct::rtn_seq_type &> resolved_topological_networks =
					d_cached_time_span.cached_resolved_network_time_span.get()->get_sample_in_time_slot(time_slot.get());
			if (resolved_topological_networks)
			{
				d_cached_resolved_networks.cached_resolved_topological_networks = resolved_topological_networks.get();

				// Get the reconstruct handle from one of the resolved networks (if any).
				if (!d_cached_resolved_networks.cached_resolved_topological_networks->empty())
				{
					boost::optional<ReconstructHandle::type> reconstruct_handle =
							d_cached_resolved_networks.cached_resolved_topological_networks->front()
									->get_reconstruct_handle();
					if (reconstruct_handle)
					{
						d_cached_resolved_networks.cached_reconstruct_handle = reconstruct_handle.get();
					}
					else
					{
						// RTN doesn't have a reconstruct handle - this shouldn't happen.
						d_cached_resolved_networks.cached_reconstruct_handle =
								ReconstructHandle::get_next_reconstruct_handle();
					}
				}
				else
				{
					// There will be no reconstructed/resolved networks for this handle.
					d_cached_resolved_networks.cached_reconstruct_handle =
							ReconstructHandle::get_next_reconstruct_handle();
				}

				return d_cached_resolved_networks.cached_resolved_topological_networks.get();
			}
		}
	}

	// Generate the resolved topological networks for the reconstruction time.
	d_cached_resolved_networks.cached_reconstruct_handle =
			create_resolved_topological_networks(
					d_cached_resolved_networks.cached_resolved_topological_networks.get(),
					topology_network_params,
					reconstruction_time);

	return d_cached_resolved_networks.cached_resolved_topological_networks.get();
}


GPlatesAppLogic::TopologyReconstruct::resolved_network_time_span_type::non_null_ptr_to_const_type
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::cache_resolved_network_time_span(
		const TimeSpanUtils::TimeRange &time_range,
		const TopologyNetworkParams &topology_network_params)
{
	//PROFILE_FUNC();

	// If one is already cached then attempt to re-use any time slots in common with the
	// new time range. If one is already cached then it contains valid resolved networks
	// - it's just that the time range has changed.
	boost::optional<TopologyReconstruct::resolved_network_time_span_type::non_null_ptr_type>
			prev_resolved_network_time_span = d_cached_time_span.cached_resolved_network_time_span;

	// Create an empty resolved network time span.
	d_cached_time_span.cached_resolved_network_time_span =
			TopologyReconstruct::resolved_network_time_span_type::create(time_range);

	const unsigned int num_time_slots = time_range.get_num_time_slots();

	// As a performance optimisation, for all our topological sections input layers we request a
	// reconstruction tree creator with a cache size the same as the resolved network time span
	// (plus one for possible extra time step).
	// This ensures we don't get a noticeable slowdown when the time span range exceeds the
	// size of the cache in the reconstruction layer proxy.
	// We don't actually use the returned ReconstructionTreeCreator here but by specifying a
	// cache size hint we set the size of its internal reconstruction tree cache.

	std::vector<ReconstructLayerProxy::non_null_ptr_type> dependent_reconstructed_geometry_topological_sections_layers;
	d_dependent_topological_sections.get_dependent_topological_section_layers(dependent_reconstructed_geometry_topological_sections_layers);
	BOOST_FOREACH(
			const ReconstructLayerProxy::non_null_ptr_type &reconstructed_geometry_topological_sections_layer_proxy,
			dependent_reconstructed_geometry_topological_sections_layers)
	{
		reconstructed_geometry_topological_sections_layer_proxy
				->get_current_reconstruction_layer_proxy()->get_reconstruction_tree_creator(num_time_slots + 1);
	}

	std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> dependent_resolved_line_topological_sections_layers;
	d_dependent_topological_sections.get_dependent_topological_section_layers(dependent_resolved_line_topological_sections_layers);
	BOOST_FOREACH(
			const TopologyGeometryResolverLayerProxy::non_null_ptr_type &resolved_line_topological_sections_layer_proxy,
			dependent_resolved_line_topological_sections_layers)
	{
		resolved_line_topological_sections_layer_proxy
				->get_current_reconstruction_layer_proxy()->get_reconstruction_tree_creator(num_time_slots + 1);
	}

	// Iterate over the time slots of the time span and fill in the resolved topological networks.
	for (unsigned int time_slot = 0; time_slot < num_time_slots; ++time_slot)
	{
		const double time = time_range.get_time(time_slot);

		// Attempt to re-use a time slot of the previous resolved network time span (if any).
		if (prev_resolved_network_time_span)
		{
			// See if the time matches a time slot of the previous resolved network time span.
			const TimeSpanUtils::TimeRange prev_time_range = prev_resolved_network_time_span.get()->get_time_range();
			boost::optional<unsigned int> prev_time_slot = prev_time_range.get_time_slot(time);
			if (prev_time_slot)
			{
				// Get the resolved topological networks from the previous resolved network time span.
				boost::optional<TopologyReconstruct::rtn_seq_type &> prev_resolved_topological_networks =
						prev_resolved_network_time_span.get()->get_sample_in_time_slot(prev_time_slot.get());
				if (prev_resolved_topological_networks)
				{
					d_cached_time_span.cached_resolved_network_time_span.get()->set_sample_in_time_slot(
							prev_resolved_topological_networks.get(),
							time_slot);

					// Continue to the next time slot.
					continue;
				}
			}
		}

		// Create the resolved topological networks for the current time slot.
		std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> resolved_topological_networks;
		create_resolved_topological_networks(resolved_topological_networks, topology_network_params, time);

		d_cached_time_span.cached_resolved_network_time_span.get()->set_sample_in_time_slot(
				resolved_topological_networks,
				time_slot);
	}

	return d_cached_time_span.cached_resolved_network_time_span.get();
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::create_resolved_topological_networks(
		std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks,
		const TopologyNetworkParams &topology_network_params,
		const double &reconstruction_time)
{
	// Get the *dependent* topological section layers.
	std::vector<ReconstructLayerProxy::non_null_ptr_type> dependent_reconstructed_geometry_topological_sections_layers;
	std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> dependent_resolved_line_topological_sections_layers;
	d_dependent_topological_sections.get_dependent_topological_section_layers(dependent_reconstructed_geometry_topological_sections_layers);
	d_dependent_topological_sections.get_dependent_topological_section_layers(dependent_resolved_line_topological_sections_layers);

	// If we have no topological network features or there are no topological section layers then we
	// can't get any topological sections and we can't resolve any topological networks.
	if (d_current_topological_network_features.empty() ||
		(dependent_reconstructed_geometry_topological_sections_layers.empty() &&
			dependent_resolved_line_topological_sections_layers.empty()))
	{
		// There will be no resolved networks for this handle.
		return ReconstructHandle::get_next_reconstruct_handle();
	}

	//
	// Generate the resolved topological networks for the reconstruction time.
	//

	std::vector<ReconstructHandle::type> topological_geometry_reconstruct_handles;

	// Find the topological section feature IDs referenced by topological networks for the *current* reconstruction time.
	//
	// This is an optimisation that avoids unnecessary reconstructions. Only those topological sections referenced
	// by networks that exist at the current reconstruction time are reconstructed (this saves quite a bit of time).
	std::set<GPlatesModel::FeatureId> topological_sections_referenced;
	TopologyInternalUtils::find_topological_sections_referenced(
			topological_sections_referenced,
			d_current_topological_network_features,
			TopologyGeometry::NETWORK,
			reconstruction_time);

	// Topological boundary sections and/or interior geometries that are reconstructed static features...
	// We're ensuring that all potential (reconstructed geometry) topological-referenced geometries are
	// reconstructed before we resolve topological networks (which reference them indirectly via feature-id).
	std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> topologically_referenced_reconstructed_geometries;
	BOOST_FOREACH(
			const ReconstructLayerProxy::non_null_ptr_type &reconstructed_geometry_topological_sections_layer_proxy,
			dependent_reconstructed_geometry_topological_sections_layers)
	{
		// Reconstruct only the referenced topological section RFGs.
		//
		// This is an optimisation that avoids unnecessary reconstructions. Only those topological sections referenced
		// by networks that exist at the current reconstruction time are reconstructed (this saves quite a bit of time).
		const ReconstructHandle::type reconstruct_handle =
				reconstructed_geometry_topological_sections_layer_proxy
						->get_reconstructed_topological_sections(
								topologically_referenced_reconstructed_geometries,
								topological_sections_referenced,
								reconstruction_time);

		// Add the reconstruct handle to our list.
		topological_geometry_reconstruct_handles.push_back(reconstruct_handle);
	}

	// Topological boundary sections and/or interior geometries that are resolved topological lines...
	// We're ensuring that all potential (resolved line) topologically-referenced geometries are
	// resolved before we resolve topological networks (which reference them indirectly via feature-id).
	std::vector<ResolvedTopologicalLine::non_null_ptr_type> topologically_referenced_resolved_lines;
	BOOST_FOREACH(
			const TopologyGeometryResolverLayerProxy::non_null_ptr_type &resolved_line_topological_sections_layer_proxy,
			dependent_resolved_line_topological_sections_layers)
	{
		// Reconstruct only the referenced topological section resolved lines.
		//
		// This is an optimisation that avoids unnecessary reconstructions. Only those topological sections
		// referenced by boundaries that exist at the current reconstruction time are reconstructed.
		const ReconstructHandle::type reconstruct_handle =
				resolved_line_topological_sections_layer_proxy
						->get_resolved_topological_sections(
								topologically_referenced_resolved_lines,
								topological_sections_referenced,
								reconstruction_time);

		// Add the reconstruct handle to our list.
		topological_geometry_reconstruct_handles.push_back(reconstruct_handle);
	}

	// Resolve our network features into our sequence of resolved topological networks.
	return TopologyUtils::resolve_topological_networks(
			resolved_topological_networks,
			reconstruction_time,
			d_current_topological_network_features,
			topological_geometry_reconstruct_handles,
			topology_network_params);
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::create_resolved_topological_network_velocities(
		std::vector<MultiPointVectorField::non_null_ptr_type> &resolved_topological_network_velocities,
		const std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks,
		const double &reconstruction_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		const double &velocity_delta_time)
{
	// Get the next global reconstruct handle - it'll be stored in each velocity field.
	const ReconstructHandle::type reconstruct_handle = ReconstructHandle::get_next_reconstruct_handle();

	// Iterate over the resolved topological networks.
	BOOST_FOREACH(
			const ResolvedTopologicalNetwork::non_null_ptr_type &resolved_topological_network,
			resolved_topological_networks)
	{
		// Boundary sub-segment velocities.
		create_resolved_topological_boundary_sub_segment_velocities(
				resolved_topological_network_velocities,
				resolved_topological_network->get_boundary_sub_segment_sequence(),
				reconstruction_time,
				velocity_delta_time_type,
				velocity_delta_time,
				reconstruct_handle,
				ResolvedTopologicalNetwork::INCLUDE_SUB_SEGMENT_RUBBER_BAND_POINTS_IN_RESOLVED_NETWORK_BOUNDARY);

		// Interior hole (rigid block polygon) velocities.
		//
		// We want to calculate velocities on the *boundary* or the network which includes its
		// exterior boundary (ie, boundary sub-segments) and an interior rigid blocks.
		create_resolved_topological_interior_hole_velocities(
				resolved_topological_network_velocities,
				resolved_topological_network->get_triangulation_network().get_rigid_blocks(),
				reconstruction_time,
				velocity_delta_time_type,
				velocity_delta_time,
				reconstruct_handle);
	}

	return reconstruct_handle;
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::create_resolved_topological_boundary_sub_segment_velocities(
		std::vector<MultiPointVectorField::non_null_ptr_type> &resolved_topological_boundary_sub_segment_velocities,
		const sub_segment_seq_type &sub_segments,
		const double &reconstruction_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		const double &velocity_delta_time,
		const ReconstructHandle::type &reconstruct_handle,
		bool include_sub_segment_rubber_band_points)
{
	// Iterate over the sub-segments.
	sub_segment_seq_type::const_iterator sub_segments_iter = sub_segments.begin();
	sub_segment_seq_type::const_iterator sub_segments_end = sub_segments.end();
	for ( ; sub_segments_iter != sub_segments_end; ++sub_segments_iter)
	{
		const ResolvedTopologicalGeometrySubSegment::non_null_ptr_type &sub_segment = *sub_segments_iter;

		// If the sub-segment has any of its own sub-segments in turn, then process those instead of the parent sub-segment.
		// This essentially is the same as simply using the parent sub-segment except that the plate ID and
		// reconstruction geometry (used for velocity colouring) will match the actual underlying reconstructed
		// feature geometries (when the parent sub-segment belongs to a resolved topological *line* which in
		// turn is a section of the boundary of the resolved topological *network*).
		const boost::optional<sub_segment_seq_type> &sub_sub_segments = sub_segment->get_sub_sub_segments();
		if (sub_sub_segments)
		{
			// Iterate over the sub-sub-segments and create velocities from them.
			create_resolved_topological_boundary_sub_segment_velocities(
					resolved_topological_boundary_sub_segment_velocities,
					sub_sub_segments.get(),
					reconstruction_time,
					velocity_delta_time_type,
					velocity_delta_time,
					reconstruct_handle,
					// Note the sub-sub-segments must belong to a resolved topological *line* since
					// a topological *network* can be used as a topological section...
					ResolvedTopologicalLine::INCLUDE_SUB_SEGMENT_RUBBER_BAND_POINTS_IN_RESOLVED_LINE);

			// Continue onto the next sub-segment.
			continue;
		}

		boost::optional<GPlatesModel::FeatureHandle::iterator> sub_segment_geometry_property =
					ReconstructionGeometryUtils::get_geometry_property_iterator(
							sub_segment->get_reconstruction_geometry());
		if (!sub_segment_geometry_property)
		{
			// This shouldn't happen.
			continue;
		}

		const ReconstructionGeometry::maybe_null_ptr_to_const_type sub_segment_plate_id_reconstruction_geometry(
				sub_segment->get_reconstruction_geometry().get());
		boost::optional<GPlatesModel::integer_plate_id_type> sub_segment_plate_id =
				ReconstructionGeometryUtils::get_plate_id(sub_segment->get_reconstruction_geometry());

		// Note that we're not interested in the reversal flag of sub-segment (ie, how it contributed
		// to this resolved topological network, or to a resolved topological line that in turn
		// contributed to this resolved topological network if sub-segment is a sub-sub-segment).
		// This is because we're just putting velocities on points (so their order doesn't matter).
		std::vector<GPlatesMaths::PointOnSphere> sub_segment_geometry_points;
		sub_segment->get_sub_segment_points(
				sub_segment_geometry_points,
				// We only need points that match the resolved topological network boundary...
				include_sub_segment_rubber_band_points/*include_rubber_band_points*/);
		resolved_vertex_source_info_seq_type sub_segment_point_source_infos;
		sub_segment->get_sub_segment_point_source_infos(
				sub_segment_point_source_infos,
				// We only need points that match the resolved topological network boundary...
				include_sub_segment_rubber_band_points/*include_rubber_band_points*/);

		// We should have the same number of points as velocities.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				sub_segment_geometry_points.size() == sub_segment_point_source_infos.size(),
				GPLATES_ASSERTION_SOURCE);

		// It's possible to have no sub-segment points if rubber band points were excluded.
		// This can happen when a sub-sub-segment of a resolved line sub-segment is entirely
		// within the start or end rubber band region of the sub-sub-segment (and hence the
		// sub-sub-segment geometry is only made up of two rubber band points, which then get excluded).
		if (sub_segment_geometry_points.empty())
		{
			continue;
		}

		// NOTE: This is slightly dodgy because we will end up creating a MultiPointVectorField
		// that stores a multi-point domain and a corresponding velocity field but the
		// geometry property iterator (referenced by the MultiPointVectorField) could be a
		// non-multi-point geometry.
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type sub_segment_velocity_domain =
				GPlatesMaths::MultiPointOnSphere::create(
						sub_segment_geometry_points.begin(),
						sub_segment_geometry_points.end());

		GPlatesMaths::MultiPointOnSphere::const_iterator domain_iter = sub_segment_velocity_domain->begin();
		GPlatesMaths::MultiPointOnSphere::const_iterator domain_end = sub_segment_velocity_domain->end();

		MultiPointVectorField::non_null_ptr_type vector_field =
				MultiPointVectorField::create_empty(
						reconstruction_time,
						sub_segment_velocity_domain,
						*sub_segment->get_feature_ref(),
						sub_segment_geometry_property.get(),
						reconstruct_handle);
		MultiPointVectorField::codomain_type::iterator field_iter = vector_field->begin();

		// Iterate over the domain points and calculate their velocities.
		unsigned int velocity_index = 0;
		for ( ; domain_iter != domain_end; ++domain_iter, ++field_iter, ++velocity_index)
		{
			// Calculate the velocity.
			const GPlatesMaths::Vector3D vector_xyz =
					sub_segment_point_source_infos[velocity_index]->get_velocity_vector(
							*domain_iter,
							reconstruction_time,
							velocity_delta_time,
							velocity_delta_time_type);

			*field_iter = MultiPointVectorField::CodomainElement(
					vector_xyz,
					// Even though it's a resolved topological geometry it's still essentially a reconstructed geometry
					// in that the velocities come from the reconstructed sections that make up the topology...
					MultiPointVectorField::CodomainElement::ReconstructedDomainPoint,
					sub_segment_plate_id,
					sub_segment_plate_id_reconstruction_geometry);
		}

		resolved_topological_boundary_sub_segment_velocities.push_back(vector_field);
	}
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::create_resolved_topological_interior_hole_velocities(
		std::vector<MultiPointVectorField::non_null_ptr_type> &resolved_topological_interior_hole_velocities,
		const ResolvedTriangulation::Network::rigid_block_seq_type &interior_holes,
		const double &reconstruction_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		const double &velocity_delta_time,
		const ReconstructHandle::type &reconstruct_handle)
{
	// Iterate over the interior holes.
	ResolvedTriangulation::Network::rigid_block_seq_type::const_iterator interior_holes_iter = interior_holes.begin();
	ResolvedTriangulation::Network::rigid_block_seq_type::const_iterator interior_holes_end = interior_holes.end();
	for ( ; interior_holes_iter != interior_holes_end; ++interior_holes_iter)
	{
		const ResolvedTriangulation::Network::RigidBlock &interior_hole = *interior_holes_iter;

		const ReconstructedFeatureGeometry::non_null_ptr_type interior_hole_rfg =
				interior_hole.get_reconstructed_feature_geometry();

		// NOTE: This is slightly dodgy because we will end up creating a MultiPointVectorField
		// that stores a multi-point domain and a corresponding velocity field but the
		// geometry property iterator (referenced by the MultiPointVectorField) could be a
		// non-multi-point geometry.
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type interior_hole_velocity_domain =
				GeometryUtils::convert_geometry_to_multi_point(
						*interior_hole_rfg->reconstructed_geometry(),
						false/*include_polygon_interior_ring_points*/);

		GPlatesMaths::MultiPointOnSphere::const_iterator domain_iter = interior_hole_velocity_domain->begin();
		GPlatesMaths::MultiPointOnSphere::const_iterator domain_end = interior_hole_velocity_domain->end();

		MultiPointVectorField::non_null_ptr_type vector_field =
				MultiPointVectorField::create_empty(
						reconstruction_time,
						interior_hole_velocity_domain,
						*interior_hole_rfg->get_feature_ref(),
						interior_hole_rfg->property(),
						reconstruct_handle);
		MultiPointVectorField::codomain_type::iterator field_iter = vector_field->begin();

		const ReconstructionGeometry::maybe_null_ptr_to_const_type interior_plate_id_reconstruction_geometry(interior_hole_rfg.get());
		boost::optional<GPlatesModel::integer_plate_id_type> interior_hole_plate_id = interior_hole_rfg->reconstruction_plate_id();

		// Reconstruct information shared by all the points in the interior hole (since it's a
		// ReconstructedFeatureGeometry and hence all its points come from the same reconstructed feature geometry).
		const ResolvedVertexSourceInfo::non_null_ptr_to_const_type interior_hole_shared_source_info =
				ResolvedVertexSourceInfo::create(interior_hole_rfg.get());

		// Iterate over the domain points and calculate their velocities.
		for ( ; domain_iter != domain_end; ++domain_iter, ++field_iter)
		{
			// Calculate the velocity.
			const GPlatesMaths::Vector3D vector_xyz =
					interior_hole_shared_source_info->get_velocity_vector(
							*domain_iter,
							reconstruction_time,
							velocity_delta_time,
							velocity_delta_time_type);

			*field_iter = MultiPointVectorField::CodomainElement(
					vector_xyz,
					// Even though it's a resolved topological network it's still essentially a reconstructed geometry
					// in that the velocities come from the reconstructed sections that make up the topology...
					MultiPointVectorField::CodomainElement::ReconstructedDomainPoint,
					interior_hole_plate_id,
					interior_plate_id_reconstruction_geometry);
		}

		resolved_topological_interior_hole_velocities.push_back(vector_field);
	}
}
