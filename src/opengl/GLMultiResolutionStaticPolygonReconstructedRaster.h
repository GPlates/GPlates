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
#include <boost/pool/object_pool.hpp>
#include <boost/ref.hpp>
#include <boost/shared_ptr.hpp>

#include "GLCompiledDrawState.h"
#include "GLCubeSubdivisionCache.h"
#include "GLLight.h"
#include "GLMatrix.h"
#include "GLMultiResolutionCubeMesh.h"
#include "GLMultiResolutionCubeRaster.h"
#include "GLMultiResolutionRasterInterface.h"
#include "GLProgramObject.h"
#include "GLReconstructedStaticPolygonMeshes.h"
#include "GLTexture.h"
#include "GLUtils.h"
#include "GLViewport.h"

#include "app-logic/ReconstructedFeatureGeometry.h"

#include "gui/SceneLightingParameters.h"

#include "maths/MathsFwd.h"
#include "maths/CubeQuadTree.h"
#include "maths/CubeQuadTreePartition.h"
#include "maths/CubeQuadTreePartitionUtils.h"
#include "maths/UnitQuaternion3D.h"

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
		 * Returns true if floating-point source raster is supported.
		 *
		 * If false is returned then only a fixed-point format source texture can be used.
		 *
		 * This is effectively a test for support of the 'GL_EXT_framebuffer_object' extension.
		 */
		static
		bool
		supports_floating_point_source_raster(
				GLRenderer &renderer);

		/**
		 * Returns true if age masks can be generated on the runtime system.
		 *
		 * This means a floating-point raster containing actual age values can be passed into
		 * @a create (if using age grids) instead of a fixed-point raster containing pre-generated
		 * age masks (results of age comparisons against a specific reconstruction time).
		 *
		 * The age comparisons are then done by a shader program using the current reconstruction time.
		 *
		 * NOTE: Age masking can still be used if this function returns false - it's just not as
		 * fast and not as good quality.
		 */
		static
		bool
		supports_age_mask_generation(
				GLRenderer &renderer);

		/**
		 * Returns true if a normal map can be used on the runtime system to accentuate surface lighting.
		 *
		 * NOTE: Internally this also calls 'GLMultiResolutionRaster::supports_normal_map_source()'.
		 */
		static
		bool
		supports_normal_map(
				GLRenderer &renderer);


		/**
		 * Creates a @a GLMultiResolutionStaticPolygonReconstructedRaster object that is reconstructed
		 * using static polygon meshes.
		 *
		 * If @a source_raster is floating-point (which means this reconstructed raster will also be
		 * floating-point) then @a supports_floating_point_source_raster *must* return true.
		 *
		 * NOTE: The following applies when using an age grid (ie, a non-null @a age_grid_raster)...
		 * If @a supports_age_mask_generation returns true then @a age_grid_raster *must*
		 * be sourced from a @a GLDataRasterSource (containing actual age values), otherwise it *must*
		 * be sourced from a @a GLAgeGridMaskSource (containing results of age comparisons against
		 * a specific reconstruction time.
		 *
		 * @param source_raster the raster to be reconstructed.
		 * @param reconstructed_static_polygon_meshes the reconstructed present day polygon meshes.
		 * @param age_grid_raster is the optional age grid raster.
		 * @param normal_map_raster is the optional normal map raster (used during surface lighting).
		 * @param light is the light direction used for surface lighting.
		 *
		 * NOTE: @a normal_map_raster and @a light are only used for surface lighting and
		 * surface lighting is ignored for a floating-point source raster (from a @a GLDataRasterSource)
		 * because it is analysed and not visualised.
		 * NOTE: If @a light is specified without @a normal_map_raster then lighting is still applied
		 * but without the perturbed lighting due to the bumps in a normal map.
		 *
		 * NOTE: If @a light is specified but lighting is not supported on the run-time system
		 * (eg, due to exceeding shader instructions limit) then the raster will not be rendered
		 * with surface lighting.
		 */
		static
		non_null_ptr_type
		create(
				GLRenderer &renderer,
				const double &reconstruction_time,
				const GLMultiResolutionCubeRaster::non_null_ptr_type &source_raster,
				const GLReconstructedStaticPolygonMeshes::non_null_ptr_type &reconstructed_static_polygon_meshes,
				boost::optional<GLMultiResolutionCubeRaster::non_null_ptr_type> age_grid_raster = boost::none,
				boost::optional<GLMultiResolutionCubeRaster::non_null_ptr_type> normal_map_raster = boost::none,
				boost::optional<GLLight::non_null_ptr_type> light = boost::none)
		{
			return non_null_ptr_type(
					new GLMultiResolutionStaticPolygonReconstructedRaster(
							renderer,
							reconstruction_time,
							source_raster,
							reconstructed_static_polygon_meshes,
							boost::none/*multi_resolution_cube_mesh*/,
							age_grid_raster,
							normal_map_raster,
							light));
		}


		/**
		 * Same as the other overload of @a create but creates a
		 * @a GLMultiResolutionStaticPolygonReconstructedRaster object that is not reconstructed.
		 *
		 * Instead of reconstructed static polygon meshes, a multi-resolution cube mesh is used
		 * to render the source raster without reconstructing to paleo positions.
		 *
		 * Note that since there is already a way to render a source raster without reconstruction
		 * in the form of @a GLMultiResolutionRaster, this method is only useful when an age grid and/or
		 * normal map is used (otherwise @a GLMultiResolutionRaster is faster and uses less memory).
		 */
		static
		non_null_ptr_type
		create(
				GLRenderer &renderer,
				const double &reconstruction_time,
				const GLMultiResolutionCubeRaster::non_null_ptr_type &source_raster,
				const GLMultiResolutionCubeMesh::non_null_ptr_to_const_type &multi_resolution_cube_mesh,
				boost::optional<GLMultiResolutionCubeRaster::non_null_ptr_type> age_grid_raster = boost::none,
				boost::optional<GLMultiResolutionCubeRaster::non_null_ptr_type> normal_map_raster = boost::none,
				boost::optional<GLLight::non_null_ptr_type> light = boost::none)
		{
			return non_null_ptr_type(
					new GLMultiResolutionStaticPolygonReconstructedRaster(
							renderer,
							reconstruction_time,
							source_raster,
							boost::none/*reconstructed_static_polygon_meshes*/,
							multi_resolution_cube_mesh,
							age_grid_raster,
							normal_map_raster,
							light));
		}


		/**
		 * Updates the current reconstruction time.
		 *
		 * This is currently used for age comparisons with the age grid.
		 */
		void
		update(
				const double &reconstruction_time);


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
			return d_source_raster->get_num_levels_of_detail();
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
		float
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
				float level_of_detail,
				cache_handle_type &cache_handle);


		/**
		 * Returns the tile texel dimension of this raster which is also the tile texel dimension
		 * of the source cube raster.
		 */
		std::size_t
		get_tile_texel_dimension() const
		{
			return d_source_raster_tile_texel_dimension;
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
			return d_source_raster->get_tile_texture_internal_format();
		}

	private:
		/**
		 * Typedef for the source raster @a GLCubeSubvision cache.
		 */
		typedef GLCubeSubdivisionCache<
				true/*CacheProjectionTransform*/, false/*CacheLooseProjectionTransform*/,
				false/*CacheFrustum*/, false/*CacheLooseFrustum*/,
				false/*CacheBoundingPolygon*/, false/*CacheLooseBoundingPolygon*/,
				true/*CacheBounds*/, false/*CacheLooseBounds*/>
						source_raster_cube_subdivision_cache_type;

		/**
		 * Typedef for the age grid @a GLCubeSubvision cache.
		 *
		 * Note that it doesn't not cache bounds like @a source_raster_cube_subdivision_cache_type.
		 */
		typedef GLCubeSubdivisionCache<
				true/*CacheProjectionTransform*/, false/*CacheLooseProjectionTransform*/,
				false/*CacheFrustum*/, false/*CacheLooseFrustum*/,
				false/*CacheBoundingPolygon*/, false/*CacheLooseBoundingPolygon*/,
				false/*CacheBounds*/, false/*CacheLooseBounds*/>
						age_grid_cube_subdivision_cache_type;

		/**
		 * Typedef for the normal map @a GLCubeSubvision cache.
		 *
		 * Note that it doesn't not cache bounds like @a source_raster_cube_subdivision_cache_type.
		 */
		typedef GLCubeSubdivisionCache<
				true/*CacheProjectionTransform*/, false/*CacheLooseProjectionTransform*/,
				false/*CacheFrustum*/, false/*CacheLooseFrustum*/,
				false/*CacheBoundingPolygon*/, false/*CacheLooseBoundingPolygon*/,
				false/*CacheBounds*/, false/*CacheLooseBounds*/>
						normal_map_cube_subdivision_cache_type;

		/**
		 * Typedef for clip @a GLCubeSubvision cache.
		 */
		typedef GLCubeSubdivisionCache<
				true/*CacheProjectionTransform*/, false/*CacheLooseProjectionTransform*/,
				false/*CacheFrustum*/, false/*CacheLooseFrustum*/,
				false/*CacheBoundingPolygon*/, false/*CacheLooseBoundingPolygon*/,
				false/*CacheBounds*/, false/*CacheLooseBounds*/>
						clip_cube_subdivision_cache_type;


		/**
		 * Age grid cube raster.
		 */
		struct AgeGridCubeRaster
		{
			explicit
			AgeGridCubeRaster(
					GLMultiResolutionCubeRaster::non_null_ptr_type age_grid_mask_cube_raster_) :
				cube_raster(age_grid_mask_cube_raster_),
				tile_texel_dimension(age_grid_mask_cube_raster_->get_tile_texel_dimension()),
				inverse_tile_texel_dimension(1.0f / age_grid_mask_cube_raster_->get_tile_texel_dimension()),
				tile_root_uv_transform(
						GLUtils::QuadTreeUVTransform::get_expand_tile_ratio(
								age_grid_mask_cube_raster_->get_tile_texel_dimension(),
								0.5/*tile_border_overlap_in_texels*/))
			{  }

			GLMultiResolutionCubeRaster::non_null_ptr_type cube_raster;

			//! The age grid tile texture dimension.
			unsigned int tile_texel_dimension;

			//! 1.0 / 'tile_texel_dimension'.
			float inverse_tile_texel_dimension;

			//! Age grid tile UV scaling/translating starts with this root UV transform (has texel overlap built in).
			GLUtils::QuadTreeUVTransform tile_root_uv_transform;

			// Keep track of changes to the age grid input rasters.
			mutable GPlatesUtils::ObserverToken cube_raster_observer_token;
		};


		/**
		 * Normal map cube raster.
		 */
		struct NormalMapCubeRaster
		{
			explicit
			NormalMapCubeRaster(
					GLMultiResolutionCubeRaster::non_null_ptr_type normal_map_cube_raster_) :
				cube_raster(normal_map_cube_raster_),
				tile_texel_dimension(normal_map_cube_raster_->get_tile_texel_dimension()),
				inverse_tile_texel_dimension(1.0f / normal_map_cube_raster_->get_tile_texel_dimension()),
				tile_root_uv_transform(
						GLUtils::QuadTreeUVTransform::get_expand_tile_ratio(
								normal_map_cube_raster_->get_tile_texel_dimension(),
								0.5/*tile_border_overlap_in_texels*/))
			{  }

			GLMultiResolutionCubeRaster::non_null_ptr_type cube_raster;

			//! The age grid tile texture dimension.
			unsigned int tile_texel_dimension;

			//! 1.0 / 'tile_texel_dimension'.
			float inverse_tile_texel_dimension;

			//! Age grid tile UV scaling/translating starts with this root UV transform (has texel overlap built in).
			GLUtils::QuadTreeUVTransform tile_root_uv_transform;

			// Keep track of changes to the age grid input rasters.
			mutable GPlatesUtils::ObserverToken cube_raster_observer_token;
		};


		/**
		 * Used to cache information, specific to a quad tree node, *during* traversal of the source raster.
		 *
		 * The cube quad tree containing these nodes is destroyed at the end of each render.
		 *
		 * NOTE: The members are optional because the internal nodes don't get rendered.
		 */
		struct RenderQuadTreeNode
		{
			struct TileDrawState
			{
				explicit
				TileDrawState(
						const GLCompiledDrawState::non_null_ptr_to_const_type &compiled_draw_state_,
						boost::optional<GLProgramObject::shared_ptr_type> program_object_ = boost::none) :
					compiled_draw_state(compiled_draw_state_),
					program_object(program_object_)
				{  }

				// Any state set by calling GLRenderer.
				GLCompiledDrawState::non_null_ptr_to_const_type compiled_draw_state;

				// Optional program object if not using the fixed-function pipeline.
				boost::optional<GLProgramObject::shared_ptr_type> program_object;

				// Optional uniform variables in program object.
				// These cannot be stored in a 'GLCompiledDrawState'.

				boost::optional<GLMatrix> source_raster_texture_transform;
				boost::optional<unsigned int> source_texture_sampler;

				boost::optional<GLMatrix> clip_texture_transform;
				boost::optional<unsigned int> clip_texture_sampler;

				boost::optional<GLMatrix> age_grid_texture_transform;
				boost::optional<unsigned int> age_grid_texture_sampler;
				boost::optional<GLfloat> reconstruction_time;

				boost::optional<GLMatrix> normal_map_texture_transform;
				boost::optional<unsigned int> normal_map_texture_sampler;

				boost::optional<GLfloat> ambient_and_diffuse_lighting;
				boost::optional<GLfloat> light_ambient_contribution;
				boost::optional<GPlatesMaths::UnitVector3D> world_space_light_direction;
				boost::optional<unsigned int> light_direction_cube_texture_sampler;
			};

			/**
			 * The compiled draw state used to render a tile.
			 *
			 * NOTE: Is actually a vector of compiled draw states since age grid rendering needs one
			 * each for active and inactive polygons.
			 *
			 * NOTE: Since our cube quad tree is destroyed at the end of the render call we
			 * don't need to worry about the chance of indefinitely holding a reference to any
			 * cached state such as a tile texture.
			 */
			boost::optional< std::vector<TileDrawState> > scene_tile_draw_state;
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
			 * The cache handle for the source raster - prevents source raster tile being recycled.
			 */
			boost::optional<GLMultiResolutionCubeRaster::cache_handle_type> source_raster_cache_handle;

			/*
			 * The cache handle for the age grid mask raster - prevents tile being recycled.
			 *
			 * NOTE: This is only used when an age grid raster is involved.
			 */
			boost::optional<GLMultiResolutionCubeRaster::cache_handle_type> age_grid_mask_cache_handle;

			/*
			 * The cache handle for the normal map raster - prevents tile being recycled.
			 *
			 * NOTE: This is only used when a normal map raster is involved.
			 */
			boost::optional<GLMultiResolutionCubeRaster::cache_handle_type> normal_map_cache_handle;
		};

		/**
		 * Typedef for a cube quad tree to return to the client, at each 'render' call, containing
		 * cached state that should be kept alive to prevent prematurely recycling our objects.
		 */
		typedef GPlatesMaths::CubeQuadTree<ClientCacheQuadTreeNode> client_cache_cube_quad_tree_type;


#if 0	// Not needed anymore but keeping in case needed in the future...
		/**
		 * Used to cache information, specific to a quad tree node, *across* render traversals (across frames).
		 *
		 * The cube quad tree containing these nodes persists across multiple render calls.
		 */
		struct QuadTreeNode
		{
			// Currently this is empty.
			// It used to contain the cached age-masked source textures but that's no longer needed.
			// Keeping this anyway in case something needs to be added in the future.
		};

		/**
		 * Typedef for a cube quad tree that persists from one render call to the next.
		 *
		 * The primary purpose of this cube quad tree used to be to cache age-masked render textures
		 * that are visible so they can be re-used is subsequent render calls (frames).
		 * That's no longer needed but we keep this in case something needs to be added in the future.
		 */
		typedef GPlatesMaths::CubeQuadTree<QuadTreeNode> cube_quad_tree_type;
#endif


		//! Typedef for a raster cube quad tree node.
		typedef GLMultiResolutionCubeRaster::quad_tree_node_type source_raster_quad_tree_node_type;

		//! Typedef for a cube quad tree of age grid mask tiles.
		typedef GLMultiResolutionCubeRaster::quad_tree_node_type age_grid_mask_quad_tree_node_type;

		//! Typedef for a cube quad tree of normal map tiles.
		typedef GLMultiResolutionCubeRaster::quad_tree_node_type normal_map_quad_tree_node_type;

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

		//! Typedef for a quad tree node of a multi-resolution cube mesh.
		typedef GLMultiResolutionCubeMesh::quad_tree_node_type cube_mesh_quad_tree_node_type;

		//! Typedef for a sequence of drawables.
		typedef std::vector<GLCompiledDrawState::non_null_ptr_to_const_type> drawable_seq_type;

		//! Classify the view projection as either a 3D globe view or 2D map view.
		enum ViewType
		{
			GLOBE_VIEW,
			MAP_VIEW
		};


		/**
		 * The current reconstruction time (used for age comparisons with age grid).
		 */
		double d_reconstruction_time;

		/**
		 * The re-sampled source raster we are reconstructing.
		 */
		GLMultiResolutionCubeRaster::non_null_ptr_type d_source_raster;

		//! Keep track of changes to @a d_source_raster.
		mutable GPlatesUtils::ObserverToken d_source_raster_texture_observer_token;

		/**
		 * The reconstructed present day static polygon meshes, or none if raster is being
		 * rendered at present day.
		 */
		boost::optional<GLReconstructedStaticPolygonMeshes::non_null_ptr_type> d_reconstructed_static_polygon_meshes;

		/**
		 * The multi-resolution mesh used when the raster is not reconstructed.
		 */
		boost::optional<GLMultiResolutionCubeMesh::non_null_ptr_to_const_type> d_multi_resolution_cube_mesh;

		//! Keep track of changes to @a d_reconstructed_static_polygon_meshes.
		mutable GPlatesUtils::ObserverToken d_reconstructed_static_polygon_meshes_observer_token;

		/**
		 * Optional age grid raster.
		 *
		 * If this is not specified then no age-grid smoothing takes place during reconstruction.
		 */
		boost::optional<AgeGridCubeRaster> d_age_grid_cube_raster;

		/**
		 * Optional normal map raster.
		 *
		 * If this is not specified then surface lighting (if applied) uses the unperturbed sphere
		 * normals instead of the perturbed surface normals in the normal map.
		 */
		boost::optional<NormalMapCubeRaster> d_normal_map_cube_raster;

		/**
		 * The source raster tile texture dimension.
		 */
		unsigned int d_source_raster_tile_texel_dimension;

		//! 1.0 / 'd_source_raster_tile_texel_dimension'.
		float d_source_raster_inverse_tile_texel_dimension;

		/**
		 * Source raster tile UV scaling/translating starts with this root UV transform (has texel overlap built in).
		 */
		GLUtils::QuadTreeUVTransform d_source_raster_tile_root_uv_transform;

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

		//
		// Shader programs to render raster tiles to the scene.
		//
		// 'boost::none' means shader program variation not needed.
		//

		//! Render a floating-point source raster.
		boost::optional<GLProgramObject::shared_ptr_type> d_render_floating_point_program_object;

		//! Render a floating-point source raster using an age grid with active polygons.
		boost::optional<GLProgramObject::shared_ptr_type>
				d_render_floating_point_with_age_grid_with_active_polygons_program_object;
		//! Render a floating-point source raster using an age grid with inactive polygons.
		boost::optional<GLProgramObject::shared_ptr_type>
				d_render_floating_point_with_age_grid_with_inactive_polygons_program_object;

		//! Render a fixed-point source raster.
		boost::optional<GLProgramObject::shared_ptr_type> d_render_fixed_point_program_object;

		//! Render a fixed-point source raster using an age grid with active polygons.
		boost::optional<GLProgramObject::shared_ptr_type>
				d_render_fixed_point_with_age_grid_with_active_polygons_program_object;
		//! Render a fixed-point source raster using an age grid with inactive polygons.
		boost::optional<GLProgramObject::shared_ptr_type>
				d_render_fixed_point_with_age_grid_with_inactive_polygons_program_object;

		//
		// The following shader program variables are arrays of size two since the surface lighting
		// algorithm depends on whether the view is a 3D globe view or 2D map view.
		//

		//! Render a fixed-point source raster using an age grid with active polygons with surface lighting.
		boost::optional<GLProgramObject::shared_ptr_type>
				d_render_fixed_point_with_age_grid_with_active_polygons_with_surface_lighting_program_object[2];
		//! Render a fixed-point source raster using an age grid with inactive polygons with surface lighting.
		boost::optional<GLProgramObject::shared_ptr_type>
				d_render_fixed_point_with_age_grid_with_inactive_polygons_with_surface_lighting_program_object[2];

		//! Render a fixed-point source raster with surface lighting.
		boost::optional<GLProgramObject::shared_ptr_type> d_render_fixed_point_with_surface_lighting_program_object[2];

		//! Render a fixed-point source raster with surface lighting with a normal map.
		boost::optional<GLProgramObject::shared_ptr_type> d_render_fixed_point_with_normal_map_program_object[2];

		//! Render a fixed-point source raster using an age grid with active polygons with surface lighting with a normal map.
		boost::optional<GLProgramObject::shared_ptr_type>
				d_render_fixed_point_with_age_grid_with_active_polygons_with_normal_map_program_object[2];
		//! Render a fixed-point source raster using an age grid with inactive polygons with surface lighting with a normal map.
		boost::optional<GLProgramObject::shared_ptr_type>
				d_render_fixed_point_with_age_grid_with_inactive_polygons_with_normal_map_program_object[2];


		/**
		 * The light (direction) used during surface lighting.
		 */
		boost::optional<GLLight::non_null_ptr_type> d_light;

		//! Keep track of changes to @a d_light.
		mutable GPlatesUtils::ObserverToken d_light_observer_token;

#if 0	// Not needed anymore but keeping in case needed in the future...
		/**
		 * Caches age-masked render textures for the cube quad tree tiles for re-use over multiple frames.
		 */
		cube_quad_tree_type::non_null_ptr_type d_cube_quad_tree;
#endif

		/**
		 * Used to inform clients that we have been updated.
		 */
		mutable GPlatesUtils::SubjectToken d_subject_token;


		//! Constructor.
		GLMultiResolutionStaticPolygonReconstructedRaster(
				GLRenderer &renderer,
				const double &reconstruction_time,
				const GLMultiResolutionCubeRaster::non_null_ptr_type &source_raster,
				boost::optional<GLReconstructedStaticPolygonMeshes::non_null_ptr_type> reconstructed_static_polygon_meshes,
				boost::optional<GLMultiResolutionCubeMesh::non_null_ptr_to_const_type> multi_resolution_cube_mesh,
				boost::optional<GLMultiResolutionCubeRaster::non_null_ptr_type> age_grid_raster,
				boost::optional<GLMultiResolutionCubeRaster::non_null_ptr_type> normal_map_raster,
				boost::optional<GLLight::non_null_ptr_type> light);

		/**
		 * Returns true if reconstructing raster (using reconstructed static polygon meshes),
		 * otherwise *not* reconstructing raster (instead using @a GLMultiResolutionCubeMesh).
		 */
		bool
		reconstructing_raster() const
		{
			return d_reconstructed_static_polygon_meshes;
		}

		void
		render_transform_group(
				GLRenderer &renderer,
				render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
				client_cache_cube_quad_tree_type &client_cache_cube_quad_tree,
				boost::optional<const reconstructed_polygon_mesh_transform_group_type &> reconstructed_polygon_mesh_transform_group,
				boost::optional<const present_day_polygon_mesh_drawables_seq_type &> polygon_mesh_drawables,
				boost::optional<const present_day_polygon_meshes_node_intersections_type &> polygon_mesh_node_intersections,
				source_raster_cube_subdivision_cache_type &source_raster_cube_subdivision_cache,
				boost::optional<age_grid_cube_subdivision_cache_type::non_null_ptr_type> &age_grid_cube_subdivision_cache,
				boost::optional<age_grid_cube_subdivision_cache_type::non_null_ptr_type> &normal_map_cube_subdivision_cache,
				clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
				const unsigned int source_raster_render_cube_quad_tree_depth,
				const unsigned int age_grid_render_cube_quad_tree_depth,
				const unsigned int normal_map_render_cube_quad_tree_depth,
				unsigned int &num_tiles_rendered_to_scene);

		void
		render_quad_tree(
				GLRenderer &renderer,
				boost::object_pool<GLUtils::QuadTreeUVTransform> &pool_quad_tree_uv_transforms,
				render_traversal_cube_quad_tree_type &render_traversal_cube_quad_tree,
				render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
				client_cache_cube_quad_tree_type &client_cache_cube_quad_tree,
				client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
				const source_raster_quad_tree_node_type &source_raster_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
				const boost::optional<age_grid_mask_quad_tree_node_type> &age_grid_mask_quad_tree_node,
				boost::optional<const GLUtils::QuadTreeUVTransform &> age_grid_uv_transform,
				const boost::optional<age_grid_mask_quad_tree_node_type> &normal_map_quad_tree_node,
				boost::optional<const GLUtils::QuadTreeUVTransform &> normal_map_uv_transform,
				boost::optional<const reconstructed_polygon_mesh_transform_group_type &> reconstructed_polygon_mesh_transform_group,
				boost::optional<const present_day_polygon_mesh_drawables_seq_type &> polygon_mesh_drawables,
				boost::optional<const present_day_polygon_meshes_node_intersections_type &> polygon_mesh_node_intersections,
				boost::optional<const present_day_polygon_meshes_intersection_partition_type::node_type &> intersections_quad_tree_node,
				const boost::optional<cube_mesh_quad_tree_node_type> &cube_mesh_quad_tree_node,
				source_raster_cube_subdivision_cache_type &source_raster_cube_subdivision_cache,
				const source_raster_cube_subdivision_cache_type::node_reference_type &source_raster_cube_subdivision_cache_node,
				boost::optional<age_grid_cube_subdivision_cache_type::non_null_ptr_type> &age_grid_cube_subdivision_cache,
				const boost::optional<age_grid_cube_subdivision_cache_type::node_reference_type> &age_grid_cube_subdivision_cache_root_node,
				boost::optional<age_grid_cube_subdivision_cache_type::non_null_ptr_type> &normal_map_cube_subdivision_cache,
				const boost::optional<age_grid_cube_subdivision_cache_type::node_reference_type> &normal_map_cube_subdivision_cache_node,
				clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
				const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
				unsigned int cube_quad_tree_depth,
				const unsigned int source_raster_render_cube_quad_tree_depth,
				const unsigned int age_grid_render_cube_quad_tree_depth,
				const unsigned int normal_map_render_cube_quad_tree_depth,
				const GLFrustum &frustum_planes,
				boost::uint32_t frustum_plane_mask,
				unsigned int &num_tiles_rendered_to_scene);

		void
		render_tile_to_scene(
				GLRenderer &renderer,
				render_traversal_cube_quad_tree_type::node_type &render_traversal_cube_quad_tree_node,
				client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
				const source_raster_quad_tree_node_type &source_raster_quad_tree_node,
				const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
				const boost::optional<age_grid_mask_quad_tree_node_type> &age_grid_mask_quad_tree_node,
				boost::optional<const GLUtils::QuadTreeUVTransform &> age_grid_uv_transform,
				const boost::optional<age_grid_mask_quad_tree_node_type> &normal_map_quad_tree_node,
				boost::optional<const GLUtils::QuadTreeUVTransform &> normal_map_uv_transform,
				boost::optional<const reconstructed_polygon_mesh_transform_group_type &> reconstructed_polygon_mesh_transform_group,
				boost::optional<const present_day_polygon_meshes_node_intersections_type &> polygon_mesh_node_intersections,
				boost::optional<const present_day_polygon_meshes_intersection_partition_type::node_type &> intersections_quad_tree_node,
				boost::optional<const present_day_polygon_mesh_drawables_seq_type &> polygon_mesh_drawables,
				const boost::optional<cube_mesh_quad_tree_node_type> &cube_mesh_quad_tree_node,
				source_raster_cube_subdivision_cache_type &source_raster_cube_subdivision_cache,
				const source_raster_cube_subdivision_cache_type::node_reference_type &source_raster_cube_subdivision_cache_node,
				boost::optional<age_grid_cube_subdivision_cache_type::non_null_ptr_type> &age_grid_cube_subdivision_cache,
				const boost::optional<age_grid_cube_subdivision_cache_type::node_reference_type> &age_grid_cube_subdivision_cache_node,
				boost::optional<age_grid_cube_subdivision_cache_type::non_null_ptr_type> &normal_map_cube_subdivision_cache,
				const boost::optional<age_grid_cube_subdivision_cache_type::node_reference_type> &normal_map_cube_subdivision_cache_node,
				clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
				const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node);

		bool
		get_tile_textures(
				GLRenderer &renderer,
				boost::optional<GLTexture::shared_ptr_to_const_type> &source_raster_texture,
				boost::optional<GLTexture::shared_ptr_to_const_type> &age_grid_mask_texture,
				boost::optional<GLTexture::shared_ptr_to_const_type> &normal_map_texture,
				client_cache_cube_quad_tree_type::node_type &client_cache_cube_quad_tree_node,
				const source_raster_quad_tree_node_type &source_raster_quad_tree_node,
				const boost::optional<age_grid_mask_quad_tree_node_type> &age_grid_mask_quad_tree_node,
				const boost::optional<age_grid_mask_quad_tree_node_type> &normal_map_quad_tree_node);

		boost::optional<GLProgramObject::shared_ptr_type>
		get_shader_program_for_tile(
				bool source_raster_is_floating_point,
				bool using_age_grid_tile,
				bool using_normal_map_tile,
				bool active_polygons);

		RenderQuadTreeNode::TileDrawState
		create_scene_tile_draw_state(
				GLRenderer &renderer,
				boost::optional<GLProgramObject::shared_ptr_type> render_tile_to_scene_program_object,
				const GLTexture::shared_ptr_to_const_type &source_raster_tile_texture,
				const GLUtils::QuadTreeUVTransform &source_raster_uv_transform,
				boost::optional<GLTexture::shared_ptr_to_const_type> age_grid_tile_texture,
				boost::optional<const GLUtils::QuadTreeUVTransform &> age_grid_uv_transform,
				boost::optional<GLTexture::shared_ptr_to_const_type> normal_map_tile_texture,
				boost::optional<const GLUtils::QuadTreeUVTransform &> normal_map_uv_transform,
				source_raster_cube_subdivision_cache_type &source_raster_cube_subdivision_cache,
				const source_raster_cube_subdivision_cache_type::node_reference_type &source_raster_cube_subdivision_cache_node,
				boost::optional<age_grid_cube_subdivision_cache_type::non_null_ptr_type> &age_grid_cube_subdivision_cache,
				const boost::optional<age_grid_cube_subdivision_cache_type::node_reference_type> &age_grid_cube_subdivision_cache_node,
				boost::optional<age_grid_cube_subdivision_cache_type::non_null_ptr_type> &normal_map_cube_subdivision_cache,
				const boost::optional<age_grid_cube_subdivision_cache_type::node_reference_type> &normal_map_cube_subdivision_cache_node,
				clip_cube_subdivision_cache_type &clip_cube_subdivision_cache,
				const clip_cube_subdivision_cache_type::node_reference_type &clip_cube_subdivision_cache_node,
				bool active_polygons);

		void
		render_tile_polygon_drawables(
				GLRenderer &renderer,
				const RenderQuadTreeNode::TileDrawState &render_scene_tile_draw_state,
				const GPlatesMaths::UnitQuaternion3D &reconstructed_polygon_mesh_rotation,
				const present_day_polygon_mesh_membership_type &reconstructed_polygon_mesh_membership,
				const present_day_polygon_meshes_node_intersections_type &polygon_mesh_node_intersections,
				const present_day_polygon_meshes_intersection_partition_type::node_type &intersections_quad_tree_node,
				const present_day_polygon_mesh_drawables_seq_type &polygon_mesh_drawables);

		void
		apply_tile_state(
				GLRenderer &renderer,
				const RenderQuadTreeNode::TileDrawState &tile_draw_state,
				const GPlatesMaths::UnitQuaternion3D &plate_rotation);

		void
		apply_tile_shader_program_state(
				GLRenderer &renderer,
				const RenderQuadTreeNode::TileDrawState &tile_draw_state);

		void
		create_shader_programs(
				GLRenderer &renderer);

		boost::optional<GLProgramObject::shared_ptr_type>
		create_shader_program(
				GLRenderer &renderer,
				const char *vertex_and_fragment_shader_defines = "");
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONSTATICPOLYGONRECONSTRUCTEDRASTER_H
