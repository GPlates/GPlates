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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include <cmath>
#include <limits>
#include <new> // For placement new.
#include <vector>
#include <boost/bind/bind.hpp>
#include <boost/cast.hpp>
#include <boost/cstdint.hpp>
#include <boost/scoped_array.hpp>

#include <QDebug>

#include "GLMultiResolutionRaster.h"

#include "GL.h"
#include "GLContext.h"
#include "GLDataRasterSource.h"
#include "GLFrustum.h"
#include "GLIntersect.h"
#include "GLNormalMapSource.h"
#include "GLScalarFieldDepthLayersSource.h"
#include "GLShader.h"
#include "GLShaderSource.h"
#include "GLUtils.h"
#include "GLVertexUtils.h"
#include "GLViewProjection.h"
#include "GLVisualRasterSource.h"
#include "OpenGLException.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Profile.h"

namespace GPlatesOpenGL
{
	namespace
	{
		//! The inverse of log(2.0).
		const float INVERSE_LOG2 = 1.0 / std::log(2.0);

		/**
		 * Vertex shader source for rendering a visual raster source.
		 */
		const char *RENDER_VISUAL_RASTER_VERTEX_SHADER_SOURCE =
			R"(
				uniform mat4 view_projection;
			
				layout(location = 0) in vec4 position;
				layout(location = 1) in vec2 vertex_tex_coord;

				out vec2 tex_coord;
			
				void main (void)
				{
					gl_Position = view_projection * position;
					tex_coord = vertex_tex_coord;
				}
			)";

		/**
		 * Fragment shader source for rendering a visual raster source.
		 */
		const char *RENDER_VISUAL_RASTER_FRAGMENT_SHADER_SOURCE =
			R"(
				uniform sampler2D source_texture_sampler;
				uniform vec4 modulate_color;

				in vec2 tex_coord;
			
				layout(location = 0) out vec4 colour;

				void main (void)
				{
					// Note: Source texture has premultiplied alpha, so multiply by
					//       a premultiplied-alpha version of the modulate colour.
					colour = texture(source_texture_sampler, tex_coord);
					colour *= vec4(modulate_color.rgb * modulate_color.a, modulate_color.a)
				}
			)";

		/**
		 * Vertex shader source for rendering a data raster source.
		 */
		const char *RENDER_DATA_RASTER_VERTEX_SHADER_SOURCE =
			R"(
				uniform mat4 view_projection;
			
				layout(location = 0) in vec4 position;
				layout(location = 1) in vec2 vertex_tex_coord;

				out vec2 tex_coord;
			
				void main (void)
				{
					gl_Position = view_projection * position;
					tex_coord = vertex_tex_coord;
				}
			)";

		/**
		 * Fragment shader source for rendering a data raster source.
		 */
		const char *RENDER_DATA_RASTER_FRAGMENT_SHADER_SOURCE =
			R"(
				uniform sampler2D source_texture_sampler;

				in vec2 tex_coord;
			
				layout(location = 0) out vec4 data;

				void main (void)
				{
					//
					// The source raster is a data raster that uses the 2-channel RG texture format
					// with data in Red and coverage in Green. Since there's no Alpha channel,
					// the texture swizzle (set in texture object) copies coverage in the Green channel
					// into the Alpha channel. Also if we're outputting to an RG render-texture then Alpha
					// gets discarded, but that's OK since coverage is still in the Green channel.
					//
					// Also note that the data (Red) is premultiplied by coverage.
					//
					data = texture(source_texture_sampler, tex_coord);
				}
			)";

		/**
		 * Vertex shader source for rendering a scalar field depth layers raster source.
		 */
		const char *RENDER_SCALAR_FIELD_DEPTH_LAYERS_RASTER_VERTEX_SHADER_SOURCE =
			R"(
				uniform mat4 view_projection;
			
				layout(location = 0) in vec4 position;
				layout(location = 1) in vec2 vertex_tex_coord;
				layout(location = 2) in vec3 vertex_tangent;
				layout(location = 3) in vec3 vertex_binormal;
				layout(location = 4) in vec3 vertex_normal;

				out vec2 tex_coord;
				out vec3 tangent;
				out vec3 binormal;
				out vec3 normal;
			
				void main (void)
				{
					gl_Position = view_projection * position;
					tex_coord = vertex_tex_coord;
					tangent = vertex_tangent;
					binormal = vertex_binormal;
					normal = vertex_normal;
				}
			)";

		/**
		 * Fragment shader source for rendering a scalar field depth layers raster source.
		 */
		const char *RENDER_SCALAR_FIELD_DEPTH_LAYERS_RASTER_FRAGMENT_SHADER_SOURCE =
			R"(
				uniform sampler2D source_texture_sampler;

				in vec2 tex_coord;
				in vec3 tangent
				in vec3 binormal
				in vec3 normal
			
				layout(location = 0) out vec4 scalar_gradient;

				void main (void)
				{
					// Sample the scalar and gradient in the R and GBA channels (of RGBA texture).
					scalar_gradient = texture(source_texture_sampler, tex_coord);

					// NOTE: We don't normalize each interpolated tangent frame axis.
					// At least the tangent and binormal vectors have magnitudes corresponding to the
					// length of a texel in the u and v directions. The normal (radial) is unit length.

					// The input texture gradient is a mixture of finite differences of the scalar field
					// and inverse distances along the tangent frame directions.
					// Complete the gradient generation by providing the tangent frame directions and the
					// missing components of the inverse distances (the inverse distance across a texel
					// along the tangent and binormal directions on the surface of the globe -
					// the input texture has taken care of radial distances for all tangent frame components).
					//
					// The gradient is in the (green,blue,alpha) channels.
					scalar_gradient.gba = mat3(tangent, binormal, normal) * scalar_gradient.gba;
				}
			)";

		/**
		 * Vertex shader source for rendering a normal map raster source.
		 */
		const char *RENDER_NORMAL_MAP_RASTER_VERTEX_SHADER_SOURCE =
			R"(
				uniform mat4 view_projection;
			
				layout(location = 0) in vec4 position;
				layout(location = 1) in vec2 vertex_tex_coord;
				layout(location = 2) in vec3 vertex_tangent;
				layout(location = 3) in vec3 vertex_binormal;
				layout(location = 4) in vec3 vertex_normal;

				out vec2 tex_coord;
				out vec3 tangent;
				out vec3 binormal;
				out vec3 normal;
			
				void main (void)
				{
					gl_Position = view_projection * position;
					tex_coord = vertex_tex_coord;
					tangent = vertex_tangent;
					binormal = vertex_binormal;
					normal = vertex_normal;
				}
			)";

		/**
		 * Fragment shader source for rendering a normal map raster source.
		 */
		const char *RENDER_NORMAL_MAP_RASTER_FRAGMENT_SHADER_SOURCE =
			R"(
				uniform sampler2D source_texture_sampler;

				in vec2 tex_coord;
				in vec3 tangent
				in vec3 binormal
				in vec3 normal
			
				layout(location = 0) out vec4 normal;

				void main (void)
				{
					// Sample the tangent-space normal in the source raster.
					vec3 tangent_space_normal = texture(source_texture_sampler, tex_coord).xyz;

					// Need to convert the x and y components from unsigned to signed ([0,1] -> [-1,1]).
					// The z component is always positive (in range [0,1]) so does not need conversion.
					tangent_space_normal.xy = 2 * tangent_space_normal.xy - 1;

					// Normalize each interpolated tangent frame axis.
					// They are unit length at vertices but not necessarily between vertices.
					// TODO: Might need to be careful if any interpolated vectors are zero length.
					tangent = normalize(tangent);
					binormal = normalize(binormal);
					normal = normalize(normal);

					// Convert tangent-space normal to world-space normal.
					vec3 world_space_normal = normalize(
						mat3(tangent, binormal, normal) * tangent_space_normal);

					// All components of world-space normal are signed and need to be converted to
					// unsigned ([-1,1] -> [0,1]) before storing in fixed-point 8-bit RGBA render target.
					normal.xyz = 0.5 * world_space_normal + 0.5;
					normal.w = 1;
				}
			)";


		/**
		 * Vertex shader source code to render sphere normals as part of clearing a render target
		 * before rendering a normal-map raster.
		 *
		 * For example, for a *regional* normal map raster the normals outside the region are not rendered by
		 * the normal map raster itself and so must be initialised to be normal to the globe's surface.
		 */
		const char *RENDER_SPHERE_NORMALS_VERTEX_SHADER_SOURCE =
			R"(
				uniform mat4 view_projection;
			
				layout(location = 0) in vec4 position;

				out vec3 cube_position;
			
				void main (void)
				{
					gl_Position = view_projection * position;
				
					cube_position = position.xyz;
				}
			)";

		/**
		 * Fragment shader source code to render sphere normals as part of clearing a render target
		 * before rendering a normal-map raster.
		 *
		 * For example, for a *regional* normal map raster the normals outside the region are not rendered by
		 * the normal map raster itself and so must be initialised to be normal to the globe's surface.
		 */
		const char *RENDER_SPHERE_NORMALS_FRAGMENT_SHADER_SOURCE =
			R"(
				in vec3 cube_position;
			
				layout(location = 0) out vec4 normal;

				void main (void)
				{
					// Normalize the interpolated cube position (which is unit length only at the vertices).
					// All components of world-space normal are signed and need to be converted to
					// unsigned ([-1,1] -> [0,1]) before storing in fixed-point 8-bit RGBA render target.
					normal.xyz = 0.5 * normalize(cube_position) + 0.5;
					normal.w = 1;
				}
			)";
	}
}


GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type
GPlatesOpenGL::GLMultiResolutionRaster::create(
		GL &gl,
		const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
		const GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_to_const_type &coordinate_transformation,
		const GLMultiResolutionRasterSource::non_null_ptr_type &raster_source,
		CacheTileTexturesType cache_tile_textures,
		RasterScanlineOrderType raster_scanline_order)
{
	return non_null_ptr_type(
			new GLMultiResolutionRaster(
					gl,
					georeferencing,
					coordinate_transformation,
					raster_source,
					cache_tile_textures,
					raster_scanline_order));
}


GPlatesOpenGL::GLMultiResolutionRaster::GLMultiResolutionRaster(
		GL &gl,
		const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
		const GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_to_const_type &coordinate_transformation,
		const GLMultiResolutionRasterSource::non_null_ptr_type &raster_source,
		CacheTileTexturesType cache_tile_textures,
		RasterScanlineOrderType raster_scanline_order) :
	d_georeferencing(georeferencing),
	d_coordinate_transformation(coordinate_transformation),
	d_raster_source(raster_source),
	// The raster dimensions (the highest resolution level-of-detail).
	d_raster_width(raster_source->get_raster_width()),
	d_raster_height(raster_source->get_raster_height()),
	d_raster_scanline_order(raster_scanline_order),
	d_tile_texel_dimension(raster_source->get_tile_texel_dimension()),
	d_inverse_tile_texel_dimension(1.0f / raster_source->get_tile_texel_dimension()),
	d_num_texels_per_vertex(/*default*/MAX_NUM_TEXELS_PER_VERTEX << 16), // ...a 16:16 fixed-point type.
	// The parentheses around min are to prevent the windows min macros
	// from stuffing numeric_limits' min.
	d_max_highest_resolution_texel_size_on_unit_sphere((std::numeric_limits<float>::min)()),
	// Start with small size cache and just let the cache grow in size as needed if caching enabled...
	d_tile_texture_cache(tile_texture_cache_type::create(2/* GPU pipeline breathing room in case caching disabled*/)),
	d_cache_tile_textures(cache_tile_textures),
	// Start with smallest size cache and just let the cache grow in size as needed...
	d_tile_vertices_cache(tile_vertices_cache_type::create()),
	d_render_raster_program(GLProgram::create(gl))
{
	// Determine number of texels between two adjacent vertices along a horizontal/vertical tile edge.
	// For most rasters this is the maximum texel density.
	// For very low resolution rasters a smaller texel density is needed to keep the mesh surface
	// looking smooth and curved instead of coarsely tessellated on the globe.
	d_num_texels_per_vertex = calculate_num_texels_per_vertex();

	// Create the levels of details and within each one create an oriented bounding box
	// tree (used to quickly find visible tiles) where the drawable tiles are in the
	// leaf nodes of the OBB tree.
	initialise_level_of_detail_pyramid();

	// If the source raster is a normal map then adjust its height field scale depending its resolution.
	if (dynamic_cast<GLNormalMapSource *>(raster_source.get()))
	{
		GPlatesUtils::dynamic_pointer_cast<GLNormalMapSource>(raster_source)
				->set_max_highest_resolution_texel_size_on_unit_sphere(
						d_max_highest_resolution_texel_size_on_unit_sphere);
	}

	// If the client has requested the entire level-of-detail pyramid be cached.
	// This does not consume memory until each individual tile is requested.
	// For example, if all level 0 tiles are accessed but none of the other levels then memory
	// will only be used for the level 0 tiles.
	if (d_cache_tile_textures == CACHE_TILE_TEXTURES_ENTIRE_LEVEL_OF_DETAIL_PYRAMID)
	{
		// This effectively disables any recycling that would otherwise happen in the cache.
		d_tile_texture_cache->set_min_num_objects(d_tiles.size());
		d_tile_vertices_cache->set_min_num_objects(d_tiles.size());
	}

	compile_link_shader_program(gl);
}


float
GPlatesOpenGL::GLMultiResolutionRaster::get_level_of_detail(
		const GLViewProjection &view_projection,
		float level_of_detail_bias) const
{
	// Get the minimum size of a pixel in the current viewport when projected
	// onto the unit sphere (in model space).
	const double min_pixel_size_on_unit_sphere = view_projection.get_min_pixel_size_on_unit_sphere();

	// Calculate the level-of-detail.
	// This is the equivalent of:
	//
	//    t = t0 * 2 ^ (lod - lod_bias)
	//
	// ...where 't0' is the texel size of the *highest* resolution level-of-detail and
	// 't' is the projected size of a pixel of the viewport. And 'lod_bias' is used
	// by clients to allow the largest texel in a drawn texture to be larger than
	// a pixel in the viewport (which can result in blockiness in places in the rendered scene).
	//
	// Note: we return the un-clamped floating-point level-of-detail so clients of this class
	// can see if they need a higher resolution render-texture, for example, to render
	// our raster into - so in that case they'd increase their render-target resolution or
	// decrease their render target view frustum until the level-of-detail was zero.
	return level_of_detail_bias + INVERSE_LOG2 *
			(std::log(min_pixel_size_on_unit_sphere) -
					std::log(d_max_highest_resolution_texel_size_on_unit_sphere));
}


float
GPlatesOpenGL::GLMultiResolutionRaster::clamp_level_of_detail(
		float level_of_detail) const
{
	// Clamp to highest resolution level of detail.
	if (level_of_detail < 0)
	{
		// If we get here then even the highest resolution level-of-detail did not have enough
		// resolution for the specified level of detail but it'll have to do since its the
		// highest resolution we have to offer.
		// This is where the user will start to see magnification of the raster.
		return 0;
	}

	// Clamp to lowest resolution level of detail.
	if (level_of_detail > d_level_of_detail_pyramid.size() - 1)
	{
		// If we get there then even our lowest resolution level of detail had too much resolution
		// for the specified level of detail - but this is pretty unlikely for all but the very
		// smallest of viewports.
		//
		// Note that float can represent integers (up to 23 bits) exactly so returning as float is fine.
		return d_level_of_detail_pyramid.size() - 1;
	}

	return level_of_detail;
}


void
GPlatesOpenGL::GLMultiResolutionRaster::get_visible_tiles(
		std::vector<tile_handle_type> &visible_tiles,
		const GLMatrix &view_projection_transform,
		float level_of_detail) const
{
	// There should be levels of detail and the specified level of detail should be in range.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			!d_level_of_detail_pyramid.empty() &&
				level_of_detail >= 0 &&
				level_of_detail <= d_level_of_detail_pyramid.size() - 1,
			GPLATES_ASSERTION_SOURCE);

	// Truncate floating-point level of detail down to an integer level-of-detail.
	const unsigned int pyramid_level = boost::numeric_cast<unsigned int>(level_of_detail);
	const LevelOfDetail &lod = *d_level_of_detail_pyramid[pyramid_level];

	//
	// Traverse the OBB tree of the level-of-detail and gather a list of tiles that
	// are visible in the view frustum.
	//

	// First get the view frustum planes.
	const GLFrustum frustum_planes(view_projection_transform);

	// Get the root OBB tree node of the level-of-detail.
	const LevelOfDetail::OBBTreeNode &lod_root_obb_tree_node = lod.get_obb_tree_node(lod.obb_tree_root_node_index);

	// Recursively traverse the OBB tree to find visible tiles.
	get_visible_tiles(frustum_planes, GLFrustum::ALL_PLANES_ACTIVE_MASK, lod, lod_root_obb_tree_node, visible_tiles);
}


void
GPlatesOpenGL::GLMultiResolutionRaster::get_visible_tiles(
		const GLFrustum &frustum_planes,
		boost::uint32_t frustum_plane_mask,
		const LevelOfDetail &lod,
		const LevelOfDetail::OBBTreeNode &obb_tree_node,
		std::vector<tile_handle_type> &visible_tiles) const
{
	// If the frustum plane mask is zero then it means we are entirely inside the view frustum.
	// So only test for intersection if the mask is non-zero.
	if (frustum_plane_mask != 0)
	{
		// See if the OBB of the current OBB tree node intersects the view frustum.
		boost::optional<boost::uint32_t> out_frustum_plane_mask =
				GLIntersect::intersect_OBB_frustum(
						obb_tree_node.bounding_box,
						frustum_planes.get_planes(),
						frustum_plane_mask);
		if (!out_frustum_plane_mask)
		{
			// No intersection so OBB is outside the view frustum and we can cull it.
			return;
		}

		// Update the frustum plane mask so we only test against those planes that
		// the current bounding box intersects. The bounding box is entirely inside
		// the planes with a zero bit and so its child nodes are also entirely inside
		// those planes too and so they won't need to test against them.
		frustum_plane_mask = out_frustum_plane_mask.get();
	}

	// See if it's a OBB tree *leaf* node.
	if (obb_tree_node.is_leaf_node)
	{
		// This leaf node is visible in the view frustum so
		// add its tile to the list of visible tiles.
		visible_tiles.push_back(obb_tree_node.tile);
		return;
	}
	// It's an *internal* OBB tree node.

	// Traverse the child nodes.
	get_visible_tiles(
			frustum_planes,
			frustum_plane_mask,
			lod,
			lod.get_obb_tree_node(obb_tree_node.child_node_indices[0]),
			visible_tiles);
	get_visible_tiles(
			frustum_planes,
			frustum_plane_mask,
			lod,
			lod.get_obb_tree_node(obb_tree_node.child_node_indices[1]),
			visible_tiles);
}


void
GPlatesOpenGL::GLMultiResolutionRaster::clear_framebuffer(
		GL &gl,
		const GLMatrix &view_projection_transform)
{
	if (dynamic_cast<GLNormalMapSource *>(d_raster_source.get()) != nullptr)
	{
		//
		// We have a normal map raster source.
		//

		// If we've not yet created the ability to render sphere normals then do so now.
		if (!d_render_sphere_normals)
		{
			d_render_sphere_normals = boost::in_place(boost::ref(gl));
		}

		// Render the sphere normals.
		d_render_sphere_normals->render(gl, view_projection_transform);
	}
	else // *not* a normal map...
	{
		// Clear the colour buffer.
		gl.ClearColor(); // Clear colour to all zeros.
		glClear(GL_COLOR_BUFFER_BIT); // Clear only the colour buffer.
	}
}


bool
GPlatesOpenGL::GLMultiResolutionRaster::render(
		GL &gl,
		const GLMatrix &view_projection_transform,
		float level_of_detail,
		cache_handle_type &cache_handle)
{
	// The GLMultiResolutionRasterInterface interface says an exception is thrown if level-of-detail
	// is outside the valid range.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			level_of_detail >= 0 && level_of_detail <= get_num_levels_of_detail() - 1,
			GPLATES_ASSERTION_SOURCE);

	// Get the tiles visible in the view frustum.
	std::vector<tile_handle_type> visible_tiles;
	get_visible_tiles(visible_tiles, view_projection_transform, level_of_detail);

	// Return early if there are no tiles to render.
	if (visible_tiles.empty())
	{
		cache_handle = cache_handle_type();
		return false;
	}

	return render(gl, view_projection_transform, visible_tiles, cache_handle);
}


bool
GPlatesOpenGL::GLMultiResolutionRaster::render(
		GL &gl,
		const GLMatrix &view_projection_transform,
		const std::vector<tile_handle_type> &tiles,
		cache_handle_type &cache_handle)
{
	PROFILE_FUNC();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Bind the shader program.
	gl.UseProgram(d_render_raster_program);

	// Set view projection matrix in the currently bound program.
	GLfloat view_projection_float_matrix[16];
	view_projection_transform.get_float_matrix(view_projection_float_matrix);
	glUniformMatrix4fv(
			d_render_raster_program->get_uniform_location("view_projection"),
			1, GL_FALSE/*transpose*/, view_projection_float_matrix);

	if (const GLVisualRasterSource *visual_raster_source = dynamic_cast<const GLVisualRasterSource *>(d_raster_source.get()))
	{
		// Visual (colour) rasters require multiplying by their modulate colour.
		const GPlatesGui::Colour &modulate_colour = visual_raster_source->get_modulate_colour();
		glUniform4f(
				d_render_raster_program->get_uniform_location("modulate_color"),
				modulate_colour.red(), modulate_colour.green(), modulate_colour.blue(), modulate_colour.alpha());
	}

	// Set alpha blending for visual and data rasters (since visual rasters have alpha and data rasters have coverage).
	// But don't set for normal map rasters or scalar field depth layer rasters (since they don't have alpha or coverage).
	if (dynamic_cast<const GLVisualRasterSource *>(d_raster_source.get()) ||
		dynamic_cast<const GLDataRasterSource *>(d_raster_source.get()))
	{
		// Visual rasters have premultiplied alpha (and data rasters have premultiplied coverage).
		// So we want the alpha blending source factor to be one instead of alpha (or coverage):
		//
		// For visual rasters this means:
		//
		//   RGB =     1 * RGB_src + (1-A_src) * RGB_dst
		//     A =     1 *   A_src + (1-A_src) *   A_dst
		//
		// ..and for data rasters this means:
		//
		//   data(R)     =     1 * data_src(R)     + (1-coverage_src(A=G)) * data_dst(R)
		//   coverage(G) =     1 * coverage_src(G) + (1-coverage_src(A=G)) * coverage_dst(G)
		//
		// Note: Data rasters use the 2-channel RG texture format with data in Red and coverage in Green.
		//       Since there's no Alpha channel, the texture swizzle (set in texture object) copies coverage in the
		//       Green channel into Alpha channel where the alpha blender can access it (with GL_ONE_MINUS_SRC_ALPHA).
		//       The alpha blender output can then only store RG channels (if rendering a render target, such as cube map,
		//       and not the final/main framebuffer) and so Alpha gets discarded (but still used by alpha-blender),
		//       however that's OK since coverage is still in the Green channel.
		//
		gl.Enable(GL_BLEND);
		gl.BlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	}

#if 0
	// Used to render as wire-frame meshes instead of filled textured meshes for
	// visualising mesh density.
	gl.PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif

	// The cached view is a sequence of tiles for the caller to keep alive until the next frame.
	boost::shared_ptr<std::vector<ClientCacheTile> > cached_tiles(new std::vector<ClientCacheTile>());
	cached_tiles->reserve(tiles.size());

	// We'll bind our tile texture to unit 0.
	gl.ActiveTexture(GL_TEXTURE0);

	// Render each tile.
	for (GLMultiResolutionRaster::tile_handle_type tile_handle : tiles)
	{
		const Tile tile = get_tile(tile_handle, gl);

		// Bind the current tile's texture.
		gl.BindTexture(GL_TEXTURE_2D, tile.tile_texture->texture);

		// Bind the current tile's vertices.
		gl.BindVertexArray(tile.tile_vertices->vertex_array);

		// Draw the current tile.
		glDrawElements(
				GL_TRIANGLES,
				tile.tile_vertices->num_vertex_elements/*count*/,
				GLVertexUtils::ElementTraits<vertex_element_type>::type,
				nullptr/*indices*/);

		// The caller will cache this tile to keep it from being prematurely recycled by our caches.
		//
		// Note that none of this has any effect if the client specified the entire level-of-detail
		// pyramid be cached (in the 'create()' method) in which case it'll get cached regardless.
		cached_tiles->push_back(ClientCacheTile(tile, d_cache_tile_textures));
	}

	// Return cached tiles to the caller.
	cache_handle = cached_tiles;

	return !tiles.empty();
}


GPlatesOpenGL::GLMultiResolutionRaster::Tile
GPlatesOpenGL::GLMultiResolutionRaster::get_tile(
		tile_handle_type tile_handle,
		GL &gl)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			tile_handle < d_tiles.size(),
			GPLATES_ASSERTION_SOURCE);

	const LevelOfDetailTile &lod_tile = *d_tiles[tile_handle];

	// Get the texture for the tile.
	tile_texture_cache_type::object_shared_ptr_type tile_texture = get_tile_texture(gl, lod_tile);

	// Get the vertices for the tile.
	tile_vertices_cache_type::object_shared_ptr_type tile_vertices = get_tile_vertices(gl, lod_tile);

	// Return the tile to the caller.
	// Each tile has its own vertices and texture but shares the same triangles (vertex indices).
	return Tile(tile_vertices, tile_texture);
}


GPlatesOpenGL::GLMultiResolutionRaster::tile_texture_cache_type::object_shared_ptr_type
GPlatesOpenGL::GLMultiResolutionRaster::get_tile_texture(
		GL &gl,
		const LevelOfDetailTile &lod_tile)
{
	// See if we've previously created our tile texture and
	// see if it hasn't been recycled by the texture cache.
	tile_texture_cache_type::object_shared_ptr_type tile_texture = lod_tile.tile_texture->get_cached_object();
	if (!tile_texture)
	{
		tile_texture = lod_tile.tile_texture->recycle_an_unused_object();
		if (!tile_texture)
		{
			// Create a new tile texture.
			tile_texture = lod_tile.tile_texture->set_cached_object(
					std::unique_ptr<TileTexture>(new TileTexture(gl)),
					// Called whenever tile texture is returned to the cache...
					boost::bind(&TileTexture::returned_to_cache, boost::placeholders::_1));

			// The texture was just allocated so we need to create it in OpenGL.
			create_texture(gl, tile_texture->texture);

			//qDebug() << "GLMultiResolutionRaster: " << d_tile_texture_cache->get_current_num_objects_in_use();
		}

		load_raster_data_into_tile_texture(
				lod_tile,
				*tile_texture,
				gl);
	}
	// Our texture wasn't recycled but see if it's still valid in case the source
	// raster changed the data underneath us.
	else if (!d_raster_source->get_subject_token().is_observer_up_to_date(lod_tile.source_texture_observer_token))
	{
		// Load the data into the texture.
		load_raster_data_into_tile_texture(
				lod_tile,
				*tile_texture,
				gl);
	}

	return tile_texture;
}


void
GPlatesOpenGL::GLMultiResolutionRaster::load_raster_data_into_tile_texture(
		const LevelOfDetailTile &lod_tile,
		TileTexture &tile_texture,
		GL &gl)
{
	PROFILE_FUNC();

	// Get our source to load data into the texture.
	tile_texture.source_cache_handle =
			d_raster_source->load_tile(
					lod_tile.lod_level,
					lod_tile.u_lod_texel_offset,
					lod_tile.v_lod_texel_offset,
					lod_tile.num_u_lod_texels,
					lod_tile.num_v_lod_texels,
					tile_texture.texture,
					gl);

	// This tile texture is now update-to-date.
	d_raster_source->get_subject_token().update_observer(lod_tile.source_texture_observer_token);
}


bool
GPlatesOpenGL::GLMultiResolutionRaster::tile_texture_is_visual() const
{
	// Only a visual raster source is visual.
	return dynamic_cast<const GLVisualRasterSource*>(d_raster_source.get()) != nullptr;
}


void
GPlatesOpenGL::GLMultiResolutionRaster::create_texture(
		GL &gl,
		const GLTexture::shared_ptr_type &texture)
{
	const GLCapabilities &capabilities = gl.get_capabilities();

	// Bind the texture.
	gl.BindTexture(GL_TEXTURE_2D, texture);

	// Note: Currently all filtering is 'nearest' instead of 'bilinear'.
	//       This is because the tiles do not overlap by border by half a texel (to avoid bilinear
	//       seams between tiles). But this may change (though it would require significant changes).
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Specify anisotropic filtering (if supported) to reduce aliasing in case tile texture is
	// subsequently sampled non-isotropically.
	//
	// Anisotropic filtering is an ubiquitous extension (that didn't become core until OpenGL 4.6).
	if (capabilities.gl_EXT_texture_filter_anisotropic)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, capabilities.gl_texture_max_anisotropy);
	}

	// If the source texture contains alpha or coverage and its not in the alpha channel then swizzle the texture
	// so it is copied to the alpha channel (eg, a data RG texture copies coverage from G to A).
	boost::optional<GLenum> texture_swizzle_alpha = d_raster_source->get_tile_texture_swizzle_alpha();
	if (texture_swizzle_alpha)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, texture_swizzle_alpha.get());
	}

	// Create the texture in OpenGL - this actually creates the texture without any data.
	// We'll be getting our raster source to load image data into the texture.
	//
	// NOTE: Since the image data is NULL it doesn't really matter what 'format' and 'type' are -
	// just use values that are compatible with all internal formats to avoid a possible error.
	glTexImage2D(
			GL_TEXTURE_2D, 0/*level*/,
			d_raster_source->get_tile_texture_internal_format(),
			d_tile_texel_dimension, d_tile_texel_dimension,
			0/*border*/, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// Check there are no OpenGL errors.
	GLUtils::check_gl_errors(GPLATES_ASSERTION_SOURCE);
}


GPlatesOpenGL::GLMultiResolutionRaster::tile_vertices_cache_type::object_shared_ptr_type
GPlatesOpenGL::GLMultiResolutionRaster::get_tile_vertices(
		GL &gl,
		const LevelOfDetailTile &lod_tile)
{
	// See if we've previously created our tile vertices and
	// see if they haven't been recycled by the tile vertices cache.
	tile_vertices_cache_type::object_shared_ptr_type tile_vertices = lod_tile.tile_vertices->get_cached_object();
	if (!tile_vertices)
	{
		tile_vertices = lod_tile.tile_vertices->recycle_an_unused_object();
		if (!tile_vertices)
		{
			tile_vertices = lod_tile.tile_vertices->set_cached_object(
					std::unique_ptr<TileVertices>(new TileVertices(gl)));

			// Bind the new tile vertex array.
			gl.BindVertexArray(tile_vertices->vertex_array);

			// Bind the new vertex buffer object (used by vertex attribute arrays, not vertex array object).
			gl.BindBuffer(GL_ARRAY_BUFFER, tile_vertices->vertex_buffer);

			//
			// Specify vertex attributes (eg, position) in currently bound vertex buffer object.
			// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
			// to currently bound vertex array object.
			//
			if (dynamic_cast<GLVisualRasterSource *>(d_raster_source.get()))
			{
				gl.EnableVertexAttribArray(0);
				gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(visual_vertex_type), BUFFER_OFFSET(visual_vertex_type, x));
				gl.EnableVertexAttribArray(1);
				gl.VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(visual_vertex_type), BUFFER_OFFSET(visual_vertex_type, u));
			}
			else if (dynamic_cast<GLDataRasterSource *>(d_raster_source.get()))
			{
				gl.EnableVertexAttribArray(0);
				gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(data_vertex_type), BUFFER_OFFSET(data_vertex_type, x));
				gl.EnableVertexAttribArray(1);
				gl.VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(data_vertex_type), BUFFER_OFFSET(data_vertex_type, u));
			}
			else if (dynamic_cast<GLNormalMapSource *>(d_raster_source.get()))
			{
				gl.EnableVertexAttribArray(0);
				gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(normal_map_vertex_type), BUFFER_OFFSET(normal_map_vertex_type, x));
				gl.EnableVertexAttribArray(1);
				gl.VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(normal_map_vertex_type), BUFFER_OFFSET(normal_map_vertex_type, u));
				gl.EnableVertexAttribArray(2);
				gl.VertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(normal_map_vertex_type), BUFFER_OFFSET(normal_map_vertex_type, tangent_x));
				gl.EnableVertexAttribArray(3);
				gl.VertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(normal_map_vertex_type), BUFFER_OFFSET(normal_map_vertex_type, binormal_x));
				gl.EnableVertexAttribArray(4);
				gl.VertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(normal_map_vertex_type), BUFFER_OFFSET(normal_map_vertex_type, normal_x));
			}
			else if (dynamic_cast<GLScalarFieldDepthLayersSource *>(d_raster_source.get()))
			{
				gl.EnableVertexAttribArray(0);
				gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(scalar_field_depth_layer_vertex_type), BUFFER_OFFSET(scalar_field_depth_layer_vertex_type, x));
				gl.EnableVertexAttribArray(1);
				gl.VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(scalar_field_depth_layer_vertex_type), BUFFER_OFFSET(scalar_field_depth_layer_vertex_type, u));
				gl.EnableVertexAttribArray(2);
				gl.VertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(scalar_field_depth_layer_vertex_type), BUFFER_OFFSET(scalar_field_depth_layer_vertex_type, tangent_x));
				gl.EnableVertexAttribArray(3);
				gl.VertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(scalar_field_depth_layer_vertex_type), BUFFER_OFFSET(scalar_field_depth_layer_vertex_type, binormal_x));
				gl.EnableVertexAttribArray(4);
				gl.VertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(scalar_field_depth_layer_vertex_type), BUFFER_OFFSET(scalar_field_depth_layer_vertex_type, normal_x));
			}
			else
			{
				// Unexpected type of raster source (its derivation of GLMultiResolutionRasterSource wasn't catered for above).
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
			}
		}

		// Bind the tile vertex array (before we bind the vertex element buffer).
		gl.BindVertexArray(tile_vertices->vertex_array);

		// Bind the vertex element buffer for the current tile to the vertex array.
		// We have to do this each time we recycle (or create) a tile since the previous vertex
		// elements (indices) may not be appropriate for the current tile (due to partial boundary tiles).
		//
		// Since most tiles can share these indices (vertex elements) we store them in a map keyed on
		// the number of vertices in each dimension.
		//
		// When we draw the vertex array it will use this vertex element buffer.
		tile_vertices->num_vertex_elements =
				bind_vertex_element_buffer(gl, lod_tile.x_num_vertices, lod_tile.y_num_vertices);

		// Load the tile vertices.
		load_vertices_into_tile_vertex_buffer(gl, lod_tile, *tile_vertices);
	}

	return tile_vertices;
}


void
GPlatesOpenGL::GLMultiResolutionRaster::load_vertices_into_tile_vertex_buffer(
		GL &gl,
		const LevelOfDetailTile &lod_tile,
		TileVertices &tile_vertices)
{
	PROFILE_FUNC();

	// Total number of vertices in this tile.
	const unsigned int num_vertices_in_tile =
			lod_tile.x_num_vertices * lod_tile.y_num_vertices;


	// Allocate memory for the geo-referenced vertex positions.
	// If we're rendering surface normals then we need extra positions around the border
	// of the tile so we can calculate tangent-space frames for each tile vertex.
	std::vector<GPlatesMaths::UnitVector3D> vertex_positions;
	vertex_positions.reserve(num_vertices_in_tile);
	// Also allocate memory for the texture V coordinates since they might get clamped at the poles
	// on the first pass.
	std::vector<double> vertex_v_coords;
	vertex_v_coords.reserve(num_vertices_in_tile);

	// Set up some variables before initialising the geo-referenced vertex positions.
	const double inverse_x_num_quads = 1.0 / (lod_tile.x_num_vertices - 1);
	const double inverse_y_num_quads = 1.0 / (lod_tile.y_num_vertices - 1);
	const double x_pixels_per_quad =
			inverse_x_num_quads * (lod_tile.x_geo_end - lod_tile.x_geo_start);
	const double y_pixels_per_quad =
			inverse_y_num_quads * (lod_tile.y_geo_end - lod_tile.y_geo_start);

	// Set up some variables before initialising the vertices.
	const double u_increment_per_quad = inverse_x_num_quads * (lod_tile.u_end - lod_tile.u_start);
	const double v_increment_per_quad = inverse_y_num_quads * (lod_tile.v_end - lod_tile.v_start);

	// The texture gradient of v wrt y.
	// This is only used when y coords are clamped, *and* when y is aligned with v
	// (ie, dv/dx = du/dy = 0; eg, the regular lat/lon projection).
	// In other words, clamping only happens inside convert_pixel_coord_to_geographic_coord()
	// when y is aligned with v (it is assumed this is the only case where clamping of
	// 'grid-registered' rasters is required).
	const double dv_dy = v_increment_per_quad / y_pixels_per_quad;

	// Calculate the geo-referenced vertex positions.
	unsigned int j;
	for (j = 0; j < lod_tile.y_num_vertices; ++j)
	{
		// NOTE: The positions of the last row of vertices should
		// match up identically with the adjacent tile otherwise
		// cracks will appear in the raster along tile edges and
		// missing pixels can show up intermittently.
		const double y = (j == lod_tile.y_num_vertices - 1)
				? lod_tile.y_geo_end
				: lod_tile.y_geo_start + j * y_pixels_per_quad;

		// The 'v' texture coordinate.
		const double v = lod_tile.v_start + j * v_increment_per_quad;

		for (unsigned int i = 0; i < lod_tile.x_num_vertices; ++i)
		{
			// NOTE: The positions of the last column of vertices should
			// match up identically with the adjacent tile otherwise
			// cracks will appear in the raster along tile edges and
			// missing pixels can show up intermittently.
			const double x = (i == lod_tile.x_num_vertices - 1)
					? lod_tile.x_geo_end
					: lod_tile.x_geo_start + i * x_pixels_per_quad;

			// Convert from pixel coordinates to a position on the unit globe.
			double y_clamped = y;
			const GPlatesMaths::PointOnSphere vertex_position =
					convert_pixel_coord_to_geographic_coord(x, y, y_clamped);
			// If the y coordinate got clamped then we need to clamp the v texture coordinate also,
			// otherwise the v texture coordinate is unchanged.
			const double v_clamped = v + (y_clamped - y) * dv_dy;

			vertex_positions.push_back(vertex_position.position_vector());
			vertex_v_coords.push_back(v_clamped);
		}
	}

	// Vertex size depends on the source raster type.
	unsigned int vertex_size;
	if (dynamic_cast<GLVisualRasterSource *>(d_raster_source.get()))
	{
		vertex_size = sizeof(visual_vertex_type);
	}
	else if (dynamic_cast<GLDataRasterSource *>(d_raster_source.get()))
	{
		vertex_size = sizeof(data_vertex_type);
	}
	else if (dynamic_cast<GLNormalMapSource *>(d_raster_source.get()))
	{
		vertex_size = sizeof(normal_map_vertex_type);
	}
	else if (dynamic_cast<GLScalarFieldDepthLayersSource *>(d_raster_source.get()))
	{
		vertex_size = sizeof(scalar_field_depth_layer_vertex_type);
	}
	else
	{
		// Unexpected type of raster source (its derivation of GLMultiResolutionRasterSource wasn't catered for above).
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

		vertex_size = 0;  // Shouldn't get there, but keep compiler happy.
	}

	// Allocate memory for the vertex array.
	const unsigned int vertex_buffer_size_in_bytes = num_vertices_in_tile * vertex_size;

	// Bind vertex buffer object (before we load data into it).
	//
	// Note: Unlike the vertex *element* buffer this vertex buffer binding is not stored in vertex array object state.
	//       So we don't have to ensure a vertex array object is currently bound.
	gl.BindBuffer(GL_ARRAY_BUFFER, tile_vertices.vertex_buffer);

	// The memory is allocated directly in the vertex buffer.
	//
	// NOTE: We could use USAGE_DYNAMIC_DRAW but that is useful if updating every few frames or so.
	// In our case we typically update much less frequently than that so it's better to use USAGE_STATIC_DRAW
	// to hint to the driver to store vertices in faster video memory rather than AGP memory.
	glBufferData(
			GL_ARRAY_BUFFER,
			vertex_buffer_size_in_bytes,
			nullptr,  // Allocate memory, but not yet initialized.
			GL_STATIC_DRAW);

	// Get access to the allocated buffer.
	// NOTE: This is a write-only pointer - it might reference video memory - and cannot be read from.
	GLvoid *vertex_data_write_ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

	// If there was an error during mapping then report it and throw exception.
	if (vertex_data_write_ptr == NULL)
	{
		// Log OpenGL error - a mapped data pointer of NULL should generate an OpenGL error.
		GLUtils::check_gl_errors(GPLATES_ASSERTION_SOURCE);

		throw OpenGLException(
				GPLATES_ASSERTION_SOURCE,
				"GLMultiResolutionRaster: Failed to map OpenGL buffer object.");
	}

	//
	// Initialise the vertices
	//

	// Only needed if gradients are calculated.
	const double inv_num_texels_per_vertex = double(1 << 16) / d_num_texels_per_vertex;

	// Calculate the vertices.
	for (j = 0; j < lod_tile.y_num_vertices; ++j)
	{
		for (unsigned int i = 0; i < lod_tile.x_num_vertices; ++i)
		{
			// Get the geo-referenced vertex position.
			const GPlatesMaths::UnitVector3D &vertex_position =
					vertex_positions[i + j * lod_tile.x_num_vertices];

			// The possibly clamped 'v' texture coordinate.
			const double v = vertex_v_coords[i + j * lod_tile.x_num_vertices];

			// The 'u' texture coordinate.
			const double u = lod_tile.u_start + i * u_increment_per_quad;

			if (dynamic_cast<GLVisualRasterSource *>(d_raster_source.get()))
			{
				// Write the vertex to vertex buffer memory.
				visual_vertex_type *vertex = static_cast<visual_vertex_type *>(vertex_data_write_ptr);

				// Write directly into (un-initialised) memory in the vertex buffer.
				// NOTE: We cannot read this memory.
				new (vertex) visual_vertex_type(vertex_position, u, v);

				// Advance the vertex buffer write pointer.
				vertex_data_write_ptr = vertex + 1;
			}
			else if (dynamic_cast<GLDataRasterSource *>(d_raster_source.get()))
			{
				// Write the vertex to vertex buffer memory.
				data_vertex_type *vertex = static_cast<data_vertex_type *>(vertex_data_write_ptr);

				// Write directly into (un-initialised) memory in the vertex buffer.
				// NOTE: We cannot read this memory.
				new (vertex) data_vertex_type(vertex_position, u, v);

				// Advance the vertex buffer write pointer.
				vertex_data_write_ptr = vertex + 1;
			}
			else if (dynamic_cast<GLNormalMapSource *>(d_raster_source.get()))
			{
				// Get the adjacent vertex positions.
				GPlatesMaths::UnitVector3D vertex_position01 = vertex_position;
				GPlatesMaths::UnitVector3D vertex_position21 = vertex_position;
				GPlatesMaths::UnitVector3D vertex_position10 = vertex_position;
				GPlatesMaths::UnitVector3D vertex_position12 = vertex_position;
				bool has_vertex_position01, has_vertex_position21, has_vertex_position10, has_vertex_position12;
				get_adjacent_vertex_positions(
						vertex_position01, has_vertex_position01,
						vertex_position21, has_vertex_position21,
						vertex_position10, has_vertex_position10,
						vertex_position12, has_vertex_position12,
						lod_tile, vertex_positions, i, j, x_pixels_per_quad, y_pixels_per_quad);

				// Calculate the tangent-space frame of the current vertex.
				const TangentSpaceFrame tangent_space_frame = calculate_tangent_space_frame(
						vertex_position,
						vertex_position01, vertex_position21, vertex_position10, vertex_position12);

				// Normal map vertex pointer into vertex buffer memory.
				normal_map_vertex_type *vertex =
						static_cast<normal_map_vertex_type *>(vertex_data_write_ptr);

				// Write directly into (un-initialised) memory in the vertex buffer.
				// NOTE: We cannot read this memory.
				new (vertex) normal_map_vertex_type(
						vertex_position,
						u, v,
						tangent_space_frame.tangent,
						tangent_space_frame.binormal,
						tangent_space_frame.normal);

				// Advance the vertex buffer write pointer.
				vertex_data_write_ptr = vertex + 1;
			}
			else if (dynamic_cast<GLScalarFieldDepthLayersSource *>(d_raster_source.get()))
			{
				// Get the adjacent vertex positions.
				GPlatesMaths::UnitVector3D vertex_position01 = vertex_position;
				GPlatesMaths::UnitVector3D vertex_position21 = vertex_position;
				GPlatesMaths::UnitVector3D vertex_position10 = vertex_position;
				GPlatesMaths::UnitVector3D vertex_position12 = vertex_position;
				bool has_vertex_position01, has_vertex_position21, has_vertex_position10, has_vertex_position12;
				get_adjacent_vertex_positions(
						vertex_position01, has_vertex_position01,
						vertex_position21, has_vertex_position21,
						vertex_position10, has_vertex_position10,
						vertex_position12, has_vertex_position12,
						lod_tile, vertex_positions, i, j, x_pixels_per_quad, y_pixels_per_quad);

				// Per-texel distance vector of constant 'u' and 'v'.
				GPlatesMaths::Vector3D delta_u =
						GPlatesMaths::Vector3D(vertex_position21) - GPlatesMaths::Vector3D(vertex_position01);
				GPlatesMaths::Vector3D delta_v =
						GPlatesMaths::Vector3D(vertex_position12) - GPlatesMaths::Vector3D(vertex_position10);

				// The inverse num texels makes the inverse distance a per-texel measure.
				if (has_vertex_position21 && has_vertex_position01)
				{
					delta_u = 0.5 * inv_num_texels_per_vertex * delta_u;
				}
				else // distance vector covers one vertex edge instead of two...
				{
					delta_u = inv_num_texels_per_vertex * delta_u;
				}
				if (has_vertex_position12 && has_vertex_position10)
				{
					delta_v = 0.5 * inv_num_texels_per_vertex * delta_v;
				}
				else // distance vector covers one vertex edge instead of two...
				{
					delta_v = inv_num_texels_per_vertex * delta_v;
				}

				// Per-texel inverse distance vector of constant 'u' and 'v'.
				// Using inverse magnitude squared since need one inverse magnitude is to normalise and
				// the other inverse magnitude is to generate the inverse distance part of gradient calculation.
				GPlatesMaths::Vector3D inv_delta_u_tangent(0,0,0);
				GPlatesMaths::Vector3D inv_delta_v_binormal(0,0,0);
				if (!are_almost_exactly_equal(delta_u.magSqrd(), 0))
				{
					inv_delta_u_tangent = (1.0 / delta_u.magSqrd()) * delta_u;
				}
				if (!are_almost_exactly_equal(delta_v.magSqrd(), 0))
				{
					inv_delta_v_binormal = (1.0 / delta_v.magSqrd()) * delta_v;
				}

				// The surface normal points outwards from the sphere regardless of tangent and binormal directions.
				const GPlatesMaths::Vector3D normal(vertex_position);

				// Scalar/gradient map vertex pointer into vertex buffer memory.
				scalar_field_depth_layer_vertex_type *vertex =
						static_cast<scalar_field_depth_layer_vertex_type *>(vertex_data_write_ptr);

				// Write directly into (un-initialised) memory in the vertex buffer.
				// NOTE: We cannot read this memory.
				new (vertex) scalar_field_depth_layer_vertex_type(
						vertex_position,
						u, v,
						inv_delta_u_tangent, inv_delta_v_binormal, normal);

				// Advance the vertex buffer write pointer.
				vertex_data_write_ptr = vertex + 1;
			}
			else
			{
				// Unexpected type of raster source (its derivation of GLMultiResolutionRasterSource wasn't catered for above).
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
			}
		}
	}

	if (!glUnmapBuffer(GL_ARRAY_BUFFER))
	{
		// Check OpenGL errors in case glUnmapBuffer used incorrectly.
		GLUtils::check_gl_errors(GPLATES_ASSERTION_SOURCE);

		// Otherwise the buffer contents have been corrupted, so just emit a warning.
		qWarning() << "GLMultiResolutionRaster: Failed to unmap OpenGL buffer object. "
				"Buffer object contents have been corrupted (such as an ALT+TAB switch between applications).";
	}
}


void
GPlatesOpenGL::GLMultiResolutionRaster::get_adjacent_vertex_positions(
		GPlatesMaths::UnitVector3D &vertex_position01,
		bool &has_vertex_position01,
		GPlatesMaths::UnitVector3D &vertex_position21,
		bool &has_vertex_position21,
		GPlatesMaths::UnitVector3D &vertex_position10,
		bool &has_vertex_position10,
		GPlatesMaths::UnitVector3D &vertex_position12,
		bool &has_vertex_position12,
		const LevelOfDetailTile &lod_tile,
		const std::vector<GPlatesMaths::UnitVector3D> &vertex_positions,
		const unsigned int i,
		const unsigned int j,
		const double &x_pixels_per_quad,
		const double &y_pixels_per_quad)
{
	//
	// Calculate the vertex positions above/below/left/right of the current vertex.
	//

	if (i != 0) // vertex in tile
	{
		vertex_position01 = vertex_positions[i - 1 + j * lod_tile.x_num_vertices];
		has_vertex_position01 = true;
	}
	else if (lod_tile.x_geo_start != 0) // vertex outside tile but in raster
	{
		vertex_position01 = convert_pixel_coord_to_geographic_coord(
				lod_tile.x_geo_start - x_pixels_per_quad,
				lod_tile.y_geo_start + j * y_pixels_per_quad).position_vector();
		has_vertex_position01 = true;
	}
	else // vertex outside raster - just use raster edge
	{
		has_vertex_position01 = false;
	}

	if (i != lod_tile.x_num_vertices - 1) // vertex in tile
	{
		vertex_position21 = vertex_positions[i + 1 + j * lod_tile.x_num_vertices];
		has_vertex_position21 = true;
	}
	else if (lod_tile.x_geo_end != d_raster_width) // vertex outside tile but in raster
	{
		vertex_position21 = convert_pixel_coord_to_geographic_coord(
				lod_tile.x_geo_end + x_pixels_per_quad,
				lod_tile.y_geo_start + j * y_pixels_per_quad).position_vector();
		has_vertex_position21 = true;
	}
	else // vertex outside raster - just use raster edge
	{
		has_vertex_position21 = false;
	}

	if (j != 0) // vertex in tile
	{
		vertex_position10 = vertex_positions[i + (j - 1) * lod_tile.x_num_vertices];
		has_vertex_position10 = true;
	}
	else if (lod_tile.y_geo_start != 0) // vertex outside tile but in raster
	{
		vertex_position10 = convert_pixel_coord_to_geographic_coord(
							lod_tile.x_geo_start + i * x_pixels_per_quad,
							lod_tile.y_geo_start - y_pixels_per_quad).position_vector();
		has_vertex_position10 = true;
	}
	else // vertex outside raster - just use raster edge
	{
		has_vertex_position10 = false;
	}

	if (j != lod_tile.y_num_vertices - 1) // vertex in tile
	{
		vertex_position12 = vertex_positions[i + (j + 1) * lod_tile.x_num_vertices];
		has_vertex_position12 = true;
	}
	else if (lod_tile.y_geo_end != d_raster_height) // vertex outside tile but in raster
	{
		vertex_position12 = convert_pixel_coord_to_geographic_coord(
							lod_tile.x_geo_start + i * x_pixels_per_quad,
							lod_tile.y_geo_end + y_pixels_per_quad).position_vector();
		has_vertex_position12 = true;
	}
	else // vertex outside raster - just use raster edge
	{
		has_vertex_position12 = false;
	}
}


GPlatesOpenGL::GLMultiResolutionRaster::TangentSpaceFrame
GPlatesOpenGL::GLMultiResolutionRaster::calculate_tangent_space_frame(
		const GPlatesMaths::UnitVector3D &vertex_position,
		const GPlatesMaths::UnitVector3D &vertex_position01,
		const GPlatesMaths::UnitVector3D &vertex_position21,
		const GPlatesMaths::UnitVector3D &vertex_position10,
		const GPlatesMaths::UnitVector3D &vertex_position12)
{
	// Calculate the tangent-space frame of the specified vertex.
	//
	// NOTE: Depending on how the raster is geo-referenced onto the globe we could get
	// a left-handed or right-handed tangent-space coordinate system. In other words you
	// could imagine flipping a raster about one of its two geo-referenced coordinates
	// and this would change from left (or right) coordinate system to right (or left).
	//
	// NOTE: The tangent-space frame is not necessarily orthogonal and so the tangent and
	// binormal can be non-orthogonal to each other (but still orthogonal to the normal).
	// This depends on the geo-referencing and is fine - the shader program will still
	// normalise each (world-space) surface normal pixel.

	boost::optional<GPlatesMaths::UnitVector3D> tangent;
	boost::optional<GPlatesMaths::UnitVector3D> binormal;
	// The surface normal points outwards from the sphere regardless of tangent and binormal.
	const GPlatesMaths::UnitVector3D &normal = vertex_position;

	// Vector of constant 'u' coming into current vertex.
	GPlatesMaths::Vector3D delta_u10 =
			GPlatesMaths::Vector3D(vertex_position) - GPlatesMaths::Vector3D(vertex_position01);
	// Vector of constant 'u' going out of current vertex.
	GPlatesMaths::Vector3D delta_u21 =
			GPlatesMaths::Vector3D(vertex_position21) - GPlatesMaths::Vector3D(vertex_position);
	// Normalise, unless the length is zero (in which case it won't contribute).
	if (!are_almost_exactly_equal(delta_u10.magSqrd(), 0))
	{
		delta_u10 = (1.0 / delta_u10.magnitude()) * delta_u10;
	}
	if (!are_almost_exactly_equal(delta_u21.magSqrd(), 0))
	{
		delta_u21 = (1.0 / delta_u21.magnitude()) * delta_u21;
	}

	// The tangent is the average of the vectors of constant 'u'.
	GPlatesMaths::Vector3D delta_u = delta_u21 + delta_u10;
	// Normalise, unless the length is zero (in which case tangent could not be determined).
	if (!are_almost_exactly_equal(delta_u.magSqrd(), 0))
	{
		tangent = delta_u.get_normalisation();
	}

	// Vector of constant 'v' coming into current vertex.
	GPlatesMaths::Vector3D delta_v10 =
			GPlatesMaths::Vector3D(vertex_position) - GPlatesMaths::Vector3D(vertex_position10);
	// Vector of constant 'v' going out of current vertex.
	GPlatesMaths::Vector3D delta_v21 =
			GPlatesMaths::Vector3D(vertex_position12) - GPlatesMaths::Vector3D(vertex_position);
	// Normalise, unless the length is zero (in which case it won't contribute).
	if (!are_almost_exactly_equal(delta_v10.magSqrd(), 0))
	{
		delta_v10 = (1.0 / delta_v10.magnitude()) * delta_v10;
	}
	if (!are_almost_exactly_equal(delta_v21.magSqrd(), 0))
	{
		delta_v21 = (1.0 / delta_v21.magnitude()) * delta_v21;
	}

	// The tangent is the average of the vectors of constant 'v'.
	GPlatesMaths::Vector3D delta_v = delta_v21 + delta_v10;
	// Normalise, unless the length is zero (in which case binormal could not be determined).
	if (!are_almost_exactly_equal(delta_v.magSqrd(), 0))
	{
		binormal = delta_v.get_normalisation();
	}

	// If both tangent and binormal could not be determined then generate any arbitrary
	// orthonormal frame using 'normal'. This could happen near the north or south pole.
	// Typically the height pixels should all be the same when they're all bunched near
	// a pole like that and so the surface normals should all be normal to the surface
	// (ie, no tangent/binormal components) and hence the arbitrary tangent/binormal frame
	// won't get used in the shader program when converting surface normals to world-space.
	if (!tangent && !binormal)
	{
		tangent = generate_perpendicular(normal);
		// Cross-product produces very close to unit vector but not good enough for
		// UnitVector3D constructor so using 'get_normalisation()' instead.
		binormal = cross(normal, tangent.get()).get_normalisation();
	}
	else if (!tangent)
	{
		// Cross-product produces very close to unit vector but not good enough for
		// UnitVector3D constructor so using 'get_normalisation()' instead.
		tangent = cross(binormal.get(), normal).get_normalisation();
	}
	else if (!binormal)
	{
		// Cross-product produces very close to unit vector but not good enough for
		// UnitVector3D constructor so using 'get_normalisation()' instead.
		binormal = cross(normal, tangent.get()).get_normalisation();
	}

	return TangentSpaceFrame(tangent.get(), binormal.get(), normal);
}


GPlatesOpenGL::GLMultiResolutionRaster::texels_per_vertex_fixed_point_type
GPlatesOpenGL::GLMultiResolutionRaster::calculate_num_texels_per_vertex()
{
	// We're calculating the texel sampling density for the entire raster.
	const unsigned int x_geo_start = 0;
	const unsigned int x_geo_end = d_raster_width;
	const unsigned int y_geo_start = 0;
	const unsigned int y_geo_end = d_raster_height;

	// Centre point of the raster.
	const double x_geo_centre = 0.5 * (x_geo_start + x_geo_end);
	const double y_geo_centre = 0.5 * (y_geo_start + y_geo_end);

	// The nine boundary points including corners and midpoints and one centre point.
	const GPlatesMaths::PointOnSphere sample_points[3][3] =
	{
		{
			convert_pixel_coord_to_geographic_coord(x_geo_start, y_geo_start),
			convert_pixel_coord_to_geographic_coord(x_geo_centre, y_geo_start),
			convert_pixel_coord_to_geographic_coord(x_geo_end, y_geo_start)
		},
		{
			convert_pixel_coord_to_geographic_coord(x_geo_start, y_geo_centre),
			convert_pixel_coord_to_geographic_coord(x_geo_centre, y_geo_centre),
			convert_pixel_coord_to_geographic_coord(x_geo_end, y_geo_centre)
		},
		{
			convert_pixel_coord_to_geographic_coord(x_geo_start, y_geo_end),
			convert_pixel_coord_to_geographic_coord(x_geo_centre, y_geo_end),
			convert_pixel_coord_to_geographic_coord(x_geo_end, y_geo_end)
		}
	};

	// Calculate the maximum angle spanned by the raster in the x direction.
	GPlatesMaths::real_t x_min_half_span = 1;

	// Iterate over the half segments and calculate dot products in the x direction.
	for (unsigned int i = 0; i < 3; ++i)
	{
		for (unsigned int j = 0; j < 2; ++j)
		{
			const GPlatesMaths::real_t x_half_span = dot(
					sample_points[i][j].position_vector(),
					sample_points[i][j+1].position_vector());
			if (x_half_span < x_min_half_span)
			{
				x_min_half_span = x_half_span;
			}
		}
	}

	// Calculate the maximum angle spanned by the raster in the y direction.
	GPlatesMaths::real_t y_min_half_span = 1;

	// Iterate over the half segments and calculate dot products in the y direction.
	for (unsigned int j = 0; j < 3; ++j)
	{
		for (unsigned int i = 0; i < 2; ++i)
		{
			const GPlatesMaths::real_t y_half_span = dot(
					sample_points[i][j].position_vector(),
					sample_points[i+1][j].position_vector());
			if (y_half_span < y_min_half_span)
			{
				y_min_half_span = y_half_span;
			}
		}
	}

	// Convert from dot product to angle.
	const GPlatesMaths::real_t x_max_spanned_angle_in_radians = 2 * acos(x_min_half_span);
	const GPlatesMaths::real_t y_max_spanned_angle_in_radians = 2 * acos(y_min_half_span);

	// Determine number of quads (segments) along each edge.
	const GPlatesMaths::real_t x_num_quads_based_on_distance_real =
			GPlatesMaths::convert_rad_to_deg(x_max_spanned_angle_in_radians) /
					MAX_ANGLE_IN_DEGREES_BETWEEN_VERTICES;
	const GPlatesMaths::real_t y_num_quads_based_on_distance_real =
			GPlatesMaths::convert_rad_to_deg(y_max_spanned_angle_in_radians) /
					MAX_ANGLE_IN_DEGREES_BETWEEN_VERTICES;

	// Determine the texel-per-vertex density along each edge.
	const GPlatesMaths::real_t x_num_texels_per_vertex =
			GPlatesMaths::real_t(d_raster_width) / x_num_quads_based_on_distance_real;
	const GPlatesMaths::real_t y_num_texels_per_vertex =
			GPlatesMaths::real_t(d_raster_height) / y_num_quads_based_on_distance_real;

	// Choose the minimum number of texels per vertex.
	// If the raster is very low resolution then it will need more vertices per texel to keep
	// the mesh tessellated finely enough (so it looks smooth and curve when drawn on the globe).
	const GPlatesMaths::real_t num_texels_per_vertex =
			(x_num_texels_per_vertex.dval() < y_num_texels_per_vertex.dval())
			? x_num_texels_per_vertex
			: y_num_texels_per_vertex;

	// Convert to 16:16 fixed-point format.
	texels_per_vertex_fixed_point_type num_texels_per_vertex_fixed_point = MAX_NUM_TEXELS_PER_VERTEX << 16;
	if (num_texels_per_vertex.dval() < MAX_NUM_TEXELS_PER_VERTEX)
	{
		num_texels_per_vertex_fixed_point =
				static_cast<texels_per_vertex_fixed_point_type>(
						num_texels_per_vertex.dval() * (1 << 16));

		// If, for some reason, the floating-point value is so low that we don't have enough
		// fixed-point precision (16-bits) to capture it then set it to the lowest fixed-point value.
		if (num_texels_per_vertex_fixed_point == 0)
		{
			num_texels_per_vertex_fixed_point = 1;
		}
	}

	//qDebug() << "texels/vertex: " << num_texels_per_vertex_fixed_point / 65536.0;

	return num_texels_per_vertex_fixed_point;
}


void
GPlatesOpenGL::GLMultiResolutionRaster::compile_link_shader_program(
		GL &gl)
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Vertex shader source.
	GLShaderSource vertex_shader_source;
	vertex_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);

	// Fragment shader source.
	GLShaderSource fragment_shader_source;
	fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);

	//
	// Compile/link different shader programs depending on the source raster:
	// * GLDataRasterSource
	// * GLNormalMapSource
	// * GLScalarFieldDepthLayersSource
	// * GLVisualRasterSource
	//

	if (dynamic_cast<GLDataRasterSource *>(d_raster_source.get()))
	{
		vertex_shader_source.add_code_segment(RENDER_DATA_RASTER_VERTEX_SHADER_SOURCE);
		fragment_shader_source.add_code_segment(RENDER_DATA_RASTER_FRAGMENT_SHADER_SOURCE);
	}
	else if (dynamic_cast<GLNormalMapSource *>(d_raster_source.get()))
	{
		// Shader source for converting tangent-space surface normals to world-space so they can be
		// later captured in a cube raster (which is decoupled from the raster geo-referencing or tangent-space).
		vertex_shader_source.add_code_segment(RENDER_NORMAL_MAP_RASTER_VERTEX_SHADER_SOURCE);
		fragment_shader_source.add_code_segment(RENDER_NORMAL_MAP_RASTER_FRAGMENT_SHADER_SOURCE);
	}
	else if (dynamic_cast<GLScalarFieldDepthLayersSource *>(d_raster_source.get()))
	{
		vertex_shader_source.add_code_segment(RENDER_SCALAR_FIELD_DEPTH_LAYERS_RASTER_VERTEX_SHADER_SOURCE);
		fragment_shader_source.add_code_segment(RENDER_SCALAR_FIELD_DEPTH_LAYERS_RASTER_FRAGMENT_SHADER_SOURCE);
	}
	else if (dynamic_cast<GLVisualRasterSource *>(d_raster_source.get()))
	{
		vertex_shader_source.add_code_segment(RENDER_VISUAL_RASTER_VERTEX_SHADER_SOURCE);
		fragment_shader_source.add_code_segment(RENDER_VISUAL_RASTER_FRAGMENT_SHADER_SOURCE);
	}
	else
	{
		// Unexpected type of raster source (its derivation of GLMultiResolutionRasterSource wasn't catered for above).
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
	}

	// Compile vertex shader.
	GLShader::shared_ptr_type vertex_shader = GLShader::create(gl, GL_VERTEX_SHADER);
	vertex_shader->shader_source(vertex_shader_source);
	vertex_shader->compile_shader();

	// Compile fragment shader.
	GLShader::shared_ptr_type fragment_shader = GLShader::create(gl, GL_FRAGMENT_SHADER);
	fragment_shader->shader_source(fragment_shader_source);
	fragment_shader->compile_shader();

	// Link vertex-fragment program.
	d_render_raster_program->attach_shader(vertex_shader);
	d_render_raster_program->attach_shader(fragment_shader);
	d_render_raster_program->link_program();

	gl.UseProgram(d_render_raster_program);

	// All versions of shader program use "source_texture_sampler" as the source texture sampler,
	// and it is always texture unit 0.
	glUniform1i(
			d_render_raster_program->get_uniform_location("source_texture_sampler"),
			0/*texture unit*/);
}


void
GPlatesOpenGL::GLMultiResolutionRaster::initialise_level_of_detail_pyramid()
{
	// The dimension of texels that contribute to a level-of-detail
	// (starting with the highest resolution level-of-detail).
	unsigned int lod_texel_width = d_raster_width;
	unsigned int lod_texel_height = d_raster_height;

	// Generate the levels of detail starting with the
	// highest resolution (original raster) at level 0.
	for (unsigned int lod_level = 0; ; ++lod_level)
	{
		// Create a level-of-detail.
		LevelOfDetail::non_null_ptr_type level_of_detail = create_level_of_detail(lod_level);

		// Add to our level-of-detail pyramid.
		d_level_of_detail_pyramid.push_back(level_of_detail);

		// Keep generating coarser level-of-details until the width and height
		// fit within a square tile of size:
		//   'tile_texel_dimension' x 'tile_texel_dimension'
		if (lod_texel_width <= d_tile_texel_dimension &&
			lod_texel_height <= d_tile_texel_dimension)
		{
			break;
		}

		// Get the raster dimensions of the next level-of-detail.
		// The '+1' is to ensure the texels of the next level-of-detail
		// cover the texels of the current level-of-detail.
		// This can mean that the next level-of-detail texels actually
		// cover a slightly larger area on the globe than the current level-of-detail.
		//
		// For example:
		// Level 0: 5x5
		// Level 1: 3x3 (covers equivalent of 6x6 level 0 texels)
		// Level 2: 2x2 (covers equivalent of 4x4 level 1 texels or 8x8 level 0 texels)
		// Level 3: 1x1 (covers same area as level 2)
		//
		lod_texel_width = (lod_texel_width + 1) / 2;
		lod_texel_height = (lod_texel_height + 1) / 2;
	}
}


GPlatesOpenGL::GLMultiResolutionRaster::LevelOfDetail::non_null_ptr_type
GPlatesOpenGL::GLMultiResolutionRaster::create_level_of_detail(
		const unsigned int lod_level)
{
	// Allocate on the heap to avoid lots of copying when it's put in a std::vector.
	LevelOfDetail::non_null_ptr_type level_of_detail = LevelOfDetail::create(lod_level);

	/*
	 * Can generate OBB tree by starting at root node and dividing all tiles into two groups of tiles
	 * such that each group is still a rectangular arrangement of tiles and recursively descending.
	 * For example, if we start out with 5.6 x 10.9 tiles then take the dimension with
	 * the largest number of tiles and divide that, for example:
	 *                           5.6x10.8
	 *                        /           \
	 *                 5.6x5                 5.6x5.8
	 *              /       \               /      \
	 *         3x5           2.6x5      5.6x3       5.6x2.8
	 *        /   \         /    \      /   \       /     \
	 *     3x3     3x2  2.6x3  2.6x2  3x3  2.6x3  3x2.8   2.6x2.8
	 *
	 * The second level-of-detail looks like this:
	 *
	 *                           2.8x5.4
	 *                        /           \
	 *                 2.8x3                 2.8x2.4
	 *              /       \               /      \
	 *         1x3           1.8x3      1x2.4       1.8x2.4
	 *
	 * ...and hence needs a different OBB tree since the tiles at the leaves of the tree
	 * are a different size for each level-of-detail and so the partitioning line between
	 * two child nodes (of any parent internal node) will differ.
	 */

	// The root OBB tree node covers the entire raster.
	const unsigned int x_geo_start = 0;
	const unsigned int x_geo_end = d_raster_width;
	const unsigned int y_geo_start = 0;
	const unsigned int y_geo_end = d_raster_height;

	// Recursively create an OBB tree starting at the root.
	level_of_detail->obb_tree_root_node_index = create_obb_tree(
			*level_of_detail, x_geo_start, x_geo_end, y_geo_start, y_geo_end);

	return level_of_detail;
}


std::size_t
GPlatesOpenGL::GLMultiResolutionRaster::create_obb_tree(
		LevelOfDetail &level_of_detail,
		const unsigned int x_geo_start,
		const unsigned int x_geo_end,
		const unsigned int y_geo_start,
		const unsigned int y_geo_end)
{
	// Level-of-detail.
	const unsigned int lod_level = level_of_detail.lod_level;

	// Texels in this level-of-detail have dimensions 'lod_factor' times larger than the
	// original raster pixels when projected on the globe (they cover a larger area on the globe).
	const unsigned int lod_factor = 1 << lod_level;

	// The size of a tile (at the current level-of-detail) in units of pixels of
	// the original raster. Pixels and geo coordinates are the same.
	const unsigned int tile_geo_dimension = d_tile_texel_dimension * lod_factor;

	// The start x coordinate should be an integer multiple of the tile dimension.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			(x_geo_start % tile_geo_dimension) == 0,
			GPLATES_ASSERTION_SOURCE);
	if (d_raster_scanline_order == TOP_TO_BOTTOM)
	{
		// The start y coordinate should be an integer multiple of the tile dimension.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				(y_geo_start % tile_geo_dimension) == 0,
				GPLATES_ASSERTION_SOURCE);
	}
	else // BOTTOM_TO_TOP ...
	{
		// The raster height minus the end y coordinate should be also be an integer multiple.
		// The inverted y is a result of the geo coordinates starting at the top-left but
		// the raster data starting at the bottom-left.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				((d_raster_height - y_geo_end) % tile_geo_dimension) == 0,
				GPLATES_ASSERTION_SOURCE);
	}

	// The width and height of this node.
	const unsigned int node_geo_width = x_geo_end - x_geo_start;
	const unsigned int node_geo_height = y_geo_end - y_geo_start;

	// Determine if this node should be a leaf node (referencing a tile).
	if (node_geo_width <= tile_geo_dimension &&
		node_geo_height <= tile_geo_dimension)
	{
		// Return the node index so the parent node can reference this node.
		return create_obb_tree_leaf_node(level_of_detail,
				x_geo_start, x_geo_end, y_geo_start, y_geo_end);
	}

	//
	// When we reach the leaf nodes of the tree we can calculate the tile OBBs and then
	// as we traverse back up the tree (returning from recursion) we can generate the OBBs
	// of the internal nodes from the OBBs of child nodes (or optionally for a possibly
	// tighter fit, from the OBBs of all the tiles bounded by each internal node).
	//

	// Indices of child OBB tree nodes.
	std::size_t child_node_indices[2];

	// Divide this node into two child nodes along the raster x or y direction
	// depending on whether this node is longer (in units of pixels) along
	// the x or y direction.
	if (node_geo_width > node_geo_height)
	{
		// Divide along the x direction.
		//
		// Determine how many tiles (at the current level-of-detail) to
		// give to each child node. Round up the number of tiles (possibly non-integer)
		// covered by this node and then divide by two and round down (truncate) -
		// this gives the most even balance across the two child nodes.
		const unsigned int num_tiles_in_left_child =
				(node_geo_width + tile_geo_dimension - 1) / tile_geo_dimension / 2;

		// Left child node.
		child_node_indices[0] = create_obb_tree(
				level_of_detail,
				x_geo_start,
				x_geo_start + num_tiles_in_left_child * tile_geo_dimension,
				y_geo_start,
				y_geo_end);

		// 'node_geo_width' is greater than 'tile_geo_dimension' so we should
		// have texels remaining for the right child node.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				x_geo_end > x_geo_start + num_tiles_in_left_child * tile_geo_dimension,
				GPLATES_ASSERTION_SOURCE);
		// Right child node.
		child_node_indices[1] = create_obb_tree(
				level_of_detail,
				x_geo_start + num_tiles_in_left_child * tile_geo_dimension,
				x_geo_end,
				y_geo_start,
				y_geo_end);
	}
	else
	{
		// Divide along the y direction.
		//
		// Determine how many tiles (at the current level-of-detail) to
		// give to each child node. Round up the number of tiles (possibly non-integer)
		// covered by this node and then divide by two and round down (truncate) -
		// this gives the most even balance across the two child nodes.

		if (d_raster_scanline_order == TOP_TO_BOTTOM)
		{
			const unsigned int num_tiles_in_top_child =
					(node_geo_height + tile_geo_dimension - 1) / tile_geo_dimension / 2;

			// Top child node.
			child_node_indices[0] = create_obb_tree(
					level_of_detail,
					x_geo_start,
					x_geo_end,
					y_geo_start,
					y_geo_start + num_tiles_in_top_child * tile_geo_dimension);

			// 'node_geo_height' is greater than 'tile_geo_dimension' so we should
			// have texels remaining for the bottom child node.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					y_geo_end > y_geo_start + num_tiles_in_top_child * tile_geo_dimension,
					GPLATES_ASSERTION_SOURCE);
			// Bottom child node.
			child_node_indices[1] = create_obb_tree(
					level_of_detail,
					x_geo_start,
					x_geo_end,
					y_geo_start + num_tiles_in_top_child * tile_geo_dimension,
					y_geo_end);
		}
		else // BOTTOM_TO_TOP ...
		{
			const unsigned int num_tiles_in_bottom_child =
					(node_geo_height + tile_geo_dimension - 1) / tile_geo_dimension / 2;

			// 'node_geo_height' is greater than 'tile_geo_dimension' so we should
			// have texels remaining for the top child node.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					y_geo_start < y_geo_end - num_tiles_in_bottom_child * tile_geo_dimension,
					GPLATES_ASSERTION_SOURCE);
			// Top child node.
			child_node_indices[0] = create_obb_tree(
					level_of_detail,
					x_geo_start,
					x_geo_end,
					y_geo_start,
					y_geo_end - num_tiles_in_bottom_child * tile_geo_dimension);

			// Bottom child node.
			child_node_indices[1] = create_obb_tree(
					level_of_detail,
					x_geo_start,
					x_geo_end,
					y_geo_end - num_tiles_in_bottom_child * tile_geo_dimension,
					y_geo_end);
		}
	}

	// Each OBB in the tree has one axis oriented radially outward from the globe at the
	// centre point of its bounding domain as this should generate the tightest bounding box.
	const double x_geo_centre = 0.5 * (x_geo_start + x_geo_end);
	const double y_geo_centre = 0.5 * (y_geo_start + y_geo_end);

	GLIntersect::OrientedBoundingBoxBuilder obb_builder =
			create_oriented_bounding_box_builder(x_geo_centre, y_geo_centre);

	// Expand the oriented bounding box to include the child node bounding boxes.
	obb_builder.add(
			level_of_detail.get_obb_tree_node(child_node_indices[0]).bounding_box);
	obb_builder.add(
			level_of_detail.get_obb_tree_node(child_node_indices[1]).bounding_box);

	// Create an OBB tree node.
	LevelOfDetail::OBBTreeNode obb_node(
			obb_builder.get_oriented_bounding_box(),
			false/*is_leaf_node*/);

	// Set the child node indices.
	obb_node.child_node_indices[0] = child_node_indices[0];
	obb_node.child_node_indices[1] = child_node_indices[1];

	// Add the node to the level-of-detail and get it's array index.
	const std::size_t obb_node_index = level_of_detail.obb_tree_nodes.size();
	level_of_detail.obb_tree_nodes.push_back(obb_node);

	// Return the node index so the parent node can reference this node.
	return obb_node_index;
}


std::size_t
GPlatesOpenGL::GLMultiResolutionRaster::create_obb_tree_leaf_node(
		LevelOfDetail &level_of_detail,
		const unsigned int x_geo_start,
		const unsigned int x_geo_end,
		const unsigned int y_geo_start,
		const unsigned int y_geo_end)
{
	// Create the level-of-detail tile that this OBB tree leaf node will reference.
	const tile_handle_type lod_tile_handle = create_level_of_detail_tile(
			level_of_detail, x_geo_start, x_geo_end, y_geo_start, y_geo_end);

	// Get the level-of-detail tile structure just created.
	const LevelOfDetailTile &lod_tile = *d_tiles[lod_tile_handle];

	// Create an oriented bounding box around the vertices of the level-of-detail tile.
	const GLIntersect::OrientedBoundingBox obb = bound_level_of_detail_tile(lod_tile);

	// Get the maximum size of any texel in the level-of-detail tile.
	// We only really need to do this for the highest resolution level because
	// the maximum texel size of the lower resolution levels will be very close to a
	// power-of-two factor of the highest resolution level (not exactly a power-of-two
	// because warping due to the map projection but it'll be close enough for our
	// purpose of level-of-detail selection).
	if (level_of_detail.lod_level == 0)
	{
		const float max_texel_size_on_unit_sphere =
				calc_max_texel_size_on_unit_sphere(level_of_detail.lod_level, lod_tile);

		// The maximum texel size for the entire original raster is the maximum texel of all
		// its highest resolution tiles.
		if (max_texel_size_on_unit_sphere > d_max_highest_resolution_texel_size_on_unit_sphere)
		{
			d_max_highest_resolution_texel_size_on_unit_sphere = max_texel_size_on_unit_sphere;
		}
	}

	// Create an OBB tree node.
	LevelOfDetail::OBBTreeNode obb_node(obb, true/*is_leaf_node*/);

	// Set the level-of-detail tile for this OBB node.
	obb_node.tile = lod_tile_handle;

	// Add the node to the level-of-detail and get it's array index.
	const std::size_t obb_node_index = level_of_detail.obb_tree_nodes.size();
	level_of_detail.obb_tree_nodes.push_back(obb_node);

	// Return the node index so the parent node can reference this node.
	return obb_node_index;
}


GPlatesOpenGL::GLMultiResolutionRaster::tile_handle_type
GPlatesOpenGL::GLMultiResolutionRaster::create_level_of_detail_tile(
		LevelOfDetail &level_of_detail,
		const unsigned int x_geo_start,
		const unsigned int x_geo_end,
		const unsigned int y_geo_start,
		const unsigned int y_geo_end)
{
	// Level-of-detail.
	const unsigned int lod_level = level_of_detail.lod_level;

	// Texels in this level-of-detail have dimensions 'lod_factor' times larger than the
	// original raster pixels when projected on the globe (they cover a larger area on the globe).
	const unsigned int lod_factor = 1 << lod_level;

	// The size of a tile (at the current level-of-detail) in units of pixels of
	// the original raster. Pixels and geo coordinates are the same.
	const unsigned int tile_geo_dimension = d_tile_texel_dimension * lod_factor;

	//
	// In each tile store enough information to be able to generate the
	// vertex and texture data as needed when rendering the raster.
	//

	// Make sure neighbouring tiles, of the same resolution level, have exactly
	// matching boundaries to avoid cracks appearing between adjacent tiles.
	// We do this by making the corner geo (pixel) coordinates of the tile match those
	// of adjacent tiles - this is no problem since we're using integer geo coordinates.
	// They will get converted to floating-point when georeferenced but as long as they go
	// through the same code path for all tiles then the final positions of the unit sphere
	// should match up identically (ie, bitwise equality of the floating-point xyz coordinates).

	// The start of the tile should be inside the raster.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			x_geo_start < d_raster_width && y_geo_start < d_raster_height,
			GPLATES_ASSERTION_SOURCE);

	// The start x coordinate should be an integer multiple of the tile dimension.
	// The raster height minus the end y coordinate should be also be an integer multiple.
	// The inverted y is a result of the geo coordinates starting at the top-left but
	// the raster data starting at the bottom-left.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			(x_geo_start % tile_geo_dimension) == 0,
			GPLATES_ASSERTION_SOURCE);
	if (d_raster_scanline_order == TOP_TO_BOTTOM)
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				(y_geo_start % tile_geo_dimension) == 0,
				GPLATES_ASSERTION_SOURCE);
	}
	else // BOTTOM_TO_TOP ...
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				((d_raster_height - y_geo_end) % tile_geo_dimension) == 0,
				GPLATES_ASSERTION_SOURCE);
	}

	// The number of texels needed to cover the tile.
	// Round this up so that the level-of-details texels cover the range of geo coordinates.
	unsigned int num_u_texels = (x_geo_end - x_geo_start + lod_factor - 1) / lod_factor;
	unsigned int num_v_texels = (y_geo_end - y_geo_start + lod_factor - 1) / lod_factor;

	// The texel offsets into the raster data.
	// Note we need to invert in the 'v' or 'y' direction because
	// our georeferencing starts at the top-left of the image but our
	// raster data starts at the bottom-left.
	// Both of the divisions here are exactly divisible.
	const unsigned int u_lod_texel_offset = x_geo_start / lod_factor;
	const unsigned int v_lod_texel_offset = (d_raster_scanline_order == TOP_TO_BOTTOM)
			? y_geo_start / lod_factor
			: (d_raster_height - y_geo_end) / lod_factor;

	const float u_start = 0; // x_geo_start begins exactly on a texel boundary
	const float u_end = static_cast<float>(x_geo_end - x_geo_start) / tile_geo_dimension;

	float v_start, v_end;
	if (d_raster_scanline_order == TOP_TO_BOTTOM)
	{
		v_start = 0; // y_geo_start begins exactly on a texel boundary
		v_end = static_cast<float>(y_geo_end - y_geo_start) / tile_geo_dimension;
	}
	else // BOTTOM_TO_TOP ...
	{
		v_start = static_cast<float>(y_geo_end - y_geo_start) / tile_geo_dimension;
		v_end = 0; // y_geo_end begins exactly on a texel boundary
	}

	// Determine the number of quads along each tile edge based on the texel resolution.
	// 'd_num_texels_per_vertex' is a 16:16 fixed-point type to allow fractional values without
	// floating-point precision issues potentially causing adjacent tiles to have different
	// tessellation (different number of vertices along common edge) and hence create gaps/cracks.
	//
	// Make sure we don't overflow the fixed-point calculation.
	// For tile dimensions less than 65,536 we should be fine.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			num_u_texels < (1 << 16) && num_v_texels < (1 << 16),
			GPLATES_ASSERTION_SOURCE);
	unsigned int x_num_quads = (num_u_texels << 16) / d_num_texels_per_vertex;
	unsigned int y_num_quads = (num_v_texels << 16) / d_num_texels_per_vertex;

	// Make sure non-zero.
	if (x_num_quads == 0)
	{
		x_num_quads = 1;
	}
	if (y_num_quads == 0)
	{
		y_num_quads = 1;
	}

	// The number of vertices each edge is the number of quads along each edge "+1".
	//
	// -------
	// | | | |
	// -------
	// | | | |
	// -------
	// | | | |
	// -------
	//
	// ...the above shows 3x3=9 quads but there's 4x4=16 vertices.
	//
	unsigned int x_num_vertices = x_num_quads + 1;
	unsigned int y_num_vertices = y_num_quads + 1;

	// Since we're using GLushort to store our vertex indices, we can't have
	// more than 65535 vertices per tile.
	if (x_num_vertices > (1 << (8 * sizeof(vertex_element_type) / 2/*sqrt*/)))
	{
		x_num_vertices = (1 << (8 * sizeof(vertex_element_type) / 2/*sqrt*/));
	}
	if (y_num_vertices > (1 << (8 * sizeof(vertex_element_type) / 2/*sqrt*/)))
	{
		y_num_vertices = (1 << (8 * sizeof(vertex_element_type) / 2/*sqrt*/));
	}

	//qDebug() << "Num vertices: " << x_num_vertices << ", " << y_num_vertices;

	// Create the level-of-detail tile now that we have all the information we need.
	LevelOfDetailTile::non_null_ptr_type lod_tile = LevelOfDetailTile::create(
			lod_level,
			x_geo_start, x_geo_end,
			y_geo_start, y_geo_end,
			x_num_vertices, y_num_vertices,
			u_start, u_end,
			v_start, v_end,
			u_lod_texel_offset, v_lod_texel_offset,
			num_u_texels, num_v_texels,
			*d_tile_vertices_cache,
			*d_tile_texture_cache);

	// Add the tile to the sequence of all tiles.
	const tile_handle_type tile_handle = d_tiles.size();
	d_tiles.push_back(lod_tile);

	// Return the tile handle.
	return tile_handle;
}


GPlatesOpenGL::GLIntersect::OrientedBoundingBox
GPlatesOpenGL::GLMultiResolutionRaster::bound_level_of_detail_tile(
		const LevelOfDetailTile &lod_tile) const
{
	// Generate the oriented axes for an OBB for this raster tile.
	//
	// Each OBB in the tree has one axis oriented radially outward from the globe at the
	// centre point of its bounding domain as this should generate the tightest bounding box.
	const double tile_centre_x_geo_coord = 0.5 * (lod_tile.x_geo_start + lod_tile.x_geo_end);
	const double tile_centre_y_geo_coord = 0.5 * (lod_tile.y_geo_start + lod_tile.y_geo_end);

	GLIntersect::OrientedBoundingBoxBuilder obb_builder = create_oriented_bounding_box_builder(
			tile_centre_x_geo_coord, tile_centre_y_geo_coord);

	// Set up some variables before calculating the boundary vertices.
	const double x_pixels_per_quad =
			static_cast<double>(lod_tile.x_geo_end - lod_tile.x_geo_start) /
					(lod_tile.x_num_vertices - 1);
	const double y_pixels_per_quad =
			static_cast<double>(lod_tile.y_geo_end - lod_tile.y_geo_start) /
					(lod_tile.y_num_vertices - 1);

	// Expand the oriented bounding box to include all vertices of the current tile.
	// The value of '4' is because:
	//  1) the lowest resolution can wrap around the entire globe (for a global raster), and
	//  2) the second lowest resolution can also wrap around the entire globe if the dimension,
	//     in pixels, of this level-of-detail is slightly above the tile dimension, and
	//  3) the third lowest resolution can wrap around *half* the entire globe, and
	//  4) the fourth lowest resolution can wrap around a *quarter* of the entire globe.
	// ...so, for the fourth lowest resolution (and higher resolutions), it is fine to exclude interior points.
	if (lod_tile.lod_level + 4 <= d_level_of_detail_pyramid.size())
	{
		// For high enough resolutions we only need the boundary vertices to accomplish this because
		// the interior vertices will then be bounded along the OBB's x and y axes (due to the boundary
		// vertices) and the z-axis will be bounded along its negative direction (due to the boundary
		// vertices) and the z-axis will be bounded along its positive direction due to the extremal
		// point already added in 'create_oriented_bounding_box_builder()'.

		// Bound the tile's top and bottom edge vertices.
		for (unsigned int i = 0; i < lod_tile.x_num_vertices; ++i)
		{
			obb_builder.add(convert_pixel_coord_to_geographic_coord(
					lod_tile.x_geo_start + i * x_pixels_per_quad,
					lod_tile.y_geo_start));
			obb_builder.add(convert_pixel_coord_to_geographic_coord(
					lod_tile.x_geo_start + i * x_pixels_per_quad,
					lod_tile.y_geo_end));
		}
		// Bound the tile's left and right edge vertices (excluding corner points already bounded).
		for (unsigned int j = 1; j < lod_tile.y_num_vertices - 1; ++j)
		{
			obb_builder.add(convert_pixel_coord_to_geographic_coord(
					lod_tile.x_geo_start,
					lod_tile.y_geo_start + j * y_pixels_per_quad));
			obb_builder.add(convert_pixel_coord_to_geographic_coord(
					lod_tile.x_geo_end,
					lod_tile.y_geo_start + j * y_pixels_per_quad));
		}
	}
	else // The lowest resolution levels of detail...
	{
		// Bound the tile's interior and exterior points since the level-of-detail is a low enough
		// resolution that (for a global raster) it could wrap around the globe more than 90 degrees.
		// This means we cannot exclude the interior points.
		for (unsigned int j = 0; j < lod_tile.y_num_vertices; ++j)
		{
			// Bound the tile's top and bottom edge vertices.
			for (unsigned int i = 0; i < lod_tile.x_num_vertices; ++i)
			{
				obb_builder.add(convert_pixel_coord_to_geographic_coord(
						lod_tile.x_geo_start + i * x_pixels_per_quad,
						lod_tile.y_geo_start + j * y_pixels_per_quad));
			}
		}
	}

	return obb_builder.get_oriented_bounding_box();
}


float
GPlatesOpenGL::GLMultiResolutionRaster::calc_max_texel_size_on_unit_sphere(
		const unsigned int lod_level,
		const LevelOfDetailTile &lod_tile) const
{
	//
	// Sample roughly 8x8 points of a 256x256 texel tile and at each point calculate the
	// size of a texel (at the level-of-detail of the tile).
	//
	// We could sample more densely to get a more accurate result but it could be
	// expensive. For example a 10,000 x 5,000 raster image will have ~800 tiles
	// at the highest resolution level-of-detail (tile size 256x256 texels).
	// If we sampled each tile at 32 x 32 (ie roughly the sampling of mesh vertices)
	// and it took 4000 CPU clock cycles per sample that would take:
	//   (800 x 32 x 32 x 4000 / 3e9) seconds
	// ...on a 3Ghz machine, which is 1.0 seconds (doesn't include other levels of detail).
	// This is only done once when the raster is first setup but still that's a noticeable delay.
	//
	// Set to 8x8 samples on a 256x256 texel tile (256 / 8 = 32).
	static const unsigned int NUM_TEXELS_PER_SAMPLE_POINT = 32;

	// Determine the number of sample points along each tile edge.
	unsigned int num_sample_segments_along_tile_x_edge =
			lod_tile.num_u_lod_texels / NUM_TEXELS_PER_SAMPLE_POINT;
	if (num_sample_segments_along_tile_x_edge == 0)
	{
		num_sample_segments_along_tile_x_edge = 1;
	}
	unsigned int num_sample_segments_along_tile_y_edge =
			lod_tile.num_v_lod_texels / NUM_TEXELS_PER_SAMPLE_POINT;
	if (num_sample_segments_along_tile_y_edge == 0)
	{
		num_sample_segments_along_tile_y_edge = 1;
	}

	// Set up some variables before calculating sample positions on the globe.
	const double x_pixels_per_quad =
			static_cast<double>(lod_tile.x_geo_end - lod_tile.x_geo_start) /
					num_sample_segments_along_tile_x_edge;
	const double y_pixels_per_quad =
			static_cast<double>(lod_tile.y_geo_end - lod_tile.y_geo_start) /
					num_sample_segments_along_tile_y_edge;

	// Number of samples along each tile edge.
	const unsigned int num_samples_along_tile_x_edge = num_sample_segments_along_tile_x_edge + 1;
	const unsigned int num_samples_along_tile_y_edge = num_sample_segments_along_tile_y_edge + 1;

	GPlatesMaths::real_t min_dot_product_texel_size(1);
	const double texel_size_in_pixels = (1 << lod_level);

	// Iterate over the sample points.
	for (unsigned int j = 0; j < num_samples_along_tile_y_edge; ++j)
	{
		const double y = lod_tile.y_geo_start + j * y_pixels_per_quad;

		// We try to avoid sampling outside the raster pixel range because we don't
		// know if that will cause problems with the inverse map projection (if any
		// was specified for the raster).
		const double y_plus_one_texel = y +
				((j == 0) ? texel_size_in_pixels : -texel_size_in_pixels);

		for (unsigned int i = 0; i < num_samples_along_tile_x_edge; ++i)
		{
			const double x = lod_tile.x_geo_start + i * x_pixels_per_quad;

			// The main sample point.
			const GPlatesMaths::PointOnSphere sample_point =
					convert_pixel_coord_to_geographic_coord(x, y);

			// We try to avoid sampling outside the raster pixel range because we don't
			// know if that will cause problems with the inverse map projection (if any
			// was specified for the raster).
			const double x_plus_one_texel = x +
					((i == 0) ? texel_size_in_pixels : -texel_size_in_pixels);

			// Sample plus one texel in x direction.
			const GPlatesMaths::PointOnSphere sample_point_plus_one_texel_x =
					convert_pixel_coord_to_geographic_coord(x_plus_one_texel, y);

			// The dot product can be converted to arc distance on unit sphere but we can
			// delay that expensive operation until we've compared all samples.
			const GPlatesMaths::real_t dot_product_texel_size_x = dot(
						sample_point.position_vector(),
						sample_point_plus_one_texel_x.position_vector());
			// We want the maximum projected texel size which means minimum dot product.
			if (dot_product_texel_size_x < min_dot_product_texel_size)
			{
				min_dot_product_texel_size = dot_product_texel_size_x;
			}

			// Sample plus one texel in y direction.
			const GPlatesMaths::PointOnSphere sample_point_plus_one_texel_y =
					convert_pixel_coord_to_geographic_coord(x, y_plus_one_texel);

			// The dot product can be converted to arc distance on unit sphere but we can
			// delay that expensive operation until we've compared all samples.
			const GPlatesMaths::real_t dot_product_texel_size_y = dot(
						sample_point.position_vector(),
						sample_point_plus_one_texel_y.position_vector());
			// We want the maximum projected texel size which means minimum dot product.
			if (dot_product_texel_size_y < min_dot_product_texel_size)
			{
				min_dot_product_texel_size = dot_product_texel_size_y;
			}
		}
	}

	// Convert from dot product to arc distance on the unit sphere.
	return acos(min_dot_product_texel_size).dval();
}


GPlatesOpenGL::GLIntersect::OrientedBoundingBoxBuilder
GPlatesOpenGL::GLMultiResolutionRaster::create_oriented_bounding_box_builder(
		const double &x_geo_coord,
		const double &y_geo_coord) const
{
	// Convert the pixel coordinates to a point on the sphere.
	const GPlatesMaths::PointOnSphere point_on_sphere =
			convert_pixel_coord_to_geographic_coord(x_geo_coord, y_geo_coord);

	// The OBB z-axis is the vector from globe origin to point on sphere.
	const GPlatesMaths::UnitVector3D &obb_z_axis = point_on_sphere.position_vector();

	// Calculate the OBB x axis by taking the centre pixel coordinate and doing a finite
	// difference in the x direction.
	// The delta value just needs to be small enough to get a nearly tangential vector
	// to the raster at the specified pixel coordinate.
	//
	// The reason for getting the OBB x-axis tangential to the raster is so the OBB
	// will align with, and hence bound, the raster tile(s) tightly.
	const double delta = 1; // Make it +/- one pixel of delta.
	const GPlatesMaths::PointOnSphere centre_point_minus_x_delta =
			convert_pixel_coord_to_geographic_coord(x_geo_coord - delta, y_geo_coord);
	const GPlatesMaths::PointOnSphere centre_point_plus_x_delta =
			convert_pixel_coord_to_geographic_coord(x_geo_coord + delta, y_geo_coord);

	// The vector difference between these two points is the x-axis.
	GPlatesMaths::Vector3D obb_x_axis_unnormalised =
			GPlatesMaths::Vector3D(centre_point_minus_x_delta.position_vector()) -
					GPlatesMaths::Vector3D(centre_point_plus_x_delta.position_vector());

	// Return a builder using the orthonormal axes.
	// We're using our x-axis as a y-axis in this function call but it doesn't matter -
	// just an axis label really.
	GLIntersect::OrientedBoundingBoxBuilder obb_builder =
			GLIntersect::create_oriented_bounding_box_builder(obb_x_axis_unnormalised, obb_z_axis);

	// The point of sphere of the pixel coordinates is the most extremal point along
	// the OBB's z-axis so add it to the OBB to set the maximum extent of the OBB
	// along its z-axis.
	obb_builder.add(point_on_sphere);

	// We'll still need to add more points before we get a usable OBB though.
	// It's up to the caller to do this.
	return obb_builder;
}


GLsizei
GPlatesOpenGL::GLMultiResolutionRaster::bind_vertex_element_buffer(
		GL &gl,
		const unsigned int num_vertices_along_tile_x_edge,
		const unsigned int num_vertices_along_tile_y_edge)
{
	// Should have at least two vertices along each edge of the tile.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			num_vertices_along_tile_x_edge > 1 && num_vertices_along_tile_y_edge > 1,
			GPLATES_ASSERTION_SOURCE);

	// Number of quads along each tile edge.
	const unsigned int num_quads_along_tile_x_edge = num_vertices_along_tile_x_edge - 1;
	const unsigned int num_quads_along_tile_y_edge = num_vertices_along_tile_y_edge - 1;

	// Total number of quads in the tile.
	const unsigned int num_quads_per_tile = num_quads_along_tile_x_edge * num_quads_along_tile_y_edge;

	// Total number of triangles and vertex indices in the tile.
	const unsigned int num_triangles_per_tile = 2 * num_quads_per_tile;
	const unsigned int num_indices_per_tile = 3 * num_triangles_per_tile;

	// Lookup our map of vertex element buffers to see if we've already created one
	// with the specified vertex dimensions.
	vertex_element_buffer_map_type::const_iterator vertex_element_buffer_iter =
			d_vertex_element_buffers.find(
					vertex_element_buffer_map_type::key_type(
							num_vertices_along_tile_x_edge,
							num_vertices_along_tile_y_edge));
	if (vertex_element_buffer_iter != d_vertex_element_buffers.end())
	{
		GLBuffer::shared_ptr_type vertex_element_buffer = vertex_element_buffer_iter->second;

		// Bind vertex element buffer object.
		//
		// NOTE: Also binds element buffer to the currently bound vertex array object.
		//       So caller must ensure a vertex array object is currently bound (required by OpenGL 3.3 core).
		gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_element_buffer);

		return num_indices_per_tile;
	}

	//
	// We haven't already created a vertex element buffer with the specified vertex dimensions
	// so create a new one.
	//

	// Total number of vertices in the tile.
	const unsigned int num_vertices_per_tile =
			num_vertices_along_tile_x_edge * num_vertices_along_tile_y_edge;

	// Since we're using GLushort to store our vertex indices, we can't have
	// more than 65535 vertices per tile - we're probably using about a thousand
	// per tile so should be no problem.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			num_vertices_per_tile < (1 << (8 * sizeof(vertex_element_type))),
			GPLATES_ASSERTION_SOURCE);

	// The array to store the vertex indices.
	boost::scoped_array<vertex_element_type> buffer_data(new vertex_element_type[num_indices_per_tile]);

	// Initialise the vertex indices.
	vertex_element_type *indices = buffer_data.get();
	for (unsigned int y = 0; y < num_quads_along_tile_y_edge; ++y)
	{
		for (unsigned int x = 0; x < num_quads_along_tile_x_edge; ++x)
		{
			//
			// These are the two triangles per quad:
			//
			// ----
			// |\ |
			// | \|
			// ----
			//

			// The lower triangle in the above diagram.
			// Front facing triangles are counter-clockwise.
			indices[0] = y * num_vertices_along_tile_x_edge + x;
			indices[1] = (y + 1) * num_vertices_along_tile_x_edge + x;
			indices[2] = (y + 1) * num_vertices_along_tile_x_edge + x + 1;
			indices += 3;

			// The upper triangle in the above diagram.
			// Front facing triangles are counter-clockwise.
			indices[0] = (y + 1) * num_vertices_along_tile_x_edge + x + 1;
			indices[1] = y * num_vertices_along_tile_x_edge + x + 1;
			indices[2] = y * num_vertices_along_tile_x_edge + x;
			indices += 3;
		}
	}

	// Set up the vertex element buffer.
	GLBuffer::shared_ptr_type vertex_element_buffer = GLBuffer::create(gl);

	// Add to our map of vertex element arrays.
	const vertex_element_buffer_map_type::key_type vertex_dimensions(
			num_vertices_along_tile_x_edge, num_vertices_along_tile_y_edge);
	d_vertex_element_buffers.insert(vertex_element_buffer_map_type::value_type(
			vertex_dimensions, vertex_element_buffer));

	// Bind vertex element buffer object.
	//
	// NOTE: Also binds element buffer to the currently bound vertex array object.
	//       So caller must ensure a vertex array object is currently bound (required by OpenGL 3.3 core).
	gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_element_buffer);

	// Transfer vertex element data to currently bound vertex element buffer object.
	glBufferData(
			GL_ELEMENT_ARRAY_BUFFER,
			num_indices_per_tile * sizeof(vertex_element_type),
			buffer_data.get(),
			GL_STATIC_DRAW);

	return num_indices_per_tile;
}


GPlatesMaths::PointOnSphere
GPlatesOpenGL::GLMultiResolutionRaster::convert_pixel_coord_to_geographic_coord(
		const double &x_pixel_coord,
		const double &y_pixel_coord,
		boost::optional<double &> y_pixel_coord_clamped) const
{
	// Get the georeferencing parameters.
	const GPlatesPropertyValues::Georeferencing::parameters_type georef = d_georeferencing->get_parameters();

	// Use the georeferencing information to convert from pixel/line coordinates to georeference
	// coordinates in the raster's spatial reference system.
	double x_geo =
			x_pixel_coord * georef.x_component_of_pixel_width +
			y_pixel_coord * georef.x_component_of_pixel_height +
			georef.top_left_x_coordinate;
	double y_geo =
			x_pixel_coord * georef.y_component_of_pixel_width +
			y_pixel_coord * georef.y_component_of_pixel_height +
			georef.top_left_y_coordinate;

	// Transform georeferenced raster coordinates to the standard geographic coordinate system WGS84
	// (this transforms from the raster's possibly *projection* spatial reference).
	d_coordinate_transformation->transform_in_place(&x_geo, &y_geo);

	// Some rasters have a geographic coordinate latitude extent of, for example, [-90.05, 90.05]
	// (for a raster of height 1801) such that the pixel centre of the first and last rows are at
	// -90 or 90 degrees. However we are rendering in cartesian (x,y,z) space and not
	// lat/lon space so we need to clamp latitudes to the poles. This means the texture coordinates
	// also need to be adjusted otherwise a slight error in positioning (georeferencing) of raster data
	// would occur (but only for raster pixels between the pole and the nearest vertex in the raster mesh
	// which is up to 5 degrees away).
	bool clamped = false;
	if (y_geo < -90)
	{
		y_geo = -90;
		clamped = true;
	}
	else if (y_geo > 90)
	{
		y_geo = 90;
		clamped = true;
	}

	if (y_pixel_coord_clamped)
	{
		if (clamped &&
			// May need to update this to work for non-identity cases if we update how datum transforms are handled
			// (eg, might have no projection transform, but still a datum transform such as geocentric -> WGS84).
			// Currently, in RasterLayerProxy::set_raster_params(), we use an identity transform if either
			// the raster has no spatial reference system or it has datum WGS84 (but no projection transform) ...
			d_coordinate_transformation->is_identity_transform() &&
			GPlatesMaths::are_almost_exactly_equal(georef.y_component_of_pixel_width, 0.0) &&
			GPlatesMaths::are_almost_exactly_equal(georef.x_component_of_pixel_height, 0.0))
		{
			// This is the reverse of the conversion from pixel coordinates to georeference coordinates above.
			y_pixel_coord_clamped.get() = (y_geo - georef.top_left_y_coordinate) / georef.y_component_of_pixel_height;
		}
		else
		{
			y_pixel_coord_clamped.get() = y_pixel_coord;
		}
	}

	// Wrap longitude into the range [-360,360] if necessary since otherwise LatLonPoint will thrown an exception.
	// Some rasters have a georeference longitude extent of, for example, [-0.05, 360.05] (for a
	// raster of width 3601) such that the pixel centre of the first and last column are at the same
	// position (0, or 360, degrees) in order to avoid a seam.
	// So we shouldn't clamp those values (we wraparound instead).
	if (x_geo < -360)
	{
		do 
		{
			x_geo += 360;
		}
		while (x_geo < -360);
	}
	else if (x_geo > 360)
	{
		do 
		{
			x_geo -= 360;
		}
		while (x_geo > 360);
	}

	// Finally convert from (longitude, latitude) to cartesian (x,y,z).
	const GPlatesMaths::LatLonPoint x_lat_lon(y_geo, x_geo);

	return make_point_on_sphere(x_lat_lon);
}


GPlatesOpenGL::GLMultiResolutionRaster::LevelOfDetailTile::LevelOfDetailTile(
		unsigned int lod_level_,
		unsigned int x_geo_start_,
		unsigned int x_geo_end_,
		unsigned int y_geo_start_,
		unsigned int y_geo_end_,
		unsigned int x_num_vertices_,
		unsigned int y_num_vertices_,
		float u_start_,
		float u_end_,
		float v_start_,
		float v_end_,
		unsigned int u_lod_texel_offset_,
		unsigned int v_lod_texel_offset_,
		unsigned int num_u_lod_texels_,
		unsigned int num_v_lod_texels_,
		tile_vertices_cache_type &tile_vertices_cache_,
		tile_texture_cache_type &tile_texture_cache_) :
	lod_level(lod_level_),
	x_geo_start(x_geo_start_),
	x_geo_end(x_geo_end_),
	y_geo_start(y_geo_start_),
	y_geo_end(y_geo_end_),
	x_num_vertices(x_num_vertices_),
	y_num_vertices(y_num_vertices_),
	u_start(u_start_),
	u_end(u_end_),
	v_start(v_start_),
	v_end(v_end_),
	u_lod_texel_offset(u_lod_texel_offset_),
	v_lod_texel_offset(v_lod_texel_offset_),
	num_u_lod_texels(num_u_lod_texels_),
	num_v_lod_texels(num_v_lod_texels_),
	tile_vertices(tile_vertices_cache_.allocate_volatile_object()),
	tile_texture(tile_texture_cache_.allocate_volatile_object())
{
}


GPlatesOpenGL::GLMultiResolutionRaster::LevelOfDetail::LevelOfDetail(
		unsigned int lod_level_) :
	lod_level(lod_level_),
	obb_tree_root_node_index(0)
{
}


const GPlatesOpenGL::GLMultiResolutionRaster::LevelOfDetail::OBBTreeNode &
GPlatesOpenGL::GLMultiResolutionRaster::LevelOfDetail::get_obb_tree_node(
		std::size_t node_index) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			node_index < obb_tree_nodes.size(),
			GPLATES_ASSERTION_SOURCE);

	return obb_tree_nodes[node_index];
}


GPlatesOpenGL::GLMultiResolutionRaster::LevelOfDetail::OBBTreeNode &
GPlatesOpenGL::GLMultiResolutionRaster::LevelOfDetail::get_obb_tree_node(
		std::size_t node_index)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			node_index < obb_tree_nodes.size(),
			GPLATES_ASSERTION_SOURCE);

	return obb_tree_nodes[node_index];
}


GPlatesOpenGL::GLMultiResolutionRaster::RenderSphereNormals::RenderSphereNormals(
		GL &gl) :
	d_vertex_array(GLVertexArray::create(gl)),
	d_vertex_buffer(GLBuffer::create(gl)),
	d_vertex_element_buffer(GLBuffer::create(gl)),
	d_num_vertex_elements(0),
	d_program(GLProgram::create(gl))
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Vertex shader source.
	GLShaderSource vertex_shader_source;
	vertex_shader_source.add_code_segment(RENDER_SPHERE_NORMALS_VERTEX_SHADER_SOURCE);

	// Vertex shader.
	GLShader::shared_ptr_type vertex_shader = GLShader::create(gl, GL_VERTEX_SHADER);
	vertex_shader->shader_source(vertex_shader_source);
	vertex_shader->compile_shader();

	// Fragment shader source.
	GLShaderSource fragment_shader_source;
	fragment_shader_source.add_code_segment(RENDER_SPHERE_NORMALS_FRAGMENT_SHADER_SOURCE);

	// Fragment shader.
	GLShader::shared_ptr_type fragment_shader = GLShader::create(gl, GL_FRAGMENT_SHADER);
	fragment_shader->shader_source(fragment_shader_source);
	fragment_shader->compile_shader();

	// Vertex-fragment program.
	d_program->attach_shader(vertex_shader);
	d_program->attach_shader(fragment_shader);
	d_program->link_program();

	//
	// Create a cube - this is one of the simplest primitives that covers the globe.
	// The per-pixel cube position (texture coordinate) gets normalised to unit normal in fragment shader.
	//

	std::vector<GLVertexUtils::Vertex> vertices;
	std::vector<vertex_element_type> vertex_elements;

	// Add the eight cube corner vertices.
	vertices.push_back(GLVertexUtils::Vertex(-1,-1,-1)); // 0
	vertices.push_back(GLVertexUtils::Vertex(+1,-1,-1)); // 1
	vertices.push_back(GLVertexUtils::Vertex(-1,+1,-1)); // 2
	vertices.push_back(GLVertexUtils::Vertex(+1,+1,-1)); // 3
	vertices.push_back(GLVertexUtils::Vertex(-1,-1,+1)); // 4
	vertices.push_back(GLVertexUtils::Vertex(+1,-1,+1)); // 5
	vertices.push_back(GLVertexUtils::Vertex(-1,+1,+1)); // 6
	vertices.push_back(GLVertexUtils::Vertex(+1,+1,+1)); // 7

	// Add the twelve cube faces (two triangles per cube face).
	vertex_elements.push_back(0); vertex_elements.push_back(1); vertex_elements.push_back(3);
	vertex_elements.push_back(0); vertex_elements.push_back(3); vertex_elements.push_back(2);
	vertex_elements.push_back(0); vertex_elements.push_back(5); vertex_elements.push_back(1);
	vertex_elements.push_back(0); vertex_elements.push_back(4); vertex_elements.push_back(5);
	vertex_elements.push_back(1); vertex_elements.push_back(7); vertex_elements.push_back(3);
	vertex_elements.push_back(1); vertex_elements.push_back(5); vertex_elements.push_back(7);
	vertex_elements.push_back(0); vertex_elements.push_back(2); vertex_elements.push_back(6);
	vertex_elements.push_back(0); vertex_elements.push_back(6); vertex_elements.push_back(4);
	vertex_elements.push_back(2); vertex_elements.push_back(3); vertex_elements.push_back(7);
	vertex_elements.push_back(2); vertex_elements.push_back(7); vertex_elements.push_back(6);
	vertex_elements.push_back(4); vertex_elements.push_back(7); vertex_elements.push_back(5);
	vertex_elements.push_back(4); vertex_elements.push_back(6); vertex_elements.push_back(7);

	d_num_vertex_elements = vertex_elements.size();

	// Bind vertex array object.
	gl.BindVertexArray(d_vertex_array);

	// Bind vertex element buffer object to currently bound vertex array object.
	gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, d_vertex_element_buffer);

	// Transfer vertex element data to currently bound vertex element buffer object.
	glBufferData(
			GL_ELEMENT_ARRAY_BUFFER,
			vertex_elements.size() * sizeof(vertex_elements[0]),
			vertex_elements.data(),
			GL_STATIC_DRAW);

	// Bind vertex buffer object (used by vertex attribute arrays, not vertex array object).
	gl.BindBuffer(GL_ARRAY_BUFFER, d_vertex_buffer);

	// Transfer vertex data to currently bound vertex buffer object.
	glBufferData(
			GL_ARRAY_BUFFER,
			vertices.size() * sizeof(vertices[0]),
			vertices.data(),
			GL_STATIC_DRAW);

	// Specify vertex attributes (position only) in currently bound vertex buffer object.
	// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
	// to currently bound vertex array object.
	gl.EnableVertexAttribArray(0);
	gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLVertexUtils::Vertex), BUFFER_OFFSET(GLVertexUtils::Vertex, x));
}


void
GPlatesOpenGL::GLMultiResolutionRaster::RenderSphereNormals::render(
		GL &gl,
		const GLMatrix &view_projection_transform)
{
	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Bind the shader program.
	gl.UseProgram(d_program);

	// Set view projection matrix in the currently bound program.
	GLfloat view_projection_float_matrix[16];
	view_projection_transform.get_float_matrix(view_projection_float_matrix);
	glUniformMatrix4fv(
			d_program->get_uniform_location("view_projection"),
			1, GL_FALSE/*transpose*/, view_projection_float_matrix);

	// Bind vertex array object.
	gl.BindVertexArray(d_vertex_array);

	// Render the cube.
	glDrawElements(
			GL_TRIANGLES,
			d_num_vertex_elements/*count*/,
			GLVertexUtils::ElementTraits<vertex_element_type>::type,
			nullptr/*indices_offset*/);
}
