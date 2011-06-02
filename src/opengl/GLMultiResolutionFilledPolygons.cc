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

#include <algorithm>
#include <cmath>

#include "global/CompilerWarnings.h"

#include <boost/cast.hpp>
#include <boost/foreach.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>

#include "GLMultiResolutionFilledPolygons.h"

#include "GLBlendState.h"
#include "GLCompositeStateSet.h"
#include "GLContext.h"
#include "GLDepthRangeState.h"
#include "GLFragmentTestStates.h"
#include "GLIntersect.h"
#include "GLMatrix.h"
#include "GLPointLinePolygonState.h"
#include "GLRenderer.h"
#include "GLTextureEnvironmentState.h"
#include "GLTextureTransformState.h"
#include "GLTransformState.h"
#include "GLUtils.h"
#include "GLVertex.h"
#include "GLViewportState.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/Colour.h"

#include "utils/IntrusiveSinglyLinkedList.h"
#include "utils/Profile.h"

namespace GPlatesOpenGL
{
	namespace
	{
		//! The inverse of log(2.0).
		const float INVERSE_LOG2 = 1.0 / std::log(2.0);

		/**
		 * Sorts filled polygon drawables by transform.
		 */
		struct SortFilledDrawables
		{
			bool
			operator()(
					const GLMultiResolutionFilledPolygons::filled_polygon_type::non_null_ptr_to_const_type &lhs,
					const GLMultiResolutionFilledPolygons::filled_polygon_type::non_null_ptr_to_const_type &rhs) const
			{
				return lhs->get_transform() < rhs->get_transform();
			}
		};
	}
}


GPlatesOpenGL::GLMultiResolutionFilledPolygons::GLMultiResolutionFilledPolygons(
		const GLMultiResolutionCubeMesh::non_null_ptr_to_const_type &multi_resolution_cube_mesh,
		const cube_subdivision_projection_transforms_cache_type::non_null_ptr_type &cube_subdivision_projection_transforms_cache,
		const cube_subdivision_bounds_cache_type::non_null_ptr_type &cube_subdivision_bounds_cache,
		const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
		const GLVertexBufferResourceManager::shared_ptr_type &vertex_buffer_resource_manager) :
	d_cube_subdivision_projection_transforms_cache(cube_subdivision_projection_transforms_cache),
	d_cube_subdivision_bounds_cache(cube_subdivision_bounds_cache),
	d_texture_resource_manager(texture_resource_manager),
	d_vertex_buffer_resource_manager(vertex_buffer_resource_manager),
	d_texture_cache(GPlatesUtils::ObjectCache<GLTexture>::create()),
	d_tile_texel_dimension(cube_subdivision_projection_transforms_cache->get_tile_texel_dimension()),
	// We probably don't need too large a texture - just want to fit a reasonable number of
	// 256x256 tile textures inside it to minimise render target switching.
	// Each filled polygon gets its own 256x256 section so 4096x4096 is 256 polygons per render target.
	d_polygon_stencil_texel_dimension(4096),
	d_clear_buffers_state(GLClearBuffersState::create()),
	d_clear_buffers(GLClearBuffers::create()),
	d_multi_resolution_cube_mesh(multi_resolution_cube_mesh)
{
	// Setup for clearing the render target colour buffer.
	d_clear_buffers_state->gl_clear_color(); // Clear colour to all zeros.
	d_clear_buffers->gl_clear(GL_COLOR_BUFFER_BIT); // Clear only the colour buffer.

	// Our polygon stencil texture should be big enough to cover a regular tile.
	if (d_polygon_stencil_texel_dimension < d_tile_texel_dimension)
	{
		d_polygon_stencil_texel_dimension = d_tile_texel_dimension;
	}
	// But it can't be larger than the maximum texture dimension for the current system.
	//
	// NOTE: An OpenGL context must be active at this point.
	if (d_polygon_stencil_texel_dimension > GLContext::get_texture_parameters().gl_max_texture_size)
	{
		d_polygon_stencil_texel_dimension = GLContext::get_texture_parameters().gl_max_texture_size;
	}

	// NOTE: An OpenGL context must be active at this point.
	create_polygon_stencil_texture();

	// NOTE: An OpenGL context must be active at this point.
	create_polygon_stencil_quads_vertex_and_index_arrays();
}


unsigned int
GPlatesOpenGL::GLMultiResolutionFilledPolygons::get_level_of_detail(
		const GLTransformState &transform_state) const
{
	// Get the minimum size of a pixel in the current viewport when projected
	// onto the unit sphere (in model space).
	const double min_pixel_size_on_unit_sphere =
			transform_state.get_min_pixel_size_on_unit_sphere();

	//
	// Calculate the level-of-detail.
	// This is the equivalent of:
	//
	//    t = t0 * 2 ^ (-lod)
	//
	// ...where 't0' is the texel size of the *lowest* resolution level-of-detail
	// (note that this is the opposite to GLMultiResolutionRaster where it's the *highest*)
	// and 't' is the projected size of a pixel of the viewport.
	//

	// The maximum texel size of any texel projected onto the unit sphere occurs at the centre
	// of the cube faces. Not all cube subdivisions occur at the face centres but the projected
	// texel size will always be less that at the face centre so at least it's bounded and the
	// variation across the cube face is not that large so we shouldn't be using a level-of-detail
	// that is much higher than what we need.
	const float max_lowest_resolution_texel_size_on_unit_sphere =
			2.0 / d_cube_subdivision_projection_transforms_cache->get_tile_texel_dimension();

	const float level_of_detail_factor = INVERSE_LOG2 *
			(std::log(max_lowest_resolution_texel_size_on_unit_sphere) -
					std::log(min_pixel_size_on_unit_sphere));

	// We need to round up instead of down and then clamp to zero.
	// We don't have an upper limit - as we traverse the quad tree to higher and higher
	// resolution nodes we might eventually reach the leaf nodes of the tree without
	// having satisfied the requested level-of-detail resolution - in this case we'll
	// just render the leaf nodes as that's the highest we can provide.
	int level_of_detail = static_cast<int>(level_of_detail_factor + 0.99f);
	// Clamp to lowest resolution level of detail.
	if (level_of_detail < 0)
	{
		// If we get there then even our lowest resolution level of detail
		// had too much resolution - but this is pretty unlikely for all but the very
		// smallest of viewports.
		level_of_detail = 0;
	}

	return boost::numeric_cast<unsigned int>(level_of_detail);
}


// namespace
// {
// 	unsigned int g_num_tiles_rendered = 0;
// 	unsigned int g_num_render_target_switches = 0;
// 	unsigned int g_num_polygons_rendered = 0;
// }

void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::render(
		GLRenderer &renderer,
		const filled_polygons_spatial_partition_type &filled_polygons_spatial_partition)
{
	PROFILE_FUNC();

// 	g_num_tiles_rendered = 0;
// 	g_num_render_target_switches = 0;
// 	g_num_polygons_rendered = 0;

	// Get the level-of-detail based on the size of viewport pixels projected onto the globe.
	// We'll try to render of this level of detail if our quad tree is deep enough.
	const unsigned int render_level_of_detail = get_level_of_detail(renderer.get_transform_state());

	// Get the view frustum planes.
	const GLFrustum &frustum_planes = renderer.get_transform_state().get_current_frustum_planes_in_model_space();

	//
	// Traverse the source raster cube quad tree and the spatial partition of reconstructed polygon meshes.
	//

	// Traverse the quad trees of the cube faces.
	for (unsigned int face = 0; face < 6; ++face)
	{
		const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
				static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

		// Get the quad tree root node of the current cube face of the source raster.
		const mesh_quad_tree_node_type mesh_quad_tree_root_node =
				d_multi_resolution_cube_mesh->get_quad_tree_root_node(cube_face);

		// This is used to find those nodes of the reconstructed polygon meshes spatial partition
		// that intersect the source raster cube quad tree.
		// This is so we know which polygon meshes to draw for each source raster tile.
		filled_polygons_intersecting_nodes_type
				filled_polygons_intersecting_nodes(
						filled_polygons_spatial_partition,
						cube_face);

		// Get the child projection transforms node.
		const cube_subdivision_projection_transforms_cache_type::node_reference_type
				projection_transforms_cache_root_node =
						d_cube_subdivision_projection_transforms_cache->get_quad_tree_root_node(cube_face);

		// Get the child bounds node.
		const cube_subdivision_bounds_cache_type::node_reference_type
				bounds_cache_root_node =
						d_cube_subdivision_bounds_cache->get_quad_tree_root_node(cube_face);

		// Initially there are no intersecting nodes...
		filled_polygons_spatial_partition_node_list_type filled_polygons_spatial_partition_node_list;

		render_quad_tree(
				renderer,
				mesh_quad_tree_root_node,
				filled_polygons_spatial_partition,
				filled_polygons_spatial_partition_node_list,
				filled_polygons_intersecting_nodes,
				projection_transforms_cache_root_node,
				bounds_cache_root_node,
				0/*level_of_detail*/,
				render_level_of_detail,
				frustum_planes,
				// There are six frustum planes initially active
				GLFrustum::ALL_PLANES_ACTIVE_MASK);
	}

// 	qWarning() << "Tiles rendered: " << g_num_tiles_rendered;
// 	qWarning() << "RT switches: " << g_num_render_target_switches;
// 	qWarning() << "Drawables: " << g_num_polygons_rendered;
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::render_quad_tree(
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
		boost::uint32_t frustum_plane_mask)
{
	// If the frustum plane mask is zero then it means we are entirely inside the view frustum.
	// So only test for intersection if the mask is non-zero.
	if (frustum_plane_mask != 0)
	{
		const GLIntersect::OrientedBoundingBox &quad_tree_node_bounds =
				d_cube_subdivision_bounds_cache->get_cached_element(bounds_cache_node)
						->get_oriented_bounding_box();

		// See if the current quad tree node intersects the view frustum.
		// Use the quad tree node's bounding box.
		boost::optional<boost::uint32_t> out_frustum_plane_mask =
				GLIntersect::intersect_OBB_frustum(
						quad_tree_node_bounds,
						frustum_planes.get_planes(),
						frustum_plane_mask);
		if (!out_frustum_plane_mask)
		{
			// No intersection so quad tree node is outside view frustum and we can cull it.
			return;
		}

		// Update the frustum plane mask so we only test against those planes that
		// the current quad tree render node intersects. The node is entirely inside
		// the planes with a zero bit and so its child nodes are also entirely inside
		// those planes too and so they won't need to test against them.
		frustum_plane_mask = out_frustum_plane_mask.get();
	}

	// If either we're at the correct level of detail for rendering then draw the filled polygons.
	if (level_of_detail == render_level_of_detail)
	{
		// Continue to recurse into the filled polygons spatial partition to continue to find
		// those polygons that intersect the current quad tree node.
		render_quad_tree_node(
				renderer,
				mesh_quad_tree_node,
				filled_polygons_spatial_partition,
				parent_filled_polygons_intersecting_node_list,
				filled_polygons_intersecting_nodes,
				projection_transforms_cache_node);

		return;
	}

	//
	// Iterate over the child quad tree nodes.
	//

	for (unsigned int child_v_offset = 0; child_v_offset < 2; ++child_v_offset)
	{
		for (unsigned int child_u_offset = 0; child_u_offset < 2; ++child_u_offset)
		{
			// Get the child node of the current mesh quad tree node.
			const mesh_quad_tree_node_type child_mesh_quad_tree_node =
					d_multi_resolution_cube_mesh->get_child_node(
							mesh_quad_tree_node,
							child_u_offset,
							child_v_offset);

			// Used to determine which filled polygons intersect the child quad tree node.
			filled_polygons_intersecting_nodes_type
					child_filled_polygons_intersecting_nodes(
							filled_polygons_intersecting_nodes,
							child_u_offset,
							child_v_offset);

			// Construct linked list nodes on the runtime stack as it simplifies memory management.
			// When the stack unwinds, the list(s) referencing these nodes, as well as the nodes themselves,
			// will disappear together (leaving any lists higher up in the stack still intact) - this happens
			// because this list implementation supports tail-sharing.
			FilledPolygonsListNode child_filled_polygons_list_nodes[
					filled_polygons_intersecting_nodes_type::parent_intersecting_nodes_type::MAX_NUM_NODES];

			// A tail-shared list to contain the filled polygon nodes that intersect the
			// current root node. The parent list contains the nodes we've been
			// accumulating so far during our quad tree traversal.
			filled_polygons_spatial_partition_node_list_type
					child_filled_polygons_intersecting_node_list(
							parent_filled_polygons_intersecting_node_list);

			// Add any new intersecting nodes from the filled polygons spatial partition.
			// These new nodes are the nodes that intersect the tile at the current quad tree depth.
			const filled_polygons_intersecting_nodes_type::parent_intersecting_nodes_type &
					parent_intersecting_nodes =
							child_filled_polygons_intersecting_nodes.get_parent_intersecting_nodes();

			// Now add those neighbours nodes that exist (not all areas of the spatial partition will be
			// populated with filled polygons).
			const unsigned int num_parent_nodes = parent_intersecting_nodes.get_num_nodes();
			for (unsigned int parent_node_index = 0; parent_node_index < num_parent_nodes; ++parent_node_index)
			{
				const filled_polygons_spatial_partition_type::const_node_reference_type &
						intersecting_parent_node_reference = parent_intersecting_nodes.get_node(parent_node_index);
				// Only need to add nodes that actually contain filled polygons.
				// NOTE: We still recurse into child nodes though - an empty internal node does not
				// mean the child nodes are necessarily empty.
				if (!intersecting_parent_node_reference.empty())
				{
					child_filled_polygons_list_nodes[parent_node_index].node_reference =
							intersecting_parent_node_reference;

					// Add to the list of filled polygon spatial partition nodes that
					// intersect the current tile.
					child_filled_polygons_intersecting_node_list.push_front(
							&child_filled_polygons_list_nodes[parent_node_index]);
				}
			}

			// Get the child projection transforms node.
			const cube_subdivision_projection_transforms_cache_type::node_reference_type
					child_projection_transforms_cache_node =
							d_cube_subdivision_projection_transforms_cache->get_child_node(
									projection_transforms_cache_node,
									child_u_offset,
									child_v_offset);

			// Get the child bounds node.
			const cube_subdivision_bounds_cache_type::node_reference_type
					child_bounds_cache_node =
							d_cube_subdivision_bounds_cache->get_child_node(
									bounds_cache_node,
									child_u_offset,
									child_v_offset);

			render_quad_tree(
					renderer,
					child_mesh_quad_tree_node,
					filled_polygons_spatial_partition,
					child_filled_polygons_intersecting_node_list,
					child_filled_polygons_intersecting_nodes,
					child_projection_transforms_cache_node,
					child_bounds_cache_node,
					level_of_detail + 1,
					render_level_of_detail,
					frustum_planes,
					frustum_plane_mask);
		}
	}
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::render_quad_tree_node(
		GLRenderer &renderer,
		const mesh_quad_tree_node_type &mesh_quad_tree_node,
		const filled_polygons_spatial_partition_type &filled_polygons_spatial_partition,
		const filled_polygons_spatial_partition_node_list_type &parent_filled_polygons_intersecting_node_list,
		const filled_polygons_intersecting_nodes_type &filled_polygons_intersecting_nodes,
		const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node)
{
	// From here on we can't allocate the list nodes on the runtime stack because we need to access
	// the list after we return from traversing the spatial partition. So use an object pool instead.
	boost::object_pool<FilledPolygonsListNode> filled_polygons_list_node_pool;

	// A tail-shared list to contain the reconstructed polygon meshes nodes that intersect the
	// current source raster root node. The parent list contains the nodes we've been
	// accumulating so far during our quad tree traversal.
	filled_polygons_spatial_partition_node_list_type
			filled_polygons_intersecting_node_list(
					parent_filled_polygons_intersecting_node_list);

	// Add any new intersecting nodes from the reconstructed polygon meshes spatial partition.
	// These new nodes are the nodes that intersect the source raster tile at the current quad tree depth.
	const filled_polygons_intersecting_nodes_type::intersecting_nodes_type &intersecting_nodes =
			filled_polygons_intersecting_nodes.get_intersecting_nodes();

	const GPlatesMaths::CubeQuadTreeLocation &tile_location =
			filled_polygons_intersecting_nodes.get_node_location();

	// Now add those intersecting nodes that exist (not all areas of the spatial partition will be
	// populated with reconstructed polygon meshes).
	const unsigned int num_intersecting_nodes = intersecting_nodes.get_num_nodes();
	for (unsigned int list_node_index = 0; list_node_index < num_intersecting_nodes; ++list_node_index)
	{
		const filled_polygons_spatial_partition_type::const_node_reference_type &
				intersecting_node_reference = intersecting_nodes.get_node(list_node_index);

		// Only need to add nodes that actually contain reconstructed polygon meshes.
		// NOTE: We still recurse into child nodes though - an empty internal node does not
		// mean the child nodes are necessarily empty.
		if (!intersecting_node_reference.empty())
		{
			// Add the node to the list.
			filled_polygons_intersecting_node_list.push_front(
					filled_polygons_list_node_pool.construct(intersecting_node_reference));
		}

		// Continue to recurse into the spatial partition of reconstructed polygon meshes.
		get_filled_polygons_intersecting_nodes(
				tile_location,
				intersecting_nodes.get_node_location(list_node_index),
				intersecting_node_reference,
				filled_polygons_intersecting_node_list,
				filled_polygons_list_node_pool);
	}

	//
	// Now traverse the list of intersecting reconstructed polygon meshes and render them.
	//

	// Render the source raster tile to the scene.
	render_tile_to_scene(
			renderer,
			mesh_quad_tree_node,
			filled_polygons_spatial_partition,
			filled_polygons_intersecting_node_list,
			projection_transforms_cache_node);
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::get_filled_polygons_intersecting_nodes(
		const GPlatesMaths::CubeQuadTreeLocation &tile_location,
		const GPlatesMaths::CubeQuadTreeLocation &intersecting_node_location,
		filled_polygons_spatial_partition_type::const_node_reference_type intersecting_node_reference,
		filled_polygons_spatial_partition_node_list_type &intersecting_node_list,
		boost::object_pool<FilledPolygonsListNode> &intersecting_list_node_pool)
{
	// Iterate over the four child nodes of the current parent node.
	for (unsigned int child_y_offset = 0; child_y_offset < 2; ++child_y_offset)
	{
		for (unsigned int child_x_offset = 0; child_x_offset < 2; ++child_x_offset)
		{
			filled_polygons_spatial_partition_type::const_node_reference_type
					child_intersecting_node_reference =
							intersecting_node_reference.get_child_node(
									child_x_offset, child_y_offset);
			if (!child_intersecting_node_reference)
			{
				continue;
			}

			const GPlatesMaths::CubeQuadTreeLocation child_intersecting_node_location(
					intersecting_node_location,
					child_x_offset,
					child_y_offset);

			// If the child node intersects the source raster tile then add the node and
			// recurse into its children.
			if (intersect_loose_cube_quad_tree_location_with_regular_cube_quad_tree_location(
					child_intersecting_node_location, tile_location))
			{
				// Only need to add nodes that actually contain reconstructed polygon meshes.
				// NOTE: We still recurse into child nodes though - an empty internal node does not
				// mean the child nodes are necessarily empty.
				if (!child_intersecting_node_reference.empty())
				{
					// Add the intersecting node to the list.
					intersecting_node_list.push_front(
							intersecting_list_node_pool.construct(child_intersecting_node_reference));
				}

				// Recurse into the current child.
				get_filled_polygons_intersecting_nodes(
						tile_location,
						child_intersecting_node_location,
						child_intersecting_node_reference,
						intersecting_node_list,
						intersecting_list_node_pool);
			}
		}
	}
}


GPlatesOpenGL::GLStateSet::non_null_ptr_to_const_type
GPlatesOpenGL::GLMultiResolutionFilledPolygons::create_tile_state_set(
		const GLTexture::shared_ptr_to_const_type &tile_texture,
		const GLTransform &projection_transform,
		const GLTransform &view_transform)
{
	// The composite state to return to the caller.
	GLCompositeStateSet::non_null_ptr_type tile_state_set = GLCompositeStateSet::create();

	GLUtils::set_frustum_texture_state(
			*tile_state_set,
			tile_texture,
			projection_transform.get_matrix(),
			view_transform.get_matrix(),
			0/*texture_unit*/,
			GL_REPLACE);

	// NOTE: We don't set alpha-blending (or alpha-testing) state here because we
	// might not be rendering directly to the final render target and hence we don't
	// want to double-blend semi-transparent rasters - the alpha value is multiplied by
	// all channels including alpha during alpha blending (R,G,B,A) -> (A*R,A*G,A*B,A*A) -
	// the final render target would then have a source blending contribution of (3A*R,3A*G,3A*B,4A)
	// which is not what we want - we want (A*R,A*G,A*B,A*A).
	// Currently we are rendering to the final render target but when we reconstruct
	// rasters in the map view (later) we will render to an intermediate render target.

#if 0
	// Used to render as wire-frame meshes instead of filled textured meshes for
	// visualising mesh density.
	GLPolygonState::non_null_ptr_type polygon_state = GLPolygonState::create();
	polygon_state->gl_polygon_mode(GL_FRONT_AND_BACK, GL_LINE);
	tile_state_set->add_state_set(polygon_state);
#endif

	return tile_state_set;
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::render_tile_to_scene(
		GLRenderer &renderer,
		const mesh_quad_tree_node_type &mesh_quad_tree_node,
		const filled_polygons_spatial_partition_type &filled_polygons_spatial_partition,
		const filled_polygons_spatial_partition_node_list_type &filled_polygons_intersecting_node_list,
		const cube_subdivision_projection_transforms_cache_type::node_reference_type &projection_transforms_cache_node)
{
	PROFILE_FUNC();

	// Sort the reconstructed polygon meshes by transform.
	filled_polygon_seq_type transformed_sorted_filled_drawables;
	get_filled_polygons(
			transformed_sorted_filled_drawables,
			filled_polygons_spatial_partition.begin_root_elements(),
			filled_polygons_spatial_partition.end_root_elements(),
			filled_polygons_intersecting_node_list);

	if (transformed_sorted_filled_drawables.empty())
	{
		return;
	}

	// Get the view/projection transforms for the current cube quad tree node.
	const GLTransform::non_null_ptr_to_const_type projection_transform =
			d_cube_subdivision_projection_transforms_cache->get_cached_element(
					projection_transforms_cache_node)->get_projection_transform();

	// The view transform never changes within a cube face so it's the same across
	// an entire cube face quad tree (each cube face has its own quad tree).
	const GLTransform::non_null_ptr_to_const_type view_transform =
			d_cube_subdivision_projection_transforms_cache->get_view_transform(
					projection_transforms_cache_node.get_cube_face());

	// Get an unused tile texture from our texture cache.
	const GLTexture::shared_ptr_to_const_type tile_texture = allocate_tile_texture();

	// Render the filled polygons to the tile texture.
	render_filled_polygons_to_tile_texture(
			renderer,
			tile_texture,
			transformed_sorted_filled_drawables,
			*projection_transform,
			*view_transform);

	// Prepare for rendering the current tile.
	const GLStateSet::non_null_ptr_to_const_type tile_state_set =
			create_tile_state_set(tile_texture, *projection_transform, *view_transform);

	// Get the mesh covering the current quad tree node tile.
	//
	// TODO: If we recurse deep enough we have to also use the clip texture.
	GLDrawable::non_null_ptr_to_const_type mesh_drawable = mesh_quad_tree_node.get_mesh_drawable();

	// Push the tile state set.
	renderer.push_state_set(tile_state_set);

	renderer.add_drawable(mesh_drawable);

	// Pop the tile state set.
	renderer.pop_state_set();
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::render_filled_polygons_to_tile_texture(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &tile_texture,
		const filled_polygon_seq_type &transformed_sorted_filled_drawables,
		const GLTransform &projection_transform,
		const GLTransform &view_transform)
{
	// Push a render target that will render the individual filled polygons to the tile texture.
	// We can render to the target in parallel because we're only going to render to it once per frame.
	// Note that even though the texture is cached we're not actually re-using it from frame to frame.
	renderer.push_render_target(
			GLTextureRenderTargetType::create(
					tile_texture, d_tile_texel_dimension, d_tile_texel_dimension),
			GLRenderer::RENDER_TARGET_USAGE_PARALLEL);

	//++g_num_tiles_rendered;

	// We clear this tile's render texture just before we render the polygon stencil texture to it.
	// This reduces the number of render target switches by one since no drawables are added
	// to the tile's render target until after switching back from the polygon stencil render target.
	bool cleared_tile_render_target = false;

	const unsigned int num_polygons_per_stencil_texture_render =
			d_polygon_stencil_texel_dimension * d_polygon_stencil_texel_dimension /
					(d_tile_texel_dimension * d_tile_texel_dimension);

	unsigned int num_polygons_left_to_render = transformed_sorted_filled_drawables.size();
	filled_polygon_seq_type::const_iterator filled_drawables_iter = transformed_sorted_filled_drawables.begin();
	while (num_polygons_left_to_render)
	{
		const unsigned int num_polygons_in_group =
				(num_polygons_left_to_render > num_polygons_per_stencil_texture_render)
				? num_polygons_per_stencil_texture_render
				: num_polygons_left_to_render;

		filled_polygon_seq_type::const_iterator filled_drawables_group_end = filled_drawables_iter;
		std::advance(filled_drawables_group_end, num_polygons_in_group);

		// Render the filled polygons to the current tile render target.
		render_filled_polygons_to_polygon_stencil_texture(
				renderer,
				filled_drawables_iter,
				filled_drawables_group_end,
				projection_transform,
				view_transform);

		if (!cleared_tile_render_target)
		{
			// Create a state set to set the viewport.
			const GLViewport viewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension);
			GLViewportState::non_null_ptr_type viewport_state = GLViewportState::create(viewport);
			// Push the viewport state set.
			renderer.push_state_set(viewport_state);
			// Let the transform state know of the new viewport.
			renderer.get_transform_state().set_viewport(viewport);

			// Clear the colour buffer of the render target.
			renderer.push_state_set(d_clear_buffers_state);
			renderer.add_drawable(d_clear_buffers);
			renderer.pop_state_set();

			cleared_tile_render_target = true;
		}

		// Render the filled polygons, in the stencil texture, to the current tile render target.
		render_filled_polygons_from_polygon_stencil_texture(
				renderer,
				num_polygons_in_group);

		// Advance to the next group of polygons.
		filled_drawables_iter = filled_drawables_group_end;
		num_polygons_left_to_render -= num_polygons_in_group;
	}

	// Pop the viewport state set.
	if (cleared_tile_render_target)
	{
		renderer.pop_state_set();
	}

	renderer.pop_render_target();
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::render_filled_polygons_to_polygon_stencil_texture(
		GLRenderer &renderer,
		const filled_polygon_seq_type::const_iterator begin_filled_drawables,
		const filled_polygon_seq_type::const_iterator end_filled_drawables,
		const GLTransform &projection_transform,
		const GLTransform &view_transform)
{
	//PROFILE_FUNC();

	// Push a render target that we'll render to the polygon stencil texture.
	// We render in serial because the stencil texture will be used immediately afterwards.
	renderer.push_render_target(
			GLTextureRenderTargetType::create(
					d_polygon_stencil_texture,
					d_polygon_stencil_texel_dimension,
					d_polygon_stencil_texel_dimension),
			GLRenderer::RENDER_TARGET_USAGE_SERIAL);

	//++g_num_render_target_switches;

	// Clear the entire colour buffer of the render target.
	// Clears the entire render target regardless of the current viewport.
	const GLClearBuffersState::non_null_ptr_type clear_buffers_state = GLClearBuffersState::create();
	const GLClearBuffers::non_null_ptr_type clear_buffers = GLClearBuffers::create();
	// Clear only the colour buffer.
	clear_buffers->gl_clear(GL_COLOR_BUFFER_BIT);
	renderer.push_state_set(clear_buffers_state);
	renderer.add_drawable(clear_buffers);
	renderer.pop_state_set();

	// The composite state.
	GLCompositeStateSet::non_null_ptr_type polygon_stencil_state = GLCompositeStateSet::create();

	// Alpha-blend state set to invert destination alpha (and colour) every time a pixel
	// is rendered (this means we get 1 where a pixel is covered by an odd number of triangles
	// and 0 by an even number of triangles).
	GLBlendState::non_null_ptr_type blend_state = GLBlendState::create();
	blend_state->gl_enable(GL_TRUE).gl_blend_func(GL_ONE_MINUS_DST_ALPHA, GL_ZERO);
	polygon_stencil_state->add_state_set(blend_state);

#if 0
	// Used to render as wire-frame meshes instead of filled textured meshes for
	// visualising mesh density.
	GLPolygonState::non_null_ptr_type polygon_state = GLPolygonState::create();
	polygon_state->gl_polygon_mode(GL_FRONT_AND_BACK, GL_LINE);
	polygon_stencil_state->add_state_set(polygon_state);

	// Set the anti-aliased line state.
	GPlatesOpenGL::GLLineState::non_null_ptr_type line_state = GPlatesOpenGL::GLLineState::create();
	line_state->gl_line_width(4.0f);
	//line_state->gl_enable_line_smooth(GL_TRUE).gl_hint_line_smooth(GL_NICEST);
	polygon_stencil_state->add_state_set(line_state);
#endif

	renderer.push_state_set(polygon_stencil_state);

	renderer.push_transform(projection_transform);
	renderer.push_transform(view_transform);

	boost::optional<GPlatesAppLogic::ReconstructMethodFiniteRotation::non_null_ptr_to_const_type> current_finite_rotation;
	boost::optional<GLTransform::non_null_ptr_type> current_transform;

	const unsigned int num_viewports_along_stencil_texture_side =
			d_polygon_stencil_texel_dimension / d_tile_texel_dimension;

	// Render each filled polygon to a separate viewport within the polygon stencil texture.
	unsigned int viewport_x_offset = 0;
	unsigned int viewport_y_offset = 0;
	for (filled_polygon_seq_type::const_iterator filled_drawables_iter = begin_filled_drawables;
		filled_drawables_iter != end_filled_drawables;
		++filled_drawables_iter)
	{
		const filled_polygon_type &filled_drawable = **filled_drawables_iter;

		// The viewport subsection of the render target for the current filled polygon.
		const GLViewport viewport(
				viewport_x_offset * d_tile_texel_dimension,
				viewport_y_offset * d_tile_texel_dimension,
				d_tile_texel_dimension,
				d_tile_texel_dimension);

		// Push the viewport state set.
		renderer.push_state_set(GLViewportState::create(viewport));

		// Get the current filled polygon's finite rotation transform.
		const boost::optional<GPlatesAppLogic::ReconstructMethodFiniteRotation::non_null_ptr_to_const_type> &
				finite_rotation = filled_drawable.get_transform();

		// If the current filled polygon has a finite rotation and it's different to the last one...
		if (finite_rotation)
		{
			if (finite_rotation.get() != current_finite_rotation)
			{
				// Convert the finite rotation from a unit quaternion to a matrix so we can feed it to OpenGL.
				const GPlatesMaths::UnitQuaternion3D &quat_rotation = finite_rotation.get()->get_finite_rotation().unit_quat();
				current_transform = GLTransform::create(GL_MODELVIEW, quat_rotation);
			}
		}
		else
		{
			current_transform = boost::none;
		}

		// Transform filled polygon if necessary.
		if (current_transform)
		{
			renderer.push_transform(*current_transform.get());
		}

		// Render the current filled polygon.
		renderer.add_drawable(filled_drawable.get_drawable().get());
		//++g_num_polygons_rendered;

		if (current_transform)
		{
			renderer.pop_transform();
		}

		current_finite_rotation = finite_rotation;

		renderer.pop_state_set(); // Pop the viewport state.

		// Move to the next row of viewport subsections if we have to.
		if (++viewport_x_offset == num_viewports_along_stencil_texture_side)
		{
			viewport_x_offset = 0;
			++viewport_y_offset;
		}
	}

	renderer.pop_transform(); // Pop view transform.
	renderer.pop_transform(); // Pop projection transform.

	renderer.pop_state_set(); // Pop the polygon stencil state.

	renderer.pop_render_target();

	//++g_num_render_target_switches;
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::render_filled_polygons_from_polygon_stencil_texture(
		GLRenderer &renderer,
		const unsigned int num_polygons_in_group)
{
	//PROFILE_FUNC();

	// The composite state.
	GLCompositeStateSet::non_null_ptr_type polygon_stencil_state = GLCompositeStateSet::create();

	// Set the alpha-blend state in case raster is semi-transparent.
	GPlatesOpenGL::GLBlendState::non_null_ptr_type blend_state =
			GPlatesOpenGL::GLBlendState::create();
	blend_state->gl_enable(GL_TRUE).gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	polygon_stencil_state->add_state_set(blend_state);

	// Set the alpha-test state to reject pixels where alpha is zero (they make no
	// change or contribution to the render target) - this is an optimisation.
	GPlatesOpenGL::GLAlphaTestState::non_null_ptr_type alpha_test_state =
			GPlatesOpenGL::GLAlphaTestState::create();
	alpha_test_state->gl_enable(GL_TRUE).gl_alpha_func(GL_GREATER, GLclampf(0));
	polygon_stencil_state->add_state_set(alpha_test_state);

	GLUtils::set_full_screen_quad_texture_state(
			*polygon_stencil_state,
			d_polygon_stencil_texture,
			0/*texture_unit*/,
			GL_REPLACE);

	renderer.push_state_set(polygon_stencil_state);

	// Create a vertex element array that shares the same indices as full array.
	const GLVertexElementArray::shared_ptr_type partial_polygon_stencil_quads_vertex_element_array =
			GLVertexElementArray::create(
					d_polygon_stencil_quads_vertex_element_array->get_array_data(),
					GL_UNSIGNED_SHORT);

	// But we'll limit the number of quads we draw to the number of polygons that were rendered
	// into the larger polygon stencil texture.
	const unsigned int num_quad_vertices = 4 * num_polygons_in_group;
	partial_polygon_stencil_quads_vertex_element_array->gl_draw_range_elements_EXT(
			GL_QUADS,
			0/*start*/,
			num_quad_vertices - 1/*end*/,
			num_quad_vertices/*count*/,
			0/*indices_offset*/);

	// Create a drawable for the partial quads and add it to the renderer.
	//
	// NOTE: We leave the model-view and projection matrices as identity as that is what we
	// we need to draw full-screen quads.
	renderer.add_drawable(
			GLVertexArrayDrawable::create(
					d_polygon_stencil_quads_vertex_array,
					partial_polygon_stencil_quads_vertex_element_array));

	renderer.pop_state_set(); // Pop the polygon stencil state.
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::get_filled_polygons(
		filled_polygon_seq_type &transformed_sorted_filled_drawables,
		filled_polygons_spatial_partition_type::element_const_iterator begin_root_filled_polygons,
		filled_polygons_spatial_partition_type::element_const_iterator end_root_filled_polygons,
		const filled_polygons_spatial_partition_node_list_type &filled_polygons_intersecting_node_list)
{
	// Add the reconstructed polygon meshes in the root of the spatial partition.
	// These are the meshes that were too large to insert in any face of the cube quad tree partition.
	// Add the reconstructed polygon meshes of the current node.
	add_filled_polygons(
			transformed_sorted_filled_drawables,
			begin_root_filled_polygons,
			end_root_filled_polygons);

	// Iterate over the nodes in the spatial partition that contain the reconstructed polygon meshes we are interested in.
	filled_polygons_spatial_partition_node_list_type::const_iterator filled_polygons_node_iter =
			filled_polygons_intersecting_node_list.begin();
	filled_polygons_spatial_partition_node_list_type::const_iterator filled_polygons_node_end =
			filled_polygons_intersecting_node_list.end();
	for ( ; filled_polygons_node_iter != filled_polygons_node_end; ++filled_polygons_node_iter)
	{
		const filled_polygons_spatial_partition_type::const_node_reference_type &node_reference =
				filled_polygons_node_iter->node_reference;

		// Add the reconstructed polygon meshes of the current node.
		add_filled_polygons(
				transformed_sorted_filled_drawables,
				node_reference.begin(),
				node_reference.end());
	}

	// Sort the sequence of filled drawables by transform.
	std::sort(
			transformed_sorted_filled_drawables.begin(),
			transformed_sorted_filled_drawables.end(),
			SortFilledDrawables());
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::add_filled_polygons(
		filled_polygon_seq_type &transform_sorted_filled_drawables,
		filled_polygons_spatial_partition_type::element_const_iterator begin_filled_polygons,
		filled_polygons_spatial_partition_type::element_const_iterator end_filled_polygons)
{
	// Iterate over the reconstructed polygon meshes.
	filled_polygons_spatial_partition_type::element_const_iterator
			filled_polygons_iter = begin_filled_polygons;
	for ( ; filled_polygons_iter != end_filled_polygons; ++filled_polygons_iter)
	{
		const filled_polygon_type::non_null_ptr_type &filled_polygon = *filled_polygons_iter;
		if (filled_polygon->get_drawable())
		{
			transform_sorted_filled_drawables.push_back(filled_polygon);
		}
	}
}


GPlatesOpenGL::GLTexture::shared_ptr_to_const_type
GPlatesOpenGL::GLMultiResolutionFilledPolygons::allocate_tile_texture()
{
	// Get an unused tile texture from the cache if there is one.
	boost::optional< boost::shared_ptr<GLTexture> > tile_texture = d_texture_cache->allocate_object();
	if (!tile_texture)
	{
		// No unused texture so create a new one...
		tile_texture = d_texture_cache->allocate_object(
				GLTexture::create_as_auto_ptr(d_texture_resource_manager));

		create_tile_texture(tile_texture.get());
	}

	return tile_texture.get();
}

DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::create_tile_texture(
		const GLTexture::shared_ptr_type &texture)
{
	PROFILE_FUNC();

	// Bind the texture so its the current texture.
	// Here we actually make a direct OpenGL call to bind the texture to the currently
	// active texture unit. It doesn't matter what the current texture unit is because
	// the only reason we're binding the texture object is so we can set its state =
	// so that subsequent binds of this texture object, when we render the scene graph,
	// will set that state to OpenGL.
	texture->gl_bind_texture(GL_TEXTURE_2D);

	//
	// No mipmaps needed so we specify no mipmap filtering.
	// We're not using mipmaps because our cube mapping does not have much distortion
	// unlike global rectangular lat/lon rasters that squash near the poles.
	//
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	if (GLEW_EXT_texture_edge_clamp)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	// Specify anisotropic filtering if it's supported since we are not using mipmaps
	// and any textures rendered near the edge of the globe will get squashed a bit due to
	// the angle we are looking at them and anisotropic filtering will help here.
	if (GLEW_EXT_texture_filter_anisotropic)
	{
		const GLfloat anisotropy = GLContext::get_texture_parameters().gl_texture_max_anisotropy_EXT;
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	}

	// Create the texture but don't load any data into it.
	// Leave it uninitialised because we will be rendering into it to initialise it.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
			d_tile_texel_dimension, d_tile_texel_dimension,
			0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::create_polygon_stencil_texture()
{
	d_polygon_stencil_texture = GLTexture::create(d_texture_resource_manager);

	// Bind the texture so its the current texture.
	// Here we actually make a direct OpenGL call to bind the texture to the currently
	// active texture unit. It doesn't matter what the current texture unit is because
	// the only reason we're binding the texture object is so we can set its state =
	// so that subsequent binds of this texture object, when we render the scene graph,
	// will set that state to OpenGL.
	d_polygon_stencil_texture->gl_bind_texture(GL_TEXTURE_2D);

	//
	// No mipmaps needed so we specify no mipmap filtering.
	// We're not using mipmaps because we simply render with one-to-one texel-to-pixel
	// mapping (using a full screen quad in a render target).
	//
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	if (GLEW_EXT_texture_edge_clamp)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	// Create the texture but don't load any data into it.
	// Leave it uninitialised because we will be rendering into it to initialise it.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
			d_polygon_stencil_texel_dimension, d_polygon_stencil_texel_dimension,
			0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}

ENABLE_GCC_WARNING("-Wold-style-cast")


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::create_polygon_stencil_quads_vertex_and_index_arrays()
{
	const unsigned int num_quads_along_polygon_stencil_dimension =
			d_polygon_stencil_texel_dimension / d_tile_texel_dimension;

	const double scale_uv = 1.0 / num_quads_along_polygon_stencil_dimension;

	const unsigned int num_quad_vertices =
			4 * num_quads_along_polygon_stencil_dimension * num_quads_along_polygon_stencil_dimension;

	// The vertices for the quads.
	std::vector<GLTexturedVertex> quad_vertices;
	quad_vertices.reserve(num_quad_vertices);

	// We're using 'GLushort' vertex indices which are 16-bit - make sure we don't overflow them.
	// 16-bit indices are faster than 32-bit for graphics cards (but again probably not much gain).
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			num_quad_vertices <= (1 << 16),
			GPLATES_ASSERTION_SOURCE);
	std::vector<GLushort> quad_indices;
	quad_indices.reserve(num_quad_vertices);

	for (unsigned int y = 0; y < num_quads_along_polygon_stencil_dimension; ++y)
	{
		for (unsigned int x = 0; x < num_quads_along_polygon_stencil_dimension; ++x)
		{
			// Add four vertices for the current quad.
			const double u0 = x * scale_uv;
			const double v0 = y * scale_uv;
			const double u1 = u0 + scale_uv;
			const double v1 = v0 + scale_uv;

			const GLushort quad_base_vertex_index = quad_vertices.size();

			//
			//  x,  y, z, u, v
			//
			// Note that the (x,y,z) positions of each quad are the same since they overlap
			// when rendering (blending) into a tile's render texture.
			quad_vertices.push_back(GLTexturedVertex(-1, -1, 0, u0, v0));
			quad_vertices.push_back(GLTexturedVertex(1, -1, 0, u1, v0));
			quad_vertices.push_back(GLTexturedVertex(1,  1, 0, u1, v1));
			quad_vertices.push_back(GLTexturedVertex(-1,  1, 0, u0, v1));

			quad_indices.push_back(quad_base_vertex_index);
			quad_indices.push_back(quad_base_vertex_index + 1);
			quad_indices.push_back(quad_base_vertex_index + 2);
			quad_indices.push_back(quad_base_vertex_index + 3);
		}
	}

	// Create a single OpenGL vertex array to contain the vertices of all 256x256 polygon stencil
	// quads that fit inside the polygon stencil texture.
	d_polygon_stencil_quads_vertex_array = GLVertexArray::create(
			quad_vertices,
			GLArray::USAGE_STATIC,
			d_vertex_buffer_resource_manager);

	// Create the associated vertex indices array.
	d_polygon_stencil_quads_vertex_element_array = GLVertexElementArray::create(
			quad_indices,
			GLArray::USAGE_STATIC,
			d_vertex_buffer_resource_manager);
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::FilledPolygon::initialise_drawable(
		const std::vector<coloured_vertex_type> &triangles_vertices,
		const std::vector<GLuint> &triangles_vertex_indices)
{
	//PROFILE_FUNC();

	// NOTE: These are plain system memory vertex arrays (we don't pass in the vertex buffer
	// resource manager) and hence are cheaper to create but not as fast. We want cheap to
	// create because we throw them away after they are rendered.
	// TODO: Use streaming vertex buffer objects.

	// Create a vertex array.
	GPlatesOpenGL::GLVertexArray::shared_ptr_type vertex_array =
			GPlatesOpenGL::GLVertexArray::create(triangles_vertices);

	// Create a vertex element array.
	GPlatesOpenGL::GLVertexElementArray::shared_ptr_type vertex_element_array =
			GPlatesOpenGL::GLVertexElementArray::create(triangles_vertex_indices);

	// Specify what to draw.
	vertex_element_array->gl_draw_range_elements_EXT(
			GL_TRIANGLES,
			0/*start*/,
			triangles_vertices.size() - 1/*end*/,
			triangles_vertex_indices.size()/*count*/,
			0 /*indices_offset*/);

	// Create a drawable using the vertex array and vertex element array.
	d_drawable = GPlatesOpenGL::GLVertexArrayDrawable::create(vertex_array, vertex_element_array);
}
