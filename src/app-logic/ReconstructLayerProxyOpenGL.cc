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
// The OpenGL-backed part of ReconstructLayerProxy, split out of "ReconstructLayerProxy.cc"
// so that no translation unit compiled into the pyGPlates module includes the "opengl/"
// headers (pyGPlates never renders, and does not link OpenGL or GLEW).
//
// "ReconstructLayerProxy.h" only forward-declares the OpenGL classes, and the cached
// pointer is a shared_ptr, so everything outside this file compiles without them.
//

#include "ReconstructLayerProxy.h"

#include "ReconstructParams.h"

#include "maths/types.h"

#include "opengl/GLReconstructedStaticPolygonMeshes.h"

#include "utils/Profile.h"
#include "utils/ReferenceCount.h"


GPlatesGlobal::PointerTraits<GPlatesOpenGL::GLReconstructedStaticPolygonMeshes>::non_null_ptr_type
GPlatesAppLogic::ReconstructLayerProxy::get_reconstructed_static_polygon_meshes(
		GPlatesOpenGL::GLRenderer &renderer,
		bool reconstructing_with_age_grid)
{
	return get_reconstructed_static_polygon_meshes(
			renderer, reconstructing_with_age_grid, d_current_reconstruction_time);
}


GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::non_null_ptr_type
GPlatesAppLogic::ReconstructLayerProxy::get_reconstructed_static_polygon_meshes(
		GPlatesOpenGL::GLRenderer &renderer,
		bool reconstructing_with_age_grid,
		const double &reconstruction_time)
{
	PROFILE_FUNC();

	bool need_to_update = false;

	// Rebuild the GLReconstructedStaticPolygonMeshes object if necessary.
	// If it doesn't exist then either it has never been requested or it was invalidated
	// because the reconstructable feature collections have changed in some way causing the
	// present day polygon meshes to (possibly) change.
	if (!d_cached_reconstructed_polygon_meshes.cached_reconstructed_static_polygon_meshes)
	{
		GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::non_null_ptr_type reconstructed_polygon_meshes =
				GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::create(
						renderer,
						get_present_day_polygon_meshes(),
						get_present_day_geometries(),
						reconstruction_time,
						get_reconstructions_spatial_partition(reconstruction_time));
		d_cached_reconstructed_polygon_meshes.cached_reconstructed_static_polygon_meshes =
				GPlatesUtils::make_shared_from_intrusive(reconstructed_polygon_meshes);

		// Even though we just created the 'GLReconstructedStaticPolygonMeshes' we still need to
		// update it in case we're using age grids which need a reconstructions spatial partition
		// that ignores the active time periods of features.
		need_to_update = true;

		// We have taken measures to be up-to-date with respect to the reconstructed polygon geometries.
		get_subject_token().update_observer(
				d_cached_reconstructed_polygon_meshes.cached_reconstructed_polygons_observer_token);
	}

	// Update if the reconstruction time has changed...
	if (d_cached_reconstructed_polygon_meshes.cached_reconstruction_time != GPlatesMaths::real_t(reconstruction_time))
	{
		need_to_update = true;

		d_cached_reconstructed_polygon_meshes.cached_reconstruction_time = GPlatesMaths::real_t(reconstruction_time);
	}

	// Update if we're changing decision to reconstruct with an age grid...
	if (d_cached_reconstructed_polygon_meshes.cached_reconstructing_with_age_grid != reconstructing_with_age_grid)
	{
		need_to_update = true;

		d_cached_reconstructed_polygon_meshes.cached_reconstructing_with_age_grid = reconstructing_with_age_grid;
	}

	// Update if we're not up-to-date with respect to the reconstructed polygon geometries...
	if (!get_subject_token().is_observer_up_to_date(
			d_cached_reconstructed_polygon_meshes.cached_reconstructed_polygons_observer_token))
	{
		need_to_update = true;

		// We have taken measures to be up-to-date with respect to the reconstructed polygon geometries.
		get_subject_token().update_observer(
				d_cached_reconstructed_polygon_meshes.cached_reconstructed_polygons_observer_token);
	}

	if (need_to_update)
	{
		//
		// Update our cached reconstructed polygon meshes.
		//

		// The reconstructions spatial partition for *active* features.
		const GPlatesAppLogic::ReconstructLayerProxy::reconstructions_spatial_partition_type::non_null_ptr_to_const_type
				reconstructions_spatial_partition =
						get_reconstructions_spatial_partition(reconstruction_time);

		// The reconstructions spatial partition for *active* or *inactive* features.
		boost::optional<GPlatesAppLogic::ReconstructLayerProxy::reconstructions_spatial_partition_type::non_null_ptr_to_const_type>
						active_or_inactive_reconstructions_spatial_partition;
		// It's only needed if we've been asked to help reconstruct a raster with the aid of an age grid.
		if (reconstructing_with_age_grid)
		{
			// Use the same reconstruct params but specify that reconstructions should include *inactive* features also.
			GPlatesAppLogic::ReconstructParams reconstruct_params = get_current_reconstruct_params();
			reconstruct_params.set_reconstruct_by_plate_id_outside_active_time_period(true);

			// Get a new reconstructions spatial partition that includes *inactive* reconstructions.
			active_or_inactive_reconstructions_spatial_partition =
					get_reconstructions_spatial_partition(reconstruct_params, reconstruction_time);
		}

		d_cached_reconstructed_polygon_meshes.cached_reconstructed_static_polygon_meshes->update(
				reconstruction_time,
				reconstructions_spatial_partition,
				active_or_inactive_reconstructions_spatial_partition);
	}

	return GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::non_null_ptr_type(
			d_cached_reconstructed_polygon_meshes.cached_reconstructed_static_polygon_meshes.get());
}
