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
#include <QDebug>

#include "GLMultiResolutionCubeReconstructedRaster.h"

#include "GLContext.h"
#include "GLTexture.h"
#include "GLUtils.h"
#include "GLViewport.h"
#include "OpenGLException.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/Base2Utils.h"
#include "utils/Profile.h"


const unsigned int GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::MIN_TILE_TEXEL_DIMENSION = 256;


GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::GLMultiResolutionCubeReconstructedRaster(
		GL &gl,
		const GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type &source_reconstructed_raster,
		bool cache_tile_textures) :
	d_reconstructed_raster(source_reconstructed_raster),
	d_level_of_detail_offset_for_scaled_tile_dimension(0),
	d_tile_texel_dimension(
			update_tile_texel_dimension(
					gl,
					source_reconstructed_raster->get_tile_texel_dimension())),
	// Start with small size cache and just let the cache grow in size as needed (if caching enabled)...
	d_texture_cache(tile_texture_cache_type::create(2/* GPU pipeline breathing room in case caching disabled*/)),
	d_cache_tile_textures(cache_tile_textures),
	d_tile_framebuffer(GLFramebuffer::create(gl)),
	d_have_checked_tile_framebuffer_completeness(false),
	d_cube_subdivision(
			// Expand the tile frustums by half a texel around the border of each frustum.
			// This causes the texel centres of the border tile texels to fall right on the edge
			// of the unmodified frustum which means adjacent tiles will have the same colour after
			// bilinear filtering and hence there will be no visible colour seams (or discontinuities
			// in the raster data if the source raster is floating-point).
			// The nice thing is this works for both bilinear filtering and nearest neighbour filtering
			// (ie, there'll be no visible seams in nearest neighbour filtering either).
			GLCubeSubdivision::create(
					GLCubeSubdivision::get_expand_frustum_ratio(
							d_tile_texel_dimension,
							0.5/* half a texel */))),
	d_cube_quad_tree(cube_quad_tree_type::create())
{
}


unsigned int
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::update_tile_texel_dimension(
		GL &gl,
		unsigned int tile_texel_dimension)
{
	// If tile dimensions are too small then we end up requiring a lot more tiles to render since
	// there's no limit on how deep we can render (see 'QuadTreeNodeImp::is_leaf_node()' for more details).
	//
	// To fix this we keep doubling the tile dimensions until they exceed a minimum (and without
	// exceeding the maximum texture size).
	d_level_of_detail_offset_for_scaled_tile_dimension = 0;
	while (tile_texel_dimension < MIN_TILE_TEXEL_DIMENSION)
	{
		// Make sure the doubled tile dimension does not exceed the maximum texture size.
		// We're requiring the final multiplier to be a power-of-two so that the level-of-detail
		// adjustment is an integer (so we can render at an exact LOD level).
		if (2 * tile_texel_dimension > gl.get_capabilities().gl_max_texture_size)
		{
			break;
		}

		tile_texel_dimension = 2 * tile_texel_dimension;
		--d_level_of_detail_offset_for_scaled_tile_dimension;
	}

	return tile_texel_dimension;
}


void
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::set_world_transform(
		const GLMatrix &world_transform)
{
	// If the world transform has changed then set it, and mark all our texture tiles dirty.
	if (d_world_transform != world_transform)
	{
		d_world_transform = world_transform;

		// Iterate over all the nodes in the cube quad tree and reset the observer tokens.
		// This will force an update when the textures are subsequently requested.
		cube_quad_tree_type::iterator cube_quad_tree_node_iter = d_cube_quad_tree->get_iterator();
		for ( ; !cube_quad_tree_node_iter.finished(); cube_quad_tree_node_iter.next())
		{
			CubeQuadTreeNode &cube_quad_tree_node = cube_quad_tree_node_iter.get_element();
			cube_quad_tree_node.d_source_texture_observer_token.reset();
		}

		// Let any clients know that they're now out-of-date (since our cube map texture has a new orientation).
		d_subject_token.invalidate();
	}
}


const GPlatesUtils::SubjectToken &
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::get_subject_token() const
{
	//
	// This covers changes to the inputs that don't require completely re-creating the inputs.
	// That is beyond our scope and is detected and managed by our owners (and owners of our inputs).
	//

	// If the source raster has changed.
	if (!d_reconstructed_raster->get_subject_token().is_observer_up_to_date(
				d_reconstructed_raster_observer_token))
	{
		d_subject_token.invalidate();

		d_reconstructed_raster->get_subject_token().update_observer(
				d_reconstructed_raster_observer_token);
	}

	return d_subject_token;
}


boost::optional<GPlatesOpenGL::GLTexture::shared_ptr_type>
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::get_tile_texture(
		GL &gl,
		const CubeQuadTreeNode &tile,
		cache_handle_type &cache_handle)
{
	//
	// Get the texture for the tile.
	//

	// Is true if reconstructed raster is visible in current node's frustum.
	// The default (a valid cached texture) is visible.
	bool visible = true;

	// See if we've generated our tile texture and
	// see if it hasn't been recycled by the texture cache.
	tile_texture_cache_type::object_shared_ptr_type tile_texture = tile.d_tile_texture->get_cached_object();
	if (!tile_texture)
	{
		tile_texture = tile.d_tile_texture->recycle_an_unused_object();
		if (!tile_texture)
		{
			// Create a new tile texture.
			tile_texture = tile.d_tile_texture->set_cached_object(
					std::unique_ptr<TileTexture>(new TileTexture(gl)),
					// Called whenever tile texture is returned to the cache...
					boost::bind(&TileTexture::returned_to_cache, boost::placeholders::_1));

			// The texture was just allocated so we need to create it in OpenGL.
			create_tile_texture(gl, tile_texture->texture);

			//qDebug() << "GLMultiResolutionCubeReconstructedRaster: " << d_texture_cache->get_current_num_objects_in_use();
		}

		// Render the source raster into our tile texture.
		visible = render_raster_data_into_tile_texture(
				gl,
				tile,
				*tile_texture);
	}
	// Our texture wasn't recycled but see if it's still valid in case the source
	// raster changed the data underneath us.
	else if (!d_reconstructed_raster->get_subject_token().is_observer_up_to_date(
				tile.d_source_texture_observer_token))
	{
		// Render the source raster into our tile texture.
		visible = render_raster_data_into_tile_texture(
				gl,
				tile,
				*tile_texture);
	}

	// The caller will cache this tile to keep it from being prematurely recycled by our caches.
	// Also the cached data accumulated by the reconstructed raster renderer will be added to the cache.
	cache_handle = boost::shared_ptr<ClientCacheTile>(
			new ClientCacheTile(
					tile_texture,
					// Only cache the tile texture if the client has requested it...
					d_cache_tile_textures &&
						// If the nothing was rendered into the tile then we don't want to return the
						// unused texture to the caller for caching - this way it'll get returned to
						// the texture cache for reuse...
						visible));

	// If nothing was rendered then inform the caller.
	if (!visible)
	{
		return boost::none;
	}

	return tile_texture->texture;
}


bool
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::render_raster_data_into_tile_texture(
		GL &gl,
		const CubeQuadTreeNode &tile,
		TileTexture &tile_texture)
{
	PROFILE_FUNC();

	// Determine if anything was rendered.
	bool rendered = false;

	// Make sure we leave the OpenGL state the way it was.
	GL::StateScope save_restore_state(
			gl,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);

	// Bind our framebuffer object for rendering to tile textures.
	gl.BindFramebuffer(GL_FRAMEBUFFER, d_tile_framebuffer);

	// Begin rendering to the 2D target tile texture.
	gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tile_texture.texture, 0/*level*/);

	// Check our framebuffer object for completeness (now that a tile texture is attached to it).
	// We only need to do this once because, while the tile texture changes, the framebuffer configuration
	// does not (ie, same texture internal format, dimensions, etc).
	if (!d_have_checked_tile_framebuffer_completeness)
	{
		// Throw OpenGLException if not complete.
		// This should succeed since we should only be using texture formats that are required by OpenGL 3.3 core.
		const GLenum completeness = gl.CheckFramebufferStatus(GL_FRAMEBUFFER);
		GPlatesGlobal::Assert<OpenGLException>(
				completeness == GL_FRAMEBUFFER_COMPLETE,
				GPLATES_ASSERTION_SOURCE,
				"Framebuffer not complete for rendering multi-resolution cube reconstructed raster tiles.");

		d_have_checked_tile_framebuffer_completeness = true;
	}

	// Specify a viewport that matches the tile dimensions.
	gl.Viewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension);

	// Clear the render target (only has colour, no depth/stencil).
	gl.ClearColor();
	gl.Clear(GL_COLOR_BUFFER_BIT);

	// The view transform of the current tile.
	GLMatrix tile_view_matrix = tile.d_view_transform->get_matrix();
	// Multiply in the requested world transform.
	tile_view_matrix.gl_mult_matrix(d_world_transform);

	// The view projection of the current tile.
	const GLViewProjection tile_view_projection(
			GLViewport(0, 0, d_tile_texel_dimension, d_tile_texel_dimension),
			tile_view_matrix,
			tile.d_projection_transform->get_matrix());

	// Reconstruct source raster by rendering into the render target using the view frustum
	// we have provided and the level-of-detail we have calculated.
	GLMultiResolutionStaticPolygonReconstructedRaster::cache_handle_type source_cache_handle;
	if (d_reconstructed_raster->render(
			gl,
			tile_view_projection.get_view_projection_transform(),
			tile.d_level_of_detail,
			source_cache_handle))
	{
		rendered = true;
	}

	tile_texture.source_cache_handle = source_cache_handle;

	// This tile texture is now update-to-date with respect to the source raster.
	d_reconstructed_raster->get_subject_token().update_observer(
			tile.d_source_texture_observer_token);

	// Return true if anything was rendered into the current node's tile.
	return rendered;
}


float
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::get_level_of_detail(
		unsigned int quad_tree_depth) const
{
	// Since our (cube map) tile dimension is the same as the reconstructed raster's input
	// source (cube map) tile dimension we will just render the (input) source (cube map) raster
	// as the same level-of-detail (which means the same quad tree depth).
	//
	// UPDATE: Our (cube map) tile dimension can now be a power-of-two multiple of the reconstructed
	// raster's input source (cube map) tile dimension if the latter is found to be too small.
	// We account for this by adding a LOD offset to the final level-of-detail.
	//
	// NOTE: Previously we did the usual thing of passing our tile's modelview/projection matrices
	// and viewport to the reconstructed raster which, in turn, determined the level-of-detail
	// (quad tree depth) to render at. However, due to the non-uniformity of pixels across a cube
	// map face (about a factor of two), we ended up rendering too high a resolution (sometimes
	// an extra two levels too deep) which just slowed things down significantly (mostly due to the
	// extra input raster data that needed to be converted to colours by palette lookup).
	// Now the size of source tiles is roughly the same in the globe and map views.
	const float cube_quad_tree_depth = quad_tree_depth;

	// Need to convert cube quad tree depth to the level-of-detail recognised by the source raster.
	// See 'GLMultiResolutionStaticPolygonReconstructedRaster::get_level_of_detail()' for more details.
	//
	// We add a LOD offset to account for possible power-of-two scaling of input source tile dimension.
	const float level_of_detail = d_level_of_detail_offset_for_scaled_tile_dimension +
			(d_reconstructed_raster->get_num_levels_of_detail() - 1) - cube_quad_tree_depth;

	// Return the clamped level-of-detail to ensure it is within a valid range.
	return d_reconstructed_raster->clamp_level_of_detail(level_of_detail);
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::quad_tree_node_type>
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::get_quad_tree_root_node(
		GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face)
{
	const GPlatesMaths::CubeQuadTreeLocation cube_quad_tree_location(cube_face);

	// See if we have a root cube quad tree node.
	cube_quad_tree_type::node_type *cube_root_node =
			d_cube_quad_tree->get_quad_tree_root_node(cube_face);

	// If not then just create one.
	if (cube_root_node == NULL)
	{
		// The view transform for the current cube face.
		const GLTransform::non_null_ptr_to_const_type view_transform =
				d_cube_subdivision->get_view_transform(cube_face);

		// The projection transform for the root cube quad tree node.
		const GLTransform::non_null_ptr_to_const_type projection_transform =
				d_cube_subdivision->get_projection_transform(
						0/*level_of_detail*/,
						0/*tile_u_offset*/,
						0/*tile_v_offset*/);

		// Create a root quad tree node.
		d_cube_quad_tree->set_quad_tree_root_node(
				cube_face,
				d_cube_quad_tree->create_node(
						CubeQuadTreeNode(
								view_transform,
								projection_transform,
								get_level_of_detail(
										cube_quad_tree_location.get_node_location()->quad_tree_depth),
								d_texture_cache->allocate_volatile_object())));

		cube_root_node = d_cube_quad_tree->get_quad_tree_root_node(cube_face);
	}

	return quad_tree_node_type(
			GPlatesUtils::non_null_intrusive_ptr<quad_tree_node_type::ImplInterface>(
					new QuadTreeNodeImpl(*cube_root_node, *this, cube_quad_tree_location)));
}


boost::optional<GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::quad_tree_node_type>
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::get_child_node(
		const quad_tree_node_type &parent_node,
		unsigned int child_x_offset,
		unsigned int child_y_offset)
{
	// Get our internal cube quad tree parent node.
	QuadTreeNodeImpl &parent_node_impl = get_quad_tree_node_impl(parent_node);

	// Location of the current child node in the cube quad tree.
	const GPlatesMaths::CubeQuadTreeLocation child_cube_quad_tree_location(
			parent_node_impl.cube_quad_tree_location,
			child_x_offset,
			child_y_offset);

	// Get the internal cube quad tree child node.
	// See if we have a child cube quad tree node.
	cube_quad_tree_type::node_type *cube_child_node =
			parent_node_impl.cube_quad_tree_node.get_child_node(child_x_offset, child_y_offset);

	// If not then just create one.
	//
	// NOTE: After a while (with the user panning and zooming) we could end up with a lot
	// of nodes (because, unlike most situations, here there's no limit to how deep into the tree
	// the client can go - well, the limit would be how much viewport zoom is allowed in the GUI).
	// Note that we still recycle the tile textures though.
	//
	// TODO: We may want to periodically release nodes but that'll be hard because will first have
	// to determine which nodes are least-recently-used and also take into account that removing
	// an internal quad tree node will also remove its descendant nodes which may not be ready to go.
	// Probably not worth it though - memory usage is probably high enough to worry about.
	if (cube_child_node == NULL)
	{
		// The view transform for the current cube face.
		const GLTransform::non_null_ptr_to_const_type view_transform =
				d_cube_subdivision->get_view_transform(
						child_cube_quad_tree_location.get_node_location()->cube_face);

		// The projection transform for the current child cube quad tree node.
		const GLTransform::non_null_ptr_to_const_type projection_transform =
				d_cube_subdivision->get_projection_transform(
						child_cube_quad_tree_location.get_node_location()->quad_tree_depth/*level_of_detail*/,
						child_cube_quad_tree_location.get_node_location()->x_node_offset/*tile_u_offset*/,
						child_cube_quad_tree_location.get_node_location()->y_node_offset/*tile_v_offset*/);

		// Create a child quad tree node.
		d_cube_quad_tree->set_child_node(
				parent_node_impl.cube_quad_tree_node,
				child_x_offset,
				child_y_offset,
				d_cube_quad_tree->create_node(
						CubeQuadTreeNode(
								view_transform,
								projection_transform,
								get_level_of_detail(
										child_cube_quad_tree_location.get_node_location()->quad_tree_depth),
								d_texture_cache->allocate_volatile_object())));

		cube_child_node = parent_node_impl.cube_quad_tree_node.get_child_node(child_x_offset, child_y_offset);
	}

	return quad_tree_node_type(
			GPlatesUtils::non_null_intrusive_ptr<quad_tree_node_type::ImplInterface>(
					new QuadTreeNodeImpl(*cube_child_node, *this, child_cube_quad_tree_location)));
}


void
GPlatesOpenGL::GLMultiResolutionCubeReconstructedRaster::create_tile_texture(
		GL &gl,
		const GLTexture::shared_ptr_type &tile_texture)
{
	const GLCapabilities& capabilities = gl.get_capabilities();

	//
	// No mipmaps needed so we specify no mipmap filtering.
	// We're not using mipmaps because our cube mapping does not have much distortion
	// unlike global rectangular lat/lon rasters that squash near the poles.
	//

	// Bilinear filtering for GL_TEXTURE_MIN_FILTER and GL_TEXTURE_MAG_FILTER.
	gl.TextureParameteri(tile_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl.TextureParameteri(tile_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// Specify anisotropic filtering (if supported) to reduce aliasing in case tile texture is
	// subsequently sampled non-isotropically.
	//
	// Anisotropic filtering is an ubiquitous extension (that didn't become core until OpenGL 4.6).
	if (capabilities.gl_EXT_texture_filter_anisotropic)
	{
		gl.TextureParameterf(tile_texture, GL_TEXTURE_MAX_ANISOTROPY_EXT, capabilities.gl_texture_max_anisotropy);
	}

	// Clamp texture coordinates to centre of edge texels -
	// it's easier for hardware to implement - and doesn't affect our calculations.
	gl.TextureParameteri(tile_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	gl.TextureParameteri(tile_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// If the source texture contains alpha or coverage and its not in the alpha channel then swizzle the texture
	// so it is copied to the alpha channel (eg, a data RG texture copies coverage from G to A).
	boost::optional<GLenum> texture_swizzle_alpha = d_reconstructed_raster->get_tile_texture_swizzle_alpha();
	if (texture_swizzle_alpha)
	{
		gl.TextureParameteri(tile_texture, GL_TEXTURE_SWIZZLE_A, texture_swizzle_alpha.get());
	}

	// Create the texture but don't load any data into it.
	// Leave it uninitialised because we will be rendering into it to initialise it.
	gl.TextureStorage2D(tile_texture, 1/*levels*/,
			d_reconstructed_raster->get_tile_texture_internal_format(),
			d_tile_texel_dimension, d_tile_texel_dimension);

	// Check there are no OpenGL errors.
	GLUtils::check_gl_errors(gl, GPLATES_ASSERTION_SOURCE);
}
