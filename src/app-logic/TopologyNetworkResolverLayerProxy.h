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

#ifndef GPLATES_APP_LOGIC_TOPOLOGYNETWORKRESOLVERLAYERPROXY_H
#define GPLATES_APP_LOGIC_TOPOLOGYNETWORKRESOLVERLAYERPROXY_H

#include <vector>
#include <boost/optional.hpp>

#include "DependentTopologicalSectionLayers.h"
#include "GeometryDeformation.h"
#include "LayerProxy.h"
#include "LayerProxyUtils.h"
#include "ReconstructHandle.h"
#include "ReconstructionLayerProxy.h"
#include "ReconstructLayerProxy.h"
#include "ResolvedTopologicalNetwork.h"
#include "TopologyGeometryResolverLayerProxy.h"
#include "TopologyNetworkParams.h"

#include "model/FeatureHandle.h"

#include "utils/SubjectObserverToken.h"


namespace GPlatesAppLogic
{
	/**
	 * A layer proxy that resolves topological networks from feature collection(s)
	 * containing topological network features.
	 */
	class TopologyNetworkResolverLayerProxy :
			public LayerProxy
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a TopologyNetworkResolverLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<TopologyNetworkResolverLayerProxy> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a TopologyNetworkResolverLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<const TopologyNetworkResolverLayerProxy> non_null_ptr_to_const_type;


		/**
		 * Creates a @a TopologyNetworkResolverLayerProxy object.
		 */
		static
		non_null_ptr_type
		create(
				const TopologyNetworkParams &topology_network_params = TopologyNetworkParams())
		{
			return non_null_ptr_type(new TopologyNetworkResolverLayerProxy(topology_network_params));
		}


		~TopologyNetworkResolverLayerProxy();


		/**
		 * Returns the resolved topological networks, for the current reconstruction time and
		 * current topology network params, by appending them to them to @a resolved_topological_networks.
		 *
		 * NOTE: The resolved topological networks can in turn reference resolved topological polylines.
		 * Currently there is a topological referencing restriction where resolved topological networks can reference
		 * resolved topological polylines (and regular static geometries) but resolved topological polylines
		 * can only reference regular static geometries (and not resolved topological geometries of any type).
		 * This ordering enables the resolving process to ensure all geometries referenced by a topology
		 * are fully reconstructed (and resolved) before that topology itself is resolved. This is needed
		 * because the topological referencing is done using feature-id lookup instead of querying input layers
		 * and only the latter can truly guarantee a correct dependency ordering without intervention
		 * required - in the former case that intervention is the imposed topological referencing restriction.
		 */
		ReconstructHandle::type
		get_resolved_topological_networks(
				std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks)
		{
			return get_resolved_topological_networks(
					resolved_topological_networks,
					d_current_topology_network_params,
					d_current_reconstruction_time);
		}

		/**
		 * Returns the resolved topological networks, for the current reconstruction time and
		 * specified topology network params, by appending them to them to @a resolved_topological_networks.
		 */
		ReconstructHandle::type
		get_resolved_topological_networks(
				std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks,
				const TopologyNetworkParams &topology_network_params)
		{
			return get_resolved_topological_networks(
					resolved_topological_networks,
					topology_network_params,
					d_current_reconstruction_time);
		}

		/**
		 * Returns the resolved topological networks, for the specified reconstruction time and
		 * current topology network params, by appending them to them to @a resolved_topological_networks.
		 */
		ReconstructHandle::type
		get_resolved_topological_networks(
				std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks,
				const double &reconstruction_time)
		{
			return get_resolved_topological_networks(
					resolved_topological_networks,
					d_current_topology_network_params,
					reconstruction_time);
		}

		/**
		 * Returns the resolved topological networks, for the specified reconstruction time and
		 * specified topology network params, by appending them to them to @a resolved_topological_networks.
		 */
		ReconstructHandle::type
		get_resolved_topological_networks(
				std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks,
				const TopologyNetworkParams &topology_network_params,
				const double &reconstruction_time);


		/**
		 * Returns a time span of resolved topological networks, for the current topology network params.
		 *
		 * The main purpose of this method, over calling @a get_resolved_topological_networks for
		 * a sequence of times (which essentially does the same thing), is to cache a time range
		 * of resolved topological networks rather than just caching for a single reconstruction time.
		 * This helps to avoid the expensive process of repeating the generation of a time span of
		 * resolved networks which would happen if multiple clients each called
		 * @a get_resolved_topological_networks over a sequence of reconstruction times because
		 * a separate resolved network would unnecessarily be created for each client - whereas,
		 * with this method, a single time range of resolved networks would be shared by all clients.
		 */
		GeometryDeformation::resolved_network_time_span_type::non_null_ptr_to_const_type
		get_resolved_network_time_span(
				const TimeSpanUtils::TimeRange &time_range)
		{
			return get_resolved_network_time_span(time_range, d_current_topology_network_params);
		}

		/**
		 * Returns a time span of resolved topological networks, for the specified topology network params.
		 */
		GeometryDeformation::resolved_network_time_span_type::non_null_ptr_to_const_type
		get_resolved_network_time_span(
				const TimeSpanUtils::TimeRange &time_range,
				const TopologyNetworkParams &topology_network_params);

		//
		// Getting current topology network params and reconstruction time as set by the layer system.
		//

		/**
		 * Gets the current reconstruction time as set by the layer system.
		 */
		const double &
		get_current_reconstruction_time() const
		{
			return d_current_reconstruction_time;
		}

		/**
		 * Gets the parameters used for resolving topological networks and their associated attributes.
		 */
		const TopologyNetworkParams &
		get_current_topology_network_params() const
		{
			return d_current_topology_network_params;
		}


		/**
		 * Returns the subject token that clients can use to determine if the resolved
		 * topological networks have changed since they were last retrieved.
		 *
		 * This is mainly useful for other layers that have this layer connected as their input.
		 */
		const GPlatesUtils::SubjectToken &
		get_subject_token();


		/**
		 * Accept a ConstLayerProxyVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstLayerProxyVisitor &visitor) const
		{
			visitor.visit(GPlatesUtils::get_non_null_pointer(this));
		}

		/**
		 * Accept a LayerProxyVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				LayerProxyVisitor &visitor)
		{
			visitor.visit(GPlatesUtils::get_non_null_pointer(this));
		}


		//
		// Used by LayerTask...
		//

		/**
		 * Sets the current reconstruction time as set by the layer system.
		 */
		void
		set_current_reconstruction_time(
				const double &reconstruction_time);

		/**
		 * Sets the parameters used for resolving topological networks and their associated attributes.
		 */
		void
		set_current_topology_network_params(
				const TopologyNetworkParams &topology_network_params);

		/**
		 * Sets the current layer proxies used to reconstruct/resolve the topological network sections.
		 */
		void
		set_current_topological_sections_layer_proxies(
				const std::vector<ReconstructLayerProxy::non_null_ptr_type> &
						reconstructed_geometry_topological_sections_layer_proxies,
				const std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> &
						resolved_line_topological_sections_layer_proxies);

		/**
		 * Add to the list of feature collections containing topological network features.
		 */
		void
		add_topological_network_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		/**
		 * Remove from the list of feature collections containing topological network features.
		 */
		void
		remove_topological_network_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		/**
		 * A topological network feature collection was modified.
		 */
		void
		modified_topological_network_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

	private:

		/**
		 * Contains resolved topological networks.
		 */
		struct ResolvedNetworks
		{
			void
			invalidate()
			{
				cached_reconstruct_handle = boost::none;
				cached_resolved_topological_networks = boost::none;
				cached_reconstruction_time = boost::none;
				cached_topology_network_params = boost::none;
			}

			/**
			 * The reconstruct handle that identifies all cached resolved topological networks
			 * in this structure.
			 */
			boost::optional<ReconstructHandle::type> cached_reconstruct_handle;

			/**
			 * The cached resolved topological networks.
			 */
			boost::optional< std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> >
					cached_resolved_topological_networks;

			/**
			 * Cached reconstruction time associated with the cache resolved topological networks.
			 */
			boost::optional<GPlatesMaths::real_t> cached_reconstruction_time;

			/**
			 * The cached topology network parameters associated with the cache resolved topological networks.
			 */
			boost::optional<TopologyNetworkParams> cached_topology_network_params;
		};

		/**
		 * Contains resolved topological network time span.
		 */
		struct ResolvedNetworkTimeSpan
		{
			void
			invalidate()
			{
				cached_resolved_network_time_span = boost::none;
				cached_topology_network_params = boost::none;
			}

			/**
			 * The cached resolved topologies over a range of reconstruction times.
			 */
			boost::optional<GeometryDeformation::resolved_network_time_span_type::non_null_ptr_type>
					cached_resolved_network_time_span;

			/**
			 * The cached topology network parameters associated with the cache resolved topological network time span.
			 */
			boost::optional<TopologyNetworkParams> cached_topology_network_params;
		};


		/**
		 * The input feature collections to reconstruct.
		 */
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> d_current_topological_network_feature_collections;

		/**
		 * Used to get reconstructed static features that form the topological sections for our topological geometries.
		 */
		LayerProxyUtils::InputLayerProxySequence<ReconstructLayerProxy>
				d_current_reconstructed_geometry_topological_sections_layer_proxies;

		/**
		 * Used to get resolved topological lines that form the topological sections for our topological geometries.
		 */
		LayerProxyUtils::InputLayerProxySequence<TopologyGeometryResolverLayerProxy>
				d_current_resolved_line_topological_sections_layer_proxies;

		/**
		 * The current reconstruction time as set by the layer system.
		 */
		double d_current_reconstruction_time;

		/**
		 * The current topology network parameters as set by the layer system.
		 */
		TopologyNetworkParams d_current_topology_network_params;

		/**
		 * The cached resolved topologies for a single reconstruction time.
		 */
		ResolvedNetworks d_cached_resolved_networks;

		/**
		 * The cached resolved topologies over a range of reconstruction times.
		 *
		 * This is cached as a performance optimisation for clients that deform geometries.
		 */
		ResolvedNetworkTimeSpan d_cached_time_span;

		/**
		 * The cached resolved networks (including time spans) depend on these topological sections.
		 */
		DependentTopologicalSectionLayers d_dependent_topological_sections;

		/**
		 * Used to notify polling observers that we've been updated.
		 */
		mutable GPlatesUtils::SubjectToken d_subject_token;


		explicit
		TopologyNetworkResolverLayerProxy(
				const TopologyNetworkParams &topology_network_params);


		/**
		 * Resets any cached variables forcing them to be recalculated next time they're accessed.
		 */
		void
		reset_cache();


		/**
		 * Checks if any input layer proxies have changed.
		 *
		 * If so then reset caches and invalidates subject token.
		 */
		void
		check_input_layer_proxies();


		/**
		 * Generates resolved topological networks for the specified reconstruction time
		 * if they're not already cached.
		 */
		std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &
		cache_resolved_topological_networks(
				const TopologyNetworkParams &topology_network_params,
				const double &reconstruction_time);


		/**
		 * Generates a resolved network time span for the specified time range if one is not already cached.
		 */
		GeometryDeformation::resolved_network_time_span_type::non_null_ptr_to_const_type
		cache_resolved_network_time_span(
				const TimeSpanUtils::TimeRange &time_range,
				const TopologyNetworkParams &topology_network_params);


		/**
		 * Creates resolved topological networks for the specified reconstruction time.
		 */
		ReconstructHandle::type
		create_resolved_topological_networks(
				std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks,
				const TopologyNetworkParams &topology_network_params,
				const double &reconstruction_time);
	};
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYNETWORKRESOLVERLAYERPROXY_H
