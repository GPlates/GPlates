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

#include "ReconstructScalarCoverageLayerProxy.h"

#include "DeformedFeatureGeometry.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructedScalarCoverage.h"
#include "ReconstructionGeometryUtils.h"
#include "ScalarCoverageEvolution.h"
#include "ScalarCoverageFeatureProperties.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


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
		get_scalar_coverages(
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
		std::vector<reconstructed_scalar_coverage_non_null_ptr_type> &reconstructed_scalar_coverages,
		const GPlatesPropertyValues::ValueObjectType &scalar_type,
		const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params,
		const double &reconstruction_time)
{
	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	// See if the scalar type or reconstruct scalar coverage parameters have changed.
	if (!d_cached_scalar_coverage_time_spans ||
		d_cached_scalar_type != scalar_type ||
		d_cached_reconstruct_scalar_coverage_params != reconstruct_scalar_coverage_params)
	{
		// Reset everything.
		// All cached reconstruction times assume a specific reconstruct scalar coverage params.
		reset_cache();

		// Create a time-indexed lookup table of scalar values for each scalar coverage feature.
		// The reconstruction-time cache will use this when it caches reconstructed scalar coverages
		// for specific reconstruction times.
		cache_scalar_coverage_time_spans(scalar_type, reconstruct_scalar_coverage_params);

		// Note that observers don't need to be updated when the parameters change - if they
		// have reconstructed scalar coverages for a different set of parameters they don't need
		// to be updated just because some other client requested a set of parameters different from theirs.
		d_cached_scalar_type = scalar_type;
		d_cached_reconstruct_scalar_coverage_params = reconstruct_scalar_coverage_params;
	}

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


const std::vector<GPlatesPropertyValues::ValueObjectType> &
GPlatesAppLogic::ReconstructScalarCoverageLayerProxy::get_scalar_types()
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

	return d_cached_scalar_types.get();
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

	// Clear any cached scalar coverage time spans for the currently cached scalar type and
	// reconstruct scalar coverage params.
	d_cached_scalar_coverage_time_spans = boost::none;

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
			d_current_reconstructed_domain_layer_proxies.get_input_layer_proxies())
	{
		check_input_layer_proxy(reconstructed_domain_layer_proxy);
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

	// Get the coverages from all domain features.
	std::vector<ScalarCoverageFeatureProperties::Coverage> coverages;

	// Iterate over the reconstructed domain layer proxies.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &reconstructed_domain_layer_proxy,
			d_current_reconstructed_domain_layer_proxies.get_input_layer_proxies())
	{
		// Get the domain features.
		std::vector<GPlatesModel::FeatureHandle::weak_ref> domain_features;
		reconstructed_domain_layer_proxy.get_input_layer_proxy()->
				get_current_reconstructable_features(domain_features);

		// Iterate over the domain features.
		std::vector<GPlatesModel::FeatureHandle::weak_ref>::const_iterator
				domain_features_iter = domain_features.begin();
		std::vector<GPlatesModel::FeatureHandle::weak_ref>::const_iterator
				domain_features_end = domain_features.end();
		for ( ; domain_features_iter != domain_features_end; ++domain_features_iter)
		{
			const GPlatesModel::FeatureHandle::weak_ref &domain_feature = *domain_features_iter;

			ScalarCoverageFeatureProperties::get_coverages(coverages, domain_feature);
		}
	}

	// Iterate over the coverages to find the set of unique scalar types.
	std::set<GPlatesPropertyValues::ValueObjectType> unique_scalar_types;
	std::vector<ScalarCoverageFeatureProperties::Coverage>::const_iterator
			coverages_iter = coverages.begin();
	std::vector<ScalarCoverageFeatureProperties::Coverage>::const_iterator
			coverages_end = coverages.end();
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
	if (d_cached_scalar_coverage_time_spans)
	{
		return;
	}

	// Create empty map.
	d_cached_scalar_coverage_time_spans = scalar_coverage_time_span_map_type();

	// Select function to evolve scalar values with (based on the scalar type).
	boost::optional<scalar_evolution_function_type> scalar_evolution_function =
			get_scalar_evolution_function(scalar_type, reconstruct_scalar_coverage_params);

	// Iterate over the reconstructed domain layers.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &reconstructed_domain_layer_proxy,
			d_current_reconstructed_domain_layer_proxies.get_input_layer_proxies())
	{
		if (!scalar_evolution_function)
		{
			//
			// We don't need to evolve scalars over time so we don't need RFGs (ie, don't need deformation strains).
			//

			// Get the domain features (instead of RFGs).
			std::vector<GPlatesModel::FeatureHandle::weak_ref> domain_features;
			reconstructed_domain_layer_proxy.get_input_layer_proxy()->
					get_current_reconstructable_features(domain_features);

			// Iterate over the domain features.
			std::vector<GPlatesModel::FeatureHandle::weak_ref>::const_iterator
					domain_features_iter = domain_features.begin();
			std::vector<GPlatesModel::FeatureHandle::weak_ref>::const_iterator
					domain_features_end = domain_features.end();
			for ( ; domain_features_iter != domain_features_end; ++domain_features_iter)
			{
				const GPlatesModel::FeatureHandle::weak_ref &domain_feature = *domain_features_iter;

				// Find scalar coverages in the domain feature matching the requested scalar type.
				std::vector<scalar_coverage_type> scalar_coverages;
				get_scalar_coverages(scalar_coverages, scalar_type, domain_feature);

				// Iterate over the matching scalar coverages.
				std::vector<scalar_coverage_type>::const_iterator scalar_coverages_iter = scalar_coverages.begin();
				std::vector<scalar_coverage_type>::const_iterator scalar_coverages_end = scalar_coverages.end();
				for ( ; scalar_coverages_iter != scalar_coverages_end; ++scalar_coverages_iter)
				{
					const scalar_coverage_type &scalar_coverage = *scalar_coverages_iter;
					const ScalarCoverageFeatureProperties::Coverage &coverage = scalar_coverage.first;
					GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_to_const_type scalar_data =
							coverage.range[scalar_coverage.second];

					// Extract the present day scalar values from the current scalar coverage.
					const std::vector<double> present_day_scalar_values(
							scalar_data->coordinates_begin(),
							scalar_data->coordinates_end());

					// Create a time span of only present day scalars
					// (will return present day scalars for all reconstruction times).
					ScalarCoverageDeformation::ScalarCoverageTimeSpan::non_null_ptr_type scalar_coverage_time_span =
							ScalarCoverageDeformation::ScalarCoverageTimeSpan::create(present_day_scalar_values);

					// Associate the time span with the (domain) geometry property so we can
					// find it later (via property look up).
					d_cached_scalar_coverage_time_spans->insert(
							scalar_coverage_time_span_map_type::value_type(
									coverage.domain_property,
									scalar_coverage_time_span_mapped_type(
											coverage.range_property,
											scalar_coverage_time_span)));
				}
			}

			continue;
		}

		//
		// We have a valid scalar evolution function so we need a time span of RFGs (containing
		// deformation info such as strain) in order to evolve scalar values backwards from present day.
		//

		const GPlatesAppLogic::ReconstructParams &reconstruct_params =
				reconstructed_domain_layer_proxy.get_input_layer_proxy()->get_current_reconstruct_params();

		const TimeSpanUtils::TimeRange time_range(
				reconstruct_params.get_deformation_begin_time(),
				reconstruct_params.get_deformation_end_time(),
				reconstruct_params.get_deformation_time_increment(),
				TimeSpanUtils::TimeRange::ADJUST_BEGIN_TIME);

		// We get reconstructed 'features' instead of 'geometries' because the former contain something
		// for all features and not just those features that exist at the reconstruction time.
		// We need this since we're building a time span table for each scalar coverage feature and
		// for some time periods it will not have any domain RFGs.
		std::vector<ReconstructContext::ReconstructedFeatureTimeSpan> reconstructed_domain_feature_time_spans;
		reconstructed_domain_layer_proxy.get_input_layer_proxy()
				->get_reconstructed_feature_time_spans(
						reconstructed_domain_feature_time_spans,
						time_range);

		// Iterate over the reconstructed features of the current domain layer.
		std::vector<ReconstructContext::ReconstructedFeatureTimeSpan>::const_iterator
				reconstructed_domain_feature_time_spans_iter = reconstructed_domain_feature_time_spans.begin();
		std::vector<ReconstructContext::ReconstructedFeatureTimeSpan>::const_iterator
				reconstructed_domain_feature_time_spans_end = reconstructed_domain_feature_time_spans.end();
		for ( ;
			reconstructed_domain_feature_time_spans_iter != reconstructed_domain_feature_time_spans_end;
			++reconstructed_domain_feature_time_spans_iter)
		{
			const ReconstructContext::ReconstructedFeatureTimeSpan &reconstructed_domain_feature_time_span =
					*reconstructed_domain_feature_time_spans_iter;

			const GPlatesModel::FeatureHandle::weak_ref domain_feature =
					reconstructed_domain_feature_time_span.get_feature();

			// Find scalar coverages in the domain feature matching the requested scalar type.
			std::vector<scalar_coverage_type> scalar_coverages;
			get_scalar_coverages(scalar_coverages, scalar_type, domain_feature);

			// Iterate over the matching scalar coverages.
			std::vector<scalar_coverage_type>::const_iterator scalar_coverages_iter = scalar_coverages.begin();
			std::vector<scalar_coverage_type>::const_iterator scalar_coverages_end = scalar_coverages.end();
			for ( ; scalar_coverages_iter != scalar_coverages_end; ++scalar_coverages_iter)
			{
				const scalar_coverage_type &scalar_coverage = *scalar_coverages_iter;
				const ScalarCoverageFeatureProperties::Coverage &coverage = scalar_coverage.first;
				GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_to_const_type scalar_data =
						coverage.range[scalar_coverage.second];

				// Extract the present day scalar values from the current scalar coverage.
				const std::vector<double> present_day_scalar_values(
						scalar_data->coordinates_begin(),
						scalar_data->coordinates_end());

				// Find the reconstruction associated with the geometry property of the current coverage (if any).
				//
				// Note: There may not be reconstruction time spans associated with the coverages if
				// the feature does not exist during the time range (of the time span) - however we still
				// generate scalar coverage time spans because we want to be able to generate
				// scalar values outside the time range.
				std::vector<ReconstructContext::ReconstructionTimeSpan>::const_iterator
						reconstructed_domain_geometry_time_spans_iter =
								reconstructed_domain_feature_time_span.get_reconstruction_time_spans().begin();
				std::vector<ReconstructContext::ReconstructionTimeSpan>::const_iterator
						reconstructed_domain_geometry_time_spans_end =
								reconstructed_domain_feature_time_span.get_reconstruction_time_spans().end();
				for ( ;
					reconstructed_domain_geometry_time_spans_iter != reconstructed_domain_geometry_time_spans_end;
					++reconstructed_domain_geometry_time_spans_iter)
				{
					if (reconstructed_domain_geometry_time_spans_iter->get_geometry_property_iterator() ==
						coverage.domain_property)
					{
						break;
					}
				}

				// Create a time span of:
				//  1) evolved scalar values, if there are RFGs associated with the current coverage (in time range), or
				//  2) only present day scalars (will return present day scalars for all reconstruction times).
				//
				ScalarCoverageDeformation::ScalarCoverageTimeSpan::non_null_ptr_type scalar_coverage_time_span =
						(reconstructed_domain_geometry_time_spans_iter != reconstructed_domain_geometry_time_spans_end)
						? ScalarCoverageDeformation::ScalarCoverageTimeSpan::create(
								scalar_evolution_function.get(),
								*reconstructed_domain_geometry_time_spans_iter->
										get_reconstructed_feature_geometry_time_span(),
								present_day_scalar_values)
						: ScalarCoverageDeformation::ScalarCoverageTimeSpan::create(present_day_scalar_values);

				// Associate the time span with the (domain) geometry property so we can
				// find it later (via property look up).
				d_cached_scalar_coverage_time_spans->insert(
						scalar_coverage_time_span_map_type::value_type(
								coverage.domain_property,
								scalar_coverage_time_span_mapped_type(
										coverage.range_property,
										scalar_coverage_time_span)));
			}
		}
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
			d_cached_scalar_coverage_time_spans,
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
			d_current_reconstructed_domain_layer_proxies.get_input_layer_proxies())
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
				d_cached_scalar_coverage_time_spans->find(
						reconstructed_domain_feature_geometry->property());
		if (scalar_coverage_time_span_iter == d_cached_scalar_coverage_time_spans->end())
		{
			// Current RFG is not from a geometry property that has an associated scalar coverage.
			// So ignore it.
			continue;
		}

		GPlatesModel::FeatureHandle::iterator scalar_coverage_range_property =
				scalar_coverage_time_span_iter->second.first;
		ScalarCoverageDeformation::ScalarCoverageTimeSpan::non_null_ptr_type scalar_coverage_time_span =
				scalar_coverage_time_span_iter->second.second;

		reconstruction_info.cached_reconstructed_scalar_coverages->push_back(
				ReconstructedScalarCoverage::create(
						reconstructed_domain_feature_geometry,
						scalar_coverage_range_property,
						scalar_coverage_time_span->get_scalar_values(reconstruction_time),
						reconstruction_info.cached_reconstructed_scalar_coverages_handle.get()));
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
