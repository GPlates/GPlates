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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include <algorithm>
#include <cmath>
#include <limits>
#include <boost/cast.hpp>
#include <QDebug>

#include "GLFilledPolygonsGlobeView.h"

#include "GL.h"
#include "GLContext.h"
#include "GLIntersect.h"
#include "GLMatrix.h"
#include "GLShader.h"
#include "GLShaderSource.h"
#include "GLUtils.h"
#include "GLVertexUtils.h"
#include "GLViewProjection.h"
#include "OpenGLException.h"

#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/Colour.h"

#include "utils/CallStackTracker.h"
#include "utils/IntrusiveSinglyLinkedList.h"
#include "utils/Profile.h"



namespace GPlatesOpenGL
{
	namespace
	{
		//! The inverse of log(2.0).
		const float INVERSE_LOG2 = 1.0 / std::log(2.0);

		/**
		 * Vertex shader source for rendering *to* the tile texture.
		 */
		const char *RENDER_TO_TILE_VERTEX_SHADER_SOURCE =
			R"(
				uniform mat4 view_projection;
			
				layout(location = 0) in vec4 position;
				layout(location = 1) in vec4 colour;

				out vec4 fill_colour;
			
				void main (void)
				{
					gl_Position = view_projection * position;
					fill_colour = colour;
				}
			)";

		/**
		 * Fragment shader source for rendering *to* the tile texture.
		 */
		const char *RENDER_TO_TILE_FRAGMENT_SHADER_SOURCE =
			R"(
				in vec4 fill_colour;
			
				layout(location = 0) out vec4 colour;

				void main (void)
				{
					colour = fill_colour;
				}
			)";

		/**
		 * Vertex shader source for rendering the tile texture to the scene.
		 */
		const char *RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE =
			R"(
				uniform mat4 scene_tile_texture_matrix;
				uniform mat4 clip_texture_matrix;
				uniform bool clip_to_tile_frustum;
				uniform bool lighting_enabled;

				layout(location = 0) in vec4 position;

				out vec4 scene_tile_texture_coord;
				out vec4 clip_texture_coord;
				out vec3 world_space_position;  // world-space coordinates interpolated across geometry
			
				void main (void)
				{
					gl_Position = position;

					// Transform present-day position by cube map projection and
					// any texture coordinate adjustments before accessing textures.
					scene_tile_texture_coord = scene_tile_texture_matrix * position;
					if (clip_to_tile_frustum)
					{
						clip_texture_coord = clip_texture_matrix * position;
					}

					if (lighting_enabled)
					{
						// This assumes the geometry does not need a model transform (eg, reconstruction rotation).
						world_space_position = position.xyz / position.w;
					}
				}
			)";

		/**
		 * Fragment shader source for rendering the tile texture to the scene.
		 */
		const char *RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE =
			R"(
				
				uniform sampler2D tile_texture_sampler;
				uniform sampler2D clip_texture_sampler;
				uniform bool clip_to_tile_frustum;
				uniform bool lighting_enabled;
				uniform float light_ambient_contribution;
				uniform vec3 world_space_light_direction;

				in vec4 scene_tile_texture_coord;
				in vec4 clip_texture_coord;
				in vec3 world_space_position;  // world-space coordinates interpolated across geometry
			
				layout(location = 0) out vec4 colour;
				
				void main (void)
				{
					// Projective texturing to handle cube map projection.
					// Tile texture has premultiplied alpha.
					colour = textureProj(tile_texture_sampler, scene_tile_texture_coord);
				
					if (clip_to_tile_frustum)
					{
						colour *= textureProj(clip_texture_sampler, clip_texture_coord);
					}
				
					// As a small optimisation discard the pixel if the alpha is zero.
					if (colour.a == 0)
					{
						discard;
					}
				
					if (lighting_enabled)
					{
						// Apply the Lambert diffuse lighting using the world-space position as the globe surface normal.
						// Note that neither the light direction nor the surface normal need be normalised.
						float lambert = lambert_diffuse_lighting(world_space_light_direction, world_space_position);
				
						// Blend between ambient and diffuse lighting.
						float lighting = mix_ambient_with_diffuse_lighting(lambert, light_ambient_contribution);
				
						colour.rgb *= lighting;
					}
				}
			)";
	}
}


GPlatesOpenGL::GLFilledPolygonsGlobeView::GLFilledPolygonsGlobeView(
		GL &gl,
		const GLMultiResolutionCubeMesh::non_null_ptr_to_const_type &multi_resolution_cube_mesh,
		boost::optional<GLLight::non_null_ptr_type> light) :
	d_multi_resolution_cube_mesh(multi_resolution_cube_mesh),
	d_light(light),
	d_drawables_vertex_array(GLVertexArray::create(gl)),
	d_drawables_vertex_buffer(GLBuffer::create(gl)),
	d_drawables_vertex_element_buffer(GLBuffer::create(gl)),
	d_tile_texel_dimension(
			// Make sure tile dimensions does not exceed maximum texture dimensions...
			(boost::numeric_cast<GLuint>(TILE_MAX_VIEWPORT_DIMENSION) > gl.get_capabilities().gl_max_texture_size)
					? gl.get_capabilities().gl_max_texture_size
					: TILE_MAX_VIEWPORT_DIMENSION),
	d_tile_texture(GLTexture::create(gl)),
	d_tile_stencil_buffer(GLRenderbuffer::create(gl)),
	d_tile_texture_framebuffer(GLFramebuffer::create(gl)),
	d_render_to_tile_program(GLProgram::create(gl)),
	d_render_tile_to_scene_program(GLProgram::create(gl))
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	create_tile_texture(gl);
	create_tile_stencil_buffer(gl);
	create_tile_texture_framebuffer(gl);  // Note: should be called after tile texture and stencil buffer created

	create_drawables_vertex_array(gl);
	compile_link_shader_programs(gl);
}


unsigned int
GPlatesOpenGL::GLFilledPolygonsGlobeView::get_level_of_detail(
		unsigned int &tile_viewport_dimension,
		const GLViewProjection &view_projection) const
{
	// Start with the highest tile viewport dimension - we will reduce it if we can.
	tile_viewport_dimension = d_tile_texel_dimension;

	// Get the minimum size of a pixel in the current viewport when projected
	// onto the unit sphere (in model space).
	const double min_pixel_size_on_unit_sphere = view_projection.get_min_max_pixel_size_on_globe().first/*min*/;

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
	// texel size will always be less than at the face centre so at least it's bounded and the
	// variation across the cube face is not that large so we shouldn't be using a level-of-detail
	// that is much higher than what we need.
	const float max_lowest_resolution_texel_size_on_unit_sphere = 2.0 / tile_viewport_dimension;

	float level_of_detail_factor = INVERSE_LOG2 *
			(std::log(max_lowest_resolution_texel_size_on_unit_sphere) -
					std::log(min_pixel_size_on_unit_sphere));

	// Reduce the tile texel dimension (by factors of two) if we don't need the extra resolution.
	while (level_of_detail_factor < -1 && tile_viewport_dimension > TILE_MIN_VIEWPORT_DIMENSION)
	{
		level_of_detail_factor += 1;
		tile_viewport_dimension >>= 1;
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
GPlatesOpenGL::GLFilledPolygonsGlobeView::render(
		GL &gl,
		const GLViewProjection &view_projection,
		const filled_drawables_type &filled_drawables)
{
	PROFILE_FUNC();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// If there are no filled drawables to render then return early.
	if (filled_drawables.d_drawable_vertex_elements.empty())
	{
		return;
	}

	// Write the vertices/indices of all filled drawables (gathered by the client) into our
	// vertex buffer and vertex element buffer.
	write_filled_drawables_to_vertex_array(gl, filled_drawables);

	// Get the level-of-detail based on the size of viewport pixels projected onto the globe.
	unsigned int tile_viewport_dimension;
	const unsigned int render_level_of_detail = get_level_of_detail(tile_viewport_dimension, view_projection);

	// Get the view frustum planes.
	const GLFrustum frustum_planes(view_projection.get_view_projection_transform());

	// Create a subdivision cube quad tree traversal.
	// No caching is required since we're only visiting each subdivision node once.
	//
	// Cube subdivision cache for half-texel-expanded projection transforms since that is what's used to
	// lookup the tile textures (the tile textures are bilinearly filtered and the centres of
	// border texels match up with adjacent tiles).
	cube_subdivision_cache_type::non_null_ptr_type
			cube_subdivision_cache =
					cube_subdivision_cache_type::create(
							GLCubeSubdivision::create(
									GLCubeSubdivision::get_expand_frustum_ratio(
											tile_viewport_dimension,
											0.5/* half a texel */)));
	// Cube subdivision cache for the clip texture (no frustum expansion here).
	clip_cube_subdivision_cache_type::non_null_ptr_type
			clip_cube_subdivision_cache =
					clip_cube_subdivision_cache_type::create(
							GLCubeSubdivision::create());

	//
	// Traverse the source raster cube quad tree and the spatial partition of filled drawables.
	//

	// Traverse the quad trees of the cube faces.
	for (unsigned int face = 0; face < 6; ++face)
	{
		const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
				static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

		// Get the quad tree root node of the current cube face of the source mesh.
		const mesh_quad_tree_node_type mesh_quad_tree_root_node =
				d_multi_resolution_cube_mesh->get_quad_tree_root_node(cube_face);

		// This is used to find those nodes of the filled drawables spatial partition
		// that intersect the source raster cube quad tree.
		// This is so we know which filled drawables to draw for each source raster tile.
		filled_drawables_intersecting_nodes_type
				filled_drawable_intersecting_nodes(
						*filled_drawables.d_filled_drawables_spatial_partition,
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
		filled_drawables_spatial_partition_node_list_type filled_drawables_spatial_partition_node_list;

		render_quad_tree(
				gl,
				tile_viewport_dimension,
				mesh_quad_tree_root_node,
				filled_drawables,
				filled_drawables_spatial_partition_node_list,
				filled_drawable_intersecting_nodes,
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
GPlatesOpenGL::GLFilledPolygonsGlobeView::render_quad_tree(
		GL &gl,
		unsigned int tile_viewport_dimension,
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

	// If either we're at the correct level of detail for rendering then draw the filled drawables.
	if (level_of_detail == render_level_of_detail)
	{
		// Continue to recurse into the filled drawables spatial partition to continue to find
		// those drawables that intersect the current quad tree node.
		render_quad_tree_node(
				gl,
				tile_viewport_dimension,
				mesh_quad_tree_node,
				filled_drawables,
				parent_filled_drawables_intersecting_node_list,
				filled_drawables_intersecting_nodes,
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

			// Used to determine which filled drawables intersect the child quad tree node.
			filled_drawables_intersecting_nodes_type
					child_filled_drawables_intersecting_nodes(
							filled_drawables_intersecting_nodes,
							child_u_offset,
							child_v_offset);

			// Construct linked list nodes on the runtime stack as it simplifies memory management.
			// When the stack unwinds, the list(s) referencing these nodes, as well as the nodes themselves,
			// will disappear together (leaving any lists higher up in the stack still intact) - this happens
			// because this list implementation supports tail-sharing.
			FilledDrawablesListNode child_filled_drawables_list_nodes[
					filled_drawables_intersecting_nodes_type::parent_intersecting_nodes_type::MAX_NUM_NODES];

			// A tail-shared list to contain the filled drawable nodes that intersect the
			// current node. The parent list contains the nodes we've been
			// accumulating so far during our quad tree traversal.
			filled_drawables_spatial_partition_node_list_type
					child_filled_drawables_intersecting_node_list(
							parent_filled_drawables_intersecting_node_list);

			// Add any new intersecting nodes from the filled drawables spatial partition.
			// These new nodes are the nodes that intersect the tile at the current quad tree depth.
			const filled_drawables_intersecting_nodes_type::parent_intersecting_nodes_type &
					parent_intersecting_nodes =
							child_filled_drawables_intersecting_nodes.get_parent_intersecting_nodes();

			// Now add those neighbours nodes that exist (not all areas of the spatial partition will be
			// populated with filled drawables).
			const unsigned int num_parent_nodes = parent_intersecting_nodes.get_num_nodes();
			for (unsigned int parent_node_index = 0; parent_node_index < num_parent_nodes; ++parent_node_index)
			{
				const filled_drawables_spatial_partition_type::const_node_reference_type &
						intersecting_parent_node_reference = parent_intersecting_nodes.get_node(parent_node_index);
				// Only need to add nodes that actually contain filled drawables.
				// NOTE: We still recurse into child nodes though - an empty internal node does not
				// mean the child nodes are necessarily empty.
				if (!intersecting_parent_node_reference.empty())
				{
					child_filled_drawables_list_nodes[parent_node_index].node_reference =
							intersecting_parent_node_reference;

					// Add to the list of filled drawable spatial partition nodes that
					// intersect the current tile.
					child_filled_drawables_intersecting_node_list.push_front(
							&child_filled_drawables_list_nodes[parent_node_index]);
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
					gl,
					tile_viewport_dimension,
					child_mesh_quad_tree_node,
					filled_drawables,
					child_filled_drawables_intersecting_node_list,
					child_filled_drawables_intersecting_nodes,
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
GPlatesOpenGL::GLFilledPolygonsGlobeView::render_quad_tree_node(
		GL &gl,
		unsigned int tile_viewport_dimension,
		const mesh_quad_tree_node_type &mesh_quad_tree_node,
		const filled_drawables_type &filled_drawables,
		const filled_drawables_spatial_partition_node_list_type &parent_filled_drawables_intersecting_node_list,
		const filled_drawables_intersecting_nodes_type &filled_drawables_intersecting_nodes,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
		clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
		const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node)
{
	// From here on we can't allocate the list nodes on the runtime stack because we need to access
	// the list after we return from traversing the spatial partition. So use an object pool instead.
	boost::object_pool<FilledDrawablesListNode> filled_drawables_list_node_pool;

	// A tail-shared list to contain the reconstructed drawable meshes nodes that intersect the
	// current source raster node. The parent list contains the nodes we've been
	// accumulating so far during our quad tree traversal.
	filled_drawables_spatial_partition_node_list_type
			filled_drawables_intersecting_node_list(
					parent_filled_drawables_intersecting_node_list);

	// Add any new intersecting nodes from the filled drawables spatial partition.
	// These new nodes are the nodes that intersect the source raster tile at the current quad tree depth.
	const filled_drawables_intersecting_nodes_type::intersecting_nodes_type &intersecting_nodes =
			filled_drawables_intersecting_nodes.get_intersecting_nodes();

	const GPlatesMaths::CubeQuadTreeLocation &tile_location =
			filled_drawables_intersecting_nodes.get_node_location();

	// Now add those intersecting nodes that exist (not all areas of the spatial partition will be
	// populated with filled drawables).
	const unsigned int num_intersecting_nodes = intersecting_nodes.get_num_nodes();
	for (unsigned int list_node_index = 0; list_node_index < num_intersecting_nodes; ++list_node_index)
	{
		const filled_drawables_spatial_partition_type::const_node_reference_type &
				intersecting_node_reference = intersecting_nodes.get_node(list_node_index);

		// Only need to add nodes that actually contain filled drawables.
		// NOTE: We still recurse into child nodes though - an empty internal node does not
		// mean the child nodes are necessarily empty.
		if (!intersecting_node_reference.empty())
		{
			// Add the node to the list.
			filled_drawables_intersecting_node_list.push_front(
					filled_drawables_list_node_pool.construct(intersecting_node_reference));
		}

		// Continue to recurse into the spatial partition of filled drawables.
		get_filled_drawables_intersecting_nodes(
				tile_location,
				intersecting_nodes.get_node_location(list_node_index),
				intersecting_node_reference,
				filled_drawables_intersecting_node_list,
				filled_drawables_list_node_pool);
	}

	//
	// Now traverse the list of intersecting filled drawables and render them.
	//

	// Render the source raster tile to the scene.
	render_tile_to_scene(
			gl,
			tile_viewport_dimension,
			mesh_quad_tree_node,
			filled_drawables,
			filled_drawables_intersecting_node_list,
			cube_subdivision_cache,
			cube_subdivision_cache_node,
			clip_cube_subdivision_cache,
			clip_cube_subdivision_cache_node);
}


void
GPlatesOpenGL::GLFilledPolygonsGlobeView::get_filled_drawables_intersecting_nodes(
		const GPlatesMaths::CubeQuadTreeLocation &tile_location,
		const GPlatesMaths::CubeQuadTreeLocation &intersecting_node_location,
		filled_drawables_spatial_partition_type::const_node_reference_type intersecting_node_reference,
		filled_drawables_spatial_partition_node_list_type &intersecting_node_list,
		boost::object_pool<FilledDrawablesListNode> &intersecting_list_node_pool)
{
	// Iterate over the four child nodes of the current parent node.
	for (unsigned int child_y_offset = 0; child_y_offset < 2; ++child_y_offset)
	{
		for (unsigned int child_x_offset = 0; child_x_offset < 2; ++child_x_offset)
		{
			filled_drawables_spatial_partition_type::const_node_reference_type
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
				// Only need to add nodes that actually contain filled drawables.
				// NOTE: We still recurse into child nodes though - an empty internal node does not
				// mean the child nodes are necessarily empty.
				if (!child_intersecting_node_reference.empty())
				{
					// Add the intersecting node to the list.
					intersecting_node_list.push_front(
							intersecting_list_node_pool.construct(child_intersecting_node_reference));
				}

				// Recurse into the current child.
				get_filled_drawables_intersecting_nodes(
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
GPlatesOpenGL::GLFilledPolygonsGlobeView::set_tile_state(
		GL &gl,
		unsigned int tile_viewport_dimension,
		const GLTransform &projection_transform,
		const GLTransform &clip_projection_transform,
		const GLTransform &view_transform,
		bool clip_to_tile_frustum)
{
	// The tile texture contains premultiplied alpha so that when we access it with a bilinear filter
	// the bilinear samples with zero alpha do not contribute to the filtered texture value.
	// This means if we're sampling near the edge of a polygon that was rendered into the tile texture,
	// and there's no adjacent polygon, then the un-rendered (black) tile texels (RGBA all zero)
	// will not corrupt the bilinearly filtered value.
	//
	// In other words the final result in the destination framebuffer (including alpha blending in []) is:
	//
	//    RGB = sum(weight(i) * RGB(i) * Alpha(i)) * [1]  // with blend src factor *1*
	//
	// ...instead of...
	//
	//    RGB = sum(weight(i) * RGB(i)) * [sum(weight(i) * Alpha(i))]  // with blend src factor *alpha*
	//
	// ...where 'weight(i)' are bilinear/anisotropic tile texture filtering weights (that sum to 1.0).
	//
	//
	// So, since RGB has been premultiplied with alpha we want its source factor to be one (instead of alpha):
	//
	//   RGB =     1 * RGB_src + (1-A_src) * RGB_dst
	//
	// And for Alpha we want its source factor to be one (as usual):
	//
	//     A =     1 *   A_src + (1-A_src) *   A_dst
	//
	// ...this enables the destination to be a texture that is subsequently blended into the final scene.
	// In this case the destination alpha must be correct in order to properly blend the texture into the final scene.
	//
	gl.Enable(GL_BLEND);
	gl.BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	// Bind the shader program for rendering *to* the tile texture.
	gl.UseProgram(d_render_tile_to_scene_program);

	// Used to transform position to texture coordinates.
	GLMatrix scene_tile_texture_matrix;
	// Scale texture coordinates [0, 1] to [0, tile_viewport_dimension / tile_dimension] since we
	// might not have used the full tile resolution (when filled polygons were rendered to tile texture).
	const double tile_viewport_scale = double(tile_viewport_dimension) / d_tile_texel_dimension;
	scene_tile_texture_matrix.gl_scale(tile_viewport_scale, tile_viewport_scale, 1.0);
	// Convert clip-space coordinates [-1, 1] to texture coordinates [0, 1].
	scene_tile_texture_matrix.gl_mult_matrix(GLUtils::get_clip_space_to_texture_space_transform());
	// Set up the texture matrix to perform model-view and projection transforms of the frustum.
	scene_tile_texture_matrix.gl_mult_matrix(projection_transform.get_matrix());
	scene_tile_texture_matrix.gl_mult_matrix(view_transform.get_matrix());

	// Load scene tile texture matrix into program.
	GLfloat scene_tile_texture_float_matrix[16];
	scene_tile_texture_matrix.get_float_matrix(scene_tile_texture_float_matrix);
	gl.UniformMatrix4fv(
			d_render_tile_to_scene_program->get_uniform_location(gl, "scene_tile_texture_matrix"),
			1, GL_FALSE/*transpose*/, scene_tile_texture_float_matrix);

	// Bind the scene tile texture to texture unit 0.
	gl.ActiveTexture(GL_TEXTURE0);
	gl.BindTexture(GL_TEXTURE_2D, d_tile_texture);

	// If we've traversed deep enough into the cube quad tree then the cube quad tree mesh
	// cannot provide a drawable that's bounded by the cube quad tree node tile and so
	// we need to use a clip texture.
	gl.Uniform1i(
			d_render_tile_to_scene_program->get_uniform_location(gl, "clip_to_tile_frustum"),
			clip_to_tile_frustum);
	if (clip_to_tile_frustum)
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

		// Load clip texture matrix into program.
		GLfloat clip_texture_float_matrix[16];
		clip_texture_matrix.get_float_matrix(clip_texture_float_matrix);
		gl.UniformMatrix4fv(
				d_render_tile_to_scene_program->get_uniform_location(gl, "clip_texture_matrix"),
				1, GL_FALSE/*transpose*/, clip_texture_float_matrix);

		// Bind the clip texture to texture unit 1.
		gl.ActiveTexture(GL_TEXTURE1);
		gl.BindTexture(GL_TEXTURE_2D, d_multi_resolution_cube_mesh->get_clip_texture());
	}

	const bool lighting_enabled = d_light &&
			d_light.get()->get_scene_lighting_parameters().is_lighting_enabled(
					GPlatesGui::SceneLightingParameters::LIGHTING_FILLED_GEOMETRY_ON_SPHERE);

	// Enable lighting if requested.
	gl.Uniform1i(
			d_render_tile_to_scene_program->get_uniform_location(gl, "lighting_enabled"),
			lighting_enabled);
	if (lighting_enabled)
	{
		// Set the world-space light direction.
		const GPlatesMaths::UnitVector3D &globe_view_light_direction = d_light.get()->get_globe_view_light_direction();
		gl.Uniform3f(
				d_render_tile_to_scene_program->get_uniform_location(gl, "world_space_light_direction"),
				globe_view_light_direction.x().dval(),
				globe_view_light_direction.y().dval(),
				globe_view_light_direction.z().dval());

		// Set the light ambient contribution.
		gl.Uniform1f(
				d_render_tile_to_scene_program->get_uniform_location(gl, "light_ambient_contribution"),
				d_light.get()->get_scene_lighting_parameters().get_ambient_light_contribution());
	}

#if 0
	// Used to render as wire-frame meshes instead of filled textured meshes for visualising mesh density.
	gl.PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
}


void
GPlatesOpenGL::GLFilledPolygonsGlobeView::render_tile_to_scene(
		GL &gl,
		unsigned int tile_viewport_dimension,
		const mesh_quad_tree_node_type &mesh_quad_tree_node,
		const filled_drawables_type &filled_drawables,
		const filled_drawables_spatial_partition_node_list_type &filled_drawables_intersecting_node_list,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
		clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
		const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node)
{
	//PROFILE_FUNC();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	const filled_drawables_spatial_partition_type &filled_drawables_spatial_partition =
			*filled_drawables.d_filled_drawables_spatial_partition;

	// Get the filled drawables.
	filled_drawable_seq_type filled_drawable_seq;
	get_filled_drawables(
			filled_drawable_seq,
			filled_drawables_spatial_partition.begin_root_elements(),
			filled_drawables_spatial_partition.end_root_elements(),
			filled_drawables_intersecting_node_list);

	if (filled_drawable_seq.empty())
	{
		return;
	}

	// Sort the drawables by their original render order.
	// This is necessary because we visited the spatial partition of drawables which is not
	// the same as the original draw order.
	std::sort(
			filled_drawable_seq.begin(),
			filled_drawable_seq.end(),
			filled_drawable_type::SortRenderOrder());

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

	// Render the filled drawables to the tile texture.
	render_filled_drawables_to_tile_texture(
			gl,
			filled_drawable_seq,
			tile_viewport_dimension,
			*projection_transform,
			*view_transform);

	// See if we've traversed deep enough in the cube mesh quad tree to require using a clip
	// texture - this occurs because the cube mesh has nodes only to a certain depth.
	const bool clip_to_tile_frustum = static_cast<bool>(
			mesh_quad_tree_node.get_clip_texture_clip_space_transform());

	// Prepare for rendering the current tile.
	set_tile_state(
			gl,
			tile_viewport_dimension,
			*projection_transform,
			*clip_projection_transform,
			*view_transform,
			clip_to_tile_frustum);

	// Draw the mesh covering the current quad tree node tile.
	mesh_quad_tree_node.render_mesh_drawable(gl);
}


void
GPlatesOpenGL::GLFilledPolygonsGlobeView::render_filled_drawables_to_tile_texture(
		GL &gl,
		const filled_drawable_seq_type &filled_drawables,
		unsigned int tile_viewport_dimension,
		const GLTransform &projection_transform,
		const GLTransform &view_transform)
{
	//PROFILE_FUNC();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(
			gl,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);

	// Bind our framebuffer object for rendering to the tile texture.
	// This directs rendering to the tile texture at the first colour attachment, and
	// its associated depth/stencil renderbuffer at the depth/stencil attachment.
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_tile_texture_framebuffer);

	// Specify the requested tile viewport.
	gl.Viewport(0, 0, tile_viewport_dimension, tile_viewport_dimension);

	// Clear the render target (colour and stencil).
	// We also clear the depth buffer (even though we're not using depth) because it's usually
	// interleaved with stencil so it's more efficient to clear both depth and stencil.
	gl.ClearColor();
	gl.ClearDepth();
	gl.ClearStencil();
	gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// Bind the shader program for rendering *to* the tile texture.
	gl.UseProgram(d_render_to_tile_program);

	// Set view projection matrix in the currently bound program.
	//
	// NOTE: We use the half-texel-expanded projection transform since we want to render the
	// border pixels (in each tile) exactly on the tile (plane) boundary.
	// The tile textures are bilinearly filtered and this way the centres of border texels match up
	// with adjacent tiles.
	GLMatrix view_projection_matrix(projection_transform.get_matrix());
	view_projection_matrix.gl_mult_matrix(view_transform.get_matrix());

	GLfloat view_projection_float_matrix[16];
	view_projection_matrix.get_float_matrix(view_projection_float_matrix);
	gl.UniformMatrix4fv(
			d_render_to_tile_program->get_uniform_location(gl, "view_projection"),
			1, GL_FALSE/*transpose*/, view_projection_float_matrix);

	//
	// For alpha-blending we want:
	//
	//   RGB = A_src * RGB_src + (1-A_src) * RGB_dst
	//     A =     1 *   A_src + (1-A_src) *   A_dst
	//
	// ...so we need to use separate (src,dst) blend factors for the RGB and alpha channels...
	//
	//   RGB uses (A_src, 1 - A_src)
	//     A uses (    1, 1 - A_src)
	//
	// ...this enables the destination to be a texture that is subsequently blended into the final scene.
	// In this case the destination alpha must be correct in order to properly blend the texture into the final scene.
	//
	// Note: We enable/disable blending further below.
	//
	gl.BlendFuncSeparate(
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	// Enable stencil writes (this is the default OpenGL state anyway).
	gl.StencilMask(~0);

	// Enable stencil testing.
	gl.Enable(GL_STENCIL_TEST);

#if 0
	// Used to render as wire-frame meshes instead of filled textured meshes for
	// visualising mesh density.
	gl.PolygonMode(GL_FRONT_AND_BACK, GL_POINT);

	// Set the anti-aliased line state.
	gl.LineWidth(10.0f);
	gl.PointSize(10.0f);
	//gl.Enable(GL_LINE_SMOOTH);
	//gl.Hint(GL_LINE_SMOOTH_HINT, GL_NICEST);
#endif

	// Bind the vertex array before using it to draw.
	gl.BindVertexArray(d_drawables_vertex_array);

	// Iterate over the filled drawables and render each one into the tile texture.
	for (const filled_drawable_type& filled_drawable : filled_drawables)
	{
		// Set the stencil function to always pass.
		gl.StencilFunc(GL_ALWAYS, 0, ~0);
		// Set the stencil operation to invert the stencil buffer value every time a pixel is
		// rendered (this means we get 1 where a pixel is covered by an odd number of triangles
		// and 0 by an even number of triangles).
		gl.StencilOp(GL_KEEP, GL_KEEP, GL_INVERT);

		// Disable colour writes and alpha blending.
		// We only want to modify the stencil buffer on this pass.
		gl.ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		gl.Disable(GL_BLEND);

		// Render the current filled drawable.
		gl.DrawRangeElements(
				GL_TRIANGLES,
				filled_drawable.d_drawable.start,
				filled_drawable.d_drawable.end,
				filled_drawable.d_drawable.count,
				GLVertexUtils::ElementTraits<drawable_vertex_element_type>::type,
				GLVertexUtils::buffer_offset(filled_drawable.d_drawable.indices_offset));

		// Set the stencil function to pass only if the stencil buffer value is non-zero.
		// This means we only draw into the tile texture for pixels 'interior' to the filled drawable.
		gl.StencilFunc(GL_NOTEQUAL, 0, ~0);
		// Set the stencil operation to set the stencil buffer to zero in preparation
		// for the next drawable (also avoids multiple alpha-blending due to overlapping fan
		// triangles as mentioned below).
		gl.StencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

		// Re-enable colour writes and alpha blending.
		gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		gl.Enable(GL_BLEND);

		// Render the current filled drawable.
		// This drawable covers at least all interior pixels of the filled drawable.
		// It also can covers exterior pixels of the filled drawable.
		// However only the interior pixels (where stencil buffer is non-zero) will
		// pass the stencil test and get written into the tile (colour) texture.
		// The drawable also can render pixels multiple times due to overlapping fan triangles.
		// To avoid alpha blending each pixel more than once, the above stencil operation zeros
		// the stencil buffer value of each pixel that passes the stencil test such that the next
		// overlapping pixel will then fail the stencil test (avoiding multiple-alpha-blending).
		gl.DrawRangeElements(
				GL_TRIANGLES,
				filled_drawable.d_drawable.start,
				filled_drawable.d_drawable.end,
				filled_drawable.d_drawable.count,
				GLVertexUtils::ElementTraits<drawable_vertex_element_type>::type,
				GLVertexUtils::buffer_offset(filled_drawable.d_drawable.indices_offset));
	}
}


void
GPlatesOpenGL::GLFilledPolygonsGlobeView::get_filled_drawables(
		filled_drawable_seq_type &filled_drawables,
		filled_drawables_spatial_partition_type::element_const_iterator begin_root_filled_drawables,
		filled_drawables_spatial_partition_type::element_const_iterator end_root_filled_drawables,
		const filled_drawables_spatial_partition_node_list_type &filled_drawables_intersecting_node_list)
{
	//PROFILE_FUNC();

	// Add the filled drawables in the root of the spatial partition.
	// These are the meshes that were too large to insert in any face of the cube quad tree partition.
	// Add the filled drawable of the current node.
	filled_drawables.insert(
			filled_drawables.end(),
			begin_root_filled_drawables,
			end_root_filled_drawables);

	// Iterate over the nodes in the spatial partition that contain the filled drawables we are interested in.
	filled_drawables_spatial_partition_node_list_type::const_iterator filled_drawables_node_iter =
			filled_drawables_intersecting_node_list.begin();
	filled_drawables_spatial_partition_node_list_type::const_iterator filled_drawables_node_end =
			filled_drawables_intersecting_node_list.end();
	for ( ; filled_drawables_node_iter != filled_drawables_node_end; ++filled_drawables_node_iter)
	{
		const filled_drawables_spatial_partition_type::const_node_reference_type &node_reference =
				filled_drawables_node_iter->node_reference;

		// Add the filled drawables of the current node.
		filled_drawables.insert(
				filled_drawables.end(),
				node_reference.begin(),
				node_reference.end());
	}
}


void
GPlatesOpenGL::GLFilledPolygonsGlobeView::create_tile_texture(
		GL &gl)
{
	//PROFILE_FUNC();

	gl.BindTexture(GL_TEXTURE_2D, d_tile_texture);

	// No mipmaps needed so we specify no mipmap filtering.
	// We're not using mipmaps because our cube mapping does not have much distortion
	// unlike global rectangular lat/lon rasters that squash near the poles.
	//
	// We do enable bilinear filtering (also note that the texture is a fixed-point format).
	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Specify anisotropic filtering (if supported) to reduce aliasing in case tile texture is
	// subsequently sampled non-isotropically (such as viewing at an angle near edge of the globe).
	if (gl.get_capabilities().gl_EXT_texture_filter_anisotropic)
	{
		const GLfloat anisotropy = gl.get_capabilities().gl_texture_max_anisotropy;
		gl.TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	}

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Create the texture in OpenGL - this actually creates the texture without any data.
	//
	// NOTE: Since the image data is NULL it doesn't really matter what 'format' (and 'type') are so
	// we just use GL_RGBA (and GL_UNSIGNED_BYTE).
	gl.TexImage2D(GL_TEXTURE_2D, 0/*level*/, GL_RGBA8,
			d_tile_texel_dimension, d_tile_texel_dimension,
			0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// Check there are no OpenGL errors.
	GLUtils::check_gl_errors(gl, GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLFilledPolygonsGlobeView::create_tile_stencil_buffer(
		GL &gl)
{
	gl.BindRenderbuffer(GL_RENDERBUFFER, d_tile_stencil_buffer);

	// Allocate a stencil buffer.
	// Note that (in OpenGL 3.3 core) an OpenGL implementation is only *required* to provide stencil if a
	// depth/stencil format is requested, and furthermore GL_DEPTH24_STENCIL8 is a specified required format.
	gl.RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, d_tile_texel_dimension, d_tile_texel_dimension);
}


void
GPlatesOpenGL::GLFilledPolygonsGlobeView::create_tile_texture_framebuffer(
		GL &gl)
{
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_tile_texture_framebuffer);

	// Bind tile depth/stencil buffer to framebuffer's depth/stencil attachment.
	//
	// We're not actually using the depth buffer but in order to ensure we got a stencil buffer we had
	// to ask for a depth/stencil internal format for the renderbuffer.
	gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, d_tile_stencil_buffer);

	// Bind tile texture to framebuffer's first colour attachment.
	gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, d_tile_texture, 0/*level*/);

	const GLenum completeness = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
	GPlatesGlobal::Assert<OpenGLException>(
			completeness == GL_FRAMEBUFFER_COMPLETE,
			GPLATES_ASSERTION_SOURCE,
			"Framebuffer not complete for rendering tiles in globe filled polygons.");
}


void
GPlatesOpenGL::GLFilledPolygonsGlobeView::create_drawables_vertex_array(
		GL &gl)
{
	// Bind vertex array object.
	gl.BindVertexArray(d_drawables_vertex_array);

	// Bind vertex element buffer object to currently bound vertex array object.
	gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_drawables_vertex_element_buffer);

	// Bind vertex buffer object (used by vertex attribute arrays, not vertex array object).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_drawables_vertex_buffer);

	// Specify vertex attributes (position and colour) in currently bound vertex buffer object.
	// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
	// to currently bound vertex array object.
	gl.EnableVertexAttribArray(0);
	gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(drawable_vertex_type), BUFFER_OFFSET(drawable_vertex_type, x));
	gl.EnableVertexAttribArray(1);
	gl.VertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(drawable_vertex_type), BUFFER_OFFSET(drawable_vertex_type, colour));
}


void
GPlatesOpenGL::GLFilledPolygonsGlobeView::write_filled_drawables_to_vertex_array(
		GL &gl,
		const filled_drawables_type &filled_drawables)
{
	//PROFILE_FUNC();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Bind the vertex array - this binds the vertex *element* buffer (before we load data into it).
	gl.BindVertexArray(d_drawables_vertex_array);

	// Bind vertex buffer object (before we load data into it).
	//
	// Note: Unlike the vertex *element* buffer this vertex buffer binding is not stored in vertex array object state.
	//       So we have to explicitly bind the vertex buffer before storing data in it.
	gl.BindBuffer(GL_ARRAY_BUFFER, d_drawables_vertex_buffer);

	//
	// It's not 'stream' because the same filled drawables are accessed many times.
	// It's not 'dynamic' because we allocate a new buffer (ie, gl.BufferData does not modify existing buffer).
	// We really want to encourage this to be in video memory (even though it's only going to live
	// there for a single rendering frame) because there are many accesses to this buffer as the same
	// drawables are rendered into multiple tiles (otherwise the PCI bus bandwidth becomes the limiting factor).
	//

	// Transfer vertex element data to currently bound vertex element buffer object.
	gl.BufferData(
			GL_ELEMENT_ARRAY_BUFFER,
			filled_drawables.d_drawable_vertex_elements.size() * sizeof(filled_drawables.d_drawable_vertex_elements[0]),
			filled_drawables.d_drawable_vertex_elements.data(),
			GL_STATIC_DRAW);

	// Transfer vertex element data to currently bound vertex buffer object.
	gl.BufferData(
			GL_ARRAY_BUFFER,
			filled_drawables.d_drawable_vertices.size() * sizeof(filled_drawables.d_drawable_vertices[0]),
			filled_drawables.d_drawable_vertices.data(),
			GL_STATIC_DRAW);

	//qDebug() << "Writing triangles: " << filled_drawables.d_drawable_vertex_elements.size() / 3;
}


void
GPlatesOpenGL::GLFilledPolygonsGlobeView::compile_link_shader_programs(
		GL &gl)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope (eg, failure to compile/link shader).
	TRACK_CALL_STACK();

	//
	// Shader program to render filled drawables to the tile texture.
	//

	// Vertex shader source.
	GLShaderSource render_to_tile_vertex_shader_source;
	render_to_tile_vertex_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	render_to_tile_vertex_shader_source.add_code_segment(RENDER_TO_TILE_VERTEX_SHADER_SOURCE);

	// Vertex shader.
	GLShader::shared_ptr_type render_to_tile_vertex_shader = GLShader::create(gl, GL_VERTEX_SHADER);
	render_to_tile_vertex_shader->shader_source(gl, render_to_tile_vertex_shader_source);
	render_to_tile_vertex_shader->compile_shader(gl);

	// Fragment shader source.
	GLShaderSource render_to_tile_fragment_shader_source;
	render_to_tile_fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	render_to_tile_fragment_shader_source.add_code_segment(RENDER_TO_TILE_FRAGMENT_SHADER_SOURCE);

	// Fragment shader.
	GLShader::shared_ptr_type render_to_tile_fragment_shader = GLShader::create(gl, GL_FRAGMENT_SHADER);
	render_to_tile_fragment_shader->shader_source(gl, render_to_tile_fragment_shader_source);
	render_to_tile_fragment_shader->compile_shader(gl);

	// Vertex-fragment program.
	d_render_to_tile_program->attach_shader(gl, render_to_tile_vertex_shader);
	d_render_to_tile_program->attach_shader(gl, render_to_tile_fragment_shader);
	d_render_to_tile_program->link_program(gl);

	//
	// Shader program for the final stage of rendering a tile to the scene.
	// To enhance (or remove effect of) anti-aliasing of drawables edges.
	//

	// Vertex shader source.
	GLShaderSource render_tile_to_scene_vertex_shader_source;
	render_tile_to_scene_vertex_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	render_tile_to_scene_vertex_shader_source.add_code_segment(RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE);

	// Vertex shader.
	GLShader::shared_ptr_type render_tile_to_scene_vertex_shader = GLShader::create(gl, GL_VERTEX_SHADER);
	render_tile_to_scene_vertex_shader->shader_source(gl, render_tile_to_scene_vertex_shader_source);
	render_tile_to_scene_vertex_shader->compile_shader(gl);

	// Fragment shader source.
	GLShaderSource render_tile_to_scene_fragment_shader_source;
	render_tile_to_scene_fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	render_tile_to_scene_fragment_shader_source.add_code_segment(RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE);

	// Fragment shader.
	GLShader::shared_ptr_type render_tile_to_scene_fragment_shader = GLShader::create(gl, GL_FRAGMENT_SHADER);
	render_tile_to_scene_fragment_shader->shader_source(gl, render_tile_to_scene_fragment_shader_source);
	render_tile_to_scene_fragment_shader->compile_shader(gl);

	// Vertex-fragment program.
	d_render_tile_to_scene_program->attach_shader(gl, render_tile_to_scene_vertex_shader);
	d_render_tile_to_scene_program->attach_shader(gl, render_tile_to_scene_fragment_shader);
	d_render_tile_to_scene_program->link_program(gl);

	// Bind the shader program so we can set some uniform parameters in it.
	gl.UseProgram(d_render_tile_to_scene_program);

	// Set the tile texture sampler to texture unit 0.
	gl.Uniform1i(
			d_render_tile_to_scene_program->get_uniform_location(gl, "tile_texture_sampler"),
			0/*texture unit*/);

	// Set the clip texture sampler to texture unit 1.
	gl.Uniform1i(
			d_render_tile_to_scene_program->get_uniform_location(gl, "clip_texture_sampler"),
			1/*texture unit*/);
}


void
GPlatesOpenGL::GLFilledPolygonsGlobeView::FilledDrawables::add_filled_polygon(
		const GPlatesMaths::PolygonOnSphere &polygon,
		GPlatesGui::rgba8_t rgba8_color,
		const boost::optional<GPlatesMaths::CubeQuadTreeLocation> &cube_quad_tree_location)
{
	// Need at least three points for a polygon.
	if (polygon.number_of_vertices_in_exterior_ring() < 3)
	{
		return;
	}

	begin_filled_drawable();

	// Add the polygon's exterior ring.
	add_polygon_ring_mesh_to_current_filled_drawable(
			polygon.exterior_ring_vertex_begin(),
			polygon.number_of_vertices_in_exterior_ring(),
			polygon.get_boundary_centroid(),
			rgba8_color);

	// Add the polygon's interior rings.
	for (unsigned int interior_ring_index = 0;
		interior_ring_index < polygon.number_of_interior_rings();
		++interior_ring_index)
	{
		add_polygon_ring_mesh_to_current_filled_drawable(
				polygon.interior_ring_vertex_begin(interior_ring_index),
				polygon.number_of_vertices_in_interior_ring(interior_ring_index),
				polygon.get_boundary_centroid(),
				rgba8_color);
	}

	end_filled_drawable(cube_quad_tree_location);
}


void
GPlatesOpenGL::GLFilledPolygonsGlobeView::FilledDrawables::add_filled_polygon(
		const GPlatesMaths::PolylineOnSphere &polyline,
		GPlatesGui::rgba8_t rgba8_color,
		const boost::optional<GPlatesMaths::CubeQuadTreeLocation> &cube_quad_tree_location)
{
	// Need at least three points for a polygon.
	if (polyline.number_of_vertices() < 3)
	{
		return;
	}

	begin_filled_drawable();

	add_polygon_ring_mesh_to_current_filled_drawable(
			polyline.vertex_begin(),
			polyline.number_of_vertices(),
			polyline.get_centroid(),
			rgba8_color);

	end_filled_drawable(cube_quad_tree_location);
}


void
GPlatesOpenGL::GLFilledPolygonsGlobeView::FilledDrawables::add_filled_triangle_to_mesh(
		const GPlatesMaths::PointOnSphere &vertex1,
		const GPlatesMaths::PointOnSphere &vertex2,
		const GPlatesMaths::PointOnSphere &vertex3,
		GPlatesGui::rgba8_t rgba8_triangle_color)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_drawable,
			GPLATES_ASSERTION_SOURCE);

	const drawable_vertex_element_type base_vertex_index = d_drawable_vertices.size();

	d_drawable_vertices.push_back(drawable_vertex_type(vertex1.position_vector(), rgba8_triangle_color));
	d_drawable_vertices.push_back(drawable_vertex_type(vertex2.position_vector(), rgba8_triangle_color));
	d_drawable_vertices.push_back(drawable_vertex_type(vertex3.position_vector(), rgba8_triangle_color));

	d_drawable_vertex_elements.push_back(base_vertex_index);
	d_drawable_vertex_elements.push_back(base_vertex_index + 1);
	d_drawable_vertex_elements.push_back(base_vertex_index + 2);

	// Update the current filled drawable.
	d_current_drawable->end += 3;
	d_current_drawable->count += 3;
}


void
GPlatesOpenGL::GLFilledPolygonsGlobeView::FilledDrawables::add_filled_triangle_to_mesh(
		const GPlatesMaths::PointOnSphere &vertex1,
		const GPlatesMaths::PointOnSphere &vertex2,
		const GPlatesMaths::PointOnSphere &vertex3,
		GPlatesGui::rgba8_t rgba8_vertex_color1,
		GPlatesGui::rgba8_t rgba8_vertex_color2,
		GPlatesGui::rgba8_t rgba8_vertex_color3)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_drawable,
			GPLATES_ASSERTION_SOURCE);

	const drawable_vertex_element_type base_vertex_index = d_drawable_vertices.size();

	// Alpha blending will be set up for premultiplied alpha.
	d_drawable_vertices.push_back(drawable_vertex_type(vertex1.position_vector(), rgba8_vertex_color1));
	d_drawable_vertices.push_back(drawable_vertex_type(vertex2.position_vector(), rgba8_vertex_color2));
	d_drawable_vertices.push_back(drawable_vertex_type(vertex3.position_vector(), rgba8_vertex_color3));

	d_drawable_vertex_elements.push_back(base_vertex_index);
	d_drawable_vertex_elements.push_back(base_vertex_index + 1);
	d_drawable_vertex_elements.push_back(base_vertex_index + 2);

	// Update the current filled drawable.
	d_current_drawable->end += 3;
	d_current_drawable->count += 3;
}


void
GPlatesOpenGL::GLFilledPolygonsGlobeView::FilledDrawables::begin_filled_drawable()
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!d_current_drawable,
			GPLATES_ASSERTION_SOURCE);

	const GLsizei base_vertex_element_index = d_drawable_vertex_elements.size();
	const drawable_vertex_element_type base_vertex_index = d_drawable_vertices.size();

	d_current_drawable = boost::in_place(
			base_vertex_index/*start*/,
			base_vertex_index/*end*/, // This will get updated.
			0/*count*/, // This will get updated.
			base_vertex_element_index * sizeof(drawable_vertex_element_type)/*indices_offset*/);
}


void
GPlatesOpenGL::GLFilledPolygonsGlobeView::FilledDrawables::end_filled_drawable(
		const boost::optional<GPlatesMaths::CubeQuadTreeLocation> &cube_quad_tree_location)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_current_drawable,
			GPLATES_ASSERTION_SOURCE);

	// Add the filled drawable if it's not empty.
	if (d_current_drawable->count > 0)
	{
		// Keep track of the order to render the drawables (order in which we're called)
		// because drawables are rendered by visiting the spatial partition which is not
		// the same as the original draw order.
		const unsigned int render_order = d_filled_drawables_spatial_partition->size();
		const filled_drawable_type filled_drawable(d_current_drawable.get(), render_order);

		if (cube_quad_tree_location)
		{
			d_filled_drawables_spatial_partition->add(filled_drawable, cube_quad_tree_location.get());
		}
		else
		{
			d_filled_drawables_spatial_partition->add_unpartitioned(filled_drawable);
		}
	}

	// Finished with the current filled drawable.
	d_current_drawable = boost::none;
}
