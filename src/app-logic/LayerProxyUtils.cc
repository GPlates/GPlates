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
#include <boost/foreach.hpp>

#include "LayerProxyUtils.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructedFeatureGeometryFinder.h"
#include "Reconstruction.h"
#include "ReconstructLayerProxy.h"
#include "ResolvedTopologicalLine.h"
#include "TopologyGeometryResolverLayerProxy.h"


void
GPlatesAppLogic::LayerProxyUtils::get_reconstructed_feature_geometries(
		std::vector<reconstructed_feature_geometry_non_null_ptr_type> &reconstructed_feature_geometries,
		std::vector<ReconstructHandle::type> &reconstruct_handles,
		const Reconstruction &reconstruction)
{
	// Get the reconstruct layer outputs.
	std::vector<ReconstructLayerProxy::non_null_ptr_type> reconstruct_layer_proxies;
	reconstruction.get_active_layer_outputs<ReconstructLayerProxy>(reconstruct_layer_proxies);

	BOOST_FOREACH(
			const ReconstructLayerProxy::non_null_ptr_type &reconstruct_layer_proxy,
			reconstruct_layer_proxies)
	{
		// Get the reconstructed feature geometries from the current layer for the
		// current reconstruction time and anchor plate id.
		const ReconstructHandle::type reconstruct_handle =
				reconstruct_layer_proxy->get_reconstructed_feature_geometries(reconstructed_feature_geometries);

		// Add the reconstruct handle to the list.
		reconstruct_handles.push_back(reconstruct_handle);
	}
}


void
GPlatesAppLogic::LayerProxyUtils::get_resolved_topological_lines(
		std::vector<resolved_topological_line_non_null_ptr_type> &resolved_topological_lines,
		std::vector<ReconstructHandle::type> &reconstruct_handles,
		const Reconstruction &reconstruction)
{
	// Get the resolved geometry layer outputs.
	std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> topology_geometry_resolver_layer_proxies;
	reconstruction.get_active_layer_outputs<TopologyGeometryResolverLayerProxy>(topology_geometry_resolver_layer_proxies);

	BOOST_FOREACH(
			const TopologyGeometryResolverLayerProxy::non_null_ptr_type &topology_geometry_resolver_layer_proxy,
			topology_geometry_resolver_layer_proxies)
	{
		// Get the resolved topological lines from the current layer.
		const ReconstructHandle::type reconstruct_handle =
				topology_geometry_resolver_layer_proxy->get_resolved_topological_lines(resolved_topological_lines);

		// Add the reconstruct handle to the list.
		reconstruct_handles.push_back(reconstruct_handle);
	}
}


void
GPlatesAppLogic::LayerProxyUtils::find_reconstructed_feature_geometries_of_feature(
		std::vector<reconstructed_feature_geometry_non_null_ptr_type> &reconstructed_feature_geometries,
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		const Reconstruction &reconstruction)
{
	// Get the RFGs (and generate if not already) for all active ReconstructLayer's.
	// This is needed because we are about to search the feature for its RFGs and
	// if we don't generate the RFGs (and no one else does either) then they might not be found.
	//
	// The RFGs - we'll keep them active until we've finished searching for RFGs below.
	std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> rfgs_in_reconstruction;
	std::vector<ReconstructHandle::type> reconstruct_handles;
	get_reconstructed_feature_geometries(rfgs_in_reconstruction, reconstruct_handles, reconstruction);

	// Iterate through all RFGs observing 'feature_ref' and that were reconstructed just now (above).
	ReconstructedFeatureGeometryFinder rfg_finder(
			boost::none/*reconstruction_tree_to_match*/,
			reconstruct_handles);
	rfg_finder.find_rfgs_of_feature(feature_ref);

	// Add the found RFGs to the caller's sequence.
	reconstructed_feature_geometries.insert(
			reconstructed_feature_geometries.end(),
			rfg_finder.found_rfgs_begin(),
			rfg_finder.found_rfgs_end());
}
