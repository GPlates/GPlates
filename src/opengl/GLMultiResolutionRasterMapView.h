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

#ifndef GPLATES_OPENGL_GLMULTIRESOLUTIONRASTERMAPVIEW_H
#define GPLATES_OPENGL_GLMULTIRESOLUTIONRASTERMAPVIEW_H

#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "GLCubeSubdivisionCache.h"
#include "GLMatrix.h"
#include "GLMultiResolutionCubeRasterInterface.h"
#include "GLMultiResolutionMapCubeMesh.h"
#include "GLProgramObject.h"
#include "GLTexture.h"
#include "GLTransform.h"
#include "GLViewport.h"

#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLFrustum;
	class GLRenderer;

	/**
	 * Used to view multi-resolution cube raster in a 2D map projection of the globe.
	 *
	 * This includes anything that can be rendered into a @a GLMultiResolutionCubeRasterInterface.
	 */
	class GLMultiResolutionRasterMapView :
			public GPlatesUtils::ReferenceCount<GLMultiResolutionRasterMapView>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLMultiResolutionRasterMapView.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMultiResolutionRasterMapView> non_null_ptr_type;
		//! A convenience typedef for a shared pointer to a const @a GLMultiResolutionMapView.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMultiResolutionRasterMapView> non_null_ptr_to_const_type;

		/**
		 * Typedef for an opaque object that caches a particular render of this map view.
		 */
		typedef boost::shared_ptr<void> cache_handle_type;


		/**
		 * Creates a @a GLMultiResolutionRasterMapView object.
		 *
		 * NOTE: The world transform gets set on @a multi_resolution_cube_raster according to the
		 * central meridian of the map projection.
		 * This means the input cube raster will get re-oriented.
		 *
		 * @a tile_texel_dimension is the (possibly unadapted) dimension of each square tile texture
		 * (returned by @a get_tile_texture).
		 */
		static
		non_null_ptr_type
		create(
				GLRenderer &renderer,
				const GLMultiResolutionCubeRasterInterface::non_null_ptr_type &multi_resolution_cube_raster,
				const GLMultiResolutionMapCubeMesh::non_null_ptr_to_const_type &multi_resolution_map_cube_mesh)
		{
			return non_null_ptr_type(
					new GLMultiResolutionRasterMapView(
							renderer,
							multi_resolution_cube_raster,
							multi_resolution_map_cube_mesh));
		}


		/**
		 * Renders the source raster, as a map projection, visible in the view frustum
		 * (determined by the current viewport and model-view/projection transforms of @a renderer).
		 *
		 * @a cache_handle can be stored by the client to keep textures (and vertices), used during this render, cached.
		 *
		 * Returns true if any rendering was performed (this can be false if source raster is
		 * not a global raster, for example, and does not intersect the view frustum).
		 */
		bool
		render(
				GLRenderer &renderer,
				cache_handle_type &cache_handle);

	private:
		/**
		 * Typedef for a @a GLCubeSubvision cache.
		 */
		typedef GLCubeSubdivisionCache<
				true/*CacheProjectionTransform*/, false/*CacheLooseProjectionTransform*/,
				false/*CacheFrustum*/, false/*CacheLooseFrustum*/,
				false/*CacheBoundingPolygon*/, false/*CacheLooseBoundingPolygon*/,
				true/*CacheBounds*/, false/*CacheLooseBounds*/>
						cube_subdivision_cache_type;

		/**
		 * Typedef for a @a GLCubeSubvision cache.
		 */
		typedef GLCubeSubdivisionCache<
				true/*CacheProjectionTransform*/, false/*CacheLooseProjectionTransform*/,
				false/*CacheFrustum*/, false/*CacheLooseFrustum*/,
				false/*CacheBoundingPolygon*/, false/*CacheLooseBoundingPolygon*/,
				false/*CacheBounds*/, false/*CacheLooseBounds*/>
						clip_cube_subdivision_cache_type;


		//! Typedef for a quad tree node of a multi-resolution cube mesh.
		typedef GLMultiResolutionMapCubeMesh::quad_tree_node_type mesh_quad_tree_node_type;

		//! Typedef for the source raster cube quad tree node.
		typedef GLMultiResolutionCubeRasterInterface::quad_tree_node_type raster_quad_tree_node_type;


		/**
		 * The viewport pixel size (in map projection coordinates) to use when there's an error.
		 *
		 * A value roughly the width of the entire map projection should cause the
		 * lowest resolution view to be rendered.
		 */
		static const double ERROR_VIEWPORT_PIXEL_SIZE_IN_MAP_PROJECTION;


		GLMultiResolutionCubeRasterInterface::non_null_ptr_type d_multi_resolution_cube_raster;
		GLMultiResolutionMapCubeMesh::non_null_ptr_to_const_type d_multi_resolution_map_cube_mesh;

		/**
		 * The texture dimension of a cube quad tree tile.
		 */
		unsigned int d_tile_texel_dimension;

		//! 1.0 / 'd_tile_texel_dimension'.
		float d_inverse_tile_texel_dimension;

		/**
		 * The map projection's central meridian longitude is used as a transform when
		 * rendering the source raster (to align it with the map cube mesh).
		 */
		double d_map_projection_central_meridian_longitude;

		/**
		 * The transform used for the map projection's central meridian longitude.
		 */
		GLMatrix d_world_transform;

		/**
		 * Shader program to render tiles to the scene.
		 *
		 * Is boost::none if shader programs not supported (in which case fixed-function pipeline is used).
		 */
		boost::optional<GLProgramObject::shared_ptr_type> d_render_tile_to_scene_program_object;

		/**
		 * Shader program to render tiles to the scene with clipping.
		 *
		 * Is boost::none if shader programs not supported (in which case fixed-function pipeline is used
		 * but without clipping - so artifacts will appear when zoomed in far enough).
		 */
		boost::optional<GLProgramObject::shared_ptr_type> d_render_tile_to_scene_with_clipping_program_object;


		GLMultiResolutionRasterMapView(
				GLRenderer &renderer,
				const GLMultiResolutionCubeRasterInterface::non_null_ptr_type &multi_resolution_cube_raster,
				const GLMultiResolutionMapCubeMesh::non_null_ptr_to_const_type &multi_resolution_map_cube_mesh);

		void
		render_quad_tree(
				GLRenderer &renderer,
				const raster_quad_tree_node_type &source_raster_quad_tree_node,
				const mesh_quad_tree_node_type &mesh_quad_tree_node,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
				clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
				const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
				const double &viewport_tile_map_projection_size,
				const GLFrustum &frustum_planes,
				boost::uint32_t frustum_plane_mask,
				std::vector<cache_handle_type> &cached_tiles,
				unsigned int &num_tiles_rendered_to_scene);

		void
		render_tile_to_scene(
				GLRenderer &renderer,
				const raster_quad_tree_node_type &source_raster_quad_tree_node,
				const mesh_quad_tree_node_type &mesh_quad_tree_node,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
				clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
				const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
				std::vector<cache_handle_type> &cached_tiles,
				unsigned int &num_tiles_rendered_to_scene);

		void
		set_tile_state(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &tile_texture,
				const GLTransform &projection_transform,
				const GLTransform &clip_projection_transform,
				const GLTransform &view_transform,
				bool clip_to_tile_frustum);

		double
		get_viewport_pixel_size_in_map_projection(
				const GLViewport &viewport,
				const GLMatrix &model_view_transform,
				const GLMatrix &projection_transform) const;

		void
		create_shader_programs(
				GLRenderer &renderer);
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONRASTERMAPVIEW_H
