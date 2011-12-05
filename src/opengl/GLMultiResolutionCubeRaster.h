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

#include <cstddef> // For std::size_t
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "GLCubeSubdivisionCache.h"
#include "GLMultiResolutionRaster.h"
#include "GLTexture.h"
#include "GLTextureUtils.h"
#include "OpenGLFwd.h"

#include "maths/MathsFwd.h"
#include "maths/CubeQuadTree.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ObjectCache.h"
#include "utils/ReferenceCount.h"
#include "utils/SubjectObserverToken.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
	class GLViewport;

	/**
	 * A raster that is re-sampled into a multi-resolution cube map (aligned with global axes).
	 */
	class GLMultiResolutionCubeRaster :
			public GPlatesUtils::ReferenceCount<GLMultiResolutionCubeRaster>
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
					const GLMultiResolutionRaster::cache_handle_type &source_cache_handle_ =
							GLMultiResolutionRaster::cache_handle_type()) :
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
		typedef boost::shared_ptr<void> cache_handle_type;

		/**
		 * The texture filter types to use for fixed-point textures.
		 *
		 * Floating-point textures always use nearest filtering with no anisotropic filtering.
		 * This is because earlier hardware does not support anything but nearest filtering and
		 * so it needs to be emulated in a fragment shader (instead of being part of a texture
		 * object's state) and hence must be done by the client (not us).
		 *
		 * NOTE: The 'minification' filtering is always 'nearest' - the 'magnification' filter is
		 * what is configurable.
		 * The 'magnification' filter also applies only to the leaf node since that is when
		 * the maximum resolution has been reached and magnification starts to happen.
		 * 
		 * If anisotropic filtering is specified it will be ignored if the
		 * 'GLEW_EXT_texture_filter_anisotropic' extension is not supported.
		 */
		enum FixedPointTextureFilterType
		{
			// Nearest neighbour magnification filtering...
			FIXED_POINT_TEXTURE_FILTER_MAG_NEAREST,
			// Nearest neighbour magnification (with anisotropic) filtering...
			FIXED_POINT_TEXTURE_FILTER_MAG_NEAREST_ANISOTROPIC,
			// Bilinear magnification filtering...
			FIXED_POINT_TEXTURE_FILTER_MAG_LINEAR,
			// Bilinear magnification (with anisotropic) filtering...
			FIXED_POINT_TEXTURE_FILTER_MAG_LINEAR_ANISOTROPIC
		};

		/**
		 * The default fixed-point texture filtering mode for the textures returned by @a get_tile_texture
		 * is bilinear (with anisotropic) filtering.
		 *
		 * Note that floating-point textures are always nearest neighbour (with no anisotropic) regardless.
		 */
		static const FixedPointTextureFilterType DEFAULT_FIXED_POINT_TEXTURE_FILTER =
				FIXED_POINT_TEXTURE_FILTER_MAG_LINEAR_ANISOTROPIC;


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
		static const std::size_t DEFAULT_TILE_TEXEL_DIMENSION = 256;


		/**
		 * A node in the quad tree of a cube face.
		 */
		class QuadTreeNode
		{
		public:
			/*
			 * NOTE: Use the 'get_child_node' method wrapped in 'cube_quad_tree_type::node_type'
			 * to return a child quad tree node of this quad tree node.
			 *
			 * It returns NULL if either:
			 * 1) the source raster is not high enough resolution to warrant creating child nodes, or
			 * 2) the source raster does not cover the child's area.
			 *
			 * For (1) this will happen when 'this' quad tree node returns true for @a is_leaf_node.
			 * In other words 'this' node is at the highest resolution so it won't have any children.
			 *
			 * For (2) this will happen the source raster is non-global and partially covers this
			 * quad tree node (in which case it's possible not all children will be covered).
			 */


			/**
			 * Returns true if this quad tree node is at the highest resolution.
			 *
			 * In other words a resolution high enough to capture the resolution of the source raster.
			 *
			 * If true is returned then this quad tree node will have *no* children, otherwise
			 * it will have one or more children depending on which child nodes are covered by the
			 * source raster (ie, if the source raster is non-global).
			 */
			bool
			is_leaf_node() const
			{
				return d_is_leaf_node;
			}

		private:
			/**
			 * A leaf node - a node containing *no* children.
			 */
			bool d_is_leaf_node;

			//! Projection matrix defining perspective frustum of this tile.
			GLTransform::non_null_ptr_to_const_type d_projection_transform;

			//! View matrix defining orientation of frustum of this tile.
			GLTransform::non_null_ptr_to_const_type d_view_transform;

			//! Tiles of source raster covered by this tile.
			std::vector<GLMultiResolutionRaster::tile_handle_type> d_src_raster_tiles;

			/**
			 * Keeps tracks of whether the source data has changed underneath us
			 * and we need to reload our texture.
			 */
			mutable GPlatesUtils::ObserverToken d_source_texture_observer_token;

			/**
			 * The texture representation of the raster data for this tile.
			 */
			tile_texture_cache_type::volatile_object_ptr_type d_tile_texture;


			//! Constructor is private so user cannot construct.
			QuadTreeNode(
					const GLTransform::non_null_ptr_to_const_type &projection_transform_,
					const GLTransform::non_null_ptr_to_const_type &view_transform_,
					const tile_texture_cache_type::volatile_object_ptr_type &tile_texture_) :
				d_is_leaf_node(true),
				d_projection_transform(projection_transform_),
				d_view_transform(view_transform_),
				d_tile_texture(tile_texture_)
			{  }

			/**
			 * Sets this node as an internal node - a node containing non-null children.
			 */
			void
			set_internal_node()
			{
				d_is_leaf_node = false;
			}

			// Make a friend so can construct and access data.
			friend class GLMultiResolutionCubeRaster;
		};


		/**
		 * Typedef for a cube quad tree with nodes containing the type @a QuadTreeNode.
		 *
		 * This is what is actually traversed by the user once the cube quad tree has been created.
		 */
		typedef GPlatesMaths::CubeQuadTree<QuadTreeNode> cube_quad_tree_type;


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
		 * So you should ensure that '2 * tile_texel_dimension' is not larger than the maximum texture dimension.
		 *
		 * NOTE: If the 'GL_ARB_texture_non_power_of_two' extension is *not* supported then the
		 * actual tile texel dimension will be rounded up to the next power-of-two dimension unless
		 * it is already a power-of-two. There is effectively no optimal adaption for power-of-two dimensions.
		 * This happens regardless of the value of @a adapt_tile_dimension_to_source_resolution.
		 *
		 * If @a cache_source_tile_textures is true then the source raster tile textures will be
		 * cached *when/if* our tile textures are cached during calls to @a get_tile_texture.
		 * Note that the @a get_tile_texture method is used to specify if, and when, *our* tile textures are cached.
		 *
		 * If @a cache_tile_textures is true then the internal texture cache is allowed to
		 * grow to encompass all existing cube quad tree nodes/tiles.
		 * WARNING: This should normally be turned off (especially for visualisation of rasters where
		 * only a small part of the raster is ever visible at any time - otherwise memory usage will
		 * grow excessively large).
		 * See similar variable in @a GLMultiResolutionRaster::create for more details.
		 * If it's turned on then it should ideally only be turned on either there or here.
		 */
		static
		non_null_ptr_type
		create(
				GLRenderer &renderer,
				const GLMultiResolutionRaster::non_null_ptr_type &source_multi_resolution_raster,
				std::size_t tile_texel_dimension = DEFAULT_TILE_TEXEL_DIMENSION,
				bool adapt_tile_dimension_to_source_resolution = true,
				FixedPointTextureFilterType fixed_point_texture_filter = DEFAULT_FIXED_POINT_TEXTURE_FILTER,
				bool cache_source_tile_textures = true,
				CacheTileTexturesType cache_tile_textures = DEFAULT_CACHE_TILE_TEXTURES)
		{
			return non_null_ptr_type(
					new GLMultiResolutionCubeRaster(
							renderer,
							source_multi_resolution_raster,
							tile_texel_dimension,
							adapt_tile_dimension_to_source_resolution,
							fixed_point_texture_filter,
							cache_source_tile_textures,
							cache_tile_textures));
		}


		/**
		 * Returns a subject token that clients can observe to see if they need to update themselves
		 * (such as any cached data we render for them) by getting us to re-render.
		 */
		const GPlatesUtils::SubjectToken &
		get_subject_token() const
		{
			// We'll just use the subject token of the raster source - if they don't change then neither do we.
			// If we had two input sources then we'd have to have our own subject token.
			return d_multi_resolution_raster->get_subject_token();
		}


		/**
		 * The returned cube quad tree can be used to traverse the cube quad tree raster.
		 *
		 * The element type of the returned cube quad tree is @a QuadTreeNode.
		 *
		 * Since the returned cube quad tree is 'const' it cannot be modified.
		 *
		 * NOTE: Use the 'get_quad_tree_root_node' method wrapped in 'cube_quad_tree_type' to get
		 * the root quad tree node of a particular face.
		 */
		const cube_quad_tree_type &
		get_cube_quad_tree() const
		{
			return *d_cube_quad_tree;
		}


		/**
		 * Returns texture of tile.
		 *
		 * NOTE: The returned texture has bilinear filtering enabled if it's a fixed-point texture.
		 * If it's a floating-point texture you should emulate bilinear filtering in a fragment shader.
		 * This is because earlier hardware (supporting floating-point textures) only supports nearest filtering.
		 *
		 * @a renderer is used if the tile's texture is not currently cached and needs
		 * to be re-rendered from the source raster.
		 *
		 * @a cache_handle can be stored by the client to keep textures (and vertices) cached.
		 * Depending on the 'cache_tile_textures' parameter to @a create the returned tile texture
		 * can be kept alive (ie, prevents it from being recycled in the internal cache due to another
		 * call to @a get_tile_texture).
		 * The same applies to the source tile textures from GLMultiResolutionRaster which depend
		 * on the 'cache_source_tile_textures' parameter to @a create.
		 *
		 * NOTE: The returned texture shared pointer should only be used for rendering
		 * and then discarded. This is because the texture is part of a texture cache and it might
		 * get used during a subsequent call to @a get_tile_texture if it isn't being cached.
		 */
		GLTexture::shared_ptr_to_const_type
		get_tile_texture(
				GLRenderer &renderer,
				const QuadTreeNode &tile,
				cache_handle_type &cache_handle);


		/**
		 * Returns the tile texel dimension passed into constructor.
		 */
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
		GLint
		get_tile_texture_internal_format() const
		{
			// It's the same as our source raster input.
			return d_multi_resolution_raster->get_target_texture_internal_format();
		}


		/**
		 * Returns the filter for fixed-point textures (selected in @a create).
		 */
		FixedPointTextureFilterType
		get_fixed_point_texture_filter() const
		{
			return d_fixed_point_texture_filter;
		}


		/**
		 * Initialises the specified tile texture to reserve memory for its (uninitialised) image and
		 * sets its various filtering options.
		 *
		 * Normally this method isn't needed since you can call @a get_tile_texture which both
		 * allocates and initialises a texture.
		 * However clients can call this if they want to modify a tile texture returned by
		 * @a get_tile_texture (such as modulating it with another texture).
		 */
		void
		create_tile_texture(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &tile_texture,
				const QuadTreeNode &tile);


		/**
		 * Updates the specified tile texture, created with @a create_tile_texture, so that its
		 * filtering options correspond to whether it belongs to a leaf node tile or not.
		 *
		 * Normally this method isn't needed since you can call @a get_tile_texture which handles
		 * @a create_tile_texture and @a update_tile_texture for you.
		 * However clients can call this if they want to modify a tile texture returned by
		 * @a get_tile_texture (such as modulating it with another texture).
		 */
		void
		update_tile_texture(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &tile_texture,
				const QuadTreeNode &tile);

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
		 * Typedef for a @a GLCubeSubvision cache.
		 */
		typedef GPlatesOpenGL::GLCubeSubdivisionCache<
				true/*CacheProjectionTransform*/>
						cube_subdivision_cache_type;


		/**
		 * The raster we are re-sampling into our cube map.
		 */
		GLMultiResolutionRaster::non_null_ptr_type d_multi_resolution_raster;


		/**
		 * The number of texels along a tiles edge (horizontal or vertical since it's square).
		 */
		std::size_t d_tile_texel_dimension;

		/**
		 * The texture filtering mode (for fixed-point textures) returned by @a get_tile_texture.
		 */
		FixedPointTextureFilterType d_fixed_point_texture_filter;

		/**
		 * Cache of tile textures.
		 */
		tile_texture_cache_type::shared_ptr_type d_texture_cache;

		/**
		 * Determines granularity of caching of *our* tile textures (from @a get_tile_texture).
		 */
		CacheTileTexturesType d_cache_tile_textures;

		/**
		 * If true then we cache the source tile textures used to render each of our tile textures.
		 */
		bool d_cache_source_tile_textures;

		/**
		 * The cube quad tree.
		 *
		 * This is what the user will traverse once we've built the cube quad tree raster.
		 */
		cube_quad_tree_type::non_null_ptr_type d_cube_quad_tree;

		//! Constructor.
		GLMultiResolutionCubeRaster(
				GLRenderer &renderer,
				const GLMultiResolutionRaster::non_null_ptr_type &multi_resolution_raster,
				std::size_t initial_tile_texel_dimension,
				bool adapt_tile_dimension_to_source_resolution,
				FixedPointTextureFilterType fixed_point_texture_filter,
				bool cache_source_tile_textures,
				CacheTileTexturesType cache_tile_textures);

		/**
		 * Adjusts @a d_tile_texel_dimension and returns the number of LODs of source raster used
		 * by this cube map raster.
		 */
		unsigned int
		adjust_tile_texel_dimension(
				bool adapt_tile_dimension_to_source_resolution);

		void
		initialise_cube_quad_trees(
				const unsigned int num_levels_of_detail);

		/**
		 * Creates a quad tree node if it is covered by the source raster.
		 */
		boost::optional<cube_quad_tree_type::node_type::ptr_type>
		create_quad_tree_node(
				const GLViewport &viewport,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
				const unsigned int level_of_detail);

		void
		render_raster_data_into_tile_texture(
				GLRenderer &renderer,
				const QuadTreeNode &tile,
				TileTexture &tile_texture);

		void
		update_fixed_point_tile_texture_mag_filter(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &tile_texture,
				const QuadTreeNode &tile);
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONCUBERASTER_H
