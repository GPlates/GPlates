/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_OPENGL_GLMULTIRESOLUTIONCUBERASTER_H
#define GPLATES_OPENGL_GLMULTIRESOLUTIONCUBERASTER_H

#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "GLCubeSubdivisionCache.h"
#include "GLFramebuffer.h"
#include "GLMultiResolutionCubeRasterInterface.h"
#include "GLMultiResolutionRaster.h"
#include "GLTexture.h"
#include "GLTextureUtils.h"
#include "GLTransform.h"
#include "OpenGL.h"  // For Class GL and the OpenGL constants/typedefs

#include "maths/CubeQuadTree.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ObjectCache.h"
#include "utils/ReferenceCount.h"
#include "utils/SubjectObserverToken.h"


namespace GPlatesOpenGL
{
	class GLCapabilities;
	class GLViewport;

	/**
	 * A raster that is re-sampled into a multi-resolution cube map.
	 */
	class GLMultiResolutionCubeRaster :
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
					GL &gl_,
					const GLMultiResolutionRaster::cache_handle_type &source_cache_handle_ =
							GLMultiResolutionRaster::cache_handle_type()) :
				texture(GLTexture::create_unique(gl_, GL_TEXTURE_2D)),
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
			GLMultiResolutionRaster::cache_handle_type source_cache_handle;
		};

		/**
		 * Typedef for a cache of tile textures.
		 */
		typedef GPlatesUtils::ObjectCache<TileTexture> tile_texture_cache_type;

	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLMultiResolutionCubeRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMultiResolutionCubeRaster> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLMultiResolutionCubeRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMultiResolutionCubeRaster> non_null_ptr_to_const_type;

		/**
		 * Typedef for an opaque object that caches a particular tile of this raster.
		 */
		typedef GLMultiResolutionCubeRasterInterface::cache_handle_type cache_handle_type;

		/**
		 * Typedef for a quad tree node.
		 */
		typedef GLMultiResolutionCubeRasterInterface::quad_tree_node_type quad_tree_node_type;


		/**
		 * Determines the granularity of caching to be used for GLMultiResolutionCubeRaster tile textures...
		 */
		enum CacheTileTexturesType
		{
			CACHE_TILE_TEXTURES_NONE,
			CACHE_TILE_TEXTURES_INDIVIDUAL_TILES,
			CACHE_TILE_TEXTURES_ENTIRE_CUBE_QUAD_TREE
		};

		/**
		 * The default granularity of tile texture caching.
		 */
		static const CacheTileTexturesType DEFAULT_CACHE_TILE_TEXTURES = CACHE_TILE_TEXTURES_INDIVIDUAL_TILES;


		/**
		 * The default tile dimension is 256.
		 *
		 * See @a create for details on how, and if, the actual tile dimension is calculated.
		 *
		 * This size gives us a small enough tile region on the globe to make good use
		 * of view frustum culling of tiles.
		 */
		static const unsigned int DEFAULT_TILE_TEXEL_DIMENSION = 256;


		/**
		 * Creates a @a GLMultiResolutionCubeRaster object.
		 *
		 * @a tile_texel_dimension is the (possibly unadapted) dimension of each square tile texture
		 * (returned by @a get_tile_texture).
		 *
		 * If @a adapt_tile_dimension_to_source_resolution is true then @a tile_texel_dimension
		 * is optimally adapted to the resolution of the source raster such that each level-of-detail
		 * in 'this' cube raster matches the source resolution - this also minimises the number of
		 * levels of detail - without this there is typically one extra level-of-detail required to
		 * capture the highest resolution of the source raster.
		 * NOTE: The adapted tile texel dimension will never be larger than twice @a tile_texel_dimension.
		 * If it is larger than the maximum supported texture dimension then it will be changed to the maximum.
		 *
		 * If @a cache_tile_textures is 'CACHE_TILE_TEXTURES_ENTIRE_CUBE_QUAD_TREE' then the internal
		 * texture cache is allowed to grow to encompass all existing cube quad tree nodes/tiles.
		 * WARNING: This should normally be set to 'CACHE_TILE_TEXTURES_INDIVIDUAL_TILES'
		 * (especially for visualisation of rasters where only a small part of the raster is ever
		 * visible at any time - otherwise memory usage will grow excessively large).
		 * WARNING: 'CACHE_TILE_TEXTURES_NONE' should be used with care since the texture returned by
		 * 'GLMultiResolutionCubeRasterInterface::QuadTreeNode::get_tile_texture()' can be recycled
		 * and overwritten by a subsequent call - so the returned texture should be used/drawn before a
		 * subsequent call. And keeping the returned texture shared pointer alive is *not* sufficient
		 * to prevent a texture from being recycled (specifying 'CACHE_TILE_TEXTURES_INDIVIDUAL_TILES'
		 * and keeping the returned 'cache_handle_type' alive will prevent recycling).
		 * See similar variable in @a GLMultiResolutionRaster::create for more details.
		 * If it's turned on then it should ideally only be turned on either there or here.
		 */
		static
		non_null_ptr_type
		create(
				GL &gl,
				const GLMultiResolutionRaster::non_null_ptr_type &source_multi_resolution_raster,
				unsigned int tile_texel_dimension = DEFAULT_TILE_TEXEL_DIMENSION,
				bool adapt_tile_dimension_to_source_resolution = true,
				CacheTileTexturesType cache_tile_textures = DEFAULT_CACHE_TILE_TEXTURES)
		{
			return non_null_ptr_type(
					new GLMultiResolutionCubeRaster(
							gl,
							source_multi_resolution_raster,
							tile_texel_dimension,
							adapt_tile_dimension_to_source_resolution,
							cache_tile_textures));
		}


		/**
		 * Gets the transform that is applied to raster/geometries when rendering into the cube map.
		 */
		const GLMatrix &
		get_world_transform() const override
		{
			return d_world_transform;
		}


		/**
		 * Sets the transform to apply to raster/geometries when rendering into the cube map.
		 */
		void
		set_world_transform(
				const GLMatrix &world_transform) override;


		/**
		 * Returns a subject token that clients can observe to see if they need to update themselves
		 * (such as any cached data we render for them) by getting us to re-render.
		 */
		const GPlatesUtils::SubjectToken &
		get_subject_token() const override;


		/**
		 * Returns the quad tree root node of the specified cube face.
		 *
		 * Returns boost::none if the source raster does not overlap the specified cube face.
		 */
		boost::optional<quad_tree_node_type>
		get_quad_tree_root_node(
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face) override;


		/**
		 * Returns the specified child cube quad tree node of specified parent node.
		 *
		 * Returns boost::none if the source raster does not overlap the specified child node.
		 */
		boost::optional<quad_tree_node_type>
		get_child_node(
				const quad_tree_node_type &parent_node,
				unsigned int child_x_offset,
				unsigned int child_y_offset) override;


		/**
		 * Returns the tile texel dimension passed into constructor.
		 */
		unsigned int
		get_tile_texel_dimension() const override
		{
			return d_tile_texel_dimension;
		}


		/**
		 * Returns the texture internal format that can be used if rendering to a texture as
		 * opposed to the main framebuffer.
		 */
		GLint
		get_tile_texture_internal_format() const override
		{
			// It's the same as our source raster input.
			return d_multi_resolution_raster->get_tile_texture_internal_format();
		}


		/**
		 * Returns the optional texture swizzle for the alpha channel (GL_TEXTURE_SWIZZLE_A).
		 *
		 * If not specified then the alpha swizzle is unchanged (ie, alpha value comes from alpha channel).
		 * This is useful for data (RG) rasters where the data value is in the Red channel and the coverage (alpha)
		 * value is in the Green channel (in which case a swizzle of GL_GREEN copies the green channel to alpha channel).
		 */
		boost::optional<GLenum>
		get_tile_texture_swizzle_alpha() const override
		{
			return d_multi_resolution_raster->get_tile_texture_swizzle_alpha();
		}


		/**
		 * Returns true if the raster is displayed visually (as opposed to a data raster used
		 * for numerical calculations).
		 *
		 * This is used to determine texture filtering for optimal display.
		 */
		bool
		tile_texture_is_visual() const override
		{
			return d_multi_resolution_raster->tile_texture_is_visual();
		}


		/**
		 * Returns the number of levels of detail.
		 *
		 * NOTE: This can be less than that returned by our source raster
		 * - ie, by 'GLMultiResolutionRaster::get_num_levels_of_detail()'.
		 * This happens when we don't support the lowest resolution(s) of the source texture
		 * (because our tile dimension is such that the second lowest resolution, for example, is
		 * needed to fill our lowest resolution which is one tile per cube face).
		 * We always support the highest source texture resolution (that's just a matter of going
		 * deep enough into our cube quad tree).
		 * Also note that depth (of cube quad tree) and source level-of-detail (LOD) are the inverse
		 * of each other - an LOD of zero corresponds to a depth of 'get_num_levels_of_detail() - 1'.
		 */
		unsigned int
		get_num_levels_of_detail() const
		{
			return d_num_source_levels_of_detail_used;
		}

	private:
		/**
		 * Used to cache information, specific to a tile, to return to the client for caching.
		 */
		struct ClientCacheTile
		{
			ClientCacheTile(
					const tile_texture_cache_type::object_shared_ptr_type &tile_texture_,
					// Set to true to cache the GLMultiResolutionCubeRaster tile texture...
					CacheTileTexturesType cache_tile_textures_) :
				// NOTE: The GLMultiResolutionRaster tile is always cached...
				source_cache_handle(tile_texture_->source_cache_handle)
			{
				if (cache_tile_textures_ != CACHE_TILE_TEXTURES_NONE)
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
					unsigned int tile_level_of_detail_,
					const GLTransform::non_null_ptr_to_const_type &world_model_view_transform_,
					const GLTransform::non_null_ptr_to_const_type &projection_transform_,
					const tile_texture_cache_type::volatile_object_ptr_type &tile_texture_) :
				// Starts off as true (and later gets set to false if this is an internal node)...
				d_is_leaf_node(true),
				d_tile_level_of_detail(tile_level_of_detail_),
				d_world_model_view_transform(world_model_view_transform_),
				d_projection_transform(projection_transform_),
				d_tile_texture(tile_texture_)
			{  }

			/**
			 * A leaf node - a node containing *no* children.
			 */
			bool d_is_leaf_node;

			/**
			 * The level-of-detail at which to render this tile.
			 *
			 * This remains constant even when the world transform changes.
			 */
			unsigned int d_tile_level_of_detail;

			//! World model view transform used to render source raster into current tile.
			GLTransform::non_null_ptr_to_const_type d_world_model_view_transform;

			//! Projection transform used to render source raster into current tile.
			GLTransform::non_null_ptr_to_const_type d_projection_transform;

			//! Tiles of source raster covered by this tile.
			mutable std::vector<GLMultiResolutionRaster::tile_handle_type> d_src_raster_tiles;

			/**
			 * The texture representation of the raster data for this tile.
			 */
			tile_texture_cache_type::volatile_object_ptr_type d_tile_texture;

			/**
			 * Keeps tracks of whether the source data has changed underneath us
			 * and we need to reload our texture.
			 */
			mutable GPlatesUtils::ObserverToken d_source_texture_observer_token;
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
					const cube_quad_tree_type::node_type &cube_quad_tree_node_,
					GLMultiResolutionCubeRaster &multi_resolution_cube_raster_) :
				cube_quad_tree_node(cube_quad_tree_node_),
				multi_resolution_cube_raster(multi_resolution_cube_raster_)
			{  }

			/**
			 * Returns true if this quad tree node is at the highest resolution.
			 */
			bool
			is_leaf_node() const override
			{
				return cube_quad_tree_node.get_element().d_is_leaf_node;
			}

			/**
			 * Returns texture of tile.
			 */
			boost::optional<GLTexture::shared_ptr_type>
			get_tile_texture(
					GL &gl,
					cache_handle_type &cache_handle) const override
			{
				return multi_resolution_cube_raster.get_tile_texture(
						gl,
						cube_quad_tree_node.get_element(),
						cache_handle);
			}


			/**
			 * Reference to the cube quad tree node containing the real data.
			 */
			const cube_quad_tree_type::node_type &cube_quad_tree_node;

			/**
			 * Pointer to parent class so can delegate to it.
			 */
			GLMultiResolutionCubeRaster &multi_resolution_cube_raster;
		};


		/**
		 * Typedef for a @a GLCubeSubvision cache.
		 */
		typedef GLCubeSubdivisionCache<
				true/*CacheProjectionTransform*/>
						cube_subdivision_cache_type;


		/**
		 * The raster we are re-sampling into our cube map.
		 */
		GLMultiResolutionRaster::non_null_ptr_type d_multi_resolution_raster;

		/**
		 * Keep track of changes to @a d_multi_resolution_raster.
		 */
		mutable GPlatesUtils::ObserverToken d_multi_resolution_raster_observer_token;


		/**
		 * The number of texels along a tiles edge (horizontal or vertical since it's square).
		 */
		unsigned int d_tile_texel_dimension;

		/**
		 * Cache of tile textures.
		 */
		tile_texture_cache_type::shared_ptr_type d_texture_cache;

		/**
		 * Determines granularity of caching of *our* tile textures (from @a get_tile_texture).
		 */
		CacheTileTexturesType d_cache_tile_textures;

		/**
		 * Framebuffer object to render to tile textures.
		 */
		GLFramebuffer::shared_ptr_type d_tile_framebuffer;

		/**
		 * Check framebuffer completeness the first time we render to a tile texture.
		 */
		bool d_have_checked_tile_framebuffer_completeness;

		/**
		 * The cube quad tree.
		 *
		 * This is what the user will traverse once we've built the cube quad tree raster.
		 */
		cube_quad_tree_type::non_null_ptr_type d_cube_quad_tree;

		/**
		 * The number of levels of detail of the source raster that we use.
		 *
		 * NOTE: This can be less than that returned by our source raster
		 * - ie, by 'GLMultiResolutionRaster::get_num_levels_of_detail()'.
		 */
		unsigned int d_num_source_levels_of_detail_used;

		/**
		 * The transform to use when rendering into the cube quad tree tiles.
		 */
		GLMatrix d_world_transform;

		/**
		 * Used to inform clients that we have been updated.
		 */
		mutable GPlatesUtils::SubjectToken d_subject_token;


		//! Constructor.
		GLMultiResolutionCubeRaster(
				GL &gl,
				const GLMultiResolutionRaster::non_null_ptr_type &multi_resolution_raster,
				unsigned int initial_tile_texel_dimension,
				bool adapt_tile_dimension_to_source_resolution,
				CacheTileTexturesType cache_tile_textures);

		/**
		 * Adjusts @a d_tile_texel_dimension and determines the number of LODs of source raster used
		 * by this cube map raster.
		 */
		void
		adjust_tile_texel_dimension(
				bool adapt_tile_dimension_to_source_resolution,
				const GLCapabilities &capabilities);

		void
		initialise_cube_quad_trees();

		/**
		 * Creates a quad tree node if it is covered by the source raster.
		 */
		boost::optional<cube_quad_tree_type::node_type::ptr_type>
		create_quad_tree_node(
				const GLViewport &viewport,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
				const unsigned int source_level_of_detail);

		GLTexture::shared_ptr_type
		get_tile_texture(
				GL &gl,
				const CubeQuadTreeNode &tile,
				cache_handle_type &cache_handle);

		void
		render_raster_data_into_tile_texture(
				GL &gl,
				const CubeQuadTreeNode &tile,
				TileTexture &tile_texture);

		void
		create_tile_texture(
				GL &gl,
				const GLTexture::shared_ptr_type &tile_texture,
				const CubeQuadTreeNode &tile);

		void
		set_tile_texture_filtering(
				GL &gl,
				const GLTexture::shared_ptr_type &tile_texture,
				const CubeQuadTreeNode &tile);

		/**
		 * Gets our internal cube quad tree node from the client's tile handle.
		 */
		static
		const cube_quad_tree_type::node_type &
		get_cube_quad_tree_node(
				const quad_tree_node_type &tile)
		{
			return dynamic_cast<QuadTreeNodeImpl &>(tile.get_impl()).cube_quad_tree_node;
		}
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONCUBERASTER_H
