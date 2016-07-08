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
#include <QDebug>

#include "RasterLayerProxy.h"

#include "ExtractRasterFeatureProperties.h"
#include "ReconstructLayerProxy.h"
#include "ReconstructUtils.h"
#include "ResolvedRaster.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "property-values/RawRasterUtils.h"


const boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &
GPlatesAppLogic::RasterLayerProxy::get_proxied_raster(
		const double &reconstruction_time,
		const GPlatesPropertyValues::TextContent &raster_band_name)
{
	// If the reconstruction time has changed then we need to re-resolve the raster feature properties.
	if (d_cached_resolved_raster_feature_properties.cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time) ||
		!d_cached_resolved_raster_feature_properties.cached_proxied_raster)
	{
		// Attempt to resolve the raster feature.
		if (!resolve_raster_feature(reconstruction_time, raster_band_name))
		{
			invalidate_proxied_raster();
		}

		d_cached_resolved_raster_feature_properties.cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
	}

	return d_cached_resolved_raster_feature_properties.cached_proxied_raster;
}


const boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> > &
GPlatesAppLogic::RasterLayerProxy::get_proxied_rasters(
		const double &reconstruction_time)
{
	// If the reconstruction time has changed then we need to re-resolve the raster feature properties.
	if (d_cached_resolved_raster_feature_properties.cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time) ||
		!d_cached_resolved_raster_feature_properties.cached_proxied_rasters)
	{
		// Attempt to resolve the raster feature.
		// The raster band is arbitrary here - just use the currently selected raster band name.
		if (!resolve_raster_feature(reconstruction_time, d_current_raster_band_name))
		{
			invalidate_proxied_raster();
		}

		d_cached_resolved_raster_feature_properties.cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
	}

	return d_cached_resolved_raster_feature_properties.cached_proxied_rasters;
}


boost::optional<GPlatesAppLogic::ResolvedRaster::non_null_ptr_type>
GPlatesAppLogic::RasterLayerProxy::get_resolved_raster()
{
	return get_resolved_raster(d_current_reconstruction_time);
}


boost::optional<GPlatesAppLogic::ResolvedRaster::non_null_ptr_type>
GPlatesAppLogic::RasterLayerProxy::get_resolved_raster(
		const double &reconstruction_time)
{
	// If we have no input raster feature then there's nothing we can do.
	if (!d_current_raster_feature ||
		!d_current_raster_feature->is_valid())
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

	// Extract the reconstruct layer proxies from their InputLayerProxy wrappers.
	std::vector<ReconstructLayerProxy::non_null_ptr_type> reconstruct_layer_proxies;
	BOOST_FOREACH(
			const LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &reconstruct_layer_proxy,
			d_current_reconstructed_polygons_layer_proxies.get_input_layer_proxies())
	{
		reconstruct_layer_proxies.push_back(reconstruct_layer_proxy.get_input_layer_proxy());
	}

	// Create a resolved raster reconstruction geometry.
	return ResolvedRaster::create(
			*d_current_raster_feature.get().handle_ptr(),
			reconstruction_time,
			GPlatesUtils::get_non_null_pointer(this),
			reconstruct_layer_proxies,
			d_current_age_grid_raster_layer_proxy.get_optional_input_layer_proxy(),
			d_current_normal_map_raster_layer_proxy.get_optional_input_layer_proxy());
}


bool
GPlatesAppLogic::RasterLayerProxy::does_raster_band_contain_numerical_data(
		const GPlatesPropertyValues::TextContent &raster_band_name)
{
	// Get the proxied raster for present day and the specified band name.
	// Using present day is rather arbitrary but if the raster is time-dependent we're expecting
	// that has the same raster data type for all rasters in the time sequence.
	const boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> proxied_raster =
			get_proxied_raster(0.0/*reconstruction_time*/, raster_band_name);
	if (!proxied_raster)
	{
		return false;
	}

	return GPlatesPropertyValues::RawRasterUtils::does_raster_contain_numerical_data(*proxied_raster.get());
}


boost::optional<GPlatesOpenGL::GLMultiResolutionRasterInterface::non_null_ptr_type>
GPlatesAppLogic::RasterLayerProxy::get_multi_resolution_data_raster(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &reconstruction_time,
		const GPlatesPropertyValues::TextContent &raster_band_name)
{
	// The runtime system needs OpenGL floating-point texture support.
	if (!GPlatesOpenGL::GLDataRasterSource::is_supported(renderer))
	{
		qWarning() << "RasterLayerProxy::get_multi_resolution_data_raster: "
			"Floating-point textures not supported on this graphics hardware.";
		return boost::none;
	}

	if (!d_current_georeferencing)
	{
		// We need georeferencing information to have a multi-resolution raster.
		return boost::none;
	}

	// Get the proxied raster for the specified time and band name.
	// NOTE: If the proxied raster is different than the currently cached proxied raster
	// (can happen for time-dependent rasters) then this call will invalidate the proxied raster.
	const boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> proxied_raster =
			get_proxied_raster(reconstruction_time, raster_band_name);
	if (!proxied_raster)
	{
		return boost::none;
	}

	// The raster type is expected to contain numerical data, not colour RGBA data.
	if (!GPlatesPropertyValues::RawRasterUtils::does_raster_contain_numerical_data(*proxied_raster.get()))
	{
		qWarning() << "RasterLayerProxy::get_multi_resolution_data_raster: "
			"Raster does not contain numerical data (contains colours instead).";
		return boost::none;
	}

	// If we're not up-to-date with respect to the proxied raster...
	// This can happen for time-dependent rasters when the time changes.
	if (!d_proxied_raster_subject_token.is_observer_up_to_date(
		d_cached_multi_resolution_data_raster.cached_proxied_raster_observer))
	{
		// If we have a data raster source then attempt to change the raster first
		// since it's cheaper than rebuilding the multi-resolution raster.
		if (d_cached_multi_resolution_data_raster.cached_data_raster_source)
		{
			if (!d_cached_multi_resolution_data_raster.cached_data_raster_source.get()->change_raster(
					renderer, proxied_raster.get()))
			{
				// The raster dimensions have probably changed - we'll need to rebuild.
				d_cached_multi_resolution_data_raster.cached_data_raster_source = boost::none;
			}
		}

		// We have taken measures to be up-to-date with respect to the proxied raster.
		d_proxied_raster_subject_token.update_observer(
				d_cached_multi_resolution_data_raster.cached_proxied_raster_observer);
	}

	// Rebuild the data raster source if necessary.
	if (!d_cached_multi_resolution_data_raster.cached_data_raster_source)
	{
		// NOTE: We also invalidate the multi-resolution raster since it must link
		// to the data raster source and hence must also be rebuilt.
		d_cached_multi_resolution_data_raster.cached_data_raster = boost::none;

		//qDebug() << "RasterLayerProxy: Rebuilding GLDataRasterSource.";

		boost::optional<GPlatesOpenGL::GLDataRasterSource::non_null_ptr_type> data_raster_source =
				GPlatesOpenGL::GLDataRasterSource::create(renderer, proxied_raster.get());

		d_cached_multi_resolution_data_raster.cached_data_raster_source = data_raster_source;
		if (!d_cached_multi_resolution_data_raster.cached_data_raster_source)
		{
			// Unable to create a data raster source so nothing we can do.
			// This can happen if the raster does not contain numerical data (ie, contains RGBA data).
			qWarning() << "RasterLayerProxy::get_multi_resolution_data_raster: Failed to create raster data source.";
			return boost::none;
		}
	}

	// Rebuild the multi-resolution raster if necessary.
	if (!d_cached_multi_resolution_data_raster.cached_data_raster)
	{
		// NOTE: We also invalidate the multi-resolution cube raster since it must link to the
		// multi-resolution raster and hence must also be rebuilt (if raster is reconstructed).
		d_cached_multi_resolution_data_raster.cached_data_cube_raster = boost::none;

		//qDebug() << "RasterLayerProxy: Rebuilding GLMultiResolutionRaster.";

		// Create the multi-resolution raster.
		//
		// NOTE: We allow caching of the entire raster because, unlike visualisation where only
		// a small region of the raster is typically visible (or it's zoomed out and only accessing
		// a low-resolution mipmap), usually the entire raster is accessed for data processing.
		// And the present day raster (time-dependent rasters aside) is usually accessed repeatedly
		// over many frames and you don't want to incur the large performance hit of continuously
		// reloading tiles from disk (eg, raster co-registration data-mining front-end)
		// - in this case the user can always choose a lower level of detail if the memory usage is
		// too high for their system.
		const GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type multi_resolution_raster =
				GPlatesOpenGL::GLMultiResolutionRaster::create(
						renderer,
						d_current_georeferencing.get(),
						d_current_coordinate_transformation,
						d_cached_multi_resolution_data_raster.cached_data_raster_source.get(),
						GPlatesOpenGL::GLMultiResolutionRaster::DEFAULT_FIXED_POINT_TEXTURE_FILTER,
						GPlatesOpenGL::GLMultiResolutionRaster::CACHE_TILE_TEXTURES_ENTIRE_LEVEL_OF_DETAIL_PYRAMID);

		d_cached_multi_resolution_data_raster.cached_data_raster = multi_resolution_raster;
	}

	// If we are not currently connected to any reconstructed polygons *and* we are not using an age grid
	// then just return the *unreconstructed* raster.
	// Note that we don't require reconstructed polygons to continue past this point.
	if (d_current_reconstructed_polygons_layer_proxies.get_input_layer_proxies().empty() &&
		!d_current_age_grid_raster_layer_proxy)
	{
		return GPlatesOpenGL::GLMultiResolutionRasterInterface::non_null_ptr_type(
			d_cached_multi_resolution_data_raster.cached_data_raster.get());
	}

	//
	// From here on we are *reconstructing* the raster...
	//

	// If we are currently connected to an age grid layer then get the age grid mask from it.
	boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type> age_grid_mask_cube_raster;
	if (d_current_age_grid_raster_layer_proxy)
	{
		age_grid_mask_cube_raster = d_current_age_grid_raster_layer_proxy.get_input_layer_proxy()
				->get_multi_resolution_age_grid_mask(renderer, reconstruction_time);

		if (!age_grid_mask_cube_raster)
		{
			qWarning() << "RasterLayerProxy::get_multi_resolution_data_raster: Failed to obtain age grid.";
		}
	}

	// If age grid mask are different objects then the age grid must have been rebuilt
	// by the age grid layer since last we accessed it.
	// Note that changes *within* age grid objects are detected and handled by the reconstructed raster
	// so we don't need to worry about that.
	if (d_cached_multi_resolution_data_raster.cached_age_grid_mask_cube_raster != age_grid_mask_cube_raster)
	{
		d_cached_multi_resolution_data_raster.cached_age_grid_mask_cube_raster = age_grid_mask_cube_raster;

		// We need to rebuild the reconstructed raster.
		d_cached_multi_resolution_data_raster.cached_data_reconstructed_raster = boost::none;
	}

	// If we have an age grid raster then we are reconstructing with an age grid.
	const bool reconstructing_with_age_grid =
			d_cached_multi_resolution_data_raster.cached_age_grid_mask_cube_raster;

	// Get the reconstructed polygon meshes from the layers containing the reconstructed polygons.
	std::vector<GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::non_null_ptr_type> reconstructed_polygon_meshes;
	if (!d_current_reconstructed_polygons_layer_proxies.get_input_layer_proxies().empty())
	{
		BOOST_FOREACH(
				const LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &reconstructed_polygons_layer_proxy,
				d_current_reconstructed_polygons_layer_proxies.get_input_layer_proxies())
		{
			GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::non_null_ptr_type reconstructed_polygon_meshes_ptr =
					reconstructed_polygons_layer_proxy.get_input_layer_proxy()
							->get_reconstructed_static_polygon_meshes(
									renderer,
									reconstructing_with_age_grid,
									reconstruction_time);
			reconstructed_polygon_meshes.push_back(reconstructed_polygon_meshes_ptr);
		}
	}
	else // *not* reconstructing raster, but still using age grid...
	{
		// Ensure the constant multi-resolution cube mesh has been created since we might be
		// accessing it below to create the reconstructed raster.
		if (!d_cached_multi_resolution_data_raster.cached_multi_resolution_cube_mesh)
		{
			d_cached_multi_resolution_data_raster.cached_multi_resolution_cube_mesh =
					GPlatesOpenGL::GLMultiResolutionCubeMesh::create(renderer);
		}
	}

	// If reconstructed polygon meshes are different objects then they must have been rebuilt by
	// the reconstructed polygons layers since last we accessed them.
	// Note that changes *within* a GLReconstructedStaticPolygonMeshes object are detected and
	// handled by the reconstructed raster so we don't need to worry about that.
	if (d_cached_multi_resolution_data_raster.cached_reconstructed_polygon_meshes != reconstructed_polygon_meshes)
	{
		d_cached_multi_resolution_data_raster.cached_reconstructed_polygon_meshes = reconstructed_polygon_meshes;

		// We need to rebuild the reconstructed raster.
		d_cached_multi_resolution_data_raster.cached_data_reconstructed_raster = boost::none;
	}

	// Rebuild the multi-resolution cube raster if necessary.
	if (!d_cached_multi_resolution_data_raster.cached_data_cube_raster)
	{
		// NOTE: We also invalidate the multi-resolution *reconstructed* raster since it must link
		// to the multi-resolution cube raster and hence must also be rebuilt.
		d_cached_multi_resolution_data_raster.cached_data_reconstructed_raster = boost::none;

		//qDebug() << "RasterLayerProxy: Rebuilding GLMultiResolutionCubeRaster.";

		// Create the multi-resolution cube raster.
		GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type cube_raster =
				GPlatesOpenGL::GLMultiResolutionCubeRaster::create(
						renderer,
						d_cached_multi_resolution_data_raster.cached_data_raster.get());

		d_cached_multi_resolution_data_raster.cached_data_cube_raster = cube_raster;
	}

	if (!d_cached_multi_resolution_data_raster.cached_data_reconstructed_raster)
	{
		// NOTE: We also invalidate the *reconstructed* multi-resolution cube raster since it must link
		// to the multi-resolution *reconstructed* raster and hence must also be rebuilt.
		d_cached_multi_resolution_data_raster.cached_data_reconstructed_cube_raster = boost::none;

		//qDebug() << "RasterLayerProxy: Rebuilding GLMultiResolutionStaticPolygonReconstructedRaster "
		//	<< (d_cached_multi_resolution_data_raster.cached_age_grid_mask_cube_raster ? "with" : "without")
		//	<< " an age grid.";

		// This handles age-grid masking both with and without reconstructing the raster (with polygons).
		GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type reconstructed_raster =
				!d_cached_multi_resolution_data_raster.cached_reconstructed_polygon_meshes.empty()
				? GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::create(
						renderer,
						reconstruction_time,
						d_cached_multi_resolution_data_raster.cached_data_cube_raster.get(),
						d_cached_multi_resolution_data_raster.cached_reconstructed_polygon_meshes,
						d_cached_multi_resolution_data_raster.cached_age_grid_mask_cube_raster)
				: GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::create(
						renderer,
						reconstruction_time,
						d_cached_multi_resolution_data_raster.cached_data_cube_raster.get(),
						d_cached_multi_resolution_data_raster.cached_multi_resolution_cube_mesh.get(),
						d_cached_multi_resolution_data_raster.cached_age_grid_mask_cube_raster);

		d_cached_multi_resolution_data_raster.cached_data_reconstructed_raster = reconstructed_raster;
	}

	// Notify the reconstructed raster of the current reconstruction time.
	d_cached_multi_resolution_data_raster.cached_data_reconstructed_raster.get()->update(reconstruction_time);

	// Return the *reconstructed* raster.
	return GPlatesOpenGL::GLMultiResolutionRasterInterface::non_null_ptr_type(
		d_cached_multi_resolution_data_raster.cached_data_reconstructed_raster.get());
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRasterInterface::non_null_ptr_type>
GPlatesAppLogic::RasterLayerProxy::get_multi_resolution_data_cube_raster(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &reconstruction_time,
		const GPlatesPropertyValues::TextContent &raster_band_name)
{
	// Get the *unreconstructed* or *reconstructed* input into the cube raster.
	boost::optional<GPlatesOpenGL::GLMultiResolutionRasterInterface::non_null_ptr_type> data_raster =
			get_multi_resolution_data_raster(renderer, reconstruction_time, raster_band_name);
	if (!data_raster)
	{
		return boost::none;
	}

	// See if it's a reconstructed raster or not.
	if (d_cached_multi_resolution_data_raster.cached_data_raster &&
		d_cached_multi_resolution_data_raster.cached_data_raster.get() == data_raster.get())
	{
		// It's an *unreconstructed* raster.

		// Rebuild the multi-resolution cube raster if necessary.
		if (!d_cached_multi_resolution_data_raster.cached_data_cube_raster)
		{
			// NOTE: We also invalidate the multi-resolution *reconstructed* raster since it must link
			// to the multi-resolution cube raster and hence must also be rebuilt.
			d_cached_multi_resolution_data_raster.cached_data_reconstructed_raster = boost::none;

			// Create the multi-resolution cube raster.
			GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type cube_raster =
					GPlatesOpenGL::GLMultiResolutionCubeRaster::create(
							renderer,
							d_cached_multi_resolution_data_raster.cached_data_raster.get());

			d_cached_multi_resolution_data_raster.cached_data_cube_raster = cube_raster;
		}

		return GPlatesOpenGL::GLMultiResolutionCubeRasterInterface::non_null_ptr_type(
				d_cached_multi_resolution_data_raster.cached_data_cube_raster.get());
	}

	// It's a *reconstructed* raster.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_cached_multi_resolution_data_raster.cached_data_reconstructed_raster &&
				d_cached_multi_resolution_data_raster.cached_data_reconstructed_raster.get() == data_raster.get(),
			GPLATES_ASSERTION_SOURCE);

	// Rebuild the multi-resolution *reconstructed* cube raster if necessary.
	if (!d_cached_multi_resolution_data_raster.cached_data_reconstructed_cube_raster)
	{
		GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::non_null_ptr_type cube_reconstructed_raster =
				GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::create(
						renderer,
						d_cached_multi_resolution_data_raster.cached_data_reconstructed_raster.get());

		d_cached_multi_resolution_data_raster.cached_data_reconstructed_cube_raster = cube_reconstructed_raster;
	}

	return GPlatesOpenGL::GLMultiResolutionCubeRasterInterface::non_null_ptr_type(
			d_cached_multi_resolution_data_raster.cached_data_reconstructed_cube_raster.get());
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type>
GPlatesAppLogic::RasterLayerProxy::get_multi_resolution_age_grid_mask(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &reconstruction_time,
		const GPlatesPropertyValues::TextContent &raster_band_name)
{
	if (!d_current_georeferencing)
	{
		// We need georeferencing information to have a multi-resolution raster.
		return boost::none;
	}

	// Get the proxied raster for the present day and the specified band name.
	// NOTE: The reconstruction time specified by the caller is used to generate the age *mask*
	// but not used to look up the proxied rasters (since the age grid itself is always present day).
	const boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> proxied_raster =
			get_proxied_raster(0/*present-day*/, raster_band_name);
	if (!proxied_raster)
	{
		return boost::none;
	}

	// The raster type is expected to contain numerical data, not colour RGBA data, because it's an age grid.
	if (!GPlatesPropertyValues::RawRasterUtils::does_raster_contain_numerical_data(*proxied_raster.get()))
	{
		qWarning() << "RasterLayerProxy::get_multi_resolution_age_grid_mask: "
				"Raster does not contain numerical data (contains colours instead).";
		return boost::none;
	}

	// Rebuild the age grid mask source if necessary.
	if (!d_cached_multi_resolution_age_grid_raster.cached_age_grid_mask_source)
	{
		d_cached_multi_resolution_age_grid_raster.cached_age_grid_mask_raster = boost::none;

		// Use a GLDataRasterSource if requested, otherwise a GLAgeGridMaskSource.
		if (d_cached_multi_resolution_age_grid_raster.use_age_grid_data_source(renderer))
		{
			//qDebug() << "RasterLayerProxy: Rebuilding age grid GLDataRasterSource.";
			boost::optional<GPlatesOpenGL::GLDataRasterSource::non_null_ptr_type> source =
					GPlatesOpenGL::GLDataRasterSource::create(renderer, proxied_raster.get());
			if (source)
			{
				d_cached_multi_resolution_age_grid_raster.cached_age_grid_mask_source =
						GPlatesOpenGL::GLMultiResolutionRasterSource::non_null_ptr_type(source.get());
			}
		}
		else // use a GLAgeGridMaskSource...
		{
			//qDebug() << "RasterLayerProxy: Rebuilding GLAgeGridMaskSource.";
			boost::optional<GPlatesOpenGL::GLAgeGridMaskSource::non_null_ptr_type> source =
					GPlatesOpenGL::GLAgeGridMaskSource::create(
							renderer,
							reconstruction_time,
							proxied_raster.get());
			if (source)
			{
				d_cached_multi_resolution_age_grid_raster.cached_age_grid_mask_source =
						GPlatesOpenGL::GLMultiResolutionRasterSource::non_null_ptr_type(source.get());
			}
		}

		if (!d_cached_multi_resolution_age_grid_raster.cached_age_grid_mask_source)
		{
			// Unable to get age grid mask source so nothing we can do.
			qWarning() << "RasterLayerProxy::get_multi_resolution_age_grid_mask: "
					"Failed to create age grid mask source.";
			return boost::none;
		}
	}

	// Update the age grid mask if the reconstruction time has changed.
	if (d_cached_multi_resolution_age_grid_raster.cached_age_grid_reconstruction_time != GPlatesMaths::real_t(reconstruction_time))
	{
		d_cached_multi_resolution_age_grid_raster.cached_age_grid_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);

		// This only needs to be done for GLAgeGridMaskSource (not GLDataRasterSource).
		if (!d_cached_multi_resolution_age_grid_raster.use_age_grid_data_source(renderer))
		{
			// Update the reconstruction time for the age grid mask.
			GPlatesUtils::dynamic_pointer_cast<GPlatesOpenGL::GLAgeGridMaskSource>(
					d_cached_multi_resolution_age_grid_raster.cached_age_grid_mask_source.get())
							->update_reconstruction_time(reconstruction_time);
		}
	}

	// Rebuild the age grid mask raster if necessary.
	if (!d_cached_multi_resolution_age_grid_raster.cached_age_grid_mask_raster)
	{
		//qDebug() << "RasterLayerProxy: Rebuilding age grid mask GLMultiResolutionRaster.";

		// Create the age grid mask multi-resolution raster.
		//
		// NOTE: The age grid can be used for visualisation *and* quantitative analysis.
		// This is because it is used to assist reconstruction of a raster in another layer and
		// that raster could be visualised or analysis (eg, raster co-registration).
		// The visual case does not require caching of the entire raster but the analysis case
		// can benefit from it - see 'get_multi_resolution_data_raster()' for more details.
		// So we allow caching of the entire raster because since it satisfies both cases albeit at
		// the expense of excess memory usage when only visualisation is used.
		const GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type age_grid_mask_raster =
				GPlatesOpenGL::GLMultiResolutionRaster::create(
						renderer,
						d_current_georeferencing.get(),
						d_current_coordinate_transformation,
						d_cached_multi_resolution_age_grid_raster.cached_age_grid_mask_source.get(),
						// Avoids blending seams due to anisotropic filtering which gives age grid
						// coverage alpha values that are not either 0.0 or 1.0...
						GPlatesOpenGL::GLMultiResolutionRaster::FIXED_POINT_TEXTURE_FILTER_NO_ANISOTROPIC,
						// Our source GLAgeGridMaskSource has caching that insulates us from the file
						// system but it doesn't cache the entire level-of-detail pyramid so we
						// rely on the multi-resolution age grid mask for that...
						GPlatesOpenGL::GLMultiResolutionRaster::CACHE_TILE_TEXTURES_ENTIRE_LEVEL_OF_DETAIL_PYRAMID);

		d_cached_multi_resolution_age_grid_raster.cached_age_grid_mask_raster = age_grid_mask_raster;
	}

	// Rebuild the age grid mask cube raster if necessary.
	if (!d_cached_multi_resolution_age_grid_raster.cached_age_grid_mask_cube_raster)
	{
		//qDebug() << "RasterLayerProxy: Rebuilding age grid mask GLMultiResolutionCubeRaster.";

		// Create the age grid mask multi-resolution cube raster.
		const GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type age_grid_mask_cube_raster =
				GPlatesOpenGL::GLMultiResolutionCubeRaster::create(
						renderer,
						d_cached_multi_resolution_age_grid_raster.cached_age_grid_mask_raster.get(),
						GPlatesOpenGL::GLMultiResolutionCubeRaster::DEFAULT_TILE_TEXEL_DIMENSION,
						true/*adapt_tile_dimension_to_source_resolution*/,
						// Avoids blending seams due to bilinear and/or anisotropic filtering which
						// gives age grid mask alpha values that are not either 0.0 or 1.0.
						GPlatesOpenGL::GLMultiResolutionCubeRaster::FIXED_POINT_TEXTURE_FILTER_MAG_NEAREST);

		d_cached_multi_resolution_age_grid_raster.cached_age_grid_mask_cube_raster = age_grid_mask_cube_raster;
	}

	return d_cached_multi_resolution_age_grid_raster.cached_age_grid_mask_cube_raster.get();
}


const GPlatesUtils::SubjectToken &
GPlatesAppLogic::RasterLayerProxy::get_subject_token()
{
	// We've checked to see if any inputs have changed except the layer proxy inputs.
	// This is because we get notified of all changes to input except input layer proxies which
	// we have to poll to see if they changed since we last accessed them - so we do that now.

	// See if the reconstructed polygons layer proxies have changed.
	BOOST_FOREACH(
			LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &reconstructed_polygons_layer_proxy,
			d_current_reconstructed_polygons_layer_proxies.get_input_layer_proxies())
	{
		if (!reconstructed_polygons_layer_proxy.is_up_to_date())
		{
			// This raster layer proxy is now invalid.
			d_subject_token.invalidate();

			// We're now up-to-date with the reconstructed polygons layer proxy.
			reconstructed_polygons_layer_proxy.set_up_to_date();
		}
	}

	// See if the age grid raster layer proxy has changed.
	if (d_current_age_grid_raster_layer_proxy &&
		// Avoid cyclic dependency is age grid layer is this layer...
		d_current_age_grid_raster_layer_proxy.get_input_layer_proxy().get() != this)
	{
		if (!d_current_age_grid_raster_layer_proxy.is_up_to_date())
		{
			// This raster layer proxy is now invalid.
			d_subject_token.invalidate();

			// We're now up-to-date with the age grid raster layer proxy.
			d_current_age_grid_raster_layer_proxy.set_up_to_date();
		}
	}

	// See if the normal map raster layer proxy has changed.
	if (d_current_normal_map_raster_layer_proxy &&
		// Avoid cyclic dependency is normal map layer is this layer...
		d_current_normal_map_raster_layer_proxy.get_input_layer_proxy().get() != this)
	{
		if (!d_current_normal_map_raster_layer_proxy.is_up_to_date())
		{
			// This raster layer proxy is now invalid.
			d_subject_token.invalidate();

			// We're now up-to-date with the normal map raster layer proxy.
			d_current_normal_map_raster_layer_proxy.set_up_to_date();
		}
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
GPlatesAppLogic::RasterLayerProxy::add_current_reconstructed_polygons_layer_proxy(
		const ReconstructLayerProxy::non_null_ptr_type &reconstructed_polygons_layer_proxy)
{
	d_current_reconstructed_polygons_layer_proxies.add_input_layer_proxy(reconstructed_polygons_layer_proxy);

	// This raster layer proxy has now changed.
	invalidate();
}


void
GPlatesAppLogic::RasterLayerProxy::remove_current_reconstructed_polygons_layer_proxy(
		const ReconstructLayerProxy::non_null_ptr_type &reconstructed_polygons_layer_proxy)
{
	d_current_reconstructed_polygons_layer_proxies.remove_input_layer_proxy(reconstructed_polygons_layer_proxy);

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
GPlatesAppLogic::RasterLayerProxy::set_current_normal_map_raster_layer_proxy(
		boost::optional<RasterLayerProxy::non_null_ptr_type> normal_map_raster_layer_proxy)
{
	d_current_normal_map_raster_layer_proxy.set_input_layer_proxy(normal_map_raster_layer_proxy);

	// This raster layer proxy has now changed.
	invalidate();
}


void
GPlatesAppLogic::RasterLayerProxy::set_current_raster_feature(
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> raster_feature,
		const RasterLayerParams &raster_params)
{
	d_current_raster_feature = raster_feature;

	set_raster_params(raster_params);

	// The raster feature has changed.
	invalidate_raster_feature();
}


void
GPlatesAppLogic::RasterLayerProxy::set_current_raster_band_name(
		const RasterLayerParams &raster_params)
{
	set_raster_params(raster_params);

	// The raster feature has changed.
	invalidate_raster_feature();
}


void
GPlatesAppLogic::RasterLayerProxy::modified_raster_feature(
		const RasterLayerParams &raster_params)
{
	set_raster_params(raster_params);

	// The raster feature has changed.
	invalidate_raster_feature();
}


void
GPlatesAppLogic::RasterLayerProxy::set_raster_params(
		const RasterLayerParams &raster_params)
{
	d_current_raster_band_name = raster_params.get_band_name();
	d_current_raster_band_names = raster_params.get_band_names();
	d_current_georeferencing = raster_params.get_georeferencing();
	d_current_spatial_reference_system = raster_params.get_spatial_reference_system();

	// Update the coordinate transformation if the raster has a spatial reference system,
	// otherwise revert to identity transformation.
	if (d_current_spatial_reference_system)
	{
		boost::optional<GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_type>
				coordinate_transformation = GPlatesPropertyValues::CoordinateTransformation::create(
						d_current_spatial_reference_system.get());
		if (coordinate_transformation)
		{
			d_current_coordinate_transformation = coordinate_transformation.get();
		}
		else
		{
			// Identity transformation.
			d_current_coordinate_transformation = GPlatesPropertyValues::CoordinateTransformation::create();
		}
	}
	else
	{
		// Identity transformation.
		d_current_coordinate_transformation = GPlatesPropertyValues::CoordinateTransformation::create();
	}
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
	d_cached_resolved_raster_feature_properties.invalidate();

	// Invalidate the age grid mask and coverage.
	// NOTE: The age grid should not be a time-dependent raster since it's only accessed at
	// present day so it should actually only get invalidated if the raster feature changes.
	// But we invalidate it here since the age grid accesses it as a proxy with a raster band name.
	d_cached_multi_resolution_age_grid_raster.invalidate();

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
	d_cached_multi_resolution_data_raster.invalidate();

	// This raster layer proxy has changed in some way.
	d_subject_token.invalidate();
}


bool
GPlatesAppLogic::RasterLayerProxy::resolve_raster_feature(
		const double &reconstruction_time,
		const GPlatesPropertyValues::TextContent &raster_band_name)
{
	if (!d_current_raster_feature ||
		!d_current_raster_feature->is_valid())
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
			find_raster_band_name(d_current_raster_band_names, raster_band_name);
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
	if (visitor.get_proxied_rasters() != d_cached_resolved_raster_feature_properties.cached_proxied_rasters ||
		visitor.get_proxied_rasters().get()[band_name_index] != d_cached_resolved_raster_feature_properties.cached_proxied_raster)
	{
		invalidate_proxied_raster();
	}

	// Get the proxied raster using the selected band index.
	d_cached_resolved_raster_feature_properties.cached_proxied_raster = visitor.get_proxied_rasters().get()[band_name_index];

	// Get the list of proxied raw rasters.
	d_cached_resolved_raster_feature_properties.cached_proxied_rasters = visitor.get_proxied_rasters();

	return true;
}


bool
GPlatesAppLogic::RasterLayerProxy::MultiResolutionAgeGridRaster::use_age_grid_data_source(
		GPlatesOpenGL::GLRenderer &renderer) const
{
	// Find out which age grid source type to use if we haven't already.
	if (!d_use_age_grid_data_source)
	{
		d_use_age_grid_data_source =
				GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::supports_age_mask_generation(renderer);
	}

	return d_use_age_grid_data_source.get();
}
