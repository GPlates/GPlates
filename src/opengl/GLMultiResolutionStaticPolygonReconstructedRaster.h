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

#ifndef GPLATES_OPENGL_GLMULTIRESOLUTIONSTATICPOLYGONRECONSTRUCTEDRASTER_H
#define GPLATES_OPENGL_GLMULTIRESOLUTIONSTATICPOLYGONRECONSTRUCTEDRASTER_H

#include <cstddef> // For std::size_t
#include <vector>
#include <boost/optional.hpp>
#include <boost/ref.hpp>
#include <boost/shared_ptr.hpp>

#include "GLCompiledDrawState.h"
#include "GLCoverageSource.h"
#include "GLCubeSubdivisionCache.h"
#include "GLMatrix.h"
#include "GLMultiResolutionCubeRaster.h"
#include "GLMultiResolutionRasterInterface.h"
#include "GLProgramObject.h"
#include "GLReconstructedStaticPolygonMeshes.h"
#include "GLTexture.h"
#include "GLUtils.h"
#include "GLViewport.h"

#include "app-logic/ReconstructedFeatureGeometry.h"

#include "maths/MathsFwd.h"
#include "maths/CubeQuadTree.h"
#include "maths/CubeQuadTreePartition.h"
#include "maths/CubeQuadTreePartitionUtils.h"

#include "utils/ObjectCache.h"
#include "utils/SubjectObserverToken.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * A raster that is reconstructed by mapping it onto a set of present-day static polygons and
	 * reconstructing the polygons (and hence partitioned pieces of the raster).
	 */
	class GLMultiResolutionStaticPolygonReconstructedRaster :
			public GLMultiResolutionRasterInterface
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLMultiResolutionStaticPolygonReconstructedRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMultiResolutionStaticPolygonReconstructedRaster> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLMultiResolutionStaticPolygonReconstructedRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMultiResolutionStaticPolygonReconstructedRaster> non_null_ptr_to_const_type;

		/**
		 * Typedef for an opaque object that caches a particular render of this raster.
		 */
		typedef GLMultiResolutionRasterInterface::cache_handle_type cache_handle_type;

		/**
		 * Age grid mask and coverage rasters.
		 */
		struct AgeGridRaster
		{
			AgeGridRaster(
					const GLMultiResolutionRaster::non_null_ptr_type &age_grid_mask_raster_,
					const GLMultiResolutionRaster::non_null_ptr_type &age_grid_coverage_raster_) :
				age_grid_mask_raster(age_grid_mask_raster_),
				age_grid_coverage_raster(age_grid_coverage_raster_)
			{  }

			GLMultiResolutionRaster::non_null_ptr_type age_grid_mask_raster;
			GLMultiResolutionRaster::non_null_ptr_type age_grid_coverage_raster;

			/**
			 * The distribution of an age grid coverage value in the pixel channels of the coverage raster.
			 *
			 * This is here because @a GLMultiResolutionStaticPolygonReconstructedRaster internal
			 * rendering expects a specific distribution.
			 */
			static const GLCoverageSource::TextureDataFormat COVERAGE_TEXTURE_DATA_FORMAT =
					GLCoverageSource::TEXTURE_DATA_FORMAT_FIXED_POINT_RGBA8_COVERAGE_INVERT_A;
		};


		/**
		 * Returns true if *reconstructed* rasters are supported on the runtime system.
		 *
		 * Currently two texture units are required - pretty much all hardware for over a decade
		 * has support for this. However some systems fallback to software rendering, which on
		 * Microsoft platforms is OpenGL 1.1 without destination alpha, and typically only supports
		 * a single texture unit. For example virtual desktops can exhibit this behaviour.
		 */
		static
		bool
		is_supported(
				GLRenderer &renderer);


		/**
		 * Creates a @a GLMultiResolutionStaticPolygonReconstructedRaster object.
		 *
		 * @param raster_to_reconstruct the raster to be reconstructed.
		 * @param reconstructed_static_polygon_meshes the reconstructed present day polygon meshes.
		 * @param age_grid_raster is the optional age grid mask and coverage rasters.
		 */
		static
		non_null_ptr_type
		create(
				GLRenderer &renderer,
				const GLMultiResolutionCubeRaster::non_null_ptr_type &raster_to_reconstruct,
				const GLReconstructedStaticPolygonMeshes::non_null_ptr_type &reconstructed_static_polygon_meshes,
				boost::optional<AgeGridRaster> age_grid_raster = boost::none)
		{
			return non_null_ptr_type(
					new GLMultiResolutionStaticPolygonReconstructedRaster(
							renderer,
							raster_to_reconstruct,
							reconstructed_static_polygon_meshes,
							age_grid_raster));
		}


		/**
		 * Returns a subject token that clients can observe to see if they need to update themselves
		 * (such as any cached data we render for them) by getting us to re-render.
		 */
		virtual
		const GPlatesUtils::SubjectToken &
		get_subject_token() const;


		/**
		 * Returns the number of levels of detail.
		 *
		 * See base class for more details.
		 */
		virtual
		std::size_t
		get_num_levels_of_detail() const
		{
			return d_raster_to_reconstruct->get_num_levels_of_detail();
		}


		/**
		 * Returns the unclamped exact floating-point level-of-detail that theoretically represents
		 * the exact level-of-detail that would be required to fulfill the resolution needs of a
		 * render target (as defined by the specified viewport and view/projection matrices).
		 *
		 * See base class for more details.
		 */
		virtual
		float
		get_level_of_detail(
				const GLMatrix &model_view_transform,
				const GLMatrix &projection_transform,
				const GLViewport &viewport,
				float level_of_detail_bias = 0.0f) const;


		/**
		 * Takes an unclamped level-of-detail (see @a get_level_of_detail) and clamps it to lie
		 * within the range [-Infinity, @a get_num_levels_of_detail - 1].
		 *
		 * NOTE: The returned level-of-detail is a *signed* integer because a *reconstructed* raster
		 * can have a negative LOD (useful when reconstructed raster uses an age grid mask that is
		 * higher resolution than the source raster itself).
		 *
		 * See base class for more details.
		 */
		virtual
		int
		clamp_level_of_detail(
				float level_of_detail) const;


		using GLMultiResolutionRasterInterface::render;


		/**
		 * Renders all tiles visible in the view frustum (determined by the current model-view/projection
		 * transforms of @a renderer) and returns true if any tiles were rendered.
		 *
		 * Throws exception if @a level_of_detail is outside the valid range.
		 * Use @a clamp_level_of_detail to clamp to a valid range before calling this method.
		 *
		 * See base class for more details.
		 */
		virtual
		bool
		render(
				GLRenderer &renderer,
				int level_of_detail,
				cache_handle_type &cache_handle);


		/**
		 * Returns the tile texel dimension of this raster which is also the tile texel dimension
		 * of the source cube raster.
		 */
		std::size_t
		get_tile_texel_dimension() const
		{
			return d_tile_texel_dimension;
		}


		/**
		 * Returns the texture internal format that can be used if rendering to a texture, when
		 * calling @a render, as opposed to the main framebuffer.
		 *
		 * This is the 'internalformat' parameter of GLTexture::gl_tex_image_2D for example.
		 *
		 * NOTE: The filtering mode is expected to be set to 'linear' in all cases except floating-point rasters.
		 * Because earlier hardware, that supports floating-point textures, does not implement
		 * bilinear filtering (any linear filtering will need to be emulated in a pixel shader).
		 */
		GLint
		get_target_texture_internal_format() const
		{
			// Delegate to our source raster input.
			return d_raster_to_reconstruct->get_tile_texture_internal_format();
		}

	private:
		/**
		 * Typedef for a @a GLCubeSubvision cache.
		 */
		typedef GLCubeSubdivisionCache<
				true/*CacheProjectionTransform*/, false/*CacheLooseProjectionTransform*/,
				false/*CacheFrustum*/, false/*CacheLooseFrustum*/,
				false/*CacheBoundingPolygon*/, false/*CacheLooseBoundingPolygon*/,
				true/*CacheBounds*/, false/*CacheLooseBounds*/>
						cube_subdivision_cache_type;

		/**
		 * Typedef for a @a GLCubeSubvision cache.
		 */
		typedef GLCubeSubdivisionCache<
				true/*CacheProjectionTransform*/, false/*CacheLooseProjectionTransform*/,
				false/*CacheFrustum*/, false/*CacheLooseFrustum*/,
				false/*CacheBoundingPolygon*/, false/*CacheLooseBoundingPolygon*/,
				false/*CacheBounds*/, false/*CacheLooseBounds*/>
						clip_cube_subdivision_cache_type;


		/**
		 * Age grid mask and coverage cube rasters.
		 */
		struct AgeGridCubeRaster
		{
			AgeGridCubeRaster(
					GLRenderer &renderer,
					const AgeGridRaster &age_grid_raster,
					std::size_t tile_texel_dimension) :
				age_grid_mask_cube_raster(
						// Create an age grid mask multi-resolution cube raster.
						GPlatesOpenGL::GLMultiResolutionCubeRaster::create(
								renderer,
								age_grid_raster.age_grid_mask_raster,
								tile_texel_dimension,
								// We want the tile dimension to match the source raster exactly...
								false/*adapt_tile_dimension_to_source_resolution*/,
								// Avoids blending seams due to bilinear and/or anisotropic filtering which
								// gives age grid mask alpha values that are not either 0.0 or 1.0...
								GPlatesOpenGL::GLMultiResolutionCubeRaster::FIXED_POINT_TEXTURE_FILTER_MAG_NEAREST)),
				age_grid_coverage_cube_raster(
						// Create an age grid coverage multi-resolution cube raster.
						GPlatesOpenGL::GLMultiResolutionCubeRaster::create(
								renderer,
								age_grid_raster.age_grid_coverage_raster,
								tile_texel_dimension,
								// We want the tile dimension to match the source raster exactly...
								false/*adapt_tile_dimension_to_source_resolution*/,
								// Avoids blending seams due to bilinear and/or anisotropic filtering which
								// gives age grid coverage alpha values that are not either 0.0 or 1.0...
								GPlatesOpenGL::GLMultiResolutionCubeRaster::FIXED_POINT_TEXTURE_FILTER_MAG_NEAREST))

			{  }

			GLMultiResolutionCubeRaster::non_null_ptr_type age_grid_mask_cube_raster;
			GLMultiResolutionCubeRaster::non_null_ptr_type age_grid_coverage_cube_raster;

			// Keep track of changes to the age grid input rasters.
			mutable GPlatesUtils::ObserverToken d_age_grid_mask_cube_raster_observer_token;
			mutable GPlatesUtils::ObserverToken d_age_grid_coverage_cube_raster_observer_token;
		};


		/**
		 * Maintains an age-masked source tile (render-texture) and its source tile cache handles
		 * (such as source raster, age grid mask and coverage rasters).
		 */
		struct AgeMaskedSourceTileTexture
		{
			explicit
			AgeMaskedSourceTileTexture(
					GLRenderer &renderer_) :
				texture(GLTexture::create_as_auto_ptr(renderer_))
			{  }

			/**
			 * Clears the source caches.
			 *
			 * Called when 'this' tile texture is returned to the cache (so texture can be reused).
			 */
			void
			returned_to_cache()
			{
				source_raster_cache_handle.reset();
				age_grid_mask_cache_handle.reset();
				age_grid_coverage_cache_handle.reset();
			}

			GLTexture::shared_ptr_type texture;
			GLMultiResolutionRaster::cache_handle_type source_raster_cache_handle;
			GLMultiResolutionRaster::cache_handle_type age_grid_mask_cache_handle;
			GLMultiResolutionRaster::cache_handle_type age_grid_coverage_cache_handle;
		};

		/**
		 * Typedef for a cache of age masked source tile textures.
		 */
		typedef GPlatesUtils::ObjectCache<AgeMaskedSourceTileTexture> age_masked_source_tile_texture_cache_type;


		/**
		 * Used to cache information, specific to a quad tree node, *during* traversal of the source raster.
		 *
		 * The cube quad tree containing these nodes is destroyed at the end of each render.
		 *
		 * NOTE: The members are optional because the internal nodes of the cube quad tree will not get rendered.
		 */
		struct RenderQuadTreeNode
		{
			/**
			 * The state set used to render a tile (also references the tile source texture).
			 *
			 * NOTE: Since our cube quad tree is destroyed at the end of the render call we
			 * don't need to worry about the chance of indefinitely holding a reference to any
			 * cached state such as a tile texture which would prevent it from being re-used
			 * (even if this tile is not visible later on).
			 */
			boost::optional<GLCompiledDrawState::non_null_ptr_to_const_type> scene_tile_compiled_draw_state;
		};

		/**
		 * Typedef for a cube quad tree used during traversal of the source raster.
		 *
		 * Note that this cube quad tree's scope is limited to the @a render method.
		 * It provides a cache to avoid recalculating information as the source raster
		 * is traversed for each transform in the reconstructed polygon meshes.
		 *
		 * NOTE: The members are optional because the internal nodes of the cube quad tree will not get rendered.
		 */
		typedef GPlatesMaths::CubeQuadTree<RenderQuadTreeNode> render_traversal_cube_quad_tree_type;


		/**
		 * Used to cache information, specific to a quad tree node, to return to the client for caching.
		 *
		 * The cube quad tree containing these nodes is then returned to the client to cache until
		 * the next time it renders us (this prevents our objects being prematurely recycled by our caches).
		 */
		struct ClientCacheQuadTreeNode
		{
			/**
			 * The age masked render texture - prevents it from being recycled.
			 *
			 * NOTE: This is only used when an age grid raster *is* involved.
			 */
			boost::optional<age_masked_source_tile_texture_cache_type::object_shared_ptr_type> age_masked_source_tile_texture;

			/**
			 * The cache handle for the source raster - prevents source raster tile being recycled.
			 *
			 * NOTE: This is only used when an age grid raster is *not* involved.
			 */
			boost::optional<GLMultiResolutionCubeRaster::cache_handle_type> source_raster_cache_handle;
		};

		/**
		 * Typedef for a cube quad tree to return to the client, at each 'render' call, containing
		 * cached state that should be kept alive to prevent prematurely recycling our objects.
		 */
		typedef GPlatesMaths::CubeQuadTree<ClientCacheQuadTreeNode> client_cache_cube_quad_tree_type;


		/**
		 * Used to cache information, specific to a quad tree node, *across* render traversals (across frames).
		 *
		 * The cube quad tree containing these nodes persists across multiple render calls.
		 */
		struct QuadTreeNode
		{
			explicit
			QuadTreeNode(
					const age_masked_source_tile_texture_cache_type::volatile_object_ptr_type &age_masked_source_tile_texture_) :
				age_masked_source_tile_texture(age_masked_source_tile_texture_)
			{  }

			/**
			 * Keeps tracks of whether the source raster has changed underneath us and
			 * we need to re-render our age-masked tile texture.
			 */
			mutable GPlatesUtils::ObserverToken source_raster_observer_token;

			/**
			 * Keeps tracks of whether the age grid mask raster has changed underneath us and
			 * we need to re-render our age-masked tile texture.
			 */
			mutable GPlatesUtils::ObserverToken age_grid_mask_observer_token;

			/**
			 * Keeps tracks of whether the age grid coverage raster has changed underneath us and
			 * we need to re-render our age-masked tile texture.
			 */
			mutable GPlatesUtils::ObserverToken age_grid_coverage_observer_token;

			/**
			 * The (volatile) cached age-masked source tile texture.
			 */
			age_masked_source_tile_texture_cache_type::volatile_object_ptr_type age_masked_source_tile_texture;
		};

		/**
		 * Typedef for a cube quad tree that persists from one render call to the next.
		 *
		 * The primary purpose of this cube quad tree is to cache age-masked render textures
		 * that are visible so they can be re-used is subsequent render calls (frames).
		 */
		typedef GPlatesMaths::CubeQuadTree<QuadTreeNode> cube_quad_tree_type;


		//! Typedef for a raster cube quad tree node.
		typedef GLMultiResolutionCubeRaster::quad_tree_node_type raster_quad_tree_node_type;

		//! Typedef for a cube quad tree of age grid mask tiles.
		typedef GLMultiResolutionCubeRaster::quad_tree_node_type age_grid_mask_quad_tree_node_type;

		//! Typedef for a cube quad tree of age grid coverage tiles.
		typedef GLMultiResolutionCubeRaster::quad_tree_node_type age_grid_coverage_quad_tree_node_type;

		//! Typedef for a sequence of present day polygon mesh drawables.
		typedef GLReconstructedStaticPolygonMeshes::present_day_polygon_mesh_drawables_seq_type
				present_day_polygon_mesh_drawables_seq_type;

		/**
		 * Typedef for a cube quad tree representing possible intersections of present day polygon
		 * meshes with each cube quad tree node.
		 */
		typedef GLReconstructedStaticPolygonMeshes::PresentDayPolygonMeshesNodeIntersections
				present_day_polygon_meshes_node_intersections_type;
		typedef present_day_polygon_meshes_node_intersections_type::intersection_partition_type
				present_day_polygon_meshes_intersection_partition_type;

		//! Typedef for membership of present day polygon meshes.
		typedef GLReconstructedStaticPolygonMeshes::PresentDayPolygonMeshMembership
				present_day_polygon_mesh_membership_type;

		//! Typedef for a sequence of transform groups (of reconstructed polygon meshes).
		typedef GLReconstructedStaticPolygonMeshes::reconstructed_polygon_mesh_transform_group_seq_type
				reconstructed_polygon_mesh_transform_group_seq_type;

		//! Typedef for a transform group (of reconstructed polygon meshes).
		typedef GLReconstructedStaticPolygonMeshes::ReconstructedPolygonMeshTransformGroup
				reconstructed_polygon_mesh_transform_group_type;

		//! Typedef for a sequences of transform groups (of reconstructed polygon meshes).
		typedef GLReconstructedStaticPolygonMeshes::ReconstructedPolygonMeshTransformsGroups
				reconstructed_polygon_mesh_transform_groups_type;

		//! Typedef for a sequence of drawables.
		typedef std::vector<GLCompiledDrawState::non_null_ptr_to_const_type> drawable_seq_type;


		/**
		 * The re-sampled raster we are reconstructing.
		 */
		GLMultiResolutionCubeRaster::non_null_ptr_type d_raster_to_reconstruct;

		//! Keep track of changes to @a d_raster_to_reconstruct.
		mutable GPlatesUtils::ObserverToken d_raster_to_reconstruct_texture_observer_token;

		/**
		 * The reconstructed present day static polygon meshes.
		 */
		GLReconstructedStaticPolygonMeshes::non_null_ptr_type d_reconstructed_static_polygon_meshes;

		//! Keep track of changes to @a d_reconstructed_static_polygon_meshes.
		mutable GPlatesUtils::ObserverToken d_reconstructed_static_polygon_meshes_observer_token;

		/**
		 * Optional age grid mask and coverage rasters.
		 *
		 * If these are not specified then no age-grid smoothing takes place during reconstruction.
		 */
		boost::optional<AgeGridCubeRaster> d_age_grid_cube_raster;

		/**
		 * The texture dimension of a cube quad tree tile.
		 */
		unsigned int d_tile_texel_dimension;

		//! 1.0 / 'd_tile_texel_dimension'.
		float d_inverse_tile_texel_dimension;

		/**
		 * Tile UV scaling/translating starts with this root UV transform (has texel overlap built in).
		 *
		 * Can be used for both source raster tile and age-grid tile since both have same texel dimension.
		 */
		GLUtils::QuadTreeUVTransform d_tile_root_uv_transform;

		/**
		 * Cache of textures for age-masked source tiles.
		 */
		age_masked_source_tile_texture_cache_type::shared_ptr_type d_age_masked_source_tile_texture_cache;

		/**
		 * Texture used to clip parts of a mesh that hang over a tile (in the cube face x/y plane).
		 */
		GLTexture::shared_ptr_type d_xy_clip_texture;

		/**
		 * Texture used to clip parts of a mesh that are inside a tile's frustum but on
		 * the opposite side of the globe (in the cube face z axis).
		 *
		 * Some polygon meshes are large enough to wrap around the globe and can appear in both
		 * a cube tile's frustum and the mirror image of that frustum on the opposite side of the
		 * globe (mirrored orthogonal to the local z-axis of the cube face that the tile is within).
		 *
		 * This texture is used to clip away negative z values.
		 */
		GLTexture::shared_ptr_type d_z_clip_texture;

		/**
		 * Matrix to convert texture coordinates from range [0,1] to range [0.25, 0.75] to
		 * map to the interior 2x2 texel region of the 4x4 clip texture.
		 */
		GLMatrix d_xy_clip_texture_transform;

		// Used to draw a textured full-screen quad into render texture.
		GLCompiledDrawState::non_null_ptr_to_const_type d_full_screen_quad_drawable;

		/**
		 * Shader program to render *floating-point* texture tiles to the scene.
		 *
		 * Is boost::none if source raster is fixed-point or if shader programs not supported
		 * (in which case fixed-function pipeline is used).
		 */
		boost::optional<GLProgramObject::shared_ptr_type> d_render_floating_point_tile_to_scene_program_object;

		/**
		 * Shader program to modulate a *floating-point* source raster tile with a (fixed-point) age mask.
		 *
		 * Is boost::none if source raster is fixed-point or if shader programs not supported
		 * (in which case fixed-function pipeline is used).
		 */
		boost::optional<GLProgramObject::shared_ptr_type>
				d_render_floating_point_age_masked_source_raster_third_pass_program_object;

		/**
		 * Caches age-masked render textures for the cube quad tree tiles for re-use over multiple frames.
		 */
		cube_quad_tree_type::non_null_ptr_type d_cube_quad_tree;

		/**
		 * Used to inform clients that we have been updated.
		 */
		mutable GPlatesUtils::SubjectToken d_subject_token;


		//! Constructor.
		GLMultiResolutionStaticPolygonReconstructedRaster(
				GLRenderer &renderer,
				const GLMultiResolutionCubeRaster::non_null_ptr_type &raster_to_reconstruct,
				const GLReconstructedStaticPolygonMeshes::non_null_ptr_type &reconstructed_static_polygon_meshes,
				boost::optional<AgeGridRaster> age_grid_raster);

		void
		render_quad_tree_source_raster(
				GLRenderer &renderer,
				render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
				render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
				client_cache_cube_quad_tree_type &client_cache_cube_quad_tree,
				client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
				const raster_quad_tree_node_type &source_raster_quad_tree_node,
				const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
				clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
				const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
				unsigned int cube_quad_tree_depth,
				unsigned int render_cube_quad_tree_depth,
				const GLFrustum &frustum_planes,
				boost::uint32_t frustum_plane_mask,
				unsigned int &num_tiles_rendered_to_scene);

		void
		render_quad_tree_scaled_source_raster_and_unscaled_age_grid(
				GLRenderer &renderer,
				cube_quad_tree_type::node_type &cube_quad_tree_node,
				render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
				render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
				client_cache_cube_quad_tree_type &client_cache_cube_quad_tree,
				client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
				const raster_quad_tree_node_type &source_raster_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
				const age_grid_mask_quad_tree_node_type &age_grid_mask_quad_tree_node,
				const age_grid_coverage_quad_tree_node_type &age_grid_coverage_quad_tree_node,
				const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
				const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
				clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
				const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
				unsigned int cube_quad_tree_depth,
				unsigned int render_cube_quad_tree_depth,
				const GLFrustum &frustum_planes,
				boost::uint32_t frustum_plane_mask,
				unsigned int &num_tiles_rendered_to_scene);

		void
		render_quad_tree_unscaled_source_raster_and_scaled_age_grid(
				GLRenderer &renderer,
				cube_quad_tree_type::node_type &cube_quad_tree_node,
				render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
				render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
				client_cache_cube_quad_tree_type &client_cache_cube_quad_tree,
				client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
				const raster_quad_tree_node_type &source_raster_quad_tree_node,
				const age_grid_mask_quad_tree_node_type &age_grid_mask_quad_tree_node,
				const age_grid_coverage_quad_tree_node_type &age_grid_coverage_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &age_grid_uv_transform,
				const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
				const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
				clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
				const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
				unsigned int cube_quad_tree_depth,
				unsigned int render_cube_quad_tree_depth,
				const GLFrustum &frustum_planes,
				boost::uint32_t frustum_plane_mask,
				unsigned int &num_tiles_rendered_to_scene);

		void
		render_quad_tree_source_raster_and_age_grid(
				GLRenderer &renderer,
				cube_quad_tree_type::node_type &cube_quad_tree_node,
				render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
				render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
				client_cache_cube_quad_tree_type &client_cache_cube_quad_tree,
				client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
				const raster_quad_tree_node_type &source_raster_quad_tree_node,
				const age_grid_mask_quad_tree_node_type &age_grid_mask_quad_tree_node,
				const age_grid_coverage_quad_tree_node_type &age_grid_coverage_quad_tree_node,
				const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
				const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
				clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
				const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
				unsigned int cube_quad_tree_depth,
				unsigned int render_cube_quad_tree_depth,
				const GLFrustum &frustum_planes,
				boost::uint32_t frustum_plane_mask,
				unsigned int &num_tiles_rendered_to_scene);

		bool
		cull_quad_tree(
				const present_day_polygon_mesh_membership_type &reconstructed_polygon_mesh_membership,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
				const GLFrustum &frustum_planes,
				boost::uint32_t &frustum_plane_mask);

		GLCompiledDrawState::non_null_ptr_to_const_type
		create_scene_tile_compiled_draw_state(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &scene_tile_texture,
				const GLUtils::QuadTreeUVTransform &scene_tile_uv_transform,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
				clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
				const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node);

		void
		render_source_raster_tile_to_scene(
				GLRenderer &renderer,
				render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
				client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
				const raster_quad_tree_node_type &source_raster_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
				const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
				clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
				const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
				unsigned int &num_tiles_rendered_to_scene);

		void
		render_tile_to_scene(
				GLRenderer &renderer,
				const GLCompiledDrawState::non_null_ptr_to_const_type &render_scene_tile_compiled_draw_state,
				const present_day_polygon_mesh_membership_type &reconstructed_polygon_mesh_membership,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
				unsigned int &num_tiles_rendered_to_scene);

		void
		render_polygon_drawables(
				GLRenderer &renderer,
				const boost::dynamic_bitset<> &polygon_mesh_membership,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables);

		void
		render_source_raster_and_age_grid_tile_to_scene(
				GLRenderer &renderer,
				cube_quad_tree_type::node_type &cube_quad_tree_node,
				render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
				client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
				const raster_quad_tree_node_type &source_raster_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
				const age_grid_mask_quad_tree_node_type &age_grid_mask_quad_tree_node,
				const age_grid_coverage_quad_tree_node_type &age_grid_coverage_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &age_grid_uv_transform,
				const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
				const reconstructed_polygon_mesh_transform_group_type &reconstructed_polygon_mesh_transform_group,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
				clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
				const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
				unsigned int &num_tiles_rendered_to_scene);

		GLTexture::shared_ptr_to_const_type
		get_age_masked_source_raster_tile(
				GLRenderer &renderer,
				cube_quad_tree_type::node_type &cube_quad_tree_node,
				client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
				const raster_quad_tree_node_type &source_raster_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
				const age_grid_mask_quad_tree_node_type &age_grid_mask_quad_tree_node,
				const age_grid_coverage_quad_tree_node_type &age_grid_coverage_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &age_grid_uv_transform,
				const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node);

		void
		render_age_masked_source_raster_into_tile(
				GLRenderer &renderer,
				AgeMaskedSourceTileTexture &age_masked_source_tile_texture,
				cube_quad_tree_type::node_type &cube_quad_tree_node,
				const raster_quad_tree_node_type &source_raster_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
				const age_grid_mask_quad_tree_node_type &age_grid_mask_quad_tree_node,
				const age_grid_coverage_quad_tree_node_type &age_grid_coverage_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &age_grid_uv_transform,
				const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node);

		void
		render_floating_point_age_masked_source_raster_into_tile(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &age_mask_source_texture,
				const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
				const present_day_polygon_mesh_membership_type &present_day_polygons_intersecting_tile,
				const GLTransform &half_texel_expanded_projection_transform,
				const GLTransform &view_transform,
				const GLMatrix &source_raster_tile_coverage_texture_matrix,
				const GLMatrix &age_grid_partial_tile_coverage_texture_matrix,
				const GLTexture::shared_ptr_to_const_type &source_raster_texture,
				const GLTexture::shared_ptr_to_const_type &age_grid_mask_texture,
				const GLTexture::shared_ptr_to_const_type &age_grid_coverage_texture);

		void
		render_fixed_point_age_masked_source_raster_into_tile(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &age_mask_source_texture,
				const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables,
				const present_day_polygon_mesh_membership_type &present_day_polygons_intersecting_tile,
				const GLTransform &half_texel_expanded_projection_transform,
				const GLTransform &view_transform,
				const GLMatrix &source_raster_tile_coverage_texture_matrix,
				const GLMatrix &age_grid_partial_tile_coverage_texture_matrix,
				const GLTexture::shared_ptr_to_const_type &source_raster_texture,
				const GLTexture::shared_ptr_to_const_type &age_grid_mask_texture,
				const GLTexture::shared_ptr_to_const_type &age_grid_coverage_texture);

		void
		render_fixed_function_pipeline_first_pass_age_masked_source_raster(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &age_grid_mask_texture,
				const GLMatrix &age_grid_partial_tile_coverage_texture_matrix);

		void
		render_fixed_function_pipeline_second_pass_age_masked_source_raster(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &age_grid_coverage_texture,
				const GLMatrix &age_grid_partial_tile_coverage_texture_matrix,
				const GLTransform &half_texel_expanded_projection_transform,
				const GLTransform &view_transform,
				const reconstructed_polygon_mesh_transform_groups_type &reconstructed_polygon_mesh_transform_groups,
				const present_day_polygon_mesh_membership_type &present_day_polygons_intersecting_tile,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables);

		void
		render_fixed_function_pipeline_third_pass_age_masked_source_raster(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_to_const_type &source_raster_texture,
				const GLMatrix &source_raster_tile_coverage_texture_matrix);

		void
		create_floating_point_shader_programs(
				GLRenderer &renderer);
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONSTATICPOLYGONRECONSTRUCTEDRASTER_H
