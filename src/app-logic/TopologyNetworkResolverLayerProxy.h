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
#include "ReconstructionLayerProxy.h"
#include "ReconstructLayerProxy.h"

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
		 */
		void
		get_resolved_topological_networks(
				std::vector<resolved_topological_network_non_null_ptr_type> &resolved_topological_networks)
		{
			get_resolved_topological_networks(resolved_topological_networks, d_current_reconstruction_time);
		}


		/**
		 * Returns the resolved topological boundaries, at the specified time, by appending
		 * them to them to @a resolved_topological_networks.
		 */
		void
		get_resolved_topological_networks(
				std::vector<resolved_topological_network_non_null_ptr_type> &resolved_topological_networks,
				const double &reconstruction_time);


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
		 * Set the reconstruct layer proxy used to reconstruct the topological sections.
		 */
		void
		set_current_reconstruct_layer_proxy(
				const boost::optional<ReconstructLayerProxy::non_null_ptr_type> &reconstruct_layer_proxy);

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
		 * The input feature collections to reconstruct.
		 */
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> d_current_topological_network_feature_collections;

		/**
		 * Used to get reconstruction trees at desired reconstruction times.
		 */
		LayerProxyUtils::InputLayerProxy<ReconstructionLayerProxy> d_current_reconstruction_layer_proxy;

		/**
		 * Used to get reconstructed features that form the topological sections.
		 */
		LayerProxyUtils::OptionalInputLayerProxy<ReconstructLayerProxy> d_current_reconstruct_layer_proxy;

		/**
		 * The current reconstruction time as set by the layer system.
		 */
		double d_current_reconstruction_time;

		/**
		 * The cached reconstructed feature geometries.
		 */
		boost::optional< std::vector<resolved_topological_network_non_null_ptr_type> >
				d_cached_resolved_topological_networks;

		/**
		 * Cached reconstruction time.
		 */
		boost::optional<GPlatesMaths::real_t> d_cached_reconstruction_time;

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
