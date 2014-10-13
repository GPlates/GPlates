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

#include <vector>
#include <boost/optional.hpp>

#include "AppLogicFwd.h"
#include "LayerProxy.h"
#include "LayerProxyUtils.h"
#include "ReconstructHandle.h"
#include "ReconstructionLayerProxy.h"
#include "ReconstructLayerProxy.h"
#include "ResolvedTopologicalGeometry.h"

#include "utils/SubjectObserverToken.h"


namespace GPlatesAppLogic
{
	/**
	 * A layer proxy that resolves topological geometries (boundaries and lines) from feature collection(s)
	 * containing topological bounary and line features.
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
				std::vector<resolved_topological_geometry_non_null_ptr_type> &resolved_topological_geometries,
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
				std::vector<resolved_topological_geometry_non_null_ptr_type> &resolved_topological_geometries,
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
				std::vector<resolved_topological_geometry_non_null_ptr_type> &resolved_topological_boundaries)
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
				std::vector<resolved_topological_geometry_non_null_ptr_type> &resolved_topological_boundaries,
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
				std::vector<resolved_topological_geometry_non_null_ptr_type> &resolved_topological_lines)
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
				std::vector<resolved_topological_geometry_non_null_ptr_type> &resolved_topological_lines,
				const double &reconstruction_time);


		/**
		 * Returns the current reconstruction layer proxy used for reconstructions.
		 */
		ReconstructionLayerProxy::non_null_ptr_type
		get_reconstruction_layer_proxy();


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
		 * Sets the current reconstruct layer proxies used to reconstruct the topological boundary sections.
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
			boost::optional< std::vector<resolved_topological_geometry_non_null_ptr_type> >
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
			boost::optional< std::vector<resolved_topological_geometry_non_null_ptr_type> >
					cached_resolved_topological_lines;

			/**
			 * Cached reconstruction time.
			 */
			boost::optional<GPlatesMaths::real_t> cached_reconstruction_time;
		};


		/**
		 * The input feature collections to reconstruct.
		 */
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> d_current_topological_geometry_feature_collections;

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
		 * The cached resolved topological boundaries (polygons).
		 */
		ResolvedBoundaries d_cached_resolved_boundaries;

		/**
		 * The cached resolved topological lines (polylines).
		 */
		ResolvedLines d_cached_resolved_lines;

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
				bool invalidate_resolved_lines = true);


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
		check_input_layer_proxies(
				bool check_resolved_line_topological_sections = true);
	};
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYGEOMETRYRESOLVERLAYERPROXY_H
