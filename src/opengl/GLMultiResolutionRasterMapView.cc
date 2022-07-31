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

#include <cmath>
#include <boost/cast.hpp>

#include <QDebug>
#include <QImage>
#include <QPainter>
#include <QString>

#include "GLMultiResolutionRasterMapView.h"

#include "GLContext.h"
#include "GLFrustum.h"
#include "GLImageUtils.h"
#include "GLIntersect.h"
#include "GLMatrix.h"
#include "GLShader.h"
#include "GLShaderSource.h"
#include "GLTexture.h"
#include "GLTextureUtils.h"
#include "GLViewport.h"
#include "GLViewProjection.h"

#include "maths/MathsUtils.h"
#include "maths/Rotation.h"

#include "utils/CallStackTracker.h"
#include "utils/Profile.h"

// Enables visually showing level-of-detail in the map view.
#define DEBUG_LEVEL_OF_DETAIL_VISUALLY

// Renders text displaying the level-of-detail of tiles, otherwise renders a checkerboard pattern
// to visualise texel-to-pixel mapping ratios.
// NOTE: 'DEBUG_LEVEL_OF_DETAIL_VISUALLY' must be defined.
#define DEBUG_LEVEL_OF_DETAIL_WITH_TEXT


namespace GPlatesOpenGL
{
	namespace
	{
		/**
		 * Vertex shader source for rendering the tile texture to the scene.
		 */
		const char *RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE =
			R"(
				uniform bool using_clip_texture;

				uniform mat4 view_projection;

				uniform mat4 tile_texture_transform;
				uniform mat4 clip_texture_transform;
			
				layout(location = 0) in vec4 position;
				layout(location = 1) in vec4 texture_coordinate; // xyz from vertex buffer and w=1 (default)

				out vec4 tile_texture_coordinate;
				out vec4 clip_texture_coordinate;

				void main (void)
				{
					gl_Position = view_projection * position;

					//
					// Transform 3D texture coords (present day position; ie, not 'position') by
					// cube map projection and any texture coordinate adjustments before accessing textures.
					// We have two texture transforms but only one texture coordinate.
					//

					// Tile texture cube map projection.
					tile_texture_coordinate = tile_texture_transform * texture_coordinate;

					if (using_clip_texture)
					{
						// Clip texture cube map projection.
						clip_texture_coordinate = clip_texture_transform * texture_coordinate;
					}
				}
			)";

		/**
		 * Fragment shader source for rendering the tile texture to the scene.
		 */
		const char *RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE =
			R"(
				uniform bool using_clip_texture;

				uniform sampler2D tile_texture_sampler;
				uniform sampler2D clip_texture_sampler;

				in vec4 tile_texture_coordinate;
				in vec4 clip_texture_coordinate;
			
				layout(location = 0) out vec4 colour;

				void main (void)
				{
					// Discard the pixel if it has been clipped by the clip texture.
					if (using_clip_texture)
					{
						float clip_mask = textureProj(clip_texture_sampler, clip_texture_coordinate).a;
						if (clip_mask == 0)
						{
							discard;
						}
					}
	
					//
					// The source raster can be either a visual source raster (RGBA) or data raster (RG).
					// A visual raster source has premultiplied alpha, and a data raster source has premultiplied coverage (from Green channel).
					//
					// Note: Data rasters use the 2-channel RG texture format with data in Red and coverage in Green.
					//       Since there's no Alpha channel, the texture swizzle (set in texture object) copies coverage in the
					//       Green channel into the Alpha channel. Also if we're outputting to an RG render-texture then Alpha gets
					//       discarded (but still used by alpha-blender), however that's OK since coverage is still in the Green channel.
					//
					colour = textureProj(tile_texture_sampler, tile_texture_coordinate);

					// Reject the pixel if its alpha (or coverage) is zero.
					//
					// Note that for data rasters the texture swizzle (set in texture object) copied the coverage from green channel to alpha channel.
					if (colour.a == 0)
					{
						discard;
					}
				}
			)";


		/**
		 * Visualise the level-of-detail of a tile (either as text or a checkerboard pattern).
		 */
#ifdef DEBUG_LEVEL_OF_DETAIL_VISUALLY
		void
		visualise_level_of_detail_in_texture(
				GL &gl,
				const GLTexture::shared_ptr_type &tile_texture,
				unsigned int tile_texel_dimension,
				unsigned int level_of_detail)
		{
#ifdef DEBUG_LEVEL_OF_DETAIL_WITH_TEXT

			const QString debug_text = QString("LOD %1").arg(level_of_detail);

			// Draw text into an image.
			const QImage debug_image = GLImageUtils::draw_text_into_qimage(
					debug_text,
					tile_texel_dimension, tile_texel_dimension,
					3.0f/*text scale*/,
					QColor(255, 0, 0, 255)/*red text*/);

#else

			// Draw a checkerboard pattern into an image.
			// This is to visualise the texel density on the screen to see how close to a one-to-one
			// texel-to-pixel mapping we get on the screen.
			QImage debug_image(tile_texel_dimension, tile_texel_dimension, QImage::Format_ARGB32);

			QPainter pattern_painter;

			// Create the base pattern for the checkerboard.
			QImage pattern2x2(2, 2, QImage::Format_ARGB32);
			pattern_painter.begin(&pattern2x2);
			pattern_painter.setCompositionMode(QPainter::CompositionMode_Clear);
			pattern_painter.fillRect(0, 0, 2, 2, Qt::transparent);
			pattern_painter.setCompositionMode(QPainter::CompositionMode_Source);
			pattern_painter.fillRect(0, 0, 1, 1, Qt::transparent);
			pattern_painter.fillRect(1, 0, 1, 1, Qt::white);
			pattern_painter.fillRect(0, 1, 1, 1, Qt::white);
			pattern_painter.fillRect(1, 1, 1, 1, Qt::transparent);
			pattern_painter.end();

			const unsigned int log2_texels_per_pattern = 3;
			const unsigned int texels_per_pattern = (1 << log2_texels_per_pattern);
			const unsigned int pattern_size = 2 * texels_per_pattern;
			QImage pattern(pattern_size, pattern_size, QImage::Format_ARGB32);
			// Paint the checkerboard pattern.
			pattern_painter.begin(&pattern);
			pattern_painter.setCompositionMode(QPainter::CompositionMode_Clear);
			pattern_painter.fillRect(0, 0, pattern_size, pattern_size, Qt::transparent);
			pattern_painter.setCompositionMode(QPainter::CompositionMode_Source);
			for (unsigned int yp = 0; yp < pattern_size / 2; ++yp)
			{
				for (unsigned int xp = 0; xp < pattern_size / 2; ++xp)
				{
					int posx = 2 * xp;
					int posy = 2 * yp;
					// Leave some patterns transparent.
					if (((posx >> log2_texels_per_pattern) & 1) ^ ((posy >> log2_texels_per_pattern) & 1))
					{
						pattern_painter.drawImage(posx, posy, pattern2x2);
					}
				}
			}
			pattern_painter.end();

			QPainter debug_image_painter(&debug_image);
			debug_image_painter.setCompositionMode(QPainter::CompositionMode_Clear);
			debug_image_painter.fillRect(0, 0, tile_texel_dimension, tile_texel_dimension, Qt::transparent);
			debug_image_painter.setCompositionMode(QPainter::CompositionMode_Source);

			// Paint the checkerboard.
			for (unsigned int y = 0; y < tile_texel_dimension / pattern_size; ++y)
			{
				for (unsigned int x = 0; x < tile_texel_dimension / pattern_size; ++x)
				{
					debug_image_painter.drawImage(x * pattern_size, y * pattern_size, pattern);
				}
			}

			debug_image_painter.end();

#endif

			// Convert to ARGB32 format so it's easier to load into a texture.
			boost::scoped_array<GPlatesGui::rgba8_t> debug_rgba8_array;
			GLTextureUtils::load_argb32_qimage_into_rgba8_array(
					debug_image.convertToFormat(QImage::Format_ARGB32),
					debug_rgba8_array);

			// Make sure we leave the OpenGL global state the way it was.
			GL::StateScope save_restore_state(gl);

			gl.BindTexture(GL_TEXTURE_2D, tile_texture);

			// Load cached image into tile texture.
			gl.TexSubImage2D(
					GL_TEXTURE_2D, 0/*level*/,
					0/*xoffset*/, 0/*yoffset*/, debug_image.width(), debug_image.height(),
					GL_RGBA, GL_UNSIGNED_BYTE, debug_rgba8_array.get());
		}
#endif
	}
}

const double GPlatesOpenGL::GLMultiResolutionRasterMapView::ERROR_VIEWPORT_PIXEL_SIZE_IN_MAP_PROJECTION = 360.0;


GPlatesOpenGL::GLMultiResolutionRasterMapView::GLMultiResolutionRasterMapView(
		GL &gl,
		const GLMultiResolutionCubeRasterInterface::non_null_ptr_type &multi_resolution_cube_raster,
		const GLMultiResolutionMapCubeMesh::non_null_ptr_to_const_type &multi_resolution_map_cube_mesh) :
	d_multi_resolution_cube_raster(multi_resolution_cube_raster),
	d_multi_resolution_map_cube_mesh(multi_resolution_map_cube_mesh),
	d_tile_texel_dimension(multi_resolution_cube_raster->get_tile_texel_dimension()),
	d_map_projection_central_meridian_longitude(0),
	d_render_tile_to_scene_program(GLProgram::create(gl))
{
	compile_link_shader_program(gl);
}


bool
GPlatesOpenGL::GLMultiResolutionRasterMapView::render(
		GL &gl,
		const GLViewProjection &view_projection,
		cache_handle_type &cache_handle)
{
	PROFILE_FUNC();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// We only have one shader program (for rendering each tile to the scene).
	// Bind it early since we'll be setting state in it as we traverse the cube quad tree.
	gl.UseProgram(d_render_tile_to_scene_program);

	// The shader program outputs colours for a visual source raster (or data and coverage for a data source raster)
	// that have premultiplied alpha (or coverage) because the source raster itself has premultiplied alpha (or coverage).
	//
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

	// Set view projection matrix in shader program.
	GLfloat view_projection_float_matrix[16];
	view_projection.get_view_projection_transform().get_float_matrix(view_projection_float_matrix);
	gl.UniformMatrix4fv(
			d_render_tile_to_scene_program->get_uniform_location(gl, "view_projection"),
			1, GL_FALSE/*transpose*/, view_projection_float_matrix);

	// First see if the map projection central meridian has changed.
	//
	// NOTE: If the projection *type* changes then we don't need to change our world transform.
	const double updated_map_projection_central_meridian =
			d_multi_resolution_map_cube_mesh->get_current_map_projection_settings().get_central_meridian();
	if (!GPlatesMaths::are_almost_exactly_equal(
			d_map_projection_central_meridian_longitude, updated_map_projection_central_meridian))
	{
		d_map_projection_central_meridian_longitude = updated_map_projection_central_meridian;

		// Specify a transform that makes the (non-zero longitude) central meridian (at equator)
		// become the global x-axis. This means an object on the central meridian
		// (of the map projection) will be transformed to be at longitude zero.
		//
		// This transform rotates, about north pole, to move central meridian longitude to zero longitude...
		const GPlatesMaths::Rotation world_transform =
				GPlatesMaths::Rotation::create(
						GPlatesMaths::UnitVector3D::zBasis()/*north pole*/,
						GPlatesMaths::convert_deg_to_rad(
								// The negative sign rotates *to* longitude zero...
								-d_map_projection_central_meridian_longitude));

		d_world_transform = GLMatrix(world_transform.quat());
	}

	// If our world transform differs from the cube raster's then set it.
	// This can happen if some other client changes the cube raster's world transform or
	// if we have a new map projection central meridian (which changes our world transform).
	if (d_world_transform != d_multi_resolution_cube_raster->get_world_transform())
	{
		// Note that this invalidates all cached textures so we only want to call it if the transform changed.
		d_multi_resolution_cube_raster->set_world_transform(d_world_transform);
	}

	// Determine the size of a viewport pixel in map projection coordinates.
	const double viewport_pixel_size_in_map_projection = get_viewport_pixel_size_in_map_projection(view_projection);

	// The size of a tile of viewport pixels projected onto the map (ie, in map-projection coordinates).
	// A tile is 'd_tile_texel_dimension' x 'd_tile_texel_dimension' pixels. When a tile of texels
	// in the map projection matches this then the correct level-of-detail has been found.
	const double viewport_tile_map_projection_size =
			d_tile_texel_dimension * viewport_pixel_size_in_map_projection;

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
											d_tile_texel_dimension,
											0.5/* half a texel */)));
	// Cube subdivision cache for the clip texture (no frustum expansion here).
	clip_cube_subdivision_cache_type::non_null_ptr_type
			clip_cube_subdivision_cache =
					clip_cube_subdivision_cache_type::create(
							GLCubeSubdivision::create());

	// Keep track of how many tiles were rendered to the scene.
	// Currently this is just used to determine if we rendered anything.
	unsigned int num_tiles_rendered_to_scene = 0;

	// The cached view is a sequence of tiles for the caller to keep alive until the next frame.
	boost::shared_ptr<std::vector<cache_handle_type> > cached_tiles(new std::vector<cache_handle_type>());

	//
	// Traverse the source raster cube quad tree and the spatial partition of reconstructed polygon meshes.
	//

	// Traverse the quad trees of the cube faces.
	for (unsigned int face = 0; face < 6; ++face)
	{
		const GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face =
				static_cast<GPlatesMaths::CubeCoordinateFrame::CubeFaceType>(face);

		// Get the quad tree root node of the current cube face of the source raster.
		const boost::optional<raster_quad_tree_node_type> source_raster_quad_tree_root =
				d_multi_resolution_cube_raster->get_quad_tree_root_node(cube_face);
		// If there is no source raster for the current cube face then continue to the next face.
		if (!source_raster_quad_tree_root)
		{
			continue;
		}

		// Get the quad tree root node of the current cube face of the source mesh.
		const mesh_quad_tree_node_type mesh_quad_tree_root_node =
				d_multi_resolution_map_cube_mesh->get_quad_tree_root_node(cube_face);

		// Get the cube subdivision root node.
		const cube_subdivision_cache_type::node_reference_type
				cube_subdivision_cache_root_node =
						cube_subdivision_cache->get_quad_tree_root_node(cube_face);
		// Get the cube subdivision root node.
		const clip_cube_subdivision_cache_type::node_reference_type
				clip_cube_subdivision_cache_root_node =
						clip_cube_subdivision_cache->get_quad_tree_root_node(cube_face);

		render_quad_tree(
				gl,
				*source_raster_quad_tree_root,
				mesh_quad_tree_root_node,
				*cube_subdivision_cache,
				cube_subdivision_cache_root_node,
				*clip_cube_subdivision_cache,
				clip_cube_subdivision_cache_root_node,
				viewport_tile_map_projection_size,
				frustum_planes,
				// There are six frustum planes initially active
				GLFrustum::ALL_PLANES_ACTIVE_MASK,
				*cached_tiles,
				num_tiles_rendered_to_scene);
	}

	// Return cached tiles to the caller.
	cache_handle = cached_tiles;

	return num_tiles_rendered_to_scene > 0;
}


void
GPlatesOpenGL::GLMultiResolutionRasterMapView::render_quad_tree(
		GL &gl,
		const raster_quad_tree_node_type &source_raster_quad_tree_node,
		const mesh_quad_tree_node_type &mesh_quad_tree_node,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
		clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
		const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
		const double &viewport_tile_map_projection_size,
		const GLFrustum &frustum_planes,
		boost::uint32_t frustum_plane_mask,
		std::vector<cache_handle_type> &cached_tiles,
		unsigned int &num_tiles_rendered_to_scene)
{
	// If the frustum plane mask is zero then it means we are entirely inside the view frustum.
	// So only test for intersection if the mask is non-zero.
	if (frustum_plane_mask != 0)
	{
		const GLIntersect::OrientedBoundingBox &quad_tree_node_bounds =
				mesh_quad_tree_node.get_map_projection_bounding_box();

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

	// If either:
	// - we're at the correct level of detail for rendering, or
	// - we've reached a leaf node of the source raster (highest resolution supplied by the source raster),
	// ...then render the current source raster tile.
	if (mesh_quad_tree_node.get_max_map_projection_size() <= viewport_tile_map_projection_size ||
		source_raster_quad_tree_node.is_leaf_node())
	{
		render_tile_to_scene(
				gl,
				source_raster_quad_tree_node,
				mesh_quad_tree_node,
				cube_subdivision_cache,
				cube_subdivision_cache_node,
				clip_cube_subdivision_cache,
				clip_cube_subdivision_cache_node,
				cached_tiles,
				num_tiles_rendered_to_scene);

		return;
	}

	//
	// Iterate over the child quad tree nodes.
	//

	for (unsigned int child_v_offset = 0; child_v_offset < 2; ++child_v_offset)
	{
		for (unsigned int child_u_offset = 0; child_u_offset < 2; ++child_u_offset)
		{
			// Get the child node of the current source raster quad tree node.
			const boost::optional<raster_quad_tree_node_type> child_source_raster_quad_tree_node =
					d_multi_resolution_cube_raster->get_child_node(
							source_raster_quad_tree_node, child_u_offset, child_v_offset);
			// If there is no source raster for the current child then continue to the next child.
			// This happens if the current child is not covered by the source raster.
			// Note that if we get here then the current parent is not a leaf node.
			if (!child_source_raster_quad_tree_node)
			{
				continue;
			}

			// Get the child node of the current mesh quad tree node.
			const mesh_quad_tree_node_type child_mesh_quad_tree_node =
					d_multi_resolution_map_cube_mesh->get_child_node(
							mesh_quad_tree_node,
							child_u_offset,
							child_v_offset);

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
					*child_source_raster_quad_tree_node,
					child_mesh_quad_tree_node,
					cube_subdivision_cache,
					child_cube_subdivision_cache_node,
					clip_cube_subdivision_cache,
					child_clip_cube_subdivision_cache_node,
					viewport_tile_map_projection_size,
					frustum_planes,
					frustum_plane_mask,
					cached_tiles,
					num_tiles_rendered_to_scene);
		}
	}
}


void
GPlatesOpenGL::GLMultiResolutionRasterMapView::render_tile_to_scene(
		GL &gl,
		const raster_quad_tree_node_type &source_raster_quad_tree_node,
		const mesh_quad_tree_node_type &mesh_quad_tree_node,
		cube_subdivision_cache_type &cube_subdivision_cache,
		const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
		clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
		const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
		std::vector<cache_handle_type> &cached_tiles,
		unsigned int &num_tiles_rendered_to_scene)
{
	// Get the tile texture from our source raster.
	GLMultiResolutionCubeRasterInterface::cache_handle_type source_raster_cache_handle;
	const boost::optional<GLTexture::shared_ptr_type> tile_texture_opt =
			source_raster_quad_tree_node.get_tile_texture(gl, source_raster_cache_handle);
	// If there is no tile texture it means there's nothing to be drawn (eg, no raster covering this node).
	if (!tile_texture_opt)
	{
		//qDebug() << "***************** Nothing render into cube quad tree tile *****************";
		return;
	}
	const GLTexture::shared_ptr_type tile_texture = tile_texture_opt.get();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

#if 0
	qDebug()
		<< "Rendered cube quad tree tile: " << num_tiles_rendered_to_scene
		<< " at LOD: " << cube_subdivision_cache_node.get_level_of_detail();
#endif
#ifdef DEBUG_LEVEL_OF_DETAIL_VISUALLY
	visualise_level_of_detail_in_texture(
			gl,
			tile_texture,
			d_tile_texel_dimension,
			cube_subdivision_cache_node.get_level_of_detail());
#endif

	// Make sure we return the cached handle to our caller so they can cache it.
	cached_tiles.push_back(source_raster_cache_handle);


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

	// See if we've traversed deep enough in the cube mesh quad tree to require using a clip
	// texture - this occurs because the cube mesh has nodes only to a certain depth.
	const bool clip_to_tile_frustum = static_cast<bool>(
			mesh_quad_tree_node.get_clip_texture_clip_space_transform());

	// Prepare for rendering the current tile.
	set_tile_state(
			gl,
			tile_texture,
			*projection_transform,
			*clip_projection_transform,
			*view_transform,
			clip_to_tile_frustum);

	// Draw the mesh covering the current quad tree node tile.
	mesh_quad_tree_node.render_mesh_drawable(gl);

	// This is the only place we increment the rendered tile count.
	++num_tiles_rendered_to_scene;
}


void
GPlatesOpenGL::GLMultiResolutionRasterMapView::set_tile_state(
		GL &gl,
		const GLTexture::shared_ptr_type &tile_texture,
		const GLTransform &projection_transform,
		const GLTransform &clip_projection_transform,
		const GLTransform &view_transform,
		bool clip_to_tile_frustum)
{
	//
	// Source raster.
	//

	// Bind tile texture to texture unit 0.
	gl.ActiveTexture(GL_TEXTURE0);
	gl.BindTexture(GL_TEXTURE_2D, tile_texture);

	// Used to transform tile texture coordinates to account for partial coverage of current tile.
	GLMatrix tile_texture_matrix;
	tile_texture_matrix.gl_mult_matrix(GLUtils::get_clip_space_to_texture_space_transform());
	// Set up the texture matrix to perform model-view and projection transforms of the frustum.
	tile_texture_matrix.gl_mult_matrix(projection_transform.get_matrix());
	tile_texture_matrix.gl_mult_matrix(view_transform.get_matrix());

	// Transfer tile texture transform to shader program.
	GLfloat tile_texture_float_matrix[16];
	tile_texture_matrix.get_float_matrix(tile_texture_float_matrix);
	gl.UniformMatrix4fv(
			d_render_tile_to_scene_program->get_uniform_location(gl, "tile_texture_transform"),
			1, GL_FALSE/*transpose*/, tile_texture_float_matrix);

	// Set whether clipping is enabled.
	gl.Uniform1i(
			d_render_tile_to_scene_program->get_uniform_location(gl, "using_clip_texture"),
			clip_to_tile_frustum);

	// If we've traversed deep enough into the cube quad tree then the cube quad tree mesh
	// then the drawable starts to cover multiple quad tree nodes (instead of a single node)
	// so we need to use a clip texture to mask off all but the node we're interested in.
	if (clip_to_tile_frustum)
	{
		// Bind clip texture to texture unit 1.
		gl.ActiveTexture(GL_TEXTURE1);
		gl.BindTexture(GL_TEXTURE_2D, d_multi_resolution_map_cube_mesh->get_clip_texture());

		// State for the clip texture.
		//
		// NOTE: We also do *not* expand the tile frustum since the clip texture uses nearest
		// filtering instead of bilinear filtering and hence we're not removing a seam between
		// tiles (instead we are clipping adjacent tiles).
		GLMatrix clip_texture_matrix(GLTextureUtils::get_clip_texture_clip_space_to_texture_space_transform());
		// Set up the texture matrix to perform model-view and projection transforms of the frustum.
		clip_texture_matrix.gl_mult_matrix(clip_projection_transform.get_matrix());
		clip_texture_matrix.gl_mult_matrix(view_transform.get_matrix());

		// Transfer clip texture transform to shader program.
		GLfloat clip_texture_float_matrix[16];
		clip_texture_matrix.get_float_matrix(clip_texture_float_matrix);
		gl.UniformMatrix4fv(
				d_render_tile_to_scene_program->get_uniform_location(gl, "clip_texture_transform"),
				1, GL_FALSE/*transpose*/, clip_texture_float_matrix);
	}

#if 0
	// Used to render as wire-frame meshes instead of filled textured meshes for visualising mesh density.
	gl.PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
}


double
GPlatesOpenGL::GLMultiResolutionRasterMapView::get_viewport_pixel_size_in_map_projection(
		const GLViewProjection &view_projection) const
{
	// The map-projection coordinates of a viewport pixel (values at bottom-left and bottom-right corners of pixel).
	double objx[2];
	double objy[2];
	double objz[2];

	// Get map-projection coordinates of bottom-left viewport pixel.
	// The depth doesn't actually matter since it's a 2D orthographic projection
	// (for 'projection_transform' as opposed to the map projection which is handled elsewhere).
	if (!view_projection.glu_un_project(
		// Bottom-left viewport pixel...
		// The 'z' value does matter in 2D orthographic projection
		// (for 'projection_transform' as opposed to the map projection).
		0, 0, 0,
		&objx[0], &objy[0], &objz[0]))
	{
		qWarning() << "GLMultiResolutionRasterMapView::get_viewport_pixel_size_in_map_projection: "
			<< "glu_un_project() failed: using lowest resolution view.";

		return ERROR_VIEWPORT_PIXEL_SIZE_IN_MAP_PROJECTION;
	}

	// Get map-projection coordinates of bottom-left viewport pixel.
	// The depth doesn't actually matter since it's a 2D orthographic projection
	// (for 'projection_transform' as opposed to the map projection which is handled elsewhere).
	if (!view_projection.glu_un_project(
		// Bottom-left viewport pixel plus one in x-direction...
		// The 'z' value does matter in 2D orthographic projection
		// (for 'projection_transform' as opposed to the map projection).
		1, 0, 0,
		&objx[1], &objy[1], &objz[1]))
	{
		qWarning() << "GLMultiResolutionRasterMapView::get_viewport_pixel_size_in_map_projection: "
			<< "glu_un_project() failed: using lowest resolution view.";

		return ERROR_VIEWPORT_PIXEL_SIZE_IN_MAP_PROJECTION;
	}

	// Return the 2D distance, in map projection coordinates, across the width of a single viewport pixel.

	double delta_objx = objx[1] - objx[0];
	double delta_objy = objy[1] - objy[0];

	return std::sqrt(delta_objx * delta_objx + delta_objy * delta_objy);
}


void
GPlatesOpenGL::GLMultiResolutionRasterMapView::compile_link_shader_program(
		GL &gl)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope (eg, failure to compile/link shader).
	TRACK_CALL_STACK();

	// Make sure we leave the OpenGL global state the way it was.
	GL::StateScope save_restore_state(gl);

	// Vertex shader source.
	GLShaderSource vertex_shader_source;
	vertex_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	vertex_shader_source.add_code_segment(RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE);

	// Vertex shader.
	GLShader::shared_ptr_type vertex_shader = GLShader::create(gl, GL_VERTEX_SHADER);
	vertex_shader->shader_source(gl, vertex_shader_source);
	vertex_shader->compile_shader(gl);

	// Fragment shader source.
	GLShaderSource fragment_shader_source;
	fragment_shader_source.add_code_segment_from_file(GLShaderSource::UTILS_FILE_NAME);
	fragment_shader_source.add_code_segment(RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE);

	// Fragment shader.
	GLShader::shared_ptr_type fragment_shader = GLShader::create(gl, GL_FRAGMENT_SHADER);
	fragment_shader->shader_source(gl, fragment_shader_source);
	fragment_shader->compile_shader(gl);

	// Vertex-fragment program.
	d_render_tile_to_scene_program->attach_shader(gl, vertex_shader);
	d_render_tile_to_scene_program->attach_shader(gl, fragment_shader);
	d_render_tile_to_scene_program->link_program(gl);

	//
	// Set some shader program constants that don't change.
	//

	gl.UseProgram(d_render_tile_to_scene_program);

	// Use texture unit 0 for tile texture.
	gl.Uniform1i(
			d_render_tile_to_scene_program->get_uniform_location(gl, "tile_texture_sampler"),
			0/*texture unit*/);
	// Use texture unit 1 for clip texture.
	gl.Uniform1i(
			d_render_tile_to_scene_program->get_uniform_location(gl, "clip_texture_sampler"),
			1/*texture unit*/);
}
