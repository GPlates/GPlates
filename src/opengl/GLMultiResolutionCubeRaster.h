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
#include "GLMultiResolutionCubeRasterInterface.h"
#include "GLMultiResolutionRaster.h"
#include "GLTexture.h"
#include "GLTextureUtils.h"
#include "GLTransform.h"

#include "maths/CubeQuadTree.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ObjectCache.h"
#include "utils/ReferenceCount.h"
#include "utils/SubjectObserverToken.h"


namespace GPlatesOpenGL
{
	class GLCapabilities;
	class GLRenderer;
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
		typedef GLMultiResolutionCubeRasterInterface::cache_handle_type cache_handle_type;

		/**
		 * Typedef for a quad tree node.
		 */
		typedef GLMultiResolutionCubeRasterInterface::quad_tree_node_type quad_tree_node_type;

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
		 * 'GL_EXT_texture_filter_anisotropic' extension is not supported.
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
		static const unsigned int DEFAULT_TILE_TEXEL_DIMENSION = 256;


		/**
		 * Returns true if floating-point source raster is supported.
		 *
		 * If false is returned then only fixed-point format textures can be used.
		 *
		 * This is effectively a test for support of the 'GL_EXT_framebuffer_object' extension.
		 */
		static
		bool
		supports_floating_point_source_raster(
				GLRenderer &renderer);


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
		 * If @a source_multi_resolution_raster is floating-point (which means this cube raster
		 * will also be floating-point) then @a supports_floating_point_source_raster *must* return true.
		 *
		 * @a fixed_point_texture_filter only applies if the texture internal format of
		 * @a source_multi_resolution_raster is fixed-point.
		 *
		 * NOTE: If the 'GL_ARB_texture_non_power_of_two' extension is *not* supported then the
		 * actual tile texel dimension will be rounded up to the next power-of-two dimension unless
		 * it is already a power-of-two. There is effectively no optimal adaption for power-of-two dimensions.
		 * This happens regardless of the value of @a adapt_tile_dimension_to_source_resolution.
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
				GLRenderer &renderer,
				const GLMultiResolutionRaster::non_null_ptr_type &source_multi_resolution_raster,
				unsigned int tile_texel_dimension = DEFAULT_TILE_TEXEL_DIMENSION,
				bool adapt_tile_dimension_to_source_resolution = true,
				FixedPointTextureFilterType fixed_point_texture_filter = DEFAULT_FIXED_POINT_TEXTURE_FILTER,
				CacheTileTexturesType cache_tile_textures = DEFAULT_CACHE_TILE_TEXTURES)
		{
			return non_null_ptr_type(
					new GLMultiResolutionCubeRaster(
							renderer,
							source_multi_resolution_raster,
							tile_texel_dimension,
							adapt_tile_dimension_to_source_resolution,
							fixed_point_texture_filter,
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
		get_subject_token() const;


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
		 * Returns the tile texel dimension passed into constructor.
		 */
		virtual
		unsigned int
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
				const quad_tree_node_type &tile)
		{
			create_tile_texture(renderer, tile_texture, get_cube_quad_tree_node(tile).get_element());
		}


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
				const quad_tree_node_type &tile)
		{
			update_tile_texture(renderer, tile_texture, get_cube_quad_tree_node(tile).get_element());
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
			virtual
			bool
			is_leaf_node() const
			{
				return cube_quad_tree_node.get_element().d_is_leaf_node;
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
				GLRenderer &renderer,
				const GLMultiResolutionRaster::non_null_ptr_type &multi_resolution_raster,
				unsigned int initial_tile_texel_dimension,
				bool adapt_tile_dimension_to_source_resolution,
				FixedPointTextureFilterType fixed_point_texture_filter,
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

		GLTexture::shared_ptr_to_const_type
		get_tile_texture(
				GLRenderer &renderer,
				const CubeQuadTreeNode &tile,
				cache_handle_type &cache_handle);

		void
		render_raster_data_into_tile_texture(
				GLRenderer &renderer,
				const CubeQuadTreeNode &tile,
				TileTexture &tile_texture);

		void
		create_tile_texture(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &tile_texture,
				const CubeQuadTreeNode &tile);

		void
		update_tile_texture(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &tile_texture,
				const CubeQuadTreeNode &tile);

		void
		update_fixed_point_tile_texture_mag_filter(
				GLRenderer &renderer,
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
