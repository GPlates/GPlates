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
#include <boost/utility/in_place_factory.hpp>

#include "CoRegistrationLayerProxy.h"

#include "CoRegistrationData.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionTreeCreator.h"

#include "data-mining/DataSelector.h"

#include "opengl/GLRasterCoRegistration.h"

#include <boost/foreach.hpp>

GPlatesAppLogic::CoRegistrationLayerProxy::CoRegistrationLayerProxy() :
	// Start off with a reconstruction layer proxy that does identity rotations.
	d_current_reconstruction_layer_proxy(ReconstructionLayerProxy::create()),
	d_current_reconstruction_time(0)
{
	// Defined in ".cc" file because...
	// non_null_ptr destructors require complete type of class they're referring to.
	// Compiler will call destructors of already-constructed members if constructor throws exception.
}


GPlatesAppLogic::CoRegistrationLayerProxy::~CoRegistrationLayerProxy()
{
	// Defined in ".cc" file because...
	// non_null_ptr destructors require complete type of class they're referring to.
}


boost::optional<GPlatesAppLogic::coregistration_data_non_null_ptr_type>
GPlatesAppLogic::CoRegistrationLayerProxy::get_coregistration_data(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &reconstruction_time)
{
	// We have at least an empty co-registration configuration table we can always proceed past that.

	// See if the reconstruction time has changed.
	if (d_cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time))
	{
		// The co-registration data is now invalid.
		reset_cache();

		// Note that observers don't need to be updated when the time changes - if they
		// have co-registration data for a different time they don't need
		// to be updated just because some other client requested a different time.
		d_cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
	}

	// See if any input layer proxies have changed.
	check_input_layer_proxies();

	if (!d_cached_coregistration_data)
	{
		// Get the co-registration reconstructed seed features from the input seed layer proxies.
		std::vector<ReconstructContext::ReconstructedFeature> reconstructed_seed_features;
		BOOST_FOREACH(
				LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &seed_layer_proxy,
				d_current_seed_layer_proxies.get_input_layer_proxies())
		{
			seed_layer_proxy.get_input_layer_proxy()->get_reconstructed_features(
					reconstructed_seed_features,
					reconstruction_time);
		}

		// The target layer proxies (reconstructed geometries and/or rasters).
		std::vector<LayerProxy::non_null_ptr_type> target_layer_proxies;

		// Get the co-registration target (reconstructed geometries) layer proxies.
		BOOST_FOREACH(
				LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &target_layer_proxy,
				d_current_target_reconstruct_layer_proxies.get_input_layer_proxies())
		{
			target_layer_proxies.push_back(target_layer_proxy.get_input_layer_proxy());
		}

		// Get the co-registration target (raster) layer proxies.
		BOOST_FOREACH(
				LayerProxyUtils::InputLayerProxy<RasterLayerProxy> &target_layer_proxy,
				d_current_target_raster_layer_proxies.get_input_layer_proxies())
		{
			target_layer_proxies.push_back(target_layer_proxy.get_input_layer_proxy());
		}

		// Get the reconstruction tree for the requested reconstruction time.
		ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
				d_current_reconstruction_layer_proxy.get_input_layer_proxy()
						->get_reconstruction_tree(reconstruction_time);

		d_cached_coregistration_data = CoRegistrationData::create(reconstruction_tree);

		// Does the actual co-registration work.
		boost::shared_ptr<GPlatesDataMining::DataSelector> selector =
				GPlatesDataMining::DataSelector::create(
						d_current_coregistration_configuration_table);

		// Co-register rasters if we can (if the run-time system supports it).
		boost::optional<GPlatesDataMining::DataSelector::RasterCoRegistration> co_register_rasters;
		if (get_raster_co_registration(renderer))
		{
			// Pass GPlatesDataMining::DataSelector::RasterCoRegistration constructor parameters to
			// construct a new object directly in-place.
			co_register_rasters = boost::in_place(
					boost::ref(renderer),
					boost::ref(get_raster_co_registration(renderer).get()));
		}
		
		// Fill the co-registration data table with results.
		selector->select(
				reconstructed_seed_features,
				target_layer_proxies,
				reconstruction_time,
				d_cached_coregistration_data.get()->data_table(),
				co_register_rasters);
	}

	return d_cached_coregistration_data.get();
}


const GPlatesUtils::SubjectToken &
GPlatesAppLogic::CoRegistrationLayerProxy::get_subject_token()
{
	// We've checked to see if any inputs have changed except the layer proxy inputs.
	// This is because we get notified of all changes to input except input layer proxies which
	// we have to poll to see if they changed since we last accessed them - so we do that now.
	check_input_layer_proxies();

	return d_subject_token;
}


void
GPlatesAppLogic::CoRegistrationLayerProxy::set_current_reconstruction_time(
		const double &reconstruction_time)
{
	d_current_reconstruction_time = reconstruction_time;

	// Note that we don't reset our caches because we only do that when the client
	// requests a reconstruction time that differs from the cached reconstruction time.
}


void
GPlatesAppLogic::CoRegistrationLayerProxy::set_current_reconstruction_layer_proxy(
		const ReconstructionLayerProxy::non_null_ptr_type &reconstruction_layer_proxy)
{
	d_current_reconstruction_layer_proxy.set_input_layer_proxy(reconstruction_layer_proxy);

	// The co-registration data is now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::CoRegistrationLayerProxy::add_coregistration_seed_layer_proxy(
		const ReconstructLayerProxy::non_null_ptr_type &coregistration_seed_layer_proxy)
{
	d_current_seed_layer_proxies.add_input_layer_proxy(coregistration_seed_layer_proxy);

	// The co-registration data is now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::CoRegistrationLayerProxy::remove_coregistration_seed_layer_proxy(
		const ReconstructLayerProxy::non_null_ptr_type &coregistration_seed_layer_proxy)
{
	d_current_seed_layer_proxies.remove_input_layer_proxy(coregistration_seed_layer_proxy);

	// The co-registration data is now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::CoRegistrationLayerProxy::add_coregistration_target_layer_proxy(
		const ReconstructLayerProxy::non_null_ptr_type &coregistration_target_layer_proxy)
{
	d_current_target_reconstruct_layer_proxies.add_input_layer_proxy(coregistration_target_layer_proxy);

	// The co-registration data is now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::CoRegistrationLayerProxy::remove_coregistration_target_layer_proxy(
		const ReconstructLayerProxy::non_null_ptr_type &coregistration_target_layer_proxy)
{
	d_current_target_reconstruct_layer_proxies.remove_input_layer_proxy(coregistration_target_layer_proxy);

	// The co-registration data is now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::CoRegistrationLayerProxy::add_coregistration_target_layer_proxy(
		const RasterLayerProxy::non_null_ptr_type &coregistration_target_layer_proxy)
{
	d_current_target_raster_layer_proxies.add_input_layer_proxy(coregistration_target_layer_proxy);

	// The co-registration data is now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::CoRegistrationLayerProxy::remove_coregistration_target_layer_proxy(
		const RasterLayerProxy::non_null_ptr_type &coregistration_target_layer_proxy)
{
	d_current_target_raster_layer_proxies.remove_input_layer_proxy(coregistration_target_layer_proxy);

	// The co-registration data is now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::CoRegistrationLayerProxy::set_current_coregistration_configuration_table(
		const GPlatesDataMining::CoRegConfigurationTable &coregistration_configuration_table)
{
	if (d_current_coregistration_configuration_table == coregistration_configuration_table)
	{
		// The current configuration hasn't changed so avoid updating any observers unnecessarily.
		return;
	}
	d_current_coregistration_configuration_table = coregistration_configuration_table;

	// The co-registration data is now invalid.
	reset_cache();

	// Polling observers need to update themselves with respect to us.
	d_subject_token.invalidate();
}


void
GPlatesAppLogic::CoRegistrationLayerProxy::reset_cache()
{
	// Clear the cached co-registration result data.
	d_cached_coregistration_data = boost::none;
	d_cached_reconstruction_time = boost::none;
}


template <class InputLayerProxyWrapperType>
void
GPlatesAppLogic::CoRegistrationLayerProxy::check_input_layer_proxy(
		InputLayerProxyWrapperType &input_layer_proxy_wrapper)
{
	// See if the input layer proxy has changed.
	if (!input_layer_proxy_wrapper.is_up_to_date())
	{
		// The co-registration data is now invalid.
		reset_cache();

		// We're now up-to-date with respect to the input layer proxy.
		input_layer_proxy_wrapper.set_up_to_date();

		// Polling observers need to update themselves with respect to us.
		d_subject_token.invalidate();
	}
}


void
GPlatesAppLogic::CoRegistrationLayerProxy::check_input_layer_proxies()
{
	// See if the reconstruction layer proxy has changed.
	check_input_layer_proxy(d_current_reconstruction_layer_proxy);

	// See if any reconstructed seed layer proxies have changed.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &seed_layer_proxy,
			d_current_seed_layer_proxies.get_input_layer_proxies())
	{
		check_input_layer_proxy(seed_layer_proxy);
	}

	// See if any target (reconstructed geometries) layer proxies have changed.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &target_layer_proxy,
			d_current_target_reconstruct_layer_proxies.get_input_layer_proxies())
	{
		check_input_layer_proxy(target_layer_proxy);
	}

	// See if any target (raster) layer proxies have changed.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<RasterLayerProxy> &target_layer_proxy,
			d_current_target_raster_layer_proxies.get_input_layer_proxies())
	{
		check_input_layer_proxy(target_layer_proxy);
	}
}


boost::optional<GPlatesOpenGL::GLRasterCoRegistration &>
GPlatesAppLogic::CoRegistrationLayerProxy::get_raster_co_registration(
		GPlatesOpenGL::GLRenderer &renderer)
{
	// Attempt to create raster co-registration if not already created.
	if (!d_raster_co_registration)
	{
		// Returns boost::none if the required OpenGL extensions are not available.
		d_raster_co_registration = GPlatesOpenGL::GLRasterCoRegistration::create(renderer);
	}

	// Convert from a non_null_ptr to a reference (if raster co-registration supported).
	if (d_raster_co_registration)
	{
		GPlatesOpenGL::GLRasterCoRegistration &raster_co_registration = *d_raster_co_registration.get();

		return raster_co_registration;
	}

	return boost::none;
}
