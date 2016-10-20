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

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include "TopologyGeometryResolverLayerProxy.h"

#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalLine.h"
#include "TopologyUtils.h"


GPlatesAppLogic::TopologyGeometryResolverLayerProxy::TopologyGeometryResolverLayerProxy() :
	// Start off with a reconstruction layer proxy that does identity rotations.
	d_current_reconstruction_layer_proxy(ReconstructionLayerProxy::create()),
	d_current_reconstruction_time(0)
{
	// Defined in ".cc" file because...
	// non_null_ptr destructors require complete type of class they're referring to.
	// Compiler will call destructors of already-constructed members if constructor throws exception.
}


GPlatesAppLogic::TopologyGeometryResolverLayerProxy::~TopologyGeometryResolverLayerProxy()
{
	// Defined in ".cc" file because...
	// non_null_ptr destructors require complete type of class they're referring to.
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::get_resolved_topological_geometries(
		std::vector<ResolvedTopologicalGeometry::non_null_ptr_type> &resolved_topological_geometries,
		const double &reconstruction_time,
		boost::optional<std::vector<ReconstructHandle::type> &> reconstruct_handles)
{
	std::vector<ResolvedTopologicalLine::non_null_ptr_type> resolved_topological_lines;
	const ReconstructHandle::type resolved_lines_reconstruct_handle =
			get_resolved_topological_lines(resolved_topological_lines, reconstruction_time);
	BOOST_FOREACH(
			const ResolvedTopologicalLine::non_null_ptr_type &resolved_topological_line,
			resolved_topological_lines)
	{
		resolved_topological_geometries.push_back(resolved_topological_line);
	}

	std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> resolved_topological_boundaries;
	const ReconstructHandle::type resolved_boundaries_reconstruct_handle =
			get_resolved_topological_boundaries(resolved_topological_boundaries, reconstruction_time);
	BOOST_FOREACH(
			const ResolvedTopologicalBoundary::non_null_ptr_type &resolved_topological_boundary,
			resolved_topological_boundaries)
	{
		resolved_topological_geometries.push_back(resolved_topological_boundary);
	}

	if (reconstruct_handles)
	{
		reconstruct_handles->push_back(resolved_lines_reconstruct_handle);
		reconstruct_handles->push_back(resolved_boundaries_reconstruct_handle);
	}
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::get_resolved_topological_boundaries(
		std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries,
		const double &reconstruction_time)
{
	// If we have no topological features or there are no topological section layers then we
	// can't get any topological sections and we can't resolve any topological close plate polygons.
	if (d_current_topological_geometry_feature_collections.empty() ||
		(d_current_reconstructed_geometry_topological_sections_layer_proxies.empty() &&
			d_current_resolved_line_topological_sections_layer_proxies.empty()))
	{
		// There will be no reconstructed/resolved geometries for this handle.
		return ReconstructHandle::get_next_reconstruct_handle();
	}

	// See if the reconstruction time has changed.
	if (d_cached_resolved_boundaries.cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time))
	{
		// The resolved boundaries are now invalid.
		d_cached_resolved_boundaries.invalidate();

		// Note that observers don't need to be updated when the time changes - if they
		// have resolved boundaries for a different time they don't need
		// to be updated just because some other client requested a different time.
		d_cached_resolved_boundaries.cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
	}

	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	if (!d_cached_resolved_boundaries.cached_resolved_topological_boundaries)
	{
		// Create empty vector of resolved topological boundaries.
		d_cached_resolved_boundaries.cached_resolved_topological_boundaries =
				std::vector<ResolvedTopologicalBoundary::non_null_ptr_type>();

		std::vector<ReconstructHandle::type> topological_sections_reconstruct_handles;

		// Topological sections that are reconstructed static features...
		// We're ensuring that all potential (reconstructed geometry) topological sections are reconstructed
		// before we resolve topological boundaries (which reference them indirectly via feature-id).
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_geometry_topological_sections;
		BOOST_FOREACH(
				LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &reconstructed_geometry_topological_sections_layer_proxy,
				d_current_reconstructed_geometry_topological_sections_layer_proxies)
		{
			// Get the potential topological section RFGs.
			const ReconstructHandle::type reconstruct_handle =
					reconstructed_geometry_topological_sections_layer_proxy.get_input_layer_proxy()
							->get_reconstructed_feature_geometries(
									reconstructed_geometry_topological_sections,
									reconstruction_time);

			// Add the reconstruct handle to our list.
			topological_sections_reconstruct_handles.push_back(reconstruct_handle);
		}

		// Topological sections that are resolved topological lines...
		// We're ensuring that all potential (resolved line) topological sections are resolved before
		// we resolve topological boundaries (which reference them indirectly via feature-id).
		std::vector<ResolvedTopologicalLine::non_null_ptr_type> resolved_line_topological_sections;
		BOOST_FOREACH(
				LayerProxyUtils::InputLayerProxy<TopologyGeometryResolverLayerProxy> &resolved_line_topological_sections_layer_proxy,
				d_current_resolved_line_topological_sections_layer_proxies)
		{
			// Get the potential topological section RTGs.
			const ReconstructHandle::type reconstruct_handle =
					resolved_line_topological_sections_layer_proxy.get_input_layer_proxy()
							->get_resolved_topological_lines(
									resolved_line_topological_sections,
									reconstruction_time);

			// Add the reconstruct handle to our list.
			topological_sections_reconstruct_handles.push_back(reconstruct_handle);
		}

		// Resolve our closed plate polygon features into our sequence of resolved topological boundaries.
		d_cached_resolved_boundaries.cached_reconstruct_handle =
				TopologyUtils::resolve_topological_boundaries(
						d_cached_resolved_boundaries.cached_resolved_topological_boundaries.get(),
						d_current_topological_geometry_feature_collections,
						d_current_reconstruction_layer_proxy.get_input_layer_proxy()->get_reconstruction_tree_creator(),
						reconstruction_time,
						topological_sections_reconstruct_handles);
	}

	// Append our cached resolved topological boundaries to the caller's sequence.
	resolved_topological_boundaries.insert(
			resolved_topological_boundaries.end(),
			d_cached_resolved_boundaries.cached_resolved_topological_boundaries->begin(),
			d_cached_resolved_boundaries.cached_resolved_topological_boundaries->end());

	return d_cached_resolved_boundaries.cached_reconstruct_handle.get();
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::get_resolved_topological_lines(
		std::vector<ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_lines,
		const double &reconstruction_time)
{
	// If we have no topological features or there are no *reconstructed geometry* topological section
	// layers then we can't get any topological sections and we can't resolve any topological lines.
	// Note that we don't check the *resolved line* topological section layers because resolved lines
	// cannot be used as topological sections for other resolved lines.
	if (d_current_topological_geometry_feature_collections.empty() ||
		d_current_reconstructed_geometry_topological_sections_layer_proxies.empty())
	{
		// There will be no reconstructed/resolved geometries for this handle.
		return ReconstructHandle::get_next_reconstruct_handle();
	}

	// See if the reconstruction time has changed.
	if (d_cached_resolved_lines.cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time))
	{
		// The resolved lines are now invalid.
		d_cached_resolved_lines.invalidate();

		// Note that observers don't need to be updated when the time changes - if they
		// have resolved boundaries for a different time they don't need
		// to be updated just because some other client requested a different time.
		d_cached_resolved_lines.cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
	}

	// See if any input layer proxies have changed.
	//
	// Note that we don't check the resolved line topological section layer inputs because
	// resolved lines cannot reference other resolved lines (like resolved boundaries can).
	// This also avoids an infinite recursion.
	check_input_layer_proxies(false/*check_resolved_line_topological_sections*/);

	if (!d_cached_resolved_lines.cached_resolved_topological_lines)
	{
		// Create empty vector of resolved topological lines.
		d_cached_resolved_lines.cached_resolved_topological_lines =
				std::vector<ResolvedTopologicalLine::non_null_ptr_type>();

		std::vector<ReconstructHandle::type> topological_sections_reconstruct_handles;

		// Topological sections that are reconstructed static features...
		// We're ensuring that all potential (reconstructed geometry) topological sections are reconstructed
		// before we resolve topological lines (which reference them indirectly via feature-id).
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_geometry_topological_sections;
		BOOST_FOREACH(
				LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &reconstructed_geometry_topological_sections_layer_proxy,
				d_current_reconstructed_geometry_topological_sections_layer_proxies)
		{
			// Get the potential topological section RFGs.
			const ReconstructHandle::type reconstruct_handle =
					reconstructed_geometry_topological_sections_layer_proxy.get_input_layer_proxy()
							->get_reconstructed_feature_geometries(
									reconstructed_geometry_topological_sections,
									reconstruction_time);

			// Add the reconstruct handle to our list.
			topological_sections_reconstruct_handles.push_back(reconstruct_handle);
		}

		// Note that we don't query *resolved line* topological section layers because resolved lines
		// cannot be used as topological sections for other resolved lines.
		// This is where topological lines differ from topological boundaries.
		// Topological boundaries can use resolved lines as topological sections.

		// Resolve our topological line features into our sequence of resolved topological lines.
		d_cached_resolved_lines.cached_reconstruct_handle =
				TopologyUtils::resolve_topological_lines(
						d_cached_resolved_lines.cached_resolved_topological_lines.get(),
						d_current_topological_geometry_feature_collections,
						d_current_reconstruction_layer_proxy.get_input_layer_proxy()->get_reconstruction_tree_creator(),
						reconstruction_time,
						topological_sections_reconstruct_handles);
	}

	// Append our cached resolved topological lines to the caller's sequence.
	resolved_topological_lines.insert(
			resolved_topological_lines.end(),
			d_cached_resolved_lines.cached_resolved_topological_lines->begin(),
			d_cached_resolved_lines.cached_resolved_topological_lines->end());

	return d_cached_resolved_lines.cached_reconstruct_handle.get();
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::get_current_features(
		std::vector<GPlatesModel::FeatureHandle::weak_ref> &features,
		bool only_topological_features) const
{
	// Iterate over the current feature collections.
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>::const_iterator feature_collections_iter =
			d_current_topological_geometry_feature_collections.begin();
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>::const_iterator feature_collections_end =
			d_current_topological_geometry_feature_collections.end();
	for ( ; feature_collections_iter != feature_collections_end; ++feature_collections_iter)
	{
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection = *feature_collections_iter;
		if (feature_collection.is_valid())
		{
			GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection->begin();
			GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection->end();
			for ( ; features_iter != features_end; ++features_iter)
			{
				const GPlatesModel::FeatureHandle::weak_ref feature = (*features_iter)->reference();

				if (!feature.is_valid())
				{
					continue;
				}

				if (only_topological_features &&
					!TopologyUtils::is_topological_geometry_feature(feature))
				{
					continue;
				}

				features.push_back(feature);
			}
		}
	}
}


GPlatesAppLogic::ReconstructionLayerProxy::non_null_ptr_type
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::get_current_reconstruction_layer_proxy()
{
	return d_current_reconstruction_layer_proxy.get_input_layer_proxy();
}


const GPlatesUtils::SubjectToken &
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::get_subject_token()
{
	// We've checked to see if any inputs have changed except the reconstruction and
	// reconstruct and resolved-line layer proxy inputs.
	// This is because we get notified of all changes to input except input layer proxies which
	// we have to poll to see if they changed since we last accessed them - so we do that now.
	check_input_layer_proxies();

	return d_subject_token;
}


const GPlatesUtils::SubjectToken &
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::get_resolved_lines_subject_token()
{
	// We've checked to see if any inputs have changed except the reconstruction and
	// reconstruct layer proxy inputs.
	// This is because we get notified of all changes to input except input layer proxies which
	// we have to poll to see if they changed since we last accessed them - so we do that now.
	//
	// Note that we don't check the resolved line topological section layer inputs because
	// resolved lines cannot reference other resolved lines (like resolved boundaries can).
	// This also avoids an infinite recursion.
	check_input_layer_proxies(false/*check_resolved_line_topological_sections*/);

	return d_resolved_lines_subject_token;
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::set_current_reconstruction_time(
		const double &reconstruction_time)
{
	d_current_reconstruction_time = reconstruction_time;

	// Note that we don't reset our caches because we only do that when the client
	// requests a reconstruction time that differs from the cached reconstruction time.
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::set_current_reconstruction_layer_proxy(
		const ReconstructionLayerProxy::non_null_ptr_type &reconstruction_layer_proxy)
{
	d_current_reconstruction_layer_proxy.set_input_layer_proxy(reconstruction_layer_proxy);

	// The resolved topological geometries (boundaries and lines) are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate(); // Lines or boundaries are invalid.
	d_resolved_lines_subject_token.invalidate(); // Lines are invalid.
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::set_current_topological_sections_layer_proxies(
		const std::vector<ReconstructLayerProxy::non_null_ptr_type> &reconstructed_geometry_topological_sections_layer_proxies,
		const std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> &resolved_line_topological_sections_layer_proxies)
{
	if (d_current_reconstructed_geometry_topological_sections_layer_proxies.set_input_layer_proxies(
			reconstructed_geometry_topological_sections_layer_proxies))
	{
		// The topological section layers are different than last time.
		// If the *dependent* layers are different then cache invalidation is necessary.
		// Dependent means the currently cached resolved geometries use topological sections from the specified layers.
		if (d_dependent_topological_sections.set_topological_section_layers(
				reconstructed_geometry_topological_sections_layer_proxies))
		{
			// All resolved topological geometries (boundaries and lines) are now invalid.
			reset_cache();

			// Polling observers need to update themselves with respect to us.
			d_subject_token.invalidate(); // Lines or boundaries are invalid.
			d_resolved_lines_subject_token.invalidate(); // Lines are invalid.
		}
	}

	// Note that we check using 'get_resolved_lines_subject_token()' instead of 'get_subject_token()'
	// because the latter checks for updates to both resolved *lines and boundaries* and we only need
	// to check resolved *lines*. This is because resolved lines cannot reference other resolved lines
	// (like resolved boundaries can).
	// This also avoids an infinite recursion during the checking of input proxies.
	if (d_current_resolved_line_topological_sections_layer_proxies.set_input_layer_proxies(
			resolved_line_topological_sections_layer_proxies,
			&TopologyGeometryResolverLayerProxy::get_resolved_lines_subject_token))
	{
		// The topological section layers are different than last time.
		// If the *dependent* layers are different then cache invalidation is necessary.
		// Dependent means the currently cached resolved networks (and time spans) use topological sections from the specified layers.
		if (d_dependent_topological_sections.set_topological_section_layers(
				resolved_line_topological_sections_layer_proxies))
		{
			// All resolved topological boundaries are now invalid.
			//
			// The resolved lines are not invalid because they can't depend on other resolved lines
			// like the boundaries can.
			reset_cache(false/*invalidate_resolved_lines*/);

			// Polling observers need to update themselves with respect to us.
			d_subject_token.invalidate(); // Boundaries are invalid.
			// Note that we don't invalidate 'd_resolved_lines_subject_token' since the resolved lines
			// can't depend on other resolved lines like the boundaries can.
		}
	}
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::add_topological_geometry_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	d_current_topological_geometry_feature_collections.push_back(feature_collection);

	// Set the feature IDs of topological sections referenced by our resolved geometries for *all* times.
	d_dependent_topological_sections.set_topological_section_feature_ids(
			d_current_topological_geometry_feature_collections);

	// The resolved topological geometries are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate(); // Lines or boundaries are invalid.
	d_resolved_lines_subject_token.invalidate(); // Lines are invalid.
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::remove_topological_geometry_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// Erase the feature collection from our list.
	d_current_topological_geometry_feature_collections.erase(
			std::find(
					d_current_topological_geometry_feature_collections.begin(),
					d_current_topological_geometry_feature_collections.end(),
					feature_collection));

	// Set the feature IDs of topological sections referenced by our resolved geometries for *all* times.
	d_dependent_topological_sections.set_topological_section_feature_ids(
			d_current_topological_geometry_feature_collections);

	// The resolved topological geometries are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate(); // Lines or boundaries are invalid.
	d_resolved_lines_subject_token.invalidate(); // Lines are invalid.
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::modified_topological_geometry_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// Set the feature IDs of topological sections referenced by our resolved geometries for *all* times.
	d_dependent_topological_sections.set_topological_section_feature_ids(
			d_current_topological_geometry_feature_collections);

	// The resolved topological geometries are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate(); // Lines or boundaries are invalid.
	d_resolved_lines_subject_token.invalidate(); // Lines are invalid.
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::reset_cache(
		bool invalidate_resolved_lines)
{
	// Clear any cached resolved topological boundaries.
	d_cached_resolved_boundaries.invalidate();

	if (invalidate_resolved_lines)
	{
		// Clear any cached resolved topological lines.
		d_cached_resolved_lines.invalidate();
	}
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::check_input_layer_proxies(
		bool check_resolved_line_topological_sections)
{
	// See if the reconstruction layer proxy has changed.
	if (!d_current_reconstruction_layer_proxy.is_up_to_date())
	{
		// The resolved geometries are now invalid.
		reset_cache();

		// We're now up-to-date with respect to the input layer proxy.
		d_current_reconstruction_layer_proxy.set_up_to_date();

		// Polling observers need to update themselves with respect to us.
		d_subject_token.invalidate();
		d_resolved_lines_subject_token.invalidate(); // Lines are invalid.
	}

	// See if any reconstructed geometry topological section layer proxies have changed.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &rfg_topological_sections_layer_proxy,
			d_current_reconstructed_geometry_topological_sections_layer_proxies)
	{
		if (rfg_topological_sections_layer_proxy.is_up_to_date())
		{
			continue;
		}

		// If any cached resolved geometries depend on these topological sections then we need to invalidate our cache.
		//
		// Typically our dependency layers include all reconstruct/resolved-geometry layers
		// due to the usual global search for topological section features. However this means
		// layers that don't contribute topological sections will trigger unnecessary cache flushes
		// which is especially noticeable in the case of rebuilding network time spans that in turn
		// depend on our resolved topologies.
		// To avoid this we check if any topological sections from a layer can actually contribute.
		if (d_dependent_topological_sections.update_topological_section_layer(
				rfg_topological_sections_layer_proxy.get_input_layer_proxy()))
		{
			// The resolved geometries are now invalid.
			reset_cache();

			// Polling observers need to update themselves with respect to us.
			d_subject_token.invalidate();
			d_resolved_lines_subject_token.invalidate(); // Lines are invalid.
		}

		// We're now up-to-date with respect to the input layer proxy.
		rfg_topological_sections_layer_proxy.set_up_to_date();
	}

	// See if any resolved line topological section layer proxies have changed.
	//
	// The *resolved line* topological section input proxies can only affect the resolved *boundaries*.
	// So only need to check when interested in resolved *boundaries*.
	if (check_resolved_line_topological_sections)
	{
		BOOST_FOREACH(
				LayerProxyUtils::InputLayerProxy<TopologyGeometryResolverLayerProxy> &rtl_topological_sections_layer_proxy,
				d_current_resolved_line_topological_sections_layer_proxies)
		{
			// NOTE: One of these layer proxies is actually 'this' layer proxy since
			// topological boundaries can reference topological lines from the same layer.
			// There's no need to check 'this' layer proxy.
			if (rtl_topological_sections_layer_proxy.get_input_layer_proxy().get() == this)
			{
				continue;
			}

			if (rtl_topological_sections_layer_proxy.is_up_to_date())
			{
				continue;
			}

			// If any cached resolved boundaries depend on these topological sections then we need to invalidate our cache.
			//
			// Typically our dependency layers include all reconstruct/resolved-geometry layers
			// due to the usual global search for topological section features. However this means
			// layers that don't contribute topological sections will trigger unnecessary cache flushes
			// which is especially noticeable in the case of rebuilding network time spans that in turn
			// depend on our resolved topologies.
			// To avoid this we check if any topological sections from a layer can actually contribute.
			if (d_dependent_topological_sections.update_topological_section_layer(
					rtl_topological_sections_layer_proxy.get_input_layer_proxy()))
			{
				// The resolved geometries are now invalid.
				reset_cache();

				// Polling observers need to update themselves with respect to us.
				d_subject_token.invalidate();
				// Note that we don't invalidate 'd_resolved_lines_subject_token' since the resolved lines
				// can't depend on other resolved lines like the boundaries can.
			}

			// We're now up-to-date with respect to the input layer proxy.
			rtl_topological_sections_layer_proxy.set_up_to_date();
		}
	}
}
