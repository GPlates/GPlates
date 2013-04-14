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

#ifndef GPLATES_OPENGL_GLFILLEDPOLYGONSGLOBEVIEW_H
#define GPLATES_OPENGL_GLFILLEDPOLYGONSGLOBEVIEW_H

#include <utility> // For std::distance.
#include <vector>
#include <boost/pool/object_pool.hpp>
#include <boost/optional.hpp>
#include <boost/ref.hpp>
#include <opengl/OpenGL.h>

#include "GLBuffer.h"
#include "GLCompiledDrawState.h"
#include "GLCubeSubdivisionCache.h"
#include "GLLight.h"
#include "GLMatrix.h"
#include "GLMultiResolutionCubeMesh.h"
#include "GLProgramObject.h"
#include "GLTexture.h"
#include "GLVertex.h"
#include "GLVertexArray.h"
#include "GLVertexBuffer.h"
#include "GLVertexElementBuffer.h"
#include "GLViewport.h"

#include "gui/Colour.h"

#include "maths/MathsFwd.h"
#include "maths/Centroid.h"
#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/CubeQuadTree.h"
#include "maths/CubeQuadTreePartition.h"
#include "maths/CubeQuadTreePartitionUtils.h"
#include "maths/UnitQuaternion3D.h"
#include "maths/UnitVector3D.h"

#include "utils/Profile.h"
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
	class GLFilledPolygonsGlobeView :
			public GPlatesUtils::ReferenceCount<GLFilledPolygonsGlobeView>
	{
	private:

		//! Typedef for a vertex element (vertex index) of a polygon.
		typedef GLuint polygon_vertex_element_type;

		//! Typedef for a coloured vertex of a polygon.
		typedef GLColourVertex polygon_vertex_type;

		/**
		 * Contains information to render a filled polygon.
		 */
		struct FilledPolygon
		{
			/**
			 * Contains 'gl_draw_range_elements' parameters that locate a geometry inside a vertex array.
			 */
			struct Drawable
			{
				Drawable(
						GLuint start_,
						GLuint end_,
						GLsizei count_,
						GLint indices_offset_) :
					start(start_),
					end(end_),
					count(count_),
					indices_offset(indices_offset_)
				{  }

				GLuint start;
				GLuint end;
				GLsizei count;
				GLint indices_offset;
			};


			//! Create a filled polygon from a polygon (fan) mesh drawable.
			explicit
			FilledPolygon(
					const Drawable &polygon_mesh_drawable,
					unsigned int render_order) :
				d_polygon_mesh_drawable(polygon_mesh_drawable),
				d_render_order(render_order)
			{  }


			/**
			 * The filled polygon's (fan) mesh.
			 */
			Drawable d_polygon_mesh_drawable;

			/**
			 * The order in which this drawable should be rendered relative to other drawables.
			 */
			unsigned int d_render_order;

			//! Used to sort by render order.
			struct SortRenderOrder
			{
				bool
				operator()(
						const FilledPolygon &lhs,
						const FilledPolygon &rhs) const
				{
					return lhs.d_render_order < rhs.d_render_order;
				}
			};
		};

		//! Typedef for a filled polygon.
		typedef FilledPolygon filled_polygon_type;

		//! Typedef for a spatial partition of filled polygons.
		typedef GPlatesMaths::CubeQuadTreePartition<filled_polygon_type> filled_polygons_spatial_partition_type;

	public:

		//! A convenience typedef for a shared pointer to a non-const @a GLFilledPolygonsGlobeView.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLFilledPolygonsGlobeView> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLFilledPolygonsGlobeView.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLFilledPolygonsGlobeView> non_null_ptr_to_const_type;

		/**
		 * Used to accumulate filled polygons (optionally as a spatial partition) for rendering.
		 */
		class FilledPolygons
		{
		public:
			//! Default constructor.
			FilledPolygons() :
				d_filled_polygons_spatial_partition(
						filled_polygons_spatial_partition_type::create(MAX_SPATIAL_PARTITION_DEPTH))
			{  }

			/**
			 * Create a filled polygon from a @a PolygonOnSphere.
			 */
			void
			add_filled_polygon(
					const GPlatesMaths::PolygonOnSphere &polygon,
					const GPlatesGui::Colour &colour,
					const boost::optional<GPlatesMaths::CubeQuadTreeLocation> &cube_quad_tree_location = boost::none)
			{
				// Alpha blending will be set up for pre-multiplied alpha.
				const GPlatesGui::Colour pre_multiplied_alpha_colour(
						colour.red() * colour.alpha(),
						colour.green() * colour.alpha(),
						colour.blue() * colour.alpha(),
						colour.alpha());

				const boost::optional<filled_polygon_type> filled_polygon =
						add_filled_polygon_mesh(
								polygon.vertex_begin(),
								polygon.number_of_vertices(),
								polygon.get_centroid(),
								GPlatesGui::Colour::to_rgba8(pre_multiplied_alpha_colour));
				if (filled_polygon)
				{
					add_filled_polygon(filled_polygon.get(), cube_quad_tree_location);
				}
			}

			/**
			 * Create a filled polygon from a @a PolylineOnSphere.
			 *
			 * A polygon is formed by closing the first and last points of the polyline.
			 * Note that if the geometry has too few points then it simply won't be used to render the filled polygon.
			 */
			void
			add_filled_polygon(
					const GPlatesMaths::PolylineOnSphere &polyline,
					const GPlatesGui::Colour &colour,
					const boost::optional<GPlatesMaths::CubeQuadTreeLocation> &cube_quad_tree_location = boost::none)
			{
				// Alpha blending will be set up for pre-multiplied alpha.
				const GPlatesGui::Colour pre_multiplied_alpha_colour(
						colour.red() * colour.alpha(),
						colour.green() * colour.alpha(),
						colour.blue() * colour.alpha(),
						colour.alpha());

				const boost::optional<filled_polygon_type> filled_polygon =
						add_filled_polygon_mesh(
								polyline.vertex_begin(),
								polyline.number_of_vertices(),
								polyline.get_centroid(),
								GPlatesGui::Colour::to_rgba8(pre_multiplied_alpha_colour));
				if (filled_polygon)
				{
					add_filled_polygon(filled_polygon.get(), cube_quad_tree_location);
				}
			}

			/**
			 * Returns true if any filled polygons have been added.
			 */
			bool
			empty() const
			{
				return d_filled_polygons_spatial_partition->empty();
			}

			/**
			 * Clears the filled polygons accumulated so far.
			 *
			 * This is more efficient than creating a new @a FilledPolygons each render since it
			 * minimises re-allocations.
			 */
			void
			clear()
			{
				d_filled_polygons_spatial_partition->clear();
				d_polygon_vertices.clear();
				d_polygon_vertex_elements.clear();
			}

		private:
			/**
			 * We don't need to go too deep - as deep as the multi-resolution cube mesh is good enough.
			 */
			static const unsigned int MAX_SPATIAL_PARTITION_DEPTH = 6;


			/**
			 * The spatial partition of filled polygons.
			 */
			filled_polygons_spatial_partition_type::non_null_ptr_type d_filled_polygons_spatial_partition;

			/**
			 * The vertices of all polygons of the current @a render call.
			 *
			 * NOTE: This is only 'clear'ed at each render call in order to avoid excessive re-allocations
			 * at each @a render call (std::vector::clear shouldn't deallocate).
			 */
			std::vector<polygon_vertex_type> d_polygon_vertices;

			/**
			 * The vertex elements (indices) of all polygons of the current @a render call.
			 *
			 * NOTE: This is only 'clear'ed at each render call in order to avoid excessive re-allocations
			 * at each @a render call (std::vector::clear shouldn't deallocate).
			 */
			std::vector<polygon_vertex_element_type> d_polygon_vertex_elements;

			// So can access accumulated vertices/indices and spatial partition of filled polygons.
			friend class GLFilledPolygonsGlobeView;


			//! Adds the filled polygon to the spatial partition.
			void
			add_filled_polygon(
					const filled_polygon_type &filled_polygon,
					const boost::optional<GPlatesMaths::CubeQuadTreeLocation> &cube_quad_tree_location)
			{
				if (cube_quad_tree_location)
				{
					d_filled_polygons_spatial_partition->add(filled_polygon, cube_quad_tree_location.get());
				}
				else
				{
					d_filled_polygons_spatial_partition->add_unpartitioned(filled_polygon);
				}
			}


			//! Adds a sequence of @a PointOnSphere points as vertices/indices in global vertex array.
			template <typename PointOnSphereForwardIter>
			boost::optional<filled_polygon_type>
			add_filled_polygon_mesh(
					const PointOnSphereForwardIter begin_points,
					const unsigned int num_points,
					const GPlatesMaths::UnitVector3D &centroid,
					const GPlatesGui::rgba8_t &colour)
			{
				// Need at least three points for a polygon.
				if (num_points < 3)
				{
					return boost::none;
				}

				//
				// Create the OpenGL coloured vertices for the filled polygon (fan) mesh.
				//

				const GLsizei base_vertex_element_index = d_polygon_vertex_elements.size();
				const polygon_vertex_element_type base_vertex_index = d_polygon_vertices.size();
				polygon_vertex_element_type vertex_index = base_vertex_index;

				// First vertex is the centroid.
				d_polygon_vertices.push_back(polygon_vertex_type(centroid, colour));
				++vertex_index;

				// The remaining vertices form the boundary.
				PointOnSphereForwardIter points_iter = begin_points;
				for (unsigned int n = 0; n < num_points; ++n, ++vertex_index, ++points_iter)
				{
					d_polygon_vertices.push_back(polygon_vertex_type(points_iter->position_vector(), colour));

					d_polygon_vertex_elements.push_back(base_vertex_index); // Centroid.
					d_polygon_vertex_elements.push_back(vertex_index); // Current boundary point.
					d_polygon_vertex_elements.push_back(vertex_index + 1); // Next boundary point.
				}

				// Wraparound back to the first boundary vertex to close off the polygon.
				d_polygon_vertices.push_back(
						polygon_vertex_type(begin_points->position_vector(), colour));

				// Create the filled polygon (fan) mesh drawable.
				const filled_polygon_type::Drawable polygon_mesh_drawable(
						base_vertex_index/*start*/,
						vertex_index/*end*/, // ...the last vertex index
						d_polygon_vertex_elements.size() - base_vertex_element_index /*count*/,
						base_vertex_element_index * sizeof(polygon_vertex_element_type)/*indices_offset*/);

				// Keep track of the order to render the drawables (order in which we're called)
				// because drawables are rendered by visiting the spatial partition which is not
				// the same as the original draw order.
				const unsigned int render_order = d_filled_polygons_spatial_partition->size();

				return filled_polygon_type(polygon_mesh_drawable, render_order);
			}
		};

		//! Typedef for a group of filled polygons.
		typedef FilledPolygons filled_polygons_type;


		/**
		 * Creates a @a GLFilledPolygonsGlobeView object.
		 *
		 * NOTE: If @a light is specified but lighting is not supported on the run-time system
		 * (eg, due to exceeding shader instructions limit) then filled polygons will not be rendered
		 * with surface lighting.
		 */
		static
		non_null_ptr_type
		create(
				GLRenderer &renderer,
				const GLMultiResolutionCubeMesh::non_null_ptr_to_const_type &multi_resolution_cube_mesh,
				boost::optional<GLLight::non_null_ptr_type> light = boost::none)
		{
			return non_null_ptr_type(
					new GLFilledPolygonsGlobeView(
							renderer, multi_resolution_cube_mesh, light));
		}


		/**
		 * Renders the specified filled polygons (spatial partition).
		 */
		void
		render(
				GLRenderer &renderer,
				const filled_polygons_type &filled_polygons);

	private:

		/**
		 * The maximum tile size for rendering filled polygons.
		 *
		 * The bigger this is the fewer times the filled polygons needed to be drawn.
		 * But too big and starts to consume too much memory.
		 * Each pixel is 8 bytes (4 bytes for colour and 4 bytes for combined depth/stencil buffer).
		 *
		 * NOTE: Using *signed* integer to avoid compiler error on 64-bit Mac (not sure why it's happening).
		 */
		static const int MAX_TILE_TEXEL_DIMENSION = 1024;

		/**
		 * The minimum tile size for rendering filled polygons.
		 *
		 * NOTE: Using *signed* integer to avoid compiler error on 64-bit Mac (not sure why it's happening).
		 */
		static const int MIN_TILE_TEXEL_DIMENSION = 256;


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
		typedef GLMultiResolutionCubeMesh::quad_tree_node_type mesh_quad_tree_node_type;

		//! Typedef for a structure that determines which nodes of a spatial partition intersect a regular cube quad tree.
		typedef GPlatesMaths::CubeQuadTreePartitionUtils::CubeQuadTreeIntersectingNodes<filled_polygon_type>
				filled_polygons_intersecting_nodes_type;

		//! A linked list node that references a spatial partition node of reconstructed polygon mesh drawables.
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
		typedef std::vector<filled_polygon_type> filled_polygon_seq_type;


		/**
		 * The maximum texture dimension of a cube quad tree tile.
		 */
		unsigned int d_max_tile_texel_dimension;

		/**
		 * The minimum texture dimension of a cube quad tree tile.
		 */
		unsigned int d_min_tile_texel_dimension;

		/**
		 * The vertex buffer containing the vertices of all polygons of the current @a render call.
		 */
		GLVertexBuffer::shared_ptr_type d_polygons_vertex_buffer;

		/**
		 * The vertex buffer containing the vertex elements (indices) of all polygons of the current @a render call.
		 */
		GLVertexElementBuffer::shared_ptr_type d_polygons_vertex_element_buffer;

		/**
		 * The vertex array containing all polygons of the current @a render call.
		 *
		 * All polygons for the current @a render call are stored here.
		 * They'll get flushed/replaced when the next render call is made.
		 */
		GLVertexArray::shared_ptr_type d_polygons_vertex_array;

		/**
		 * Contains meshes for each cube quad tree node.
		 */
		GLMultiResolutionCubeMesh::non_null_ptr_to_const_type d_multi_resolution_cube_mesh;


		/**
		 * The light (direction) used during surface lighting.
		 */
		boost::optional<GLLight::non_null_ptr_type> d_light;

		/**
		 * Shader program to render tiles to the scene (the final stage).
		 *
		 * Is boost::none if shader programs not supported (in which case fixed-function pipeline is used).
		 */
		boost::optional<GLProgramObject::shared_ptr_type> d_render_tile_to_scene_program_object;

		/**
		 * Shader program to render tiles to the scene (the final stage) with clipping.
		 *
		 * Is boost::none if shader programs not supported (in which case fixed-function pipeline is used).
		 */
		boost::optional<GLProgramObject::shared_ptr_type> d_render_tile_to_scene_with_clipping_program_object;

		/**
		 * Shader program to render tiles to the scene (the final stage) with lighting.
		 *
		 * Is boost::none if shader programs not supported (in which case fixed-function pipeline is used).
		 */
		boost::optional<GLProgramObject::shared_ptr_type> d_render_tile_to_scene_with_lighting_program_object;

		/**
		 * Shader program to render tiles to the scene (the final stage) with clipping and lighting.
		 *
		 * Is boost::none if shader programs not supported (in which case fixed-function pipeline is used).
		 */
		boost::optional<GLProgramObject::shared_ptr_type> d_render_tile_to_scene_with_clipping_and_lighting_program_object;


		//! Constructor.
		GLFilledPolygonsGlobeView(
				GLRenderer &renderer,
				const GLMultiResolutionCubeMesh::non_null_ptr_to_const_type &multi_resolution_cube_mesh,
				boost::optional<GLLight::non_null_ptr_type> light);

		unsigned int
		get_level_of_detail(
				unsigned int &tile_texel_dimension,
				const GLViewport &viewport,
				const GLMatrix &model_view_transform,
				const GLMatrix &projection_transform) const;

		void
		render_quad_tree(
				GLRenderer &renderer,
				unsigned int tile_texel_dimension,
				const mesh_quad_tree_node_type &mesh_quad_tree_node,
				const filled_polygons_type &filled_polygons,
				const filled_polygons_spatial_partition_node_list_type &parent_filled_polygons_intersecting_node_list,
				const filled_polygons_intersecting_nodes_type &filled_polygons_intersecting_nodes,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
				clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
				const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
				unsigned int level_of_detail,
				unsigned int render_level_of_detail,
				const GLFrustum &frustum_planes,
				boost::uint32_t frustum_plane_mask);

		void
		render_quad_tree_node(
				GLRenderer &renderer,
				unsigned int tile_texel_dimension,
				const mesh_quad_tree_node_type &mesh_quad_tree_node,
				const filled_polygons_type &filled_polygons,
				const filled_polygons_spatial_partition_node_list_type &
						parent_reconstructed_polygon_meshes_intersecting_node_list,
				const filled_polygons_intersecting_nodes_type &
						reconstructed_polygon_meshes_intersecting_nodes,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
				clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
				const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node);

		void
		get_filled_polygons_intersecting_nodes(
				const GPlatesMaths::CubeQuadTreeLocation &source_raster_tile_location,
				const GPlatesMaths::CubeQuadTreeLocation &intersecting_node_location,
				filled_polygons_spatial_partition_type::const_node_reference_type intersecting_node_reference,
				filled_polygons_spatial_partition_node_list_type &intersecting_node_list,
				boost::object_pool<FilledPolygonsListNode> &intersecting_list_node_pool);

		void
		set_tile_state(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &tile_texture,
				const GLTransform &projection_transform,
				const GLTransform &clip_projection_transform,
				const GLTransform &view_transform,
				bool clip_to_tile_frustum);

		void
		render_tile_to_scene(
				GLRenderer &renderer,
				unsigned int tile_texel_dimension,
				const mesh_quad_tree_node_type &mesh_quad_tree_node,
				const filled_polygons_type &filled_polygons,
				const filled_polygons_spatial_partition_node_list_type &filled_polygons_intersecting_node_list,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
				clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
				const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node);

		void
		render_filled_polygons_to_tile_texture(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &tile_texture,
				const filled_polygon_seq_type &transformed_sorted_filled_drawables,
				const GLTransform &projection_transform,
				const GLTransform &view_transform);

		void
		get_filled_polygons(
				filled_polygon_seq_type &filled_drawables,
				filled_polygons_spatial_partition_type::element_const_iterator begin_root_filled_polygons,
				filled_polygons_spatial_partition_type::element_const_iterator end_root_filled_polygons,
				const filled_polygons_spatial_partition_node_list_type &filled_polygons_intersecting_node_list);

		GLTexture::shared_ptr_type
		acquire_tile_texture(
				GLRenderer &renderer,
				unsigned int tile_texel_dimension);

		void
		create_polygons_vertex_array(
				GLRenderer &renderer);

		void
		write_filled_polygon_meshes_to_vertex_array(
				GLRenderer &renderer,
				const filled_polygons_type &filled_polygons);

		void
		create_shader_programs(
				GLRenderer &renderer);
	};
}

#endif // GPLATES_OPENGL_GLFILLEDPOLYGONSGLOBEVIEW_H
