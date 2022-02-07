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

#include "LayerProxy.h"
#include "LayerProxyUtils.h"
#include "ReconstructContext.h"
#include "ReconstructedScalarCoverage.h"
#include "ReconstructHandle.h"
#include "ReconstructLayerProxy.h"
#include "ReconstructScalarCoverageParams.h"
#include "ScalarCoverageDeformation.h"
#include "ScalarCoverageEvolution.h"
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
		 * Returns the reconstructed scalar coverages, for the current scalar type, coverage params and
		 * current reconstruction time, by appending them to them to @a reconstructed_scalar_coverages.
		 */
		ReconstructHandle::type
		get_reconstructed_scalar_coverages(
				std::vector<ReconstructedScalarCoverage::non_null_ptr_type> &reconstructed_scalar_coverages)
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
				std::vector<ReconstructedScalarCoverage::non_null_ptr_type> &reconstructed_scalar_coverages,
				const GPlatesPropertyValues::ValueObjectType &scalar_type)
		{
			return get_reconstructed_scalar_coverages(
					reconstructed_scalar_coverages,
					scalar_type,
					d_current_reconstruct_scalar_coverage_params,
					d_current_reconstruction_time);
		}

		/**
		 * Returns the reconstructed scalar coverages, for the specified scalar coverage params and
		 * current scalar type and reconstruction time, by appending them to @a reconstructed_scalar_coverages.
		 */
		ReconstructHandle::type
		get_reconstructed_scalar_coverages(
				std::vector<ReconstructedScalarCoverage::non_null_ptr_type> &reconstructed_scalar_coverages,
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
		 * current scalar type and coverage params, by appending them to @a reconstructed_scalar_coverages.
		 */
		ReconstructHandle::type
		get_reconstructed_scalar_coverages(
				std::vector<ReconstructedScalarCoverage::non_null_ptr_type> &reconstructed_scalar_coverages,
				const double &reconstruction_time)
		{
			return get_reconstructed_scalar_coverages(
					reconstructed_scalar_coverages,
					d_current_scalar_type,
					d_current_reconstruct_scalar_coverage_params,
					reconstruction_time);
		}

		/**
		 * Returns the reconstructed scalar coverages, for the specified scalar type and coverage params
		 * and current reconstruction time, by appending them to @a reconstructed_scalar_coverages.
		 */
		ReconstructHandle::type
		get_reconstructed_scalar_coverages(
				std::vector<ReconstructedScalarCoverage::non_null_ptr_type> &reconstructed_scalar_coverages,
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
		 * and current coverage params, by appending them to @a reconstructed_scalar_coverages.
		 */
		ReconstructHandle::type
		get_reconstructed_scalar_coverages(
				std::vector<ReconstructedScalarCoverage::non_null_ptr_type> &reconstructed_scalar_coverages,
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
		 * Returns the reconstructed scalar coverages, for the specified coverage params and reconstruction time
		 * and current scalar type, by appending them to @a reconstructed_scalar_coverages.
		 */
		ReconstructHandle::type
		get_reconstructed_scalar_coverages(
				std::vector<ReconstructedScalarCoverage::non_null_ptr_type> &reconstructed_scalar_coverages,
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
		 * Returns the reconstructed scalar coverages, for the specified scalar type, coverage params and
		 * reconstruction time, by appending them to @a reconstructed_scalar_coverages.
		 */
		ReconstructHandle::type
		get_reconstructed_scalar_coverages(
				std::vector<ReconstructedScalarCoverage::non_null_ptr_type> &reconstructed_scalar_coverages,
				const GPlatesPropertyValues::ValueObjectType &scalar_type,
				const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params,
				const double &reconstruction_time);


		/**
		 * A time span of scalar coverages associated with a feature and a specific scalar type.
		 */
		class ReconstructedScalarCoverageTimeSpan
		{
		public:

			/**
			 * Association of scalar coverage time span with domain/range properties.
			 */
			class ScalarCoverageTimeSpan
			{
			public:

				ScalarCoverageTimeSpan(
						GPlatesModel::FeatureHandle::iterator domain_property_iterator,
						GPlatesModel::FeatureHandle::iterator range_property_iterator,
						const ScalarCoverageDeformation::ScalarCoverageTimeSpan::non_null_ptr_type &scalar_coverage_time_span) :
					d_domain_property_iterator(domain_property_iterator),
					d_range_property_iterator(range_property_iterator),
					d_scalar_coverage_time_span(scalar_coverage_time_span)
				{  }


				/**
				 * Access the feature property which contained the domain geometry associated with the scalar values.
				 */
				const GPlatesModel::FeatureHandle::iterator
				get_domain_property() const
				{
					return d_domain_property_iterator;
				}

				/**
				 * Access the feature property from which the scalar values were reconstructed.
				 */
				const GPlatesModel::FeatureHandle::iterator
				get_range_property() const
				{
					return d_range_property_iterator;
				}

				/**
				 * The scalar coverage time span associated with this geometry property.
				 */
				ScalarCoverageDeformation::ScalarCoverageTimeSpan::non_null_ptr_type
				get_scalar_coverage_time_span() const
				{
					return d_scalar_coverage_time_span;
				}

				/**
				 * Returns optional geometry time span if one was used (to obtain deformation info to
				 * evolve scalar values, or to deactivate points/scalars, or both).
				 *
				 * Returns none if a geometry time span was not used
				 * (ie, if associated domain geometry was not topologically reconstructed).
				 */
				boost::optional<TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type>
				get_geometry_time_span() const
				{
					return d_scalar_coverage_time_span->get_geometry_time_span();
				}

			private:
				GPlatesModel::FeatureHandle::iterator d_domain_property_iterator;
				GPlatesModel::FeatureHandle::iterator d_range_property_iterator;
				ScalarCoverageDeformation::ScalarCoverageTimeSpan::non_null_ptr_type d_scalar_coverage_time_span;
			};

			//! Typedef for a sequence of @a ScalarCoverageTimeSpan objects.
			typedef std::vector<ScalarCoverageTimeSpan> scalar_coverage_time_span_seq_type;


			ReconstructedScalarCoverageTimeSpan(
					const GPlatesModel::FeatureHandle::weak_ref &feature,
					const GPlatesPropertyValues::ValueObjectType &scalar_type) :
				d_feature(feature),
				d_scalar_type(scalar_type)
			{  }

			ReconstructedScalarCoverageTimeSpan(
					const GPlatesModel::FeatureHandle::weak_ref &feature,
					const GPlatesPropertyValues::ValueObjectType &scalar_type,
					const scalar_coverage_time_span_seq_type &scalar_coverage_time_spans) :
				d_feature(feature),
				d_scalar_type(scalar_type),
				d_scalar_coverage_time_spans(scalar_coverage_time_spans)
			{  }

			/**
			 * Returns the feature.
			 */
			const GPlatesModel::FeatureHandle::weak_ref &
			get_feature() const
			{
				return d_feature;
			}

			/**
			 * Returns the type of the scalar values in the scalar coverage time spans.
			 *
			 * Each range feature property contains one or more scalar sequences.
			 * Each scalar sequence is identified by a scalar type.
			 */
			const GPlatesPropertyValues::ValueObjectType &
			get_scalar_type() const
			{
				return d_scalar_type;
			}

			/**
			 * Returns the scalar coverage time spans of 'this' feature that match @a get_scalar_type.
			 */
			const scalar_coverage_time_span_seq_type &
			get_scalar_coverage_time_spans() const
			{
				return d_scalar_coverage_time_spans;
			}

		private:
			GPlatesModel::FeatureHandle::weak_ref d_feature;
			GPlatesPropertyValues::ValueObjectType d_scalar_type;
			scalar_coverage_time_span_seq_type d_scalar_coverage_time_spans;

			friend class ReconstructScalarCoverageLayerProxy;
		};


		//
		// Getting a sequence of @a ReconstructedScalarCoverageTimeSpan objects.
		//

		/**
		 * Returns the reconstructed scalar coverage time spans, for the current scalar type and coverage params,
		 * by appending them to them to @a reconstructed_scalar_coverage_time_spans.
		 */
		void
		get_reconstructed_scalar_coverage_time_spans(
				std::vector<ReconstructedScalarCoverageTimeSpan> &reconstructed_scalar_coverage_time_spans)
		{
			get_reconstructed_scalar_coverage_time_spans(
					reconstructed_scalar_coverage_time_spans,
					d_current_scalar_type,
					d_current_reconstruct_scalar_coverage_params);
		}

		/**
		 * Returns the reconstructed scalar coverage time spans, for the specified scalar type and the
		 * current coverage params, by appending them to @a reconstructed_scalar_coverage_time_spans.
		 */
		void
		get_reconstructed_scalar_coverage_time_spans(
				std::vector<ReconstructedScalarCoverageTimeSpan> &reconstructed_scalar_coverage_time_spans,
				const GPlatesPropertyValues::ValueObjectType &scalar_type)
		{
			get_reconstructed_scalar_coverage_time_spans(
					reconstructed_scalar_coverage_time_spans,
					scalar_type,
					d_current_reconstruct_scalar_coverage_params);
		}

		/**
		 * Returns the reconstructed scalar coverage time spans, for the specified scalar coverage params and
		 * current scalar type, by appending them to @a reconstructed_scalar_coverage_time_spans.
		 */
		void
		get_reconstructed_scalar_coverage_time_spans(
				std::vector<ReconstructedScalarCoverageTimeSpan> &reconstructed_scalar_coverage_time_spans,
				const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params)
		{
			get_reconstructed_scalar_coverage_time_spans(
					reconstructed_scalar_coverage_time_spans,
					d_current_scalar_type,
					reconstruct_scalar_coverage_params);
		}

		/**
		 * Returns the reconstructed scalar coverage time spans, for the specified scalar type and coverage params,
		 * by appending them to @a reconstructed_scalar_coverage_time_spans.
		 */
		void
		get_reconstructed_scalar_coverage_time_spans(
				std::vector<ReconstructedScalarCoverageTimeSpan> &reconstructed_scalar_coverage_time_spans,
				const GPlatesPropertyValues::ValueObjectType &scalar_type,
				const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params);


		/**
		 * Gets all scalar coverages available across the scalar coverage features.
		 */
		void
		get_scalar_coverages(
				std::vector<ScalarCoverageFeatureProperties::Coverage> &scalar_coverages);


		/**
		 * Gets all scalar types available across the scalar coverage features.
		 */
		void
		get_scalar_types(
				std::vector<GPlatesPropertyValues::ValueObjectType> &scalar_types);


		//
		// Getting current scalar coverage params and reconstruction time as set by the layer system.
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
		 * Contains optional cached scalar coverage time spans.
		 */
		struct ScalarCoverageTimeSpanInfo
		{
			/**
			 * The reconstructed scalar value time spans.
			 */
			std::vector<ReconstructedScalarCoverageTimeSpan> cached_reconstructed_scalar_coverage_time_spans;

			/**
			 * The map to look up scalar value time spans indexed by geometry property.
			 */
			scalar_coverage_time_span_map_type cached_scalar_coverage_time_span_map;
		};


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
		 * Cached scalar type (each GmlDataBlock can have multiple scalars).
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
		 * Cached scalar coverages parameters associated with @a d_cached_scalar_coverage_time_span_map.
		 */
		boost::optional<ReconstructScalarCoverageParams> d_cached_reconstruct_scalar_coverage_params;

		/**
		 * The cached scalar value time spans.
		 */
		boost::optional<ScalarCoverageTimeSpanInfo> d_cached_scalar_coverage_time_span_info;

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
		 * Cache time spans for topology-reconstructed scalar coverages.
		 */
		void
		cache_topology_reconstructed_scalar_coverage_time_spans(
				const GPlatesPropertyValues::ValueObjectType &scalar_type,
				const std::vector<ReconstructContext::TopologyReconstructedFeatureTimeSpan> &topology_reconstructed_feature_time_spans,
				boost::optional<scalar_evolution_function_type> scalar_evolution_function);

		/**
		 * Cache time spans for non-topology-reconstructed scalar coverages.
		 */
		void
		cache_non_topology_reconstructed_scalar_coverage_time_spans(
				const GPlatesPropertyValues::ValueObjectType &scalar_type,
				const std::vector<GPlatesModel::FeatureHandle::weak_ref> &domain_features);


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
