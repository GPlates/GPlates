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

#include "ResolvedTopologicalGeometry.h"
#include "ResolvedTopologicalNetwork.h"
#include "TopologyUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/MathsUtils.h"


GPlatesAppLogic::TopologyNetworkResolverLayerProxy::TopologyNetworkResolverLayerProxy() :
	d_current_reconstruction_time(0)
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
		std::vector<resolved_topological_network_non_null_ptr_type> &resolved_topological_networks,
		const double &reconstruction_time)
{
	// See if the reconstruction time has changed.
	if (d_cached_resolved_networks.cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time))
	{
		// The resolved networks are now invalid.
		d_cached_resolved_networks.invalidate();

		// Note that observers don't need to be updated when the time changes - if they
		// have resolved networks for a different time they don't need
		// to be updated just because some other client requested a different time.
		d_cached_resolved_networks.cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
	}

	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	if (!d_cached_resolved_networks.cached_resolved_topological_networks)
	{
		cache_resolved_topological_networks(reconstruction_time);
	}

	// Append our cached resolved topological networks to the caller's sequence.
	resolved_topological_networks.insert(
			resolved_topological_networks.end(),
			d_cached_resolved_networks.cached_resolved_topological_networks->begin(),
			d_cached_resolved_networks.cached_resolved_topological_networks->end());

	return d_cached_resolved_networks.cached_reconstruct_handle.get();
}


GPlatesAppLogic::GeometryDeformation::resolved_network_time_span_type::non_null_ptr_to_const_type
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::get_resolved_network_time_span(
		const TimeSpanUtils::TimeRange &time_range)
{
	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	// If the resolved network time span did not get invalidated (due to updated inputs)
	// then see if the time range has changed.
	if (d_cached_resolved_network_time_span)
	{
		const TimeSpanUtils::TimeRange &cached_time_range =
				d_cached_resolved_network_time_span.get()->get_time_range();

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
			cache_resolved_network_time_span(time_range);

			return d_cached_resolved_network_time_span.get();
		}
	}

	if (!d_cached_resolved_network_time_span)
	{
		cache_resolved_network_time_span(time_range);
	}

	return d_cached_resolved_network_time_span.get();
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
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::set_current_reconstructed_geometry_topological_sections_layer_proxies(
		const std::vector<ReconstructLayerProxy::non_null_ptr_type> &reconstructed_geometry_topological_sections_layer_proxies)
{
	// If the topological sections layer proxies are the same ones as last time then no invalidation necessary.
	if (!d_current_reconstructed_geometry_topological_sections_layer_proxies.set_input_layer_proxies(
		reconstructed_geometry_topological_sections_layer_proxies))
	{
		return;
	}

	// All resolved topological networks are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::set_current_resolved_line_topological_sections_layer_proxies(
		const std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> &resolved_line_topological_sections_layer_proxies)
{
	// If the topological sections layer proxies are the same ones as last time then no invalidation necessary.
	if (!d_current_resolved_line_topological_sections_layer_proxies.set_input_layer_proxies(
		resolved_line_topological_sections_layer_proxies))
	{
		return;
	}

	// All resolved topological networks are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::add_topological_network_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	d_current_topological_network_feature_collections.push_back(feature_collection);

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
	d_current_topological_network_feature_collections.erase(
			std::find(
					d_current_topological_network_feature_collections.begin(),
					d_current_topological_network_feature_collections.end(),
					feature_collection));

	// The resolved topological networks are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::modified_topological_network_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
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
	d_cached_resolved_network_time_span = boost::none;
}


template <class InputLayerProxyWrapperType>
void
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::check_input_layer_proxy(
		InputLayerProxyWrapperType &input_layer_proxy_wrapper)
{
	// See if the input layer proxy has changed.
	if (!input_layer_proxy_wrapper.is_up_to_date())
	{
		// The velocities are now invalid.
		reset_cache();

		// We're now up-to-date with respect to the input layer proxy.
		input_layer_proxy_wrapper.set_up_to_date();

		// Polling observers need to update themselves with respect to us.
		d_subject_token.invalidate();
	}
}


void
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::check_input_layer_proxies()
{
	// See if any reconstructed geometry topological section layer proxies have changed.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &rfg_topological_sections_layer_proxy,
			d_current_reconstructed_geometry_topological_sections_layer_proxies.get_input_layer_proxies())
	{
		check_input_layer_proxy(rfg_topological_sections_layer_proxy);
	}

	// See if any resolved geometry topological section layer proxies have changed.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<TopologyGeometryResolverLayerProxy> &rtl_topological_sections_layer_proxy,
			d_current_resolved_line_topological_sections_layer_proxies.get_input_layer_proxies())
	{
		check_input_layer_proxy(rtl_topological_sections_layer_proxy);
	}
}


std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type> &
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::cache_resolved_topological_networks(
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

	// First see if we've already cached the current reconstruction time in the resolved network time span.
	if (d_cached_resolved_network_time_span)
	{
		// If there's a time slot in the time span that matches the reconstruction time
		// then we can re-use the resolved networks in that time slot.
		const boost::optional<unsigned int> time_slot =
				d_cached_resolved_network_time_span.get()->get_time_range().get_time_slot(reconstruction_time);
		if (time_slot)
		{
			// Extract the resolved topological networks for the reconstruction time.
			boost::optional<GeometryDeformation::rtn_seq_type &> resolved_topological_networks =
					d_cached_resolved_network_time_span.get()->get_sample_in_time_slot(time_slot.get());
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
					reconstruction_time);

	return d_cached_resolved_networks.cached_resolved_topological_networks.get();
}


GPlatesAppLogic::GeometryDeformation::resolved_network_time_span_type::non_null_ptr_to_const_type
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::cache_resolved_network_time_span(
		const TimeSpanUtils::TimeRange &time_range)
{
	//PROFILE_FUNC();

	// If one is already cached then attempt to re-use any time slots in common with the
	// new time range. If one is already cached then it contains valid resolved networks
	// - it's just that the time range has changed.
	boost::optional<GeometryDeformation::resolved_network_time_span_type::non_null_ptr_type>
			prev_resolved_network_time_span = d_cached_resolved_network_time_span;

	// Create an empty resolved network time span.
	d_cached_resolved_network_time_span =
			GeometryDeformation::resolved_network_time_span_type::create(time_range);

	const unsigned int num_time_slots = time_range.get_num_time_slots();

	// As a performance optimisation, for all our topological sections input layers we request a
	// reconstruction tree creator with a cache size the same as the resolved network time span
	// (plus one for possible extra time step).
	// This ensures we don't get a noticeable slowdown when the time span range exceeds the
	// size of the cache in the reconstruction layer proxy.
	// We don't actually use the returned ReconstructionTreeCreator here but by specifying a
	// cache size hint we set the size of its internal reconstruction tree cache.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &reconstructed_geometry_topological_sections_layer_proxy,
			d_current_reconstructed_geometry_topological_sections_layer_proxies.get_input_layer_proxies())
	{
		reconstructed_geometry_topological_sections_layer_proxy.get_input_layer_proxy()
				->get_current_reconstruction_layer_proxy()->get_reconstruction_tree_creator(num_time_slots + 1);
	}
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<TopologyGeometryResolverLayerProxy> &resolved_line_topological_sections_layer_proxy,
			d_current_resolved_line_topological_sections_layer_proxies.get_input_layer_proxies())
	{
		resolved_line_topological_sections_layer_proxy.get_input_layer_proxy()
				->get_reconstruction_layer_proxy()->get_reconstruction_tree_creator(num_time_slots + 1);
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
				boost::optional<GeometryDeformation::rtn_seq_type &> prev_resolved_topological_networks =
						prev_resolved_network_time_span.get()->get_sample_in_time_slot(prev_time_slot.get());
				if (prev_resolved_topological_networks)
				{
					d_cached_resolved_network_time_span.get()->set_sample_in_time_slot(
							prev_resolved_topological_networks.get(),
							time_slot);

					// Continue to the next time slot.
					continue;
				}
			}
		}

		// Create the resolved topological networks for the current time slot.
		std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> resolved_topological_networks;
		create_resolved_topological_networks(resolved_topological_networks, time);

		d_cached_resolved_network_time_span.get()->set_sample_in_time_slot(
				resolved_topological_networks,
				time_slot);
	}

	return d_cached_resolved_network_time_span.get();
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyNetworkResolverLayerProxy::create_resolved_topological_networks(
		std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks,
		const double &reconstruction_time)
{
	// If we have no topological features or there are no topological section layers then we
	// can't get any topological sections and we can't resolve any topological networks.
	if (d_current_topological_network_feature_collections.empty() ||
		(d_current_reconstructed_geometry_topological_sections_layer_proxies.get_input_layer_proxies().empty() &&
			d_current_resolved_line_topological_sections_layer_proxies.get_input_layer_proxies().empty()))
	{
		// There will be no reconstructed/resolved networks for this handle.
		return ReconstructHandle::get_next_reconstruct_handle();
	}

	//
	// Generate the resolved topological networks for the reconstruction time.
	//

	std::vector<ReconstructHandle::type> topological_geometry_reconstruct_handles;

	// Topological boundary sections and/or interior geometries that are reconstructed static features...
	// We're ensuring that all potential (reconstructed geometry) topological-referenced geometries are
	// reconstructed before we resolve topological networks (which reference them indirectly via feature-id).
	std::vector<reconstructed_feature_geometry_non_null_ptr_type> topologically_referenced_reconstructed_geometries;
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &reconstructed_geometry_topological_sections_layer_proxy,
			d_current_reconstructed_geometry_topological_sections_layer_proxies.get_input_layer_proxies())
	{
		// Get the potential topological section RFGs.
		const ReconstructHandle::type reconstruct_handle =
				reconstructed_geometry_topological_sections_layer_proxy.get_input_layer_proxy()
						->get_reconstructed_feature_geometries(
								topologically_referenced_reconstructed_geometries,
								reconstruction_time);

		// Add the reconstruct handle to our list.
		topological_geometry_reconstruct_handles.push_back(reconstruct_handle);
	}

	// Topological boundary sections and/or interior geometries that are resolved topological lines...
	// We're ensuring that all potential (resolved line) topologically-referenced geometries are
	// resolved before we resolve topological networks (which reference them indirectly via feature-id).
	std::vector<resolved_topological_geometry_non_null_ptr_type> topologically_referenced_resolved_lines;
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<TopologyGeometryResolverLayerProxy> &resolved_line_topological_sections_layer_proxy,
			d_current_resolved_line_topological_sections_layer_proxies.get_input_layer_proxies())
	{
		// Get the potential topological section RTGs.
		const ReconstructHandle::type reconstruct_handle =
				resolved_line_topological_sections_layer_proxy.get_input_layer_proxy()
						->get_resolved_topological_lines(
								topologically_referenced_resolved_lines,
								reconstruction_time);

		// Add the reconstruct handle to our list.
		topological_geometry_reconstruct_handles.push_back(reconstruct_handle);
	}

	// Resolve our network features into our sequence of resolved topological networks.
	return TopologyUtils::resolve_topological_networks(
			resolved_topological_networks,
			reconstruction_time,
			d_current_topological_network_feature_collections,
			topological_geometry_reconstruct_handles);
}
