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
 
#ifndef GPLATES_GUI_PERSISTENTOPENGLOBJECTS_H
#define GPLATES_GUI_PERSISTENTOPENGLOBJECTS_H

#include <map>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "Colour.h"
#include "MapProjection.h"
#include "RasterColourPalette.h"

#include "app-logic/Layer.h"
#include "app-logic/RasterLayerProxy.h"
#include "app-logic/ReconstructGraph.h"
#include "app-logic/ResolvedRaster.h"

#include "maths/CubeQuadTreePartition.h"
#include "maths/types.h"

#include "opengl/GLCoverageSource.h"
#include "opengl/GLAgeGridMaskSource.h"
#include "opengl/GLContext.h"
#include "opengl/GLMultiResolutionCubeMesh.h"
#include "opengl/GLMultiResolutionCubeRaster.h"
#include "opengl/GLMultiResolutionCubeRasterInterface.h"
#include "opengl/GLMultiResolutionFilledPolygons.h"
#include "opengl/GLMultiResolutionMapCubeMesh.h"
#include "opengl/GLMultiResolutionRaster.h"
#include "opengl/GLMultiResolutionRasterMapView.h"
#include "opengl/GLMultiResolutionStaticPolygonReconstructedRaster.h"
#include "opengl/GLVisualRasterSource.h"
#include "opengl/GLReconstructedStaticPolygonMeshes.h"
#include "opengl/OpenGLFwd.h"

#include "property-values/Georeferencing.h"
#include "property-values/RawRaster.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"

#include "view-operations/RenderedGeometry.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesOpenGL
{
	class GLRenderer;
}

namespace GPlatesGui
{
	/**
	 * Keeps track of any OpenGL-related objects that are persistent beyond one rendering frame.
	 *
	 * Until now there have been no such objects, but rasters are now persistent otherwise
	 * it would be far too expensive to rebuild them each time they need to be rendered.
	 *
	 * Each OpenGL context that does not share list objects, such as textures and display lists,
	 * will require a separate instance of this class.
	 */
	class PersistentOpenGLObjects :
			public QObject,
			public GPlatesUtils::ReferenceCount<PersistentOpenGLObjects>
	{
		Q_OBJECT

	public:
		//! A convenience typedef for a shared pointer to a non-const @a PersistentOpenGLObjects.
		typedef GPlatesUtils::non_null_intrusive_ptr<PersistentOpenGLObjects> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a PersistentOpenGLObjects.
		typedef GPlatesUtils::non_null_intrusive_ptr<const PersistentOpenGLObjects> non_null_ptr_to_const_type;

		
		//! Typedef for a collection of filled polygons.
		typedef GPlatesOpenGL::GLMultiResolutionFilledPolygons::filled_polygons_type filled_polygons_type;

		/**
		 * Typedef for an opaque object that caches a particular render (eg, raster or filled polygons).
		 */
		typedef boost::shared_ptr<void> cache_handle_type;


		/**
		 * Creates a new @a PersistentOpenGLObjects object.
		 *
		 * Currently listens for removed layers to determine when to flush objects.
		 */
		static
		non_null_ptr_type
		create(
				const GPlatesOpenGL::GLContext::non_null_ptr_type &opengl_context,
				GPlatesAppLogic::ApplicationState &application_state)
		{
			return non_null_ptr_type(new PersistentOpenGLObjects(opengl_context, application_state));
		}

		/**
		 * Creates a @a PersistentOpenGLObjects object and that always shares the non-list objects
		 * and only shares the list objects if @a objects_from_another_context uses a context
		 * that shares the same shared state as @a opengl_context.
		 *
		 * This basically allows objects that use textures and display lists to be shared
		 * across QGLWidgets objects (or whatever objects have different OpenGL contexts).
		 * The sharing depends on whether the two OpenGL contexts allow shared textures/display-lists.
		 */
		static
		non_null_ptr_type
		create(
				const GPlatesOpenGL::GLContext::non_null_ptr_type &opengl_context,
				const PersistentOpenGLObjects::non_null_ptr_type &objects_from_another_context,
				GPlatesAppLogic::ApplicationState &application_state)
		{
			return non_null_ptr_type(
					new PersistentOpenGLObjects(opengl_context, objects_from_another_context, application_state));
		}


		/**
		 * Renders the possibly reconstructed multi-resolution raster.
		 *
		 * This method will try to reuse an existing multi-resolution raster as
		 * best it can if some of the parameters are common.
		 *
		 * @a source_raster_modulate_colour can be used to modulate the raster by the specified
		 * colour (eg, to enable semi-transparent rasters).
		 *
		 * If @a map_projection is specified then the raster is rendered using the specified
		 * 2D map projection, otherwise it's rendered to the 3D globe.
		 */
		cache_handle_type
		render_raster(
				GPlatesOpenGL::GLRenderer &renderer,
				const GPlatesAppLogic::ResolvedRaster::non_null_ptr_to_const_type &source_resolved_raster,
				const RasterColourPalette::non_null_ptr_to_const_type &source_raster_colour_palette,
				const Colour &source_raster_modulate_colour = Colour::get_white(),
				boost::optional<MapProjection::non_null_ptr_to_const_type> map_projection = boost::none);


		/**
		 * Renders filled polygons.
		 *
		 * These correspond to @a RenderedGeometry objects that have had their 'fill' option
		 * turned on and can be polygons or polylines - the latter geometry type is treated as an
		 * ordered sequence of points that join to form a polygon.
		 *
		 * A self-intersecting polygon is filled in those parts of the polygon that intersect the
		 * polygon an odd numbers of times when a line is formed from the point (part) in question
		 * to a point outside the exterior of the polygon. Same applies to polylines.
		 */
		void
		render_filled_polygons(
				GPlatesOpenGL::GLRenderer &renderer,
				const filled_polygons_type &filled_polygons);

	public slots:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		/**
		 * Called when an existing layer is about to be removed.
		 *
		 * Release objects associated with the specified layer (as it's about to be destroyed).
		 */
		void
		handle_layer_about_to_be_removed(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				GPlatesAppLogic::Layer layer);

	private:
		/**
		 * Base class for all layer usages.
		 *
		 * A layer usage is one way to use the output of a layer.
		 */
		class LayerUsage :
				public GPlatesUtils::ReferenceCount<LayerUsage>
		{
		public:
			typedef GPlatesUtils::non_null_intrusive_ptr<LayerUsage> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const LayerUsage> non_null_ptr_to_const_type;

			//! The types of layer usage.
			enum Type
			{
				RASTER,
				CUBE_RASTER,
				AGE_GRID,
				RECONSTRUCTED_STATIC_POLYGON_MESHES,
				STATIC_POLYGON_RECONSTRUCTED_RASTER,
				MAP_RASTER,

				NUM_TYPES
			};

			virtual
			~LayerUsage()
			{  }

			/**
			 * Returns true if this layer usage depends (directory, or indirectly via dependency
			 * layer usages) on the specified layer proxy.
			 *
			 * This is used to determine which layer usages to remove when a layer proxy is removed.
			 */
			virtual
			bool
			is_required_direct_or_indirect_dependency(
					const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy) const = 0;

			/**
			 * Notifies that a layer (proxy) is about to be removed.
			 *
			 * Gives this layer usage a chance stop using an *optional* dependency (either directly
			 * or indirectly via dependency layer usages). The default does nothing
			 * (the default for layer usages that have no *optional* dependencies).
			 */
			virtual
			void
			removing_layer(
					const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy)
			{  }
		};


		/**
		 * A regular, unreconstructed coloured raster (can be time-dependent).
		 */
		class RasterLayerUsage :
				public LayerUsage
		{
		public:
			explicit
			RasterLayerUsage(
					const GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type &raster_layer_proxy);

			//! Sets the raster colour palette.
			void
			set_raster_colour_palette(
					GPlatesOpenGL::GLRenderer &renderer,
					const RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette);

			//! Sets the raster modulation colour.
			void
			set_raster_modulate_colour(
					GPlatesOpenGL::GLRenderer &renderer,
					const Colour &raster_modulate_colour);

			/**
			 * Returns multi-resolution raster - rebuilds if out-of-date with respect to its dependencies.
			 *
			 * Returns false if the raster is not a proxy raster or if it's uninitialised.
			 */
			boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type>
			get_multi_resolution_raster(
					GPlatesOpenGL::GLRenderer &renderer);

			virtual
			bool
			is_required_direct_or_indirect_dependency(
					const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy) const;

		private:
			GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type d_raster_layer_proxy;
			GPlatesUtils::ObserverToken d_proxied_raster_observer_token;
			GPlatesUtils::ObserverToken d_raster_feature_observer_token;

			boost::optional<RasterColourPalette::non_null_ptr_to_const_type> d_raster_colour_palette;
			bool d_raster_colour_palette_dirty;

			Colour d_raster_modulate_colour;
			bool d_raster_modulate_colour_dirty;

			boost::optional<GPlatesOpenGL::GLVisualRasterSource::non_null_ptr_type> d_visual_raster_source;
			boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type> d_multi_resolution_raster;
		};


		/**
		 * A regular, unreconstructed coloured raster mapped into a cube map.
		 *
		 * The cube map allows the raster to be reconstructed.
		 */
		class CubeRasterLayerUsage :
				public LayerUsage
		{
		public:
			explicit
			CubeRasterLayerUsage(
					const GPlatesUtils::non_null_intrusive_ptr<RasterLayerUsage> &raster_layer_usage);

			/**
			 * Returns multi-resolution cube raster - rebuilds if out-of-date with respect to its dependencies.
			 */
			boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type>
			get_multi_resolution_cube_raster(
					GPlatesOpenGL::GLRenderer &renderer);

			virtual
			bool
			is_required_direct_or_indirect_dependency(
					const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy) const;

		private:
			GPlatesUtils::non_null_intrusive_ptr<RasterLayerUsage> d_raster_layer_usage;

			boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type> d_multi_resolution_raster;
			boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type> d_multi_resolution_cube_raster;
		};


		/**
		 * A present-day floating-point raster used to age-mask another reconstructed raster.
		 */
		class AgeGridLayerUsage :
				public LayerUsage
		{
		public:
			explicit
			AgeGridLayerUsage(
					const GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type &age_grid_raster_layer_proxy);

			//! Specify the current reconstruction time.
			void
			set_reconstruction_time(
					GPlatesOpenGL::GLRenderer &renderer,
					const double &reconstruction_time);

			/**
			 * Returns the age grid *mask* multi-resolution raster.
			 *
			 * Rebuilds if out-of-date with respect to its dependencies.
			 */
			boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type>
			get_age_grid_mask_multi_resolution_raster(
					GPlatesOpenGL::GLRenderer &renderer);

			/**
			 * Returns the age grid *mask* multi-resolution raster.
			 *
			 * Rebuilds if out-of-date with respect to its dependencies.
			 */
			boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type>
			get_age_grid_coverage_multi_resolution_raster(
					GPlatesOpenGL::GLRenderer &renderer);


			virtual
			bool
			is_required_direct_or_indirect_dependency(
					const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy) const;

		private:
			GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type d_age_grid_raster_layer_proxy;
			GPlatesUtils::ObserverToken d_age_grid_raster_feature_observer_token;

			GPlatesMaths::real_t d_reconstruction_time;
			bool d_reconstruction_time_dirty;

			boost::optional<GPlatesOpenGL::GLAgeGridMaskSource::non_null_ptr_type> d_age_grid_mask_multi_resolution_source;
			boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type> d_age_grid_mask_multi_resolution_raster;

			boost::optional<GPlatesOpenGL::GLCoverageSource::non_null_ptr_type> d_age_grid_coverage_multi_resolution_source;
			boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type> d_age_grid_coverage_multi_resolution_raster;

			void
			check_input_raster();

			void
			update();
		};


		/**
		 * A group of reconstructed static polygon meshes.
		 */
		class ReconstructedStaticPolygonMeshesLayerUsage :
				public LayerUsage
		{
		public:
			explicit
			ReconstructedStaticPolygonMeshesLayerUsage(
					const GPlatesAppLogic::ReconstructLayerProxy::non_null_ptr_type &reconstructed_polygons_layer_proxy);

			//! Specify the current reconstruction time.
			void
			set_reconstruction_time(
					GPlatesOpenGL::GLRenderer &renderer,
					const double &reconstruction_time);

			/**
			 * Returns the reconstructed static polygon meshes.
			 *
			 * Rebuilds if out-of-date with respect to its dependencies.
			 */
			GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::non_null_ptr_type
			get_reconstructed_static_polygon_meshes(
					GPlatesOpenGL::GLRenderer &renderer,
					bool reconstructing_with_age_grid);

			virtual
			bool
			is_required_direct_or_indirect_dependency(
					const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy) const;

		private:
			GPlatesAppLogic::ReconstructLayerProxy::non_null_ptr_type d_reconstructed_static_polygon_meshes_layer_proxy;
			GPlatesUtils::ObserverToken d_reconstructed_polygons_observer_token;
			GPlatesUtils::ObserverToken d_present_day_polygons_observer_token;

			GPlatesMaths::real_t d_reconstruction_time;
			bool d_reconstruction_time_dirty;

			boost::optional<bool> d_reconstructing_with_age_grid;

			boost::optional<GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::non_null_ptr_type>
					d_reconstructed_static_polygon_meshes;

			void
			update();
		};


		/**
		 * A raster reconstructed using static polygons (and optionally an age-grid).
		 *
		 * The raster can also be time-dependent.
		 */
		class StaticPolygonReconstructedRasterLayerUsage :
				public LayerUsage
		{
		public:
			explicit
			StaticPolygonReconstructedRasterLayerUsage(
					const GPlatesUtils::non_null_intrusive_ptr<CubeRasterLayerUsage> &cube_raster_layer_usage);

			/**
			 * Set/update the layer usages that come from other layers.
			 *
			 * This is done in case the user connects to new layers or disconnects.
			 */
			void
			set_layer_inputs(
					const boost::optional<GPlatesUtils::non_null_intrusive_ptr<ReconstructedStaticPolygonMeshesLayerUsage> > &
							reconstructed_polygon_meshes_layer_usage,
					const boost::optional<GPlatesUtils::non_null_intrusive_ptr<AgeGridLayerUsage> > &age_grid_layer_usage);

			/**
			 * Returns the static polygon reconstructed raster.
			 *
			 * Rebuilds if out-of-date with respect to its dependencies.
			 * Including if the specified layer usages are different objects since last time.
			 */
			boost::optional<GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type>
			get_static_polygon_reconstructed_raster(
					GPlatesOpenGL::GLRenderer &renderer);

			virtual
			bool
			is_required_direct_or_indirect_dependency(
					const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy) const;

			virtual
			void
			removing_layer(
					const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy);

		private:
			boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type> d_multi_resolution_cube_raster;
			GPlatesUtils::non_null_intrusive_ptr<CubeRasterLayerUsage> d_cube_raster_layer_usage;

			boost::optional<GPlatesOpenGL::GLReconstructedStaticPolygonMeshes::non_null_ptr_type>
					d_reconstructed_polygon_meshes;
			boost::optional<GPlatesUtils::non_null_intrusive_ptr<ReconstructedStaticPolygonMeshesLayerUsage> >
					d_reconstructed_polygon_meshes_layer_usage;

			boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type> d_age_grid_mask_raster;
			boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type> d_age_grid_coverage_raster;
			boost::optional<GPlatesUtils::non_null_intrusive_ptr<AgeGridLayerUsage> > d_age_grid_layer_usage;

			boost::optional<GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type>
					d_reconstructed_raster;
		};


		/**
		 * A map-view of a (possibly reconstructed) raster.
		 */
		class MapRasterLayerUsage :
				public LayerUsage
		{
		public:
			explicit
			MapRasterLayerUsage(
					const GPlatesUtils::non_null_intrusive_ptr<RasterLayerUsage> &raster_layer_usage,
					const GPlatesUtils::non_null_intrusive_ptr<StaticPolygonReconstructedRasterLayerUsage> &
							reconstructed_raster_layer_usage);

			/**
			 * Returns multi-resolution raster in map view - rebuilds if out-of-date with respect to its dependencies.
			 *
			 * @a multi_resolution_map_cube_mesh is shared by all layers (because contains no layer-specific state).
			 */
			boost::optional<GPlatesOpenGL::GLMultiResolutionRasterMapView::non_null_ptr_type>
			get_multi_resolution_raster_map_view(
					GPlatesOpenGL::GLRenderer &renderer,
					const GPlatesOpenGL::GLMultiResolutionMapCubeMesh::non_null_ptr_to_const_type &multi_resolution_map_cube_mesh);

			virtual
			bool
			is_required_direct_or_indirect_dependency(
					const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy) const;

		private:
			boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type> d_raster;
			GPlatesUtils::non_null_intrusive_ptr<RasterLayerUsage> d_raster_layer_usage;

			boost::optional<GPlatesOpenGL::GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type> d_reconstructed_raster;
			GPlatesUtils::non_null_intrusive_ptr<StaticPolygonReconstructedRasterLayerUsage> d_reconstructed_raster_layer_usage;

			boost::optional<GPlatesOpenGL::GLMultiResolutionRasterMapView::non_null_ptr_type> d_multi_resolution_raster_map_view;
		};


		/**
		 * Represents OpenGL objects (in the various layer usage classes) associated with a layer.
		 *
		 * Each layer contains all the possible uses of any layer type.
		 * Although not all uses are applicable - depends on layer (proxy) type.
		 * And of the applicable uses only a subset might actually be used.
		 */
		class GLLayer :
				public GPlatesUtils::ReferenceCount<GLLayer>
		{
		public:
			typedef GPlatesUtils::non_null_intrusive_ptr<GLLayer> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const GLLayer> non_null_ptr_to_const_type;

			static
			non_null_ptr_type
			create(
					const GPlatesAppLogic::LayerProxy::non_null_ptr_type &layer_proxy)
			{
				return non_null_ptr_type(new GLLayer(layer_proxy));
			}

			//! Returns the raster layer usage (creates one if does not yet exist).
			GPlatesUtils::non_null_intrusive_ptr<RasterLayerUsage>
			get_raster_layer_usage();

			//! Returns the cube raster layer usage (creates one if does not yet exist).
			GPlatesUtils::non_null_intrusive_ptr<CubeRasterLayerUsage>
			get_cube_raster_layer_usage();

			//! Returns the age grid layer usage (creates one if does not yet exist).
			GPlatesUtils::non_null_intrusive_ptr<AgeGridLayerUsage>
			get_age_grid_layer_usage();

			//! Returns the reconstructed static polygon meshes layer usage (creates one if does not yet exist).
			GPlatesUtils::non_null_intrusive_ptr<ReconstructedStaticPolygonMeshesLayerUsage>
			get_reconstructed_static_polygon_meshes_layer_usage();

			//! Returns the static polygon reconstructed raster layer usage (creates one if does not yet exist).
			GPlatesUtils::non_null_intrusive_ptr<StaticPolygonReconstructedRasterLayerUsage>
			get_static_polygon_reconstructed_raster_layer_usage();

			//! Returns the map raster layer usage (creates one if does not yet exist).
			GPlatesUtils::non_null_intrusive_ptr<MapRasterLayerUsage>
			get_map_raster_layer_usage();

			/**
			 * Called by @a GLLayers when a layer (proxy) is about to be removed.
			 */
			void
			remove_references_to_layer(
					const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_to_be_removed);

		private:
			//! Typedef for a sequence of optional layer usage objects.
			typedef std::vector<boost::optional<GPlatesUtils::non_null_intrusive_ptr<LayerUsage> > >
					optional_layer_usage_seq_type;

			GPlatesAppLogic::LayerProxy::non_null_ptr_type d_layer_proxy;
			optional_layer_usage_seq_type d_layer_usages;

			GLLayer(
					const GPlatesAppLogic::LayerProxy::non_null_ptr_type &layer_proxy);
		};


		/**
		 * Associates each @a GLLayer with a layer proxy (the output of an application-logic layer).
		 */
		class GLLayers
		{
		public:
			GLLayer &
			get_layer(
					const GPlatesAppLogic::LayerProxy::non_null_ptr_type &layer_proxy);

			void
			remove_layer(
					const GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type &layer_proxy_handle);

		private:
			typedef std::map<
					GPlatesAppLogic::LayerProxyHandle::non_null_ptr_type,
					GLLayer::non_null_ptr_type> layer_map_type;

			layer_map_type d_layer_map;
		};


		/**
		 * Any objects that do *not* use textures, display lists, vertex buffer objects, etc
		 * can go here, otherwise use @a ListObjects.
		 *
		 * These objects will be shared even if two OpenGL contexts don't share list objects.
		 *
		 * Objects that might go here are things like vertex arrays (which reside in system memory)
		 * that are large and hence sharing would reduce memory usage. Because they reside in
		 * system memory (ie, not dedicated RAM on the graphics card) it doesn't matter which
		 * OpenGL context is active when we draw them.
		 * NOTE: Vertex arrays are different from vertex buffer objects - the latter are shared objects.
		 */
		struct NonListObjects
		{
		};


		/**
		 * Any objects that use textures, display lists, vertex buffer objects, etc
		 * should go here, otherwise use @a NonListObjects.
		 */
		struct ListObjects
		{
			ListObjects(
					const boost::shared_ptr<GPlatesOpenGL::GLContext::SharedState> &opengl_shared_state,
					const NonListObjects &non_list_objects);


			/**
			 * Shared textures, etc.
			 */
			boost::shared_ptr<GPlatesOpenGL::GLContext::SharedState> opengl_shared_state;

			/**
			 * Keeps track of each GL layer associated with each layer proxy (one-to-one).
			 *
			 * This is stored in the @a ListObjects structure so that it only gets shared
			 * with others that are using the same OpenGL context.
			 * If the context is not shared then the layers will not be shared (although the
			 * non-list objects can still be shared).
			 */
			GLLayers gl_layers;

			/**
			 * Returns the multi-resolution cube mesh.
			 *
			 * This consumes a reasonable amount of memory (~50Mb) so it is shared across all layers.
			 *
			 * NOTE: This must be called when an OpenGL context is currently active.
			 */
			GPlatesOpenGL::GLMultiResolutionCubeMesh::non_null_ptr_to_const_type
			get_multi_resolution_cube_mesh(
					GPlatesOpenGL::GLRenderer &renderer) const;

			/**
			 * Returns the multi-resolution *map* cube mesh.
			 *
			 * This also consumes a reasonable amount of memory so it is shared across all layers.
			 *
			 * NOTE: This must be called when an OpenGL context is currently active.
			 */
			GPlatesOpenGL::GLMultiResolutionMapCubeMesh::non_null_ptr_to_const_type
			get_multi_resolution_map_cube_mesh(
					GPlatesOpenGL::GLRenderer &renderer,
					const MapProjection &map_projection) const;

			/**
			 * Returns the multi-resolution filled polygons renderer.
			 *
			 * NOTE: This must be called when an OpenGL context is currently active.
			 */
			GPlatesOpenGL::GLMultiResolutionFilledPolygons::non_null_ptr_type
			get_multi_resolution_filled_polygons(
					GPlatesOpenGL::GLRenderer &renderer) const;

		private:
			const NonListObjects &d_non_list_objects;

			/**
			 * Used to get a mesh for any cube quad tree node.
			 *
			 * NOTE: This can be shared by all layers since it contains no state specific
			 * to anything a layer will draw with it.
			 */
			mutable boost::optional<GPlatesOpenGL::GLMultiResolutionCubeMesh::non_null_ptr_to_const_type>
					d_multi_resolution_cube_mesh;

			/**
			 * Used to get a mesh to view any cube quad tree raster in a map-projection view.
			 *
			 * NOTE: This can be shared by all layers since it contains no state specific
			 * to anything a layer will draw with it (contains only global map projection).
			 */
			mutable boost::optional<GPlatesOpenGL::GLMultiResolutionMapCubeMesh::non_null_ptr_type>
					d_multi_resolution_map_cube_mesh;

			/**
			 * Used to render coloured filled polygons as raster masks (instead of polygon meshes).
			 *
			 * NOTE: This can be shared by all layers since it contains no state specific
			 * to anything a layer will draw with it. The filled polygons specific state is
			 * stored externally and maintained by the clients (eg, the filled polygon vertex arrays).
			 *
			 * NOTE: Must be defined after @a d_multi_resolution_cube_mesh since it's a dependency.
			 */
			mutable boost::optional<GPlatesOpenGL::GLMultiResolutionFilledPolygons::non_null_ptr_type>
					d_multi_resolution_filled_polygons;
		};


		// NOTE: The non-list objects *must* be declared *before* the list objects (construction order).
		boost::shared_ptr<NonListObjects> d_non_list_objects;
		boost::shared_ptr<ListObjects> d_list_objects;


		//! Constructor.
		explicit
		PersistentOpenGLObjects(
				const GPlatesOpenGL::GLContext::non_null_ptr_type &opengl_context,
				GPlatesAppLogic::ApplicationState &application_state);

		//! Constructor.
		PersistentOpenGLObjects(
				const GPlatesOpenGL::GLContext::non_null_ptr_type &opengl_context,
				const PersistentOpenGLObjects::non_null_ptr_type &objects_from_another_context,
				GPlatesAppLogic::ApplicationState &application_state);


		void
		make_signal_slot_connections(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph);
	};
}

#endif // GPLATES_GUI_PERSISTENTOPENGLOBJECTS_H
