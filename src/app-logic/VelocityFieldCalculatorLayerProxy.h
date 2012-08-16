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

#ifndef GPLATES_APP_LOGIC_VELOCITYFIELDCALCULATORLAYERPROXY_H
#define GPLATES_APP_LOGIC_VELOCITYFIELDCALCULATORLAYERPROXY_H

#include <vector>
#include <boost/optional.hpp>

#include "AppLogicFwd.h"
#include "LayerProxy.h"
#include "LayerProxyUtils.h"
#include "ReconstructionLayerProxy.h"
#include "ReconstructLayerProxy.h"
#include "TopologyBoundaryResolverLayerProxy.h"
#include "TopologyNetworkResolverLayerProxy.h"

#include "utils/SubjectObserverToken.h"


namespace GPlatesAppLogic
{
	/**
	 * A layer proxy that calculates velocity fields on domains of mesh points inside
	 * reconstructed static polygons, resolved topological dynamic polygons or resolved
	 * topological networks.
	 */
	class VelocityFieldCalculatorLayerProxy :
			public LayerProxy
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a VelocityFieldCalculatorLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<VelocityFieldCalculatorLayerProxy> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a VelocityFieldCalculatorLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<const VelocityFieldCalculatorLayerProxy> non_null_ptr_to_const_type;


		/**
		 * Creates a @a VelocityFieldCalculatorLayerProxy object.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new VelocityFieldCalculatorLayerProxy());
		}


		~VelocityFieldCalculatorLayerProxy();


		/**
		 * Returns the velocities in multi-point vector fields, for the current reconstruction time,
		 * by appending them to them to @a multi_point_vector_fields.
		 */
		void
		get_velocity_multi_point_vector_fields(
				std::vector<multi_point_vector_field_non_null_ptr_type> &multi_point_vector_fields)
		{
			get_velocity_multi_point_vector_fields(multi_point_vector_fields, d_current_reconstruction_time);
		}


		/**
		 * Returns the velocities in multi-point vector fields, at the specified time, by appending
		 * them to them to @a multi_point_vector_fields.
		 */
		void
		get_velocity_multi_point_vector_fields(
				std::vector<multi_point_vector_field_non_null_ptr_type> &multi_point_vector_fields,
				const double &reconstruction_time);


		/**
		 * Returns the subject token that clients can use to determine if the velocities have
		 * changed since they were last retrieved.
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
		 * Set the reconstruction layer proxy.
		 */
		void
		set_current_reconstruction_layer_proxy(
				const ReconstructionLayerProxy::non_null_ptr_type &reconstruction_layer_proxy);

		/**
		 * Add a reconstructed static polygons layer proxy.
		 */
		void
		add_reconstructed_polygons_layer_proxy(
				const ReconstructLayerProxy::non_null_ptr_type &reconstructed_polygons_layer_proxy);

		/**
		 * Remove a reconstructed static polygons layer proxy.
		 */
		void
		remove_reconstructed_polygons_layer_proxy(
				const ReconstructLayerProxy::non_null_ptr_type &reconstructed_polygons_layer_proxy);

		/**
		 * Add a topological boundary resolver layer proxy.
		 */
		void
		add_topological_boundary_resolver_layer_proxy(
				const TopologyBoundaryResolverLayerProxy::non_null_ptr_type &topological_boundary_resolver_layer_proxy);

		/**
		 * Remove a topological boundary resolver layer proxy.
		 */
		void
		remove_topological_boundary_resolver_layer_proxy(
				const TopologyBoundaryResolverLayerProxy::non_null_ptr_type &topological_boundary_resolver_layer_proxy);

		/**
		 * Add a topological network resolver layer proxy.
		 */
		void
		add_topological_network_resolver_layer_proxy(
				const TopologyNetworkResolverLayerProxy::non_null_ptr_type &topological_network_resolver_layer_proxy);

		/**
		 * Remove a topological network resolver layer proxy.
		 */
		void
		remove_topological_network_resolver_layer_proxy(
				const TopologyNetworkResolverLayerProxy::non_null_ptr_type &topological_network_resolver_layer_proxy);

		/**
		 * Add to the list of feature collections containing (multi-point) geometries.
		 *
		 * NOTE: Originally the geometries were expected to be multi-point geometries but now any
		 * geometry type can be used (the points in the geometry are treated as a collection of points).
		 */
		void
		add_velocity_domain_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		/**
		 * Remove from the list of feature collections containing (multi-point) geometries.
		 *
		 * NOTE: Originally the geometries were expected to be multi-point geometries but now any
		 * geometry type can be used (the points in the geometry are treated as a collection of points).
		 */
		void
		remove_velocity_domain_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		/**
		 * A feature collection containing (multi-point) geometries was modified.
		 *
		 * NOTE: Originally the geometries were expected to be multi-point geometries but now any
		 * geometry type can be used (the points in the geometry are treated as a collection of points).
		 */
		void
		modified_velocity_domain_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

	private:
		/**
		 * The input feature collections containing the points at which to calculate velocities.
		 */
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> d_current_velocity_domain_feature_collections;

		/**
		 * Used to get reconstruction trees at desired reconstruction times.
		 */
		LayerProxyUtils::InputLayerProxy<ReconstructionLayerProxy> d_current_reconstruction_layer_proxy;

		/**
		 * Used to get reconstructed static polygon surfaces to calculate velocities on.
		 */
		LayerProxyUtils::InputLayerProxySequence<ReconstructLayerProxy>
				d_current_reconstructed_polygon_layer_proxies;

		/**
		 * Used to get resolved topology boundary surfaces to calculate velocities on.
		 */
		LayerProxyUtils::InputLayerProxySequence<TopologyBoundaryResolverLayerProxy>
				d_current_topological_boundary_resolver_layer_proxies;

		/**
		 * Used to get resolved topology network surfaces to calculate velocities on.
		 */
		LayerProxyUtils::InputLayerProxySequence<TopologyNetworkResolverLayerProxy>
				d_current_topological_network_resolver_layer_proxies;

		/**
		 * The current reconstruction time as set by the layer system.
		 */
		double d_current_reconstruction_time;

		/**
		 * The cached velocities.
		 */
		boost::optional< std::vector<multi_point_vector_field_non_null_ptr_type> >
				d_cached_multi_point_velocity_fields;

		/**
		 * Cached reconstruction time.
		 */
		boost::optional<GPlatesMaths::real_t> d_cached_reconstruction_time;

		/**
		 * Used to notify polling observers that we've been updated.
		 */
		mutable GPlatesUtils::SubjectToken d_subject_token;


		//! Default constructor.
		VelocityFieldCalculatorLayerProxy();


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

#endif // GPLATES_APP_LOGIC_VELOCITYFIELDCALCULATORLAYERPROXY_H
