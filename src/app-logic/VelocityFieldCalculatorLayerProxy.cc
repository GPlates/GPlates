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
#include <boost/static_assert.hpp>

#include "VelocityFieldCalculatorLayerProxy.h"
#include "MultiPointVectorField.h"
#include "PlateVelocityUtils.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionTreeCreator.h"
#include "ResolvedTopologicalGeometry.h"
#include "ResolvedTopologicalNetwork.h"

#include "global/GPlatesAssert.h"

#include "maths/MathsUtils.h"


GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::VelocityFieldCalculatorLayerProxy(
		const VelocityParams &velocity_params) :
	d_current_reconstruction_time(0),
	d_current_velocity_params(velocity_params)
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
		const VelocityParams &velocity_params,
		const double &reconstruction_time)
{
	// If we have no velocity domains then there's no points at which to calculate velocities.
	if (d_current_velocity_domain_layer_proxies.get_input_layer_proxies().empty())
	{
		return;
	}

	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	// See if the reconstruction time or velocity parameters have changed.
	if (d_cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time) ||
		d_cached_velocity_params != velocity_params)
	{
		// The velocities are now invalid.
		reset_cache();

		// Note that observers don't need to be updated when the time changes - if they
		// have velocities for a different time they don't need
		// to be updated just because some other client requested a different time.
		d_cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
		d_cached_velocity_params = velocity_params;
	}

	if (!d_cached_multi_point_velocity_fields)
	{
		// Create empty vector of velocity multi-point vector fields.
		d_cached_multi_point_velocity_fields =
				std::vector<MultiPointVectorField::non_null_ptr_type>();

		//
		// Calculate velocities based on the solve velocities method type.
		//

		if (velocity_params.get_solve_velocities_method() ==
			VelocityParams::SOLVE_VELOCITIES_OF_DOMAIN_POINTS)
		{
			//
			// Get the velocities of the reconstructed feature geometries in the velocity domain layers.
			// This requires no surfaces and is essentially the velocities at the positions of the
			// reconstructed feature geometries.
			//

			BOOST_FOREACH(
					LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &velocity_domains_layer_proxy,
					d_current_velocity_domain_layer_proxies.get_input_layer_proxies())
			{
				velocity_domains_layer_proxy.get_input_layer_proxy()->get_reconstructed_feature_velocities(
						d_cached_multi_point_velocity_fields.get(),
						reconstruction_time);
			}
		}
		else if (velocity_params.get_solve_velocities_method() ==
			VelocityParams::SOLVE_VELOCITIES_OF_SURFACES_AT_DOMAIN_POINTS)
		{
			//
			// Get the domain geometries for the velocity calculation.
			//

			std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> velocity_domains;
			BOOST_FOREACH(
					LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &velocity_domains_layer_proxy,
					d_current_velocity_domain_layer_proxies.get_input_layer_proxies())
			{
				velocity_domains_layer_proxy.get_input_layer_proxy()->get_reconstructed_feature_geometries(
						velocity_domains,
						reconstruction_time);
			}

			//
			// Get the input surfaces for the velocity calculation.
			//

			// Static polygons...
			std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_static_polygons;
			BOOST_FOREACH(
					LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &reconstructed_polygons_layer_proxy,
					d_current_reconstructed_polygon_layer_proxies.get_input_layer_proxies())
			{
				reconstructed_polygons_layer_proxy.get_input_layer_proxy()->get_reconstructed_feature_geometries(
						reconstructed_static_polygons,
						reconstruction_time);
			}

			// Topological closed plate polygons...
			std::vector<ResolvedTopologicalGeometry::non_null_ptr_type> resolved_topological_boundaries;

			BOOST_FOREACH(
					LayerProxyUtils::InputLayerProxy<TopologyGeometryResolverLayerProxy> &topological_boundary_resolver_layer_proxy,
					d_current_topological_boundary_resolver_layer_proxies.get_input_layer_proxies())
			{
				topological_boundary_resolver_layer_proxy.get_input_layer_proxy()->get_resolved_topological_boundaries(
						resolved_topological_boundaries,
						reconstruction_time);
			}

			// Topological networks...
			std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> resolved_topological_networks;
			BOOST_FOREACH(
					LayerProxyUtils::InputLayerProxy<TopologyNetworkResolverLayerProxy> &topological_network_resolver_layer_proxy,
					d_current_topological_network_resolver_layer_proxies.get_input_layer_proxies())
			{
				topological_network_resolver_layer_proxy.get_input_layer_proxy()->get_resolved_topological_networks(
						resolved_topological_networks,
						reconstruction_time);
			}

			// Get the velocity smoothing angular distance, etc, if velocity smoothing is enabled.
			boost::optional<PlateVelocityUtils::VelocitySmoothingOptions> velocity_smoothing_options;
			if (velocity_params.get_is_boundary_smoothing_enabled())
			{
				const double boundary_smoothing_angular_half_extent_radians =
						GPlatesMaths::convert_deg_to_rad(
								velocity_params.get_boundary_smoothing_angular_half_extent_degrees());

				const bool exclude_deforming_regions_from_smoothing =
						velocity_params.get_exclude_deforming_regions_from_smoothing();

				velocity_smoothing_options = PlateVelocityUtils::VelocitySmoothingOptions(
						boundary_smoothing_angular_half_extent_radians,
						exclude_deforming_regions_from_smoothing);
			}

			// Calculate the velocity fields using the surfaces.
			PlateVelocityUtils::solve_velocities_on_surfaces(
					d_cached_multi_point_velocity_fields.get(),
					reconstruction_time,
					velocity_domains,
					reconstructed_static_polygons,
					resolved_topological_boundaries,
					resolved_topological_networks,
					velocity_smoothing_options);
		}
		else
		{
			// Update this source code if more 'solve velocities' enumeration values have been added (or removed).
			BOOST_STATIC_ASSERT(VelocityParams::NUM_SOLVE_VELOCITY_METHODS == 2);

			// Shouldn't get here.
			GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		}
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
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::set_current_velocity_params(
		const VelocityParams &velocity_params)
{
	if (d_current_velocity_params == velocity_params)
	{
		// The current velocity params haven't changed so avoid updating any observers unnecessarily.
		return;
	}
	d_current_velocity_params = velocity_params;

	// Note that we don't invalidate our velocities cache because if a velocities is
	// not cached for a requested velocity params then a new velocities is created.
	// Observers need to be aware that the default velocity params have changed.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::add_velocity_domain_layer_proxy(
		const ReconstructLayerProxy::non_null_ptr_type &velocity_domain_layer_proxy)
{
	d_current_velocity_domain_layer_proxies.add_input_layer_proxy(velocity_domain_layer_proxy);

	// The velocities are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::remove_velocity_domain_layer_proxy(
		const ReconstructLayerProxy::non_null_ptr_type &velocity_domain_layer_proxy)
{
	d_current_velocity_domain_layer_proxies.remove_input_layer_proxy(velocity_domain_layer_proxy);

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
		const TopologyGeometryResolverLayerProxy::non_null_ptr_type &topological_boundary_resolver_layer_proxy)
{
	d_current_topological_boundary_resolver_layer_proxies.add_input_layer_proxy(topological_boundary_resolver_layer_proxy);

	// The velocities are now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::remove_topological_boundary_resolver_layer_proxy(
		const TopologyGeometryResolverLayerProxy::non_null_ptr_type &topological_boundary_resolver_layer_proxy)
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
GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::reset_cache()
{
	// Clear any cached resolved topological boundaries.
	d_cached_multi_point_velocity_fields = boost::none;
	d_cached_reconstruction_time = boost::none;
	d_cached_velocity_params = boost::none;
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
	// See if any reconstructed polygons layer proxies have changed.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &reconstructed_polygons_layer_proxy,
			d_current_reconstructed_polygon_layer_proxies.get_input_layer_proxies())
	{
		check_input_layer_proxy(reconstructed_polygons_layer_proxy);
	}

	// See if any resolved boundaries layer proxies have changed.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<TopologyGeometryResolverLayerProxy> &topological_boundary_resolver_layer_proxy,
			d_current_topological_boundary_resolver_layer_proxies.get_input_layer_proxies())
	{
		check_input_layer_proxy(topological_boundary_resolver_layer_proxy);
	}

	// See if the resolved networks layer proxies have changed.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<TopologyNetworkResolverLayerProxy> &topological_network_resolver_layer_proxy,
			d_current_topological_network_resolver_layer_proxies.get_input_layer_proxies())
	{
		check_input_layer_proxy(topological_network_resolver_layer_proxy);
	}

	// See if the velocity domain layer proxies have changed.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &velocity_domain_layer_proxy,
			d_current_velocity_domain_layer_proxies.get_input_layer_proxies())
	{
		check_input_layer_proxy(velocity_domain_layer_proxy);
	}
}
