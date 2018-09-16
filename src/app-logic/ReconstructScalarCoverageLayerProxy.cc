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

#include <set>
#include <utility>
#include <vector>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>

#include "ReconstructScalarCoverageLayerProxy.h"

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructedScalarCoverage.h"
#include "ReconstructionGeometryUtils.h"
#include "TopologyReconstructedFeatureGeometry.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/PropertyName.h"

#include "property-values/GmlTimeInstant.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * Typedef for a scalar type within a coverage.
		 *
		 * The integer indexes into the scalar types in the coverage's range.
		 */
		typedef std::pair<
						ScalarCoverageFeatureProperties::Coverage,
						unsigned int/*scalar index in coverage's range*/> scalar_coverage_type;

		/**
		 * Appends to @a coverages those coverages in @a feature that match @a scalar_type.
		 *
		 * Each integer indexes into the scalar types in the coverage's range that match @a scalar_type.
		 */
		void
		get_scalar_coverages_of_scalar_type_from_feature(
				std::vector<scalar_coverage_type> &scalar_coverages,
				const GPlatesPropertyValues::ValueObjectType &scalar_type,
				const GPlatesModel::FeatureHandle::weak_ref &feature)
		{
			// Extract the coverages from the feature.
			// Note: There can be multiple coverages in a single feature if it has multiple geometry
			// properties that are each associated with a coverage property (scalar values).
			std::vector<ScalarCoverageFeatureProperties::Coverage> coverages;
			ScalarCoverageFeatureProperties::get_coverages(coverages, feature);

			// Iterate over the coverages of the current feature.
			std::vector<ScalarCoverageFeatureProperties::Coverage>::const_iterator
					coverages_iter = coverages.begin();
			std::vector<ScalarCoverageFeatureProperties::Coverage>::const_iterator
					coverages_end = coverages.end();
			for ( ; coverages_iter != coverages_end; ++coverages_iter)
			{
				const ScalarCoverageFeatureProperties::Coverage &coverage = *coverages_iter;

				// See if any scalar types of the current coverage are what we're currently interested in.
				const unsigned int num_scalar_types = coverage.range.size();
				for (unsigned int scalar_type_index = 0; scalar_type_index < num_scalar_types; ++scalar_type_index)
				{
					if (coverage.range[scalar_type_index]->value_object_type() == scalar_type)
					{
						scalar_coverages.push_back(
								scalar_coverage_type(coverage, scalar_type_index));

						break;
					}
				}
			}
		}
	}
}


GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::ReconstructScalarCoverageLayerProxy(
		const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params,
		unsigned int max_num_reconstructions_in_cache) :
	d_current_reconstruction_time(0),
	d_current_scalar_type(GPlatesPropertyValues::ValueObjectType::create_gpml("")),
	d_current_reconstruct_scalar_coverage_params(reconstruct_scalar_coverage_params),
	d_cached_reconstructions(
			boost::bind(&ReconstructScalarCoverageLayerProxy::create_empty_reconstruction_info, this, _1),
			max_num_reconstructions_in_cache)
{
	// Defined in ".cc" file because...
	// non_null_ptr destructors require complete type of class they're referring to.
	// Compiler will call destructors of already-constructed members if constructor throws exception.
}


GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::~ReconstructScalarCoverageLayerProxy()
{
	// Defined in ".cc" file because...
	// non_null_ptr destructors require complete type of class they're referring to.
}


GPlatesAppLogic::ReconstructHandle::type
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::get_reconstructed_scalar_coverages(
		std::vector<ReconstructedScalarCoverage::non_null_ptr_type> &reconstructed_scalar_coverages,
		const GPlatesPropertyValues::ValueObjectType &scalar_type,
		const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params,
		const double &reconstruction_time)
{
	// Ensure that the scalar coverage "time spans" are cached for the specified scalar type and params.
	// This will also clear any cached reconstructed scalar coverages if the time spans were out-of-date.
	std::vector<ReconstructedScalarCoverageTimeSpan> reconstructed_scalar_coverage_time_spans;
	get_reconstructed_scalar_coverage_time_spans(
			reconstructed_scalar_coverage_time_spans,
			scalar_type,
			reconstruct_scalar_coverage_params);

	// Lookup the cached ReconstructionInfo associated with the reconstruction time.
	ReconstructionInfo &reconstruction_info = d_cached_reconstructions.get_value(reconstruction_time);

	// If the cached reconstruction info has not been initialised or has been evicted from the cache...
	if (!reconstruction_info.cached_reconstructed_scalar_coverages)
	{
		cache_reconstructed_scalar_coverages(reconstruction_info, reconstruction_time);
	}

	// Append our cached reconstructed scalar coverages to the caller's sequence.
	reconstructed_scalar_coverages.insert(
			reconstructed_scalar_coverages.end(),
			reconstruction_info.cached_reconstructed_scalar_coverages->begin(),
			reconstruction_info.cached_reconstructed_scalar_coverages->end());

	return reconstruction_info.cached_reconstructed_scalar_coverages_handle.get();
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::get_reconstructed_scalar_coverage_time_spans(
		std::vector<ReconstructedScalarCoverageTimeSpan> &reconstructed_scalar_coverage_time_spans,
		const GPlatesPropertyValues::ValueObjectType &scalar_type,
		const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params)
{
	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	// See if the scalar type or reconstruct scalar coverage parameters have changed.
	if (!d_cached_scalar_coverage_time_span_info ||
		d_cached_scalar_type != scalar_type ||
		d_cached_reconstruct_scalar_coverage_params != reconstruct_scalar_coverage_params)
	{
		// Reset everything.
		// All cached reconstruction times assume a specific reconstruct scalar coverage params.
		reset_cache();

		// Create a time-indexed lookup table of scalar values for each scalar coverage feature.
		// The reconstruction-time cache will also use this when it caches reconstructed scalar
		// coverages for specific reconstruction times.
		cache_scalar_coverage_time_spans(scalar_type, reconstruct_scalar_coverage_params);

		// Note that observers don't need to be updated when the parameters change - if they
		// have reconstructed scalar coverages for a different set of parameters they don't need
		// to be updated just because some other client requested a set of parameters different from theirs.
		d_cached_scalar_type = scalar_type;
		d_cached_reconstruct_scalar_coverage_params = reconstruct_scalar_coverage_params;
	}

	// Append our cached reconstructed scalar coverage time spans to the caller's sequence.
	reconstructed_scalar_coverage_time_spans.insert(
			reconstructed_scalar_coverage_time_spans.end(),
			d_cached_scalar_coverage_time_span_info->cached_reconstructed_scalar_coverage_time_spans.begin(),
			d_cached_scalar_coverage_time_span_info->cached_reconstructed_scalar_coverage_time_spans.end());
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::get_scalar_coverages(
		std::vector<GPlatesAppLogic::ScalarCoverageFeatureProperties::Coverage> &scalar_coverages)
{
	// See if any input layer proxies have changed.
	//
	// Note: We actually only need to detect if the domain *features* have changed,
	// but it's easier to just check for *any* changes (though means updating more than necessary).
	check_input_layer_proxies();

	if (!d_cached_scalar_coverages)
	{
		cache_scalar_coverages();
	}

	scalar_coverages.insert(
			scalar_coverages.end(),
			d_cached_scalar_coverages->begin(),
			d_cached_scalar_coverages->end());
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::get_scalar_types(
		std::vector<GPlatesPropertyValues::ValueObjectType> &scalar_types)
{
	// See if any input layer proxies have changed.
	//
	// Note: We actually only need to detect if the domain *features* have changed,
	// but it's easier to just check for *any* changes (though means updating more than necessary).
	check_input_layer_proxies();

	if (!d_cached_scalar_types)
	{
		cache_scalar_types();
	}

	scalar_types.insert(
			scalar_types.end(),
			d_cached_scalar_types->begin(),
			d_cached_scalar_types->end());
}


const GPlatesUtils::SubjectToken &
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::get_subject_token()
{
	// We've checked to see if any inputs have changed except the layer proxy inputs.
	// This is because we get notified of all changes to input except input layer proxies which
	// we have to poll to see if they changed since we last accessed them - so we do that now.
	check_input_layer_proxies();

	return d_subject_token;
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::set_current_reconstruction_time(
		const double &reconstruction_time)
{
	d_current_reconstruction_time = reconstruction_time;

	// Note that we don't invalidate our reconstructed scalar coverages cache because it caches
	// over all reconstruction times (well, it has a lookup table indexed by time).
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::set_current_scalar_type(
		const GPlatesPropertyValues::ValueObjectType &scalar_type)
{
	if (d_current_scalar_type == scalar_type)
	{
		// The current scalar type hasn't changed so avoid updating any observers unnecessarily.
		return;
	}
	d_current_scalar_type = scalar_type;

	// Note that we don't invalidate our reconstructed scalar coverages cache because if a scalar coverages is
	// not cached for a requested scalar type then a new scalar coverages is created.
	// Observers need to be aware that the default scalar type has changed.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::set_current_reconstruct_scalar_coverage_params(
		const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params)
{
	if (d_current_reconstruct_scalar_coverage_params == reconstruct_scalar_coverage_params)
	{
		// The current scalar coverages params haven't changed so avoid updating any observers unnecessarily.
		return;
	}
	d_current_reconstruct_scalar_coverage_params = reconstruct_scalar_coverage_params;

	// Note that we don't invalidate our reconstructed scalar coverages cache because if a scalar coverages is
	// not cached for a requested scalar coverages params then a new scalar coverages is created.
	// Observers need to be aware that the default scalar coverages params have changed.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::add_reconstructed_domain_layer_proxy(
		const ReconstructLayerProxy::non_null_ptr_type &reconstructed_domain_layer_proxy)
{
	d_current_reconstructed_domain_layer_proxies.add_input_layer_proxy(reconstructed_domain_layer_proxy);

	// The cached reconstruction info is now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::remove_reconstructed_domain_layer_proxy(
		const ReconstructLayerProxy::non_null_ptr_type &reconstructed_domain_layer_proxy)
{
	d_current_reconstructed_domain_layer_proxies.remove_input_layer_proxy(reconstructed_domain_layer_proxy);

	// The cached reconstruction info is now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::reset_cache()
{
	// Clear the cached scalar type and reconstruct scalar coverage params.
	d_cached_scalar_type = boost::none;
	d_cached_reconstruct_scalar_coverage_params = boost::none;

	// Clear the cache scalar types (associated with the domain features).
	d_cached_scalar_types = boost::none;

	// Clear the cache scalar coverages (associated with the domain features).
	d_cached_scalar_coverages = boost::none;

	// Clear any cached scalar coverage time spans for the currently cached scalar type and
	// reconstruct scalar coverage params.
	d_cached_scalar_coverage_time_span_info = boost::none;

	// Clear any cached reconstruction info for any reconstruction times.
	d_cached_reconstructions.clear();
}


template <class InputLayerProxyWrapperType>
void
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::check_input_layer_proxy(
		InputLayerProxyWrapperType &input_layer_proxy_wrapper)
{
	// See if the input layer proxy has changed.
	if (!input_layer_proxy_wrapper.is_up_to_date())
	{
		// The cached reconstruction info is now invalid.
		reset_cache();

		// We're now up-to-date with respect to the input layer proxy.
		input_layer_proxy_wrapper.set_up_to_date();

		// Polling observers need to update themselves with respect to us.
		d_subject_token.invalidate();
	}
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::check_input_layer_proxies()
{
	// See if the reconstructed domain layer proxies have changed.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &reconstructed_domain_layer_proxy,
			d_current_reconstructed_domain_layer_proxies)
	{
		check_input_layer_proxy(reconstructed_domain_layer_proxy);
	}
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::cache_scalar_coverages()
{
	// If already cached then return.
	if (d_cached_scalar_coverages)
	{
		return;
	}

	// Create an empty vector.
	d_cached_scalar_coverages = std::vector<ScalarCoverageFeatureProperties::Coverage>();

	// Iterate over the reconstructed domain layer proxies.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &reconstructed_domain_layer_proxy,
			d_current_reconstructed_domain_layer_proxies)
	{
		// Get the domain features.
		//
		// Note that we only consider non-topological features since a feature collection may contain a mixture
		// of topological and non-topological (thus creating reconstruct layer and topological layer).
		std::vector<GPlatesModel::FeatureHandle::weak_ref> domain_features;
		reconstructed_domain_layer_proxy.get_input_layer_proxy()->get_current_reconstructable_features(domain_features);

		// Iterate over the domain features.
		std::vector<GPlatesModel::FeatureHandle::weak_ref>::const_iterator
				domain_features_iter = domain_features.begin();
		std::vector<GPlatesModel::FeatureHandle::weak_ref>::const_iterator
				domain_features_end = domain_features.end();
		for ( ; domain_features_iter != domain_features_end; ++domain_features_iter)
		{
			const GPlatesModel::FeatureHandle::weak_ref &domain_feature = *domain_features_iter;

			ScalarCoverageFeatureProperties::get_coverages(d_cached_scalar_coverages.get(), domain_feature);
		}
	}
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::cache_scalar_types()
{
	// If already cached then return.
	if (d_cached_scalar_types)
	{
		return;
	}

	// Create an empty vector.
	d_cached_scalar_types = std::vector<GPlatesPropertyValues::ValueObjectType>();

	std::vector<ScalarCoverageFeatureProperties::Coverage> scalar_coverages;
	get_scalar_coverages(scalar_coverages);

	// Iterate over the coverages to find the set of unique scalar types.
	std::set<GPlatesPropertyValues::ValueObjectType> unique_scalar_types;
	std::vector<ScalarCoverageFeatureProperties::Coverage>::const_iterator
			coverages_iter = scalar_coverages.begin();
	std::vector<ScalarCoverageFeatureProperties::Coverage>::const_iterator
			coverages_end = scalar_coverages.end();
	for ( ; coverages_iter != coverages_end; ++coverages_iter)
	{
		const ScalarCoverageFeatureProperties::Coverage &coverage = *coverages_iter;

		const unsigned int num_scalar_types = coverage.range.size();
		for (unsigned int scalar_type_index = 0; scalar_type_index < num_scalar_types; ++scalar_type_index)
		{
			unique_scalar_types.insert(
					coverage.range[scalar_type_index]->value_object_type());
		}
	}

	d_cached_scalar_types->insert(
			d_cached_scalar_types->end(),
			unique_scalar_types.begin(),
			unique_scalar_types.end());
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::cache_scalar_coverage_time_spans(
		const GPlatesPropertyValues::ValueObjectType &scalar_type,
		const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params)
{
	// If they're already cached then nothing to do.
	if (d_cached_scalar_coverage_time_span_info)
	{
		return;
	}
	d_cached_scalar_coverage_time_span_info = ScalarCoverageTimeSpanInfo();

	// Select function to evolve scalar values with (based on the scalar type).
	boost::optional<scalar_evolution_function_type> scalar_evolution_function =
			get_scalar_evolution_function(scalar_type, reconstruct_scalar_coverage_params);

	// Iterate over the reconstructed domain layers.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &reconstructed_domain_layer_proxy,
			d_current_reconstructed_domain_layer_proxies)
	{
		if (reconstructed_domain_layer_proxy.get_input_layer_proxy()->using_topologies_to_reconstruct())
		{
			//
			// Since the current reconstruct layer is using topologies to reconstruct features then its
			// RFGs will affect the scalar values (since has deformation strain and subduction/consumption
			// of geometry points) so we will need to evolve scalars over time using a geometry time span.
			//

			// Get the topology-reconstructed feature time spans.
			std::vector<ReconstructContext::TopologyReconstructedFeatureTimeSpan> topology_reconstructed_feature_time_spans;
			reconstructed_domain_layer_proxy.get_input_layer_proxy()
					->get_topology_reconstructed_feature_time_spans(topology_reconstructed_feature_time_spans);

			cache_topology_reconstructed_scalar_coverage_time_spans(
					scalar_type,
					topology_reconstructed_feature_time_spans,
					scalar_evolution_function);
		}
		else
		{
			//
			// Since the current reconstruct layer is *not* using topologies to reconstruct features then its
			// RFGs will not affect the scalar values (since no deformation strain and no subduction/consumption
			// of geometry points) so we don't need to evolve scalars over time.
			//

			// Get the domain features (instead of RFGs).
			//
			// Note that we only consider non-topological features since a feature collection may contain a mixture
			// of topological and non-topological (thus creating reconstruct layer and topological layer).
			std::vector<GPlatesModel::FeatureHandle::weak_ref> domain_features;
			reconstructed_domain_layer_proxy.get_input_layer_proxy()->get_current_reconstructable_features(domain_features);

			cache_non_topology_reconstructed_scalar_coverage_time_spans(scalar_type, domain_features);
		}
	}
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::cache_topology_reconstructed_scalar_coverage_time_spans(
		const GPlatesPropertyValues::ValueObjectType &scalar_type,
		const std::vector<ReconstructContext::TopologyReconstructedFeatureTimeSpan> &topology_reconstructed_feature_time_spans,
		boost::optional<scalar_evolution_function_type> scalar_evolution_function)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_cached_scalar_coverage_time_span_info,
			GPLATES_ASSERTION_SOURCE);

	// Iterate over the topology reconstructed features of the current domain layer.
	std::vector<ReconstructContext::TopologyReconstructedFeatureTimeSpan>::const_iterator
			topology_reconstructed_feature_time_spans_iter = topology_reconstructed_feature_time_spans.begin();
	std::vector<ReconstructContext::TopologyReconstructedFeatureTimeSpan>::const_iterator
			topology_reconstructed_feature_time_spans_end = topology_reconstructed_feature_time_spans.end();
	for ( ;
		topology_reconstructed_feature_time_spans_iter != topology_reconstructed_feature_time_spans_end;
		++topology_reconstructed_feature_time_spans_iter)
	{
		const ReconstructContext::TopologyReconstructedFeatureTimeSpan &topology_reconstructed_feature_time_span =
				*topology_reconstructed_feature_time_spans_iter;

		const GPlatesModel::FeatureHandle::weak_ref domain_feature = topology_reconstructed_feature_time_span.get_feature();

		// Find scalar coverages in the domain feature matching the requested scalar type.
		std::vector<scalar_coverage_type> scalar_coverages;
		get_scalar_coverages_of_scalar_type_from_feature(scalar_coverages, scalar_type, domain_feature);

		// Skip current domain feature if it contains no coverages of the specified scalar type.
		if (scalar_coverages.empty())
		{
			continue;
		}

		// Will contain all scalar coverage time spans for the current feature and specified scalar type
		ReconstructedScalarCoverageTimeSpan reconstructed_scalar_coverage_time_span(domain_feature, scalar_type);

		// Iterate over the matching scalar coverages.
		std::vector<scalar_coverage_type>::const_iterator scalar_coverages_iter = scalar_coverages.begin();
		std::vector<scalar_coverage_type>::const_iterator scalar_coverages_end = scalar_coverages.end();
		for ( ; scalar_coverages_iter != scalar_coverages_end; ++scalar_coverages_iter)
		{
			const scalar_coverage_type &scalar_coverage = *scalar_coverages_iter;
			const ScalarCoverageFeatureProperties::Coverage &coverage = scalar_coverage.first;
			GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_to_const_type scalar_data =
					coverage.range[scalar_coverage.second];

			// Extract the scalar values from the current scalar coverage.
			const std::vector<double> scalar_values(
					scalar_data->coordinates_begin(),
					scalar_data->coordinates_end());

			// Find the geometry time span associated with the geometry property of the current coverage (if any).
			std::vector<ReconstructContext::TopologyReconstructedFeatureTimeSpan::GeometryTimeSpan>::const_iterator
					topology_reconstructed_geometry_time_span_iter =
							topology_reconstructed_feature_time_span.get_geometry_time_spans().begin();
			std::vector<ReconstructContext::TopologyReconstructedFeatureTimeSpan::GeometryTimeSpan>::const_iterator
					topology_reconstructed_geometry_time_span_end =
							topology_reconstructed_feature_time_span.get_geometry_time_spans().end();
			for ( ;
				topology_reconstructed_geometry_time_span_iter != topology_reconstructed_geometry_time_span_end;
				++topology_reconstructed_geometry_time_span_iter)
			{
				if (topology_reconstructed_geometry_time_span_iter->get_geometry_property_iterator() ==
					coverage.domain_property)
				{
					break;
				}
			}

			// Create a time span:
			//  1) using the topology-reconstructed geometry time span (and scalar values and evolution function), or
			//  2) only the scalars (will return these same scalars for all reconstruction times).
			//
			ScalarCoverageDeformation::ScalarCoverageTimeSpan::non_null_ptr_type scalar_coverage_time_span =
					(topology_reconstructed_geometry_time_span_iter != topology_reconstructed_geometry_time_span_end)
					? ScalarCoverageDeformation::ScalarCoverageTimeSpan::create(
							topology_reconstructed_geometry_time_span_iter->get_geometry_time_span(),
							scalar_values,
							scalar_evolution_function)
					: ScalarCoverageDeformation::ScalarCoverageTimeSpan::create(scalar_values);

			reconstructed_scalar_coverage_time_span.d_scalar_coverage_time_spans.push_back(
					ReconstructedScalarCoverageTimeSpan::ScalarCoverageTimeSpan(
							coverage.domain_property,
							coverage.range_property,
							scalar_coverage_time_span));

			// Associate the scalar coverage time span with the (domain) geometry property so we can
			// find it later (via property look up) when generating reconstructed scalar coverages.
			d_cached_scalar_coverage_time_span_info->cached_scalar_coverage_time_span_map.insert(
					scalar_coverage_time_span_map_type::value_type(
							coverage.domain_property,
							scalar_coverage_time_span_mapped_type(
									coverage.range_property,
									scalar_coverage_time_span)));
		}

		// Cache all scalar coverages (of specified scalar type) for the current feature.
		d_cached_scalar_coverage_time_span_info->cached_reconstructed_scalar_coverage_time_spans.push_back(
				reconstructed_scalar_coverage_time_span);
	}
}


void
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::cache_non_topology_reconstructed_scalar_coverage_time_spans(
		const GPlatesPropertyValues::ValueObjectType &scalar_type,
		const std::vector<GPlatesModel::FeatureHandle::weak_ref> &domain_features)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_cached_scalar_coverage_time_span_info,
			GPLATES_ASSERTION_SOURCE);

	// Iterate over the domain features.
	std::vector<GPlatesModel::FeatureHandle::weak_ref>::const_iterator domain_features_iter = domain_features.begin();
	std::vector<GPlatesModel::FeatureHandle::weak_ref>::const_iterator domain_features_end = domain_features.end();
	for ( ; domain_features_iter != domain_features_end; ++domain_features_iter)
	{
		const GPlatesModel::FeatureHandle::weak_ref &domain_feature = *domain_features_iter;

		// Find scalar coverages in the domain feature matching the requested scalar type.
		std::vector<scalar_coverage_type> scalar_coverages;
		get_scalar_coverages_of_scalar_type_from_feature(scalar_coverages, scalar_type, domain_feature);

		// Skip current domain feature if it contains no coverages of the specified scalar type.
		if (scalar_coverages.empty())
		{
			continue;
		}

		// Will contain all scalar coverage time spans for the current feature and specified scalar type
		ReconstructedScalarCoverageTimeSpan reconstructed_scalar_coverage_time_span(domain_feature, scalar_type);

		// Iterate over the matching scalar coverages.
		std::vector<scalar_coverage_type>::const_iterator scalar_coverages_iter = scalar_coverages.begin();
		std::vector<scalar_coverage_type>::const_iterator scalar_coverages_end = scalar_coverages.end();
		for ( ; scalar_coverages_iter != scalar_coverages_end; ++scalar_coverages_iter)
		{
			const scalar_coverage_type &scalar_coverage = *scalar_coverages_iter;
			const ScalarCoverageFeatureProperties::Coverage &coverage = scalar_coverage.first;
			GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_to_const_type scalar_data =
					coverage.range[scalar_coverage.second];

			// Extract the scalar values from the current scalar coverage.
			const std::vector<double> scalar_values(
					scalar_data->coordinates_begin(),
					scalar_data->coordinates_end());

			// Create a time span of only the extracted scalar values (they're not evolved over time).
			// The time span will return these scalar values for all reconstruction times.
			ScalarCoverageDeformation::ScalarCoverageTimeSpan::non_null_ptr_type scalar_coverage_time_span =
					ScalarCoverageDeformation::ScalarCoverageTimeSpan::create(scalar_values);

			reconstructed_scalar_coverage_time_span.d_scalar_coverage_time_spans.push_back(
					ReconstructedScalarCoverageTimeSpan::ScalarCoverageTimeSpan(
							coverage.domain_property,
							coverage.range_property,
							scalar_coverage_time_span));

			// Associate the scalar coverage time span with the (domain) geometry property so we can
			// find it later (via property look up) when generating reconstructed scalar coverages.
			d_cached_scalar_coverage_time_span_info->cached_scalar_coverage_time_span_map.insert(
					scalar_coverage_time_span_map_type::value_type(
							coverage.domain_property,
							scalar_coverage_time_span_mapped_type(
									coverage.range_property,
									scalar_coverage_time_span)));
		}

		// Cache all scalar coverages (of specified scalar type) for the current feature.
		d_cached_scalar_coverage_time_span_info->cached_reconstructed_scalar_coverage_time_spans.push_back(
				reconstructed_scalar_coverage_time_span);
	}
}


std::vector<GPlatesAppLogic::ReconstructedScalarCoverage::non_null_ptr_type> &
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::cache_reconstructed_scalar_coverages(
		ReconstructionInfo &reconstruction_info,
		const double &reconstruction_time)
{
	// If they're already cached then nothing to do.
	if (reconstruction_info.cached_reconstructed_scalar_coverages)
	{
		return reconstruction_info.cached_reconstructed_scalar_coverages.get();
	}

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_cached_scalar_coverage_time_span_info &&
				d_cached_scalar_type &&
				d_cached_reconstruct_scalar_coverage_params,
			GPLATES_ASSERTION_SOURCE);

	// Get the next global reconstruct handle - it'll be stored in each RSC.
	reconstruction_info.cached_reconstructed_scalar_coverages_handle =
			ReconstructHandle::get_next_reconstruct_handle();

	// Create empty vector of reconstructed scalar coverages.
	reconstruction_info.cached_reconstructed_scalar_coverages =
			std::vector<ReconstructedScalarCoverage::non_null_ptr_type>();

	// First get the domain RFGs for the reconstruction time.
	// Note that some of those features will not generate RFGs for the reconstruction time if
	// the feature does not exist at the reconstruction time.
	std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_domain_feature_geometries;
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &reconstructed_domain_layer_proxy,
			d_current_reconstructed_domain_layer_proxies)
	{
		reconstructed_domain_layer_proxy.get_input_layer_proxy()
				->get_reconstructed_feature_geometries(reconstructed_domain_feature_geometries, reconstruction_time);
	}

	// Then match each RFG to a scalar coverage time span and create a ReconstructedScalarCoverage.
	BOOST_FOREACH(
			const ReconstructedFeatureGeometry::non_null_ptr_type &reconstructed_domain_feature_geometry,
			reconstructed_domain_feature_geometries)
	{
		scalar_coverage_time_span_map_type::const_iterator scalar_coverage_time_span_iter =
				d_cached_scalar_coverage_time_span_info->cached_scalar_coverage_time_span_map.find(
						reconstructed_domain_feature_geometry->property());
		if (scalar_coverage_time_span_iter == d_cached_scalar_coverage_time_span_info->cached_scalar_coverage_time_span_map.end())
		{
			// Current RFG is not from a geometry property that has an associated scalar coverage time span.
			// So ignore it.
			continue;
		}

		GPlatesModel::FeatureHandle::iterator scalar_coverage_range_property =
				scalar_coverage_time_span_iter->second.first;
		ScalarCoverageDeformation::ScalarCoverageTimeSpan::non_null_ptr_type scalar_coverage_time_span =
				scalar_coverage_time_span_iter->second.second;

		// If the geometry has not been subducted/consumed at the reconstruction time then
		// create a reconstructed scalar coverage.
		//
		// Actually shouldn't need to do this because if we have the domain RFG then its geometry time span already
		// passed this test (both the scalar coverage time span and domain geometry time span are in sync).
		if (scalar_coverage_time_span->is_valid(reconstruction_time))
		{
			reconstruction_info.cached_reconstructed_scalar_coverages->push_back(
					ReconstructedScalarCoverage::create(
							reconstructed_domain_feature_geometry,
							scalar_coverage_range_property,
							d_cached_scalar_type.get(),
							scalar_coverage_time_span,
							reconstruction_info.cached_reconstructed_scalar_coverages_handle.get()));
		}
	}

	return reconstruction_info.cached_reconstructed_scalar_coverages.get();
}


GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::ReconstructionInfo
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::create_empty_reconstruction_info(
		const reconstruction_time_type &reconstruction_time)
{
	// Return empty structure.
	// We'll fill-in/cache the parts of it that are needed - currently there's only
	// reconstructed scalar coverages cached anyway.
	return ReconstructionInfo();
}
