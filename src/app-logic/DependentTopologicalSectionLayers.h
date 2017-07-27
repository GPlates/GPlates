/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_DEPENDENTTOPOLOGICALSECTIONLAYERS_H
#define GPLATES_APP_LOGIC_DEPENDENTTOPOLOGICALSECTIONLAYERS_H

#include <set>
#include <vector>
#include <boost/optional.hpp>

#include "ReconstructLayerProxy.h"
#include "TopologyGeometryType.h"

#include "global/PointerTraits.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/FeatureId.h"


namespace GPlatesAppLogic
{
	// Forward declaration.
	class TopologyGeometryResolverLayerProxy;

	/**
	 * Keeps track of which layers actually contribute topological sections to resolved topologies in a resolved topology layer.
	 *
	 * This is an optimisation to avoid flushing resolved topology caches when unrelated topological section layers are updated.
	 */
	class DependentTopologicalSectionLayers
	{
	public:

		~DependentTopologicalSectionLayers();


		/**
		 * Sets the topological section feature IDs referenced by the topological features for *all* times.
		 *
		 * If @a topology_geometry_type is specified then only features with matching topology geometries are considered.
		 *
		 * Also sets the *dependency* topological section layers that the topological features depend on.
		 */
		void
		set_topological_section_feature_ids(
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &topological_feature_collections,
				boost::optional<TopologyGeometry::Type> topology_geometry_type = boost::none);


		/**
		 * Set the *reconstructed geometry* topological section layers.
		 *
		 * Returns true if the *dependent* subset of the specified topological section layers are different to the current ones.
		 */
		bool
		set_topological_section_layers(
				const std::vector<ReconstructLayerProxy::non_null_ptr_type> &all_layers);

		/**
		 * Set the *resolved line* topological section layers.
		 *
		 * Returns true if the *dependent* subset of the specified topological section layers are different to the current ones.
		 */
		bool
		set_topological_section_layers(
				const std::vector<GPlatesGlobal::PointerTraits<TopologyGeometryResolverLayerProxy>::non_null_ptr_type> &all_layers);


		/**
		 * Call when the specified *reconstructed geometry* topological layer has changed (been updated).
		 *
		 * Returns true if the dependent resolved topologies should be updated due to the updated topological section layer.
		 */
		bool
		update_topological_section_layer(
				const ReconstructLayerProxy::non_null_ptr_type &layer);

		/**
		 * Call when the specified *resolved line* topological layer has changed (been updated).
		 *
		 * Returns true if the dependent resolved topologies should be updated due to the updated topological section layer.
		 */
		bool
		update_topological_section_layer(
				const GPlatesGlobal::PointerTraits<TopologyGeometryResolverLayerProxy>::non_null_ptr_type &layer);


		/**
		 * Get the *reconstructed geometry* topological layers that the topological features depend on.
		 */
		void
		get_dependent_topological_section_layers(
				std::vector<ReconstructLayerProxy::non_null_ptr_type> &dependent_layers);

		/**
		 * Get the *reconstructed geometry* topological layers that the topological features depend on.
		 */
		void
		get_dependent_topological_section_layers(
				std::vector<GPlatesGlobal::PointerTraits<TopologyGeometryResolverLayerProxy>::non_null_ptr_type> &dependent_layers);

	private:

		// All topological section layers (even ones that don't contribute to resolved topologies).
		std::vector<ReconstructLayerProxy::non_null_ptr_type> d_reconstructed_geometry_layers;
		std::vector<GPlatesGlobal::PointerTraits<TopologyGeometryResolverLayerProxy>::non_null_ptr_type> d_resolved_line_layers;

		// Unique list of dependency topological section layers that contribute to resolved topologies.
		std::set<ReconstructLayerProxy *> d_dependency_reconstructed_geometry_layers;
		std::set<TopologyGeometryResolverLayerProxy *> d_dependency_resolved_line_layers;

		//! Unique list of topological section feature IDs that contribute to resolved topologies.
		std::set<GPlatesModel::FeatureId> d_feature_ids;


		template <class LayerProxyType>
		bool
		set_dependency_topological_section_layers(
				const std::vector<typename LayerProxyType::non_null_ptr_type> &all_layers,
				std::set<LayerProxyType *> &layers);

		template <class LayerProxyType>
		bool
		update_topological_section_layer(
				const typename LayerProxyType::non_null_ptr_type &layer,
				std::set<LayerProxyType *> &layers);

		template <class LayerProxyType>
		void
		get_dependent_topological_section_layers(
				std::vector<typename LayerProxyType::non_null_ptr_type> &dependent_layers,
				std::set<LayerProxyType *> &layers);

		/**
		 * Checks if any topology depends on any of the specified topological section layer.
		 *
		 * The specified features come from a dependency layer.
		 */
		bool
		topologies_depend_on_layer(
				const ReconstructLayerProxy::non_null_ptr_type &layer) const;
		bool
		topologies_depend_on_layer(
				const GPlatesGlobal::PointerTraits<TopologyGeometryResolverLayerProxy>::non_null_ptr_type &layer) const;

		//! Checks if any topology depends on any of the specified topological section features.
		bool
		topologies_depend_on_features(
				const std::vector<GPlatesModel::FeatureHandle::weak_ref> &features) const;
	};
}

#endif // GPLATES_APP_LOGIC_DEPENDENTTOPOLOGICALSECTIONLAYERS_H
