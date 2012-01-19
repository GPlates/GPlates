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

#ifndef GPLATES_OPENGL_GLMULTIRESOLUTIONMAPCUBEMESH_H
#define GPLATES_OPENGL_GLMULTIRESOLUTIONMAPCUBEMESH_H

#include <vector>
#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLIntersectPrimitives.h"
#include "GLMapCubeMeshGenerator.h"
#include "GLMatrix.h"
#include "GLTexture.h"
#include "GLTextureUtils.h"
#include "GLUtils.h"
#include "GLVertex.h"
#include "GLVertexArray.h"
#include "OpenGLFwd.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/MapProjection.h"

#include "maths/MathsFwd.h"
#include "maths/CubeCoordinateFrame.h"
#include "maths/CubeQuadTree.h"
#include "maths/CubeQuadTreeLocation.h"
#include "maths/UnitVector3D.h"

#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * A mesh, projected on a 2D map, that is gridded along the cube subdivision tiles.
	 */
	class GLMultiResolutionMapCubeMesh :
			public GPlatesUtils::ReferenceCount<GLMultiResolutionMapCubeMesh>
	{
	private:
		/**
		 * A 2D axis-aligned bounding box to bound the map-projected coordinates.
		 */
		struct AABB
		{
			AABB();

			void
			add(
					const GLMapCubeMeshGenerator::Point2D &point);

			//! Expand bounds to surround specified bounding box.
			void
			add(
					const AABB &aabb);

			double min_x;
			double min_y;
			double max_x;
			double max_y;
		};

		/**
		 * Information needed to render a quad tree node mesh.
		 *
		 * Previously we used @a GLCompiledDrawState for this since it's a lot easier to capture
		 * renderer state and draw calls with it. But it consumed a bit too much memory due to
		 * using a compiled draw state for each mesh drawable (adds up to a total of ~150Mb for a
		 * quad tree depth of 6 - each GLState consumes a few Kb and there are about 32,000 at level 6).
		 *
		 * Now we just store the draw parameters ourselves and submit them in a draw call when requested.
		 */
		struct MeshDrawable
		{
			MeshDrawable(
					const GLVertexArray::shared_ptr_to_const_type &vertex_array_,
					GLuint start_,
					GLuint end_,
					GLsizei count_,
					GLint indices_offset_);

			GLVertexArray::shared_ptr_to_const_type vertex_array;
			GLuint start;
			GLuint end;
			GLsizei count;
			GLint indices_offset;
		};

		/**
		 * Stores mesh information for a cube quad tree node.
		 */
		struct MeshQuadTreeNode
		{
			MeshQuadTreeNode(
					const MeshDrawable &mesh_drawable_,
					const AABB &aabb_,
					const double &max_map_projection_size_);


			MeshDrawable mesh_drawable;

			// Only need single-precision for the final bounding box (saves some memory since lots of nodes)...
			float bounding_box_centre_x;
			float bounding_box_centre_y;
			float bounding_box_half_length_x;
			float bounding_box_half_length_y;

			float max_map_projection_size;
		};

		/**
		 * Typedef for a cube quad tree with nodes containing the type @a MeshQuadTreeNode.
		 */
		typedef GPlatesMaths::CubeQuadTree<MeshQuadTreeNode> mesh_cube_quad_tree_type;

	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLMultiResolutionMapCubeMesh.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMultiResolutionMapCubeMesh> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLMultiResolutionMapCubeMesh.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMultiResolutionMapCubeMesh> non_null_ptr_to_const_type;


		/**
		 * Used during traversal of the mesh cube quad tree to obtain quad tree node meshes.
		 */
		class QuadTreeNode
		{
		public:
			/**
			 * Returns the map-projected coordinates bounding box for this quad tree node.
			 *
			 * The bounding box is oriented and 3D even though the actual bounding box is less general
			 * (it's only 2D and only axis-aligned) - however the frustum intersection code currently
			 * uses the more general 3D oriented bounding boxes.
			 */
			const GLIntersect::OrientedBoundingBox &
			get_map_projection_bounding_box() const
			{
				return d_map_projected_bounding_box;
			}

			/**
			 * Returns the maximum map projection size of any part of this quad tree node.
			 *
			 * This is the size of the mesh covered by this node in map projection space if all
			 * vertices contained in this node had the same spacing as the maximum spacing.
			 * This gives an inflated sense of size compared to the real size but helps to ensure
			 * that the texels covering that region of this node with the maximum spacing are adequate.
			 */
			float
			get_max_map_projection_size() const
			{
				return d_max_map_projection_size;
			}

			/**
			 * Renders the mesh drawable for this quad tree node.
			 *
			 * The vertices in the drawable are of type 'GLTexture3DVertex' with the map projection
			 * in the 'x' and 'y' coordinates and the point-on-sphere position in the 's', 't' and 'r'
			 * texture coordinates.
			 */
			void
			render_mesh_drawable(
					GLRenderer &renderer) const;

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
			 * Bounding box of this quad tree node and all its children (bounds map-projected coordinates).
			 *
			 * This is stored as the full 3D oriented bounding box for the client (eg, for view frustum culling).
			 */
			GLIntersect::OrientedBoundingBox d_map_projected_bounding_box;

			/**
			 * The maximum map projection size of any part of this quad tree node.
			 */
			float d_max_map_projection_size;

			/**
			 * The mesh drawable - it's a pointer instead of a reference so 'this' object can be copied.
			 */
			const MeshDrawable *d_mesh_drawable;

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
					const mesh_cube_quad_tree_type::node_type &mesh_node);

			//! Constructor for when we *don't* have a mesh quad tree node - ie, deeper than the mesh tree.
			QuadTreeNode(
					const QuadTreeNode &parent_node,
					const GLUtils::QuadTreeClipSpaceTransform &clip_space_transform);

			// Make a friend so can construct and access data.
			friend class GLMultiResolutionMapCubeMesh;
		};

		//! Typedef for a quad tree node.
		typedef QuadTreeNode quad_tree_node_type;


		/**
		 * Creates a @a GLMultiResolutionMapCubeMesh object.
		 */
		static
		non_null_ptr_type
		create(
				GLRenderer &renderer,
				const GPlatesGui::MapProjection &map_projection)
		{
			return non_null_ptr_type(new GLMultiResolutionMapCubeMesh(renderer, map_projection));
		}


		/**
		 * Updates the internal mesh if the specified map projection differs from the previous one.
		 *
		 * Returns true if an update was required.
		 */
		bool
		update_map_projection(
				GLRenderer &renderer,
				const GPlatesGui::MapProjection &map_projection);

		/**
		 * Returns the map projection settings corresponding to the internal mesh.
		 */
		const GPlatesGui::MapProjectionSettings &
		get_current_map_projection_settings() const
		{
			return d_map_projection_settings;
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
		//! Typedef for the vertex indices - 32-bit since we're likely to exceed 65536 vertices (16-bit).
		typedef GLuint vertex_element_type;


		/**
		 * The maximum depth of the meshes cube quad tree.
		 *
		 * If this depth is exceeded then clients will need to use the clip texture.
		 * This only needs to be deep enough to get reasonably good view frustum culling as the view zooms in.
		 * Too deep and it starts to use up a noticeable amount of memory.
		 *
		 * NOTE: (1 << MESH_CUBE_QUAD_TREE_MAXIMUM_DEPTH) must be less than or equal to
		 * CUBE_FACE_DIMENSION because the cube quad tree depth cannot exceed
		 * that supported by the number of vertices.
		 */
		static const unsigned int MESH_CUBE_QUAD_TREE_MAXIMUM_DEPTH = 5;

		/**
		 * The dimension of a cube face in terms of vertex spacings.
		 *
		 * NOTE: This must be a power-of-two and must be greater than or equal to
		 * (1 << MESH_CUBE_QUAD_TREE_MAXIMUM_DEPTH) because the cube quad tree depth cannot exceed
		 * that supported by the number of vertices.
		 */
		static const unsigned int CUBE_FACE_DIMENSION = 128;

		/**
		 * The number of mesh vertices across the length of a cube face.
		 */
		static const unsigned int NUM_MESH_VERTICES_PER_CUBE_FACE_SIDE = CUBE_FACE_DIMENSION + 1;

		/**
		 * The dimension of a cube face *quadrant* in terms of vertex spacings.
		 */
		static const unsigned int CUBE_FACE_QUADRANT_DIMENSION = CUBE_FACE_DIMENSION / 2;

		/**
		 * The number of mesh vertices across the length of a cube face *quadrant*.
		 */
		static const unsigned int NUM_MESH_VERTICES_PER_CUBE_FACE_QUADRANT_SIDE = CUBE_FACE_QUADRANT_DIMENSION + 1;


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

		/**
		 * The settings of the most recent map projection (used to generate internal mesh).
		 */
		GPlatesGui::MapProjectionSettings d_map_projection_settings;



		//! Constructor.
		GLMultiResolutionMapCubeMesh(
				GLRenderer &renderer,
				const GPlatesGui::MapProjection &map_projection);

		void
		create_mesh(
				GLRenderer &renderer,
				const GPlatesGui::MapProjection &map_projection);

		mesh_cube_quad_tree_type::node_type::ptr_type
		create_cube_face_mesh(
				GLRenderer &renderer,
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face,
				const GLMapCubeMeshGenerator &map_cube_mesh_generator);

		mesh_cube_quad_tree_type::node_type::ptr_type
		create_cube_face_quad_tree_mesh(
				std::vector<GLTexture3DVertex> &mesh_vertices,
				std::vector<vertex_element_type> &mesh_indices,
				AABB &parent_node_bounding_box,
				double &parent_max_quad_size_in_map_projection,
				const std::vector<GLMapCubeMeshGenerator::Point> &cube_face_quadrant_mesh_vertices,
				const unsigned int cube_face_quadrant_x_offset,
				const unsigned int cube_face_quadrant_y_offset,
				const GPlatesMaths::CubeQuadTreeLocation &quadrant_quad_tree_node_location);

		void
		create_cube_face_quad_tree_mesh_vertices(
				std::vector<GLTexture3DVertex> &mesh_vertices,
				std::vector<vertex_element_type> &mesh_indices,
				AABB &node_bounding_box,
				double &max_quad_size_in_map_projection,
				const std::vector<GLMapCubeMeshGenerator::Point> &cube_face_quadrant_mesh_vertices,
				const unsigned int cube_face_quadrant_x_offset,
				const unsigned int cube_face_quadrant_y_offset,
				const GPlatesMaths::CubeQuadTreeLocation &quad_tree_node_location);
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONMAPCUBEMESH_H
