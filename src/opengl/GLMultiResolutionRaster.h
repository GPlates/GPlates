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
 
#ifndef GPLATES_OPENGL_GLMULTIRESOLUTIONRASTER_H
#define GPLATES_OPENGL_GLMULTIRESOLUTIONRASTER_H

#include <cstddef> // For std::size_t
#include <map>
#include <vector>
#include <boost/cstdint.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "GLBuffer.h"
#include "GLIntersectPrimitives.h"
#include "GLMatrix.h"
#include "GLMultiResolutionRasterInterface.h"
#include "GLMultiResolutionRasterSource.h"
#include "GLProgram.h"
#include "GLTexture.h"
#include "GLTextureUtils.h"
#include "GLViewProjection.h"
#include "GLVertexArray.h"
#include "GLVertexUtils.h"

#include "maths/PointOnSphere.h"
#include "maths/Vector3D.h"

#include "property-values/CoordinateTransformation.h"
#include "property-values/Georeferencing.h"

#include "utils/ObjectCache.h"


namespace GPlatesOpenGL
{
	class GL;
	class GLFrustum;

	/**
	 * An arbitrary dimension raster image represented as a multi-resolution pyramid
	 * of tiled OpenGL textures and associated vertex meshes.
	 *
	 * The idea is to first determine the resolution required to display the raster
	 * such that one texel is roughly one pixel on the screen and secondly to find those
	 * texture tiles that are visible on the screen such that only those tiles need be
	 * generated for display.
	 *
	 * The meshes generated for each texture tile are dense enough to ensure that each texel
	 * in the raster is bounded in its deviation from its true position on the globe.
	 * This bound is a constant times the size of a texel on the globe.
	 *
	 * Note that this class can also be used to render a normal-map raster where the raster pixels
	 * are surface normals instead of colours.
	 * During rendering the surface normals are converted from tangent-space to world-space so that
	 * they can be captured in a cube raster (which is decoupled from the raster geo-referencing,
	 * or local tangent-space of raster). The surface lighting is then applied using the cube raster.
	 */
	class GLMultiResolutionRaster :
			public GLMultiResolutionRasterInterface
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLMultiResolutionRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMultiResolutionRaster> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLMultiResolutionRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMultiResolutionRaster> non_null_ptr_to_const_type;


		/**
		 * Typedef for a handle to a tile.
		 *
		 * A tile represents an arbitrary patch of the raster that is covered by a single OpenGL texture.
		 */
		typedef std::size_t tile_handle_type;

		/**
		 * Typedef for an opaque object that caches a particular render of this raster.
		 */
		typedef GLMultiResolutionRasterInterface::cache_handle_type cache_handle_type;

		/**
		 * The texture filter types.
		 *
		 * NOTE: Currently all filtering is 'nearest' instead of 'bilinear' but this will probably change.
		 * 
		 * If anisotropic filtering is specified it will be ignored if the 'GL_EXT_texture_filter_anisotropic'
		 * extension is not supported (it an ubiquitous extension that didn't become core until OpenGL 4.6).
		 */
		enum TextureFilterType
		{
			// Nearest neighbour filtering...
			TEXTURE_FILTER_NO_ANISOTROPIC,
			// Nearest neighbour (with anisotropic) filtering...
			TEXTURE_FILTER_ANISOTROPIC,

			NUM_TEXTURE_FILTER_TYPES // Must be the last enum.
		};

		/**
		 * The order of scanlines or rows of data in the raster as visualised in the image.
		 */
		enum RasterScanlineOrderType
		{
			TOP_TO_BOTTOM, // the first row of data in the raster is for the *top* of the image
			BOTTOM_TO_TOP  // the first row of data in the raster is for the *bottom* of the image
		};


		/**
		 * The default texture filtering mode for the textures returned by @a get_tile_texture
		 * is bilinear (with anisotropic) filtering.
		 */
		static const TextureFilterType DEFAULT_TEXTURE_FILTER = TEXTURE_FILTER_ANISOTROPIC;


		/**
		 * Determines the granularity of caching to be used for GLMultiResolutionRaster tile textures...
		 */
		enum CacheTileTexturesType
		{
			CACHE_TILE_TEXTURES_NONE,
			CACHE_TILE_TEXTURES_INDIVIDUAL_TILES,
			CACHE_TILE_TEXTURES_ENTIRE_LEVEL_OF_DETAIL_PYRAMID
		};

		/**
		 * The default granularity of tile texture caching.
		 */
		static const CacheTileTexturesType DEFAULT_CACHE_TILE_TEXTURES = CACHE_TILE_TEXTURES_INDIVIDUAL_TILES;


		/**
		 * Creates a @a GLMultiResolutionRaster object.
		 *
		 * @a georeferencing locates the raster pixels/lines in the raster's spatial reference system.
		 * @a coordinate_transformation transforms georeferenced raster coordinates to the standard
		 * geographic coordinate system WGS84 (this transforms from the raster's possibly
		 * *projection* spatial reference).
		 * @a raster_source is the source of raster data.
		 *
		 * Default texture filtering mode for the internal textures rendered during @a render is
		 * nearest neighbour (with anisotropic) filtering.
		 *
		 * If @a cache_tile_textures is 'CACHE_TILE_TEXTURES_ENTIRE_LEVEL_OF_DETAIL_PYRAMID' then the
		 * internal texture/vertex cache is allowed to grow to encompass all tiles in all levels of detail.
		 * WARNING: This should normally be turned off (especially for visualisation of rasters where
		 * only a small part of the raster is ever visible at any time - otherwise memory usage will
		 * grow excessively large).
		 * An example of turning this on is data analysis of a floating-point raster where the whole
		 * raster is likely to be accessed (not just a small window as in the case of visual display)
		 * repeatedly over many frames and you don't want to incur the large performance hit of
		 * continuously reloading tiles from disk (eg, raster co-registration data-mining front-end)
		 * - in this case the user can always choose a lower level of detail if the memory usage is
		 * too high for their system.
		 * Note that if @a cache_tile_textures is 'CACHE_TILE_TEXTURES_NONE' then the returned cache
		 * handle from @a render will not contain any caching (of tile textures) and hence those same
		 * rendered tile textures will need to be re-generated the next time @a render is called.
		 */
		static
		non_null_ptr_type
		create(
				GL &gl,
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
				const GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_to_const_type &coordinate_transformation,
				const GLMultiResolutionRasterSource::non_null_ptr_type &raster_source,
				TextureFilterType texture_filter = DEFAULT_TEXTURE_FILTER,
				CacheTileTexturesType cache_tile_textures = DEFAULT_CACHE_TILE_TEXTURES,
				RasterScanlineOrderType raster_scanline_order = TOP_TO_BOTTOM);


		/**
		 * Returns a subject token that clients can observe to see if they need to update themselves
		 * (such as any cached data we render for them) by getting us to re-render.
		 */
		virtual
		const GPlatesUtils::SubjectToken &
		get_subject_token() const
		{
			// We'll just use the valid token of the raster source - if they don't change then neither do we.
			// If we had two input sources then we'd have to have our own valid token.
			return d_raster_source->get_subject_token();
		}


		/**
		 * Returns the number of levels of detail.
		 *
		 * See base class for more details.
		 */
		virtual
		unsigned int
		get_num_levels_of_detail() const
		{
			return d_level_of_detail_pyramid.size();
		}


		/**
		 * Returns the unclamped exact floating-point level-of-detail that theoretically represents
		 * the exact level-of-detail that would be required to fulfill the resolution needs of a
		 * render target as defined by the specified viewport and view/projection matrices (@a view_projection).
		 *
		 * See base class for more details.
		 */
		virtual
		float
		get_level_of_detail(
				const GLViewProjection &view_projection,
				float level_of_detail_bias = 0.0f) const;


		/**
		 * Takes an unclamped level-of-detail (see @a get_level_of_detail) and clamps it to lie
		 * within the the range [0, @a get_num_levels_of_detail - 1].
		 *
		 * See base class for more details.
		 */
		virtual
		float
		clamp_level_of_detail(
				float level_of_detail) const;


		using GLMultiResolutionRasterInterface::render;


		/**
		 * Renders all tiles visible in the view frustum, determined by the specified
		 * view-projection transform, and returns true if any tiles were rendered.
		 *
		 * Throws exception if @a level_of_detail is outside the valid range.
		 * Use @a clamp_level_of_detail to clamp to a valid range before calling this method.
		 *
		 * See base class for more details.
		 */
		virtual
		bool
		render(
				GL &gl,
				const GLMatrix &view_projection_transform,
				float level_of_detail,
				cache_handle_type &cache_handle);


		/**
		 * Returns a list of tiles that are visible inside the view frustum planes defined
		 * by the specified view-projection transform.
		 *
		 * The specified level-of-detail determines which tiles (in the LOD hierarchy) should
		 * be returned/rendered - see @a get_level_of_detail for viewport sizing details.
		 *
		 * NOTE: If @a get_level_of_detail was used to determine @a level_of_detail then
		 * the view-projection transform specified here and in @a get_level_of_detail should match.
		 *
		 * NOTE: @a level_of_detail must be in the range [0, @a get_num_levels_of_detail - 1].
		 */
		void
		get_visible_tiles(
				std::vector<tile_handle_type> &visible_tiles,
				const GLMatrix &view_projection_transform,
				float level_of_detail) const;


		/**
		 * Returns a list of tiles that are visible inside the view frustum planes defined
		 * by the specified view and projection transforms (in @a view_projection).
		 *
		 * The specified viewport (in @a view_projection) determines the internal level-of-detail
		 * required that in turn determines which tiles (in the LOD hierarchy) should be returned/rendered.
		 *
		 * All returned tiles are from the same level-of-detail in the multi-resolution raster.
		 *
		 * Returns the lowest resolution tiles that adequately fulfill the resolution
		 * needs of a render target (or the highest level-of-detail if none of the
		 * level-of-details is adequate).
		 * The required resolution is determined by the specified viewport and model-view/projection transforms.
		 *
		 * See @a render for a description of @a level_of_detail_bias.
		 */
		void
		get_visible_tiles(
				std::vector<tile_handle_type> &visible_tiles,
				const GLViewProjection &view_projection,
				float level_of_detail_bias = 0.0f) const
		{
			get_visible_tiles(
					visible_tiles,
					view_projection.get_view_projection_transform(),
					clamp_level_of_detail(get_level_of_detail(view_projection, level_of_detail_bias)));
		}


		/**
		 * Renders the specified tiles (returns true if the specified tiles are not empty).
		 *
		 * See the first overload of @a render for more details.
		 */
		bool
		render(
				GL &gl,
				const GLMatrix &view_projection_transform,
				const std::vector<tile_handle_type> &tiles,
				cache_handle_type &cache_handle);


		/**
		 * Returns the tile texel dimension of this raster which is also the tile texel dimension
		 * of the raster source.
		 */
		unsigned int
		get_tile_texel_dimension() const
		{
			return d_tile_texel_dimension;
		}


		/**
		 * Returns the texture internal format that can be used if rendering to a texture, when
		 * calling @a render, as opposed to the main framebuffer.
		 *
		 * This is the 'internalformat' parameter of glTexImage2D for example.
		 *
		 * NOTE: The filtering mode is expected to be set to 'nearest' in all cases.
		 * Currently 'nearest' fits best with the georeferencing information of rasters.
		 */
		GLint
		get_target_texture_internal_format() const
		{
			// Delegate to our source raster input.
			return d_raster_source->get_target_texture_internal_format();
		}


		/**
		 * Clears the currently bound framebuffer as appropriate for the raster type.
		 *
		 * This is useful when rendering *regional* (non-global) normal maps to a target texture
		 * because @a render only renders within the regional raster and the normals outside the
		 * region must be normal to the globe's surface.
		 */
		void
		clear_framebuffer(
				GL &gl);

	private:
		/**
		 * Maintains a tile's vertices in the form of a vertex buffer and vertex array wrapper.
		 *
		 * The tile vertices (vertex buffer/array) are cached.
		 */
		struct TileVertices
		{
			explicit
			TileVertices(
					GL &gl_) :
				vertex_buffer(GLBuffer::create(gl_)),
				vertex_array(GLVertexArray::create(gl_)),
				num_vertex_elements(0)
			{  }

			GLBuffer::shared_ptr_type vertex_buffer;
			GLVertexArray::shared_ptr_type vertex_array;
			GLsizei num_vertex_elements;
		};

		/**
		 * Typedef for a cache of tile vertices.
		 */
		typedef GPlatesUtils::ObjectCache<TileVertices> tile_vertices_cache_type;


		/**
		 * Maintains a tile's texture and source tile cache handle.
		 */
		struct TileTexture
		{
			explicit
			TileTexture(
					GL &gl_,
					const GLMultiResolutionRasterSource::cache_handle_type &source_cache_handle_ =
							GLMultiResolutionRasterSource::cache_handle_type()) :
				texture(GLTexture::create_unique(gl_)),
				source_cache_handle(source_cache_handle_)
			{  }

			/**
			 * Clears the source cache.
			 *
			 * Called when 'this' tile texture is returned to the cache (so texture can be reused).
			 */
			void
			returned_to_cache()
			{
				source_cache_handle.reset();
			}

			GLTexture::shared_ptr_type texture;
			GLMultiResolutionRasterSource::cache_handle_type source_cache_handle;
		};

		/**
		 * Typedef for a cache of tile textures.
		 */
		typedef GPlatesUtils::ObjectCache<TileTexture> tile_texture_cache_type;


		/**
		 * A tile represents an arbitrary patch of the raster that is covered by a single OpenGL texture.
		 *
		 * This tile has valid vertices and a valid texture and is ready to draw.
		 *
		 * As long as this @a Tile object exists the vertices and texture will be valid and cannot
		 * be reclaimed by the vertex and texture cache.
		 */
		class Tile
		{
		public:
			Tile(
					const tile_vertices_cache_type::object_shared_ptr_type &tile_vertices_,
					const tile_texture_cache_type::object_shared_ptr_type &tile_texture_) :
				tile_vertices(tile_vertices_),
				tile_texture(tile_texture_)
			{  }

			tile_vertices_cache_type::object_shared_ptr_type tile_vertices;
			tile_texture_cache_type::object_shared_ptr_type tile_texture;
		};


		/**
		 * Used to cache information, specific to a tile, to return to the client for caching.
		 */
		struct ClientCacheTile
		{
			ClientCacheTile(
					const Tile &tile_,
					// Set to true to cache the GLMultiResolutionRaster tile texture...
					CacheTileTexturesType cache_tile_textures_) :
				tile_vertices(tile_.tile_vertices),
				// NOTE: Anything the GLMultiResolutionRasterSource itself wants cached is always cached...
				source_cache_handle(tile_.tile_texture->source_cache_handle)
			{
				if (cache_tile_textures_ != CACHE_TILE_TEXTURES_NONE)
				{
					tile_texture = tile_.tile_texture;
				}
			}

			tile_vertices_cache_type::object_shared_ptr_type tile_vertices;
			tile_texture_cache_type::object_shared_ptr_type tile_texture;
			GLMultiResolutionRasterSource::cache_handle_type source_cache_handle;
		};


		/**
		 * Retains information to build a single tile of the raster.
		 */
		class LevelOfDetailTile :
				public GPlatesUtils::ReferenceCount<LevelOfDetailTile>
		{
		public:
			typedef GPlatesUtils::non_null_intrusive_ptr<LevelOfDetailTile> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const LevelOfDetailTile> non_null_ptr_to_const_type;


			static
			non_null_ptr_type
			create(
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
					tile_texture_cache_type &tile_texture_cache_)
			{
				return non_null_ptr_type(new LevelOfDetailTile(
						lod_level_,
						x_geo_start_, x_geo_end_, y_geo_start_, y_geo_end_,
						x_num_vertices_, y_num_vertices_, u_start_, u_end_, v_start_, v_end_,
						u_lod_texel_offset_, v_lod_texel_offset_, num_u_lod_texels_, num_v_lod_texels_,
						tile_vertices_cache_, tile_texture_cache_));
			}


			/**
			 * The level-of-detail of this tile.
			 */
			unsigned int lod_level;


			// NOTE: 'x_geo_start', 'x_geo_end', 'y_geo_start' and 'y_geo_end'
			// are georeference coordinates of the original high-resolution raster and not
			// the texels of this level-of-detail (unless, of course, this
			// level-of-detail is the highest resolution).
			//
			// This is because georeferencing works with the highest resolution pixel coordinates.

			/**
			 * The start georeference x coordinate of this tile.
			 * The left edge of the first pixel (not the pixel centre).
			 */
			unsigned int x_geo_start;

			/**
			 * The end georeference x coordinate of this tile.
			 * The right edge of the last pixel (not the pixel centre).
			 */
			unsigned int x_geo_end;

			/**
			 * The start georeference y coordinate of this tile.
			 * The top edge of the first pixel (not the pixel centre).
			 */
			unsigned int y_geo_start;

			/**
			 * The end georeference y coordinate of this tile.
			 * The bottom edge of the last pixel (not the pixel centre).
			 */
			unsigned int y_geo_end;

			/**
			 * The number of vertices along the x direction of this tile.
			 */
			unsigned int x_num_vertices;

			/**
			 * The number of vertices along the y direction of this tile.
			 */
			unsigned int y_num_vertices;

			//! The 'u' texture coordinate corresponding to @a x_start.
			float u_start;
			//! The 'u' texture coordinate corresponding to @a x_end.
			float u_end;

			//! The 'v' texture coordinate corresponding to @a y_start.
			float v_start;
			//! The 'v' texture coordinate corresponding to @a y_end.
			float v_end;

			/*
			 * Offsets to the first texel in the region covered by this tile.
			 * Note that the 'y' and the 'v' directions are inverted wrt each other -
			 * this is reflected in the 'v' texture coordinates.
			 */
			unsigned int u_lod_texel_offset;
			unsigned int v_lod_texel_offset;

			/*
			 * The number of texels that we need to load from the raster image in order
			 * to cover the tile.
			 *
			 * For example, the texels needed by a 5x5 raster image are:
			 * Level 0: 5x5
			 * Level 1: 3x3 (covers equivalent of 6x6 level 0 texels)
			 * Level 2: 2x2 (covers equivalent of 4x4 level 1 texels or 8x8 level 0 texels)
			 * Level 3: 1x1 (covers same area as level 2)
			 *
			 * Whereas the same area on the globe must be covered by all levels of detail so
			 * the area covered on the globe in units of texels (at that level-of-detail) is:
			 * Level 0: 5x5
			 * Level 1: 2.5 x 2.5
			 * Level 2: 1.25 * 1.25
			 * Level 3: 0.625 * 0.625
			 *
			 * ...so if this is a level 1 tile then the number of texels needed is 3x3 whereas
			 * only 2.5 x 2.5 texels actually contribute to the tile on the globe. However
			 * we need 3x3 texels because we can't load partial texels into an OpenGL texture.
			 */
			unsigned int num_u_lod_texels;
			unsigned int num_v_lod_texels;

			//
			// The following data members use @a ObjectCache because:
			// - we don't want to generate them up front for all tiles,
			// - resource memory usage would be too expensive if we allocated all tiles up front,
			// - the time taken to generate all tiles up front might be too expensive,
			// - not all tiles might be needed (eg, user might not zoom into all high-res regions).
			//

			/**
			 * The vertices required to draw this tile onto the globe.
			 */
			tile_vertices_cache_type::volatile_object_ptr_type tile_vertices;

			/**
			 * The texture representation of the raster data for this tile.
			 */
			tile_texture_cache_type::volatile_object_ptr_type tile_texture;

			/**
			 * Keeps tracks of whether the source data has changed underneath us
			 * and we need to reload our texture.
			 */
			mutable GPlatesUtils::ObserverToken source_texture_observer_token;

		private:
			//! Constructor.
			LevelOfDetailTile(
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
					tile_texture_cache_type &tile_texture_cache_);
		};


		/**
		 * A level-of-detail represents a full set of tiles covering the entire raster,
		 * but at a particular resolution.
		 */
		class LevelOfDetail :
				public GPlatesUtils::ReferenceCount<LevelOfDetail>
		{
		public:
			typedef GPlatesUtils::non_null_intrusive_ptr<LevelOfDetail> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const LevelOfDetail> non_null_ptr_to_const_type;

			/**
			 * A node in an oriented bounding box tree.
			 *
			 * The tree is recursively visited to quickly find tiles that are visible
			 * in the view frustum (tiles are referenced by leaf nodes of the tree).
			 */
			struct OBBTreeNode
			{
				//! Constructor.
				OBBTreeNode(
						const GLIntersect::OrientedBoundingBox &bounding_box_,
						bool is_leaf_node_) :
					bounding_box(bounding_box_),
					is_leaf_node(is_leaf_node_)
				{  }

				/**
				 * An oriented box that bounds the subtree rooted at this node (for internal
				 * tree nodes) or bounds the vertices of a raster tile and hence the
				 * triangle mesh attached to its vertices (for leaf tree nodes).
				 */
				GLIntersect::OrientedBoundingBox bounding_box;

				//! Structure containing internal or leaf node information.
				struct
				{
					bool is_leaf_node;

					union
					{
						//! The child indices if this is *not* a leaf node.
						std::size_t child_node_indices[2];

						//! The tile handle if this *is* a leaf node.
						tile_handle_type tile;
					};
				};
			};

			//! Typedef for the container of all OBB tree nodes belonging to a level-of-detail.
			typedef std::vector<OBBTreeNode> obb_tree_node_seq_type;


			static
			non_null_ptr_type
			create(
					unsigned int lod_level)
			{
				return non_null_ptr_type(new LevelOfDetail(lod_level));
			}


			/**
			 * Returns the node of the OBB tree at index @a node_index.
			 *
			 * NOTE: The root node is the entry point for accessing the tiles (in the leaf nodes).
			 */
			const OBBTreeNode &
			get_obb_tree_node(
					std::size_t node_index) const;

			/**
			 * Returns the node of the OBB tree at index @a node_index.
			 *
			 * NOTE: The root node is the entry point for accessing the tiles (in the leaf nodes).
			 */
			OBBTreeNode &
			get_obb_tree_node(
					std::size_t node_index);


			/**
			 * The level-of-detail level (highest resolution is 0).
			 */
			unsigned int lod_level;

			/**
			 * The OBB tree nodes are stored here.
			 */
			obb_tree_node_seq_type obb_tree_nodes;

			/**
			 * Index of the root OBB tree node in @a obb_tree_nodes.
			 */
			std::size_t obb_tree_root_node_index;

		private:
			explicit
			LevelOfDetail(
					unsigned int lod_level_);
		};


		//! Typedef for a sequence of level-of-detail tiles.
		typedef std::vector<LevelOfDetailTile::non_null_ptr_type> level_of_detail_tile_seq_type;

		//! Typedef for a sequence of level-of-details.
		typedef std::vector<LevelOfDetail::non_null_ptr_type> level_of_detail_seq_type;


		//! The tangent space coordinate frame (not necessarily orthogonal) at a position on the sphere.
		struct TangentSpaceFrame
		{
			TangentSpaceFrame(
					const GPlatesMaths::UnitVector3D &tangent_,
					const GPlatesMaths::UnitVector3D &binormal_,
					const GPlatesMaths::UnitVector3D &normal_) :
				tangent(tangent_),
				binormal(binormal_),
				normal(normal_)
			{  }

			GPlatesMaths::UnitVector3D tangent;
			GPlatesMaths::UnitVector3D binormal;
			GPlatesMaths::UnitVector3D normal;
		};

		//! Typedef for visual raster vertices.
		typedef GLVertexUtils::TextureVertex visual_vertex_type;

		//! Typedef for data raster vertices.
		typedef GLVertexUtils::TextureVertex data_vertex_type;

		//! Typedef for normal-map vertices.
		typedef GLVertexUtils::TextureTangentSpaceVertex normal_map_vertex_type;

		//! Typedef for scalar-gradient-map vertices.
		typedef GLVertexUtils::TextureTangentSpaceVertex scalar_field_depth_layer_vertex_type;

		//! Typedef for vertex indices.
		typedef GLushort vertex_element_type;

		//! Typedef for mapping a tile's vertex dimensions to vertex indices (and draw call).
		typedef std::map<
				std::pair<unsigned int,unsigned int>,
				GLBuffer::shared_ptr_type> vertex_element_buffer_map_type;

		//! A 16:16 fixed point type to get fractional values without floating-point precision issues.
		typedef boost::uint32_t texels_per_vertex_fixed_point_type;


		/**
		 * Used to render sphere normals.
		 *
		 * This is useful when rendering *regional* (non-global) normal maps to a target texture
		 * because @a render only renders within the regional raster and the normals outside the
		 * region must be normal to the globe's surface.
		 */
		class RenderSphereNormals
		{
		public:
			explicit
			RenderSphereNormals(
					GL &gl);

			void
			render(
					GL &gl);

		private:
			typedef GLushort vertex_element_type;

			GLVertexArray::shared_ptr_type d_vertex_array;
			GLBuffer::shared_ptr_type d_vertex_buffer;
			GLBuffer::shared_ptr_type d_vertex_element_buffer;
			unsigned int d_num_vertex_elements;

			GLProgram::shared_ptr_type d_program;
		};


		/**
		 * Georeferencing information to position the raster pixels/lines in the raster's spatial reference system.
		 */
		GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type d_georeferencing;

		/**
		 * Transforms georeferenced raster coordinates to the standard geographic coordinate system WGS84
		 * (this transforms from the raster's possibly *projection* spatial reference).
		 */
		GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_to_const_type d_coordinate_transformation;

		/**
		 * The source of multi-resolution raster data.
		 */
		GLMultiResolutionRasterSource::non_null_ptr_type d_raster_source;

		//! Original raster width.
		unsigned int d_raster_width;

		//! Original raster height.
		unsigned int d_raster_height;

		/**
		 * The scanline order of the raster (whether first row of data is at top or bottom of image).
		 */
		RasterScanlineOrderType d_raster_scanline_order;

		/**
		 * The texture filtering mode for textures rendered during @a render.
		 */
		TextureFilterType d_texture_filter;


		/**
		 * The number of texels along a tiles edge (horizontal or vertical since it's square).
		 */
		unsigned int d_tile_texel_dimension;

		//! 1.0 / 'd_tile_texel_dimension'.
		float d_inverse_tile_texel_dimension;

		/**
		 * The (fractional) number of texels between two adjacent vertices along a horizontal or vertical edge of the tile.
		 *
		 * This is a 16:16 fixed point type to allow fractional values without floating-point precision issues.
		 *
		 * See @a MAX_NUM_TEXELS_PER_VERTEX for more details.
		 */
		texels_per_vertex_fixed_point_type d_num_texels_per_vertex;

		/**
		 * All tiles of all resolution are grouped into one array for easy lookup for clients.
		 */
		level_of_detail_tile_seq_type d_tiles;

		level_of_detail_seq_type d_level_of_detail_pyramid;

		/**
		 * The maximum size of any texel in the original raster (the highest resolution
		 * level-of-detail) when projected onto the unit sphere.
		 *
		 * This is used for level-of-detail selection.
		 */
		float d_max_highest_resolution_texel_size_on_unit_sphere;

		/**
		 * This raster has its own cache of textures which gets reused/recycled as the
		 * view pans across the raster.
		 * Since the maximum number of screen pixels covered by a raster is bounded
		 * (by the number of pixels in the viewport) then there is also a bound on the
		 * maximum number of texture tiles needed to display the raster - note that this
		 * is independent of raster resolution currently being displayed because as the view
		 * zooms into a raster the visible part of the raster decreases to compensate for the
		 * increased raster resolution being displayed.
		 */
		tile_texture_cache_type::shared_ptr_type d_tile_texture_cache;

		/**
		 * Determines granularity of caching of *our* tile textures.
		 */
		CacheTileTexturesType d_cache_tile_textures;

		/**
		 * A cache of tile vertices to limit memory usage.
		 *
		 * Having a cache also means we are not creating/destroying vertex buffer objects unnecessarily.
		 */
		tile_vertices_cache_type::shared_ptr_type d_tile_vertices_cache;

		/**
		 * Shared vertex indices used by the tiles of this raster.
		 */
		vertex_element_buffer_map_type d_vertex_element_buffers;

		/**
		 * Shader program for all rendering (eg, colour, normal-map, data, etc).
		 */
		GLProgram::shared_ptr_type d_render_raster_program;

		/**
		 * Used to render sphere normals.
		 *
		 * This is useful when rendering *regional* (non-global) normal maps to a target texture
		 * because @a render only renders within the regional raster and the normals outside the
		 * region must be normal to the globe's surface.
		 */
		boost::optional<RenderSphereNormals> d_render_sphere_normals;


		/**
		 * The maximum number of texels between two adjacent vertices along a horizontal or
		 * vertical edge of the tile.
		 *
		 * For most rasters this is the texel density used. However for very low resolution rasters
		 * a smaller texel density is needed (especially for global rasters) otherwise the vertex
		 * mesh is too coarse and the raster no longer looks like a spherical surface but instead
		 * looks like a coarse polyhedron embedded inside the globe.
		 *
		 * This is relatively small to minimise texel deviation on the sphere.
		 * In other words we want each texel to be positioned approximately where it
		 * should be on the globe. The texel is positioned exactly only at vertices
		 * because vertices tie the raster to the globe. The further a texel is from
		 * a vertex the higher it's position error is likely to be, so more vertices
		 * means less error.
		 *
		 * We could instead have adaptively sampled the mesh where the deviation exceeded
		 * a threshold but there are benefits to having a simple mesh and GPUs are
		 * fast enough that having more vertices than we need won't slow it down.
		 * See http://developer.nvidia.com/docs/IO/8230/BatchBatchBatch.pdf
		 *
		 * NOTE: This means there are at most MAX_NUM_TEXELS_PER_VERTEX * MAX_NUM_TEXELS_PER_VERTEX
		 * texels between four adjacent vertices that form a quad (two mesh triangles).
		 */
		static const unsigned int MAX_NUM_TEXELS_PER_VERTEX = 16;


		/**
		 * We also need to make sure there are enough vertices to follow the curvature of the
		 * globe, otherwise the mesh segments will dip too far below the surface of the sphere.
		 */
		static const unsigned int MAX_ANGLE_IN_DEGREES_BETWEEN_VERTICES = 5;


		//! Constructor.
		GLMultiResolutionRaster(
				GL &gl,
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
				const GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_to_const_type &coordinate_transformation,
				const GLMultiResolutionRasterSource::non_null_ptr_type &raster_source,
				TextureFilterType texture_filter,
				CacheTileTexturesType cache_tile_textures,
				RasterScanlineOrderType raster_scanline_order);

		void
		compile_link_shader_program(
				GL &gl);

		/**
		 * Creates the level-of-detail pyramid structures.
		 * They initially contain no raster or vertex data though.
		 */
		void
		initialise_level_of_detail_pyramid();


		/**
		 * Calculates the (fractional) number of texels per vertex required for the entire raster.
		 */
		texels_per_vertex_fixed_point_type
		calculate_num_texels_per_vertex();


		/**
		 * Creates a level-of-detail structure for level @a lod_level.
		 */
		LevelOfDetail::non_null_ptr_type
		create_level_of_detail(
				const unsigned int lod_level);


		/**
		 * Creates an oriented bounding box tree covering a level-of-detail.
		 */
		std::size_t
		create_obb_tree(
				LevelOfDetail &level_of_detail,
				const unsigned int x_geo_start,
				const unsigned int x_geo_end,
				const unsigned int y_geo_start,
				const unsigned int y_geo_end);


		/**
		 * Creates a leaf node of an OBB tree covering a specific level-of-detail.
		 */
		std::size_t
		create_obb_tree_leaf_node(
				LevelOfDetail &level_of_detail,
				const unsigned int x_geo_start,
				const unsigned int x_geo_end,
				const unsigned int y_geo_start,
				const unsigned int y_geo_end);


		/**
		 * Creates a raster tile structure containing enough information to
		 * subsequently generate texture and vertex data for rendering the tile.
		 */
		tile_handle_type
		create_level_of_detail_tile(
				LevelOfDetail &level_of_detail,
				const unsigned int x_geo_start,
				const unsigned int x_geo_end,
				const unsigned int y_geo_start,
				const unsigned int y_geo_end);


		/**
		 * Creates an oriented bounding box for @a lod_tile.
		 */
		GLIntersect::OrientedBoundingBox
		bound_level_of_detail_tile(
				const LevelOfDetailTile &lod_tile) const;

		/**
		 * Creates an oriented bounding box (OBB) builder whose z-axis coincides with
		 * the position of the specified pixel coordinates (of the raster) on the unit sphere.
		 * This same point on the sphere is then added to the builder to set the maximum
		 * extent of the OBB along its z-axis. The caller will need to add more points though.
		 *
		 * The returned bounding box (builder) depends on georeferencing (mapping of pixel
		 * coordinates to a position on the globe).
		 */
		GLIntersect::OrientedBoundingBoxBuilder
		create_oriented_bounding_box_builder(
				const double &x_geo_coord,
				const double &y_geo_coord) const;


		/**
		 * Calculates the maximum size of any texel of @a lod_tile projected onto the unit sphere.
		 */
		float
		calc_max_texel_size_on_unit_sphere(
				const unsigned int lod_level,
				const LevelOfDetailTile &lod_tile) const;


		/**
		 * Binds a vertex element buffer that indexes into a uniform mesh of triangles covering a tile
		 * with @a num_vertices_along_tile_x_edge by @a num_vertices_along_tile_y_edge vertices.
		 *
		 * If a matching vertex element buffer is not found then one is created and cached.
		 * This enables sharing of a vertex element buffer by multiple tiles.
		 *
		 * This is just the vertex indices (not the vertices themselves which differ for each tile).
		 *
		 * Returns the number of indices (vertex elements) in tile.
		 */
		GLsizei
		bind_vertex_element_buffer(
				GL &gl,
				const unsigned int num_vertices_along_tile_x_edge,
				const unsigned int num_vertices_along_tile_y_edge);


		/**
		 * Recursively traverses OBB tree to find visible tiles.
		 */
		void
		get_visible_tiles(
				const GLFrustum &frustum_planes,
				boost::uint32_t frustum_plane_mask,
				const LevelOfDetail &lod,
				const LevelOfDetail::OBBTreeNode &obb_tree_node,
				std::vector<tile_handle_type> &visible_tiles) const;


		/**
		 * Returns the tile corresponding to @a tile_handle.
		 *
		 * Discard the returned tile after rendering so that it's vertices and texture
		 * can be recycled for subsequent scene renders.
		 */
		Tile
		get_tile(
				tile_handle_type tile_handle,
				GL &gl);

		/**
		 * Returns the tile texture for the tile @a lod_tile.
		 */
		tile_texture_cache_type::object_shared_ptr_type
		get_tile_texture(
				GL &gl,
				const LevelOfDetailTile &lod_tile);

		/**
		 * Returns the tile vertices for the tile @a lod_tile.
		 */
		tile_vertices_cache_type::object_shared_ptr_type
		get_tile_vertices(
				GL &gl,
				const LevelOfDetailTile &lod_tile);

		/**
		 * Loads raster data into @a lod_tile.
		 */
		void
		load_raster_data_into_tile_texture(
				const LevelOfDetailTile &lod_tile,
				TileTexture &tile_texture,
				GL &gl);

		/**
		 * Creates a texture in OpenGL but doesn't load any image data into it.
		 */
		void
		create_texture(
				GL &gl,
				const GLTexture::shared_ptr_type &texture);

		/**
		 * Loads vertex data into @a lod_tile.
		 */
		void
		load_vertices_into_tile_vertex_buffer(
				GL &gl,
				const LevelOfDetailTile &lod_tile,
				TileVertices &tile_vertices);

		void
		get_adjacent_vertex_positions(
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
				const double &y_pixels_per_quad);

		TangentSpaceFrame
		calculate_tangent_space_frame(
				const GPlatesMaths::UnitVector3D &vertex_position,
				const GPlatesMaths::UnitVector3D &vertex_position01,
				const GPlatesMaths::UnitVector3D &vertex_position21,
				const GPlatesMaths::UnitVector3D &vertex_position10,
				const GPlatesMaths::UnitVector3D &vertex_position12);

		/**
		 * Converts from raster pixel coordinates to a position on the globe.
		 */
		GPlatesMaths::PointOnSphere
		convert_pixel_coord_to_geographic_coord(
				const double &x_pixel_coord,
				const double &y_pixel_coord,
				boost::optional<double &> y_pixel_coord_clamped = boost::none) const;
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONRASTER_H
