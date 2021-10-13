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

#include <algorithm>
#include <boost/foreach.hpp>

#include "RasterLayerProxy.h"

#include "ExtractRasterFeatureProperties.h"
#include "ReconstructUtils.h"


boost::optional<GPlatesAppLogic::ResolvedRaster::non_null_ptr_type>
GPlatesAppLogic::RasterLayerProxy::get_resolved_raster(
		const double &reconstruction_time)
{
	// If we have no input raster feature then there's nothing we can do.
	if (!d_current_raster_feature)
	{
		return boost::none;
	}

	if (!d_current_georeferencing)
	{
		// We need georeferencing information to have a meaningful raster.
		return boost::none;
	}

	if (!get_proxied_raster(reconstruction_time))
	{
		// We need a valid proxied raster for the specified reconstruction time.
		return boost::none;
	}

	// Create a resolved raster reconstruction geometry.
	return ResolvedRaster::create(
			*d_current_raster_feature.get().handle_ptr(),
			reconstruction_time,
			// FIXME: Dodgy - need to remove requirement of providing a reconstruction tree...
			create_reconstruction_tree(reconstruction_time, 0),
			GPlatesUtils::get_non_null_pointer(this),
			d_current_reconstructed_polygons_layer_proxy.get_optional_input_layer_proxy(),
			d_current_age_grid_raster_layer_proxy.get_optional_input_layer_proxy());
}


const boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &
GPlatesAppLogic::RasterLayerProxy::get_proxied_raster(
		const double &reconstruction_time)
{
	// If the reconstruction time has changed then we need to re-resolve the raster feature properties.
	if (d_cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time) ||
		!d_cached_resolved_raster_feature_properties.proxied_raster)
	{
		// Attempt to resolve the raster feature.
		if (!resolve_raster_feature(reconstruction_time))
		{
			invalidate_proxied_raster();
		}

		d_cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
	}

	return d_cached_resolved_raster_feature_properties.proxied_raster;
}


const boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> > &
GPlatesAppLogic::RasterLayerProxy::get_proxied_rasters(
		const double &reconstruction_time)
{
	// If the reconstruction time has changed then we need to re-resolve the raster feature properties.
	if (d_cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time) ||
		!d_cached_resolved_raster_feature_properties.proxied_rasters)
	{
		// Attempt to resolve the raster feature.
		if (!resolve_raster_feature(reconstruction_time))
		{
			invalidate_proxied_raster();
		}

		d_cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
	}

	return d_cached_resolved_raster_feature_properties.proxied_rasters;
}


const GPlatesUtils::SubjectToken &
GPlatesAppLogic::RasterLayerProxy::get_subject_token()
{
	// We've checked to see if any inputs have changed except the layer proxy inputs.
	// This is because we get notified of all changes to input except input layer proxies which
	// we have to poll to see if they changed since we last accessed them - so we do that now.

	// See if the reconstructed polygons layer proxy has changed.
	if (!d_current_reconstructed_polygons_layer_proxy.is_up_to_date())
	{
		// This raster layer proxy is now invalid.
		d_subject_token.invalidate();

		// We're now up-to-date with the reconstructed polygons layer proxy.
		d_current_reconstructed_polygons_layer_proxy.set_up_to_date();
	}

	// See if the age grid raster layer proxy has changed.
	if (!d_current_age_grid_raster_layer_proxy.is_up_to_date())
	{
		// This raster layer proxy is now invalid.
		d_subject_token.invalidate();

		// We're now up-to-date with the age grid raster layer proxy.
		d_current_age_grid_raster_layer_proxy.set_up_to_date();
	}

	return d_subject_token;
}


const GPlatesUtils::SubjectToken &
GPlatesAppLogic::RasterLayerProxy::get_proxied_raster_subject_token(
		const double &reconstruction_time)
{
	// We need to check if the new reconstruction time will resolve to a different proxied raster.
	// Because if it will then we need to let the caller know.
	//
	// Get the proxied raster for the specified time - this will invalidate the
	// proxied raster subject token if the proxied raster has changed (or if the proxied raster
	// could not be obtained).
	get_proxied_raster(reconstruction_time);

	return d_proxied_raster_subject_token;
}


const GPlatesUtils::SubjectToken &
GPlatesAppLogic::RasterLayerProxy::get_raster_feature_subject_token()
{
	return d_raster_feature_subject_token;
}


void
GPlatesAppLogic::RasterLayerProxy::set_current_reconstruction_time(
		const double &reconstruction_time)
{
	d_current_reconstruction_time = reconstruction_time;

	// Note that we don't invalidate our caches because we only do that when the client
	// requests a reconstruction time that differs from the cached reconstruction time.
}


void
GPlatesAppLogic::RasterLayerProxy::set_current_reconstructed_polygons_layer_proxy(
		boost::optional<ReconstructLayerProxy::non_null_ptr_type> reconstructed_polygons_layer_proxy)
{
	d_current_reconstructed_polygons_layer_proxy.set_input_layer_proxy(reconstructed_polygons_layer_proxy);

	// This raster layer proxy has now changed.
	invalidate();
}


void
GPlatesAppLogic::RasterLayerProxy::set_current_age_grid_raster_layer_proxy(
		boost::optional<RasterLayerProxy::non_null_ptr_type> age_grid_raster_layer_proxy)
{
	d_current_age_grid_raster_layer_proxy.set_input_layer_proxy(age_grid_raster_layer_proxy);

	// This raster layer proxy has now changed.
	invalidate();
}


void
GPlatesAppLogic::RasterLayerProxy::set_current_raster_feature(
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> raster_feature,
		const RasterLayerTask::Params &raster_params)
{
	d_current_raster_feature = raster_feature;

	set_raster_params(raster_params);

	// The raster feature has changed.
	invalidate_raster_feature();
}


void
GPlatesAppLogic::RasterLayerProxy::set_current_raster_band_name(
		const RasterLayerTask::Params &raster_params)
{
	set_raster_params(raster_params);

	// The raster feature has changed.
	invalidate_raster_feature();
}


void
GPlatesAppLogic::RasterLayerProxy::modified_raster_feature(
		const RasterLayerTask::Params &raster_params)
{
	set_raster_params(raster_params);

	// The raster feature has changed.
	invalidate_raster_feature();
}


void
GPlatesAppLogic::RasterLayerProxy::set_raster_params(
		const RasterLayerTask::Params &raster_params)
{
	d_current_raster_band_name = raster_params.get_band_name();
	d_current_raster_band_names = raster_params.get_band_names();
	d_current_georeferencing = raster_params.get_georeferencing();
}


void
GPlatesAppLogic::RasterLayerProxy::invalidate_raster_feature()
{
	// The raster feature has changed.
	d_raster_feature_subject_token.invalidate();

	// Also means the proxied raster might have changed so invalidate it.
	invalidate_proxied_raster();
}


void
GPlatesAppLogic::RasterLayerProxy::invalidate_proxied_raster()
{
	d_cached_resolved_raster_feature_properties.reset();

	// The proxied raster is different.
	// Either it's a time-dependent raster and a new time was requested, or
	// the raster feature changed.
	d_proxied_raster_subject_token.invalidate();

	// Also means this raster layer proxy has changed.
	invalidate();
}


void
GPlatesAppLogic::RasterLayerProxy::invalidate()
{
	// This raster layer proxy has changed in some way.
	d_subject_token.invalidate();
}


bool
GPlatesAppLogic::RasterLayerProxy::resolve_raster_feature(
		const double &reconstruction_time)
{
	if (!d_current_raster_feature)
	{
		// We must have a raster feature.
		return false;
	}

	// Extract the raster feature properties at the specified reconstruction time.
	GPlatesAppLogic::ExtractRasterFeatureProperties visitor(reconstruction_time);
	visitor.visit_feature(d_current_raster_feature.get());

	if (!visitor.get_proxied_rasters())
	{
		return false;
	}

	// Is the selected band name one of the available bands in the raster?
	// If not, then change the band name to be the first of the available bands.
	boost::optional<std::size_t> band_name_index_opt =
			find_raster_band_name(d_current_raster_band_names, d_current_raster_band_name);
	if (!band_name_index_opt)
	{
		return false;
	}
	const std::size_t band_name_index = band_name_index_opt.get();

	// Get the proxied raw raster of the selected raster band.
	// Check that the band name index can be used to index into the proxied rasters.
	if (band_name_index >= visitor.get_proxied_rasters().get().size())
	{
		return false;
	}

	// If the proxied rasters have changed then let clients know.
	// This happens for time-dependent rasters as the reconstruction time is changed far enough
	// away from the last cached time that a new raster is encountered.
	if (visitor.get_proxied_rasters() != d_cached_resolved_raster_feature_properties.proxied_rasters ||
		visitor.get_proxied_rasters().get()[band_name_index] != d_cached_resolved_raster_feature_properties.proxied_raster)
	{
		d_proxied_raster_subject_token.invalidate();
	}

	// Get the proxied raster using the selected band index.
	d_cached_resolved_raster_feature_properties.proxied_raster =
			visitor.get_proxied_rasters().get()[band_name_index];

	// Get the list of proxied raw rasters.
	d_cached_resolved_raster_feature_properties.proxied_rasters = visitor.get_proxied_rasters();

	return true;
}
