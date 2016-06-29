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
#include <boost/utility/in_place_factory.hpp>
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

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

#include "gui/Colour.h"

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

		//! Typedef for a vertex element (vertex index) of a drawable.
		typedef GLuint drawable_vertex_element_type;

		//! Typedef for a coloured vertex of a drawable.
		typedef GLColourVertex drawable_vertex_type;

		/**
		 * Contains information to render a filled drawable.
		 */
		struct FilledDrawable
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


			//! Create a filled drawable.
			explicit
			FilledDrawable(
					const Drawable &drawable,
					unsigned int render_order) :
				d_drawable(drawable),
				d_render_order(render_order)
			{  }


			/**
			 * The filled drawable's mesh.
			 */
			Drawable d_drawable;

			/**
			 * The order in which this drawable should be rendered relative to other drawables.
			 */
			unsigned int d_render_order;

			//! Used to sort by render order.
			struct SortRenderOrder
			{
				bool
				operator()(
						const FilledDrawable &lhs,
						const FilledDrawable &rhs) const
				{
					return lhs.d_render_order < rhs.d_render_order;
				}
			};
		};

		//! Typedef for a filled drawable.
		typedef FilledDrawable filled_drawable_type;

		//! Typedef for a spatial partition of filled drawables.
		typedef GPlatesMaths::CubeQuadTreePartition<filled_drawable_type> filled_drawables_spatial_partition_type;

	public:

		//! A convenience typedef for a shared pointer to a non-const @a GLFilledPolygonsGlobeView.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLFilledPolygonsGlobeView> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLFilledPolygonsGlobeView.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLFilledPolygonsGlobeView> non_null_ptr_to_const_type;

		/**
		 * Used to accumulate filled drawables (optionally as a spatial partition) for rendering.
		 */
		class FilledDrawables
		{
		public:
			//! Default constructor.
			FilledDrawables() :
				d_filled_drawables_spatial_partition(
						filled_drawables_spatial_partition_type::create(MAX_SPATIAL_PARTITION_DEPTH))
			{  }

			/**
			 * Returns true if any filled drawables have been added.
			 */
			bool
			empty() const
			{
				return d_filled_drawables_spatial_partition->empty();
			}

			/**
			 * Clears the filled drawables accumulated so far.
			 *
			 * This is more efficient than creating a new @a FilledDrawables each render since it
			 * minimises re-allocations.
			 */
			void
			clear()
			{
				d_filled_drawables_spatial_partition->clear();
				d_drawable_vertices.clear();
				d_drawable_vertex_elements.clear();
				d_current_drawable = boost::none;
			}

			/**
			 * Create a filled polygon from a @a PolygonOnSphere.
			 */
			void
			add_filled_polygon(
					const GPlatesMaths::PolygonOnSphere &polygon,
					GPlatesGui::rgba8_t rgba8_color,
					const boost::optional<GPlatesMaths::CubeQuadTreeLocation> &cube_quad_tree_location = boost::none);

			/**
			 * Create a filled polygon from a @a PolylineOnSphere.
			 *
			 * A polygon is formed by closing the first and last points of the polyline.
			 * Note that if the geometry has too few points then it simply won't be used to render the filled polygon.
			 */
			void
			add_filled_polygon(
					const GPlatesMaths::PolylineOnSphere &polyline,
					GPlatesGui::rgba8_t rgba8_color,
					const boost::optional<GPlatesMaths::CubeQuadTreeLocation> &cube_quad_tree_location = boost::none);

			/**
			 * Begins a single drawable for a filled mesh composed of individually added triangles.
			 */
			void
			begin_filled_triangle_mesh()
			{
				begin_filled_drawable();
			}

			/**
			 * Ends the current filled triangle mesh drawable (started by @a begin_filled_triangle_mesh).
			 */
			void
			end_filled_triangle_mesh(
					const boost::optional<GPlatesMaths::CubeQuadTreeLocation> &cube_quad_tree_location = boost::none)
			{
				end_filled_drawable(cube_quad_tree_location);
			}

			/**
			 * Adds a coloured triangle to the current filled triangle mesh drawable.
			 *
			 * This must be called between @a begin_filled_triangle_mesh and @a end_filled_triangle_mesh.
			 */
			void
			add_filled_triangle_to_mesh(
					const GPlatesMaths::PointOnSphere &vertex1,
					const GPlatesMaths::PointOnSphere &vertex2,
					const GPlatesMaths::PointOnSphere &vertex3,
					GPlatesGui::rgba8_t rgba8_color);

		private:
			/**
			 * We don't need to go too deep - as deep as the multi-resolution cube mesh is good enough.
			 */
			static const unsigned int MAX_SPATIAL_PARTITION_DEPTH = 6;


			/**
			 * The spatial partition of filled drawables.
			 */
			filled_drawables_spatial_partition_type::non_null_ptr_type d_filled_drawables_spatial_partition;

			/**
			 * The vertices of all drawables of the current @a render call.
			 *
			 * NOTE: This is only 'clear'ed at each render call in order to avoid excessive re-allocations
			 * at each @a render call (std::vector::clear shouldn't deallocate).
			 */
			std::vector<drawable_vertex_type> d_drawable_vertices;

			/**
			 * The vertex elements (indices) of all drawables of the current @a render call.
			 *
			 * NOTE: This is only 'clear'ed at each render call in order to avoid excessive re-allocations
			 * at each @a render call (std::vector::clear shouldn't deallocate).
			 */
			std::vector<drawable_vertex_element_type> d_drawable_vertex_elements;

			/**
			 * The current drawable.
			 *
			 * Is only valid between @a begin_filled_drawable and @a end_filled_drawable.
			 */
			boost::optional<FilledDrawable::Drawable> d_current_drawable;


			// So can access accumulated vertices/indices and spatial partition of filled drawables.
			friend class GLFilledPolygonsGlobeView;


			/**
			 * Begin a new drawable.
			 *
			 * Everything in a drawable is rendered in one draw call and stenciled as a unit.
			 */
			void
			begin_filled_drawable();

			/**
			 * End the current drawable.
			 */
			void
			end_filled_drawable(
					const boost::optional<GPlatesMaths::CubeQuadTreeLocation> &cube_quad_tree_location = boost::none);

			/**
			 * Adds a polygon ring as a fan mesh (with the polygon centroid as the fan apex).
			 *
			 * Adds a sequence of @a PointOnSphere points as vertices/indices in global vertex array.
			 */
			template <typename PointOnSphereForwardIter>
			void
			add_polygon_ring_mesh_to_current_filled_drawable(
					const PointOnSphereForwardIter begin_points,
					const unsigned int num_points,
					const GPlatesMaths::UnitVector3D &centroid,
					GPlatesGui::rgba8_t rgba8_color)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						d_current_drawable,
						GPLATES_ASSERTION_SOURCE);

				// Alpha blending will be set up for pre-multiplied alpha.
				const GPlatesGui::rgba8_t pre_multiplied_alpha_rgba8_colour = pre_multiply_alpha(rgba8_color);

				//
				// Create the OpenGL coloured vertices for the filled polygon ring (fan) mesh.
				//

				const GLsizei initial_vertex_elements_size = d_drawable_vertex_elements.size();
				const drawable_vertex_element_type base_vertex_index = d_drawable_vertices.size();
				drawable_vertex_element_type vertex_index = base_vertex_index;

				// First vertex is the centroid.
				d_drawable_vertices.push_back(drawable_vertex_type(centroid, pre_multiplied_alpha_rgba8_colour));
				++vertex_index;

				// The remaining vertices form the boundary.
				PointOnSphereForwardIter points_iter = begin_points;
				for (unsigned int n = 0; n < num_points; ++n, ++vertex_index, ++points_iter)
				{
					d_drawable_vertices.push_back(
							drawable_vertex_type(points_iter->position_vector(), pre_multiplied_alpha_rgba8_colour));

					d_drawable_vertex_elements.push_back(base_vertex_index); // Centroid.
					d_drawable_vertex_elements.push_back(vertex_index); // Current boundary point.
					d_drawable_vertex_elements.push_back(vertex_index + 1); // Next boundary point.
				}

				// Wraparound back to the first boundary vertex to close off the polygon.
				d_drawable_vertices.push_back(
						drawable_vertex_type(begin_points->position_vector(), pre_multiplied_alpha_rgba8_colour));

				// Update the current filled drawable.
				d_current_drawable->end = vertex_index;
				d_current_drawable->count += d_drawable_vertex_elements.size() - initial_vertex_elements_size;
			}
		};

		//! Typedef for a group of filled drawables.
		typedef FilledDrawables filled_drawables_type;


		/**
		 * Creates a @a GLFilledPolygonsGlobeView object.
		 *
		 * NOTE: If @a light is specified but lighting is not supported on the run-time system
		 * (eg, due to exceeding shader instructions limit) then filled drawables will not be rendered
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
		 * Renders the specified filled drawables (spatial partition).
		 */
		void
		render(
				GLRenderer &renderer,
				const filled_drawables_type &filled_drawables);

	private:

		/**
		 * The maximum tile size for rendering filled drawables.
		 *
		 * The bigger this is the fewer times the filled drawables needed to be drawn.
		 * But too big and starts to consume too much memory.
		 * Each pixel is 8 bytes (4 bytes for colour and 4 bytes for combined depth/stencil buffer).
		 *
		 * NOTE: Using *signed* integer to avoid compiler error on 64-bit Mac (not sure why it's happening).
		 */
		static const int MAX_TILE_TEXEL_DIMENSION = 1024;

		/**
		 * The minimum tile size for rendering filled drawables.
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
		typedef GPlatesMaths::CubeQuadTreePartitionUtils::CubeQuadTreeIntersectingNodes<filled_drawable_type>
				filled_drawables_intersecting_nodes_type;

		//! A linked list node that references a spatial partition node of filled drawables.
		struct FilledDrawablesListNode :
				public GPlatesUtils::IntrusiveSinglyLinkedList<FilledDrawablesListNode>::Node
		{
			//! Default constructor.
			FilledDrawablesListNode()
			{  }

			explicit
			FilledDrawablesListNode(
					filled_drawables_spatial_partition_type::const_node_reference_type node_reference_) :
				node_reference(node_reference_)
			{  }

			filled_drawables_spatial_partition_type::const_node_reference_type node_reference;
		};

		//! Typedef for a list of spatial partition nodes referencing reconstructed filled drawables.
		typedef GPlatesUtils::IntrusiveSinglyLinkedList<FilledDrawablesListNode> 
				filled_drawables_spatial_partition_node_list_type;

		//! Typedef for a sequence of filled drawables.
		typedef std::vector<filled_drawable_type> filled_drawable_seq_type;


		/**
		 * The maximum texture dimension of a cube quad tree tile.
		 */
		unsigned int d_max_tile_texel_dimension;

		/**
		 * The minimum texture dimension of a cube quad tree tile.
		 */
		unsigned int d_min_tile_texel_dimension;

		/**
		 * The vertex buffer containing the vertices of all drawables of the current @a render call.
		 */
		GLVertexBuffer::shared_ptr_type d_drawables_vertex_buffer;

		/**
		 * The vertex buffer containing the vertex elements (indices) of all drawables of the current @a render call.
		 */
		GLVertexElementBuffer::shared_ptr_type d_drawables_vertex_element_buffer;

		/**
		 * The vertex array containing all drawables of the current @a render call.
		 *
		 * All drawables for the current @a render call are stored here.
		 * They'll get flushed/replaced when the next render call is made.
		 */
		GLVertexArray::shared_ptr_type d_drawables_vertex_array;

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
				const filled_drawables_type &filled_drawables,
				const filled_drawables_spatial_partition_node_list_type &parent_filled_drawables_intersecting_node_list,
				const filled_drawables_intersecting_nodes_type &filled_drawables_intersecting_nodes,
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
				const filled_drawables_type &filled_drawables,
				const filled_drawables_spatial_partition_node_list_type &
						parent_reconstructed_drawable_meshes_intersecting_node_list,
				const filled_drawables_intersecting_nodes_type &
						reconstructed_drawable_meshes_intersecting_nodes,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
				clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
				const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node);

		void
		get_filled_drawables_intersecting_nodes(
				const GPlatesMaths::CubeQuadTreeLocation &source_raster_tile_location,
				const GPlatesMaths::CubeQuadTreeLocation &intersecting_node_location,
				filled_drawables_spatial_partition_type::const_node_reference_type intersecting_node_reference,
				filled_drawables_spatial_partition_node_list_type &intersecting_node_list,
				boost::object_pool<FilledDrawablesListNode> &intersecting_list_node_pool);

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
				const filled_drawables_type &filled_drawables,
				const filled_drawables_spatial_partition_node_list_type &filled_drawables_intersecting_node_list,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
				clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
				const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node);

		void
		render_filled_drawables_to_tile_texture(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &tile_texture,
				const filled_drawable_seq_type &transformed_sorted_filled_drawables,
				const GLTransform &projection_transform,
				const GLTransform &view_transform);

		void
		get_filled_drawables(
				filled_drawable_seq_type &filled_drawables,
				filled_drawables_spatial_partition_type::element_const_iterator begin_root_filled_drawables,
				filled_drawables_spatial_partition_type::element_const_iterator end_root_filled_drawables,
				const filled_drawables_spatial_partition_node_list_type &filled_drawables_intersecting_node_list);

		GLTexture::shared_ptr_type
		acquire_tile_texture(
				GLRenderer &renderer,
				unsigned int tile_texel_dimension);

		void
		create_drawables_vertex_array(
				GLRenderer &renderer);

		void
		write_filled_drawables_to_vertex_array(
				GLRenderer &renderer,
				const filled_drawables_type &filled_drawables);

		void
		create_shader_programs(
				GLRenderer &renderer);
	};
}

#endif // GPLATES_OPENGL_GLFILLEDPOLYGONSGLOBEVIEW_H
