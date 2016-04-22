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

#ifndef GPLATES_APP_LOGIC_RASTERLAYERPROXY_H
#define GPLATES_APP_LOGIC_RASTERLAYERPROXY_H

#include <utility>
#include <vector>
#include <boost/optional.hpp>

#include "LayerProxy.h"
#include "LayerProxyUtils.h"
#include "RasterLayerTask.h"
#include "ReconstructLayerProxy.h"
#include "ResolvedRaster.h"

#include "maths/types.h"

#include "model/FeatureHandle.h"

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

#include "property-values/CoordinateTransformation.h"
#include "property-values/Georeferencing.h"
#include "property-values/GpmlRasterBandNames.h"
#include "property-values/RawRaster.h"
#include "property-values/SpatialReferenceSystem.h"
#include "property-values/TextContent.h"

#include "utils/SubjectObserverToken.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
}

namespace GPlatesAppLogic
{
	/**
	 * A layer proxy for resolving, and optionally reconstructing, a raster.
	 */
	class RasterLayerProxy :
			public LayerProxy
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a RasterLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<RasterLayerProxy> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a RasterLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<const RasterLayerProxy> non_null_ptr_to_const_type;


		/**
		 * Creates a @a RasterLayerProxy object.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new RasterLayerProxy());
		}


		/**
		 * Returns the georeferencing of the raster feature (if any).
		 */
		const boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> &
		get_georeferencing() const
		{
			return d_current_georeferencing;
		}


		/**
		 * Returns the raster's spatial reference system (if any).
		 */
		boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type>
		get_spatial_reference_system() const
		{
			return d_current_spatial_reference_system;
		}


		/**
		 * Returns the transform from the raster's spatial reference to the standard WGS84.
		 *
		 * The georeference coordinates undergo a coordinate system transformation from the raster's
		 * (possibly projected) geographic coordinate system to lat/lon coordinates in the standard
		 * WGS84 datum (which GPlates currently considers to be a perfectly spherical globe, ie,
		 * anything in WGS84 is not converted to spherical coordinates).
		 */
		GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_to_const_type
		get_coordinate_transformation() const
		{
			return d_current_coordinate_transformation;
		}


		/**
		 * Returns the name of the band selected for this raster.
		 */
		const GPlatesPropertyValues::TextContent &
		get_raster_band_name() const
		{
			return d_current_raster_band_name;
		}

		/**
		 * Sets the names of the bands of the raster.
		 */
		const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &
		get_raster_band_names() const
		{
			return d_current_raster_band_names;
		}


		/**
		 * Returns the proxied raw raster, for the current reconstruction time, of the band
		 * of the raster selected for processing.
		 */
		const boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &
		get_proxied_raster()
		{
			return get_proxied_raster(d_current_reconstruction_time, d_current_raster_band_name);
		}

		/**
		 * Returns the proxied raw raster, for the current reconstruction time and specified raster band name.
		 */
		const boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &
		get_proxied_raster(
				const GPlatesPropertyValues::TextContent &raster_band_name)
		{
			return get_proxied_raster(d_current_reconstruction_time, raster_band_name);
		}

		/**
		 * Returns the proxied raw raster, current raster band name at the specified time.
		 */
		const boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &
		get_proxied_raster(
				const double &reconstruction_time)
		{
			return get_proxied_raster(reconstruction_time, d_current_raster_band_name);
		}

		/**
		 * Returns the proxied raw raster, for the specified time and specified raster band name.
		 */
		const boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &
		get_proxied_raster(
				const double &reconstruction_time,
				const GPlatesPropertyValues::TextContent &raster_band_name);


		/**
		 * Returns the list of proxied rasters, for the current reconstruction time, for the raster bands.
		 */
		const boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> > &
		get_proxied_rasters()
		{
			return get_proxied_rasters(d_current_reconstruction_time);
		}

		/**
		 * Returns the list of proxied rasters, for the specified time, for the raster bands.
		 */
		const boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> > &
		get_proxied_rasters(
				const double &reconstruction_time);


		/**
		 * Returns the resolved raster for the current reconstruction time.
		 *
		 * This is currently (a derivation of @a ReconstructionGeometry) that just references this
		 * layer proxy and the optional age grid and reconstructed polygon layer proxies.
		 * An example client of @a ResolvedRaster is @a GLVisualLayers which is
		 * responsible for *visualising* the raster on the screen.
		 *
		 * Returns boost::none if there is no input raster feature connected or it cannot be resolved.
		 */
		boost::optional<ResolvedRaster::non_null_ptr_type>
		get_resolved_raster()
		{
			return get_resolved_raster(d_current_reconstruction_time);
		}

		/**
		 * Returns the resolved raster for the specified time.
		 *
		 * Returns boost::none if there is no input raster feature connected or it cannot be resolved.
		 */
		boost::optional<ResolvedRaster::non_null_ptr_type>
		get_resolved_raster(
				const double &reconstruction_time);


		/**
		 * Returns true if the raster (in the specified band) contains numerical data (such as
		 * floating-point or integer pixels, but not RGBA colour pixels).
		 *
		 * If this returns false then @a get_multi_resolution_data_raster will always return boost::none
		 * for the same raster band name.
		 */
		bool
		does_raster_band_contain_numerical_data(
				const GPlatesPropertyValues::TextContent &raster_band_name);


		/**
		 * Returns the possibly reconstructed (multi-resolution) *data* raster for the current
		 * reconstruction time and current raster band.
		 *
		 * This is used to render (possibly reconstructed) floating-point numerical raster data
		 * to a floating-point render target. The data can then either be processed on the GPU or
		 * read back to the CPU or both - the raster co-registration client actually does both.
		 *
		 * NOTE: Returns boost::none if the raster does not contain *numerical* data (see
		 * @a does_raster_band_contain_numerical_data). Also returns boost::none for various errors
		 * such as lack of OpenGL floating-point texture support on the runtime system.
		 * Raster *visualisation* is currently handled by @a get_resolved_raster in conjunction
		 * with "GLVisualLayers::render_raster()" - ie, handled at the visualisation tier
		 * because this is application logic code that knows nothing about presentation (nor should it).
		 *
		 * NOTE: We allow caching of the entire raster because, unlike visualisation where only
		 * a small region of the raster is typically visible (or it's zoomed out and only accessing
		 * a low-resolution mipmap), usually the entire raster can be accessed for data processing.
		 * And the present day raster (time-dependent rasters aside) is usually accessed repeatedly
		 * over many frames and you don't want to incur the large performance hit of continuously
		 * reloading tiles from disk (eg, raster co-registration data-mining front-end).
		 * In this case you should provide the user with an option to choose a lower level of detail
		 * (see the multi-resolution raster interface) and the user can judge when/if the memory usage
		 * is too high for their system (eg, if their hard drive starts to thrash).
		 */
		boost::optional<GPlatesOpenGL::GLMultiResolutionRasterInterface::non_null_ptr_type>
		get_multi_resolution_data_raster(
				GPlatesOpenGL::GLRenderer &renderer)
		{
			return get_multi_resolution_data_raster(renderer, d_current_reconstruction_time, d_current_raster_band_name);
		}

		/**
		 * Returns the possibly reconstructed (multi-resolution) *data* raster, for the current
		 * reconstruction time and specified raster band name.
		 */
		boost::optional<GPlatesOpenGL::GLMultiResolutionRasterInterface::non_null_ptr_type>
		get_multi_resolution_data_raster(
				GPlatesOpenGL::GLRenderer &renderer,
				const GPlatesPropertyValues::TextContent &raster_band_name)
		{
			return get_multi_resolution_data_raster(renderer, d_current_reconstruction_time, raster_band_name);
		}

		/**
		 * Returns the possibly reconstructed (multi-resolution) *data* raster, current raster
		 * band name at the specified time.
		 */
		boost::optional<GPlatesOpenGL::GLMultiResolutionRasterInterface::non_null_ptr_type>
		get_multi_resolution_data_raster(
				GPlatesOpenGL::GLRenderer &renderer,
				const double &reconstruction_time)
		{
			return get_multi_resolution_data_raster(renderer, reconstruction_time, d_current_raster_band_name);
		}

		/**
		 * Returns the possibly reconstructed (multi-resolution) *data* raster, for the specified
		 * time and specified raster band name.
		 */
		boost::optional<GPlatesOpenGL::GLMultiResolutionRasterInterface::non_null_ptr_type>
		get_multi_resolution_data_raster(
				GPlatesOpenGL::GLRenderer &renderer,
				const double &reconstruction_time,
				const GPlatesPropertyValues::TextContent &raster_band_name);


		/**
		 * This is the same as @a get_multi_resolution_data_raster but returns a *cube* version of the raster.
		 *
		 * As with @a get_multi_resolution_data_raster this returns either a reconstructed or
		 * unreconstructed raster depending on the layer setup.
		 *
		 * This method is useful for @a GLMultiResolutionRasterMapView when it renders a reconstructed
		 * or unreconstructed raster (eg, during numerical raster export).
		 *
		 * NOTE: Since it is possible to set the world transform directly on a cube raster it is not
		 * guaranteed that the identity world transform is set of the returned cube raster (this will
		 * be the case if another caller has changed it and the cube raster is still cached internally).
		 * For this reason it's probably better to instead use @a get_multi_resolution_data_raster
		 * and create our own cube raster to wrap it with.
		 *
		 * See @a get_multi_resolution_data_raster for more details.
		 */
		boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRasterInterface::non_null_ptr_type>
		get_multi_resolution_data_cube_raster(
				GPlatesOpenGL::GLRenderer &renderer)
		{
			return get_multi_resolution_data_cube_raster(renderer, d_current_reconstruction_time, d_current_raster_band_name);
		}

		/**
		 * Returns the possibly reconstructed (multi-resolution) *data* cube raster, for the current
		 * reconstruction time and specified raster band name.
		 */
		boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRasterInterface::non_null_ptr_type>
		get_multi_resolution_data_cube_raster(
				GPlatesOpenGL::GLRenderer &renderer,
				const GPlatesPropertyValues::TextContent &raster_band_name)
		{
			return get_multi_resolution_data_cube_raster(renderer, d_current_reconstruction_time, raster_band_name);
		}

		/**
		 * Returns the possibly reconstructed (multi-resolution) *data* cube raster, current raster
		 * band name at the specified time.
		 */
		boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRasterInterface::non_null_ptr_type>
		get_multi_resolution_data_cube_raster(
				GPlatesOpenGL::GLRenderer &renderer,
				const double &reconstruction_time)
		{
			return get_multi_resolution_data_cube_raster(renderer, reconstruction_time, d_current_raster_band_name);
		}

		/**
		 * Returns the possibly reconstructed (multi-resolution) *data* cube raster, for the specified
		 * time and specified raster band name.
		 */
		boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRasterInterface::non_null_ptr_type>
		get_multi_resolution_data_cube_raster(
				GPlatesOpenGL::GLRenderer &renderer,
				const double &reconstruction_time,
				const GPlatesPropertyValues::TextContent &raster_band_name);


		/**
		 * Returns the multi-resolution age grid mask cube raster for the current
		 * reconstruction time and current raster band.
		 *
		 * This is used to assist with reconstruction of a data raster in another layer.
		 *
		 * NOTE: If 'GLMultiResolutionStaticPolygonReconstructedRaster::supports_age_mask_generation()'
		 * is true then a floating-point raster containing actual age values is returned
		 * (see GLDataRasterSource), otherwise a fixed-point raster containing pre-generated age masks,
		 * the results of age comparisons against a specific reconstruction time
		 * (see GLAgeGridMaskSource), is returned.
		 */
		boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type>
		get_multi_resolution_age_grid_mask(
				GPlatesOpenGL::GLRenderer &renderer)
		{
			return get_multi_resolution_age_grid_mask(
					renderer, d_current_reconstruction_time, d_current_raster_band_name);
		}

		/**
		 * Returns the multi-resolution age grid mask cube raster for the current
		 * reconstruction time and specified raster band.
		 */
		boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type>
		get_multi_resolution_age_grid_mask(
				GPlatesOpenGL::GLRenderer &renderer,
				const GPlatesPropertyValues::TextContent &raster_band_name)
		{
			return get_multi_resolution_age_grid_mask(
					renderer, d_current_reconstruction_time, raster_band_name);
		}

		/**
		 * Returns the multi-resolution age grid mask cube raster for the specified
		 * reconstruction time and current raster band.
		 */
		boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type>
		get_multi_resolution_age_grid_mask(
				GPlatesOpenGL::GLRenderer &renderer,
				const double &reconstruction_time)
		{
			return get_multi_resolution_age_grid_mask(
					renderer, reconstruction_time, d_current_raster_band_name);
		}

		/**
		 * Returns the multi-resolution age grid mask cube raster for the specified
		 * reconstruction time and specified raster band.
		 */
		boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type>
		get_multi_resolution_age_grid_mask(
				GPlatesOpenGL::GLRenderer &renderer,
				const double &reconstruction_time,
				const GPlatesPropertyValues::TextContent &raster_band_name);


		/**
		 * Returns the subject token that clients can use to determine if this raster layer proxy has changed.
		 *
		 * This is mainly useful for other layers that have this layer connected as their input.
		 */
		const GPlatesUtils::SubjectToken &
		get_subject_token();


		/**
		 * Returns the subject token that clients can use to determine if the proxied raster
		 * has changed for the current reconstruction time.
		 *
		 * This is useful for time-dependent rasters where only the proxied raw rasters change.
		 */
		const GPlatesUtils::SubjectToken &
		get_proxied_raster_subject_token()
		{
			return get_proxied_raster_subject_token(d_current_reconstruction_time);
		}

		/**
		 * Returns the subject token that clients can use to determine if the proxied raster
		 * has changed for the specified reconstruction time.
		 *
		 * This is useful for time-dependent rasters where only the proxied raw rasters change.
		 */
		const GPlatesUtils::SubjectToken &
		get_proxied_raster_subject_token(
				const double &reconstruction_time);


		/**
		 * Returns the subject token that clients can use to determine if the raster feature has changed.
		 *
		 * This is useful for determining if only the raster feature has changed (excludes any
		 * changes to the optional reconstructed polygons and the optional age grid and any changes
		 * in the reconstruction time).
		 *
		 * This is useful if this raster layer represents an age grid raster - another raster layer
		 * can use this age grid layer to help reconstruct it - in which case only the *present-day*
		 * proxied raster of this age grid raster is accessed (ie, the proxied raster subject token,
		 * which is time-dependent, is avoided).
		 */
		const GPlatesUtils::SubjectToken &
		get_raster_feature_subject_token();


		/**
		 * Accept a ConstLayerProxyVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstLayerProxyVisitor &visitor) const
		{
			visitor.visit(GPlatesUtils::get_non_null_pointer(this));
		}

		/**
		 * Accept a LayerProxyVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				LayerProxyVisitor &visitor)
		{
			visitor.visit(GPlatesUtils::get_non_null_pointer(this));
		}


		//
		// Used by LayerTask...
		//

		/**
		 * Sets the current reconstruction time as set by the layer system.
		 */
		void
		set_current_reconstruction_time(
				const double &reconstruction_time);

		/**
		 * Adds the specified reconstructed polygons layer proxy.
		 */
		void
		add_current_reconstructed_polygons_layer_proxy(
				const ReconstructLayerProxy::non_null_ptr_type &reconstructed_polygons_layer_proxy);

		/**
		 * Removes the specified reconstructed polygons layer proxy.
		 */
		void
		remove_current_reconstructed_polygons_layer_proxy(
				const ReconstructLayerProxy::non_null_ptr_type &reconstructed_polygons_layer_proxy);

		/**
		 * Set the age grid raster layer proxy.
		 */
		void
		set_current_age_grid_raster_layer_proxy(
				boost::optional<RasterLayerProxy::non_null_ptr_type> age_grid_raster_layer_proxy);

		/**
		 * Set the normal map raster layer proxy.
		 */
		void
		set_current_normal_map_raster_layer_proxy(
				boost::optional<RasterLayerProxy::non_null_ptr_type> normal_map_raster_layer_proxy);

		/**
		 * Specify the raster feature.
		 */
		void
		set_current_raster_feature(
				boost::optional<GPlatesModel::FeatureHandle::weak_ref> raster_feature,
				const RasterLayerTask::Params &raster_params);


		/**
		 * The currently selected raster band name has changed.
		 */
		void
		set_current_raster_band_name(
				const RasterLayerTask::Params &raster_params);

		/**
		 * The raster feature has been modified.
		 */
		void
		modified_raster_feature(
				const RasterLayerTask::Params &raster_params);

	private:
		/**
		 * Potentially time-varying feature properties for the currently resolved raster
		 * (ie, at the cached reconstruction time).
		 */
		struct ResolvedRasterFeatureProperties
		{
			void
			invalidate()
			{
				cached_proxied_raster = boost::none;
				cached_proxied_rasters = boost::none;
			}

			//! The proxied raw raster of the currently selected raster band.
			boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> cached_proxied_raster;

			//! The proxied raw raster for all the raster bands.
			boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> > cached_proxied_rasters;

			/**
			 * The reconstruction time of the cached reconstructed polygon meshes.
			 */
			boost::optional<GPlatesMaths::real_t> cached_reconstruction_time;
		};


		/**
		 * A cached OpenGL multi-resolution *data* raster (and its raster data source) containing numerical raster data.
		 *
		 * The raster is reconstructed if we are connected to a reconstructed polygons layer.
		 */
		struct MultiResolutionDataRaster
		{
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
		 * A cached OpenGL multi-resolution *age grid* raster.
		 *
		 * The following are used if *this* layer is treated as an age grid.
		 * In other words if *this* layer is used to assist with the reconstruction of a raster
		 * in *another* layer.
		 *
		 * NOTE: A raster layer can simultaneously serve as a regular raster and an age grid raster.
		 * This happens when the age grid raster is visualised/analysed *and* assists with the
		 * reconstruction of *another* raster (in a different layer).
		 */
		struct MultiResolutionAgeGridRaster
		{
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


		/**
		 * The input reconstructed polygons, if any connected to our input.
		 */
		LayerProxyUtils::InputLayerProxySequence<ReconstructLayerProxy> d_current_reconstructed_polygons_layer_proxies;

		/**
		 * Optional age grid raster input.
		 */
		LayerProxyUtils::OptionalInputLayerProxy<RasterLayerProxy> d_current_age_grid_raster_layer_proxy;

		/**
		 * Optional normal map raster input.
		 *
		 * FIXME: A normal map is for visualisation so shouldn't be in app-logic code.
		 */
		LayerProxyUtils::OptionalInputLayerProxy<RasterLayerProxy> d_current_normal_map_raster_layer_proxy;

		/**
		 * The raster input feature.
		 */
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> d_current_raster_feature;

		//! The selected raster band name.
		GPlatesPropertyValues::TextContent d_current_raster_band_name;

		//! The raster band names.
		GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type d_current_raster_band_names;

		//! The georeferencing of the raster.
		boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> d_current_georeferencing;

		//! The raster's spatial reference system (if any).
		boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type> d_current_spatial_reference_system;

		//! The coordinate transformation from raster to WGS84.
		GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_to_const_type d_current_coordinate_transformation;

		/**
		 * The current reconstruction time as set by the layer system.
		 */
		double d_current_reconstruction_time;

		//! Time-varying (potentially) raster feature properties.
		ResolvedRasterFeatureProperties d_cached_resolved_raster_feature_properties;

		//! An OpenGL (possibly reconstructed) multi-resolution *data* raster containing numerical raster data.
		MultiResolutionDataRaster d_cached_multi_resolution_data_raster;

		//! An OpenGL multi-resolution *age grid* raster.
		MultiResolutionAgeGridRaster d_cached_multi_resolution_age_grid_raster;

		/**
		 * Used to notify polling observers that we've been updated.
		 */
		mutable GPlatesUtils::SubjectToken d_subject_token;

		/**
		 * The subject token that clients can use to determine if the proxied raster has changed.
		 */
		mutable GPlatesUtils::SubjectToken d_proxied_raster_subject_token;

		/**
		 * The subject token that clients can use to determine if the raster feature has changed.
		 */
		mutable GPlatesUtils::SubjectToken d_raster_feature_subject_token;


		RasterLayerProxy() :
			d_current_raster_band_name(GPlatesUtils::UnicodeString()),
			d_current_coordinate_transformation(
					// Default to identity transformation...
					GPlatesPropertyValues::CoordinateTransformation::create()),
			d_current_reconstruction_time(0)
		{  }


		void
		invalidate_raster_feature();

		void
		invalidate_proxied_raster();

		void
		invalidate();


		/**
		 * Attempts to resolve a raster.
		 *
		 * Can fail if not enough information is available to resolve the raster.
		 * Such as no raster feature or raster feature does not have the required property values.
		 * In this case the returned value will be false.
		 */
		bool
		resolve_raster_feature(
				const double &reconstruction_time,
				const GPlatesPropertyValues::TextContent &raster_band_name);


		//! Sets some raster parameters.
		void
		set_raster_params(
				const RasterLayerTask::Params &raster_params);
	};
}

#endif // GPLATES_APP_LOGIC_RASTERLAYERPROXY_H
