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
#include "TopologyInternalUtils.h"
#include "TopologyUtils.h"

#include "maths/MathsUtils.h"

#include "model/FeatureHandle.h"


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
		cache_resolved_topological_boundaries(reconstruction_time);
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

		// Resolve our topological line features into our sequence of resolved topological lines.
		d_cached_resolved_lines.cached_reconstruct_handle =
				create_resolved_topological_lines(
						d_cached_resolved_lines.cached_resolved_topological_lines.get(),
						d_current_topological_geometry_feature_collections,
						reconstruction_time);
	}

	// Append our cached resolved topological lines to the caller's sequence.
	resolved_topological_lines.insert(
			resolved_topological_lines.end(),
			d_cached_resolved_lines.cached_resolved_topological_lines->begin(),
			d_cached_resolved_lines.cached_resolved_topological_lines->end());

	return d_cached_resolved_lines.cached_reconstruct_handle.get();
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::get_resolved_topological_sections(
		std::vector<ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_sections,
		const std::set<GPlatesModel::FeatureId> &topological_sections_referenced,
		const double &reconstruction_time)
{
	// See if there are any cached resolved lines associated with the reconstruction time.
	// We don't want to want to re-generate the cache - we only want to re-use the cache if it's there.
	if (d_cached_resolved_lines.cached_reconstruction_time == GPlatesMaths::real_t(reconstruction_time))
	{
		// See if any input layer proxies have changed.
		//
		// Note that we don't check the resolved line topological section layer inputs because
		// resolved lines cannot reference other resolved lines (like resolved boundaries can).
		// This also avoids an infinite recursion.
		check_input_layer_proxies(false/*check_resolved_line_topological_sections*/);

		// If we have cached lines then just return them.
		if (d_cached_resolved_lines.cached_resolved_topological_lines)
		{
			// Append, to the caller's sequence, those cached lines that match the topological section feature IDs.
			const std::vector<ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_lines =
					d_cached_resolved_lines.cached_resolved_topological_lines.get();
			BOOST_FOREACH(const ResolvedTopologicalLine::non_null_ptr_type &resolved_topological_line, resolved_topological_lines)
			{
				const GPlatesModel::FeatureHandle::weak_ref feature_ref = resolved_topological_line->get_feature_ref();
				if (feature_ref.is_valid() &&
					topological_sections_referenced.find(feature_ref->feature_id()) != topological_sections_referenced.end())
				{
					resolved_topological_sections.push_back(resolved_topological_line);
				}
			}

			return d_cached_resolved_lines.cached_reconstruct_handle.get();
		}
	}

	// Gather only those features (into a temporary feature collection) that are referenced as topological sections.
	GPlatesModel::FeatureCollectionHandle::non_null_ptr_type topological_section_features_referenced =
			GPlatesModel::FeatureCollectionHandle::create();
	BOOST_FOREACH(
			const GPlatesModel::FeatureCollectionHandle::weak_ref &topological_geometry_feature_collection,
			d_current_topological_geometry_feature_collections)
	{
		if (topological_geometry_feature_collection.is_valid())
		{
			GPlatesModel::FeatureCollectionHandle::iterator feature_iter = topological_geometry_feature_collection->begin();
			GPlatesModel::FeatureCollectionHandle::iterator feature_end = topological_geometry_feature_collection->end();
			for ( ; feature_iter != feature_end; ++feature_iter)
			{
				const GPlatesModel::FeatureHandle::non_null_ptr_type feature = *feature_iter;
				if (topological_sections_referenced.find(feature->feature_id()) != topological_sections_referenced.end())
				{
					topological_section_features_referenced->add(feature);
				}
			}
		}
	}

	if (topological_section_features_referenced->size() == 0)
	{
		// There will be no resolved topological sections for this handle.
		return ReconstructHandle::get_next_reconstruct_handle();
	}

	// Generate resolved lines only for the requested topological sections.
	// Note that we don't cache these results because we'd then have to keep track of which
	// feature IDs we've cached for (we could do that though, but currently it's not really necessary).
	return create_resolved_topological_lines(
			resolved_topological_sections,
			std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>(1, topological_section_features_referenced->reference()),
			reconstruction_time);
}


GPlatesAppLogic::TopologyReconstruct::resolved_boundary_time_span_type::non_null_ptr_to_const_type
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::get_resolved_boundary_time_span(
		const TimeSpanUtils::TimeRange &time_range)
{
	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	// If the resolved boundary time span did not get invalidated (due to updated inputs)
	// then see if the time range has changed.
	if (d_cached_resolved_boundary_time_span.cached_resolved_boundary_time_span)
	{
		const TimeSpanUtils::TimeRange &cached_time_range =
				d_cached_resolved_boundary_time_span.cached_resolved_boundary_time_span.get()->get_time_range();

		if (!GPlatesMaths::are_geo_times_approximately_equal(
				time_range.get_begin_time(),
				cached_time_range.get_begin_time()) ||
			!GPlatesMaths::are_geo_times_approximately_equal(
				time_range.get_end_time(),
				cached_time_range.get_end_time()) ||
			!GPlatesMaths::are_geo_times_approximately_equal(
				time_range.get_time_increment(),
				cached_time_range.get_time_increment()))
		{
			// The resolved boundary time span has a different time range.
			// Instead of invalidating the current resolved boundary time span we will attempt
			// to build a new one from the existing one since they may have time slots in common.
			// Note that we've already checked our input proxies so we know that the current
			// resolved boundary time span still contains valid resolved boundaries.
			cache_resolved_boundary_time_span(time_range);

			return d_cached_resolved_boundary_time_span.cached_resolved_boundary_time_span.get();
		}
	}

	if (!d_cached_resolved_boundary_time_span.cached_resolved_boundary_time_span)
	{
		cache_resolved_boundary_time_span(time_range);
	}

	return d_cached_resolved_boundary_time_span.cached_resolved_boundary_time_span.get();
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
	// Filter out layers that use topologies to reconstruct. These layers cannot supply
	// topological sections because they use topology layers thus creating a cyclic dependency.
	std::vector<ReconstructLayerProxy::non_null_ptr_type> valid_reconstructed_geometry_topological_sections_layer_proxies;
	BOOST_FOREACH(
			const ReconstructLayerProxy::non_null_ptr_type &reconstructed_geometry_topological_sections_layer_proxy,
			reconstructed_geometry_topological_sections_layer_proxies)
	{
		if (!reconstructed_geometry_topological_sections_layer_proxy->using_topologies_to_reconstruct())
		{
			valid_reconstructed_geometry_topological_sections_layer_proxies.push_back(
					reconstructed_geometry_topological_sections_layer_proxy);
		}
	}

	if (d_current_reconstructed_geometry_topological_sections_layer_proxies.set_input_layer_proxies(
			valid_reconstructed_geometry_topological_sections_layer_proxies))
	{
		// The topological section layers are different than last time.
		// If the *dependent* layers are different then cache invalidation is necessary.
		// Dependent means the currently cached resolved geometries use topological sections from the specified layers.
		if (d_resolved_boundary_dependent_topological_sections.set_topological_section_layers(
				valid_reconstructed_geometry_topological_sections_layer_proxies))
		{
			// All resolved topological *boundaries* are now invalid.
			reset_cache(true/*invalidate_resolved_boundaries*/, false/*invalidate_resolved_lines*/);

			// Polling observers need to update themselves with respect to us.
			d_subject_token.invalidate(); // Lines or boundaries are invalid.
		}
		if (d_resolved_line_dependent_topological_sections.set_topological_section_layers(
				valid_reconstructed_geometry_topological_sections_layer_proxies))
		{
			// All resolved topological *lines* are now invalid.
			reset_cache(false/*invalidate_resolved_boundaries*/, true/*invalidate_resolved_lines*/);

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
		// Dependent means the currently cached resolved boundaries (and time spans) use topological sections
		// from the specified layers.
		if (d_resolved_boundary_dependent_topological_sections.set_topological_section_layers(
				resolved_line_topological_sections_layer_proxies))
		{
			// All resolved topological boundaries are now invalid.
			//
			// The resolved lines are not invalid because they can't depend on other resolved lines
			// like the boundaries can.
			reset_cache(true/*invalidate_resolved_boundaries*/, false/*invalidate_resolved_lines*/);

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

	// Set the feature IDs of topological sections referenced by our resolved *boundaries* for *all* times.
	d_resolved_boundary_dependent_topological_sections.set_topological_section_feature_ids(
			d_current_topological_geometry_feature_collections,
			TopologyGeometry::BOUNDARY);

	// Set the feature IDs of topological sections referenced by our resolved *lines* for *all* times.
	d_resolved_line_dependent_topological_sections.set_topological_section_feature_ids(
			d_current_topological_geometry_feature_collections,
			TopologyGeometry::LINE);

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

	// Set the feature IDs of topological sections referenced by our resolved *boundaries* for *all* times.
	d_resolved_boundary_dependent_topological_sections.set_topological_section_feature_ids(
			d_current_topological_geometry_feature_collections,
			TopologyGeometry::BOUNDARY);

	// Set the feature IDs of topological sections referenced by our resolved *lines* for *all* times.
	d_resolved_line_dependent_topological_sections.set_topological_section_feature_ids(
			d_current_topological_geometry_feature_collections,
			TopologyGeometry::LINE);

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
	// Set the feature IDs of topological sections referenced by our resolved *boundaries* for *all* times.
	d_resolved_boundary_dependent_topological_sections.set_topological_section_feature_ids(
			d_current_topological_geometry_feature_collections,
			TopologyGeometry::BOUNDARY);

	// Set the feature IDs of topological sections referenced by our resolved *lines* for *all* times.
	d_resolved_line_dependent_topological_sections.set_topological_section_feature_ids(
			d_current_topological_geometry_feature_collections,
			TopologyGeometry::LINE);

	// The resolved topological geometries are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate(); // Lines or boundaries are invalid.
	d_resolved_lines_subject_token.invalidate(); // Lines are invalid.
}


void
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::reset_cache(
		bool invalidate_resolved_boundaries,
		bool invalidate_resolved_lines)
{
	if (invalidate_resolved_boundaries)
	{
		// Clear any cached resolved topological boundaries.
		d_cached_resolved_boundaries.invalidate();
		d_cached_resolved_boundary_time_span.invalidate();
	}

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
		// Filter out layers that use topologies to reconstruct. These layers cannot supply
		// topological sections because they use topology layers thus creating a cyclic dependency.
		//
		// This also avoids infinite recursion by not checking if the layer is up-to-date (which might then check us, etc).
		//
		// Normally this layer would get excluded when the topological section layers are set, but that only happens when
		// a new reconstruction is performed and we might get called just before that happens, so we need to exclude here also.
		if (rfg_topological_sections_layer_proxy.get_input_layer_proxy()->using_topologies_to_reconstruct())
		{
			continue;
		}

		if (rfg_topological_sections_layer_proxy.is_up_to_date())
		{
			continue;
		}

		// If any cached resolved geometries depend on these topological sections then we need to invalidate our cache.
		//
		// Typically our dependency layers include all reconstruct/resolved-geometry layers
		// due to the usual global search for topological section features. However this means
		// layers that don't contribute topological sections will trigger unnecessary cache flushes
		// which is especially noticeable in the case of rebuilding topology time spans that in turn
		// depend on our resolved topologies.
		// To avoid this we check if any topological sections from a layer can actually contribute.
		if (d_resolved_boundary_dependent_topological_sections.update_topological_section_layer(
				rfg_topological_sections_layer_proxy.get_input_layer_proxy()))
		{
			// All resolved topological *boundaries* are now invalid.
			reset_cache(true/*invalidate_resolved_boundaries*/, false/*invalidate_resolved_lines*/);

			// Polling observers need to update themselves with respect to us.
			d_subject_token.invalidate(); // Lines or boundaries are invalid.
		}
		if (d_resolved_line_dependent_topological_sections.update_topological_section_layer(
				rfg_topological_sections_layer_proxy.get_input_layer_proxy()))
		{
			// All resolved topological *lines* are now invalid.
			reset_cache(false/*invalidate_resolved_boundaries*/, true/*invalidate_resolved_lines*/);

			// Polling observers need to update themselves with respect to us.
			d_subject_token.invalidate(); // Lines or boundaries are invalid.
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
			// which is especially noticeable in the case of rebuilding topology time spans that in turn
			// depend on our resolved topologies.
			// To avoid this we check if any topological sections from a layer can actually contribute.
			if (d_resolved_boundary_dependent_topological_sections.update_topological_section_layer(
					rtl_topological_sections_layer_proxy.get_input_layer_proxy()))
			{
				// All resolved topological *boundaries* are now invalid.
				reset_cache(true/*invalidate_resolved_boundaries*/, false/*invalidate_resolved_lines*/);

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


std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type> &
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::cache_resolved_topological_boundaries(
		const double &reconstruction_time)
{
	// If they're already cached then nothing to do.
	if (d_cached_resolved_boundaries.cached_resolved_topological_boundaries)
	{
		return d_cached_resolved_boundaries.cached_resolved_topological_boundaries.get();
	}

	// Create empty vector of resolved topological boundaries.
	d_cached_resolved_boundaries.cached_resolved_topological_boundaries =
			std::vector<ResolvedTopologicalBoundary::non_null_ptr_type>();

	// First see if we've already cached the current reconstruction time in the resolved boundary time span.
	if (d_cached_resolved_boundary_time_span.cached_resolved_boundary_time_span)
	{
		// If there's a time slot in the time span that matches the reconstruction time
		// then we can re-use the resolved boundaries in that time slot.
		const boost::optional<unsigned int> time_slot =
				d_cached_resolved_boundary_time_span.cached_resolved_boundary_time_span.get()
						->get_time_range().get_time_slot(reconstruction_time);
		if (time_slot)
		{
			// Extract the resolved topological boundaries for the reconstruction time.
			boost::optional<TopologyReconstruct::rtb_seq_type &> resolved_topological_boundaries =
					d_cached_resolved_boundary_time_span.cached_resolved_boundary_time_span.get()
							->get_sample_in_time_slot(time_slot.get());
			if (resolved_topological_boundaries)
			{
				d_cached_resolved_boundaries.cached_resolved_topological_boundaries = resolved_topological_boundaries.get();

				// Get the reconstruct handle from one of the resolved boundaries (if any).
				if (!d_cached_resolved_boundaries.cached_resolved_topological_boundaries->empty())
				{
					boost::optional<ReconstructHandle::type> reconstruct_handle =
							d_cached_resolved_boundaries.cached_resolved_topological_boundaries->front()
									->get_reconstruct_handle();
					if (reconstruct_handle)
					{
						d_cached_resolved_boundaries.cached_reconstruct_handle = reconstruct_handle.get();
					}
					else
					{
						// RTB doesn't have a reconstruct handle - this shouldn't happen.
						d_cached_resolved_boundaries.cached_reconstruct_handle =
								ReconstructHandle::get_next_reconstruct_handle();
					}
				}
				else
				{
					// There will be no reconstructed/resolved boundaries for this handle.
					d_cached_resolved_boundaries.cached_reconstruct_handle =
							ReconstructHandle::get_next_reconstruct_handle();
				}

				return d_cached_resolved_boundaries.cached_resolved_topological_boundaries.get();
			}
		}
	}

	// Generate the resolved topological boundaries for the reconstruction time.
	d_cached_resolved_boundaries.cached_reconstruct_handle =
			create_resolved_topological_boundaries(
					d_cached_resolved_boundaries.cached_resolved_topological_boundaries.get(),
					reconstruction_time);

	return d_cached_resolved_boundaries.cached_resolved_topological_boundaries.get();
}


GPlatesAppLogic::TopologyReconstruct::resolved_boundary_time_span_type::non_null_ptr_to_const_type
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::cache_resolved_boundary_time_span(
		const TimeSpanUtils::TimeRange &time_range)
{
	//PROFILE_FUNC();

	// If one is already cached then attempt to re-use any time slots in common with the
	// new time range. If one is already cached then it contains valid resolved boundaries
	// - it's just that the time range has changed.
	boost::optional<TopologyReconstruct::resolved_boundary_time_span_type::non_null_ptr_type>
			prev_resolved_boundary_time_span = d_cached_resolved_boundary_time_span.cached_resolved_boundary_time_span;

	// Create an empty resolved boundary time span.
	d_cached_resolved_boundary_time_span.cached_resolved_boundary_time_span =
			TopologyReconstruct::resolved_boundary_time_span_type::create(time_range);

	const unsigned int num_time_slots = time_range.get_num_time_slots();

	// As a performance optimisation, for all our topological sections input layers we request a
	// reconstruction tree creator with a cache size the same as the resolved boundary time span
	// (plus one for possible extra time step).
	// This ensures we don't get a noticeable slowdown when the time span range exceeds the
	// size of the cache in the reconstruction layer proxy.
	// We don't actually use the returned ReconstructionTreeCreator here but by specifying a
	// cache size hint we set the size of its internal reconstruction tree cache.

	std::vector<ReconstructLayerProxy::non_null_ptr_type> dependent_reconstructed_geometry_topological_sections_layers;
	d_resolved_boundary_dependent_topological_sections.get_dependent_topological_section_layers(
			dependent_reconstructed_geometry_topological_sections_layers);
	BOOST_FOREACH(
			const ReconstructLayerProxy::non_null_ptr_type &reconstructed_geometry_topological_sections_layer_proxy,
			dependent_reconstructed_geometry_topological_sections_layers)
	{
		reconstructed_geometry_topological_sections_layer_proxy
				->get_current_reconstruction_layer_proxy()->get_reconstruction_tree_creator(num_time_slots + 1);
	}

	std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> dependent_resolved_line_topological_sections_layers;
	d_resolved_boundary_dependent_topological_sections.get_dependent_topological_section_layers(
			dependent_resolved_line_topological_sections_layers);
	BOOST_FOREACH(
			const TopologyGeometryResolverLayerProxy::non_null_ptr_type &resolved_line_topological_sections_layer_proxy,
			dependent_resolved_line_topological_sections_layers)
	{
		resolved_line_topological_sections_layer_proxy
				->get_current_reconstruction_layer_proxy()->get_reconstruction_tree_creator(num_time_slots + 1);
	}

	// Iterate over the time slots of the time span and fill in the resolved topological boundaries.
	for (unsigned int time_slot = 0; time_slot < num_time_slots; ++time_slot)
	{
		const double time = time_range.get_time(time_slot);

		// Attempt to re-use a time slot of the previous resolved boundary time span (if any).
		if (prev_resolved_boundary_time_span)
		{
			// See if the time matches a time slot of the previous resolved boundary time span.
			const TimeSpanUtils::TimeRange prev_time_range = prev_resolved_boundary_time_span.get()->get_time_range();
			boost::optional<unsigned int> prev_time_slot = prev_time_range.get_time_slot(time);
			if (prev_time_slot)
			{
				// Get the resolved topological boundaries from the previous resolved boundary time span.
				boost::optional<TopologyReconstruct::rtb_seq_type &> prev_resolved_topological_boundaries =
						prev_resolved_boundary_time_span.get()->get_sample_in_time_slot(prev_time_slot.get());
				if (prev_resolved_topological_boundaries)
				{
					d_cached_resolved_boundary_time_span.cached_resolved_boundary_time_span.get()->set_sample_in_time_slot(
							prev_resolved_topological_boundaries.get(),
							time_slot);

					// Continue to the next time slot.
					continue;
				}
			}
		}

		// Create the resolved topological boundaries for the current time slot.
		std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> resolved_topological_boundaries;
		create_resolved_topological_boundaries(resolved_topological_boundaries, time);

		d_cached_resolved_boundary_time_span.cached_resolved_boundary_time_span.get()->set_sample_in_time_slot(
				resolved_topological_boundaries,
				time_slot);
	}

	return d_cached_resolved_boundary_time_span.cached_resolved_boundary_time_span.get();
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::create_resolved_topological_boundaries(
		std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries,
		const double &reconstruction_time)
{
	// Get the *dependent* topological section layers.
	std::vector<ReconstructLayerProxy::non_null_ptr_type> dependent_reconstructed_geometry_topological_sections_layers;
	std::vector<TopologyGeometryResolverLayerProxy::non_null_ptr_type> dependent_resolved_line_topological_sections_layers;
	d_resolved_boundary_dependent_topological_sections.get_dependent_topological_section_layers(
			dependent_reconstructed_geometry_topological_sections_layers);
	d_resolved_boundary_dependent_topological_sections.get_dependent_topological_section_layers(
			dependent_resolved_line_topological_sections_layers);

	// If we have no topological features or there are no topological section layers then we
	// can't get any topological sections and we can't resolve any topological boundaries.
	if (d_current_topological_geometry_feature_collections.empty() ||
		(dependent_reconstructed_geometry_topological_sections_layers.empty() &&
			dependent_resolved_line_topological_sections_layers.empty()))
	{
		// There will be no resolved boundaries for this handle.
		return ReconstructHandle::get_next_reconstruct_handle();
	}

	//
	// Generate the resolved topological boundaries for the reconstruction time.
	//

	std::vector<ReconstructHandle::type> topological_geometry_reconstruct_handles;

	// Find the topological section feature IDs referenced by topological *boundaries* for the *current* reconstruction time.
	//
	// This is an optimisation that avoids unnecessary reconstructions. Only those topological sections referenced
	// by boundaries that exist at the current reconstruction time are reconstructed (this saves quite a bit of time).
	std::set<GPlatesModel::FeatureId> topological_sections_referenced;
	BOOST_FOREACH(
			const GPlatesModel::FeatureCollectionHandle::weak_ref &topological_geometry_feature_collection,
			d_current_topological_geometry_feature_collections)
	{
		TopologyInternalUtils::find_topological_sections_referenced(
				topological_sections_referenced,
				topological_geometry_feature_collection,
				TopologyGeometry::BOUNDARY,
				reconstruction_time);
	}

	// Topological boundary sections that are reconstructed static features...
	// We're ensuring that all potential (reconstructed geometry) topological-referenced geometries are
	// reconstructed before we resolve topological boundaries (which reference them indirectly via feature-id).
	std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> topologically_referenced_reconstructed_geometries;
	BOOST_FOREACH(
			const ReconstructLayerProxy::non_null_ptr_type &reconstructed_geometry_topological_sections_layer_proxy,
			dependent_reconstructed_geometry_topological_sections_layers)
	{
		// Reconstruct only the referenced topological section RFGs.
		//
		// This is an optimisation that avoids unnecessary reconstructions. Only those topological sections referenced
		// by boundaries that exist at the current reconstruction time are reconstructed (this saves quite a bit of time).
		const ReconstructHandle::type reconstruct_handle =
				reconstructed_geometry_topological_sections_layer_proxy
						->get_reconstructed_topological_sections(
								topologically_referenced_reconstructed_geometries,
								topological_sections_referenced,
								reconstruction_time);

		// Add the reconstruct handle to our list.
		topological_geometry_reconstruct_handles.push_back(reconstruct_handle);
	}

	// Topological boundary sections that are resolved topological lines...
	// We're ensuring that all potential (resolved line) topologically-referenced geometries are
	// resolved before we resolve topological boundaries (which reference them indirectly via feature-id).
	std::vector<ResolvedTopologicalLine::non_null_ptr_type> topologically_referenced_resolved_lines;
	BOOST_FOREACH(
			const TopologyGeometryResolverLayerProxy::non_null_ptr_type &resolved_line_topological_sections_layer_proxy,
			dependent_resolved_line_topological_sections_layers)
	{
		// Reconstruct only the referenced topological section resolved lines.
		//
		// This is an optimisation that avoids unnecessary reconstructions. Only those topological sections
		// referenced by boundaries that exist at the current reconstruction time are reconstructed.
		const ReconstructHandle::type reconstruct_handle =
				resolved_line_topological_sections_layer_proxy
						->get_resolved_topological_sections(
								topologically_referenced_resolved_lines,
								topological_sections_referenced,
								reconstruction_time);

		// Add the reconstruct handle to our list.
		topological_geometry_reconstruct_handles.push_back(reconstruct_handle);
	}

	// Resolve our boundary features into our sequence of resolved topological boundaries.
	return TopologyUtils::resolve_topological_boundaries(
			resolved_topological_boundaries,
			d_current_topological_geometry_feature_collections,
			d_current_reconstruction_layer_proxy.get_input_layer_proxy()->get_reconstruction_tree_creator(),
			reconstruction_time,
			topological_geometry_reconstruct_handles);
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::TopologyGeometryResolverLayerProxy::create_resolved_topological_lines(
		std::vector<ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_lines,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &topological_geometry_feature_collections,
		const double &reconstruction_time)
{
	// Get the *dependent* topological section layers.
	std::vector<ReconstructLayerProxy::non_null_ptr_type> dependent_reconstructed_geometry_topological_sections_layers;
	d_resolved_line_dependent_topological_sections.get_dependent_topological_section_layers(
			dependent_reconstructed_geometry_topological_sections_layers);

	// If we have no topological features or there are no *reconstructed geometry* topological section
	// layers then we can't get any topological sections and we can't resolve any topological lines.
	// Note that we don't check the *resolved line* topological section layers because resolved lines
	// cannot be used as topological sections for other resolved lines.
	if (topological_geometry_feature_collections.empty() ||
		dependent_reconstructed_geometry_topological_sections_layers.empty())
	{
		// There will be no reconstructed/resolved geometries for this handle.
		return ReconstructHandle::get_next_reconstruct_handle();
	}

	//
	// Generate the resolved topological lines for the reconstruction time.
	//

	std::vector<ReconstructHandle::type> topological_sections_reconstruct_handles;

	// Find the topological section feature IDs referenced by topological *lines* for the *current* reconstruction time.
	//
	// This is an optimisation that avoids unnecessary reconstructions. Only those topological sections referenced
	// by lines that exist at the current reconstruction time are reconstructed (this saves quite a bit of time).
	std::set<GPlatesModel::FeatureId> topological_sections_referenced;
	BOOST_FOREACH(
			const GPlatesModel::FeatureCollectionHandle::weak_ref &topological_geometry_feature_collection,
			topological_geometry_feature_collections)
	{
		TopologyInternalUtils::find_topological_sections_referenced(
				topological_sections_referenced,
				topological_geometry_feature_collection,
				TopologyGeometry::LINE,
				reconstruction_time);
	}

	// Topological sections that are reconstructed static features...
	// We're ensuring that all potential (reconstructed geometry) topological sections are reconstructed
	// before we resolve topological lines (which reference them indirectly via feature-id).
	std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_geometry_topological_sections;
	BOOST_FOREACH(
			const ReconstructLayerProxy::non_null_ptr_type &reconstructed_geometry_topological_sections_layer_proxy,
			dependent_reconstructed_geometry_topological_sections_layers)
	{
		// Reconstruct only the referenced topological section RFGs.
		//
		// This is an optimisation that avoids unnecessary reconstructions. Only those topological sections referenced
		// by lines that exist at the current reconstruction time are reconstructed (this saves quite a bit of time).
		const ReconstructHandle::type reconstruct_handle =
				reconstructed_geometry_topological_sections_layer_proxy
						->get_reconstructed_topological_sections(
								reconstructed_geometry_topological_sections,
								topological_sections_referenced,
								reconstruction_time);

		// Add the reconstruct handle to our list.
		topological_sections_reconstruct_handles.push_back(reconstruct_handle);
	}

	// Note that we don't query *resolved line* topological section layers because resolved lines
	// cannot be used as topological sections for other resolved lines.
	// This is where topological lines differ from topological boundaries.
	// Topological boundaries can use resolved lines as topological sections.

	// Resolve our topological line features into our sequence of resolved topological lines.
	return TopologyUtils::resolve_topological_lines(
			resolved_topological_lines,
			topological_geometry_feature_collections,
			d_current_reconstruction_layer_proxy.get_input_layer_proxy()->get_reconstruction_tree_creator(),
			reconstruction_time,
			topological_sections_reconstruct_handles);
}
