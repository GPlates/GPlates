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

#include "TopologyBoundaryResolverLayerProxy.h"

#include "ResolvedTopologicalBoundary.h"
#include "TopologyUtils.h"


GPlatesAppLogic::TopologyBoundaryResolverLayerProxy::TopologyBoundaryResolverLayerProxy() :
	// Start off with a reconstruction layer proxy that does identity rotations.
	d_current_reconstruction_layer_proxy(ReconstructionLayerProxy::create()),
	d_current_reconstruction_time(0)
{
	// Defined in ".cc" file because...
	// non_null_ptr destructors require complete type of class they're referring to.
	// Compiler will call destructors of already-constructed members if constructor throws exception.
}


GPlatesAppLogic::TopologyBoundaryResolverLayerProxy::~TopologyBoundaryResolverLayerProxy()
{
	// Defined in ".cc" file because...
	// non_null_ptr destructors require complete type of class they're referring to.
}


void
GPlatesAppLogic::TopologyBoundaryResolverLayerProxy::get_resolved_topological_boundaries(
		std::vector<resolved_topological_boundary_non_null_ptr_type> &resolved_topological_boundaries,
		const double &reconstruction_time)
{
	// If we have no topological features or we are not attached to a reconstruct layer then we
	// can't get any reconstructed topological boundary sections and we can't resolve any
	// topological close plate polygons.
	if (d_current_topological_closed_plate_polygon_feature_collections.empty() ||
		d_current_topological_sections_layer_proxies.get_input_layer_proxies().empty())
	{
		return;
	}

	// See if the reconstruction time has changed.
	if (d_cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time))
	{
		// The resolved boundaries are now invalid.
		reset_cache();

		// Note that observers don't need to be updated when the time changes - if they
		// have resolved boundaries for a different time they don't need
		// to be updated just because some other client requested a different time.
		d_cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
	}

	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	if (!d_cached_resolved_topological_boundaries)
	{
		// Create empty vector of resolved topological boundaries.
		d_cached_resolved_topological_boundaries =
				std::vector<resolved_topological_boundary_non_null_ptr_type>();

		// Topological sections...
		std::vector<reconstructed_feature_geometry_non_null_ptr_type> reconstructed_topological_boundary_sections;
		std::vector<ReconstructHandle::type> topological_sections_reconstruct_handles;
		BOOST_FOREACH(
				LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &topological_sections_layer_proxy,
				d_current_topological_sections_layer_proxies.get_input_layer_proxies())
		{
			// Get the potential topological section RFGs.
			const ReconstructHandle::type reconstruct_handle =
					topological_sections_layer_proxy.get_input_layer_proxy()->get_reconstructed_feature_geometries(
							reconstructed_topological_boundary_sections,
							reconstruction_time);

			// Add the reconstruct handle to our list.
			topological_sections_reconstruct_handles.push_back(reconstruct_handle);
		}

		// We only resolve boundaries if we have topological sections.
		if (!reconstructed_topological_boundary_sections.empty())
		{
			// Resolve our closed plate polygon features into our sequence of resolved topological boundaries.
			TopologyUtils::resolve_topological_boundaries(
					d_cached_resolved_topological_boundaries.get(),
					d_current_topological_closed_plate_polygon_feature_collections,
					d_current_reconstruction_layer_proxy.get_input_layer_proxy()->get_reconstruction_tree(reconstruction_time),
					topological_sections_reconstruct_handles);
		}
	}

	// Append our cached resolved topological boundaries to the caller's sequence.
	resolved_topological_boundaries.insert(
			resolved_topological_boundaries.end(),
			d_cached_resolved_topological_boundaries->begin(),
			d_cached_resolved_topological_boundaries->end());
}


GPlatesAppLogic::ReconstructionLayerProxy::non_null_ptr_type
GPlatesAppLogic::TopologyBoundaryResolverLayerProxy::get_reconstruction_layer_proxy()
{
	return d_current_reconstruction_layer_proxy.get_input_layer_proxy();
}


const GPlatesUtils::SubjectToken &
GPlatesAppLogic::TopologyBoundaryResolverLayerProxy::get_subject_token()
{
	// We've checked to see if any inputs have changed except the reconstruction and
	// reconstruct layer proxy inputs.
	// This is because we get notified of all changes to input except input layer proxies which
	// we have to poll to see if they changed since we last accessed them - so we do that now.
	check_input_layer_proxies();

	return d_subject_token;
}


void
GPlatesAppLogic::TopologyBoundaryResolverLayerProxy::set_current_reconstruction_time(
		const double &reconstruction_time)
{
	d_current_reconstruction_time = reconstruction_time;

	// Note that we don't reset our caches because we only do that when the client
	// requests a reconstruction time that differs from the cached reconstruction time.
}


void
GPlatesAppLogic::TopologyBoundaryResolverLayerProxy::set_current_reconstruction_layer_proxy(
		const ReconstructionLayerProxy::non_null_ptr_type &reconstruction_layer_proxy)
{
	d_current_reconstruction_layer_proxy.set_input_layer_proxy(reconstruction_layer_proxy);

	// The resolved topological boundaries are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::TopologyBoundaryResolverLayerProxy::set_current_topological_sections_layer_proxies(
		const std::vector<ReconstructLayerProxy::non_null_ptr_type> &topological_sections_layer_proxies)
{
	// If the topological sections layer proxies are the same ones as last time then no invalidation necessary.
	if (!d_current_topological_sections_layer_proxies.set_input_layer_proxies(topological_sections_layer_proxies))
	{
		return;
	}

	// The resolved topological boundaries are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::TopologyBoundaryResolverLayerProxy::add_topological_closed_plate_polygon_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	d_current_topological_closed_plate_polygon_feature_collections.push_back(feature_collection);

	// The resolved topological boundaries are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::TopologyBoundaryResolverLayerProxy::remove_topological_closed_plate_polygon_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// Erase the feature collection from our list.
	d_current_topological_closed_plate_polygon_feature_collections.erase(
			std::find(
					d_current_topological_closed_plate_polygon_feature_collections.begin(),
					d_current_topological_closed_plate_polygon_feature_collections.end(),
					feature_collection));

	// The resolved topological boundaries are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::TopologyBoundaryResolverLayerProxy::modified_topological_closed_plate_polygon_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// The resolved topological boundaries are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::TopologyBoundaryResolverLayerProxy::reset_cache()
{
	// Clear any cached resolved topological boundaries.
	d_cached_resolved_topological_boundaries = boost::none;
	d_cached_reconstruction_time = boost::none;
}


template <class InputLayerProxyWrapperType>
void
GPlatesAppLogic::TopologyBoundaryResolverLayerProxy::check_input_layer_proxy(
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
GPlatesAppLogic::TopologyBoundaryResolverLayerProxy::check_input_layer_proxies()
{
	// See if the reconstruction layer proxy has changed.
	check_input_layer_proxy(d_current_reconstruction_layer_proxy);

	// See if any topological section layer proxies have changed.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &topological_sections_layer_proxy,
			d_current_topological_sections_layer_proxies.get_input_layer_proxies())
	{
		check_input_layer_proxy(topological_sections_layer_proxy);
	}
}
