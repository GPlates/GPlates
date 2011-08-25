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

#include "VelocityFieldCalculatorLayerProxy.h"

#include "MultiPointVectorField.h"
#include "PlateVelocityUtils.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionTreeCreator.h"
#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalNetwork.h"


GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::VelocityFieldCalculatorLayerProxy() :
	// Start off with a reconstruction layer proxy that does identity rotations.
	d_current_reconstruction_layer_proxy(ReconstructionLayerProxy::create()),
	d_current_reconstruction_time(0)
{
	// Defined in ".cc" file because...
	// non_null_ptr destructors require complete type of class they're referring to.
	// Compiler will call destructors of already-constructed members if constructor throws exception.
}


GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::~VelocityFieldCalculatorLayerProxy()
{
	// Defined in ".cc" file because...
	// non_null_ptr destructors require complete type of class they're referring to.
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::get_velocity_multi_point_vector_fields(
		std::vector<multi_point_vector_field_non_null_ptr_type> &multi_point_vector_fields,
		const double &reconstruction_time)
{
	// If we have no multi-point features or we are not attached to a reconstruct layer then we
	// can't get any reconstructed topological boundary sections and we can't resolve any
	// topological networks.
	if (d_current_multi_point_feature_collections.empty())
	{
		return;
	}

	// See if the reconstruction time has changed.
	if (d_cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time))
	{
		// The velocities are now invalid.
		reset_cache();

		// Note that observers don't need to be updated when the time changes - if they
		// have velocities for a different time they don't need
		// to be updated just because some other client requested a different time.
		d_cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
	}

	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	if (!d_cached_multi_point_velocity_fields)
	{
		// Create empty vector of velocity multi-point vector fields.
		d_cached_multi_point_velocity_fields =
				std::vector<MultiPointVectorField::non_null_ptr_type>();

		//
		// Get the input surfaces for the velocity calculation.
		//

		// Static polygons...
		std::vector<reconstructed_feature_geometry_non_null_ptr_type> reconstructed_static_polygons;
		BOOST_FOREACH(
				LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &reconstructed_polygons_layer_proxy,
				d_current_reconstructed_polygon_layer_proxies.get_input_layer_proxies())
		{
			reconstructed_polygons_layer_proxy.get_input_layer_proxy()->get_reconstructed_feature_geometries(
					reconstructed_static_polygons,
					reconstruction_time);
		}

		// Topological closed plate polygons...
		std::vector<resolved_topological_boundary_non_null_ptr_type> resolved_topological_boundaries;
		BOOST_FOREACH(
				LayerProxyUtils::InputLayerProxy<TopologyBoundaryResolverLayerProxy> &topological_boundary_resolver_layer_proxy,
				d_current_topological_boundary_resolver_layer_proxies.get_input_layer_proxies())
		{
			topological_boundary_resolver_layer_proxy.get_input_layer_proxy()->get_resolved_topological_boundaries(
					resolved_topological_boundaries,
					reconstruction_time);
		}

		// Topological networks...
		std::vector<resolved_topological_network_non_null_ptr_type> resolved_topological_networks;
		BOOST_FOREACH(
				LayerProxyUtils::InputLayerProxy<TopologyNetworkResolverLayerProxy> &topological_network_resolver_layer_proxy,
				d_current_topological_network_resolver_layer_proxies.get_input_layer_proxies())
		{
			topological_network_resolver_layer_proxy.get_input_layer_proxy()->get_resolved_topological_networks(
					resolved_topological_networks,
					reconstruction_time);
		}

		// Calculate the velocity fields.
		PlateVelocityUtils::solve_velocities(
				d_cached_multi_point_velocity_fields.get(),
				// Used to call when a reconstruction tree is needed for any time/anchor.
				d_current_reconstruction_layer_proxy.get_input_layer_proxy()->get_reconstruction_tree_creator(),
				reconstruction_time,
				d_current_multi_point_feature_collections,
				reconstructed_static_polygons,
				resolved_topological_boundaries,
				resolved_topological_networks);
	}

	// Append our cached multi-point velocity fields to the caller's sequence.
	multi_point_vector_fields.insert(
			multi_point_vector_fields.end(),
			d_cached_multi_point_velocity_fields->begin(),
			d_cached_multi_point_velocity_fields->end());
}


const GPlatesUtils::SubjectToken &
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::get_subject_token()
{
	// We've checked to see if any inputs have changed except the layer proxy inputs.
	// This is because we get notified of all changes to input except input layer proxies which
	// we have to poll to see if they changed since we last accessed them - so we do that now.
	check_input_layer_proxies();

	return d_subject_token;
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::set_current_reconstruction_time(
		const double &reconstruction_time)
{
	d_current_reconstruction_time = reconstruction_time;

	// Note that we don't reset our caches because we only do that when the client
	// requests a reconstruction time that differs from the cached reconstruction time.
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::set_current_reconstruction_layer_proxy(
		const ReconstructionLayerProxy::non_null_ptr_type &reconstruction_layer_proxy)
{
	d_current_reconstruction_layer_proxy.set_input_layer_proxy(reconstruction_layer_proxy);

	// The velocities are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::add_reconstructed_polygons_layer_proxy(
		const ReconstructLayerProxy::non_null_ptr_type &reconstructed_polygons_layer_proxy)
{
	d_current_reconstructed_polygon_layer_proxies.add_input_layer_proxy(reconstructed_polygons_layer_proxy);

	// The velocities are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::remove_reconstructed_polygons_layer_proxy(
		const ReconstructLayerProxy::non_null_ptr_type &reconstructed_polygons_layer_proxy)
{
	d_current_reconstructed_polygon_layer_proxies.remove_input_layer_proxy(reconstructed_polygons_layer_proxy);

	// The velocities are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::add_topological_boundary_resolver_layer_proxy(
		const TopologyBoundaryResolverLayerProxy::non_null_ptr_type &topological_boundary_resolver_layer_proxy)
{
	d_current_topological_boundary_resolver_layer_proxies.add_input_layer_proxy(topological_boundary_resolver_layer_proxy);

	// The velocities are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::remove_topological_boundary_resolver_layer_proxy(
		const TopologyBoundaryResolverLayerProxy::non_null_ptr_type &topological_boundary_resolver_layer_proxy)
{
	d_current_topological_boundary_resolver_layer_proxies.remove_input_layer_proxy(topological_boundary_resolver_layer_proxy);

	// The velocities are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::add_topological_network_resolver_layer_proxy(
		const TopologyNetworkResolverLayerProxy::non_null_ptr_type &topological_network_resolver_layer_proxy)
{
	d_current_topological_network_resolver_layer_proxies.add_input_layer_proxy(topological_network_resolver_layer_proxy);

	// The velocities are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::remove_topological_network_resolver_layer_proxy(
		const TopologyNetworkResolverLayerProxy::non_null_ptr_type &topological_network_resolver_layer_proxy)
{
	d_current_topological_network_resolver_layer_proxies.remove_input_layer_proxy(topological_network_resolver_layer_proxy);

	// The velocities are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::add_multi_point_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	d_current_multi_point_feature_collections.push_back(feature_collection);

	// The velocities are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::remove_multi_point_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// Erase the feature collection from our list.
	d_current_multi_point_feature_collections.erase(
			std::find(
					d_current_multi_point_feature_collections.begin(),
					d_current_multi_point_feature_collections.end(),
					feature_collection));

	// The velocities are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::modified_multi_point_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection)
{
	// The velocities are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::reset_cache()
{
	// Clear any cached resolved topological boundaries.
	d_cached_multi_point_velocity_fields = boost::none;
	d_cached_reconstruction_time = boost::none;
}


template <class InputLayerProxyWrapperType>
void
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::check_input_layer_proxy(
		InputLayerProxyWrapperType &input_layer_proxy_wrapper)
{
	// See if the input layer proxy has changed.
	if (!input_layer_proxy_wrapper.is_up_to_date())
	{
		// The velocities are now invalid.
		reset_cache();

		// We're now up-to-date with respect to the input layer proxy.
		input_layer_proxy_wrapper.set_up_to_date();

		// Polling observers need to update themselves with respect to us.
		d_subject_token.invalidate();
	}
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::check_input_layer_proxies()
{
	// See if the reconstruction layer proxy has changed.
	check_input_layer_proxy(d_current_reconstruction_layer_proxy);

	// See if any reconstructed polygons layer proxies have changed.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &reconstructed_polygons_layer_proxy,
			d_current_reconstructed_polygon_layer_proxies.get_input_layer_proxies())
	{
		check_input_layer_proxy(reconstructed_polygons_layer_proxy);
	}

	// See if any resolved boundaries layer proxies have changed.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<TopologyBoundaryResolverLayerProxy> &topological_boundary_resolver_layer_proxy,
			d_current_topological_boundary_resolver_layer_proxies.get_input_layer_proxies())
	{
		check_input_layer_proxy(topological_boundary_resolver_layer_proxy);
	}

	// See if the resolved networks layer proxy has changed.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<TopologyNetworkResolverLayerProxy> &topological_network_resolver_layer_proxy,
			d_current_topological_network_resolver_layer_proxies.get_input_layer_proxies())
	{
		check_input_layer_proxy(topological_network_resolver_layer_proxy);
	}
}
