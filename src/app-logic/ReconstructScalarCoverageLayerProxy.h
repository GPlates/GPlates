/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTSCALARCOVERAGELAYERPROXY_H
#define GPLATES_APP_LOGIC_RECONSTRUCTSCALARCOVERAGELAYERPROXY_H

#include <map>
#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include "AppLogicFwd.h"
#include "LayerProxy.h"
#include "LayerProxyUtils.h"
#include "ReconstructedScalarCoverage.h"
#include "ReconstructHandle.h"
#include "ReconstructLayerProxy.h"
#include "ReconstructScalarCoverageLayerTask.h"
#include "ReconstructScalarCoverageParams.h"
#include "ScalarCoverageDeformation.h"
#include "ScalarCoverageFeatureProperties.h"

#include "model/FeatureHandle.h"

#include "property-values/ValueObjectType.h"

#include "utils/KeyValueCache.h"
#include "utils/SubjectObserverToken.h"


namespace GPlatesAppLogic
{
	/**
	 * A layer proxy that can evolve specific types of scalar coverages over time
	 * (such as crustal thickness and topography).
	 *
	 * The domains are regular geometries (points/multipoints/polylines/polygons) whose positions
	 * are deformed by a ReconstructLayerProxy, whereas the scalar values associated with those positions
	 * can be evolved (according to strain calculated in ReconstructLayerProxy) to account for the
	 * deformation in the resolved topological networks.
	 *
	 * If the type of scalar coverage does not support evolving (changing over time due to deformation)
	 * then the scalar values are not modified (they remain constant over time).
	 */
	class ReconstructScalarCoverageLayerProxy :
			public LayerProxy
	{
	public:

		//! A convenience typedef for a shared pointer to a non-const @a ReconstructScalarCoverageLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructScalarCoverageLayerProxy> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a ReconstructScalarCoverageLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructScalarCoverageLayerProxy> non_null_ptr_to_const_type;


		/**
		 * The maximum number of reconstructions to cache for different reconstruction times -
		 * each combination represents one cached object.
		 *
		 * WARNING: This value has a direct affect on the memory used by GPlates.
		 * Setting this too high can result in significant memory usage.
		 */
		static const unsigned int MAX_NUM_RECONSTRUCTIONS_IN_CACHE = 4;


		/**
		 * Creates a @a ReconstructScalarCoverageLayerProxy object.
		 */
		static
		non_null_ptr_type
		create(
				const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params = ReconstructScalarCoverageParams(),
				unsigned int max_num_reconstructions_in_cache = MAX_NUM_RECONSTRUCTIONS_IN_CACHE)
		{
			return non_null_ptr_type(
					new ReconstructScalarCoverageLayerProxy(
							reconstruct_scalar_coverage_params, max_num_reconstructions_in_cache));
		}


		~ReconstructScalarCoverageLayerProxy();


		//
		// Getting a sequence of @a ReconstructedScalarCoverage objects.
		//

		/**
		 * Returns the reconstructed scalar coverages, for the current scalar type, coverages params and
		 * current reconstruction time, by appending them to them to @a reconstructed_scalar_coverages.
		 */
		ReconstructHandle::type
		get_reconstructed_scalar_coverages(
				std::vector<reconstructed_scalar_coverage_non_null_ptr_type> &reconstructed_scalar_coverages)
		{
			return get_reconstructed_scalar_coverages(
					reconstructed_scalar_coverages,
					d_current_scalar_type,
					d_current_reconstruct_scalar_coverage_params,
					d_current_reconstruction_time);
		}

		/**
		 * Returns the reconstructed scalar coverages, for the specified scalar type and the
		 * current coverage params and reconstruction time, by appending them to @a reconstructed_scalar_coverages.
		 */
		ReconstructHandle::type
		get_reconstructed_scalar_coverages(
				std::vector<reconstructed_scalar_coverage_non_null_ptr_type> &reconstructed_scalar_coverages,
				const GPlatesPropertyValues::ValueObjectType &scalar_type)
		{
			return get_reconstructed_scalar_coverages(
					reconstructed_scalar_coverages,
					scalar_type,
					d_current_reconstruct_scalar_coverage_params,
					d_current_reconstruction_time);
		}

		/**
		 * Returns the reconstructed scalar coverages, for the specified scalar coverages params and
		 * current scalar type and reconstruction time, by appending them to @a reconstructed_scalar_coverages.
		 */
		ReconstructHandle::type
		get_reconstructed_scalar_coverages(
				std::vector<reconstructed_scalar_coverage_non_null_ptr_type> &reconstructed_scalar_coverages,
				const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params)
		{
			return get_reconstructed_scalar_coverages(
					reconstructed_scalar_coverages,
					d_current_scalar_type,
					reconstruct_scalar_coverage_params,
					d_current_reconstruction_time);
		}

		/**
		 * Returns the reconstructed scalar coverages, for the specified reconstruction time and
		 * current scalar type and coverages params, by appending them to @a reconstructed_scalar_coverages.
		 */
		ReconstructHandle::type
		get_reconstructed_scalar_coverages(
				std::vector<reconstructed_scalar_coverage_non_null_ptr_type> &reconstructed_scalar_coverages,
				const double &reconstruction_time)
		{
			return get_reconstructed_scalar_coverages(
					reconstructed_scalar_coverages,
					d_current_scalar_type,
					d_current_reconstruct_scalar_coverage_params,
					reconstruction_time);
		}

		/**
		 * Returns the reconstructed scalar coverages, for the specified scalar type and coverages params
		 * and current reconstruction time, by appending them to @a reconstructed_scalar_coverages.
		 */
		ReconstructHandle::type
		get_reconstructed_scalar_coverages(
				std::vector<reconstructed_scalar_coverage_non_null_ptr_type> &reconstructed_scalar_coverages,
				const GPlatesPropertyValues::ValueObjectType &scalar_type,
				const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params)
		{
			return get_reconstructed_scalar_coverages(
					reconstructed_scalar_coverages,
					scalar_type,
					reconstruct_scalar_coverage_params,
					d_current_reconstruction_time);
		}

		/**
		 * Returns the reconstructed scalar coverages, for the specified scalar type and reconstruction time
		 * and current coverages params, by appending them to @a reconstructed_scalar_coverages.
		 */
		ReconstructHandle::type
		get_reconstructed_scalar_coverages(
				std::vector<reconstructed_scalar_coverage_non_null_ptr_type> &reconstructed_scalar_coverages,
				const GPlatesPropertyValues::ValueObjectType &scalar_type,
				const double &reconstruction_time)
		{
			return get_reconstructed_scalar_coverages(
					reconstructed_scalar_coverages,
					scalar_type,
					d_current_reconstruct_scalar_coverage_params,
					reconstruction_time);
		}

		/**
		 * Returns the reconstructed scalar coverages, for the specified coverages params and reconstruction time
		 * and current scalar type, by appending them to @a reconstructed_scalar_coverages.
		 */
		ReconstructHandle::type
		get_reconstructed_scalar_coverages(
				std::vector<reconstructed_scalar_coverage_non_null_ptr_type> &reconstructed_scalar_coverages,
				const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params,
				const double &reconstruction_time)
		{
			return get_reconstructed_scalar_coverages(
					reconstructed_scalar_coverages,
					d_current_scalar_type,
					reconstruct_scalar_coverage_params,
					reconstruction_time);
		}

		/**
		 * Returns the reconstructed scalar coverages, for the specified scalar type, coverages params and
		 * reconstruction time, by appending them to @a reconstructed_scalar_coverages.
		 */
		ReconstructHandle::type
		get_reconstructed_scalar_coverages(
				std::vector<reconstructed_scalar_coverage_non_null_ptr_type> &reconstructed_scalar_coverages,
				const GPlatesPropertyValues::ValueObjectType &scalar_type,
				const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params,
				const double &reconstruction_time);


		/**
		 * Gets all scalar coverages available across the scalar coverage features.
		 */
		const std::vector<ScalarCoverageFeatureProperties::Coverage> &
		get_scalar_coverages();


		/**
		 * Gets all scalar types available across the scalar coverage features.
		 */
		const std::vector<GPlatesPropertyValues::ValueObjectType> &
		get_scalar_types();


		//
		// Getting current scalar coverages params and reconstruction time as set by the layer system.
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
		 * Gets the current scalar type.
		 */
		const GPlatesPropertyValues::ValueObjectType &
		get_current_scalar_type() const
		{
			return d_current_scalar_type;
		}

		/**
		 * Gets the current parameters used for scalar coverages.
		 */
		const ReconstructScalarCoverageParams &
		get_current_reconstruct_scalar_coverage_params() const
		{
			return d_current_reconstruct_scalar_coverage_params;
		}


		/**
		 * Returns the subject token that clients can use to determine if the scalar coverages have
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
		 * Sets the current scalar type as set by the layer system.
		 */
		void
		set_current_scalar_type(
				const GPlatesPropertyValues::ValueObjectType &scalar_type);

		/**
		 * Sets the parameters used for scalar coverages.
		 */
		void
		set_current_reconstruct_scalar_coverage_params(
				const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params);

		/**
		 * Add a reconstructed domain layer proxy.
		 */
		void
		add_reconstructed_domain_layer_proxy(
				const ReconstructLayerProxy::non_null_ptr_type &reconstructed_domain_layer_proxy);

		/**
		 * Remove a reconstructed domain layer proxy.
		 */
		void
		remove_reconstructed_domain_layer_proxy(
				const ReconstructLayerProxy::non_null_ptr_type &reconstructed_domain_layer_proxy);

	private:

		/**
		 * The range property iterator and scalar coverage time span.
		 */
		typedef std::pair<
				GPlatesModel::FeatureHandle::iterator, /* range property iterator */
				ScalarCoverageDeformation::ScalarCoverageTimeSpan::non_null_ptr_type>
						scalar_coverage_time_span_mapped_type;

		//! Typedef for mapping geometry properties to their scalar coverage lookup tables.
		typedef std::map<
				GPlatesModel::FeatureHandle::const_iterator,
				scalar_coverage_time_span_mapped_type>
						scalar_coverage_time_span_map_type;


		/**
		 * Contains optional reconstructed scalar coverages.
		 *
		 * Each instance of this structure represents cached reconstruction information for
		 * a specific reconstruction time.
		 *
		 * Note: When the scalar coverage parameters change then these structures get reset/removed.
		 */
		struct ReconstructionInfo
		{
			/**
			 * The reconstruct handle that identifies all cached reconstructed scalar coverages in this structure.
			 */
			boost::optional<ReconstructHandle::type> cached_reconstructed_scalar_coverages_handle;

			/**
			 * The cached reconstructed scalar coverages.
			 */
			boost::optional< std::vector<ReconstructedScalarCoverage::non_null_ptr_type> >
					cached_reconstructed_scalar_coverages;
		};

		//! Typedef for the key type to the reconstruction cache (reconstruction time).
		typedef GPlatesMaths::real_t reconstruction_time_type;

		//! Typedef for the value type stored in the reconstruction cache.
		typedef ReconstructionInfo reconstruction_cache_value_type;

		/**
		 * Typedef for a cache of reconstruction information keyed by reconstruct scalar coverage params.
		 */
		typedef GPlatesUtils::KeyValueCache<
				reconstruction_time_type,
				reconstruction_cache_value_type>
						reconstruction_cache_type;


		/**
		 * Used to get reconstructed domain geometries, and optionally strains to evolve coverages at.
		 */
		LayerProxyUtils::InputLayerProxySequence<ReconstructLayerProxy>
				d_current_reconstructed_domain_layer_proxies;

		/**
		 * The current reconstruction time as set by the layer system.
		 */
		double d_current_reconstruction_time;

		/**
		 * The current scalar type (each GmlDataBlock can have multiple scalars).
		 */
		GPlatesPropertyValues::ValueObjectType d_current_scalar_type;

		/**
		 * The current scalar coverages parameters as set by the layer system.
		 */
		ReconstructScalarCoverageParams d_current_reconstruct_scalar_coverage_params;

		/**
		 * Cached scalar type (each GmlDataBlock can have multiple scalars)
		 * associated with @a d_cached_scalar_coverage_time_spans.
		 */
		boost::optional<GPlatesPropertyValues::ValueObjectType> d_cached_scalar_type;

		/**
		 * Cached scalar types associated with the reconstructed domain *features*.
		 */
		boost::optional< std::vector<GPlatesPropertyValues::ValueObjectType> > d_cached_scalar_types;

		/**
		 * Cached scalar coverages associated with the reconstructed domain *features*.
		 */
		boost::optional< std::vector<ScalarCoverageFeatureProperties::Coverage> > d_cached_scalar_coverages;

		/**
		 * Cached scalar coverages parameters associated with @a d_cached_scalar_coverage_time_spans.
		 */
		boost::optional<ReconstructScalarCoverageParams> d_cached_reconstruct_scalar_coverage_params;

		/**
		 * The cached scalar values (in a lookup table indexed by geometry property).
		 */
		boost::optional<scalar_coverage_time_span_map_type> d_cached_scalar_coverage_time_spans;

		/**
		 * The various reconstructions cached according to reconstruction time.
		 *
		 * Note: When the scalar coverage parameters change then this cache gets cleared.
		 */
		reconstruction_cache_type d_cached_reconstructions;

		/**
		 * Used to notify polling observers that we've been updated.
		 */
		mutable GPlatesUtils::SubjectToken d_subject_token;


		explicit
		ReconstructScalarCoverageLayerProxy(
				const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params,
				unsigned int max_num_reconstructions_in_cache);


		/**
		 * Resets all cached data forcing it to be recalculated next time it's accessed.
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
		 * Cache all scalar coverages of all scalar coverage features.
		 */
		void
		cache_scalar_coverages();


		/**
		 * Cache the unique set of scalar types of all scalar coverage features.
		 */
		void
		cache_scalar_types();


		/**
		 * Cache time spans for all scalar coverages.
		 */
		void
		cache_scalar_coverage_time_spans(
				const GPlatesPropertyValues::ValueObjectType &scalar_type,
				const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params);


		/**
		 * Cache reconstructed scalar coverages for the specified reconstruction time.
		 */
		std::vector<ReconstructedScalarCoverage::non_null_ptr_type> &
		cache_reconstructed_scalar_coverages(
				ReconstructionInfo &reconstruction_info,
				const double &reconstruction_time);


		/**
		 * Create an empty @a ReconstructionInfo for the key/value cache.
		 */
		ReconstructionInfo
		create_empty_reconstruction_info(
				const reconstruction_time_type &reconstruction_time);
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTSCALARCOVERAGELAYERPROXY_H
