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

#ifndef GPLATES_OPENGL_GLMULTIRESOLUTIONCUBERECONSTRUCTEDRASTER_H
#define GPLATES_OPENGL_GLMULTIRESOLUTIONCUBERECONSTRUCTEDRASTER_H

#include <cstddef> // For std::size_t
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "GLCubeSubdivision.h"
#include "GLMultiResolutionStaticPolygonReconstructedRaster.h"
#include "GLMultiResolutionRaster.h"
#include "GLTexture.h"
#include "GLTextureUtils.h"

#include "maths/CubeQuadTree.h"
#include "maths/CubeQuadTreeLocation.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ObjectCache.h"
#include "utils/ReferenceCount.h"
#include "utils/SubjectObserverToken.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
	class GLViewport;

	/**
	 * A reconstructed raster rendered into a multi-resolution cube map.
	 */
	class GLMultiResolutionCubeReconstructedRaster :
			public GLMultiResolutionCubeRasterInterface
	{
	private:
		/**
		 * Maintains a tile's texture and source tile cache handle.
		 */
		struct TileTexture
		{
			explicit
			TileTexture(
					GLRenderer &renderer_,
					const GLMultiResolutionStaticPolygonReconstructedRaster::cache_handle_type &source_cache_handle_ =
							GLMultiResolutionStaticPolygonReconstructedRaster::cache_handle_type()) :
				texture(GLTexture::create_as_auto_ptr(renderer_)),
				source_cache_handle(source_cache_handle_)
			{  }

			/**
			 * Clears the source cache.
			 *
			 * Called when 'this' tile texture is returned to the cache (so texture can be reused).
			 */
			void
			returned_to_cache()
			{
				source_cache_handle.reset();
			}

			GLTexture::shared_ptr_type texture;
			GLMultiResolutionStaticPolygonReconstructedRaster::cache_handle_type source_cache_handle;
		};

		/**
		 * Typedef for a cache of tile textures.
		 */
		typedef GPlatesUtils::ObjectCache<TileTexture> tile_texture_cache_type;

	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLMultiResolutionCubeReconstructedRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMultiResolutionCubeReconstructedRaster> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLMultiResolutionCubeReconstructedRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMultiResolutionCubeReconstructedRaster> non_null_ptr_to_const_type;

		/**
		 * Typedef for an opaque object that caches a particular tile of this raster.
		 */
		typedef GLMultiResolutionCubeRasterInterface::cache_handle_type cache_handle_type;

		/**
		 * Typedef for a quad tree node.
		 */
		typedef GLMultiResolutionCubeRasterInterface::quad_tree_node_type quad_tree_node_type;


		/**
		 * Creates a @a GLMultiResolutionCubeReconstructedRaster object.
		 *
		 * The dimension of tiles is the same as the source reconstructed raster.
		 *
		 * If @a cache_tile_textures is true then the tile textures will be cached.
		 */
		static
		non_null_ptr_type
		create(
				GLRenderer &renderer,
				const GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type &source_reconstructed_raster,
				bool cache_tile_textures = true)
		{
			return non_null_ptr_type(
					new GLMultiResolutionCubeReconstructedRaster(
							renderer,
							source_reconstructed_raster,
							cache_tile_textures));
		}


		/**
		 * Gets the transform that is applied to raster/geometries when rendering into the cube map.
		 */
		virtual
		const GLMatrix &
		get_world_transform() const
		{
			return d_world_transform;
		}


		/**
		 * Sets the transform to apply to raster/geometries when rendering into the cube map.
		 */
		virtual
		void
		set_world_transform(
				const GLMatrix &world_transform);


		/**
		 * Returns a subject token that clients can observe to see if they need to update themselves
		 * (such as any cached data we render for them) by getting us to re-render.
		 */
		virtual
		const GPlatesUtils::SubjectToken &
		get_subject_token() const
		{
			// We'll just use the subject token of the raster source - if they don't change then neither do we.
			// If we had two input sources then we'd have to have our own subject token.
			return d_reconstructed_raster->get_subject_token();
		}


		/**
		 * Returns the quad tree root node of the specified cube face.
		 *
		 * Returns boost::none if the source raster does not overlap the specified cube face.
		 */
		virtual
		boost::optional<quad_tree_node_type>
		get_quad_tree_root_node(
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face);


		/**
		 * Returns the specified child cube quad tree node of specified parent node.
		 *
		 * Returns boost::none if the source raster does not overlap the specified child node.
		 */
		virtual
		boost::optional<quad_tree_node_type>
		get_child_node(
				const quad_tree_node_type &parent_node,
				unsigned int child_x_offset,
				unsigned int child_y_offset);


		/**
		 * Returns the tile texel dimension.
		 */
		virtual
		std::size_t
		get_tile_texel_dimension() const
		{
			return d_tile_texel_dimension;
		}


		/**
		 * Returns the texture internal format that can be used if rendering to a texture as
		 * opposed to the main framebuffer.
		 *
		 * This is the internal format of the texture returned by @a get_tile_texture.
		 */
		virtual
		GLint
		get_tile_texture_internal_format() const
		{
			// It's the same as our source raster input.
			return d_reconstructed_raster->get_target_texture_internal_format();
		}

	private:
		/**
		 * Used to cache information, specific to a tile, to return to the client for caching.
		 */
		struct ClientCacheTile
		{
			ClientCacheTile(
					const tile_texture_cache_type::object_shared_ptr_type &tile_texture_,
					// Set to true to cache the GLMultiResolutionCubeReconstructedRaster tile texture...
					bool cache_tile_texture_) :
				// NOTE: The source GLMultiResolutionStaticPolygonReconstructedRaster cache is always cached...
				source_cache_handle(tile_texture_->source_cache_handle)
			{
				if (cache_tile_texture_)
				{
					tile_texture = tile_texture_;
				}
			}

			tile_texture_cache_type::object_shared_ptr_type tile_texture;
			GLMultiResolutionRaster::cache_handle_type source_cache_handle;
		};


		/**
		 * A node in the quad tree of a cube face.
		 */
		struct CubeQuadTreeNode
		{
			CubeQuadTreeNode(
					const GLTransform::non_null_ptr_to_const_type &view_transform_,
					const GLTransform::non_null_ptr_to_const_type &projection_transform_,
					float level_of_detail_,
					const tile_texture_cache_type::volatile_object_ptr_type &tile_texture_) :
				d_view_transform(view_transform_),
				d_projection_transform(projection_transform_),
				d_level_of_detail(level_of_detail_),
				d_tile_texture(tile_texture_)
			{  }

			//! View transform used to render source raster into current tile.
			GLTransform::non_null_ptr_to_const_type d_view_transform;

			//! Projection transform used to render source raster into current tile.
			GLTransform::non_null_ptr_to_const_type d_projection_transform;

			//! Rendering LOD.
			float d_level_of_detail;

			/**
			 * Keeps tracks of whether the source data has changed underneath us
			 * and we need to reload our texture.
			 */
			mutable GPlatesUtils::ObserverToken d_source_texture_observer_token;

			/**
			 * The texture representation of the raster data for this tile.
			 */
			tile_texture_cache_type::volatile_object_ptr_type d_tile_texture;
		};


		/**
		 * Typedef for a cube quad tree with nodes containing the type @a CubeQuadTreeNode.
		 *
		 * This is what is actually traversed by the user once the cube quad tree has been created.
		 */
		typedef GPlatesMaths::CubeQuadTree<CubeQuadTreeNode> cube_quad_tree_type;


		/**
		 * Implementation of base class node to return to the client.
		 */
		struct QuadTreeNodeImpl :
				public GLMultiResolutionCubeRasterInterface::QuadTreeNode::ImplInterface
		{
			QuadTreeNodeImpl(
					cube_quad_tree_type::node_type &cube_quad_tree_node_,
					GLMultiResolutionCubeReconstructedRaster &multi_resolution_cube_raster_,
					const GPlatesMaths::CubeQuadTreeLocation &cube_quad_tree_location_) :
				cube_quad_tree_node(cube_quad_tree_node_),
				multi_resolution_cube_raster(multi_resolution_cube_raster_),
				cube_quad_tree_location(cube_quad_tree_location_)
			{  }

			/**
			 * We always return false for reconstructed rasters because we need to rasterise polygon edges
			 * accurately (eg, if user zooms in a lot) and there's no resolution limit on that.
			 * Also the raster might be reconstructed with a higher resolution age grid and hence we
			 * couldn't rely solely on the source raster resolution anyway.
			 */
			virtual
			bool
			is_leaf_node() const
			{
				return false;
			}

			/**
			 * Returns texture of tile.
			 */
			virtual
			boost::optional<GLTexture::shared_ptr_to_const_type>
			get_tile_texture(
					GLRenderer &renderer,
					cache_handle_type &cache_handle) const
			{
				return multi_resolution_cube_raster.get_tile_texture(
						renderer,
						cube_quad_tree_node.get_element(),
						cache_handle);
			}


			/**
			 * Reference to the cube quad tree node containing the real data.
			 */
			cube_quad_tree_type::node_type &cube_quad_tree_node;

			/**
			 * Pointer to parent class so can delegate to it.
			 */
			GLMultiResolutionCubeReconstructedRaster &multi_resolution_cube_raster;

			/**
			 * Used to determine location of cube quad tree node so can build view/projection transforms.
			 */
			GPlatesMaths::CubeQuadTreeLocation cube_quad_tree_location;
		};


		/**
		 * Minimum tile texel dimension.
		 *
		 * If tile dimensions are too small then we end up requiring a lot more tiles to render since
		 * there's no limit on how deep we can render (see 'QuadTreeNodeImp::is_leaf_node()' for more details).
		 */
		static const unsigned int MIN_TILE_TEXEL_DIMENSION;


		/**
		 * The reconstructed raster we are rendering into our cube map.
		 */
		GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type d_reconstructed_raster;


		/**
		 * If we increased the tile texel dimension then we need to adjust LOD correspondingly.
		 *
		 * Our (cube map) tile dimension can now be a power-of-two multiple of the reconstructed
		 * raster's input source (cube map) tile dimension if the latter is found to be below
		 * 'MIN_TILE_TEXEL_DIMENSION'. We account for this by adding a LOD offset to the final level-of-detail.
		 */
		int d_level_of_detail_offset_for_scaled_tile_dimension;

		/**
		 * The number of texels along a tiles edge (horizontal or vertical since it's square).
		 */
		std::size_t d_tile_texel_dimension;

		/**
		 * Cache of tile textures.
		 */
		tile_texture_cache_type::shared_ptr_type d_texture_cache;

		/**
		 * If true then we cache the tile textures.
		 */
		bool d_cache_tile_textures;

		/**
		 * Used to calculate projection transforms for the cube quad tree.
		 */
		GLCubeSubdivision::non_null_ptr_to_const_type d_cube_subdivision;

		/**
		 * The cube quad tree.
		 *
		 * This is what the user will traverse once we've built the cube quad tree raster.
		 */
		cube_quad_tree_type::non_null_ptr_type d_cube_quad_tree;

		/**
		 * The transform to use when rendering into the cube quad tree tiles.
		 */
		GLMatrix d_world_transform;


		//! Constructor.
		GLMultiResolutionCubeReconstructedRaster(
				GLRenderer &renderer,
				const GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type &source_reconstructed_raster,
				bool cache_tile_textures);

		unsigned int
		update_tile_texel_dimension(
				GLRenderer &renderer,
				unsigned int tile_texel_dimension);

		boost::optional<GLTexture::shared_ptr_to_const_type>
		get_tile_texture(
				GLRenderer &renderer,
				const CubeQuadTreeNode &tile,
				cache_handle_type &cache_handle);

		bool
		render_raster_data_into_tile_texture(
				GLRenderer &renderer,
				const CubeQuadTreeNode &tile,
				TileTexture &tile_texture);

		float
		get_level_of_detail(
				unsigned int quad_tree_depth) const;

		void
		create_tile_texture(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &tile_texture);

		/**
		 * Gets our quad tree node impl from the client's tile handle.
		 */
		static
		QuadTreeNodeImpl &
		get_quad_tree_node_impl(
				const quad_tree_node_type &tile)
		{
			return dynamic_cast<QuadTreeNodeImpl &>(tile.get_impl());
		}
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONCUBERECONSTRUCTEDRASTER_H
