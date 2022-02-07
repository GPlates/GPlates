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

#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include "LayerProxy.h"
#include "LayerProxyUtils.h"
#include "MultiPointVectorField.h"
#include "ReconstructionLayerProxy.h"
#include "ReconstructLayerProxy.h"
#include "TopologyGeometryResolverLayerProxy.h"
#include "TopologyNetworkResolverLayerProxy.h"
#include "VelocityParams.h"

#include "utils/KeyValueCache.h"
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
		 * The maximum number of velocity results to cache for different reconstruction time / velocity param
		 * combinations - each combination represents one cached object.
		 *
		 * A value of 2 is suitable since rendering a velocity layer will typically use one velocity
		 * delta time while the export velocity animation might override it and use another.
		 *
		 * WARNING: This value has a direct affect on the memory used by GPlates.
		 * The cache is mainly to allow multiple clients to make different velocity
		 * requests (eg, different reconstruction time and/or velocity params) without
		 * each one invalidating the cache and forcing already calculated results (for a
		 * particular reconstruction time / velocity params pair) to be calculated again
		 * in the same frame).
		 */
		static const unsigned int MAX_NUM_VELOCITY_RESULTS_IN_CACHE = 2;


		/**
		 * Creates a @a VelocityFieldCalculatorLayerProxy object.
		 */
		static
		non_null_ptr_type
		create(
				const VelocityParams &velocity_params = VelocityParams(),
				unsigned int max_num_velocity_results_in_cache = MAX_NUM_VELOCITY_RESULTS_IN_CACHE)
		{
			return non_null_ptr_type(
					new VelocityFieldCalculatorLayerProxy(
							velocity_params, max_num_velocity_results_in_cache));
		}


		~VelocityFieldCalculatorLayerProxy();


		//
		// Getting a sequence of @a MultiPointVectorField objects.
		//

		/**
		 * Returns the velocities in multi-point vector fields, for the current velocity params and
		 * current reconstruction time, by appending them to them to @a multi_point_vector_fields.
		 */
		void
		get_velocity_multi_point_vector_fields(
				std::vector<MultiPointVectorField::non_null_ptr_type> &multi_point_vector_fields)
		{
			return get_velocity_multi_point_vector_fields(
					multi_point_vector_fields, d_current_velocity_params, d_current_reconstruction_time);
		}

		/**
		 * Returns the velocities, for the specified velocity params and
		 * current reconstruction time, by appending them to @a multi_point_vector_fields.
		 */
		void
		get_velocity_multi_point_vector_fields(
				std::vector<MultiPointVectorField::non_null_ptr_type> &multi_point_vector_fields,
				const VelocityParams &velocity_params)
		{
			return get_velocity_multi_point_vector_fields(
					multi_point_vector_fields, velocity_params, d_current_reconstruction_time);
		}

		/**
		 * Returns the velocities, for the current velocity params and
		 * specified reconstruction time, by appending them to @a multi_point_vector_fields.
		 */
		void
		get_velocity_multi_point_vector_fields(
				std::vector<MultiPointVectorField::non_null_ptr_type> &multi_point_vector_fields,
				const double &reconstruction_time)
		{
			return get_velocity_multi_point_vector_fields(
					multi_point_vector_fields, d_current_velocity_params, reconstruction_time);
		}

		/**
		 * Returns the velocities, for the specified velocity params and
		 * reconstruction time, by appending them to @a multi_point_vector_fields.
		 */
		void
		get_velocity_multi_point_vector_fields(
				std::vector<MultiPointVectorField::non_null_ptr_type> &multi_point_vector_fields,
				const VelocityParams &velocity_params,
				const double &reconstruction_time);


		//
		// Getting current velocity params and reconstruction time as set by the layer system.
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
		 * Gets the parameters used for calculating velocities.
		 */
		const VelocityParams &
		get_current_velocity_params() const
		{
			return d_current_velocity_params;
		}


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
		 * Sets the parameters used for calculating velocities.
		 */
		void
		set_current_velocity_params(
				const VelocityParams &velocity_params);

		/**
		 * Add a velocity domain layer proxy.
		 */
		void
		add_velocity_domain_layer_proxy(
				const ReconstructLayerProxy::non_null_ptr_type &velocity_domain_layer_proxy);

		/**
		 * Remove a velocity domain layer proxy.
		 */
		void
		remove_velocity_domain_layer_proxy(
				const ReconstructLayerProxy::non_null_ptr_type &velocity_domain_layer_proxy);

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
				const TopologyGeometryResolverLayerProxy::non_null_ptr_type &topological_boundary_resolver_layer_proxy);

		/**
		 * Remove a topological boundary resolver layer proxy.
		 */
		void
		remove_topological_boundary_resolver_layer_proxy(
				const TopologyGeometryResolverLayerProxy::non_null_ptr_type &topological_boundary_resolver_layer_proxy);

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

	private:

		/**
		 * Contains optional multi-point velocity fields.
		 *
		 * Each instance of this structure represents cached velocity information for
		 * a specific reconstruction time and velocity parameters.
		 */
		struct VelocityInfo
		{
			/**
			 * The cached velocities.
			 */
			boost::optional< std::vector<MultiPointVectorField::non_null_ptr_type> > cached_multi_point_velocity_fields;
		};

		//! Typedef for the key type to the velocity cache (reconstruction time and velocity params).
		typedef std::pair<GPlatesMaths::real_t, VelocityParams> velocity_cache_key_type;

		//! Typedef for the value type stored in the velocity cache.
		typedef VelocityInfo velocity_cache_value_type;

		/**
		 * Typedef for a cache of velocity information keyed by reconstruction time and velocity params.
		 */
		typedef GPlatesUtils::KeyValueCache<velocity_cache_key_type, velocity_cache_value_type> velocity_cache_type;


		/**
		 * Used to get reconstructed static polygon surfaces to calculate velocities on.
		 */
		LayerProxyUtils::InputLayerProxySequence<ReconstructLayerProxy>
				d_current_reconstructed_polygon_layer_proxies;

		/**
		 * Used to get resolved topology boundary surfaces to calculate velocities on.
		 */
		LayerProxyUtils::InputLayerProxySequence<TopologyGeometryResolverLayerProxy>
				d_current_topological_boundary_resolver_layer_proxies;

		/**
		 * Used to get resolved topology network surfaces to calculate velocities on.
		 */
		LayerProxyUtils::InputLayerProxySequence<TopologyNetworkResolverLayerProxy>
				d_current_topological_network_resolver_layer_proxies;

		/**
		 * Used to get velocity domain geometries to calculate velocities at.
		 */
		LayerProxyUtils::InputLayerProxySequence<ReconstructLayerProxy>
				d_current_velocity_domain_layer_proxies;

		/**
		 * The current reconstruction time as set by the layer system.
		 */
		double d_current_reconstruction_time;

		/**
		 * The current velocity parameters as set by the layer system.
		 */
		VelocityParams d_current_velocity_params;

		/**
		 * The velocities cached according to reconstruction time and velocity params.
		 */
		velocity_cache_type d_cached_velocities;

		/**
		 * Used to notify polling observers that we've been updated.
		 */
		mutable GPlatesUtils::SubjectToken d_subject_token;


		explicit
		VelocityFieldCalculatorLayerProxy(
				const VelocityParams &velocity_params,
				unsigned int max_num_velocity_results_in_cache);


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


		/**
		 * Generates velocities for the specified velocity params and reconstruction time
		 * if they're not already cached.
		 */
		std::vector<MultiPointVectorField::non_null_ptr_type> &
		cache_velocities(
				VelocityInfo &velocity_info,
				const VelocityParams &velocity_params,
				const double &reconstruction_time);
	};
}

#endif // GPLATES_APP_LOGIC_VELOCITYFIELDCALCULATORLAYERPROXY_H
