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

#ifndef GPLATES_APP_LOGIC_TOPOLOGYGEOMETRYRESOLVERLAYERPROXY_H
#define GPLATES_APP_LOGIC_TOPOLOGYGEOMETRYRESOLVERLAYERPROXY_H

#include <set>
#include <vector>
#include <boost/optional.hpp>

#include "DependentTopologicalSectionLayers.h"
#include "LayerProxy.h"
#include "LayerProxyUtils.h"
#include "ReconstructHandle.h"
#include "ReconstructionLayerProxy.h"
#include "ReconstructLayerProxy.h"
#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalLine.h"
#include "TopologyReconstruct.h"

#include "model/FeatureHandle.h"
#include "model/FeatureId.h"

#include "utils/SubjectObserverToken.h"


namespace GPlatesAppLogic
{
	/**
	 * A layer proxy that resolves topological geometries (boundaries and lines) from feature collection(s)
	 * containing topological boundary and line features.
	 */
	class TopologyGeometryResolverLayerProxy :
			public LayerProxy
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a TopologyGeometryResolverLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<TopologyGeometryResolverLayerProxy> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a TopologyGeometryResolverLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<const TopologyGeometryResolverLayerProxy> non_null_ptr_to_const_type;


		/**
		 * Creates a @a TopologyGeometryResolverLayerProxy object.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new TopologyGeometryResolverLayerProxy());
		}


		~TopologyGeometryResolverLayerProxy();


		/**
		 * Returns the resolved topological geometries (polygons and polylines), for the current reconstruction time,
		 * by appending them to them to @a resolved_topological_geometries.
		 *
		 * If @a reconstruct_handles is specified then reconstruct handles for the resolved topological
		 * boundaries and lines are appended to it.
		 */
		void
		get_resolved_topological_geometries(
				std::vector<ResolvedTopologicalGeometry::non_null_ptr_type> &resolved_topological_geometries,
				boost::optional<std::vector<ReconstructHandle::type> &> reconstruct_handles = boost::none)
		{
			get_resolved_topological_geometries(
					resolved_topological_geometries, 
					d_current_reconstruction_time,
					reconstruct_handles);
		}

		/**
		 * Returns the resolved topological geometries (polygons and polylines), at the specified time,
		 * by appending them to them to @a resolved_topological_geometries.
		 */
		void
		get_resolved_topological_geometries(
				std::vector<ResolvedTopologicalGeometry::non_null_ptr_type> &resolved_topological_geometries,
				const double &reconstruction_time,
				boost::optional<std::vector<ReconstructHandle::type> &> reconstruct_handles = boost::none);


		/**
		 * Returns the resolved topological boundaries (polygons), for the current reconstruction time,
		 * by appending them to them to @a resolved_topological_boundaries.
		 *
		 * NOTE: These are resolved topological geometries that are *polygons*.
		 *
		 * NOTE: The resolved topological polygons can in turn reference resolved topological polylines.
		 * Currently there is a topological referencing restriction where resolved topological polygons can reference
		 * resolved topological polylines (and regular static geometries) but resolved topological polylines
		 * can only reference regular static geometries (and not resolved topological geometries of any type).
		 * This ordering enables the resolving process to ensure all geometries referenced by a topology
		 * are fully reconstructed (and resolved) before that topology itself is resolved. This is needed
		 * because the topological referencing is done using feature-id lookup instead of querying input layers
		 * and only the latter can truly guarantee a correct dependency ordering without intervention
		 * required - in the former case that intervention is the imposed topological referencing restriction.
		 */
		ReconstructHandle::type
		get_resolved_topological_boundaries(
				std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries)
		{
			return get_resolved_topological_boundaries(
					resolved_topological_boundaries, 
					d_current_reconstruction_time);
		}

		/**
		 * Returns the resolved topological boundaries (polygons), at the specified time, by appending
		 * them to them to @a resolved_topological_boundaries.
		 */
		ReconstructHandle::type
		get_resolved_topological_boundaries(
				std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries,
				const double &reconstruction_time);


		/**
		 * Returns the resolved topological lines (polylines), for the current reconstruction time,
		 * by appending them to them to @a resolved_topological_lines.
		 *
		 * NOTE: These are resolved topological geometries that are *polylines*.
		 *
		 * NOTE: The resolved topological polylines can *not* reference resolved topological
		 * boundaries (polygons) the latter can reference the former.
		 * See comment for @a get_resolved_topological_boundaries for the reasoning behind this.
		 */
		ReconstructHandle::type
		get_resolved_topological_lines(
				std::vector<ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_lines)
		{
			return get_resolved_topological_lines(
					resolved_topological_lines, 
					d_current_reconstruction_time);
		}

		/**
		 * Returns the resolved topological lines (polylines), at the specified time, by appending
		 * them to them to @a resolved_topological_lines.
		 */
		ReconstructHandle::type
		get_resolved_topological_lines(
				std::vector<ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_lines,
				const double &reconstruction_time);


		/**
		 * Same as @a get_resolved_topological_lines except limits the resolved topological lines
		 * to those matching the specified feature IDs.
		 *
		 * This is a useful optimisation when only a subset of all resolved lines are referenced as
		 * topological sections by a topological plate boundary or deforming network.
		 * Note that topological *boundaries* cannot be used as topological sections (only *lines* can).
		 */
		ReconstructHandle::type
		get_resolved_topological_sections(
				std::vector<ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_sections,
				const std::set<GPlatesModel::FeatureId> &topological_sections_referenced)
		{
			return get_resolved_topological_sections(
					resolved_topological_sections, 
					topological_sections_referenced,
					d_current_reconstruction_time);
		}

		ReconstructHandle::type
		get_resolved_topological_sections(
				std::vector<ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_sections,
				const std::set<GPlatesModel::FeatureId> &topological_sections_referenced,
				const double &reconstruction_time);


		/**
		 * Returns a time span of resolved topological boundaries.
		 *
		 * The main purpose of this method, over calling @a get_resolved_topological_boundaries for
		 * a sequence of times (which essentially does the same thing), is to cache a time range
		 * of resolved topological boundaries rather than just caching for a single reconstruction time.
		 * This helps to avoid the expensive process of repeating the generation of a time span of
		 * resolved boundaries which would happen if multiple clients each called
		 * @a get_resolved_topological_boundaries over a sequence of reconstruction times because
		 * a separate resolved boundary would unnecessarily be created for each client - whereas,
		 * with this method, a single time range of resolved boundaries would be shared by all clients.
		 */
		TopologyReconstruct::resolved_boundary_time_span_type::non_null_ptr_to_const_type
		get_resolved_boundary_time_span(
				const TimeSpanUtils::TimeRange &time_range);


		/**
		 * Returns only the topological geometry subset (topological lines and boundaries) of features
		 * set by @a add_topological_geometry_feature_collection, etc.
		 */
		void
		get_current_topological_geometry_features(
				std::vector<GPlatesModel::FeatureHandle::weak_ref> &topological_geometry_features) const;

		/**
		 * Returns all features set by @a add_topological_geometry_feature_collection, etc.
		 *
		 * Note that the features might be a mixture of topological and non-topological.
		 * Some files contain a mixture of both and hence create both topological and non-topological layers.
		 */
		void
		get_current_features(
				std::vector<GPlatesModel::FeatureHandle::weak_ref> &features) const;


		/**
		 * Returns the current reconstruction layer proxy used for reconstructions.
		 */
		ReconstructionLayerProxy::non_null_ptr_type
		get_current_reconstruction_layer_proxy();


		/**
		 * Inserts the feature IDs of topological sections referenced by the current topological features
		 * (resolved lines and boundaries) for *all* times (not just the current time).
		 */
		void
		get_current_dependent_topological_sections(
				std::set<GPlatesModel::FeatureId> &resolved_boundary_dependent_topological_sections,
				std::set<GPlatesModel::FeatureId> &resolved_line_dependent_topological_sections) const;


		/**
		 * Returns the subject token that clients can use to determine if the resolved
		 * topological geometries (boundaries and lines) have changed since they were last retrieved.
		 *
		 * This is mainly useful for other layers that have this layer connected as their input.
		 */
		const GPlatesUtils::SubjectToken &
		get_subject_token();

		/**
		 * Returns the subject token that clients can use to determine if *just* the resolved
		 * topological geometries *lines* have changed since they were last retrieved.
		 *
		 * This is mainly useful for other layers that source this layer for their resolved lines
		 * topological sections.
		 */
		const GPlatesUtils::SubjectToken &
		get_resolved_lines_subject_token();


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
		 * Set the reconstruction layer proxy that defines velocities inside rigid topological boundaries.
		 */
		void
		set_current_reconstruction_layer_proxy(
				const ReconstructionLayerProxy::non_null_ptr_type &reconstruction_layer_proxy);

		/**
		 * Sets the current layer proxies used to reconstruct/resolve the topological geometry sections.
		 */
		void
		set_current_topological_sections_layer_proxies(
				const std::vector<ReconstructLayerProxy::non_null_ptr_type> &
						reconstructed_geometry_topological_sections_layer_proxies,
				const std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> &
						resolved_line_topological_sections_layer_proxies);

		/**
		 * Add to the list of feature collections containing topological geometry features.
		 */
		void
		add_topological_geometry_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		/**
		 * Remove from the list of feature collections containing topological geometry features.
		 */
		void
		remove_topological_geometry_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		/**
		 * A topological geometry feature collection was modified.
		 */
		void
		modified_topological_geometry_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

	private:

		/**
		 * Contains resolved topological boundary (polygon) geometries.
		 */
		struct ResolvedBoundaries
		{
			void
			invalidate()
			{
				cached_reconstruct_handle = boost::none;
				cached_resolved_topological_boundaries = boost::none;
				cached_reconstruction_time = boost::none;
			}

			/**
			 * The reconstruct handle that identifies all cached resolved topological boundary geometries
			 * in this structure.
			 */
			boost::optional<ReconstructHandle::type> cached_reconstruct_handle;

			/**
			 * The cached resolved topological boundaries.
			 */
			boost::optional< std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> >
					cached_resolved_topological_boundaries;

			/**
			 * Cached reconstruction time.
			 */
			boost::optional<GPlatesMaths::real_t> cached_reconstruction_time;
		};

		/**
		 * Contains resolved topological line (polyline) geometries.
		 */
		struct ResolvedLines
		{
			void
			invalidate()
			{
				cached_reconstruct_handle = boost::none;
				cached_resolved_topological_lines = boost::none;
				cached_reconstruction_time = boost::none;
			}

			/**
			 * The reconstruct handle that identifies all cached resolved topological line geometries
			 * in this structure.
			 */
			boost::optional<ReconstructHandle::type> cached_reconstruct_handle;

			/**
			 * The cached resolved topological lines.
			 */
			boost::optional< std::vector<ResolvedTopologicalLine::non_null_ptr_type> >
					cached_resolved_topological_lines;

			/**
			 * Cached reconstruction time.
			 */
			boost::optional<GPlatesMaths::real_t> cached_reconstruction_time;
		};

		/**
		 * Contains resolved topological boundaries time span.
		 */
		struct ResolvedBoundaryTimeSpan
		{
			void
			invalidate()
			{
				cached_resolved_boundary_time_span = boost::none;
			}

			/**
			 * The cached resolved topologies over a range of reconstruction times.
			 */
			boost::optional<TopologyReconstruct::resolved_boundary_time_span_type::non_null_ptr_type>
					cached_resolved_boundary_time_span;
		};


		/**
		 * The subset of features that are topological lines.
		 */
		std::vector<GPlatesModel::FeatureHandle::weak_ref> d_current_topological_line_features;

		/**
		 * The subset of features that are topological boundaries.
		 */
		std::vector<GPlatesModel::FeatureHandle::weak_ref> d_current_topological_boundary_features;

		/**
		 * All input feature collections.
		 *
		 * Note that the full set of features might be a mixture of topological and non-topological.
		 * Some files contain a mixture of both and hence create both topological and non-topological layers.
		 */
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> d_current_feature_collections;

		/**
		 * Used to get reconstruction trees at desired reconstruction times.
		 *
		 * TODO: I'm not sure we really need a reconstruction tree layer ?
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
		 * The cached resolved boundary topologies over a range of reconstruction times.
		 *
		 * This is cached as a performance optimisation for clients that reconstruct geometries using topologies.
		 */
		ResolvedBoundaryTimeSpan d_cached_resolved_boundary_time_span;

		/**
		 * The cached resolved topological boundaries (polygons).
		 */
		ResolvedBoundaries d_cached_resolved_boundaries;

		/**
		 * The cached resolved topological lines (polylines).
		 */
		ResolvedLines d_cached_resolved_lines;

		/**
		 * The cached resolved *boundaries* depend on these topological sections.
		 */
		DependentTopologicalSectionLayers d_resolved_boundary_dependent_topological_sections;

		/**
		 * The cached resolved *lines* depend on these topological sections.
		 */
		DependentTopologicalSectionLayers d_resolved_line_dependent_topological_sections;

		/**
		 * The current reconstruction time as set by the layer system.
		 */
		double d_current_reconstruction_time;

		/**
		 * Used to notify polling observers that we've been updated (either resolved lines or boundaries).
		 */
		mutable GPlatesUtils::SubjectToken d_subject_token;

		/**
		 * Used to notify polling observers that our resolved *lines* have been updated.
		 */
		mutable GPlatesUtils::SubjectToken d_resolved_lines_subject_token;

		/**
		 * Used to prevent 
		 */
		mutable bool d_inside_get_subject_token_method;


		//! Default constructor.
		TopologyGeometryResolverLayerProxy();


		/**
		 * Resets any cached variables forcing them to be recalculated next time they're accessed.
		 */
		void
		reset_cache(
				bool invalidate_resolved_boundaries = true,
				bool invalidate_resolved_lines = true);


		/**
		 * Checks if any input layer proxies have changed.
		 *
		 * If so then reset caches and invalidates subject token.
		 */
		void
		check_input_layer_proxies(
				bool check_resolved_line_topological_sections = true);


		/**
		 * Generates resolved topological boundaries for the specified reconstruction time
		 * if they're not already cached.
		 */
		std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &
		cache_resolved_topological_boundaries(
				const double &reconstruction_time);


		/**
		 * Generates a resolved boundary time span for the specified time range if one is not already cached.
		 */
		TopologyReconstruct::resolved_boundary_time_span_type::non_null_ptr_to_const_type
		cache_resolved_boundary_time_span(
				const TimeSpanUtils::TimeRange &time_range);


		/**
		 * Creates resolved topological boundaries for the specified reconstruction time.
		 */
		ReconstructHandle::type
		create_resolved_topological_boundaries(
				std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries,
				const double &reconstruction_time);


		/**
		 * Creates resolved topological lines for the specified reconstruction time.
		 */
		ReconstructHandle::type
		create_resolved_topological_lines(
				std::vector<ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_lines,
				const std::vector<GPlatesModel::FeatureHandle::weak_ref> &topological_line_features,
				const double &reconstruction_time);
	};
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYGEOMETRYRESOLVERLAYERPROXY_H
