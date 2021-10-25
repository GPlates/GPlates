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
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>

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
#include "GLProjectionUtils.h"
#include "GLRenderer.h"
#include "GLShaderProgramUtils.h"
#include "GLShaderSource.h"
#include "GLTexture.h"
#include "GLTextureUtils.h"
#include "GLViewport.h"

#include "maths/MathsUtils.h"
#include "maths/Rotation.h"

#include "utils/Profile.h"

// Enables visually showing level-of-detail in the map view.
//#define DEBUG_LEVEL_OF_DETAIL_VISUALLY

// Renders text displaying the level-of-detail of tiles, otherwise renders a checkerboard pattern
// to visualise texel-to-pixel mapping ratios.
// NOTE: 'DEBUG_LEVEL_OF_DETAIL_VISUALLY' must be defined.
//#define DEBUG_LEVEL_OF_DETAIL_WITH_TEXT


namespace GPlatesOpenGL
{
	namespace
	{
		//! Vertex shader source code to render a tile to the scene.
		const QString RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE_FILE_NAME =
				":/opengl/multi_resolution_raster_map_view/render_tile_to_scene_vertex_shader.glsl";

		/**
		 * Fragment shader source code to render a tile to the scene.
		 */
		const QString RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE_FILE_NAME =
				":/opengl/multi_resolution_raster_map_view/render_tile_to_scene_fragment_shader.glsl";


		/**
		 * Visualise the level-of-detail of a tile (either as text or a checkerboard pattern).
		 */
#ifdef DEBUG_LEVEL_OF_DETAIL_VISUALLY
		void
		visualise_level_of_detail_in_texture(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &tile_texture,
				unsigned int level_of_detail)
		{
			const unsigned int tile_texel_dimension = tile_texture->get_width().get();

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
			debug_image = debug_image.convertToFormat(QImage::Format_ARGB32);

			// Load cached image into raster texture.
			GLTextureUtils::load_argb32_qimage_into_rgba8_texture_2D(
					renderer,
					boost::const_pointer_cast<GLTexture>(tile_texture),
					debug_image,
					0, 0);
		}
#endif
	}
}

const double GPlatesOpenGL::GLMultiResolutionRasterMapView::ERROR_VIEWPORT_PIXEL_SIZE_IN_MAP_PROJECTION = 360.0;


GPlatesOpenGL::GLMultiResolutionRasterMapView::GLMultiResolutionRasterMapView(
		GLRenderer &renderer,
		const GLMultiResolutionCubeRasterInterface::non_null_ptr_type &multi_resolution_cube_raster,
		const GLMultiResolutionMapCubeMesh::non_null_ptr_to_const_type &multi_resolution_map_cube_mesh) :
	d_multi_resolution_cube_raster(multi_resolution_cube_raster),
	d_multi_resolution_map_cube_mesh(multi_resolution_map_cube_mesh),
	d_tile_texel_dimension(multi_resolution_cube_raster->get_tile_texel_dimension()),
	d_inverse_tile_texel_dimension(1.0f / multi_resolution_cube_raster->get_tile_texel_dimension()),
	d_map_projection_central_meridian_longitude(0)
{
	create_shader_programs(renderer);
}


bool
GPlatesOpenGL::GLMultiResolutionRasterMapView::render(
		GLRenderer &renderer,
		cache_handle_type &cache_handle)
{
	PROFILE_FUNC();

	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// First see if the map projection central meridian has changed.
	//
	// NOTE: If the projection *type* changes then we don't need to change our world transform.
	const double updated_map_projection_central_meridian_longitude =
			d_multi_resolution_map_cube_mesh->get_current_map_projection_settings().get_central_llp().longitude();
	if (!GPlatesMaths::are_almost_exactly_equal(
			d_map_projection_central_meridian_longitude, updated_map_projection_central_meridian_longitude))
	{
		d_map_projection_central_meridian_longitude = updated_map_projection_central_meridian_longitude;

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
	const double viewport_pixel_size_in_map_projection =
			get_viewport_pixel_size_in_map_projection(
					renderer.gl_get_viewport(),
					renderer.gl_get_matrix(GL_MODELVIEW),
					renderer.gl_get_matrix(GL_PROJECTION));

	// The size of a tile of viewport pixels projected onto the map (ie, in map-projection coordinates).
	// A tile is 'd_tile_texel_dimension' x 'd_tile_texel_dimension' pixels. When a tile of texels
	// in the map projection matches this then the correct level-of-detail has been found.
	const double viewport_tile_map_projection_size =
			d_tile_texel_dimension * viewport_pixel_size_in_map_projection;

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
				renderer,
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
		GLRenderer &renderer,
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
				renderer,
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
					renderer,
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
		GLRenderer &renderer,
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
	const boost::optional<GLTexture::shared_ptr_to_const_type> tile_texture_opt =
			source_raster_quad_tree_node.get_tile_texture(
					renderer,
					source_raster_cache_handle);
	// If there is no tile texture it means there's nothing to be drawn (eg, no raster covering this node).
	if (!tile_texture_opt)
	{
		//qDebug() << "***************** Nothing render into cube quad tree tile *****************";
		return;
	}
	const GLTexture::shared_ptr_to_const_type tile_texture = tile_texture_opt.get();

#if 0
	qDebug()
		<< "Rendered cube quad tree tile: " << num_tiles_rendered_to_scene
		<< " at LOD: " << cube_subdivision_cache_node.get_level_of_detail();
#endif
#ifdef DEBUG_LEVEL_OF_DETAIL_VISUALLY
	visualise_level_of_detail_in_texture(
			renderer,
			tile_texture,
			cube_subdivision_cache_node.get_level_of_detail());
#endif

	// Make sure we return the cached handle to our caller so they can cache it.
	cached_tiles.push_back(source_raster_cache_handle);


	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

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

	// This is the only place we increment the rendered tile count.
	++num_tiles_rendered_to_scene;
}


void
GPlatesOpenGL::GLMultiResolutionRasterMapView::set_tile_state(
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

	// Use shader program (if supported), otherwise the fixed-function pipeline.
	if (d_render_tile_to_scene_program_object && d_render_tile_to_scene_with_clipping_program_object)
	{
		// If we've traversed deep enough into the cube quad tree then the cube quad tree mesh
		// then the drawable starts to cover multiple quad tree nodes (instead of a single node)
		// so we need to use a clip texture to mask off all but the node we're interested in.
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
			// Load texture transform into texture unit 1.
			renderer.gl_load_texture_matrix(GL_TEXTURE1, clip_texture_matrix);

			// Bind the clip texture to texture unit 1.
			renderer.gl_bind_texture(d_multi_resolution_map_cube_mesh->get_clip_texture(), GL_TEXTURE1, GL_TEXTURE_2D);


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
		// NOTE: Since our texture coordinates are 3D and contain the original point-on-sphere positions
		// (before map projection) we don't need to set up texture coordinate generation from the vertices (x,y,z).

		// However for the fixed-function pipeline clipping is not supported.
		// We would need a second set of texture coordinates in the vertices that the clip texture
		// transform could apply to - but the vertices come from GLMultiResolutionMapCubeMesh and
		// it's too intrusive to add vertex variations in there - besides most hardware should have
		// basic support for shaders.
		if (clip_to_tile_frustum)
		{
			// Only emit warning message once.
			static bool emitted_warning = false;
			if (!emitted_warning)
			{
				qWarning() <<
						"High zoom levels of map view NOT supported by this graphics hardware - \n"
						"  requires shader programs - visual results will be incorrect.\n"
						"  Most graphics hardware supports this - software renderer fallback "
						"  might have occurred - possibly via remote desktop software.";
				emitted_warning = true;
			}
		}
	}

#if 0
	// Used to render as wire-frame meshes instead of filled textured meshes for
	// visualising mesh density.
	renderer.gl_polygon_mode(GL_FRONT_AND_BACK, GL_LINE);
#endif
}


double
GPlatesOpenGL::GLMultiResolutionRasterMapView::get_viewport_pixel_size_in_map_projection(
		const GLViewport &viewport,
		const GLMatrix &model_view_transform,
		const GLMatrix &projection_transform) const
{
	// The map-projection coordinates of a viewport pixel (values at bottom-left and bottom-right corners of pixel).
	double objx[2];
	double objy[2];
	double objz[2];

	// Get map-projection coordinates of bottom-left viewport pixel.
	// The depth doesn't actually matter since it's a 2D orthographic projection
	// (for 'projection_transform' as opposed to the map projection which is handled elsewhere).
	if (GLProjectionUtils::glu_un_project(
		viewport,
		model_view_transform,
		projection_transform,
		// Bottom-left viewport pixel...
		// The 'z' value does matter in 2D orthographic projection
		// (for 'projection_transform' as opposed to the map projection).
		0, 0, 0,
		&objx[0], &objy[0], &objz[0]) == GL_FALSE)
	{
		qWarning() << "GLMultiResolutionRasterMapView::get_viewport_pixel_size_in_map_projection: "
			<< "glu_un_project() failed: using lowest resolution view.";

		return ERROR_VIEWPORT_PIXEL_SIZE_IN_MAP_PROJECTION;
	}

	// Get map-projection coordinates of bottom-left viewport pixel.
	// The depth doesn't actually matter since it's a 2D orthographic projection
	// (for 'projection_transform' as opposed to the map projection which is handled elsewhere).
	if (GLProjectionUtils::glu_un_project(
		viewport,
		model_view_transform,
		projection_transform,
		// Bottom-left viewport pixel plus one in x-direction...
		// The 'z' value does matter in 2D orthographic projection
		// (for 'projection_transform' as opposed to the map projection).
		1, 0, 0,
		&objx[1], &objy[1], &objz[1]) == GL_FALSE)
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
GPlatesOpenGL::GLMultiResolutionRasterMapView::create_shader_programs(
		GLRenderer &renderer)
{
	// Shader programs to render tiles to the scene.
	// We fallback to the fixed-function pipeline when shader programs are not supported but
	// we don't clip with the fixed-function pipeline which was the reason for using shader programs.
	// The clipping is only needed for high zoom levels so for reasonable zoom levels the fixed-function
	// pipeline (available on all hardware) should render fine.

	const bool is_floating_point_source_raster =
			GLTexture::is_format_floating_point(d_multi_resolution_cube_raster->get_tile_texture_internal_format());

	//
	// A version without clipping.
	//

	GLShaderSource render_tile_to_scene_without_clipping_fragment_shader_source;

	// Add the '#define's first.
	if (is_floating_point_source_raster)
	{
		// Configure shader for floating-point rasters.
		render_tile_to_scene_without_clipping_fragment_shader_source.add_code_segment(
				"#define SOURCE_RASTER_IS_FLOATING_POINT\n");
	}

	// Then add the GLSL function to bilinearly interpolate.
	render_tile_to_scene_without_clipping_fragment_shader_source.add_code_segment_from_file(
			GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);

	// Then add the GLSL 'main()' function.
	render_tile_to_scene_without_clipping_fragment_shader_source.add_code_segment_from_file(
			RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	d_render_tile_to_scene_program_object =
			GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
					renderer,
					GLShaderSource::create_shader_source_from_file(
							RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE_FILE_NAME),
					render_tile_to_scene_without_clipping_fragment_shader_source);

	if (d_render_tile_to_scene_program_object &&
		is_floating_point_source_raster)
	{
		// We need to setup for bilinear filtering of floating-point texture in the fragment shader.
		// Set the source tile texture dimensions (and inverse dimensions).
		// This uniform is constant (only needs to be reloaded if shader program is re-linked).
		d_render_tile_to_scene_program_object.get()->gl_uniform4f(
				renderer,
				"source_texture_dimensions",
				d_tile_texel_dimension, d_tile_texel_dimension,
				d_inverse_tile_texel_dimension, d_inverse_tile_texel_dimension);
	}

	//
	// A version with clipping.
	//

	GLShaderSource render_tile_to_scene_with_clipping_fragment_shader_source;

	// Add the '#define's first.
	render_tile_to_scene_with_clipping_fragment_shader_source.add_code_segment("#define ENABLE_CLIPPING\n");
	if (is_floating_point_source_raster)
	{
		// Configure shader for floating-point rasters.
		render_tile_to_scene_with_clipping_fragment_shader_source.add_code_segment(
				"#define SOURCE_RASTER_IS_FLOATING_POINT\n");
	}

	// Then add the GLSL function to bilinearly interpolate.
	render_tile_to_scene_with_clipping_fragment_shader_source.add_code_segment_from_file(
			GLShaderProgramUtils::UTILS_SHADER_SOURCE_FILE_NAME);

	// Then add the GLSL 'main()' function.
	render_tile_to_scene_with_clipping_fragment_shader_source.add_code_segment_from_file(
			RENDER_TILE_TO_SCENE_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	// Create the program object.
	d_render_tile_to_scene_with_clipping_program_object =
			GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
					renderer,
					GLShaderSource::create_shader_source_from_file(
							RENDER_TILE_TO_SCENE_VERTEX_SHADER_SOURCE_FILE_NAME),
					render_tile_to_scene_with_clipping_fragment_shader_source);

	if (d_render_tile_to_scene_with_clipping_program_object &&
		is_floating_point_source_raster)
	{
		// We need to setup for bilinear filtering of floating-point texture in the fragment shader.
		// Set the source tile texture dimensions (and inverse dimensions).
		// This uniform is constant (only needs to be reloaded if shader program is re-linked).
		d_render_tile_to_scene_with_clipping_program_object.get()->gl_uniform4f(
				renderer,
				"source_texture_dimensions",
				d_tile_texel_dimension, d_tile_texel_dimension,
				d_inverse_tile_texel_dimension, d_inverse_tile_texel_dimension);
	}
}
