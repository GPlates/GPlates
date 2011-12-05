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

#include "GLContext.h"
#include "GLIntersect.h"
#include "GLMatrix.h"
#include "GLProjectionUtils.h"
#include "GLRenderer.h"
#include "GLShaderProgramUtils.h"
#include "GLUtils.h"
#include "GLVertex.h"

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
	}

	//! Vertex shader source code to render a tile to the scene.
	const char *RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE =
			"void main (void)\n"
			"{\n"

			"	// Ensure position is transformed exactly same as fixed-function pipeline.\n"
			"	gl_Position = ftransform(); //gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"

			"	// Transform present-day position by cube map projection and\n"
			"	// any texture coordinate adjustments before accessing textures.\n"
			"	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_Vertex;\n"
			"	gl_TexCoord[1] = gl_TextureMatrix[1] * gl_Vertex;\n"

			"}\n";

	/**
	 * Fragment shader source code to render a tile to the scene.
	 *
	 * If we're near the edge of a polygon (and there's no adjacent polygon)
	 * then the fragment alpha will not be 1.0 (also happens if clipped).
	 * This reduces the anti-aliasing affect of the bilinear filtering since the bilinearly
	 * filtered alpha will soften the edge (during the alpha-blend stage) but also the RGB colour
	 * has been bilinearly filtered with black (RGB of zero) which is a double-reduction that
	 * reduces the softness of the anti-aliasing.
	 * To get around this we revert the effect of blending with black leaving only the alpha-blending.
	 */
	const char *RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE =
			"uniform sampler2D tile_texture_sampler;\n"

			"#ifdef ENABLE_CLIPPING\n"
			"uniform sampler2D clip_texture_sampler;\n"
			"#endif // ENABLE_CLIPPING\n"

			"void main (void)\n"
			"{\n"

			"	// Projective texturing to handle cube map projection.\n"
			"	gl_FragColor = texture2DProj(tile_texture_sampler, gl_TexCoord[0]);\n"

			"#ifdef ENABLE_CLIPPING\n"
			"	gl_FragColor *= texture2DProj(clip_texture_sampler, gl_TexCoord[1]);\n"
			"#endif // ENABLE_CLIPPING\n"

			"	// Revert effect of blending with black texels near polygon edge.\n"
			"	if (gl_FragColor.a > 0)\n"
			"	{\n"
			"		gl_FragColor.rgb /= gl_FragColor.a;\n"
			"	}\n"

			"}\n";
}


GPlatesOpenGL::GLMultiResolutionFilledPolygons::GLMultiResolutionFilledPolygons(
		GLRenderer &renderer,
		const GLMultiResolutionCubeMesh::non_null_ptr_to_const_type &multi_resolution_cube_mesh) :
	d_texture_cache(GPlatesUtils::ObjectCache<GLTexture>::create()),
	d_tile_texel_dimension(DEFAULT_TILE_TEXEL_DIMENSION),
	d_multi_resolution_cube_mesh(multi_resolution_cube_mesh)
{
	create_polygon_stencil_texture(renderer);

	create_polygons_vertex_array(renderer);

	create_polygon_stencil_quads_vertex_array(renderer);

	// If there's support for shader programs then create them.
	create_shader_programs(renderer);
}


unsigned int
GPlatesOpenGL::GLMultiResolutionFilledPolygons::get_level_of_detail(
		const GLViewport &viewport,
		const GLMatrix &model_view_transform,
		const GLMatrix &projection_transform) const
{
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
	const float max_lowest_resolution_texel_size_on_unit_sphere = 2.0 / d_tile_texel_dimension;

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

// 	g_num_tiles_rendered = 0;
// 	g_num_render_target_switches = 0;
// 	g_num_polygons_rendered = 0;

	// First write the vertices/indices of the filled polygons gathered by the client into
	// our vertex buffer and vertex element buffer.
	write_filled_polygon_meshes_to_vertex_array(renderer, filled_polygons);

	// Get the level-of-detail based on the size of viewport pixels projected onto the globe.
	const unsigned int render_level_of_detail = get_level_of_detail(
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
											d_tile_texel_dimension,
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

		// Get the quad tree root node of the current cube face of the source raster.
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
				mesh_quad_tree_root_node,
				*filled_polygons.d_filled_polygons_spatial_partition,
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
				mesh_quad_tree_node,
				filled_polygons_spatial_partition,
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
					child_mesh_quad_tree_node,
					filled_polygons_spatial_partition,
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
		const mesh_quad_tree_node_type &mesh_quad_tree_node,
		const filled_polygons_spatial_partition_type &filled_polygons_spatial_partition,
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
			mesh_quad_tree_node,
			filled_polygons_spatial_partition,
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
		if (GLContext::get_parameters().texture.gl_max_texture_units >= 2)
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
	if (d_render_tile_to_scene_program_object && d_render_tile_to_scene_with_clipping_program_object)
	{
		if (clip_to_tile_frustum)
		{
			// Bind the shader program with clipping.
			renderer.gl_bind_program_object(d_render_tile_to_scene_with_clipping_program_object.get());

			// Set the tile texture sampler to texture unit 0.
			d_render_tile_to_scene_with_clipping_program_object.get()->gl_uniform1i(
					renderer, "tile_texture_sampler", 0/*texture unit*/);

			// Set the clip texture sampler to texture unit 1.
			d_render_tile_to_scene_with_clipping_program_object.get()->gl_uniform1i(
					renderer, "clip_texture_sampler", 1/*texture unit*/);
		}
		else
		{
			// Bind the shader program.
			renderer.gl_bind_program_object(d_render_tile_to_scene_program_object.get());

			// Set the tile texture sampler to texture unit 0.
			d_render_tile_to_scene_program_object.get()->gl_uniform1i(
					renderer, "tile_texture_sampler", 0/*texture unit*/);
		}
	}
	else // Fixed function...
	{
		// Enable texturing and set the texture function on texture unit 0.
		renderer.gl_enable_texture(GL_TEXTURE0, GL_TEXTURE_2D);
		renderer.gl_tex_env(GL_TEXTURE0, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		// Set up texture coordinate generation from the vertices (x,y,z) on texture unit 0.
		GLUtils::set_object_linear_tex_gen_state(renderer, 0/*texture_unit*/);

		// NOTE: If two texture units are not supported then just don't clip to the tile.
		// The 'is_supported()' method should have been called to prevent us from getting here though.
		if (GLContext::get_parameters().texture.gl_max_texture_units >= 2)
		{
			// Enable texturing and set the texture function on texture unit 1.
			renderer.gl_enable_texture(GL_TEXTURE1, GL_TEXTURE_2D);
			renderer.gl_tex_env(GL_TEXTURE1, GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			// Set up texture coordinate generation from the vertices (x,y,z) on texture unit 1.
			GLUtils::set_object_linear_tex_gen_state(renderer, 1/*texture_unit*/);
		}
	}

	// NOTE: We don't set alpha-blending (or alpha-testing) state here because we
	// might not be rendering directly to the final render target and hence we don't
	// want to double-blend semi-transparent rasters - the alpha value is multiplied by
	// all channels including alpha during alpha blending (R,G,B,A) -> (A*R,A*G,A*B,A*A) -
	// the final render target would then have a source blending contribution of (3A*R,3A*G,3A*B,4A)
	// which is not what we want - we want (A*R,A*G,A*B,A*A).

#if 0
	// Used to render as wire-frame meshes instead of filled textured meshes for
	// visualising mesh density.
	renderer.gl_polygon_mode(GL_FRONT_AND_BACK, GL_LINE);
#endif
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::render_tile_to_scene(
		GLRenderer &renderer,
		const mesh_quad_tree_node_type &mesh_quad_tree_node,
		const filled_polygons_spatial_partition_type &filled_polygons_spatial_partition,
		const filled_polygons_spatial_partition_node_list_type &filled_polygons_intersecting_node_list,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
		clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
		const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node)
{
	PROFILE_FUNC();

	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

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
	const GLTexture::shared_ptr_to_const_type tile_texture = allocate_tile_texture(renderer);

	// Render the filled polygons to the tile texture.
	render_filled_polygons_to_tile_texture(
			renderer,
			tile_texture,
			transformed_sorted_filled_drawables,
			*projection_transform,
			*view_transform);

	// Get the mesh covering the current quad tree node tile.
	GLCompiledDrawState::non_null_ptr_to_const_type mesh_drawable = mesh_quad_tree_node.get_mesh_drawable();

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

	renderer.apply_compiled_draw_state(*mesh_drawable);
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::render_filled_polygons_to_tile_texture(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_to_const_type &tile_texture,
		const filled_polygon_seq_type &transformed_sorted_filled_drawables,
		const GLTransform &projection_transform,
		const GLTransform &view_transform)
{
	//PROFILE_FUNC();

	//++g_num_tiles_rendered;

	// Begin a render target that will render the individual filled polygons to the tile texture.
	GLRenderer::RenderTarget2DScope render_target_scope(renderer, tile_texture);

	// The viewport for the tile texture.
	renderer.gl_viewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension);

	// Set the alpha-blend state since filled polygon could have a transparent colour.
	renderer.gl_enable(GL_BLEND);
	renderer.gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Set the alpha-test state to reject pixels where alpha is zero (they make no
	// change or contribution to the render target) - this is an optimisation.
	renderer.gl_enable(GL_ALPHA_TEST);
	renderer.gl_alpha_func(GL_GREATER, GLclampf(0));

	// Set up texture state to use the polygon stencil texture to render to the tile texture.
	GLUtils::set_full_screen_quad_texture_state(
			renderer,
			d_polygon_stencil_texture,
			0/*texture_unit*/,
			GL_REPLACE);

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

	// Bind the vertex array used to copy the polygon stencil texture into the tile texture.
	// We only need to bind it once - note that 'render_filled_polygons_to_polygon_stencil_texture'
	// has its own render target and hence its own state so it doesn't interfere with our state here
	// (ie, this binding will get rebound as needed when the nested render target block goes out of scope).
	d_polygon_stencil_quads_vertex_array->gl_bind(renderer);

	// We clear this tile's render texture just before we render the polygon stencil texture to it.
	// This reduces the number of render target switches by one since no drawables are added
	// to the tile's render target until after switching back from the polygon stencil render target.
	bool cleared_tile_render_target = false;

	// Get the maximum render target dimensions in case the main framebuffer is used as a render-target.
	// Ie, if we're limited to the current dimensions of the main framebuffer (the current window).
	unsigned int render_target_width;
	unsigned int render_target_height;
	renderer.get_max_render_target_dimensions(render_target_width, render_target_height);
	if (render_target_width < d_tile_texel_dimension)
	{
		render_target_width = d_tile_texel_dimension;
	}
	if (render_target_height < d_tile_texel_dimension)
	{
		render_target_height = d_tile_texel_dimension;
	}
	if (render_target_width > d_polygon_stencil_texture->get_width().get())
	{
		render_target_width = d_polygon_stencil_texture->get_width().get();
	}
	if (render_target_height > d_polygon_stencil_texture->get_height().get())
	{
		render_target_height = d_polygon_stencil_texture->get_height().get();
	}

	// If framebuffer objects are supported then naturally our render target dimensions will
	// match the polygon stencil texture dimensions (that's how FBO's work), but if we're
	// falling back to the main framebuffer as a render-target. In this case our polygon stencil
	// quads vertex array can't be used fully (because it's populated assuming the render target
	// dimension is the polygon stencil texture dimension). We can however use the first row
	// of quads without problem so we'll make the render target height one tile in size.
	if (render_target_width != d_polygon_stencil_texture->get_width().get())
	{
		render_target_height = d_tile_texel_dimension;
	}

	const unsigned int num_polygons_per_stencil_texture_render =
			render_target_width * render_target_height /
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
				render_target_width,
				render_target_height,
				filled_drawables_iter,
				filled_drawables_group_end,
				projection_transform,
				view_transform);

		// We delay clearing of the tile render target until after the first rendering to the
		// polygon stencil texture - this is an optimisation only in case the main framebuffer is
		// being for render targets.
		if (!cleared_tile_render_target)
		{
			// Clear the colour buffer of the render target.
			renderer.gl_clear_color(); // Clear colour to all zeros.
			renderer.gl_clear(GL_COLOR_BUFFER_BIT); // Clear only the colour buffer.

			cleared_tile_render_target = true;
		}

		//PROFILE_BLOCK("d_polygon_stencil_quads_vertex_array->gl_draw_range_elements");

		// Render the filled polygons, in the stencil texture, to the current tile render target.
		//
		// Draw as many quads as there were polygons rendered into the larger polygon stencil texture.
		const unsigned int num_quad_vertices = 4 * num_polygons_in_group;
		d_polygon_stencil_quads_vertex_array->gl_draw_range_elements(
				renderer,
				GL_QUADS,
				0/*start*/,
				num_quad_vertices - 1/*end*/,
				num_quad_vertices/*count*/,
				GLVertexElementTraits<stencil_quad_vertex_element_type>::type,
				0/*indices_offset*/);

		// Advance to the next group of polygons.
		filled_drawables_iter = filled_drawables_group_end;
		num_polygons_left_to_render -= num_polygons_in_group;
	}
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::render_filled_polygons_to_polygon_stencil_texture(
		GLRenderer &renderer,
		unsigned int render_target_width,
		unsigned int render_target_height,
		const filled_polygon_seq_type::const_iterator begin_filled_drawables,
		const filled_polygon_seq_type::const_iterator end_filled_drawables,
		const GLTransform &projection_transform,
		const GLTransform &view_transform)
{
	//PROFILE_FUNC();

	// Begin a render target that will render the individual filled polygons to the tile texture.
	// This is also an implicit state block (saves/restores state).
	GLRenderer::RenderTarget2DScope render_target_scope(
			renderer,
			d_polygon_stencil_texture,
			// Limit rendering to a part of the polygon stencil texture if it's too big for render-target...
			GLViewport(0, 0, render_target_width, render_target_height));

	//++g_num_render_target_switches;

	// Clear the entire colour buffer of the render target.
	// Clears the entire render target regardless of the current viewport.
	renderer.gl_clear_color(); // All zeros.
	// Clear only the colour buffer.
	renderer.gl_clear(GL_COLOR_BUFFER_BIT);

	// Alpha-blend state set to invert destination alpha (and colour) every time a pixel
	// is rendered (this means we get 1 where a pixel is covered by an odd number of triangles
	// and 0 by an even number of triangles).
	renderer.gl_enable(GL_BLEND);
	renderer.gl_blend_func(GL_ONE_MINUS_DST_ALPHA, GL_ZERO);

#if 0
	// Used to render as wire-frame meshes instead of filled textured meshes for
	// visualising mesh density.
	renderer.gl_polygon_mode(GL_FRONT_AND_BACK, GL_LINE);

	// Set the anti-aliased line state.
	renderer.gl_line_width(4.0f);
	//renderer.gl_enable(GL_LINE_SMOOTH);
	//renderer.gl_hint(GL_LINE_SMOOTH_HINT, GL_NICEST);
#endif

	renderer.gl_mult_matrix(GL_MODELVIEW, view_transform.get_matrix());

	// NOTE: We use the half-texel-expanded projection transform since we want to render the
	// border pixels (in each tile) exactly on the tile (plane) boundary.
	// The tile textures are bilinearly filtered and this way the centres of border texels match up
	// with adjacent tiles.
	renderer.gl_mult_matrix(GL_PROJECTION, projection_transform.get_matrix());

	const GLMatrix view_matrix = renderer.gl_get_matrix(GL_MODELVIEW);

	// Bind the vertex array before using it to draw.
	d_polygons_vertex_array->gl_bind(renderer);

	// Start off with the identity model transform and change as needed.
	boost::optional<GPlatesAppLogic::ReconstructMethodFiniteRotation::non_null_ptr_to_const_type> current_finite_rotation;

	const unsigned int num_viewports_along_stencil_texture_width =
			render_target_width / d_tile_texel_dimension;

	// Render each filled polygon to a separate viewport within the polygon stencil texture.
	unsigned int viewport_x_offset = 0;
	unsigned int viewport_y_offset = 0;
	for (filled_polygon_seq_type::const_iterator filled_drawables_iter = begin_filled_drawables;
		filled_drawables_iter != end_filled_drawables;
		++filled_drawables_iter)
	{
		const filled_polygon_type &filled_drawable = *filled_drawables_iter;

		// The viewport subsection of the render target for the current filled polygon.
		renderer.gl_viewport(
				viewport_x_offset * d_tile_texel_dimension,
				viewport_y_offset * d_tile_texel_dimension,
				d_tile_texel_dimension,
				d_tile_texel_dimension);

		// If the finite rotation has changed then set update it in the renderer...
		if (filled_drawable.d_transform != current_finite_rotation)
		{
			renderer.gl_load_matrix(GL_MODELVIEW, view_matrix);

			if (filled_drawable.d_transform)
			{
				// Convert the finite rotation from a unit quaternion to a matrix so we can feed it to OpenGL.
				const GPlatesMaths::UnitQuaternion3D &quat_rotation =
						filled_drawable.d_transform.get()->get_finite_rotation().unit_quat();

				// Multiply in the model transform.
				renderer.gl_mult_matrix(GL_MODELVIEW, GLMatrix(quat_rotation));
			}

			current_finite_rotation = filled_drawable.d_transform;
		}

		//PROFILE_BLOCK("d_polygons_vertex_array->gl_draw_range_elements");

		// Render the current filled polygon.
		d_polygons_vertex_array->gl_draw_range_elements(
				renderer,
				GL_TRIANGLES,
				filled_drawable.d_drawable.start,
				filled_drawable.d_drawable.end,
				filled_drawable.d_drawable.count,
				GLVertexElementTraits<polygon_vertex_element_type>::type,
				filled_drawable.d_drawable.indices_offset);

		//++g_num_polygons_rendered;

		// Move to the next row of viewport subsections if we have to.
		if (++viewport_x_offset == num_viewports_along_stencil_texture_width)
		{
			viewport_x_offset = 0;
			++viewport_y_offset;
		}
	}

	//++g_num_render_target_switches;
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::get_filled_polygons(
		filled_polygon_seq_type &transform_sorted_filled_drawables,
		filled_polygons_spatial_partition_type::element_const_iterator begin_root_filled_polygons,
		filled_polygons_spatial_partition_type::element_const_iterator end_root_filled_polygons,
		const filled_polygons_spatial_partition_node_list_type &filled_polygons_intersecting_node_list)
{
	//PROFILE_FUNC();

	// Add the reconstructed polygon meshes in the root of the spatial partition.
	// These are the meshes that were too large to insert in any face of the cube quad tree partition.
	// Add the reconstructed polygon meshes of the current node.
	transform_sorted_filled_drawables.insert(
			transform_sorted_filled_drawables.end(),
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
		transform_sorted_filled_drawables.insert(
				transform_sorted_filled_drawables.end(),
				node_reference.begin(),
				node_reference.end());
	}

	// Sort the sequence of filled drawables by transform.
	std::sort(
			transform_sorted_filled_drawables.begin(),
			transform_sorted_filled_drawables.end(),
			SortFilledDrawables());
}


GPlatesOpenGL::GLTexture::shared_ptr_to_const_type
GPlatesOpenGL::GLMultiResolutionFilledPolygons::allocate_tile_texture(
		GLRenderer &renderer)
{
	// Get an unused tile texture from the cache if there is one.
	boost::optional<GLTexture::shared_ptr_type> tile_texture = d_texture_cache->allocate_object();
	if (!tile_texture)
	{
		// No unused texture so create a new one...
		tile_texture = d_texture_cache->allocate_object(GLTexture::create_as_auto_ptr(renderer));

		create_tile_texture(renderer, tile_texture.get());
	}

	return tile_texture.get();
}

DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::create_tile_texture(
		GLRenderer &renderer,
		const GLTexture::shared_ptr_type &texture)
{
	PROFILE_FUNC();

	//
	// No mipmaps needed so we specify no mipmap filtering.
	// We're not using mipmaps because our cube mapping does not have much distortion
	// unlike global rectangular lat/lon rasters that squash near the poles.
	//
	// We do enable bilinear filtering (also note that the texture is a fixed-point format).
	texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Specify anisotropic filtering if it's supported since we are not using mipmaps
	// and any textures rendered near the edge of the globe will get squashed a bit due to
	// the angle we are looking at them and anisotropic filtering will help here.
	if (GLEW_EXT_texture_filter_anisotropic)
	{
		const GLfloat anisotropy = GLContext::get_parameters().texture.gl_texture_max_anisotropy;
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	}

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	if (GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp)
	{
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	// Create the texture but don't load any data into it.
	// Leave it uninitialised because we will be rendering into it to initialise it.
	texture->gl_tex_image_2D(renderer, GL_TEXTURE_2D, 0, GL_RGBA8,
			d_tile_texel_dimension, d_tile_texel_dimension,
			0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::create_polygon_stencil_texture(
		GLRenderer &renderer)
{
	//
	// The texture dimensions of the single polygon stencil rendering texture.
	//
	// This is ideally much larger than the cube quad tree node tile textures to
	// minimise render target switching.
	//
	// We probably don't need too large a texture - just want to fit a reasonable number of
	// 256x256 tile textures inside it to minimise render target switching.
	// Each filled polygon gets its own 256x256 section so 4096x4096 is 256 polygons per render target.
	//
	unsigned int polygon_stencil_texel_width = 4096;
	unsigned int polygon_stencil_texel_height = 4096;

	// Our polygon stencil texture should be big enough to cover a regular tile.
	if (polygon_stencil_texel_width < d_tile_texel_dimension)
	{
		polygon_stencil_texel_width = d_tile_texel_dimension;
	}
	if (polygon_stencil_texel_height < d_tile_texel_dimension)
	{
		polygon_stencil_texel_height = d_tile_texel_dimension;
	}
	// But it can't be larger than the maximum texture dimension for the current system.
	if (polygon_stencil_texel_width > GLContext::get_parameters().texture.gl_max_texture_size)
	{
		polygon_stencil_texel_width = GLContext::get_parameters().texture.gl_max_texture_size;
	}
	if (polygon_stencil_texel_height > GLContext::get_parameters().texture.gl_max_texture_size)
	{
		polygon_stencil_texel_height = GLContext::get_parameters().texture.gl_max_texture_size;
	}
	// And it can't be larger than the maximum viewport dimensions for the current system.
	if (polygon_stencil_texel_width > GLContext::get_parameters().viewport.gl_max_viewport_width)
	{
		polygon_stencil_texel_width = GLContext::get_parameters().viewport.gl_max_viewport_width;
	}
	if (polygon_stencil_texel_height > GLContext::get_parameters().viewport.gl_max_viewport_height)
	{
		polygon_stencil_texel_height = GLContext::get_parameters().viewport.gl_max_viewport_height;
	}

	d_polygon_stencil_texture = GLTexture::create(renderer);

	//
	// No mipmaps needed so we specify no mipmap filtering.
	// We're not using mipmaps because we simply render with one-to-one texel-to-pixel
	// mapping (using a full screen quad in a render target).
	//
	d_polygon_stencil_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	d_polygon_stencil_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	if (GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp)
	{
		d_polygon_stencil_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		d_polygon_stencil_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		d_polygon_stencil_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		d_polygon_stencil_texture->gl_tex_parameteri(renderer, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	// Create the texture but don't load any data into it.
	// Leave it uninitialised because we will be rendering into it to initialise it.
	d_polygon_stencil_texture->gl_tex_image_2D(renderer, GL_TEXTURE_2D, 0, GL_RGBA8,
			polygon_stencil_texel_width, polygon_stencil_texel_height,
			0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
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
	// Attach vertex buffer to the vertex array.
	bind_vertex_buffer_to_vertex_array<polygon_vertex_type>(renderer, *d_polygons_vertex_array, d_polygons_vertex_buffer);
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::write_filled_polygon_meshes_to_vertex_array(
		GLRenderer &renderer,
		const filled_polygons_type &filled_polygons)
{
	GLBuffer::shared_ptr_type vertex_element_buffer_data = d_polygons_vertex_element_buffer->get_buffer();
	vertex_element_buffer_data->gl_buffer_data(
			renderer,
			GLBuffer::TARGET_ELEMENT_ARRAY_BUFFER,
			filled_polygons.d_polygon_vertex_elements,
			// It's not 'stream' because filled polygons can be accessed more than once per 'render' call...
			GLBuffer::USAGE_DYNAMIC_DRAW);

	GLBuffer::shared_ptr_type vertex_buffer_data = d_polygons_vertex_buffer->get_buffer();
	vertex_buffer_data->gl_buffer_data(
			renderer,
			GLBuffer::TARGET_ARRAY_BUFFER,
			filled_polygons.d_polygon_vertices,
			// It's not 'stream' because filled polygons can be accessed more than once per 'render' call...
			GLBuffer::USAGE_DYNAMIC_DRAW);
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::create_polygon_stencil_quads_vertex_array(
		GLRenderer &renderer)
{
	const unsigned int num_quads_along_polygon_stencil_width =
			d_polygon_stencil_texture->get_width().get() / d_tile_texel_dimension;
	const unsigned int num_quads_along_polygon_stencil_height =
			d_polygon_stencil_texture->get_height().get() / d_tile_texel_dimension;

	const double scale_u = 1.0 / num_quads_along_polygon_stencil_width;
	const double scale_v = 1.0 / num_quads_along_polygon_stencil_height;

	const unsigned int num_quad_vertices =
			4 * num_quads_along_polygon_stencil_width * num_quads_along_polygon_stencil_height;

	// The vertices for the quads.
	std::vector<stencil_quad_vertex_type> quad_vertices;
	quad_vertices.reserve(num_quad_vertices);

	// We're using 'GLushort' vertex indices which are 16-bit - make sure we don't overflow them.
	// 16-bit indices are faster than 32-bit for graphics cards (but again probably not much gain).
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			num_quad_vertices <= (1 << 16),
			GPLATES_ASSERTION_SOURCE);
	std::vector<stencil_quad_vertex_element_type> quad_indices;
	quad_indices.reserve(num_quad_vertices);

	for (unsigned int y = 0; y < num_quads_along_polygon_stencil_width; ++y)
	{
		for (unsigned int x = 0; x < num_quads_along_polygon_stencil_height; ++x)
		{
			// Add four vertices for the current quad.
			const double u0 = x * scale_u;
			const double v0 = y * scale_v;
			const double u1 = u0 + scale_u;
			const double v1 = v0 + scale_v;

			const stencil_quad_vertex_element_type quad_base_vertex_index = quad_vertices.size();

			//
			//  x,  y, z, u, v
			//
			// Note that the (x,y,z) positions of each quad are the same since they overlap
			// when rendering (blending) into a tile's render texture.
			quad_vertices.push_back(stencil_quad_vertex_type(-1, -1, 0, u0, v0));
			quad_vertices.push_back(stencil_quad_vertex_type(1, -1, 0, u1, v0));
			quad_vertices.push_back(stencil_quad_vertex_type(1,  1, 0, u1, v1));
			quad_vertices.push_back(stencil_quad_vertex_type(-1,  1, 0, u0, v1));

			quad_indices.push_back(quad_base_vertex_index);
			quad_indices.push_back(quad_base_vertex_index + 1);
			quad_indices.push_back(quad_base_vertex_index + 2);
			quad_indices.push_back(quad_base_vertex_index + 3);
		}
	}

	// Create a single OpenGL vertex array to contain the vertices of all 256x256 polygon stencil
	// quads that fit inside the polygon stencil texture.
	d_polygon_stencil_quads_vertex_array = GLVertexArray::create(renderer);
	// Store the vertices/indices in a new vertex buffer and vertex element buffer that is then
	// bound to the vertex array.
	set_vertex_array_data(renderer, *d_polygon_stencil_quads_vertex_array, quad_vertices, quad_indices);
}


void
GPlatesOpenGL::GLMultiResolutionFilledPolygons::create_shader_programs(
		GLRenderer &renderer)
{
	// We only make use of shader programs for the final stage of rendering a tile to the scene.

	// A version without clipping.
	d_render_tile_to_scene_program_object =
			GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
					renderer,
					RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE,
					RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE);

	// A version with clipping.
	GLShaderProgramUtils::ShaderSource render_tile_to_scene_with_clipping_shader_source;
	// Add the '#define' first.
	render_tile_to_scene_with_clipping_shader_source.add_shader_source("#define ENABLE_CLIPPING\n");
	// Then add the GLSL 'main()' function.
	render_tile_to_scene_with_clipping_shader_source.add_shader_source(RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE);
	// Create the program object.
	d_render_tile_to_scene_with_clipping_program_object =
			GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
					renderer,
					RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE,
					render_tile_to_scene_with_clipping_shader_source);
}
