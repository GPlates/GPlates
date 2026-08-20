/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2026 The University of Sydney, Australia
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

//
// The OpenGL-backed part of CoRegistrationLayerProxy, split out of
// "CoRegistrationLayerProxy.cc" so that no translation unit compiled into the pyGPlates
// module includes the "opengl/" headers (pyGPlates never renders, and does not link
// OpenGL or GLEW - raster co-registration runs on the GPU so its entry points live here).
//
// "CoRegistrationLayerProxy.h" only forward-declares the OpenGL classes, and the cached
// GLRasterCoRegistration is a shared_ptr, so everything outside this file compiles
// without them.
//

#include <boost/foreach.hpp>
#include <boost/ref.hpp>
#include <boost/utility/in_place_factory.hpp>

#include <QDebug>

#include "CoRegistrationLayerProxy.h"

#include "CoRegistrationData.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionTreeCreator.h"
#include "ReconstructMethodRegistry.h"
#include "ReconstructUtils.h"

#include "data-mining/DataSelector.h"

#include "model/ModelUtils.h"

#include "opengl/GLRasterCoRegistration.h"

#include "utils/FeatureUtils.h"
#include "utils/ReferenceCount.h"


boost::optional<GPlatesAppLogic::CoRegistrationData::non_null_ptr_type>
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
				d_current_seed_layer_proxies)
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
				d_current_target_reconstruct_layer_proxies)
		{
			target_layer_proxies.push_back(target_layer_proxy.get_input_layer_proxy());
		}

		// Get the co-registration target (raster) layer proxies.
		BOOST_FOREACH(
				LayerProxyUtils::InputLayerProxy<RasterLayerProxy> &target_layer_proxy,
				d_current_target_raster_layer_proxies)
		{
			target_layer_proxies.push_back(target_layer_proxy.get_input_layer_proxy());
		}

		d_cached_coregistration_data = CoRegistrationData::create(reconstruction_time);

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


boost::optional<GPlatesAppLogic::CoRegistrationData::non_null_ptr_type>
GPlatesAppLogic::CoRegistrationLayerProxy::get_birth_attribute_data(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesModel::FeatureId &feature_id)
{
	using namespace GPlatesModel;
	//First, find the birth time for the feature.
	FeatureHandle::weak_ref feature = ModelUtils::find_feature(feature_id);
	boost::optional<GPlatesMaths::Real> begin_time = 
		GPlatesUtils::get_begin_time(feature.handle_ptr());
	//If this is no "begin time" in this feature, the attribute data at birth age is not liable.
	//Instead of guessing the birth age and returning questionable result, 
	//we just print warning message and return. No result is better than wrong result.
	if(!begin_time)
	{
		qWarning() << "In CoRegistrationLayerProxy::get_birth_attribute_data() function, " 
				   << "Could not get 'begin time' for the feature: " << feature_id.get().qstring();
		return boost::none;
	}

	//Set the reconstruction time as the birth time of the seed feature.
	double reconstruction_time = (*begin_time).dval();

	//Reconstruct the seed feature.
	//The approach is not ideal because it assumes that each seed layer uses the same rotation tree layer - 
	//but it's probably the case in most situations now so no one will notice it.
	//then later we can revisit a better approach that might involve extending the ReconstructLayerProxy interface 
	//a bit to support co-registration better.
	//That's going to require some thinking though so we'll have to delay that for a later re-factor/re-design.
	ReconstructMethodRegistry reconstruct_method_registry;
	if (d_current_seed_layer_proxies.empty())
	{
		qWarning() << "No input seed layer found.";
		return boost::none;
	}
	ReconstructionTreeCreator reconstruction_tree_creator = 
		d_current_seed_layer_proxies.begin()->get_input_layer_proxy()->
		get_current_reconstruction_layer_proxy()->get_reconstruction_tree_creator();
	std::vector<FeatureCollectionHandle::weak_ref> reconstructable_features_collections;
	FeatureCollectionHandle::non_null_ptr_type fc = FeatureCollectionHandle::create();
	fc->add(FeatureHandle::non_null_ptr_type(feature.handle_ptr()));
	reconstructable_features_collections.push_back(FeatureCollectionHandle::weak_ref(*fc));
	std::vector<ReconstructContext::ReconstructedFeature> reconstructed_seed_features;
	ReconstructUtils::reconstruct(
			reconstructed_seed_features,
			reconstruction_time,
			reconstruct_method_registry,
			reconstructable_features_collections,
			reconstruction_tree_creator);

	//we are expecting exact one reconstructed seed feature here.
	std::size_t num_of_reconstructed_seed = reconstructed_seed_features.size();
	if( num_of_reconstructed_seed !=1 )
	{
		if(0 == num_of_reconstructed_seed)
		{
			qWarning() << "Could not find the reconstructed feature for : " << feature_id.get().qstring();
		}
		if(1 < num_of_reconstructed_seed)
		{
			qWarning() << "More than one reconstructed feature found for : " << feature_id.get().qstring();
			qWarning() << "Check the reason of multiple reconstructed features. Return boost::none for this seed.";
		}
		return boost::none;
	}

	// The target layer proxies (reconstructed geometries and/or rasters).
	std::vector<LayerProxy::non_null_ptr_type> target_layer_proxies;

	// Get the co-registration target (reconstructed geometries) layer proxies.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &target_layer_proxy,
			d_current_target_reconstruct_layer_proxies)
	{
		target_layer_proxies.push_back(target_layer_proxy.get_input_layer_proxy());
	}

	// Get the co-registration target (raster) layer proxies.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<RasterLayerProxy> &target_layer_proxy,
			d_current_target_raster_layer_proxies)
	{
		target_layer_proxies.push_back(target_layer_proxy.get_input_layer_proxy());
	}

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
		
	boost::optional<CoRegistrationData::non_null_ptr_type> coreg_data = 
		CoRegistrationData::create(reconstruction_time);

	// Does the actual co-registration work.
	boost::shared_ptr<GPlatesDataMining::DataSelector> selector =
			GPlatesDataMining::DataSelector::create(
					d_current_coregistration_configuration_table);
	
	// Fill the co-registration data table with results.
	selector->select(
			reconstructed_seed_features,
			target_layer_proxies,
			reconstruction_time,
			coreg_data.get()->data_table(),
			co_register_rasters);
	return coreg_data.get();
}


boost::optional<GPlatesOpenGL::GLRasterCoRegistration &>
GPlatesAppLogic::CoRegistrationLayerProxy::get_raster_co_registration(
		GPlatesOpenGL::GLRenderer &renderer)
{
	// Attempt to create raster co-registration if not already created.
	if (!d_raster_co_registration)
	{
		// Returns boost::none if the required OpenGL extensions are not available.
		boost::optional<GPlatesOpenGL::GLRasterCoRegistration::non_null_ptr_type> raster_co_registration =
				GPlatesOpenGL::GLRasterCoRegistration::create(renderer);
		if (raster_co_registration)
		{
			// Stored as a shared_ptr so the class destructor (compiled into pyGPlates)
			// doesn't need the complete GLRasterCoRegistration type.
			d_raster_co_registration = GPlatesUtils::make_shared_from_intrusive(raster_co_registration.get());
		}
	}

	// Convert to a reference (if raster co-registration supported).
	if (d_raster_co_registration)
	{
		GPlatesOpenGL::GLRasterCoRegistration &raster_co_registration = *d_raster_co_registration;

		return raster_co_registration;
	}

	return boost::none;
}
