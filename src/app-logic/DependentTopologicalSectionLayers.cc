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

#include "DependentTopologicalSectionLayers.h"

#include "TopologyGeometryResolverLayerProxy.h"
#include "TopologyInternalUtils.h"


GPlatesAppLogic::DependentTopologicalSectionLayers::~DependentTopologicalSectionLayers()
{
	// Defined in ".cc" file because...
	// non_null_ptr destructors require complete type of class they're referring to.
}


void
GPlatesAppLogic::DependentTopologicalSectionLayers::set_topological_section_feature_ids(
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &topological_feature_collections)
{
	d_feature_ids.clear();

	// Set the feature IDs of topological sections referenced by the topologies.
	BOOST_FOREACH(
			const GPlatesModel::FeatureCollectionHandle::weak_ref &topological_feature_collection,
			topological_feature_collections)
	{
		// Find referenced feature IDs for *all* reconstruction times.
		TopologyInternalUtils::find_topological_sections_referenced(
				d_feature_ids,
				topological_feature_collection);
	}

	// Using our existing topological section layers, find those that have any of the above feature IDs.
	set_dependency_topological_section_layers(d_reconstructed_geometry_layers, d_dependency_reconstructed_geometry_layers);
	set_dependency_topological_section_layers(d_resolved_line_layers, d_dependency_resolved_line_layers);
}


bool
GPlatesAppLogic::DependentTopologicalSectionLayers::set_topological_section_layers(
		const std::vector<ReconstructLayerProxy::non_null_ptr_type> &all_layers)
{
	d_reconstructed_geometry_layers = all_layers;
	return set_dependency_topological_section_layers(all_layers, d_dependency_reconstructed_geometry_layers);
}


bool
GPlatesAppLogic::DependentTopologicalSectionLayers::set_topological_section_layers(
		const std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> &all_layers)
{
	d_resolved_line_layers = all_layers;
	return set_dependency_topological_section_layers(all_layers, d_dependency_resolved_line_layers);
}


bool
GPlatesAppLogic::DependentTopologicalSectionLayers::update_topological_section_layer(
		const ReconstructLayerProxy::non_null_ptr_type &layer)
{
	return update_topological_section_layer(layer, d_dependency_reconstructed_geometry_layers);
}


bool
GPlatesAppLogic::DependentTopologicalSectionLayers::update_topological_section_layer(
		const TopologyGeometryResolverLayerProxy::non_null_ptr_type &layer)
{
	return update_topological_section_layer(layer, d_dependency_resolved_line_layers);
}


template <class LayerProxyType>
bool
GPlatesAppLogic::DependentTopologicalSectionLayers::set_dependency_topological_section_layers(
		const std::vector<typename LayerProxyType::non_null_ptr_type> &all_layers,
		std::set<LayerProxyType *> &layers)
{
	std::set<LayerProxyType *> new_layers;

	// Iterate over all the layers and insert those that the topologies depend on.
	for (unsigned int n = 0; n < all_layers.size(); ++n)
	{
		const typename LayerProxyType::non_null_ptr_type &layer = all_layers[n];

		if (topologies_depend_on_layer(layer))
		{
			new_layers.insert(layer.get());
		}
	}

	bool dependent_layers_changed = false;

	// If the sub-set of dependent layers changed in any way then the resolved topologies will need updating.
	if (layers != new_layers)
	{
		layers.swap(new_layers);
		dependent_layers_changed = true;
	}

	return dependent_layers_changed;
}


template <class LayerProxyType>
bool
GPlatesAppLogic::DependentTopologicalSectionLayers::update_topological_section_layer(
		const typename LayerProxyType::non_null_ptr_type &layer,
		std::set<LayerProxyType *> &layers)
{
	if (layers.find(layer.get()) != layers.end())
	{
		if (topologies_depend_on_layer(layer))
		{
			// Layer was a previous dependency and remains one.
			// No change has occurred but the layer itself was updated so the resolved topologies still need updating.
			return true;
		}
		else
		{
			// No longer a dependency, so remove from dependency list.
			layers.erase(layer.get());

			// The resolved topologies still need updating though, since previously layer was a dependency.
			// In other words, a change has occurred.
			return true;
		}
	}
	else
	{
		if (topologies_depend_on_layer(layer))
		{
			// Add new dependency layer.
			layers.insert(layer.get());
			return true;
		}
		else
		{
			// Layer was not a previous dependency and is not one now either.
			// So the resolved topologies don't need updating.
			return false;
		}
	}
}


bool
GPlatesAppLogic::DependentTopologicalSectionLayers::topologies_depend_on_layer(
		const ReconstructLayerProxy::non_null_ptr_type &layer) const
{
	// Return early if there's nothing to depend on yet.
	if (d_feature_ids.empty())
	{
		return false;
	}

	// Get the topological section features.
	//
	// Note that only the *non-topological* features are returned (the topological ones are ignored).
	// This helps avoid a dependency on a non-topological layer when actually we should only depend
	// on the topological layer (connected to same file as non-topological layer if file has a mixture
	// of topological and non-topological features).
	std::vector<GPlatesModel::FeatureHandle::weak_ref> topological_section_features;
	layer->get_current_features(topological_section_features, true/*only_non_topological_features*/);

	return topologies_depend_on_features(topological_section_features);
}


bool
GPlatesAppLogic::DependentTopologicalSectionLayers::topologies_depend_on_layer(
		const TopologyGeometryResolverLayerProxy::non_null_ptr_type &layer) const
{
	// Return early if there's nothing to depend on yet.
	if (d_feature_ids.empty())
	{
		return false;
	}

	// Get the topological section features.
	//
	// Note that only the *topological* features are returned (the non-topological ones are ignored).
	// This helps avoid a dependency on a topological layer when actually we should only depend
	// on the non-topological layer (connected to same file as topological layer if file has a mixture
	// of topological and non-topological features) - since that topological layer will in turn
	// depend on other non-topological layers thus unnecessarily widening our dependency graph.
	std::vector<GPlatesModel::FeatureHandle::weak_ref> topological_section_features;
	layer->get_current_features(topological_section_features, true/*only_topological_features*/);

	return topologies_depend_on_features(topological_section_features);
}


bool
GPlatesAppLogic::DependentTopologicalSectionLayers::topologies_depend_on_features(
		const std::vector<GPlatesModel::FeatureHandle::weak_ref> &features) const
{
	std::vector<GPlatesModel::FeatureHandle::weak_ref>::const_iterator features_iter = features.begin();
	std::vector<GPlatesModel::FeatureHandle::weak_ref>::const_iterator features_end = features.end();
	for ( ; features_iter != features_end; ++features_iter)
	{
		const GPlatesModel::FeatureHandle::weak_ref &feature = *features_iter;

		if (!feature.is_valid())
		{
			continue;
		}

		if (d_feature_ids.find(feature->feature_id()) != d_feature_ids.end())
		{
			return true;
		}
	}

	return false;
}
