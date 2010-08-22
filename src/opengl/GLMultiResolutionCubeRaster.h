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

#include "GLClearBuffers.h"
#include "GLClearBuffersState.h"
#include "GLCubeSubdivision.h"
#include "GLMultiResolutionRaster.h"
#include "GLResourceManager.h"
#include "GLTexture.h"
#include "GLTextureCache.h"
#include "GLTextureUtils.h"
#include "GLTransform.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLTransformState;

	/**
	 * A raster that is re-sampled into a multi-resolution cube map (aligned with global axes).
	 */
	class GLMultiResolutionCubeRaster :
			public GPlatesUtils::ReferenceCount<GLMultiResolutionCubeRaster>
	{
	private:
		class QuadTreeNodeImpl;

	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLMultiResolutionCubeRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMultiResolutionCubeRaster> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLMultiResolutionCubeRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMultiResolutionCubeRaster> non_null_ptr_to_const_type;

		/**
		 * Typedef for a handle to a tile.
		 *
		 * A tile represents a single OpenGL texture that is the projection of the source raster
		 * onto a quad tree section of a face of the cube surrounding the globe.
		 */
		typedef std::size_t tile_handle_type;


		/**
		 * A node in the quad tree of a cube face.
		 *
		 * NOTE: This should only be used to traverse the quad tree and
		 * should not be stored for later access.
		 */
		class QuadTreeNode
		{
		public:
			explicit
			QuadTreeNode(
					const QuadTreeNodeImpl &impl_) :
				impl(&impl_)
			{  }

			/**
			 * Get child quad tree node.
			 *
			 * Returns false if specific child node does not cover the source raster.
			 *
			 * @a child_v_offset and @a child_u_offset are 0 or 1 and represent
			 * the placement of tile_v_offset and tile_u_offset in the cube subdivision
			 * among the four child nodes.
			 */
			boost::optional<QuadTreeNode>
			get_child_node(
					unsigned int child_v_offset,
					unsigned int child_u_offset) const;

			tile_handle_type
			get_tile_handle() const
			{
				return impl->tile_handle;
			}

		private:
			const QuadTreeNodeImpl *impl;
		};


		/**
		 * Creates a @a GLMultiResolutionCubeRaster object.
		 *
		 * See @a GLMultiResolutionRaster for a description of @a source_raster_level_of_detail_bias.
		 * This is the bias applied to the source @a source_multi_resolution_raster when it
		 * is rendered into a tile of this cube raster.
		 */
		static
		non_null_ptr_type
		create(
				const GLMultiResolutionRaster::non_null_ptr_type &source_multi_resolution_raster,
				const GLCubeSubdivision::non_null_ptr_to_const_type &cube_subdivision,
				const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
				float source_raster_level_of_detail_bias = 0.0f)
		{
			return non_null_ptr_type(
					new GLMultiResolutionCubeRaster(
							source_multi_resolution_raster,
							cube_subdivision,
							texture_resource_manager,
							source_raster_level_of_detail_bias));
		}


		/**
		 * Returns a token that clients can store with any cached data we render for them and
		 * compare against their current token to determine if they need us to re-render.
		 */
		GLTextureUtils::ValidToken
		get_current_valid_token();


		/**
		 * Get the root quad tree node of the specified face.
		 *
		 * Returns false if source raster does not cover any part of the specified cube face.
		 * The raster will cover at least one face of the cube though.
		 *
		 * NOTE: This method should only be used to traverse the quad tree - the nodes
		 * should not be stored for later access.
		 */
		boost::optional<QuadTreeNode>
		get_root_quad_tree_node(
				GLCubeSubdivision::CubeFaceType cube_face);


		/**
		 * Returns texture of tile.
		 *
		 * @a renderer is used if the tile's texture is not currently cached and needs
		 * to be re-rendered from the source raster.
		 *
		 * NOTE: The returned shared pointer should only be used for rendering
		 * and then discarded. This is because the texture is part of a texture cache
		 * and so it cannot be recycled if a shared pointer is holding onto it.
		 */
		GLTexture::shared_ptr_to_const_type
		get_tile_texture(
				tile_handle_type tile_handle,
				GLRenderer &renderer); 

	private:
		class QuadTreeNodeImpl :
				public GPlatesUtils::ReferenceCount<QuadTreeNodeImpl>
		{
		public:
			typedef GPlatesUtils::non_null_intrusive_ptr<QuadTreeNodeImpl> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const QuadTreeNodeImpl> non_null_ptr_to_const_type;

			static
			non_null_ptr_type
			create(
					tile_handle_type tile_handle_,
					const GLTransform::non_null_ptr_to_const_type &projection_transform_,
					const GLTransform::non_null_ptr_to_const_type &view_transform_)
			{
				return non_null_ptr_type(new QuadTreeNodeImpl(
						tile_handle_, projection_transform_, view_transform_));
			}

			/**
			 * Optional coverage of source raster.
			 *
			 * The 2D array has 'v' offsets as 1st array and 'u' offsets as 2nd array.
			 */
			boost::optional<QuadTreeNodeImpl::non_null_ptr_type> child_nodes[2][2];

			tile_handle_type tile_handle;

			//! Projection matrix defining perspective frustum of this tile.
			GLTransform::non_null_ptr_to_const_type projection_transform;

			//! View matrix defining orientation of frustum of this tile.
			GLTransform::non_null_ptr_to_const_type view_transform;

			//! Tiles of source raster covered by this tile.
			std::vector<GLMultiResolutionRaster::tile_handle_type> src_raster_tiles;

			/**
			 * Keeps tracks of whether the source data has changed underneath us
			 * and we need to reload our texture.
			 */
			mutable GLTextureUtils::ValidToken source_texture_valid_token;

			/**
			 * The texture representation of the raster data for this tile.
			 */
			GLVolatileTexture render_texture;

		private:
			QuadTreeNodeImpl(
					tile_handle_type tile_handle_,
					const GLTransform::non_null_ptr_to_const_type &projection_transform_,
					const GLTransform::non_null_ptr_to_const_type &view_transform_) :
				tile_handle(tile_handle_),
				projection_transform(projection_transform_),
				view_transform(view_transform_)
			{  }
		};

		//! Typedef for a sequence of quad tree nodes.
		typedef std::vector<QuadTreeNodeImpl::non_null_ptr_type> tile_seq_type;

		struct QuadTree
		{
			// Optional coverage of source raster.
			boost::optional<QuadTreeNodeImpl::non_null_ptr_type> root_node;
		};

		struct CubeFace
		{
			QuadTree quad_tree;
		};

		struct Cube
		{
			CubeFace faces[6];
		};


		/**
		 * The raster we are re-sampling into our cube map.
		 */
		GLMultiResolutionRaster::non_null_ptr_type d_multi_resolution_raster;

		/**
		 * Used to determine if we need to rebuild any cached textures due to source data changing.
		 */
		GLTextureUtils::ValidToken d_source_raster_valid_token;


		/**
		 * Defines the quadtree subdivision of each cube face and any overlaps of extents.
		 */
		GLCubeSubdivision::non_null_ptr_to_const_type d_cube_subdivision;

		/**
		 * The number of texels along a tiles edge (horizontal or vertical since it's square).
		 */
		std::size_t d_tile_texel_dimension;

		/**
		 * The LOD bias to apply to the source raster when rendering it into our
		 * render textures.
		 */
		float d_source_raster_level_of_detail_bias;

		Cube d_cube;

		tile_seq_type d_tiles;

		/**
		 * Cache of tile textures.
		 */
		GLTextureCache::non_null_ptr_type d_texture_cache;

		//
		// Used to clear the render target colour buffer.
		//
		GLClearBuffersState::non_null_ptr_type d_clear_buffers_state;
		GLClearBuffers::non_null_ptr_type d_clear_buffers;

		//! Constructor.
		GLMultiResolutionCubeRaster(
				const GLMultiResolutionRaster::non_null_ptr_type &multi_resolution_raster,
				const GLCubeSubdivision::non_null_ptr_to_const_type &cube_subdivision,
				const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
				float source_raster_level_of_detail_bias);


		void
		initialise_cube_quad_trees();

		boost::optional<QuadTreeNodeImpl::non_null_ptr_type>
		create_quad_tree_node(
				GLCubeSubdivision::CubeFaceType cube_face,
				GLTransformState &transform_state,
				unsigned int level_of_detail,
				unsigned int tile_u_offset,
				unsigned int tile_v_offset);

		void
		render_raster_data_into_tile_texture(
				const QuadTreeNodeImpl &tile,
				const GLTexture::shared_ptr_type &texture,
				GLRenderer &renderer);

		void
		update_raster_source_valid_token();

		void
		create_tile_texture(
				const GLTexture::shared_ptr_type &texture);
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONCUBERASTER_H
