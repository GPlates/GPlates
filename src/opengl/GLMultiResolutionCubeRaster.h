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
#include "GLCubeSubdivisionCache.h"
#include "GLMultiResolutionRaster.h"
#include "GLResourceManager.h"
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
	class GLTransformState;

	/**
	 * A raster that is re-sampled into a multi-resolution cube map (aligned with global axes).
	 */
	class GLMultiResolutionCubeRaster :
			public GPlatesUtils::ReferenceCount<GLMultiResolutionCubeRaster>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLMultiResolutionCubeRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMultiResolutionCubeRaster> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLMultiResolutionCubeRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMultiResolutionCubeRaster> non_null_ptr_to_const_type;

		/**
		 * Typedef for a @a GLCubeSubvision cache of projection transforms.
		 */
		typedef GPlatesOpenGL::GLCubeSubdivisionCache<
				true/*CacheProjectionTransform*/>
						cube_subdivision_projection_transforms_cache_type;


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
			GPlatesUtils::ObjectCache<GLTexture>::volatile_object_ptr_type d_render_texture;


			//! Constructor is private so user cannot construct.
			QuadTreeNode(
					const GLTransform::non_null_ptr_to_const_type &projection_transform_,
					const GLTransform::non_null_ptr_to_const_type &view_transform_,
					const GPlatesUtils::ObjectCache<GLTexture>::volatile_object_ptr_type &render_texture_) :
				d_is_leaf_node(true),
				d_projection_transform(projection_transform_),
				d_view_transform(view_transform_),
				d_render_texture(render_texture_)
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
		 * See @a GLMultiResolutionRaster for a description of @a source_raster_level_of_detail_bias.
		 * This is the bias applied to the source @a source_multi_resolution_raster when it
		 * is rendered into a tile of this cube raster.
		 */
		static
		non_null_ptr_type
		create(
				const GLMultiResolutionRaster::non_null_ptr_type &source_multi_resolution_raster,
				const cube_subdivision_projection_transforms_cache_type::non_null_ptr_type &
						cube_subdivision_projection_transforms_cache,
				const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
				float source_raster_level_of_detail_bias = 0.0f)
		{
			return non_null_ptr_type(
					new GLMultiResolutionCubeRaster(
							source_multi_resolution_raster,
							cube_subdivision_projection_transforms_cache,
							texture_resource_manager,
							source_raster_level_of_detail_bias));
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
		 * Since the returned cube quad tree is 'const' is cannot be modified.
		 *
		 * NOTE: Use the 'get_quad_tree_root_node' method wrapped in 'cube_quad_tree_type' to get
		 * the root quad tree node of a particular face.
		 *
		 * It returns NULL if source raster does not cover any part of the specified cube face.
		 * The raster should cover at least one face of the cube though.
		 *
		 * Note that, unlike child quad tree nodes, NULL will *never* be returned for the
		 * root quad tree node if the resolution of the source raster is not high enough.
		 */
		const cube_quad_tree_type &
		get_cube_quad_tree() const
		{
			return *d_cube_quad_tree;
		}


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
				const QuadTreeNode &tile,
				GLRenderer &renderer); 

	private:
		/**
		 * The raster we are re-sampling into our cube map.
		 */
		GLMultiResolutionRaster::non_null_ptr_type d_multi_resolution_raster;


		/**
		 * Used to retrieve the cube quadtree projection transforms.
		 */
		GPlatesGlobal::PointerTraits<cube_subdivision_projection_transforms_cache_type>::non_null_ptr_type
				d_cube_subdivision_projection_transforms_cache;

		/**
		 * The number of texels along a tiles edge (horizontal or vertical since it's square).
		 */
		std::size_t d_tile_texel_dimension;

		/**
		 * The LOD bias to apply to the source raster when rendering it into our
		 * render textures.
		 */
		float d_source_raster_level_of_detail_bias;

		/**
		 * Used to create new texture resources.
		 */
		GLTextureResourceManager::shared_ptr_type d_texture_resource_manager;

		/**
		 * Cache of tile textures.
		 */
		GPlatesUtils::ObjectCache<GLTexture>::shared_ptr_type d_texture_cache;

		/**
		 * The cube quad tree.
		 *
		 * This is what the user will traverse once we've built the cube quad tree raster.
		 */
		cube_quad_tree_type::non_null_ptr_type d_cube_quad_tree;

		//
		// Used to clear the render target colour buffer.
		//
		GLClearBuffersState::non_null_ptr_type d_clear_buffers_state;
		GLClearBuffers::non_null_ptr_type d_clear_buffers;

		//! Constructor.
		GLMultiResolutionCubeRaster(
				const GLMultiResolutionRaster::non_null_ptr_type &multi_resolution_raster,
				const cube_subdivision_projection_transforms_cache_type::non_null_ptr_type &cube_subdivision_projection_transforms_cache,
				const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
				float source_raster_level_of_detail_bias);


		void
		initialise_cube_quad_trees();

		/**
		 * Creates a quad tree node if it is covered by the source raster.
		 */
		boost::optional<cube_quad_tree_type::node_type::ptr_type>
		create_quad_tree_node(
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
				GLTransformState &transform_state,
				const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transform_quad_tree_node);

		void
		render_raster_data_into_tile_texture(
				const QuadTreeNode &tile,
				const GLTexture::shared_ptr_type &texture,
				GLRenderer &renderer);

		void
		create_tile_texture(
				const GLTexture::shared_ptr_type &texture);
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONCUBERASTER_H
