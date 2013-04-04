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
#include <limits>
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

#include "GLContext.h"
#include "GLIntersect.h"
#include "GLMatrix.h"
#include "GLProjectionUtils.h"
#include "GLRenderer.h"
#include "GLShaderProgramUtils.h"
#include "GLUtils.h"
#include "GLVertex.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"

#include "gui/Colour.h"

#include "utils/Base2Utils.h"
#include "utils/IntrusiveSinglyLinkedList.h"
#include "utils/Profile.h"


namespace GPlatesOpenGL
{
	namespace
	{
		//! The inverse of log(2.0).
		const float INVERSE_LOG2 = 1.0 / std::log(2.0);

		//! Vertex shader source code to render a tile to the scene.
		const QString RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE_FILE_NAME =
				":/opengl/multi_resolution_filled_polygons/render_tile_to_scene_vertex_shader.glsl";

		//! Fragment shader source code to render a tile to the scene.
		const QString RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE_FILE_NAME =
				":/opengl/multi_resolution_filled_polygons/render_tile_to_scene_fragment_shader.glsl";
	}
}


GPlatesOpenGL::GLMultiResolutionFilledPolygons::GLMultiResolutionFilledPolygons(
		GLRenderer &renderer,
		const GLMultiResolutionCubeMesh::non_null_ptr_to_const_type &multi_resolution_cube_mesh,
		boost::optional<GLLight::non_null_ptr_type> light) :
	d_max_tile_texel_dimension(
			// Make sure tile dimensions does not exceed maximum texture dimensions...
			(MAX_TILE_TEXEL_DIMENSION > renderer.get_capabilities().texture.gl_max_texture_size)
					? renderer.get_capabilities().texture.gl_max_texture_size
					: MAX_TILE_TEXEL_DIMENSION),
	d_min_tile_texel_dimension(
			// Make sure tile dimensions does not exceed maximum texture dimensions...
			(MIN_TILE_TEXEL_DIMENSION > renderer.get_capabilities().texture.gl_max_texture_size)
					? renderer.get_capabilities().texture.gl_max_texture_size
					: MIN_TILE_TEXEL_DIMENSION),
	d_multi_resolution_cube_mesh(multi_resolution_cube_mesh),
	d_light(light)
{
	// We want a power-of-two tile texel dimension range to ensure power-of-two tiles
	// which enhances re-use of acquired tile textures and depth/stencil buffers.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			MAX_TILE_TEXEL_DIMENSION >= MIN_TILE_TEXEL_DIMENSION &&
				GPlatesUtils::Base2::is_power_of_two(MAX_TILE_TEXEL_DIMENSION) &&
				GPlatesUtils::Base2::is_power_of_two(MIN_TILE_TEXEL_DIMENSION),
			GPLATES_ASSERTION_SOURCE);
	if (!GPlatesUtils::Base2::is_power_of_two(d_max_tile_texel_dimension))
	{
		d_max_tile_texel_dimension = GPlatesUtils::Base2::previous_power_of_two(d_max_tile_texel_dimension);
	}
	if (!GPlatesUtils::Base2::is_power_of_two(d_min_tile_texel_dimension))
	{
		d_min_tile_texel_dimension = GPlatesUtils::Base2::previous_power_of_two(d_min_tile_texel_dimension);
	}

	create_polygons_vertex_array(renderer);

	// If there's support for shader programs then create them.
	create_shader_programs(renderer);
}


unsigned int
GPlatesOpenGL::GLMultiResolutionFilledPolygons::get_level_of_detail(
		unsigned int &tile_texel_dimension,
		const GLViewport &viewport,
		const GLMatrix &model_view_transform,
		const GLMatrix &projection_transform) const
{
	// Start with the highest tile texel dimension - we will reduce it if we can.
	tile_texel_dimension = d_max_tile_texel_dimension;

	// Get the minimum size of a pixel in the current viewport when projected
	// onto the unit sphere (in model space).
	const double min_pixel_size_on_unit_sphere =
			GLProjectionUtils::get_min_pixel_size_on_unit_sphere(
					viewport, model_view_transform, projection_transform);

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
	const float max_lowest_resolution_texel_size_on_unit_sphere = 2.0 / tile_texel_dimension;

	float level_of_detail_factor = INVERSE_LOG2 *
			(std::log(max_lowest_resolution_texel_size_on_unit_sphere) -
					std::log(min_pixel_size_on_unit_sphere));

	// Reduce the tile texel dimension (by factors of two) if we don't need the extra resolution.
	while (level_of_detail_factor < -1 && tile_texel_dimension > d_min_tile_texel_dimension)
	{
		level_of_detail_factor += 1;
		tile_texel_dimension >>= 1;
	}

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


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::render(
		GLRenderer &renderer,
		const filled_polygons_type &filled_polygons)
{
	PROFILE_FUNC();

	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// If there are no filled polygons to render then return early.
	if (filled_polygons.d_polygon_vertex_elements.empty())
	{
		return;
	}

	// Write the vertices/indices of all filled polygons (gathered by the client) into our
	// vertex buffer and vertex element buffer.
	write_filled_polygon_meshes_to_vertex_array(renderer, filled_polygons);

	// Get the level-of-detail based on the size of viewport pixels projected onto the globe.
	unsigned int tile_texel_dimension;
	const unsigned int render_level_of_detail = get_level_of_detail(
			tile_texel_dimension,
			renderer.gl_get_viewport(),
			renderer.gl_get_matrix(GL_MODELVIEW),
			renderer.gl_get_matrix(GL_PROJECTION));

	// Get the view frustum planes.
	const GLFrustum frustum_planes(
			renderer.gl_get_matrix(GL_MODELVIEW),
			renderer.gl_get_matrix(GL_PROJECTION));

	// Create a subdivision cube quad tree traversal.
	// No caching is required since we're only visiting each subdivision node once.
	//
	// Cube subdivision cache for half-texel-expanded projection transforms since that is what's used to
	// lookup the tile textures (the tile textures are bilinearly filtered and the centres of
	// border texels match up with adjacent tiles).
	cube_subdivision_cache_type::non_null_ptr_type
			cube_subdivision_cache =
					cube_subdivision_cache_type::create(
							GPlatesOpenGL::GLCubeSubdivision::create(
									GPlatesOpenGL::GLCubeSubdivision::get_expand_frustum_ratio(
											tile_texel_dimension,
											0.5/* half a texel */)));
	// Cube subdivision cache for the clip texture (no frustum expansion here).
	clip_cube_subdivision_cache_type::non_null_ptr_type
			clip_cube_subdivision_cache =
					clip_cube_subdivision_cache_type::create(
							GPlatesOpenGL::GLCubeSubdivision::create());

	//
	// Traverse the source raster cube quad tree and the spatial partition of reconstructed polygon meshes.
	//

	// Traverse the quad trees of the cube faces.
	for (unsigned int face = 0; face < 6; ++face)
	{
		const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
				static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

		// Get the quad tree root node of the current cube face of the source mesh.
		const mesh_quad_tree_node_type mesh_quad_tree_root_node =
				d_multi_resolution_cube_mesh->get_quad_tree_root_node(cube_face);

		// This is used to find those nodes of the reconstructed polygon meshes spatial partition
		// that intersect the source raster cube quad tree.
		// This is so we know which polygon meshes to draw for each source raster tile.
		filled_polygons_intersecting_nodes_type
				filled_polygons_intersecting_nodes(
						*filled_polygons.d_filled_polygons_spatial_partition,
						cube_face);

		// Get the cube subdivision root node.
		const cube_subdivision_cache_type::node_reference_type
				cube_subdivision_cache_root_node =
						cube_subdivision_cache->get_quad_tree_root_node(cube_face);
		// Get the cube subdivision root node.
		const clip_cube_subdivision_cache_type::node_reference_type
				clip_cube_subdivision_cache_root_node =
						clip_cube_subdivision_cache->get_quad_tree_root_node(cube_face);

		// Initially there are no intersecting nodes...
		filled_polygons_spatial_partition_node_list_type filled_polygons_spatial_partition_node_list;

		render_quad_tree(
				renderer,
				tile_texel_dimension,
				mesh_quad_tree_root_node,
				filled_polygons,
				filled_polygons_spatial_partition_node_list,
				filled_polygons_intersecting_nodes,
				*cube_subdivision_cache,
				cube_subdivision_cache_root_node,
				*clip_cube_subdivision_cache,
				clip_cube_subdivision_cache_root_node,
				0/*level_of_detail*/,
				render_level_of_detail,
				frustum_planes,
				// There are six frustum planes initially active
				GLFrustum::ALL_PLANES_ACTIVE_MASK);
	}
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::render_quad_tree(
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
		boost::uint32_t frustum_plane_mask)
{
	// If the frustum plane mask is zero then it means we are entirely inside the view frustum.
	// So only test for intersection if the mask is non-zero.
	if (frustum_plane_mask != 0)
	{
		const GLIntersect::OrientedBoundingBox quad_tree_node_bounds =
				cube_subdivision_cache.get_oriented_bounding_box(
						cube_subdivision_cache_node);

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
				tile_texel_dimension,
				mesh_quad_tree_node,
				filled_polygons,
				parent_filled_polygons_intersecting_node_list,
				filled_polygons_intersecting_nodes,
				cube_subdivision_cache,
				cube_subdivision_cache_node,
				clip_cube_subdivision_cache,
				clip_cube_subdivision_cache_node);

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
			// current node. The parent list contains the nodes we've been
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

			// Get the child cube subdivision cache node.
			const cube_subdivision_cache_type::node_reference_type
					child_cube_subdivision_cache_node =
							cube_subdivision_cache.get_child_node(
									cube_subdivision_cache_node,
									child_u_offset,
									child_v_offset);
			// Get the child clip cube subdivision cache node.
			const clip_cube_subdivision_cache_type::node_reference_type
					child_clip_cube_subdivision_cache_node =
							clip_cube_subdivision_cache.get_child_node(
									clip_cube_subdivision_cache_node,
									child_u_offset,
									child_v_offset);

			render_quad_tree(
					renderer,
					tile_texel_dimension,
					child_mesh_quad_tree_node,
					filled_polygons,
					child_filled_polygons_intersecting_node_list,
					child_filled_polygons_intersecting_nodes,
					cube_subdivision_cache,
					child_cube_subdivision_cache_node,
					clip_cube_subdivision_cache,
					child_clip_cube_subdivision_cache_node,
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
		unsigned int tile_texel_dimension,
		const mesh_quad_tree_node_type &mesh_quad_tree_node,
		const filled_polygons_type &filled_polygons,
		const filled_polygons_spatial_partition_node_list_type &parent_filled_polygons_intersecting_node_list,
		const filled_polygons_intersecting_nodes_type &filled_polygons_intersecting_nodes,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
		clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
		const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node)
{
	// From here on we can't allocate the list nodes on the runtime stack because we need to access
	// the list after we return from traversing the spatial partition. So use an object pool instead.
	boost::object_pool<FilledPolygonsListNode> filled_polygons_list_node_pool;

	// A tail-shared list to contain the reconstructed polygon meshes nodes that intersect the
	// current source raster node. The parent list contains the nodes we've been
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
			tile_texel_dimension,
			mesh_quad_tree_node,
			filled_polygons,
			filled_polygons_intersecting_node_list,
			cube_subdivision_cache,
			cube_subdivision_cache_node,
			clip_cube_subdivision_cache,
			clip_cube_subdivision_cache_node);
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


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::set_tile_state(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &tile_texture,
		const GLTransform &projection_transform,
		const GLTransform &clip_projection_transform,
		const GLTransform &view_transform,
		bool clip_to_tile_frustum)
{
	// Used to transform texture coordinates to account for partial coverage of current tile.
	GLMatrix scene_tile_texture_matrix;
	scene_tile_texture_matrix.gl_mult_matrix(GLUtils::get_clip_space_to_texture_space_transform());
	// Set up the texture matrix to perform model-view and projection transforms of the frustum.
	scene_tile_texture_matrix.gl_mult_matrix(projection_transform.get_matrix());
	scene_tile_texture_matrix.gl_mult_matrix(view_transform.get_matrix());
	// Load texture transform into texture unit 0.
	renderer.gl_load_texture_matrix(GL_TEXTURE0, scene_tile_texture_matrix);

	// Bind the scene tile texture to texture unit 0.
	renderer.gl_bind_texture(tile_texture, GL_TEXTURE0, GL_TEXTURE_2D);

	// If we've traversed deep enough into the cube quad tree then the cube quad tree mesh
	// cannot provide a drawable that's bounded by the cube quad tree node tile and so
	// we need to use a clip texture.
	if (clip_to_tile_frustum)
	{
		// NOTE: If two texture units are not supported then just don't clip to the tile.
		// It'll look worse but at least it'll still work mostly and will only be noticeable
		// if they zoom in far enough (which is when this code gets activated).
		if (renderer.get_capabilities().texture.gl_max_texture_units >= 2)
		{
			// State for the clip texture.
			//
			// NOTE: We also do *not* expand the tile frustum since the clip texture uses nearest
			// filtering instead of bilinear filtering and hence we're not removing a seam between
			// tiles (instead we are clipping adjacent tiles).
			GLMatrix clip_texture_matrix(GLTextureUtils::get_clip_texture_clip_space_to_texture_space_transform());
			// Set up the texture matrix to perform model-view and projection transforms of the frustum.
			clip_texture_matrix.gl_mult_matrix(clip_projection_transform.get_matrix());
			clip_texture_matrix.gl_mult_matrix(view_transform.get_matrix());
			// Load texture transform into texture unit 1.
			renderer.gl_load_texture_matrix(GL_TEXTURE1, clip_texture_matrix);

			// Bind the clip texture to texture unit 1.
			renderer.gl_bind_texture(d_multi_resolution_cube_mesh->get_clip_texture(), GL_TEXTURE1, GL_TEXTURE_2D);
		}
		else
		{
			// Only emit warning message once.
			static bool emitted_warning = false;
			if (!emitted_warning)
			{
				qWarning() <<
						"High zoom levels of filled polygons NOT supported by this OpenGL system - \n"
						"  requires two texture units - visual results will be incorrect.\n"
						"  Most graphics hardware for over a decade supports this -\n"
						"  most likely software renderer fallback has occurred - possibly via remote desktop software.";
				emitted_warning = true;
			}
		}
	}

	// Use shader program (if supported), otherwise the fixed-function pipeline.
	if (d_render_tile_to_scene_program_object &&
		d_render_tile_to_scene_with_clipping_program_object &&
		d_render_tile_to_scene_with_lighting_program_object &&
		d_render_tile_to_scene_with_clipping_and_lighting_program_object)
	{
		boost::optional<GLProgramObject::shared_ptr_type> program_object;

		const bool lighting_enabled = d_light && d_light.get()->get_scene_lighting_parameters().is_lighting_enabled();

		// Determine which shader program to use.
		if (clip_to_tile_frustum)
		{
			program_object = lighting_enabled
					? d_render_tile_to_scene_with_clipping_and_lighting_program_object
					: d_render_tile_to_scene_with_clipping_program_object;
		}
		else
		{
			program_object = lighting_enabled
					? d_render_tile_to_scene_with_lighting_program_object
					: d_render_tile_to_scene_program_object;
		}

		// Bind the shader program.
		renderer.gl_bind_program_object(program_object.get());

		// Set the tile texture sampler to texture unit 0.
		program_object.get()->gl_uniform1i(renderer, "tile_texture_sampler", 0/*texture unit*/);

		if (clip_to_tile_frustum)
		{
			// Set the clip texture sampler to texture unit 1.
			program_object.get()->gl_uniform1i(renderer, "clip_texture_sampler", 1/*texture unit*/);
		}

		if (lighting_enabled)
		{
			// Set the world-space light direction.
			program_object.get()->gl_uniform3f(
					renderer,
					"world_space_light_direction",
					d_light.get()->get_globe_view_light_direction(renderer));

			// Set the light ambient contribution.
			program_object.get()->gl_uniform1f(
					renderer,
					"light_ambient_contribution",
					d_light.get()->get_scene_lighting_parameters().get_ambient_light_contribution());
		}

		// Instead of alpha-testing, the fragment shader uses 'discard'.
	}
	else // Fixed function (no lighting supported)...
	{
		// Enable texturing and set the texture function on texture unit 0.
		renderer.gl_enable_texture(GL_TEXTURE0, GL_TEXTURE_2D);
		renderer.gl_tex_env(GL_TEXTURE0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		// Set up texture coordinate generation from the vertices (x,y,z) on texture unit 0.
		GLUtils::set_object_linear_tex_gen_state(renderer, 0/*texture_unit*/);

		if (clip_to_tile_frustum)
		{
			// NOTE: If two texture units are not supported then just don't clip to the tile.
			if (renderer.get_capabilities().texture.gl_max_texture_units >= 2)
			{
				// Enable texturing and set the texture function on texture unit 1.
				renderer.gl_enable_texture(GL_TEXTURE1, GL_TEXTURE_2D);
				renderer.gl_tex_env(GL_TEXTURE1, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				// Set up texture coordinate generation from the vertices (x,y,z) on texture unit 1.
				GLUtils::set_object_linear_tex_gen_state(renderer, 1/*texture_unit*/);
			}
		}

		// Alpha-test state.
		// This enables alpha texture clipping when tile frustum is smaller than the multi-resolution
		// cube mesh drawable. It's also a small optimisation for those areas of the tile that are
		// not covered by any polygons (alpha-testing those pixels away avoids the alpha-blending stage).
		renderer.gl_enable(GL_ALPHA_TEST);
		renderer.gl_alpha_func(GL_GREATER, GLclampf(0));
	}

#if 0
	// Used to render as wire-frame meshes instead of filled textured meshes for
	// visualising mesh density.
	renderer.gl_polygon_mode(GL_FRONT_AND_BACK, GL_LINE);
#endif
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::render_tile_to_scene(
		GLRenderer &renderer,
		unsigned int tile_texel_dimension,
		const mesh_quad_tree_node_type &mesh_quad_tree_node,
		const filled_polygons_type &filled_polygons,
		const filled_polygons_spatial_partition_node_list_type &filled_polygons_intersecting_node_list,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
		clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
		const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node)
{
	PROFILE_FUNC();

	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	const filled_polygons_spatial_partition_type &filled_polygons_spatial_partition =
			*filled_polygons.d_filled_polygons_spatial_partition;

	// Get the reconstructed polygon meshes.
	filled_polygon_seq_type filled_drawables;
	get_filled_polygons(
			filled_drawables,
			filled_polygons_spatial_partition.begin_root_elements(),
			filled_polygons_spatial_partition.end_root_elements(),
			filled_polygons_intersecting_node_list);

	if (filled_drawables.empty())
	{
		return;
	}

	// The view transform never changes within a cube face so it's the same across
	// an entire cube face quad tree (each cube face has its own quad tree).
	const GLTransform::non_null_ptr_to_const_type view_transform =
			cube_subdivision_cache.get_view_transform(
					cube_subdivision_cache_node);

	// Regular projection transform.
	const GLTransform::non_null_ptr_to_const_type projection_transform =
			cube_subdivision_cache.get_projection_transform(
					cube_subdivision_cache_node);

	// Clip texture projection transform.
	const GLTransform::non_null_ptr_to_const_type clip_projection_transform =
			clip_cube_subdivision_cache.get_projection_transform(
					clip_cube_subdivision_cache_node);

	// Get an unused tile texture from our texture cache.
	const GLTexture::shared_ptr_to_const_type tile_texture =
			acquire_tile_texture(renderer, tile_texel_dimension);

	// Render the filled polygons to the tile texture.
	render_filled_polygons_to_tile_texture(
			renderer,
			tile_texture,
			filled_drawables,
			*projection_transform,
			*view_transform);

	// See if we've traversed deep enough in the cube mesh quad tree to require using a clip
	// texture - this occurs because the cube mesh has nodes only to a certain depth.
	const bool clip_to_tile_frustum = mesh_quad_tree_node.get_clip_texture_clip_space_transform();

	// Prepare for rendering the current tile.
	set_tile_state(
			renderer,
			tile_texture,
			*projection_transform,
			*clip_projection_transform,
			*view_transform,
			clip_to_tile_frustum);

	// Draw the mesh covering the current quad tree node tile.
	mesh_quad_tree_node.render_mesh_drawable(renderer);
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::render_filled_polygons_to_tile_texture(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &tile_texture,
		const filled_polygon_seq_type &filled_drawables,
		const GLTransform &projection_transform,
		const GLTransform &view_transform)
{
	PROFILE_FUNC();

	// Begin a render target that will render the individual filled polygons to the tile texture.
	// Enable stencil buffering since we use it to fill each polygon.
	GLRenderer::RenderTarget2DScope render_target_scope(
			renderer,
			tile_texture,
			boost::none/*viewport covers entire texture*/,
			0/*max_point_size_and_line_width*/,
			0/*level*/,
			true/*reset_to_default_state*/,
			false/*depth_buffer*/,
			true/*stencil_buffer*/);

	// The render target tiling loop...
	do
	{
		// Begin the current render target tile - this also sets the viewport.
		GLTransform::non_null_ptr_to_const_type tile_projection = render_target_scope.begin_tile();

		// Set up the projection transform adjustment for the current render target tile.
		renderer.gl_load_matrix(GL_PROJECTION, tile_projection->get_matrix());
		// NOTE: We use the half-texel-expanded projection transform since we want to render the
		// border pixels (in each tile) exactly on the tile (plane) boundary.
		// The tile textures are bilinearly filtered and this way the centres of border texels match up
		// with adjacent tiles.
		renderer.gl_mult_matrix(GL_PROJECTION, projection_transform.get_matrix());

		renderer.gl_load_matrix(GL_MODELVIEW, view_transform.get_matrix());

		// Clear the render target (colour and stencil).
		// We also clear the depth buffer (even though we're not using depth) because it's usually
		// interleaved with stencil so it's more efficient to clear both depth and stencil.
		renderer.gl_clear_color();
		renderer.gl_clear_depth();
		renderer.gl_clear_stencil();
		renderer.gl_clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		// Set the alpha-blend state since filled polygon could have a transparent colour.
		// Set up alpha blending for pre-multiplied alpha.
		// This has (src,dst) blend factors of (1, 1-src_alpha) instead of (src_alpha, 1-src_alpha).
		// This is where the RGB channels have already been multiplied by the alpha channel.
		// See class GLVisualRasterSource for why this is done.
		renderer.gl_blend_func(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

		// Enable stencil writes (this is the default OpenGL state anyway).
		renderer.gl_stencil_mask(~0);

		// Enable stencil testing.
		renderer.gl_enable(GL_STENCIL_TEST);

#if 0
		// Used to render as wire-frame meshes instead of filled textured meshes for
		// visualising mesh density.
		renderer.gl_polygon_mode(GL_FRONT_AND_BACK, GL_POINT);

		// Set the anti-aliased line state.
		renderer.gl_line_width(10.0f);
		renderer.gl_point_size(10.0f);
		//renderer.gl_enable(GL_LINE_SMOOTH);
		//renderer.gl_hint(GL_LINE_SMOOTH_HINT, GL_NICEST);
#endif

		// Bind the vertex array before using it to draw.
		d_polygons_vertex_array->gl_bind(renderer);

		// Iterate over the filled drawables and render each one into the tile texture.
		filled_polygon_seq_type::const_iterator filled_drawables_iter = filled_drawables.begin();
		filled_polygon_seq_type::const_iterator filled_drawables_end = filled_drawables.end();
		for ( ; filled_drawables_iter != filled_drawables_end; ++filled_drawables_iter)
		{
			const filled_polygon_type &filled_drawable = *filled_drawables_iter;

			// Set the stencil function to always pass.
			renderer.gl_stencil_func(GL_ALWAYS, 0, ~0);
			// Set the stencil operation to invert the stencil buffer value every time a pixel is
			// rendered (this means we get 1 where a pixel is covered by an odd number of triangles
			// and 0 by an even number of triangles).
			renderer.gl_stencil_op(GL_KEEP, GL_KEEP, GL_INVERT);

			// Disable colour writes and alpha blending.
			// We only want to modify the stencil buffer on this pass.
			renderer.gl_color_mask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			renderer.gl_enable(GL_BLEND, false);

			// Render the current filled polygon as a polygon (fan) mesh.
			d_polygons_vertex_array->gl_draw_range_elements(
					renderer,
					GL_TRIANGLES,
					filled_drawable.d_polygon_mesh_drawable.start,
					filled_drawable.d_polygon_mesh_drawable.end,
					filled_drawable.d_polygon_mesh_drawable.count,
					GLVertexElementTraits<polygon_vertex_element_type>::type,
					filled_drawable.d_polygon_mesh_drawable.indices_offset);

			// Set the stencil function to pass only if the stencil buffer value is non-zero.
			// This means we only draw into the tile texture for pixels 'interior' to the filled polygon.
			renderer.gl_stencil_func(GL_NOTEQUAL, 0, ~0);
			// Set the stencil operation to set the stencil buffer to zero in preparation
			// for the next polygon (also avoids multiple alpha-blending due to overlapping fan
			// triangles as mentioned below).
			renderer.gl_stencil_op(GL_KEEP, GL_KEEP, GL_ZERO);

			// Re-enable colour writes and alpha blending.
			renderer.gl_color_mask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			renderer.gl_enable(GL_BLEND, true);

			// Render the current filled polygon as a polygon (fan) mesh again.
			// This drawable covers at least all interior pixels of the filled polygon.
			// It also can covers exterior pixels of the filled polygon.
			// However only the interior pixels (where stencil buffer is non-zero) will
			// pass the stencil test and get written into the tile (colour) texture.
			// The drawable also can render pixels multiple times due to overlapping fan triangles.
			// To avoid alpha blending each pixel more than once, the above stencil operation zeros
			// the stencil buffer value of each pixel that passes the stencil test such that the next
			// overlapping pixel will then fail the stencil test (avoiding multiple-alpha-blending).
			d_polygons_vertex_array->gl_draw_range_elements(
					renderer,
					GL_TRIANGLES,
					filled_drawable.d_polygon_mesh_drawable.start,
					filled_drawable.d_polygon_mesh_drawable.end,
					filled_drawable.d_polygon_mesh_drawable.count,
					GLVertexElementTraits<polygon_vertex_element_type>::type,
					filled_drawable.d_polygon_mesh_drawable.indices_offset);
		}
	}
	while (render_target_scope.end_tile());
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::get_filled_polygons(
		filled_polygon_seq_type &filled_drawables,
		filled_polygons_spatial_partition_type::element_const_iterator begin_root_filled_polygons,
		filled_polygons_spatial_partition_type::element_const_iterator end_root_filled_polygons,
		const filled_polygons_spatial_partition_node_list_type &filled_polygons_intersecting_node_list)
{
	PROFILE_FUNC();

	// Add the reconstructed polygon meshes in the root of the spatial partition.
	// These are the meshes that were too large to insert in any face of the cube quad tree partition.
	// Add the reconstructed polygon meshes of the current node.
	filled_drawables.insert(
			filled_drawables.end(),
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
		filled_drawables.insert(
				filled_drawables.end(),
				node_reference.begin(),
				node_reference.end());
	}
}

DISABLE_GCC_WARNING("-Wold-style-cast")

GPlatesOpenGL::GLTexture::shared_ptr_type
GPlatesOpenGL::GLMultiResolutionFilledPolygons::acquire_tile_texture(
		GLRenderer &renderer,
		unsigned int tile_texel_dimension)
{
	PROFILE_FUNC();

	// Acquire a cached texture for rendering a tile to.
	// It'll get returned to its cache when we no longer reference it.
	const GLTexture::shared_ptr_type tile_texture =
			renderer.get_context().get_shared_state()->acquire_texture(
					renderer,
					GL_TEXTURE_2D,
					GL_RGBA8,
					tile_texel_dimension,
					tile_texel_dimension);

	// 'acquire_texture' initialises the texture memory (to empty) but does not set the filtering
	// state when it creates a new texture.
	// Also even if the texture was cached it might have been used by another client that specified
	// different filtering settings for it.
	// So we set the filtering settings each time we acquire.

	// No mipmaps needed so we specify no mipmap filtering.
	// We're not using mipmaps because our cube mapping does not have much distortion
	// unlike global rectangular lat/lon rasters that squash near the poles.
	//
	// We do enable bilinear filtering (also note that the texture is a fixed-point format).
	tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Specify anisotropic filtering if it's supported since we are not using mipmaps
	// and any textures rendered near the edge of the globe will get squashed a bit due to
	// the angle we are looking at them and anisotropic filtering will help here.
	if (GLEW_EXT_texture_filter_anisotropic)
	{
		const GLfloat anisotropy = renderer.get_capabilities().texture.gl_texture_max_anisotropy;
		tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	}

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	if (GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp)
	{
		tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		tile_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	return tile_texture;
}

ENABLE_GCC_WARNING("-Wold-style-cast")


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::create_polygons_vertex_array(
		GLRenderer &renderer)
{
	d_polygons_vertex_array = GLVertexArray::create(renderer);

	// Set up the vertex element buffer.
	GLBuffer::shared_ptr_type vertex_element_buffer_data = GLBuffer::create(renderer);
	d_polygons_vertex_element_buffer = GLVertexElementBuffer::create(renderer, vertex_element_buffer_data);
	// Attach vertex element buffer to the vertex array.
	d_polygons_vertex_array->set_vertex_element_buffer(renderer, d_polygons_vertex_element_buffer);

	// Set up the vertex buffer.
	GLBuffer::shared_ptr_type vertex_buffer_data = GLBuffer::create(renderer);
	d_polygons_vertex_buffer = GLVertexBuffer::create(renderer, vertex_buffer_data);

	// Attach polygons vertex buffer to the vertex array.
	//
	// Later we'll be allocating a vertex buffer large enough to contain all polygons and
	// rendering each polygon with its own OpenGL draw call.
	bind_vertex_buffer_to_vertex_array<polygon_vertex_type>(
			renderer,
			*d_polygons_vertex_array,
			d_polygons_vertex_buffer);
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::write_filled_polygon_meshes_to_vertex_array(
		GLRenderer &renderer,
		const filled_polygons_type &filled_polygons)
{
	PROFILE_FUNC();

	// It's not 'stream' because the same filled polygons are accessed many times.
	// It's not 'dynamic' because we allocate a new buffer (ie, glBufferData does not modify existing buffer).
	// We really want to encourage this to be in video memory (even though it's only going to live
	// there for a single rendering frame) because there are many accesses to this buffer as the same
	// polygons are rendered into multiple tiles (otherwise the PCI bus bandwidth becomes the limiting factor).

	GLBuffer::shared_ptr_type vertex_element_buffer_data = d_polygons_vertex_element_buffer->get_buffer();
	vertex_element_buffer_data->gl_buffer_data(
			renderer,
			GLBuffer::TARGET_ELEMENT_ARRAY_BUFFER,
			filled_polygons.d_polygon_vertex_elements,
			GLBuffer::USAGE_STATIC_DRAW);

	GLBuffer::shared_ptr_type vertex_buffer_data = d_polygons_vertex_buffer->get_buffer();
	vertex_buffer_data->gl_buffer_data(
			renderer,
			GLBuffer::TARGET_ARRAY_BUFFER,
			filled_polygons.d_polygon_vertices,
			GLBuffer::USAGE_STATIC_DRAW);

	//qDebug() << "Writing triangles: " << filled_polygons.d_polygon_vertex_elements.size() / 3;
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::create_shader_programs(
		GLRenderer &renderer)
{
	//
	// Shader programs for the final stage of rendering a tile to the scene.
	// To enhance (or remove effect of) anti-aliasing of polygons edges.
	//

	// A version without clipping or lighting.
	d_render_tile_to_scene_program_object =
			GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
					renderer,
					GLShaderProgramUtils::ShaderSource::create_shader_source_from_file(
							RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE_FILE_NAME),
					GLShaderProgramUtils::ShaderSource::create_shader_source_from_file(
							RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE_FILE_NAME));

	// A version with clipping but without lighting.
	GLShaderProgramUtils::ShaderSource render_tile_to_scene_with_clipping_vertex_shader_source;
	render_tile_to_scene_with_clipping_vertex_shader_source.add_shader_source("#define ENABLE_CLIPPING\n");
	render_tile_to_scene_with_clipping_vertex_shader_source.add_shader_source_from_file(
			GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
	render_tile_to_scene_with_clipping_vertex_shader_source.add_shader_source_from_file(
			RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE_FILE_NAME);
	GLShaderProgramUtils::ShaderSource render_tile_to_scene_with_clipping_fragment_shader_source;
	render_tile_to_scene_with_clipping_fragment_shader_source.add_shader_source("#define ENABLE_CLIPPING\n");
	render_tile_to_scene_with_clipping_fragment_shader_source.add_shader_source_from_file(
			GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
	render_tile_to_scene_with_clipping_fragment_shader_source.add_shader_source_from_file(
			RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE_FILE_NAME);
	d_render_tile_to_scene_with_clipping_program_object =
			GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
					renderer,
					render_tile_to_scene_with_clipping_vertex_shader_source,
					render_tile_to_scene_with_clipping_fragment_shader_source);

	// A version with lighting but without clipping.
	GLShaderProgramUtils::ShaderSource render_tile_to_scene_with_lighting_vertex_shader_source;
	render_tile_to_scene_with_lighting_vertex_shader_source.add_shader_source("#define SURFACE_LIGHTING\n");
	render_tile_to_scene_with_lighting_vertex_shader_source.add_shader_source_from_file(
			GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
	render_tile_to_scene_with_lighting_vertex_shader_source.add_shader_source_from_file(
			RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE_FILE_NAME);
	GLShaderProgramUtils::ShaderSource render_tile_to_scene_with_lighting_fragment_shader_source;
	render_tile_to_scene_with_lighting_fragment_shader_source.add_shader_source("#define SURFACE_LIGHTING\n");
	render_tile_to_scene_with_lighting_fragment_shader_source.add_shader_source_from_file(
			GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
	render_tile_to_scene_with_lighting_fragment_shader_source.add_shader_source_from_file(
			RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE_FILE_NAME);
	d_render_tile_to_scene_with_lighting_program_object =
			GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
					renderer,
					render_tile_to_scene_with_lighting_vertex_shader_source,
					render_tile_to_scene_with_lighting_fragment_shader_source);

	// A version with clipping and lighting.
	GLShaderProgramUtils::ShaderSource render_tile_to_scene_with_clipping_and_lighting_vertex_shader_source;
	render_tile_to_scene_with_clipping_and_lighting_vertex_shader_source.add_shader_source(
			"#define ENABLE_CLIPPING\n"
			"#define SURFACE_LIGHTING\n");
	render_tile_to_scene_with_clipping_and_lighting_vertex_shader_source.add_shader_source_from_file(
			GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
	render_tile_to_scene_with_clipping_and_lighting_vertex_shader_source.add_shader_source_from_file(
			RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE_FILE_NAME);
	GLShaderProgramUtils::ShaderSource render_tile_to_scene_with_clipping_and_lighting_fragment_shader_source;
	render_tile_to_scene_with_clipping_and_lighting_fragment_shader_source.add_shader_source(
			"#define ENABLE_CLIPPING\n"
			"#define SURFACE_LIGHTING\n");
	render_tile_to_scene_with_clipping_and_lighting_fragment_shader_source.add_shader_source_from_file(
			GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);
	render_tile_to_scene_with_clipping_and_lighting_fragment_shader_source.add_shader_source_from_file(
			RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE_FILE_NAME);
	d_render_tile_to_scene_with_clipping_and_lighting_program_object =
			GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
					renderer,
					render_tile_to_scene_with_clipping_and_lighting_vertex_shader_source,
					render_tile_to_scene_with_clipping_and_lighting_fragment_shader_source);
}
