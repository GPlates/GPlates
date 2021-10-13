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

#ifndef GPLATES_OPENGL_GLMULTIRESOLUTIONFILLEDPOLYGONS_H
#define GPLATES_OPENGL_GLMULTIRESOLUTIONFILLEDPOLYGONS_H

#include <utility> // For std::distance.
#include <vector>
#include <boost/pool/object_pool.hpp>
#include <boost/optional.hpp>
#include <boost/ref.hpp>

#include "GLClearBuffers.h"
#include "GLClearBuffersState.h"
#include "GLCubeSubdivisionCache.h"
#include "GLMultiResolutionCubeMesh.h"
#include "GLResourceManager.h"
#include "GLStateSet.h"
#include "GLTexture.h"
#include "GLTransformState.h"
#include "GLVertex.h"

#include "app-logic/ReconstructMethodFiniteRotation.h"

#include "gui/Colour.h"

#include "maths/MathsFwd.h"
#include "maths/Centroid.h"
#include "maths/CubeQuadTree.h"
#include "maths/CubeQuadTreePartition.h"
#include "maths/CubeQuadTreePartitionUtils.h"
#include "maths/UnitVector3D.h"

#include "utils/ObjectCache.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * A representation of (reconstructed) filled polygons (static or dynamic) that uses
	 * multi-resolution cube textures instead of polygons meshes.
	 *
	 * The reason for not using polygon meshes is they are expensive to compute (ie, not interactive)
	 * and hence cannot be used for dynamic topological polygons.
	 */
	class GLMultiResolutionFilledPolygons :
			public GPlatesUtils::ReferenceCount<GLMultiResolutionFilledPolygons>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLMultiResolutionFilledPolygons.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMultiResolutionFilledPolygons> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLMultiResolutionFilledPolygons.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMultiResolutionFilledPolygons> non_null_ptr_to_const_type;

		/**
		 * Typedef for a @a GLCubeSubvision cache of projection transforms.
		 */
		typedef GLCubeSubdivisionCache<
				true/*CacheProjectionTransform*/,
				false/*CacheBounds*/,
				false/*CacheLooseBounds*/>
						cube_subdivision_projection_transforms_cache_type;

		/**
		 * Typedef for a @a GLCubeSubvision cache of subdivision tile bounds.
		 */
		typedef GLCubeSubdivisionCache<
				false/*CacheProjectionTransform*/,
				true/*CacheBounds*/,
				false/*CacheLooseBounds*/>
						cube_subdivision_bounds_cache_type;

		//! Typedef for a coloured vertex.
		typedef GLColouredVertex coloured_vertex_type;


		/**
		 * Contains information to render a filled polygon.
		 */
		class FilledPolygon :
				public GPlatesUtils::ReferenceCount<FilledPolygon>
		{
		public:
			typedef GPlatesUtils::non_null_intrusive_ptr<FilledPolygon> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const FilledPolygon> non_null_ptr_to_const_type;

			/**
			 * Create a filled polygon for a @a PolygonOnSphere with an optional finite rotation transform.
			 */
			static
			non_null_ptr_type
			create(
					const GPlatesMaths::PolygonOnSphere &polygon,
					const GPlatesGui::rgba8_t &colour,
					boost::optional<GPlatesAppLogic::ReconstructMethodFiniteRotation::non_null_ptr_to_const_type>
							transform = boost::none)
			{
				return non_null_ptr_type(
						new FilledPolygon(
								polygon.vertex_begin(),
								// Use constructor with number of vertices since more efficient memory allocation...
								polygon.number_of_vertices(),
								polygon.get_centroid(),
								colour,
								transform));
			}

			/**
			 * Create a filled polygon for a @a PolylineOnSphere with an optional finite rotation transform.
			 */
			static
			non_null_ptr_type
			create(
					const GPlatesMaths::PolylineOnSphere &polyline,
					const GPlatesGui::rgba8_t &colour,
					boost::optional<GPlatesAppLogic::ReconstructMethodFiniteRotation::non_null_ptr_to_const_type>
							transform = boost::none)
			{
				return non_null_ptr_type(
						new FilledPolygon(
								polyline.vertex_begin(),
								// Use constructor with number of vertices since more efficient memory allocation...
								polyline.number_of_vertices(),
								polyline.get_centroid(),
								colour,
								transform));
			}

			/**
			 * Create from a sequence of @a UnitVector3D objects with an optional finite rotation transform.
			 *
			 * The centroid of the points is calculated internally.
			 */
			template <typename UnitVector3DForwardIter>
			static
			non_null_ptr_type
			create(
					UnitVector3DForwardIter begin_points,
					UnitVector3DForwardIter end_points,
					const GPlatesGui::rgba8_t &colour,
					boost::optional<GPlatesAppLogic::ReconstructMethodFiniteRotation::non_null_ptr_to_const_type>
							transform = boost::none)
			{
				return non_null_ptr_type(new FilledPolygon(begin_points, end_points, colour, transform));
			}


			/**
			 * Returns the drawable for rendering the filled polygons.
			 *
			 * Returns boost::none if polygon has too few points (less than three).
			 */
			const boost::optional<GLDrawable::non_null_ptr_to_const_type> &
			get_drawable() const
			{
				return d_drawable;
			}

			//! Return the optional finite rotation transform for the filled polygon.
			const boost::optional<GPlatesAppLogic::ReconstructMethodFiniteRotation::non_null_ptr_to_const_type> &
			get_transform() const
			{
				return d_transform;
			}

		private:
			//! Constructor for a sequence of @a UnitVector3D points.
			template <typename UnitVector3DForwardIter>
			FilledPolygon(
					const UnitVector3DForwardIter begin_points,
					const UnitVector3DForwardIter end_points,
					const GPlatesGui::rgba8_t &colour,
					boost::optional<GPlatesAppLogic::ReconstructMethodFiniteRotation::non_null_ptr_to_const_type> transform) :
				d_transform(transform)
			{
				// It's still better to traverse sequence first to determine number of points
				// to reserve memory for - beats re-allocating, re-copying when std::vector
				// grows for very large number of points.
				const unsigned int num_points = std::distance(begin_points, end_points);
				if (num_points < 3)
				{
					return;
				}

				// Create the OpenGL coloured vertices for the filled polygon boundary.
				std::vector<coloured_vertex_type> triangles_vertices;
				triangles_vertices.reserve(num_points + 2);
				std::vector<GLuint> triangles_vertex_indices;
				triangles_vertex_indices.reserve(3 * num_points);

				// First vertex is the centroid.
				triangles_vertices.push_back(
						coloured_vertex_type(
								GPlatesMaths::Centroid::calculate_vertices_centroid(begin_points, end_points),
								colour));

				// The remaining vertices form the boundary.
				UnitVector3DForwardIter points_iter = begin_points;
				for (unsigned int n = 0; n < num_points; ++n, ++points_iter)
				{
					triangles_vertices.push_back(coloured_vertex_type(*points_iter, colour));

					triangles_vertex_indices.push_back(0); // Centroid.
					triangles_vertex_indices.push_back(n + 1); // Current boundary point.
					triangles_vertex_indices.push_back(n + 2); // Next boundary point.
				}

				// Wraparound back to the first boundary vertex to close off the polygon.
				triangles_vertices.push_back(coloured_vertex_type(*begin_points, colour));

				// Create the filled polygon drawable.
				initialise_drawable(triangles_vertices, triangles_vertex_indices);
			}

			//! Constructor for a sequence of @a PointOnSphere points.
			template <typename PointOnSphereForwardIter>
			FilledPolygon(
					const PointOnSphereForwardIter begin_points,
					const unsigned int num_points,
					const GPlatesMaths::UnitVector3D &centroid,
					const GPlatesGui::rgba8_t &colour,
					boost::optional<GPlatesAppLogic::ReconstructMethodFiniteRotation::non_null_ptr_to_const_type> transform) :
				d_transform(transform)
			{
				if (num_points < 3)
				{
					return;
				}

				// Create the OpenGL coloured vertices for the filled polygon boundary.
				std::vector<coloured_vertex_type> triangles_vertices;
				triangles_vertices.reserve(num_points + 2);
				std::vector<GLuint> triangles_vertex_indices;
				triangles_vertex_indices.reserve(3 * num_points);

				// First vertex is the centroid.
				triangles_vertices.push_back(coloured_vertex_type(centroid, colour));

				// The remaining vertices form the boundary.
				PointOnSphereForwardIter points_iter = begin_points;
				for (unsigned int n = 0; n < num_points; ++n, ++points_iter)
				{
					triangles_vertices.push_back(
							coloured_vertex_type(points_iter->position_vector(), colour));

					triangles_vertex_indices.push_back(0); // Centroid.
					triangles_vertex_indices.push_back(n + 1); // Current boundary point.
					triangles_vertex_indices.push_back(n + 2); // Next boundary point.
				}

				// Wraparound back to the first boundary vertex to close off the polygon.
				triangles_vertices.push_back(
						coloured_vertex_type(begin_points->position_vector(), colour));

				// Create the filled polygon drawable.
				initialise_drawable(triangles_vertices, triangles_vertex_indices);
			}


			/**
			 * The filled polygon drawable as a sequence of triangles formed between each
			 * boundary edge and centroid.
			 *
			 * It's a boost::optional, however it's always valid unless there's too few points to form a polygon.
			 */
			boost::optional<GLDrawable::non_null_ptr_to_const_type> d_drawable;

			/**
			 * Optional finite rotation transform for the filled polygon.
			 */
			boost::optional<GPlatesAppLogic::ReconstructMethodFiniteRotation::non_null_ptr_to_const_type> d_transform;


			void
			initialise_drawable(
					const std::vector<coloured_vertex_type> &triangles_vertices,
					const std::vector<GLuint> &triangles_vertex_indices);
		};

		//! Typedef for a filled polygon.
		typedef FilledPolygon filled_polygon_type;

		//! Typedef for a spatial partition of filled polygons.
		typedef GPlatesMaths::CubeQuadTreePartition<filled_polygon_type::non_null_ptr_type>
				filled_polygons_spatial_partition_type;


		/**
		 * Creates a @a GLMultiResolutionFilledPolygons object.
		 */
		static
		non_null_ptr_type
		create(
				const GLMultiResolutionCubeMesh::non_null_ptr_to_const_type &multi_resolution_cube_mesh,
				const cube_subdivision_projection_transforms_cache_type::non_null_ptr_type &
						cube_subdivision_projection_transforms_cache,
				const cube_subdivision_bounds_cache_type::non_null_ptr_type &cube_subdivision_bounds_cache,
				const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
				const GLVertexBufferResourceManager::shared_ptr_type &vertex_buffer_resource_manager)
		{
			return non_null_ptr_type(
					new GLMultiResolutionFilledPolygons(
							multi_resolution_cube_mesh,
							cube_subdivision_projection_transforms_cache,
							cube_subdivision_bounds_cache,
							texture_resource_manager,
							vertex_buffer_resource_manager));
		}


		/**
		 * Renders the specified filled polygons (spatial partition) using the current
		 * transform state of @a renderer.
		 */
		void
		render(
				GLRenderer &renderer,
				const filled_polygons_spatial_partition_type &filled_polygons);

	private:
		/**
		 * Used to cache information specific to a quad tree node.
		 */
		struct QuadTreeNode
		{
		};

		/**
		 * Typedef for a cube quad tree with nodes containing the type @a QuadTreeNode.
		 */
		typedef GPlatesMaths::CubeQuadTree<QuadTreeNode> cube_quad_tree_type;

		//! Typedef for a quad tree node of a multi-resolution cube mesh.
		typedef GLMultiResolutionCubeMesh::quad_tree_node_type mesh_quad_tree_node_type;

		//! Typedef for a structure that determines which nodes of a spatial partition intersect a regular cube quad tree.
		typedef GPlatesMaths::CubeQuadTreePartitionUtils::CubeQuadTreeIntersectingNodes<filled_polygon_type::non_null_ptr_type>
				filled_polygons_intersecting_nodes_type;

		//! A linked list node that references a spatial partition node of reconstructed polygon meshes.
		struct FilledPolygonsListNode :
				public GPlatesUtils::IntrusiveSinglyLinkedList<FilledPolygonsListNode>::Node
		{
			//! Default constructor.
			FilledPolygonsListNode()
			{  }

			explicit
			FilledPolygonsListNode(
					filled_polygons_spatial_partition_type::const_node_reference_type node_reference_) :
				node_reference(node_reference_)
			{  }

			filled_polygons_spatial_partition_type::const_node_reference_type node_reference;
		};

		//! Typedef for a list of spatial partition nodes referencing reconstructed polygon meshes.
		typedef GPlatesUtils::IntrusiveSinglyLinkedList<FilledPolygonsListNode> 
				filled_polygons_spatial_partition_node_list_type;

		//! Typedef for a sequence of filled polygons.
		typedef std::vector<filled_polygon_type::non_null_ptr_to_const_type> filled_polygon_seq_type;


		/**
		 * Used to get projection transforms as the cube quad tree is traversed.
		 */
		cube_subdivision_projection_transforms_cache_type::non_null_ptr_type
				d_cube_subdivision_projection_transforms_cache;

		/**
		 * Used to get node bounds for view-frustum culling as the cube quad tree is traversed.
		 */
		cube_subdivision_bounds_cache_type::non_null_ptr_type d_cube_subdivision_bounds_cache;

		/**
		 * Used to allocate any textures we need.
		 */
		GLTextureResourceManager::shared_ptr_type d_texture_resource_manager;

		/**
		 * Used when allocating vertex arrays.
		 */
		GLVertexBufferResourceManager::shared_ptr_type d_vertex_buffer_resource_manager;

		/**
		 * Cache of tile textures - the textures are not reused over frames though.
		 */
		GPlatesUtils::ObjectCache<GLTexture>::shared_ptr_type d_texture_cache;

		/**
		 * Used to render each filled polygon as a stencil.
		 */
		GLTexture::shared_ptr_to_const_type d_polygon_stencil_texture;

		/**
		 * The texture dimension of a cube quad tree tile.
		 */
		unsigned int d_tile_texel_dimension;

		/**
		 * The texture dimension of the single polygon stencil rendering texture.
		 *
		 * This is ideally much larger than the cube quad tree node tile textures to
		 * minimise render target switching.
		 */
		unsigned int d_polygon_stencil_texel_dimension;

		// Used to draw a textured full-screen quad into render texture.
		GLVertexArrayDrawable::non_null_ptr_type d_full_screen_quad_drawable;

		//
		// Used to clear the render target colour buffer.
		//
		GLClearBuffersState::non_null_ptr_type d_clear_buffers_state;
		GLClearBuffers::non_null_ptr_type d_clear_buffers;

		/**
		 * Contains meshes for each cube quad tree node.
		 */
		GLMultiResolutionCubeMesh::non_null_ptr_to_const_type d_multi_resolution_cube_mesh;


		//! Constructor.
		GLMultiResolutionFilledPolygons(
				const GLMultiResolutionCubeMesh::non_null_ptr_to_const_type &multi_resolution_cube_mesh,
				const cube_subdivision_projection_transforms_cache_type::non_null_ptr_type &
						cube_subdivision_projection_transforms_cache,
				const cube_subdivision_bounds_cache_type::non_null_ptr_type &cube_subdivision_bounds_cache,
				const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
				const GLVertexBufferResourceManager::shared_ptr_type &vertex_buffer_resource_manager);


		unsigned int
		get_level_of_detail(
				const GLTransformState &transform_state) const;

		void
		render_quad_tree(
				GLRenderer &renderer,
				const mesh_quad_tree_node_type &mesh_quad_tree_node,
				const filled_polygons_spatial_partition_type &filled_polygons_spatial_partition,
				const filled_polygons_spatial_partition_node_list_type &parent_filled_polygons_intersecting_node_list,
				const filled_polygons_intersecting_nodes_type &filled_polygons_intersecting_nodes,
				const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node,
				const cube_subdivision_bounds_cache_type::node_reference_type &bounds_cache_node,
				unsigned int level_of_detail,
				unsigned int render_level_of_detail,
				const GLFrustum &frustum_planes,
				boost::uint32_t frustum_plane_mask);

		void
		render_quad_tree_node(
				GLRenderer &renderer,
				const mesh_quad_tree_node_type &mesh_quad_tree_node,
				const filled_polygons_spatial_partition_type &
						reconstructed_polygon_meshes_spatial_partition,
				const filled_polygons_spatial_partition_node_list_type &
						parent_reconstructed_polygon_meshes_intersecting_node_list,
				const filled_polygons_intersecting_nodes_type &
						reconstructed_polygon_meshes_intersecting_nodes,
				const cube_subdivision_projection_transforms_cache_type::node_reference_type &
						projection_transforms_cache_node);

		void
		get_filled_polygons_intersecting_nodes(
				const GPlatesMaths::CubeQuadTreeLocation &source_raster_tile_location,
				const GPlatesMaths::CubeQuadTreeLocation &intersecting_node_location,
				filled_polygons_spatial_partition_type::const_node_reference_type intersecting_node_reference,
				filled_polygons_spatial_partition_node_list_type &intersecting_node_list,
				boost::object_pool<FilledPolygonsListNode> &intersecting_list_node_pool);

		GLStateSet::non_null_ptr_to_const_type
		create_tile_state_set(
				const GLTexture::shared_ptr_to_const_type &tile_texture,
				const GLTransform &projection_transform,
				const GLTransform &view_transform);

		void
		render_tile_to_scene(
				GLRenderer &renderer,
				const mesh_quad_tree_node_type &mesh_quad_tree_node,
				const filled_polygons_spatial_partition_type &filled_polygons_spatial_partition,
				const filled_polygons_spatial_partition_node_list_type &filled_polygons_intersecting_node_list,
				const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node);

		void
		render_filled_polygons_to_tile_texture(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &tile_texture,
				const filled_polygon_seq_type &transformed_sorted_filled_drawables,
				const GLTransform &projection_transform,
				const GLTransform &view_transform);

		void
		render_filled_polygons_to_polygon_stencil_texture(
				GLRenderer &renderer,
				const filled_polygon_seq_type::const_iterator begin_filled_drawables,
				const filled_polygon_seq_type::const_iterator end_filled_drawables,
				const GLTransform &projection_transform,
				const GLTransform &view_transform);

		void
		render_filled_polygons_from_polygon_stencil_texture(
				GLRenderer &renderer,
				const filled_polygon_seq_type::const_iterator begin_filled_drawables,
				const filled_polygon_seq_type::const_iterator end_filled_drawables);

		void
		get_filled_polygons(
				filled_polygon_seq_type &transformed_sorted_filled_drawables,
				filled_polygons_spatial_partition_type::element_const_iterator begin_root_filled_polygons,
				filled_polygons_spatial_partition_type::element_const_iterator end_root_filled_polygons,
				const filled_polygons_spatial_partition_node_list_type &filled_polygons_intersecting_node_list);

		void
		add_filled_polygons(
				filled_polygon_seq_type &transform_sorted_filled_drawables,
				filled_polygons_spatial_partition_type::element_const_iterator begin_filled_polygons,
				filled_polygons_spatial_partition_type::element_const_iterator end_filled_polygons);

		GLTexture::shared_ptr_to_const_type
		allocate_tile_texture();

		void
		create_tile_texture(
				const GLTexture::shared_ptr_type &texture);

		void
		create_polygon_stencil_texture();
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONFILLEDPOLYGONS_H
