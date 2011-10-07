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

#ifndef GPLATES_OPENGL_GLMULTIRESOLUTIONCUBEMESH_H
#define GPLATES_OPENGL_GLMULTIRESOLUTIONCUBEMESH_H

#include <vector>
#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLCompiledDrawState.h"
#include "GLMatrix.h"
#include "GLTexture.h"
#include "GLTextureUtils.h"
#include "GLUtils.h"
#include "GLVertex.h"
#include "GLVertexArray.h"
#include "OpenGLFwd.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/MathsFwd.h"
#include "maths/CubeCoordinateFrame.h"
#include "maths/CubeQuadTree.h"
#include "maths/CubeQuadTreeLocation.h"

#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * A mesh that is gridded along the cube subdivision tiles.
	 */
	class GLMultiResolutionCubeMesh :
			public GPlatesUtils::ReferenceCount<GLMultiResolutionCubeMesh>
	{
	private:
		/**
		 * Stores mesh information for a cube quad tree node.
		 */
		struct MeshQuadTreeNode
		{
			MeshQuadTreeNode(
					const GLCompiledDrawState::non_null_ptr_to_const_type &mesh_drawable_) :
				mesh_drawable(mesh_drawable_)
			{  }

			GLCompiledDrawState::non_null_ptr_to_const_type mesh_drawable;
		};

		/**
		 * Typedef for a cube quad tree with nodes containing the type @a MeshQuadTreeNode.
		 */
		typedef GPlatesMaths::CubeQuadTree<MeshQuadTreeNode> mesh_cube_quad_tree_type;

	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLMultiResolutionCubeMesh.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMultiResolutionCubeMesh> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLMultiResolutionCubeMesh.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMultiResolutionCubeMesh> non_null_ptr_to_const_type;


		/**
		 * Used during traversal of the mesh cube quad tree to obtain quad tree node meshes.
		 */
		class QuadTreeNode
		{
		public:
			/**
			 * Returns the mesh drawable for this quad tree node.
			 */
			GLCompiledDrawState::non_null_ptr_to_const_type
			get_mesh_drawable() const
			{
				return d_mesh_drawable;
			}

			/**
			 * Returns the clip space transform for this quad tree node.
			 *
			 * The returned matrix should post-multiply the matrix returned by
			 * @a get_clip_texture_clip_space_to_texture_space_transform in order to convert from
			 * clip space [-1, 1] to the appropriate opaque texels (inner 2x2) in the clip texture,
			 * and for the full tile texture.
			 *
			 * Also the returned matrix should post-multiply the matrix returned by
			 * @a get_tile_texture_clip_space_to_texture_space_transform in order to convert from
			 * clip space [-1, 1] to the *full* tile texture.
			 *
			 * If boost::none is returned then no clip texture is required because the drawable
			 * mesh for the specified quad tree node exactly matches the area of the corresponding tile.
			 * This means @a get_clip_texture_clip_space_to_texture_space_transform and
			 * @a get_tile_texture_clip_space_to_texture_space_transform aren't required either.
			 *
			 * boost::none is returned until you traverse deeper in the quad tree than the
			 * pre-generated mesh quad tree at which point texture clipping is required since
			 * the mesh is larger than the current quad tree node tile.
			 *
			 * NOTE: The above texture matrix matrix multiplies are not needed if the
			 * the projection transform of the tile's frustum is used because this already
			 * takes into account the clip space adjustments.
			 */
			const boost::optional<GLUtils::QuadTreeClipSpaceTransform> &
			get_clip_texture_clip_space_transform() const
			{
				return d_clip_space_transform;
			}

		private:
			/**
			 * Reference to the cube quad tree node containing the mesh drawable.
			 */
			const mesh_cube_quad_tree_type::node_type *d_mesh_node;

			/**
			 * The mesh drawable.
			 */
			GLCompiledDrawState::non_null_ptr_to_const_type d_mesh_drawable;

			/**
			 * The transform required to transform clip space to texture coordinates for
			 * the clip texture (for this tile).
			 *
			 * This is optional because it's only required if the user traverses deeper into
			 * the quad tree than our pre-generated mesh cube quad tree.
			 */
			boost::optional<GLUtils::QuadTreeClipSpaceTransform> d_clip_space_transform;


			//! Constructor for when we have a mesh quad tree node.
			explicit
			QuadTreeNode(
					const mesh_cube_quad_tree_type::node_type &mesh_node) :
				d_mesh_node(&mesh_node),
				d_mesh_drawable(mesh_node.get_element().mesh_drawable)
			{  }

			//! Constructor for when we *don't* have a mesh quad tree node - ie, deeper than the mesh tree.
			QuadTreeNode(
					const GLCompiledDrawState::non_null_ptr_to_const_type &mesh_drawable,
					const GLUtils::QuadTreeClipSpaceTransform &clip_space_transform) :
				d_mesh_node(NULL),
				d_mesh_drawable(mesh_drawable),
				d_clip_space_transform(clip_space_transform)
			{  }

			// Make a friend so can construct and access data.
			friend class GLMultiResolutionCubeMesh;
		};

		//! Typedef for a quad tree node.
		typedef QuadTreeNode quad_tree_node_type;


		/**
		 * Creates a @a GLMultiResolutionCubeMesh object.
		 */
		static
		non_null_ptr_type
		create(
				GLRenderer &renderer)
		{
			return non_null_ptr_type(new GLMultiResolutionCubeMesh(renderer));
		}


		/**
		 * Returns the quad tree root node.
		 */
		QuadTreeNode
		get_quad_tree_root_node(
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face) const;


		/**
		 * Returns the child node of specified parent node.
		 */
		QuadTreeNode
		get_child_node(
				const QuadTreeNode &parent_node,
				unsigned int child_x_offset,
				unsigned int child_y_offset) const;


		/**
		 * Returns the clip texture to use for texture clipping when needed.
		 *
		 * It's needed when QuadTreeNode::get_clip_texture_clip_space_transform() returns
		 * a valid transform (happens when traversed deeper than pre-generated mesh cube quad tree).
		 */
		GLTexture::shared_ptr_type
		get_clip_texture() const
		{
			return d_xy_clip_texture;
		}


		/**
		 * Returns the matrix that transforms clip-space [-1, 1] to the appropriate texture
		 * coordinates in the clip texture [0.25, 0.75].
		 *
		 * Texture space is [0, 1] but the clip texture is 4x4 texels with the inner 2x2 texels
		 * being white and the remaining texels being black - hence the [0.25, 0.75] range
		 * of texture coordinates maps to the white texels and the remaining area is clipped.
		 */
		GLMatrix
		get_clip_texture_clip_space_to_texture_space_transform() const
		{
			return GLTextureUtils::get_clip_texture_clip_space_to_texture_space_transform();
		}


		/**
		 * Returns the matrix that transforms clip-space [-1, 1] to the appropriate texture
		 * coordinates in the tile texture [0, 1].
		 *
		 * This differs from the clip texture in that the *full* tile texture is mapped whereas
		 * only the inner 2x2 texels of the clip texture are mapped.
		 */
		GLMatrix
		get_tile_texture_clip_space_to_texture_space_transform() const
		{
			return GLUtils::get_clip_space_to_texture_space_transform();
		}

	private:
		/**
		 * The maximum depth of the meshes cube quad tree.
		 *
		 * A value of 7 fits in nicely with the size of a 16-bit vertex element array because
		 * (1<<7) is 128 and 128x128 tiles per cube face where each tile has 4 vertices means
		 * 65536 vertices which fits exactly into 16-bit vertex indices.
		 *
		 * NOTE: 7 is quite dense so using 6 instead (still takes a lot of zoom to get to 6 so
		 * the clip texture should only be needed for high zoom levels).
		 *
		 * NOTE: 6 consumes a bit too much memory due to using a compiled draw state for each mesh
		 * drawable (adds up to a total of ~150Mb - each GLState consumes a few Kb and there are
		 * about 32,000 at level 6). Reducing to level 5 brings the memory usage down to ~40Mb.
		 */
		static const unsigned int MESH_CUBE_QUAD_TREE_MAXIMUM_DEPTH = 5;

		/**
		 * The maximum number of mesh tiles across the length of a cube face.
		 */
		static const unsigned int MESH_MAXIMUM_TILES_PER_CUBE_FACE_SIDE =
				(1 << MESH_CUBE_QUAD_TREE_MAXIMUM_DEPTH);

		/**
		 * The maximum number of mesh vertices across the length of a cube face.
		 */
		static const unsigned int MESH_MAXIMUM_VERTICES_PER_CUBE_FACE_SIDE =
				MESH_MAXIMUM_TILES_PER_CUBE_FACE_SIDE + 1;


		/**
		 * Texture used to clip parts of a mesh that hang over a tile (in the cube face x/y plane).
		 *
		 * NOTE: This is only needed when the client retrieves a tile mesh at a quad tree depth
		 * that is greater than our maximum pre-built mesh depth and hence the requested tile is
		 * smaller than the smallest tile mesh we've pre-generated. Otherwise the tile mesh itself
		 * covers the tile area exactly and no clip texture is needed.
		 */
		GLTexture::shared_ptr_type d_xy_clip_texture;

		/**
		 * All mesh drawables within a cube face share a single vertex array.
		 */
		GLVertexArray::shared_ptr_type d_meshes_vertex_array[6];

		/**
		 * The cube quad tree containing mesh drawables for the quad tree node tiles.
		 */
		mesh_cube_quad_tree_type::non_null_ptr_type d_mesh_cube_quad_tree;



		//! Constructor.
		explicit
		GLMultiResolutionCubeMesh(
				GLRenderer &renderer);

		void
		create_mesh_drawables(
				GLRenderer &renderer);

		void
		create_cube_edge_vertices(
				std::vector<GLVertex> cube_edge_vertices_array[],
				const GLVertex cube_corner_vertices[]);

		void
		create_cube_face_mesh_vertices(
				std::vector<GLVertex> &cube_face_mesh_vertices,
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
				const std::vector<GLVertex> cube_edge_vertices[]);

		void
		create_cube_face_vertex_and_index_array(
				GLRenderer &renderer,
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
				const std::vector<GLVertex> &unique_cube_face_mesh_vertices);

		void
		create_cube_face_vertex_and_index_array(
				std::vector<GLVertex> &mesh_vertices,
				std::vector<GLushort> &mesh_indices,
				const std::vector<GLVertex> &unique_cube_face_mesh_vertices,
				const GPlatesMaths::CubeQuadTreeLocation &quad_tree_node_location);

		void
		create_quad_tree_mesh_drawables(
				GLRenderer &renderer,
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face);

		mesh_cube_quad_tree_type::node_type::ptr_type
		create_quad_tree_mesh_drawables(
				GLRenderer &renderer,
				unsigned int &vertex_index,
				unsigned int &vertex_element_index,
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
				unsigned int depth);
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONCUBEMESH_H
