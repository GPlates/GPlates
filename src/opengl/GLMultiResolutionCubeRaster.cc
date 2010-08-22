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

#include <boost/foreach.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>

#include "GLMultiResolutionCubeRaster.h"

#include "GLCompositeStateSet.h"
#include "GLContext.h"
#include "GLRenderer.h"
#include "GLRenderTargetType.h"
#include "GLTextureEnvironmentState.h"
#include "GLTransformState.h"
#include "GLUtils.h"
#include "GLViewport.h"
#include "GLViewportState.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


GPlatesOpenGL::GLMultiResolutionCubeRaster::GLMultiResolutionCubeRaster(
		const GLMultiResolutionRaster::non_null_ptr_type &multi_resolution_raster,
		const GLCubeSubdivision::non_null_ptr_to_const_type &cube_subdivision,
		const GLTextureResourceManager::shared_ptr_type &texture_resource_manager,
		float source_raster_level_of_detail_bias) :
	d_multi_resolution_raster(multi_resolution_raster),
	d_source_raster_valid_token(multi_resolution_raster->get_current_valid_token()),
	d_cube_subdivision(cube_subdivision),
	d_tile_texel_dimension(cube_subdivision->get_tile_texel_dimension()),
	d_source_raster_level_of_detail_bias(source_raster_level_of_detail_bias),
	d_texture_cache(create_texture_cache(200, texture_resource_manager)),
	d_clear_buffers_state(GLClearBuffersState::create()),
	d_clear_buffers(GLClearBuffers::create())
{
	// Setup for clearing the render target colour buffer.
	d_clear_buffers_state->gl_clear_color(); // Clear colour to all zeros.
	d_clear_buffers->gl_clear(GL_COLOR_BUFFER_BIT); // Clear only the colour buffer.

	initialise_cube_quad_trees();
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::QuadTreeNode>
GPlatesOpenGL::GLMultiResolutionCubeRaster::get_root_quad_tree_node(
		GLCubeSubdivision::CubeFaceType cube_face)
{

	if (!d_cube.faces[cube_face].quad_tree.root_node)
	{
		return boost::none;
	}

	return QuadTreeNode(*d_cube.faces[cube_face].quad_tree.root_node.get());
}


GPlatesOpenGL::GLTextureUtils::ValidToken
GPlatesOpenGL::GLMultiResolutionCubeRaster::get_current_valid_token()
{
	update_raster_source_valid_token();

	// We'll just use the valid token of the raster source - if they don't change then neither do we.
	// If we had two inputs sources then we'd have to have our own valid token.
	return d_source_raster_valid_token;
}


void
GPlatesOpenGL::GLMultiResolutionCubeRaster::update_raster_source_valid_token()
{
	d_source_raster_valid_token = d_multi_resolution_raster->get_current_valid_token();
}


GPlatesOpenGL::GLTexture::shared_ptr_to_const_type
GPlatesOpenGL::GLMultiResolutionCubeRaster::get_tile_texture(
		tile_handle_type tile_handle,
		GLRenderer &renderer)
{
	// Make sure our cached version of the raster source's valid token is up to date
	// so our texture tiles can decide whether they need to reload their caches.
	update_raster_source_valid_token();

	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			tile_handle < d_tiles.size(),
			GPLATES_ASSERTION_SOURCE);

	const QuadTreeNodeImpl::non_null_ptr_type &tile = d_tiles[tile_handle];

	//
	// Get the texture for the tile.
	//

	// See if we've generated our tile texture and
	// see if it hasn't been recycled by the texture cache.
	boost::optional<GLTexture::shared_ptr_type> tile_texture = tile->render_texture.get_object();
	if (!tile_texture)
	{
		// We need to allocate a new texture from the texture cache and fill it with data.
		const std::pair<GLVolatileTexture, bool/*recycled*/> volatile_texture_result =
				d_texture_cache->allocate_object();

		// Extract allocation results.
		tile->render_texture = volatile_texture_result.first;
		const bool texture_was_recycled = volatile_texture_result.second;

		// Get the tile texture again - this time it should have a valid texture.
		tile_texture = tile->render_texture.get_object();
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				tile_texture,
				GPLATES_ASSERTION_SOURCE);

		if (!texture_was_recycled)
		{
			// If texture just allocated then we need to create it in OpenGL.
			create_tile_texture(tile_texture.get());
		}

		// Render the source raster into our tile texture.
		render_raster_data_into_tile_texture(
				*tile,
				tile_texture.get(),
				renderer);
	}

	// Our texture wasn't recycled but see if it's still valid in case the source
	// raster changed the data underneath us.
	if (!tile->source_texture_valid_token.is_still_valid(d_source_raster_valid_token))
	{
		// Render the source raster into our tile texture.
		render_raster_data_into_tile_texture(
				*tile,
				tile_texture.get(),
				renderer);
	}

	return tile_texture.get();
}


void
GPlatesOpenGL::GLMultiResolutionCubeRaster::render_raster_data_into_tile_texture(
		const QuadTreeNodeImpl &tile,
		const GLTexture::shared_ptr_type &texture,
		GLRenderer &renderer)
{
	// Push a render target that will render to the tile texture.
	renderer.push_render_target(
			GLTextureRenderTargetType::create(
					texture, d_tile_texel_dimension, d_tile_texel_dimension));

	// Create a state set to set the viewport.
	const GLViewport viewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension);
	GLViewportState::non_null_ptr_type viewport_state = GLViewportState::create(viewport);
	// Push the viewport state set.
	renderer.push_state_set(viewport_state);
	// Let the transform state know of the new viewport.
	// This is necessary since it is used to determine pixel projections in world space
	// which are in turn used for level-of-detail selection for rasters.
	renderer.get_transform_state().set_viewport(viewport);

	// Clear the colour buffer of the render target.
	renderer.push_state_set(d_clear_buffers_state);
	renderer.add_drawable(d_clear_buffers);
	renderer.pop_state_set();

	renderer.push_transform(*tile.projection_transform);
	renderer.push_transform(*tile.view_transform);

	// Get the source raster to render into the render target using the view frustum
	// we have provided. We have already cached the visible source raster tiles that need to be
	// rendered into our frustum to save it a bit of culling work.
	d_multi_resolution_raster->render(
			renderer,
			tile.src_raster_tiles);

	renderer.pop_transform();
	renderer.pop_transform();

	// Pop the viewport state set.
	renderer.pop_state_set();

	renderer.pop_render_target();

	// This tile texture is now update-to-date.
	tile.source_texture_valid_token = d_source_raster_valid_token;
}


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesOpenGL::GLMultiResolutionCubeRaster::create_tile_texture(
		const GLTexture::shared_ptr_type &texture)
{
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

	// Clamp texture coordinates to centre of edge texels - it's easier for hardware.
	// We don't really need it though since we adjust our frustums so they lie
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

ENABLE_GCC_WARNING("-Wold-style-cast")


void
GPlatesOpenGL::GLMultiResolutionCubeRaster::initialise_cube_quad_trees()
{
	for (unsigned int face = 0; face < 6; ++face)
	{
		GLCubeSubdivision::CubeFaceType cube_face =
				static_cast<GLCubeSubdivision::CubeFaceType>(face);

		// Create a transform state so we can cache the coverage of the source raster in our
		// quad tree subdivisions. The cache is in the form of source raster tile handles.
		GLTransformState::non_null_ptr_type transform_state = GLTransformState::create();

		// Set the viewport - it only needs to be done once since all subdivisions
		// use the same tile texture dimension.
		const GLViewport viewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension);
		transform_state->set_viewport(viewport);

		// Recursively generate a quad tree for the current cube face.
		boost::optional<QuadTreeNodeImpl::non_null_ptr_type> quad_tree_root_node =
				create_quad_tree_node(
						cube_face,
						*transform_state,
						0,/*level_of_detail*/
						0,/*tile_u_offset*/
						0/*tile_v_offset*/);

		// If the source raster covered any part of the current cube face.
		if (quad_tree_root_node)
		{
			d_cube.faces[cube_face].quad_tree.root_node = quad_tree_root_node;
		}
	}
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::QuadTreeNodeImpl::non_null_ptr_type>
GPlatesOpenGL::GLMultiResolutionCubeRaster::create_quad_tree_node(
		GLCubeSubdivision::CubeFaceType cube_face,
		GLTransformState &transform_state,
		unsigned int level_of_detail,
		unsigned int tile_u_offset,
		unsigned int tile_v_offset)
{
	//
	// Setup the model-view and projection matrices and setup the viewport.
	// These are required when the source raster looks for tiles visible in the view frustum
	// of our quad tree subdivision cell (the viewport is used to determine the correct
	// level-of-detail tiles to use).
	//

	// Set the projection transform.
	const GLTransform::non_null_ptr_to_const_type projection_transform =
			d_cube_subdivision->get_projection_transform(
					cube_face, level_of_detail, tile_u_offset, tile_v_offset);
	transform_state.load_transform(*projection_transform);

	// Set the view transform.
	const GLTransform::non_null_ptr_to_const_type view_transform =
			d_cube_subdivision->get_view_transform(
					cube_face, level_of_detail, tile_u_offset, tile_v_offset);
	transform_state.load_transform(*view_transform);

	// Get the source tiles that are visible in the current view frustum.
	std::vector<GLMultiResolutionRaster::tile_handle_type> source_raster_tile_handles;
	const float source_raster_level_of_detail = d_multi_resolution_raster->get_visible_tiles(
			transform_state, source_raster_tile_handles, d_source_raster_level_of_detail_bias);

	// If there are no tiles it means the source raster does not have global extents
	// and we are looking at a part of the globe not covered by it.
	if (source_raster_tile_handles.empty())
	{
		return boost::none;
	}

	// Create a quad tree node.
	const tile_handle_type tile_handle = d_tiles.size();
	QuadTreeNodeImpl::non_null_ptr_type quad_tree_node = QuadTreeNodeImpl::create(
			tile_handle, projection_transform, view_transform);
	d_tiles.push_back(quad_tree_node);

	// Initialise the quad tree node's source tiles.
	quad_tree_node->src_raster_tiles.swap(source_raster_tile_handles);

	// If we have reached the highest resolution level of detail in the source raster
	// then we don't need to create any child quad tree nodes.
	// When the LOD first goes negative it means we have a render texture / render frustum
	// that has enough resolution to contain the highest resolution the source raster can provide.
	if (source_raster_level_of_detail <= 0)
	{
		return quad_tree_node;
	}


	//
	// Iterate over the child subdivision regions and create if cover source raster.
	//

	const unsigned int child_level_of_detail = level_of_detail + 1;

	for (unsigned int child_v_offset = 0; child_v_offset < 2; ++child_v_offset)
	{
		for (unsigned int child_u_offset = 0; child_u_offset < 2; ++child_u_offset)
		{
			quad_tree_node->child_nodes[child_v_offset][child_u_offset] =
					create_quad_tree_node(
							cube_face,
							transform_state,
							child_level_of_detail,
							2 * tile_u_offset + child_u_offset,
							2 * tile_v_offset + child_v_offset);
		}
	}

	return quad_tree_node;
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::QuadTreeNode>
GPlatesOpenGL::GLMultiResolutionCubeRaster::QuadTreeNode::get_child_node(
		unsigned int child_v_offset,
		unsigned int child_u_offset) const
{
	const boost::optional<QuadTreeNodeImpl::non_null_ptr_type> &child_node =
			impl->child_nodes[child_v_offset][child_u_offset];
	if (!child_node)
	{
		return boost::none;
	}

	return QuadTreeNode(*child_node.get());
}
