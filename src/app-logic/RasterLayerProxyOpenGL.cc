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
// The OpenGL-backed part of RasterLayerProxy, split out of "RasterLayerProxy.cc" so that
// no translation unit compiled into the pyGPlates module includes the "opengl/" headers
// (pyGPlates never renders, and does not link OpenGL or GLEW).
//
// "RasterLayerProxy.h" only forward-declares the OpenGL classes, and the OpenGL caches
// live behind opaque OpenGLCache pointers that are only created here, so everything
// outside this file compiles without them.
//

#include <vector>
#include <boost/optional.hpp>
#include <QDebug>

#include "RasterLayerProxy.h"

#include "ReconstructLayerProxy.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/types.h"

#include "opengl/GLAgeGridMaskSource.h"
#include "opengl/GLDataRasterSource.h"
#include "opengl/GLMultiResolutionCubeMesh.h"
#include "opengl/GLMultiResolutionCubeRaster.h"
#include "opengl/GLMultiResolutionCubeRasterInterface.h"
#include "opengl/GLMultiResolutionCubeReconstructedRaster.h"
#include "opengl/GLMultiResolutionRaster.h"
#include "opengl/GLMultiResolutionRasterInterface.h"
#include "opengl/GLMultiResolutionStaticPolygonReconstructedRaster.h"
#include "opengl/GLReconstructedStaticPolygonMeshes.h"

#include "property-values/RawRasterUtils.h"

#include "utils/SubjectObserverToken.h"


/**
 * The cached OpenGL multi-resolution *data* raster (and its raster data source).
 *
 * Defined here (rather than in "RasterLayerProxy.h") because its members hold OpenGL
 * objects - see @a OpenGLCache in the header.
 */
struct GPlatesAppLogic::RasterLayerProxy::MultiResolutionDataRaster :
		public GPlatesAppLogic::RasterLayerProxy::OpenGLCache
{
	virtual
	void
	invalidate()
	{
		// NOTE: We don't actually clear the OpenGL multi-resolution (unreconstructed) *data* raster
		// because it has its own observer token so it can track when it needs to be rebuilt.
		// Allows it to more efficiently rebuild in the presence of time-dependent rasters.

		// We do however invalidate the reconstructed raster since it depends on other layers such as
		// the reconstructed polygons layer and the age grid layer.
		cached_data_reconstructed_raster = boost::none;

		// Invalidate structures from other layers used to reconstruct the raster.
		cached_reconstructed_polygon_meshes.clear();
		cached_age_grid_mask_cube_raster = boost::none;
	}

	/**
	 * Determines when/if the multi-resolution raster should be rebuilt because out-of-date.
	 *
	 * NOTE: Allows more efficient rebuilds in the presence of time-dependent rasters.
	 */
	GPlatesUtils::ObserverToken cached_proxied_raster_observer;

	/**
	 * Cached OpenGL raster data source (for the currently cached proxied raster).
	 *
	 * NOTE: If raster is RGBA (ie, not numerical data) then it is never cached.
	 * This is application logic level data that has nothing to do with visualisation (ie, colour).
	 */
	boost::optional<GPlatesOpenGL::GLDataRasterSource::non_null_ptr_type> cached_data_raster_source;

	/**
	 * Cached OpenGL (unreconstructed) multi-resolution *data* raster (for the currently cached proxied raster).
	 */
	boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type> cached_data_raster;

	/**
	 * Cached OpenGL multi-resolution cube *data* raster (for the currently cached proxied raster).
	 */
	boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type> cached_data_cube_raster;

	/**
	 * Cached OpenGL reconstructed polygon meshes (from other layers) for reconstructing the raster.
	 */
	std::vector<GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::non_null_ptr_type>
			cached_reconstructed_polygon_meshes;

	/**
	 * Mesh that used when *not* reconstructing raster (but still using age grid).
	 *
	 * This is constant so could be shared by multiple layers if uses a lot of memory.
	 */
	boost::optional<GPlatesOpenGL::GLMultiResolutionCubeMesh::non_null_ptr_to_const_type>
			cached_multi_resolution_cube_mesh;

	/**
	 * Cached OpenGL age grid mask (from another layer) for reconstructing the raster.
	 *
	 * NOTE: This is different than the age grid in @a MultiResolutionAgeGridRaster.
	 * Here the age grid refers to *another* layer (not this layer).
	 */
	boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type> cached_age_grid_mask_cube_raster;

	/**
	 * Cached OpenGL (reconstructed) multi-resolution *data* raster (for the currently cached proxied raster).
	 *
	 * This is only valid if we are currently connected to a reconstructed polygons layer.
	 */
	boost::optional<GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type>
			cached_data_reconstructed_raster;

	/**
	 * Cached OpenGL (reconstructed) multi-resolution cube *data* raster (for the currently cached proxied raster).
	 */
	boost::optional<GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::non_null_ptr_type>
			cached_data_reconstructed_cube_raster;
};


/**
 * The cached OpenGL multi-resolution *age grid* raster.
 *
 * Defined here (rather than in "RasterLayerProxy.h") because its members hold OpenGL
 * objects - see @a OpenGLCache in the header.
 */
struct GPlatesAppLogic::RasterLayerProxy::MultiResolutionAgeGridRaster :
		public GPlatesAppLogic::RasterLayerProxy::OpenGLCache
{
	virtual
	void
	invalidate()
	{
		cached_age_grid_mask_source = boost::none;
		cached_age_grid_mask_raster = boost::none;
		cached_age_grid_mask_cube_raster = boost::none;
		cached_age_grid_reconstruction_time = boost::none;
	}

	/**
	 * Cached OpenGL age grid mask source (for the currently cached proxied raster).
	 */
	boost::optional<GPlatesOpenGL::GLMultiResolutionRasterSource::non_null_ptr_type> cached_age_grid_mask_source;

	/**
	 * Cached OpenGL multi-resolution age grid mask (for the currently cached proxied raster).
	 */
	boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type> cached_age_grid_mask_raster;

	/**
	 * Cached OpenGL multi-resolution age grid mask cube raster (for the currently cached proxied raster).
	 */
	boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type> cached_age_grid_mask_cube_raster;

	/**
	 * The reconstruction time of the cached age grid.
	 */
	boost::optional<GPlatesMaths::real_t> cached_age_grid_reconstruction_time;

	/**
	 * If returns true then use a floating-point raster containing actual age values instead
	 * of a fixed-point raster containing age masks (results of age comparisons against
	 * a specific reconstruction time).
	 */
	bool
	use_age_grid_data_source(
			GPlatesOpenGL::GLRenderer &renderer) const;

private:
	/**
	 * If true then use a GLDataRasterSource for age grid (instead of GLAgeGridMaskSource).
	 */
	mutable boost::optional<bool> d_use_age_grid_data_source;
};


boost::optional<GPlatesOpenGL::GLMultiResolutionRasterInterface::non_null_ptr_type>
GPlatesAppLogic::RasterLayerProxy::get_multi_resolution_data_raster(
		GPlatesOpenGL::GLRenderer &renderer)
{
	return get_multi_resolution_data_raster(renderer, d_current_reconstruction_time, d_current_raster_band_name);
}


boost::optional<GPlatesOpenGL::GLMultiResolutionRasterInterface::non_null_ptr_type>
GPlatesAppLogic::RasterLayerProxy::get_multi_resolution_data_raster(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesPropertyValues::TextContent &raster_band_name)
{
	return get_multi_resolution_data_raster(renderer, d_current_reconstruction_time, raster_band_name);
}


boost::optional<GPlatesOpenGL::GLMultiResolutionRasterInterface::non_null_ptr_type>
GPlatesAppLogic::RasterLayerProxy::get_multi_resolution_data_raster(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &reconstruction_time)
{
	return get_multi_resolution_data_raster(renderer, reconstruction_time, d_current_raster_band_name);
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRasterInterface::non_null_ptr_type>
GPlatesAppLogic::RasterLayerProxy::get_multi_resolution_data_cube_raster(
		GPlatesOpenGL::GLRenderer &renderer)
{
	return get_multi_resolution_data_cube_raster(renderer, d_current_reconstruction_time, d_current_raster_band_name);
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRasterInterface::non_null_ptr_type>
GPlatesAppLogic::RasterLayerProxy::get_multi_resolution_data_cube_raster(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesPropertyValues::TextContent &raster_band_name)
{
	return get_multi_resolution_data_cube_raster(renderer, d_current_reconstruction_time, raster_band_name);
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRasterInterface::non_null_ptr_type>
GPlatesAppLogic::RasterLayerProxy::get_multi_resolution_data_cube_raster(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &reconstruction_time)
{
	return get_multi_resolution_data_cube_raster(renderer, reconstruction_time, d_current_raster_band_name);
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type>
GPlatesAppLogic::RasterLayerProxy::get_multi_resolution_age_grid_mask(
		GPlatesOpenGL::GLRenderer &renderer)
{
	return get_multi_resolution_age_grid_mask(
			renderer, d_current_reconstruction_time, d_current_raster_band_name);
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type>
GPlatesAppLogic::RasterLayerProxy::get_multi_resolution_age_grid_mask(
		GPlatesOpenGL::GLRenderer &renderer,
		const GPlatesPropertyValues::TextContent &raster_band_name)
{
	return get_multi_resolution_age_grid_mask(
			renderer, d_current_reconstruction_time, raster_band_name);
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type>
GPlatesAppLogic::RasterLayerProxy::get_multi_resolution_age_grid_mask(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &reconstruction_time)
{
	return get_multi_resolution_age_grid_mask(
			renderer, reconstruction_time, d_current_raster_band_name);
}


boost::optional<GPlatesOpenGL::GLMultiResolutionRasterInterface::non_null_ptr_type>
GPlatesAppLogic::RasterLayerProxy::get_multi_resolution_data_raster(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &reconstruction_time,
		const GPlatesPropertyValues::TextContent &raster_band_name)
{
	// Create the OpenGL cache the first time it is used.
	// (pyGPlates does not compile this file, so there the pointer stays null.)
	if (!d_cached_multi_resolution_data_raster)
	{
		d_cached_multi_resolution_data_raster.reset(new MultiResolutionDataRaster());
	}
	MultiResolutionDataRaster &data_raster_cache =
			static_cast<MultiResolutionDataRaster &>(*d_cached_multi_resolution_data_raster);

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
		data_raster_cache.cached_proxied_raster_observer))
	{
		// If we have a data raster source then attempt to change the raster first
		// since it's cheaper than rebuilding the multi-resolution raster.
		if (data_raster_cache.cached_data_raster_source)
		{
			if (!data_raster_cache.cached_data_raster_source.get()->change_raster(
					renderer, proxied_raster.get()))
			{
				// The raster dimensions have probably changed - we'll need to rebuild.
				data_raster_cache.cached_data_raster_source = boost::none;
			}
		}

		// We have taken measures to be up-to-date with respect to the proxied raster.
		d_proxied_raster_subject_token.update_observer(
				data_raster_cache.cached_proxied_raster_observer);
	}

	// Rebuild the data raster source if necessary.
	if (!data_raster_cache.cached_data_raster_source)
	{
		// NOTE: We also invalidate the multi-resolution raster since it must link
		// to the data raster source and hence must also be rebuilt.
		data_raster_cache.cached_data_raster = boost::none;

		//qDebug() << "RasterLayerProxy: Rebuilding GLDataRasterSource.";

		boost::optional<GPlatesOpenGL::GLDataRasterSource::non_null_ptr_type> data_raster_source =
				GPlatesOpenGL::GLDataRasterSource::create(renderer, proxied_raster.get());

		data_raster_cache.cached_data_raster_source = data_raster_source;
		if (!data_raster_cache.cached_data_raster_source)
		{
			// Unable to create a data raster source so nothing we can do.
			// This can happen if the raster does not contain numerical data (ie, contains RGBA data).
			qWarning() << "RasterLayerProxy::get_multi_resolution_data_raster: Failed to create raster data source.";
			return boost::none;
		}
	}

	// Rebuild the multi-resolution raster if necessary.
	if (!data_raster_cache.cached_data_raster)
	{
		// NOTE: We also invalidate the multi-resolution cube raster since it must link to the
		// multi-resolution raster and hence must also be rebuilt (if raster is reconstructed).
		data_raster_cache.cached_data_cube_raster = boost::none;

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
						data_raster_cache.cached_data_raster_source.get(),
						GPlatesOpenGL::GLMultiResolutionRaster::DEFAULT_FIXED_POINT_TEXTURE_FILTER,
						GPlatesOpenGL::GLMultiResolutionRaster::CACHE_TILE_TEXTURES_ENTIRE_LEVEL_OF_DETAIL_PYRAMID);

		data_raster_cache.cached_data_raster = multi_resolution_raster;
	}

	// If we are not currently connected to any reconstructed polygons *and* we are not using an age grid
	// then just return the *unreconstructed* raster.
	// Note that we don't require reconstructed polygons to continue past this point.
	if (d_current_reconstructed_polygons_layer_proxies.empty() &&
		!d_current_age_grid_raster_layer_proxy)
	{
		return GPlatesOpenGL::GLMultiResolutionRasterInterface::non_null_ptr_type(
			data_raster_cache.cached_data_raster.get());
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
	if (data_raster_cache.cached_age_grid_mask_cube_raster != age_grid_mask_cube_raster)
	{
		data_raster_cache.cached_age_grid_mask_cube_raster = age_grid_mask_cube_raster;

		// We need to rebuild the reconstructed raster.
		data_raster_cache.cached_data_reconstructed_raster = boost::none;
	}

	// If we have an age grid raster then we are reconstructing with an age grid.
	const bool reconstructing_with_age_grid = static_cast<bool>(
			data_raster_cache.cached_age_grid_mask_cube_raster);

	// Get the reconstructed polygon meshes from the layers containing the reconstructed polygons.
	std::vector<GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::non_null_ptr_type> reconstructed_polygon_meshes;
	if (!d_current_reconstructed_polygons_layer_proxies.empty())
	{
		BOOST_FOREACH(
				const LayerProxyUtils::InputLayerProxy<ReconstructLayerProxy> &reconstructed_polygons_layer_proxy,
				d_current_reconstructed_polygons_layer_proxies)
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
		if (!data_raster_cache.cached_multi_resolution_cube_mesh)
		{
			data_raster_cache.cached_multi_resolution_cube_mesh =
					GPlatesOpenGL::GLMultiResolutionCubeMesh::create(renderer);
		}
	}

	// If reconstructed polygon meshes are different objects then they must have been rebuilt by
	// the reconstructed polygons layers since last we accessed them.
	// Note that changes *within* a GLReconstructedStaticPolygonMeshes object are detected and
	// handled by the reconstructed raster so we don't need to worry about that.
	if (data_raster_cache.cached_reconstructed_polygon_meshes != reconstructed_polygon_meshes)
	{
		data_raster_cache.cached_reconstructed_polygon_meshes = reconstructed_polygon_meshes;

		// We need to rebuild the reconstructed raster.
		data_raster_cache.cached_data_reconstructed_raster = boost::none;
	}

	// Rebuild the multi-resolution cube raster if necessary.
	if (!data_raster_cache.cached_data_cube_raster)
	{
		// NOTE: We also invalidate the multi-resolution *reconstructed* raster since it must link
		// to the multi-resolution cube raster and hence must also be rebuilt.
		data_raster_cache.cached_data_reconstructed_raster = boost::none;

		//qDebug() << "RasterLayerProxy: Rebuilding GLMultiResolutionCubeRaster.";

		// Create the multi-resolution cube raster.
		GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type cube_raster =
				GPlatesOpenGL::GLMultiResolutionCubeRaster::create(
						renderer,
						data_raster_cache.cached_data_raster.get());

		data_raster_cache.cached_data_cube_raster = cube_raster;
	}

	if (!data_raster_cache.cached_data_reconstructed_raster)
	{
		// NOTE: We also invalidate the *reconstructed* multi-resolution cube raster since it must link
		// to the multi-resolution *reconstructed* raster and hence must also be rebuilt.
		data_raster_cache.cached_data_reconstructed_cube_raster = boost::none;

		//qDebug() << "RasterLayerProxy: Rebuilding GLMultiResolutionStaticPolygonReconstructedRaster "
		//	<< (data_raster_cache.cached_age_grid_mask_cube_raster ? "with" : "without")
		//	<< " an age grid.";

		// This handles age-grid masking both with and without reconstructing the raster (with polygons).
		GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type reconstructed_raster =
				!data_raster_cache.cached_reconstructed_polygon_meshes.empty()
				? GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::create(
						renderer,
						reconstruction_time,
						data_raster_cache.cached_data_cube_raster.get(),
						data_raster_cache.cached_reconstructed_polygon_meshes,
						data_raster_cache.cached_age_grid_mask_cube_raster)
				: GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::create(
						renderer,
						reconstruction_time,
						data_raster_cache.cached_data_cube_raster.get(),
						data_raster_cache.cached_multi_resolution_cube_mesh.get(),
						data_raster_cache.cached_age_grid_mask_cube_raster);

		data_raster_cache.cached_data_reconstructed_raster = reconstructed_raster;
	}

	// Notify the reconstructed raster of the current reconstruction time.
	data_raster_cache.cached_data_reconstructed_raster.get()->update(reconstruction_time);

	// Return the *reconstructed* raster.
	return GPlatesOpenGL::GLMultiResolutionRasterInterface::non_null_ptr_type(
		data_raster_cache.cached_data_reconstructed_raster.get());
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRasterInterface::non_null_ptr_type>
GPlatesAppLogic::RasterLayerProxy::get_multi_resolution_data_cube_raster(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &reconstruction_time,
		const GPlatesPropertyValues::TextContent &raster_band_name)
{
	// Create the OpenGL cache the first time it is used.
	// (pyGPlates does not compile this file, so there the pointer stays null.)
	if (!d_cached_multi_resolution_data_raster)
	{
		d_cached_multi_resolution_data_raster.reset(new MultiResolutionDataRaster());
	}
	MultiResolutionDataRaster &data_raster_cache =
			static_cast<MultiResolutionDataRaster &>(*d_cached_multi_resolution_data_raster);

	// Get the *unreconstructed* or *reconstructed* input into the cube raster.
	boost::optional<GPlatesOpenGL::GLMultiResolutionRasterInterface::non_null_ptr_type> data_raster =
			get_multi_resolution_data_raster(renderer, reconstruction_time, raster_band_name);
	if (!data_raster)
	{
		return boost::none;
	}

	// See if it's a reconstructed raster or not.
	if (data_raster_cache.cached_data_raster &&
		data_raster_cache.cached_data_raster.get() == data_raster.get())
	{
		// It's an *unreconstructed* raster.

		// Rebuild the multi-resolution cube raster if necessary.
		if (!data_raster_cache.cached_data_cube_raster)
		{
			// NOTE: We also invalidate the multi-resolution *reconstructed* raster since it must link
			// to the multi-resolution cube raster and hence must also be rebuilt.
			data_raster_cache.cached_data_reconstructed_raster = boost::none;

			// Create the multi-resolution cube raster.
			GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type cube_raster =
					GPlatesOpenGL::GLMultiResolutionCubeRaster::create(
							renderer,
							data_raster_cache.cached_data_raster.get());

			data_raster_cache.cached_data_cube_raster = cube_raster;
		}

		return GPlatesOpenGL::GLMultiResolutionCubeRasterInterface::non_null_ptr_type(
				data_raster_cache.cached_data_cube_raster.get());
	}

	// It's a *reconstructed* raster.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			data_raster_cache.cached_data_reconstructed_raster &&
				data_raster_cache.cached_data_reconstructed_raster.get() == data_raster.get(),
			GPLATES_ASSERTION_SOURCE);

	// Rebuild the multi-resolution *reconstructed* cube raster if necessary.
	if (!data_raster_cache.cached_data_reconstructed_cube_raster)
	{
		GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::non_null_ptr_type cube_reconstructed_raster =
				GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::create(
						renderer,
						data_raster_cache.cached_data_reconstructed_raster.get());

		data_raster_cache.cached_data_reconstructed_cube_raster = cube_reconstructed_raster;
	}

	return GPlatesOpenGL::GLMultiResolutionCubeRasterInterface::non_null_ptr_type(
			data_raster_cache.cached_data_reconstructed_cube_raster.get());
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type>
GPlatesAppLogic::RasterLayerProxy::get_multi_resolution_age_grid_mask(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &reconstruction_time,
		const GPlatesPropertyValues::TextContent &raster_band_name)
{
	// Create the OpenGL cache the first time it is used.
	// (pyGPlates does not compile this file, so there the pointer stays null.)
	if (!d_cached_multi_resolution_age_grid_raster)
	{
		d_cached_multi_resolution_age_grid_raster.reset(new MultiResolutionAgeGridRaster());
	}
	MultiResolutionAgeGridRaster &age_grid_raster_cache =
			static_cast<MultiResolutionAgeGridRaster &>(*d_cached_multi_resolution_age_grid_raster);

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
	if (!age_grid_raster_cache.cached_age_grid_mask_source)
	{
		age_grid_raster_cache.cached_age_grid_mask_raster = boost::none;

		// Use a GLDataRasterSource if requested, otherwise a GLAgeGridMaskSource.
		if (age_grid_raster_cache.use_age_grid_data_source(renderer))
		{
			//qDebug() << "RasterLayerProxy: Rebuilding age grid GLDataRasterSource.";
			boost::optional<GPlatesOpenGL::GLDataRasterSource::non_null_ptr_type> source =
					GPlatesOpenGL::GLDataRasterSource::create(renderer, proxied_raster.get());
			if (source)
			{
				age_grid_raster_cache.cached_age_grid_mask_source =
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
				age_grid_raster_cache.cached_age_grid_mask_source =
						GPlatesOpenGL::GLMultiResolutionRasterSource::non_null_ptr_type(source.get());
			}
		}

		if (!age_grid_raster_cache.cached_age_grid_mask_source)
		{
			// Unable to get age grid mask source so nothing we can do.
			qWarning() << "RasterLayerProxy::get_multi_resolution_age_grid_mask: "
					"Failed to create age grid mask source.";
			return boost::none;
		}
	}

	// Update the age grid mask if the reconstruction time has changed.
	if (age_grid_raster_cache.cached_age_grid_reconstruction_time != GPlatesMaths::real_t(reconstruction_time))
	{
		age_grid_raster_cache.cached_age_grid_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);

		// This only needs to be done for GLAgeGridMaskSource (not GLDataRasterSource).
		if (!age_grid_raster_cache.use_age_grid_data_source(renderer))
		{
			// Update the reconstruction time for the age grid mask.
			GPlatesUtils::dynamic_pointer_cast<GPlatesOpenGL::GLAgeGridMaskSource>(
					age_grid_raster_cache.cached_age_grid_mask_source.get())
							->update_reconstruction_time(reconstruction_time);
		}
	}

	// Rebuild the age grid mask raster if necessary.
	if (!age_grid_raster_cache.cached_age_grid_mask_raster)
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
						age_grid_raster_cache.cached_age_grid_mask_source.get(),
						// Avoids blending seams due to anisotropic filtering which gives age grid
						// coverage alpha values that are not either 0.0 or 1.0...
						GPlatesOpenGL::GLMultiResolutionRaster::FIXED_POINT_TEXTURE_FILTER_NO_ANISOTROPIC,
						// Our source GLAgeGridMaskSource has caching that insulates us from the file
						// system but it doesn't cache the entire level-of-detail pyramid so we
						// rely on the multi-resolution age grid mask for that...
						GPlatesOpenGL::GLMultiResolutionRaster::CACHE_TILE_TEXTURES_ENTIRE_LEVEL_OF_DETAIL_PYRAMID);

		age_grid_raster_cache.cached_age_grid_mask_raster = age_grid_mask_raster;
	}

	// Rebuild the age grid mask cube raster if necessary.
	if (!age_grid_raster_cache.cached_age_grid_mask_cube_raster)
	{
		//qDebug() << "RasterLayerProxy: Rebuilding age grid mask GLMultiResolutionCubeRaster.";

		// Create the age grid mask multi-resolution cube raster.
		const GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type age_grid_mask_cube_raster =
				GPlatesOpenGL::GLMultiResolutionCubeRaster::create(
						renderer,
						age_grid_raster_cache.cached_age_grid_mask_raster.get(),
						GPlatesOpenGL::GLMultiResolutionCubeRaster::DEFAULT_TILE_TEXEL_DIMENSION,
						true/*adapt_tile_dimension_to_source_resolution*/,
						// Avoids blending seams due to bilinear and/or anisotropic filtering which
						// gives age grid mask alpha values that are not either 0.0 or 1.0.
						GPlatesOpenGL::GLMultiResolutionCubeRaster::FIXED_POINT_TEXTURE_FILTER_MAG_NEAREST);

		age_grid_raster_cache.cached_age_grid_mask_cube_raster = age_grid_mask_cube_raster;
	}

	return age_grid_raster_cache.cached_age_grid_mask_cube_raster.get();
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
