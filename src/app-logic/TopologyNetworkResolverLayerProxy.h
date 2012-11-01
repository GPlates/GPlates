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

#ifndef GPLATES_APP_LOGIC_TOPOLOGYNETWORKRESOLVERLAYERPROXY_H
#define GPLATES_APP_LOGIC_TOPOLOGYNETWORKRESOLVERLAYERPROXY_H

#include <vector>
#include <boost/optional.hpp>

#include "AppLogicFwd.h"
#include "LayerProxy.h"
#include "LayerProxyUtils.h"
#include "ReconstructHandle.h"
#include "ReconstructionLayerProxy.h"
#include "ReconstructLayerProxy.h"
#include "ResolvedTopologicalNetwork.h"
#include "TopologyGeometryResolverLayerProxy.h"

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
		create()
		{
			return non_null_ptr_type(new TopologyNetworkResolverLayerProxy());
		}


		~TopologyNetworkResolverLayerProxy();


		/**
		 * Returns the resolved topological networks, for the current reconstruction time,
		 * by appending them to them to @a resolved_topological_networks.
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
				std::vector<resolved_topological_network_non_null_ptr_type> &resolved_topological_networks)
		{
			return get_resolved_topological_networks(resolved_topological_networks, d_current_reconstruction_time);
		}


		/**
		 * Returns the resolved topological networks, at the specified time, by appending
		 * them to them to @a resolved_topological_networks.
		 */
		ReconstructHandle::type
		get_resolved_topological_networks(
				std::vector<resolved_topological_network_non_null_ptr_type> &resolved_topological_networks,
				const double &reconstruction_time);


		/**
		 * Returns the current reconstruction layer proxy used for reconstructions.
		 */
		ReconstructionLayerProxy::non_null_ptr_type
		get_reconstruction_layer_proxy();


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
		 * Set the reconstruction layer proxy used to rotate feature geometries.
		 */
		void
		set_current_reconstruction_layer_proxy(
				const ReconstructionLayerProxy::non_null_ptr_type &reconstruction_layer_proxy);

		/**
		 * Sets the current reconstruct layer proxies used to reconstruct the topological network boundary sections.
		 */
		void
		set_current_reconstructed_geometry_topological_sections_layer_proxies(
				const std::vector<ReconstructLayerProxy::non_null_ptr_type> &
						reconstructed_geometry_topological_sections_layer_proxies);

		/**
		 * Sets the current resolved topological line layer proxies used to resolve the topological boundary line sections.
		 */
		void
		set_current_resolved_line_topological_sections_layer_proxies(
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
			}

			/**
			 * The reconstruct handle that identifies all cached resolved topological networks
			 * in this structure.
			 */
			boost::optional<ReconstructHandle::type> cached_reconstruct_handle;

			/**
			 * The cached resolved topological networks.
			 */
			boost::optional< std::vector<resolved_topological_network_non_null_ptr_type> >
					cached_resolved_topological_networks;

			/**
			 * Cached reconstruction time.
			 */
			boost::optional<GPlatesMaths::real_t> cached_reconstruction_time;
		};

		/**
		 * The input feature collections to reconstruct.
		 */
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> d_current_topological_network_feature_collections;

		/**
		 * Used to get reconstruction trees at desired reconstruction times.
		 */
		LayerProxyUtils::InputLayerProxy<ReconstructionLayerProxy> d_current_reconstruction_layer_proxy;

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
		 * The cached resolved topologies.
		 */
		ResolvedNetworks d_cached_resolved_networks;

		/**
		 * Used to notify polling observers that we've been updated.
		 */
		mutable GPlatesUtils::SubjectToken d_subject_token;


		//! Default constructor.
		TopologyNetworkResolverLayerProxy();


		/**
		 * Resets any cached variables forcing them to be recalculated next time they're accessed.
		 */
		void
		reset_cache();


		/**
		 * Checks if the specified input layer proxy has changed.
		 *
		 * If so then reset caches and invalidates subject token.
		 */
		template <class InputLayerProxyWrapperType>
		void
		check_input_layer_proxy(
				InputLayerProxyWrapperType &input_layer_proxy_wrapper);


		/**
		 * Checks if any input layer proxies have changed.
		 *
		 * If so then reset caches and invalidates subject token.
		 */
		void
		check_input_layer_proxies();
	};
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYNETWORKRESOLVERLAYERPROXY_H
