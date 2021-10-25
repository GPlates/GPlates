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

#ifndef GPLATES_OPENGL_GLRASTERCOREGISTRATION_H
#define GPLATES_OPENGL_GLRASTERCOREGISTRATION_H

#include <deque>
#include <vector>
#include <boost/integer/static_log2.hpp>
#include <boost/optional.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/shared_ptr.hpp>
#include <QString>

#include "GLCubeSubdivisionCache.h"
#include "GLFrameBufferObject.h"
#include "GLMultiResolutionRasterInterface.h"
#include "GLPixelBuffer.h"
#include "GLProgramObject.h"
#include "GLStreamPrimitives.h"
#include "GLTexture.h"
#include "GLTransform.h"
#include "GLUtils.h"
#include "GLVertex.h"
#include "GLVertexArray.h"
#include "GLVertexBuffer.h"
#include "GLVertexElementBuffer.h"

#include "app-logic/ReconstructContext.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/CubeQuadTreePartition.h"
#include "maths/CubeQuadTreePartitionUtils.h"
#include "maths/GreatCircleArc.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/UnitQuaternion3D.h"

#include "utils/IntrusiveSinglyLinkedList.h"
#include "utils/ReferenceCount.h"

// Used for visually debugging arbitrary render targets during raster co-registration by saving to image files.
//#define DEBUG_RASTER_COREGISTRATION_RENDER_TARGET


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * Co-registers the seed (geometry) features with a (possibly reconstructed) floating-point raster.
	 *
	 * Raster pixels within a specified distance from the seed geometry are collected and processed
	 * to generate a single scalar co-registration result per seed feature.
	 * An example of a processing operation is calculating the mean of those raster pixels.
	 */
	class GLRasterCoRegistration :
			public GPlatesUtils::ReferenceCount<GLRasterCoRegistration>
	{
	public:
		class Operation; // Forward declaration

	private:

		/**
		 * The power-of-two square texture dimension to use when creating floating-point textures
		 * to render the target raster to and to render the seed geometries into.
		 *
		 * This should not be too large since each floating-point texel consumes 16 bytes (4 floats - RGBA).
		 *
		 * Also graphics cards supporting floating-point textures typically support at least 2048
		 * dimension textures so probably shouldn't go above that - wouldn't want to anyway because
		 * the texture memory usage for the reduce stages would become larger than the video memory
		 * available for most of those cards.
		 */
		static const unsigned int TEXTURE_DIMENSION = 1024;

		/**
		 * The number of reduce stages depends on the texture dimension since each
		 * texture is reduced by a (dimension) factor of two (hence the dependence on log2).
		 */
		static const unsigned int NUM_REDUCE_STAGES = boost::static_log2<TEXTURE_DIMENSION>::value + 1;

		// Used to declare different lists types.
		struct ReduceStageListTag;
		struct PointListTag;
		struct MultiPointListTag;
		struct PolylineListTag;
		struct PolygonListTag;

		/**
		 * Associates a reconstructed geometry of a seed feature with the feature and an operation.
		 *
		 * Inherits from GPlatesUtils::IntrusiveSinglyLinkedList<...>::Node so that we can
		 * efficiently add to an intrusive list. Each inheritance is so that same @a SeedCoRegistration
		 * object can be in different lists at the same time.
		 */
		struct SeedCoRegistration :
				public GPlatesUtils::IntrusiveSinglyLinkedList<SeedCoRegistration, ReduceStageListTag>::Node,
				public GPlatesUtils::IntrusiveSinglyLinkedList<SeedCoRegistration, PointListTag>::Node,
				public GPlatesUtils::IntrusiveSinglyLinkedList<SeedCoRegistration, MultiPointListTag>::Node,
				public GPlatesUtils::IntrusiveSinglyLinkedList<SeedCoRegistration, PolylineListTag>::Node,
				public GPlatesUtils::IntrusiveSinglyLinkedList<SeedCoRegistration, PolygonListTag>::Node
		{
			/**
			 * If @a transform_ is null then @a geometry_ represents the reconstructed geometry
			 * otherwise it represents the present-day geometry (to be transformed by @a transform_).
			 */
			SeedCoRegistration(
					unsigned int operation_index_,
					unsigned int feature_index_,
					const GPlatesMaths::GeometryOnSphere &geometry_,
					const GPlatesMaths::UnitQuaternion3D &transform_) :
				operation_index(operation_index_),
				feature_index(feature_index_),
				geometry(geometry_),
				transform(transform_),
				// Default values (initialised properly later on if needed - only needed for 'loose' seed frustums)...
				raster_frustum_to_seed_frustum_post_projection_scale(1.0),
				raster_frustum_to_seed_frustum_post_projection_translate_x(0.0),
				raster_frustum_to_seed_frustum_post_projection_translate_y(0.0),
				seed_frustum_to_render_target_post_projection_scale(1.0),
				seed_frustum_to_render_target_post_projection_translate_x(0.0),
				seed_frustum_to_render_target_post_projection_translate_y(0.0)
			{  }

			unsigned int operation_index;
			unsigned int feature_index;
			const GPlatesMaths::GeometryOnSphere &geometry;
			const GPlatesMaths::UnitQuaternion3D &transform;

			//
			// These are initialised when traversing the spatial partition (ie, not by constructor).
			// They take the view frustum of a target raster tile and refine it to represent
			// a sub-section of that view frustum (enough to completely contain the seed geometry).
			//

			// Transforms clip-space of raster tile frustum to quad-tree node (of seed spatial partition)
			// containing this seed's geometry).
			// This takes the clip-space range [-1,1] covering a raster tile frustum and makes it
			// cover the seed frustum.
			double raster_frustum_to_seed_frustum_post_projection_scale;
			double raster_frustum_to_seed_frustum_post_projection_translate_x;
			double raster_frustum_to_seed_frustum_post_projection_translate_y;

			// Transforms clip-space of quad-tree node (of seed spatial partition) containing
			// this seed's geometry) to the sub-viewport of render target to render seed geometry into.
			// This takes the clip-space range [-1,1] covering the seed frustum and makes it
			// cover the render target frustum - effectively carving out of small sub-section of
			// the render target to render this seed geometry into.
			double seed_frustum_to_render_target_post_projection_scale;
			double seed_frustum_to_render_target_post_projection_translate_x;
			double seed_frustum_to_render_target_post_projection_translate_y;
		};

		//! Typedef for a list of seed co-registrations used for a reduce stage.
		typedef GPlatesUtils::IntrusiveSinglyLinkedList<SeedCoRegistration, ReduceStageListTag>
				seed_co_registration_reduce_stage_list_type;

		//! Typedef for a list of *point* seed co-registrations.
		typedef GPlatesUtils::IntrusiveSinglyLinkedList<SeedCoRegistration, PointListTag> seed_co_registration_points_list_type;
		//! Typedef for a list of *multipoint* seed co-registrations.
		typedef GPlatesUtils::IntrusiveSinglyLinkedList<SeedCoRegistration, MultiPointListTag> seed_co_registration_multi_points_list_type;
		//! Typedef for a list of *polyline* seed co-registrations.
		typedef GPlatesUtils::IntrusiveSinglyLinkedList<SeedCoRegistration, PolylineListTag> seed_co_registration_polylines_list_type;
		//! Typedef for a list of *polygon* seed co-registrations.
		typedef GPlatesUtils::IntrusiveSinglyLinkedList<SeedCoRegistration, PolygonListTag> seed_co_registration_polygons_list_type;

		//! Used when distributing SeedCoRegistration's among reduce stages.
		struct SeedCoRegistrationReduceStageLists
		{
			seed_co_registration_reduce_stage_list_type reduce_stage_lists[NUM_REDUCE_STAGES];
		};

		/**
		 * Each seed geometry can be rendered as points [and outlines [and fills]] depending on
		 * whether it's a point (or multipoint), polyline or polygon geometry.
		 */
		struct SeedCoRegistrationGeometryLists
		{
			//! Clear the geometry lists.
			void
			clear()
			{
				points_list.clear();
				multi_points_list.clear();
				polylines_list.clear();
				polygons_list.clear();
			}

			//! Returns true if all geometry lists are empty.
			bool
			empty() const
			{
				return points_list.empty() &&
					multi_points_list.empty() &&
					polylines_list.empty() &&
					polygons_list.empty();
			}

			//! List of *point* seed co-registrations.
			seed_co_registration_points_list_type points_list;
			//! List of *multipoint* seed co-registrations.
			seed_co_registration_multi_points_list_type multi_points_list;
			//! List of *polyline* seed co-registrations.
			seed_co_registration_polylines_list_type polylines_list;
			//! List of *polygon* seed co-registrations.
			seed_co_registration_polygons_list_type polygons_list;
		};

		// Forward declaration.
		class ResultsQueue;

	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLRasterCoRegistration.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLRasterCoRegistration> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLRasterCoRegistration.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLRasterCoRegistration> non_null_ptr_to_const_type;


		/**
		 * How the raster pixels in the region-of-interest of geometries are combined into a single value.
		 */
		enum OperationType
		{
			OPERATION_MEAN,
			OPERATION_STANDARD_DEVIATION,
			OPERATION_MINIMUM,
			OPERATION_MAXIMUM
		};


		/**
		 * Specifies the type of operation and region-of-interest and contains co-registration results.
		 */
		class Operation
		{
		public:
			/**
			 * Typedef for a sequence of co-registration results.
			 *
			 * There is one element per seed feature.
			 * Null elements indicate no co-registration results (eg, no raster in region of
			 * seed geometry or seed feature does not exist at the current reconstruction time).
			 */
			typedef std::vector< boost::optional<double> > result_seq_type;


			/**
			 * Define an operation as a type of operation, a region-of-interest and a fill polygon flag.
			 *
			 * The fill polygon flag @a fill_polygons determines if the entire interior region of
			 * a polygon seed geometry should be used to collect target raster pixels for processing.
			 * If this is 'false' then only target raster pixels within distance @a region_of_interest_radius
			 * from a polygon outline are collected (as is the case for polylines).
			 * Note that regardless of the value of this flag the area outside a polygon is always
			 * handled using the region-of-interest distance test.
			 */
			Operation(
					const double &region_of_interest_radius/* angular radial extent in radians */,
					OperationType operation,
					bool fill_polygons) :
				d_region_of_interest_radius(region_of_interest_radius),
				d_operation(operation),
				d_fill_polygons(fill_polygons)
			{  }

			/**
			 * Returns results of co-registration.
			 *
			 * The length of the returned sequence is the number of seed features.
			 *
			 * Null elements indicate no co-registration results (eg, no raster in region of
			 * seed geometry or seed feature does not exist at the current reconstruction time).
			 */
			const result_seq_type &
			get_co_registration_results() const
			{
				return d_results;
			}

		private:
			//
			// Operation configuration...
			//
			double d_region_of_interest_radius;
			OperationType d_operation;
			bool d_fill_polygons;

			/**
			 * The final co-registration results.
			 */
			result_seq_type d_results;

			friend class GLRasterCoRegistration;
		};


		/**
		 * Returns true if raster co-registration is supported on the runtime system.
		 *
		 * The most stringent requirement is OpenGL support for floating-point textures.
		 * If the runtime system supports 'GL_ARB_texture_float' then it will very likely
		 * support the other requirements such as shader programs and framebuffer objects.
		 */
		static
		bool
		is_supported(
				GLRenderer &renderer);


		/**
		 * Creates a @a GLRasterCoRegistration that co-registers the specified seed (geometry) features
		 * with the specified (possibly reconstructed) floating-point raster.
		 *
		 * @a raster_level_of_detail is the level-of-detail at which to process the target raster.
		 * For the highest resolution this is zero.
		 * This is used to increase performance and reduce memory usage on systems that need it.
		 *
		 * Returns boost::none if @a is_supported returns false.
		 */
		static
		boost::optional<non_null_ptr_type>
		create(
				GLRenderer &renderer)
		{
			if (!is_supported(renderer))
			{
				return boost::none;
			}

			return non_null_ptr_type(new GLRasterCoRegistration(renderer));
		}


		/**
		 * For each specified operation the specified (reconstructed) seed features and (possibly
		 * reconstructed) floating-point target raster are co-registered.
		 *
		 * The co-registration results are returned in @a operations.
		 *
		 * @a raster_level_of_detail is the level-of-detail at which to process the target raster.
		 * For the highest resolution this is zero.
		 * This is used to increase performance and reduce memory usage on systems that need it.
		 *
		 * NOTE: It is *much* more efficient to process any, and all, operations in one pass than to
		 * separate them in individual passes (per operation/region-of-interest).
		 *
		 * Returns boost::none if @a is_supported returns false.
		 */
		void
		co_register(
				GLRenderer &renderer,
				std::vector<Operation> &operations,
				const std::vector<GPlatesAppLogic::ReconstructContext::ReconstructedFeature> &reconstructed_seed_features,
				const GLMultiResolutionRasterInterface::non_null_ptr_type &reconstructed_target_raster,
				unsigned int raster_level_of_detail);

	private:
		/**
		 * The minimum viewport size to render seed geometries into.
		 *
		 * We don't need to go smaller than this in order to get good batching of seed geometries.
		 *
		 * NOTE: This should be a power-of-two.
		 */
		static const unsigned int MINIMUM_SEED_GEOMETRIES_VIEWPORT_DIMENSION = 16;

		/**
		 * The number of quad primitives (in the reduce vertex array) lined up along either
		 * horizontal or vertical side of texture.
		 *
		 * The total number of quads in the vertex array is the square of this.
		 */
		static const unsigned int NUM_REDUCE_VERTEX_ARRAY_QUADS_ACROSS_TEXTURE =
				TEXTURE_DIMENSION / MINIMUM_SEED_GEOMETRIES_VIEWPORT_DIMENSION;

		/**
		 * The number of bytes in the vertex buffer used to stream.
		 */
		static const unsigned int NUM_BYTES_IN_STREAMING_VERTEX_BUFFER = 2 * 1024 * 1024;

		/**
		 * The minimum number of bytes to stream in the vertex buffer.
		 */
		static const unsigned int MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER =
				NUM_BYTES_IN_STREAMING_VERTEX_BUFFER / 16;

		/**
		 * The number of bytes in the vertex element (indices) buffer used to stream.
		 */
		static const unsigned int NUM_BYTES_IN_STREAMING_VERTEX_ELEMENT_BUFFER =
				NUM_BYTES_IN_STREAMING_VERTEX_BUFFER / 8/*indices need a lot less space than vertices*/;

		/**
		 * The minimum number of bytes to stream in the vertex element buffer.
		 */
		static const unsigned int MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER =
				NUM_BYTES_IN_STREAMING_VERTEX_ELEMENT_BUFFER / 16;

		/**
		 * Typedef for a @a GLCubeSubvision cache.
		 */
		typedef GLCubeSubdivisionCache<
				true/*CacheProjectionTransform*/, true/*CacheLooseProjectionTransform*/,
				false/*CacheFrustum*/, false/*CacheLooseFrustum*/,
				false/*CacheBoundingPolygon*/, false/*CacheLooseBoundingPolygon*/,
				true/*CacheBounds*/, false/*CacheLooseBounds*/>
						cube_subdivision_cache_type;


		/**
		 * Typedef for a spatial partition of reconstructed seed co-registrations whose geometry
		 * bounding small circle have been expanded by a region-of-interest radius.
		 */
		typedef GPlatesMaths::CubeQuadTreePartition<SeedCoRegistration> seed_geometries_spatial_partition_type;

		//! Typedef for a structure that determines which nodes of a seed spatial partition intersect a regular cube quad tree.
		typedef GPlatesMaths::CubeQuadTreePartitionUtils::CubeQuadTreeIntersectingNodes<
				SeedCoRegistration, GPlatesMaths::CubeQuadTreePartition<SeedCoRegistration> /*non-const*/ >
						seed_geometries_intersecting_nodes_type;


		//! Typedef for vertex elements (indices) used in reduction vertex array.
		typedef GLuint reduction_vertex_element_type;

		//! Typedef for vertex elements (indices) used for streaming vertex array.
		typedef GLuint streaming_vertex_element_type;

		/**
		 * A vertex of the region-of-interest geometry around a point.
		 */
		struct PointRegionOfInterestVertex
		{
			//! Initialises *only* those data members that are constant across the seed geometry.
			void
			initialise_seed_geometry_constants(
					const SeedCoRegistration &seed_co_registration);

			GLfloat point_centre[3];
			GLfloat tangent_frame_weights[3];
			GLfloat world_space_quaternion[4]; // Constant across seed geometry.
			GLfloat raster_frustum_to_seed_frustum_clip_space_transform[3]; // Constant across seed geometry.
			GLfloat seed_frustum_to_render_target_clip_space_transform[3]; // Constant across seed geometry.
		};

		//! Typedef for a static stream of seed geometry point vertices.
		typedef GLStaticStreamPrimitives<PointRegionOfInterestVertex, streaming_vertex_element_type>
				point_region_of_interest_stream_primitives_type;

		/**
		 * A vertex of the region-of-interest geometry around a line (great circle arc).
		 */
		struct LineRegionOfInterestVertex
		{
			//! Initialises *only* those data members that are constant across the seed geometry.
			void
			initialise_seed_geometry_constants(
					const SeedCoRegistration &seed_co_registration);

			GLfloat line_arc_start_point[3];
			GLfloat line_arc_normal[3];
			GLfloat tangent_frame_weights[2];
			GLfloat world_space_quaternion[4]; // Constant across seed geometry.
			GLfloat raster_frustum_to_seed_frustum_clip_space_transform[3]; // Constant across seed geometry.
			GLfloat seed_frustum_to_render_target_clip_space_transform[3]; // Constant across seed geometry.
		};

		//! Typedef for a static stream of seed geometry line (GCA) vertices.
		typedef GLStaticStreamPrimitives<LineRegionOfInterestVertex, streaming_vertex_element_type>
				line_region_of_interest_stream_primitives_type;

		/**
		 * A vertex of the region-of-interest geometry of a fill (polygon interior).
		 */
		struct FillRegionOfInterestVertex
		{
			//! Initialises *only* those data members that are constant across the seed geometry.
			void
			initialise_seed_geometry_constants(
					const SeedCoRegistration &seed_co_registration);

			GLfloat fill_position[3];
			GLfloat world_space_quaternion[4]; // Constant across seed geometry.
			GLfloat raster_frustum_to_seed_frustum_clip_space_transform[3]; // Constant across seed geometry.
			GLfloat seed_frustum_to_render_target_clip_space_transform[3]; // Constant across seed geometry.
		};

		//! Typedef for a static stream of seed geometry fill (polygon) vertices.
		typedef GLStaticStreamPrimitives<FillRegionOfInterestVertex, streaming_vertex_element_type>
				fill_region_of_interest_stream_primitives_type;

		/**
		 * A vertex of a quad used to mask target raster with region-of-interest texture.
		 */
		struct MaskRegionOfInterestVertex
		{
			GLfloat screen_space_position[2];
			GLfloat raster_frustum_to_seed_frustum_clip_space_transform[3];
			GLfloat seed_frustum_to_render_target_clip_space_transform[3];
		};

		//! Typedef for a static stream of quads used to mask target raster with region-of-interest texture.
		typedef GLStaticStreamPrimitives<MaskRegionOfInterestVertex, streaming_vertex_element_type>
				mask_region_of_interest_stream_primitives_type;


		//! A linked list node that references a spatial partition node of reconstructed seed geometries.
		struct SeedGeometriesNodeListNode :
				public GPlatesUtils::IntrusiveSinglyLinkedList<SeedGeometriesNodeListNode>::Node
		{
			//! Default constructor.
			SeedGeometriesNodeListNode()
			{  }

			explicit
			SeedGeometriesNodeListNode(
					seed_geometries_spatial_partition_type::node_reference_type node_reference_) :
				node_reference(node_reference_)
			{  }

			seed_geometries_spatial_partition_type::node_reference_type node_reference;
		};

		//! Typedef for a list of spatial partition nodes referencing reconstructed seed geometries.
		typedef GPlatesUtils::IntrusiveSinglyLinkedList<SeedGeometriesNodeListNode> 
				seed_geometries_spatial_partition_node_list_type;


		/**
		 * Adds a @a SeedCoRegistration object to a list depending on associated its @a GeometryOnSphere type.
		 */
		class AddSeedCoRegistrationToGeometryLists :
				public GPlatesMaths::ConstGeometryOnSphereVisitor
		{
		public:
			AddSeedCoRegistrationToGeometryLists(
					SeedCoRegistrationGeometryLists &geometry_lists,
					SeedCoRegistration &seed_co_registration) :
				d_geometry_lists(geometry_lists),
				d_seed_co_registration(seed_co_registration)
			{  }

			virtual
			void
			visit_point_on_sphere(
					GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type /*point_on_sphere*/)
			{
				d_geometry_lists.points_list.push_front(&d_seed_co_registration);
			}

			virtual
			void
			visit_multi_point_on_sphere(
					GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type /*multi_point_on_sphere*/)
			{
				d_geometry_lists.multi_points_list.push_front(&d_seed_co_registration);
			}

			virtual
			void
			visit_polyline_on_sphere(
					GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type /*polyline_on_sphere*/)
			{
				d_geometry_lists.polylines_list.push_front(&d_seed_co_registration);
			}

			virtual
			void
			visit_polygon_on_sphere(
					GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type /*polygon_on_sphere*/)
			{
				d_geometry_lists.polygons_list.push_front(&d_seed_co_registration);
			}

		private:
			SeedCoRegistrationGeometryLists &d_geometry_lists;
			SeedCoRegistration &d_seed_co_registration;
		};


		/**
		 * A single 'GL_RGBA32F_ARB' pixel containing the result of an operation.
		 *
		 * The interpretation of the pixel result depends on the operation that generated it.
		 */
		struct ResultPixel
		{
			GLfloat red, green, blue, alpha;
		};

		/**
		 * Contains a single co-registration result.
		 *
		 * A seed feature can contain multiple co-registration results if either:
		 *   1) a seed feature has more than one geometry, and/or
		 *   2) a seed geometry needed to be split across multiple render targets with
		 *      each target containing a separate result.
		 *
		 * Ultimately all results for a seed *feature* must be combined into a single result
		 * (eg, multiple 'mean' results need to be weight-averaged together to get a single 'mean').
		 */
		struct SeedCoRegistrationPartialResult :
				public GPlatesUtils::IntrusiveSinglyLinkedList<SeedCoRegistrationPartialResult>::Node
		{
			explicit
			SeedCoRegistrationPartialResult(
					const ResultPixel &result_pixel_) :
				result_pixel(result_pixel_)
			{  }

			// The interpretation of the pixel result depends on the operation that generated it.
			ResultPixel result_pixel;
		};

		//! Typedef for a list of @a SeedCoRegistrationPartialResult objects.
		typedef GPlatesUtils::IntrusiveSinglyLinkedList<SeedCoRegistrationPartialResult>
				seed_co_registration_partial_result_list_type;

		/**
		 * Stores (potentially partial) seed co-registration results for seed features (for an operation).
		 */
		struct OperationSeedFeaturePartialResults
		{
			OperationSeedFeaturePartialResults() :
				partial_result_pool(new partial_result_pool_type())
			{  }

			//! Add a partial co-registration result for the specified seed feature index.
			void
			add_partial_result(
					const ResultPixel &result_pixel,
					unsigned int seed_feature_index)
			{
				partial_result_lists[seed_feature_index].push_front(
						partial_result_pool->construct(result_pixel));
			}

			/**
			 * One result list for each seed feature.
			 */
			std::vector<seed_co_registration_partial_result_list_type> partial_result_lists;

		private:
			typedef boost::object_pool<SeedCoRegistrationPartialResult> partial_result_pool_type;

			//! Allocate @a SeedCoRegistrationPartialResult objects using this object pool.
			boost::shared_ptr<partial_result_pool_type> partial_result_pool;
		};


		class ReduceQuadTree; // Forward declaration.

		/**
		 * Base class for a node in a quad tree used during the reduce stage to track a seed geometry
		 * co-registration as it gets reduced across reduce textures to eventually become a single scalar value.
		 */
		class ReduceQuadTreeNode
		{
		public:
			// For assertion checking (could have used dynamic_cast and virtual table instead).
			const bool is_leaf_node;

		protected:
			explicit
			ReduceQuadTreeNode(
					bool is_leaf_node_) :
				is_leaf_node(is_leaf_node_)
			{  }
		};

		/**
		 * A leaf reduce quad tree node.
		 */
		class ReduceQuadTreeLeafNode :
				public ReduceQuadTreeNode
		{
		public:
			explicit
			ReduceQuadTreeLeafNode(
					SeedCoRegistration &seed_co_registration_) :
				ReduceQuadTreeNode(true/*is_leaf_node*/),
				seed_co_registration(seed_co_registration_)
			{  }

			SeedCoRegistration &seed_co_registration;
		};

		/**
		 * An internal reduce quad tree node.
		 */
		class ReduceQuadTreeInternalNode :
				public ReduceQuadTreeNode
		{
		public:
			explicit
			ReduceQuadTreeInternalNode(
					unsigned int reduce_stage_index) :
				ReduceQuadTreeNode(false/*is_leaf_node*/),
				d_reduce_stage_index(reduce_stage_index),
				d_num_descendant_leaf_nodes(0)
			{
				d_children[0][0] = d_children[0][1] = d_children[1][0] = d_children[1][1] = NULL;
			}

			/**
			 * Returns the specified child *internal* node if it exists, otherwise NULL.
			 */
			const ReduceQuadTreeInternalNode *
			get_child_internal_node(
					unsigned int child_x_offset,
					unsigned int child_y_offset) const;

			/**
			 * Returns the specified child *internal* node if it exists, otherwise NULL.
			 */
			ReduceQuadTreeInternalNode *
			get_child_internal_node(
					unsigned int child_x_offset,
					unsigned int child_y_offset)
			{
				return const_cast<ReduceQuadTreeInternalNode *>(
						static_cast<const ReduceQuadTreeInternalNode *>(this)
								->get_child_internal_node(child_x_offset, child_y_offset));
			}

			/**
			 * Returns the specified child *leaf* node if it exists, otherwise NULL.
			 */
			const ReduceQuadTreeLeafNode *
			get_child_leaf_node(
					unsigned int child_x_offset,
					unsigned int child_y_offset) const;

			/**
			 * Returns the specified child *leaf* node if it exists, otherwise NULL.
			 */
			ReduceQuadTreeLeafNode *
			get_child_leaf_node(
					unsigned int child_x_offset,
					unsigned int child_y_offset)
			{
				return const_cast<ReduceQuadTreeLeafNode *>(
						static_cast<const ReduceQuadTreeInternalNode *>(this)
								->get_child_leaf_node(child_x_offset, child_y_offset));
			}

			/**
			 * Returns true if the sub-tree rooted at 'this' node contains the maximum number of leaf nodes.
			 */
			bool
			is_sub_tree_full() const
			{
				// d_num_descendant_leaf_nodes == (2 ^ d_reduce_stage_index) ^ 2
				return d_num_descendant_leaf_nodes == static_cast<unsigned>(1 << (d_reduce_stage_index << 1));
			}

			/**
			 * Updates the number of descendant leaf nodes of 'this' node.
			 */
			void
			accumulate_descendant_leaf_node_count(
					unsigned int num_new_leaf_nodes)
			{
				d_num_descendant_leaf_nodes += num_new_leaf_nodes;
			}

			/**
			 * Returns the number of descendant leaf nodes of 'this' node.
			 */
			unsigned int
			get_num_descendant_leaf_nodes() const
			{
				return d_num_descendant_leaf_nodes;
			}

			/**
			 * The reduce stage associated with the depth of 'this' quad tree node.
			 *
			 * Depth is [0, NUM_REDUCE_STAGES) whereas stage is (NUM_REDUCE_STAGES, 0]. 
			 */
			unsigned int
			get_reduce_stage_index() const
			{
				return d_reduce_stage_index;
			}

		private:
			/**
			 * Pointers to child nodes if they exist (child nodes that don't exits are NULL).
			 */
			ReduceQuadTreeNode *d_children[2][2];

			/**
			 * The reduce stage associated with the depth of 'this' quad tree node.
			 *
			 * Depth is [0, NUM_REDUCE_STAGES) whereas stage is (NUM_REDUCE_STAGES, 0]. 
			 */
			unsigned int d_reduce_stage_index;

			/**
			 * The number of leaf nodes the in the sub-tree rooted at 'this' node.
			 */
			unsigned int d_num_descendant_leaf_nodes;

			// So can modify @a d_children.
			friend class ReduceQuadTree;
		};

		/**
		 * A quad tree used during the reduce stage to track seed geometry co-registrations
		 * as they get reduced across reduce textures to eventually become single scalar values.
		 *
		 * The quad tree allows the final reduce stage texture (that contains the single scalar results
		 * - one per texel) to trace back to the @a SeedCoRegistration where the result should be stored.
		 *
		 * It also provides a way for a reduce stage to track how full it is and quickly find an empty
		 * slot to insert a seed geometry (for rendering to its own sub-viewport of reduce texture).
		 */
		class ReduceQuadTree :
			public GPlatesUtils::ReferenceCount<ReduceQuadTree>
		{
		public:
			//! A convenience typedef for a shared pointer to a non-const @a ReduceQuadTree.
			typedef GPlatesUtils::non_null_intrusive_ptr<ReduceQuadTree> non_null_ptr_type;

			//! A convenience typedef for a shared pointer to a const @a ReduceQuadTree.
			typedef GPlatesUtils::non_null_intrusive_ptr<const ReduceQuadTree> non_null_ptr_to_const_type;


			/**
			 * Creates a @a ReduceQuadTree object.
			 */
			static
			non_null_ptr_type
			create()
			{
				return non_null_ptr_type(new ReduceQuadTree());
			}

			/**
			 * Returns true if this quad tree has any leaf nodes.
			 */
			bool
			empty() const
			{
				return d_root_node.get_num_descendant_leaf_nodes() == 0;
			}

			/**
			 * Returns the root (internal) node of the quad tree.
			 */
			const ReduceQuadTreeInternalNode &
			get_root_node() const
			{
				return d_root_node;
			}

			/**
			 * Returns the root (internal) node of the quad tree.
			 */
			ReduceQuadTreeInternalNode &
			get_root_node()
			{
				return d_root_node;
			}

			/**
			 * Creates an internal node and sets it as a child of @a parent_node.
			 */
			ReduceQuadTreeInternalNode &
			create_child_internal_node(
					ReduceQuadTreeInternalNode &parent_node,
					unsigned int child_x_offset,
					unsigned int child_y_offset)
			{
				ReduceQuadTreeInternalNode *child_internal_node =
						d_reduce_quad_tree_internal_node_pool.construct(parent_node.get_reduce_stage_index() - 1);

				parent_node.d_children[child_y_offset][child_x_offset] = child_internal_node;

				return *child_internal_node;
			}

			/**
			 * Creates a leaf node and sets it as a child of @a parent_node.
			 */
			ReduceQuadTreeLeafNode &
			create_child_leaf_node(
					ReduceQuadTreeInternalNode &parent_node,
					unsigned int child_x_offset,
					unsigned int child_y_offset,
					SeedCoRegistration &seed_co_registration)
			{
				ReduceQuadTreeLeafNode *child_leaf_node =
						d_reduce_quad_tree_leaf_node_pool.construct(boost::ref(seed_co_registration));

				parent_node.d_children[child_y_offset][child_x_offset] = child_leaf_node;

				return *child_leaf_node;
			}

		private:
			//! Typedef for an object pool for internal nodes.
			typedef boost::object_pool<ReduceQuadTreeInternalNode> reduce_quad_tree_internal_node_pool_type;

			//! Typedef for an object pool for leaf nodes.
			typedef boost::object_pool<ReduceQuadTreeLeafNode> reduce_quad_tree_leaf_node_pool_type;

			//! All quad tree internal nodes are allocated in this object pool.
			reduce_quad_tree_internal_node_pool_type d_reduce_quad_tree_internal_node_pool;

			//! All quad tree leaf nodes are allocated in this object pool.
			reduce_quad_tree_leaf_node_pool_type d_reduce_quad_tree_leaf_node_pool;

			//! The root node - must be declared *after* @a d_reduce_quad_tree_internal_node_pool.
			ReduceQuadTreeInternalNode &d_root_node;

			ReduceQuadTree();
		};


		/**
		 * Manages queuing and asynchronous read back of result texture data from GPU to CPU.
		 */
		class ResultsQueue
		{
		public:
			explicit
			ResultsQueue(
					GLRenderer &renderer);

			/**
			 * Queue the results stored in @a results_texture for read back from GPU to CPU memory.
			 *
			 * This starts asynchronous read back of the texture to CPU memory via a pixel buffer.
			 *
			 * @a reduce_quad_tree determines which @a SeedCoRegistration each result pixel in
			 * @a results_texture should be written.
			 */
			void
			queue_reduce_pyramid_output(
					GLRenderer &renderer,
					const GLFrameBufferObject::shared_ptr_type &framebuffer_object,
					const GLTexture::shared_ptr_to_const_type &results_texture,
					const ReduceQuadTree::non_null_ptr_to_const_type &reduce_quad_tree,
					std::vector<OperationSeedFeaturePartialResults> &seed_feature_partial_results);

			/**
			 * Flushes any queued results.
			 *
			 * Any pixel buffers containing results are read and distributed to @a SeedCoRegistration objects.
			 *
			 * NOTE: Better efficiency is obtained if this is delayed as much as possible to avoid
			 * (or minimise) blocking to wait for the GPU to finish generating and transferring results.
			 */
			void
			flush_results(
					GLRenderer &renderer,
					std::vector<OperationSeedFeaturePartialResults> &seed_feature_partial_results);

		private:
			/**
			 * The number of pixel buffers to use in order to minimise stalls on the CPU while
			 * it waits for the GPU to finish generating results and sending them to the CPU.
			 *
			 * Limiting to two buffers is probably good - we don't want too many since each one buffer
			 * consumes 1024*1024*4*sizeof(float) = 16Mb.
			 *
			 * And two buffers allows us to process one buffer while the GPU is busy generating/transferring
			 * results for the other buffer.
			 *
			 * NOTE: Each pixel buffer supports asynchronous read back which makes this possible.
			 */
			static const unsigned int NUM_PIXEL_BUFFERS = 2;

			/**
			 * The number of bytes required to store a 4-channel 'GL_RGBA32F_ARB' format texture.
			 */
			static const unsigned int PIXEL_BUFFER_SIZE_IN_BYTES = TEXTURE_DIMENSION * TEXTURE_DIMENSION * 4 * sizeof(GLfloat);

			/**
			 * The minimum pixel rectangle dimension that we should read back from GPU to CPU.
			 *
			 * NOTE: This should be a power-of-two.
			 *
			 * Assuming a slow read back speed of 500Mb/s a 128x128 'GL_RGBA32F_ARB' texture should take
			 * "128 * 128 * 16 / 5e8 * 1000 = 0.1" milliseconds to transfer across the PCI bus.
			 */
			static const unsigned int MIN_DISTRIBUTE_READ_BACK_PIXEL_DIMENSION = 128;

			/**
			 * Associate pixel buffer results with a reduce quad tree.
			 */
			struct ReducePyramidOutput
			{
				ReducePyramidOutput(
						const ReduceQuadTree::non_null_ptr_to_const_type &reduce_quad_tree_,
						const GLPixelBuffer::shared_ptr_type &pixel_buffer_) :
					reduce_quad_tree(reduce_quad_tree_),
					pixel_buffer(pixel_buffer_)
				{  }

				/**
				 * Tracks the sub-viewport where each seed geometry is rendered and tracks final reduced
				 * results back to their seed co-registrations.
				 */
				ReduceQuadTree::non_null_ptr_to_const_type reduce_quad_tree;

				/**
				 * The reduce texture read back asynchronously from the GPU to CPU memory.
				 */
				GLPixelBuffer::shared_ptr_type pixel_buffer;
			};

			//! Typedef for a sequence of @a ReducePyramidOutput objects.
			typedef std::deque<ReducePyramidOutput> reduce_pyramid_output_queue_type;

			//! Typedef for a list of available pixel buffer objects.
			typedef std::vector<GLPixelBuffer::shared_ptr_type> pixel_buffer_seq_type;

			pixel_buffer_seq_type d_free_pixel_buffers;

			reduce_pyramid_output_queue_type d_results_queue;


			void
			flush_least_recently_queued_result(
					GLRenderer &renderer,
					std::vector<OperationSeedFeaturePartialResults> &seed_feature_partial_results);

			void
			distribute_async_read_back(
					GLRenderer &renderer,
					const ReduceQuadTree &reduce_quad_tree,
					GLPixelBuffer &pixel_buffer);

			void
			distribute_async_read_back(
					GLRenderer &renderer,
					const ReduceQuadTreeInternalNode &reduce_quad_tree_internal_node,
					GLPixelBuffer &pixel_buffer,
					GLint &pixel_buffer_offset,
					GLint pixel_rect_offset_x,
					GLint pixel_rect_offset_y,
					GLsizei pixel_rect_dimension);

			void
			distribute_result_data(
					GLRenderer &renderer,
					const void *result_data,
					const ReduceQuadTree &reduce_quad_tree,
					std::vector<OperationSeedFeaturePartialResults> &seed_feature_partial_results);

			void
			distribute_result_data(
					GLRenderer &renderer,
					const ReduceQuadTreeInternalNode &reduce_quad_tree_internal_node,
					const ResultPixel *const result_pixel_data,
					unsigned int &result_data_pixel_offset,
					GLsizei pixel_rect_dimension,
					std::vector<OperationSeedFeaturePartialResults> &seed_feature_partial_results);

			void
			distribute_result_data_from_gl_read_pixels_rect(
					GLRenderer &renderer,
					const ReduceQuadTreeInternalNode &reduce_quad_tree_internal_node,
					const ResultPixel *const gl_read_pixels_result_data,
					const GLsizei gl_read_pixels_rect_dimension,
					GLint pixel_offset_x,
					GLint pixel_offset_y,
					GLsizei pixel_rect_dimension,
					std::vector<OperationSeedFeaturePartialResults> &seed_feature_partial_results);
		};


		/**
		 * Parameters used when rendering seed co-registrations during reduce quad tree traversal.
		 *
		 * This simply avoids having to pass each parameter as function parameters during traversal.
		 */
		struct RenderSeedCoRegistrationParameters
		{
			RenderSeedCoRegistrationParameters(
					Operation &operation_,
					const GPlatesMaths::UnitVector3D &cube_face_centre_,
					const GLTexture::shared_ptr_type &target_raster_texture_,
					const GLTransform::non_null_ptr_to_const_type &target_raster_view_transform_,
					const GLTransform::non_null_ptr_to_const_type &target_raster_projection_transform_,
					ReduceQuadTree &reduce_quad_tree_,
					unsigned int node_x_offsets_relative_to_root_[],
					unsigned int node_y_offsets_relative_to_root_[],
					boost::optional<GLTexture::shared_ptr_type> reduce_stage_textures_[],
					unsigned int &reduce_stage_index_,
					SeedCoRegistrationReduceStageLists &operation_reduce_stage_list_,
					seed_co_registration_reduce_stage_list_type::iterator &seed_co_registration_iter_,
					seed_co_registration_reduce_stage_list_type::iterator &seed_co_registration_end_,
					SeedCoRegistrationGeometryLists seed_co_registration_geometry_lists_[],
					bool are_seed_geometries_bounded_) :
				operation(operation_),
				cube_face_centre(cube_face_centre_),
				target_raster_texture(target_raster_texture_),
				target_raster_view_transform(target_raster_view_transform_),
				target_raster_projection_transform(target_raster_projection_transform_),
				reduce_quad_tree(reduce_quad_tree_),
				node_x_offsets_relative_to_root(node_x_offsets_relative_to_root_),
				node_y_offsets_relative_to_root(node_y_offsets_relative_to_root_),
				reduce_stage_textures(reduce_stage_textures_),
				reduce_stage_index(reduce_stage_index_),
				operation_reduce_stage_list(operation_reduce_stage_list_),
				seed_co_registration_iter(seed_co_registration_iter_),
				seed_co_registration_end(seed_co_registration_end_),
				seed_co_registration_geometry_lists(seed_co_registration_geometry_lists_),
				are_seed_geometries_bounded(are_seed_geometries_bounded_)
			{  }

			Operation &operation;
			const GPlatesMaths::UnitVector3D &cube_face_centre;
			GLTexture::shared_ptr_type target_raster_texture;
			GLTransform::non_null_ptr_to_const_type target_raster_view_transform;
			GLTransform::non_null_ptr_to_const_type target_raster_projection_transform;
			ReduceQuadTree &reduce_quad_tree;
			unsigned int *const node_x_offsets_relative_to_root;
			unsigned int *const node_y_offsets_relative_to_root;
			boost::optional<GLTexture::shared_ptr_type> *const reduce_stage_textures;
			unsigned int &reduce_stage_index/*note it's a reference*/;
			SeedCoRegistrationReduceStageLists &operation_reduce_stage_list;
			seed_co_registration_reduce_stage_list_type::iterator &seed_co_registration_iter/*note it's a reference*/;
			seed_co_registration_reduce_stage_list_type::iterator &seed_co_registration_end/*note it's a reference*/;
			SeedCoRegistrationGeometryLists *const seed_co_registration_geometry_lists;
			bool are_seed_geometries_bounded;
		};


		/**
		 * Parameters used when co-registering a raster with reconstructed seed geometries.
		 *
		 * This data would normally be data members of this class (GLRasterCoRegistration) but
		 * instead are specific to a specific raster and *reconstructed* seed geometries. The data
		 * members of this class (GLRasterCoRegistration) are now those that are constant across all
		 * co-registrations runs (eg, shader program objects, streaming vertex buffers, etc).
		 *
		 * This simply avoids having to pass each parameter as function parameters during traversal.
		 */
		struct CoRegistrationParameters
		{
			CoRegistrationParameters(
					const std::vector<GPlatesAppLogic::ReconstructContext::ReconstructedFeature> &seed_features_,
					const GLMultiResolutionRasterInterface::non_null_ptr_type &target_raster_,
					unsigned int raster_level_of_detail_,
					unsigned int raster_texture_cube_quad_tree_depth_,
					unsigned int seed_geometries_spatial_partition_depth_,
					const seed_geometries_spatial_partition_type::non_null_ptr_type &seed_geometries_spatial_partition_,
					std::vector<Operation> &operations_,
					std::vector<OperationSeedFeaturePartialResults> &seed_feature_partial_results_,
					ResultsQueue &results_queue_) :
				d_seed_features(seed_features_),
				d_target_raster(target_raster_),
				d_raster_level_of_detail(raster_level_of_detail_),
				d_raster_texture_cube_quad_tree_depth(raster_texture_cube_quad_tree_depth_),
				d_seed_geometries_spatial_partition_depth(seed_geometries_spatial_partition_depth_),
				seed_geometries_spatial_partition(seed_geometries_spatial_partition_),
				operations(operations_),
				seed_feature_partial_results(seed_feature_partial_results_),
				d_results_queue(results_queue_)
			{  }

			/**
			 * The seed features - each feature could contain one or more geometries - all geometries
			 * of a feature are combined to give one co-registration scalar result per operation.
			 */
			const std::vector<GPlatesAppLogic::ReconstructContext::ReconstructedFeature> &d_seed_features;

			/**
			 * The raster that is the co-registration target data (co-registered onto the seed features).
			 */
			const GLMultiResolutionRasterInterface::non_null_ptr_type d_target_raster;

			/**
			 * The level-of-detail at which to process the target raster.
			 */
			const unsigned int d_raster_level_of_detail;

			/**
			 * The quad tree depth (in cube quad tree) to transition from rendering the raster as a
			 * regular quad tree partition to rendering the raster as a loose (overlapping) set of
			 * textures.
			 */
			const unsigned int d_raster_texture_cube_quad_tree_depth;

			/**
			 * The maximum depth of the quad tree(s) in the seed geometries spatial partition.
			 */
			const unsigned int d_seed_geometries_spatial_partition_depth;

			/**
			 * The seed geometries spatial partition.
			 */
			const seed_geometries_spatial_partition_type::non_null_ptr_type seed_geometries_spatial_partition;

			/**
			 * The client-specified co-registration operations.
			 */
			std::vector<Operation> &operations;

			/**
			 * Intermediate co-registration results - each seed feature can have multiple (partial)
			 * co-registration results that need to be combined into a single result for each seed feature
			 * before returning results to the caller.
			 *
			 * This vector is indexed by operation.
			 */
			std::vector<OperationSeedFeaturePartialResults> &seed_feature_partial_results;

			/**
			 * Queues asynchronous reading back of results from GPU to CPU memory.
			 */
			ResultsQueue &d_results_queue;
		};


		//
		// NOTE: The data members of this class (GLRasterCoRegistration) are constant across all
		// co-registrations runs (such as shader program objects, streaming vertex buffers, etc).
		// Anything specific to a particular raster or set of seed geometries should go into
		// class @a CoRegistrationParameters instead (or one of the other lower-level nested classes).
		// This enables an instance of @a GLRasterCoRegistration to be used for all co-registration runs.
		//

		/**
		 * Used to render to floating-point textures.
		 */
		GLFrameBufferObject::shared_ptr_type d_framebuffer_object;

		/**
		 * Used to stream indices (vertex elements) such as region-of-interest geometries.
		 */
		GLVertexElementBuffer::shared_ptr_type d_streaming_vertex_element_buffer;

		/**
		 * Used to stream vertices such as region-of-interest geometries.
		 */
		GLVertexBuffer::shared_ptr_type d_streaming_vertex_buffer;

		/**
		 * Used to contain *point* region-of-interest geometries.
		 */
		GLVertexArray::shared_ptr_type d_point_region_of_interest_vertex_array;

		/**
		 * Used to contain *line* (great circle arc) region-of-interest geometries.
		 */
		GLVertexArray::shared_ptr_type d_line_region_of_interest_vertex_array;

		/**
		 * Used to contain *fill* (polygon-interior) region-of-interest geometries.
		 */
		GLVertexArray::shared_ptr_type d_fill_region_of_interest_vertex_array;

		/**
		 * Contains quads used to mask target raster with region-of-interest texture.
		 */
		GLVertexArray::shared_ptr_type d_mask_region_of_interest_vertex_array;

		/**
		 * Used to reduce (by 2x2 -> 1x1) region-of-interest filter results.
		 */
		GLVertexArray::shared_ptr_type d_reduction_vertex_array;

		/**
		 * Shader program to render point regions-of-interest for seed geometries with small region-of-interest angles.
		 */
		GLProgramObject::shared_ptr_type d_render_points_of_seed_geometries_with_small_roi_angle_program_object;

		/**
		 * Shader program to render point regions-of-interest for seed geometries with large region-of-interest angles.
		 */
		GLProgramObject::shared_ptr_type d_render_points_of_seed_geometries_with_large_roi_angle_program_object;

		/**
		 * Shader program to render line (great circle arc) regions-of-interest for seed geometries
		 * with small region-of-interest angles.
		 */
		GLProgramObject::shared_ptr_type d_render_lines_of_seed_geometries_with_small_roi_angle_program_object;

		/**
		 * Shader program to render line (great circle arc) regions-of-interest for seed geometries
		 * with large region-of-interest angles.
		 */
		GLProgramObject::shared_ptr_type d_render_lines_of_seed_geometries_with_large_roi_angle_program_object;

		/**
		 * Shader program to render fill (polygon-interior) regions-of-interest.
		 */
		GLProgramObject::shared_ptr_type d_render_fill_of_seed_geometries_program_object;

		/**
		 * Shader program to copy target raster into seed sub-viewport with region-of-interest masking.
		 *
		 * This versions sets up for reduction of moments (mean or standard deviation).
		 */
		GLProgramObject::shared_ptr_type d_mask_region_of_interest_moments_program_object;

		/**
		 * Shader program to copy target raster into seed sub-viewport with region-of-interest masking.
		 *
		 * This versions sets up for reduction of min/max.
		 */
		GLProgramObject::shared_ptr_type d_mask_region_of_interest_minmax_program_object;

		/**
		 * Shader program to reduce by calculating *sum* of regions-of-interest filter results.
		 *
		 * This is used by the mean and standard deviation operations.
		 */
		GLProgramObject::shared_ptr_type d_reduction_sum_program_object;

		/**
		 * Shader program to reduce by calculating *minimum* of regions-of-interest filter results.
		 */
		GLProgramObject::shared_ptr_type d_reduction_min_program_object;

		/**
		 * Shader program to reduce by calculating *maximum* of regions-of-interest filter results.
		 */
		GLProgramObject::shared_ptr_type d_reduction_max_program_object;

		/**
		 * Simplifies some code since seed geometry can reference identity quaternion if has no finite rotation.
		 */
		GPlatesMaths::UnitQuaternion3D d_identity_quaternion;

		/**
		 * Used to retrieve render target data and save to an image file for debugging purposes.
		 */
#if defined(DEBUG_RASTER_COREGISTRATION_RENDER_TARGET)
		GLPixelBuffer::shared_ptr_type d_debug_pixel_buffer;
#endif


		explicit
		GLRasterCoRegistration(
				GLRenderer &renderer);

		void
		initialise_vertex_arrays_and_shader_programs(
				GLRenderer &renderer);

		void
		initialise_point_region_of_interest_shader_programs(
				GLRenderer &renderer);

		void
		initialise_line_region_of_interest_shader_program(
				GLRenderer &renderer);

		void
		initialise_fill_region_of_interest_shader_program(
				GLRenderer &renderer);

		void
		initialise_mask_region_of_interest_shader_program(
				GLRenderer &renderer);

		GLProgramObject::shared_ptr_type
		create_region_of_interest_shader_program(
				GLRenderer &renderer,
				const char *vertex_shader_defines,
				const char *fragment_shader_defines);

		void
		initialise_reduction_of_region_of_interest_shader_programs(
				GLRenderer &renderer);

		void
		initialise_reduction_of_region_of_interest_vertex_array(
				GLRenderer &renderer);

		void
		initialise_reduction_vertex_array_in_quad_tree_traversal_order(
				std::vector<GLTextureVertex> &vertices,
				std::vector<reduction_vertex_element_type> &vertex_elements,
				unsigned int x_quad_offset,
				unsigned int y_quad_offset,
				unsigned int width_in_quads);

		void
		initialise_texture_level_of_detail_parameters(
				GLRenderer &renderer,
				const GLMultiResolutionRasterInterface::non_null_ptr_type &target_raster,
				const unsigned int raster_level_of_detail,
				unsigned int &raster_texture_cube_quad_tree_depth,
				unsigned int &seed_geometries_spatial_partition_depth);

		seed_geometries_spatial_partition_type::non_null_ptr_type
		create_reconstructed_seed_geometries_spatial_partition(
				std::vector<Operation> &operations,
				const std::vector<GPlatesAppLogic::ReconstructContext::ReconstructedFeature> &seed_features,
				const unsigned int seed_geometries_spatial_partition_depth);

		void
		filter_reduce_seed_geometries_spatial_partition(
				GLRenderer &renderer,
				const CoRegistrationParameters &co_registration_parameters);

		void
		filter_reduce_seed_geometries(
				GLRenderer &renderer,
				const CoRegistrationParameters &co_registration_parameters,
				seed_geometries_spatial_partition_type::node_reference_type seed_geometries_spatial_partition_node,
				const seed_geometries_spatial_partition_node_list_type &parent_seed_geometries_intersecting_node_list,
				const seed_geometries_intersecting_nodes_type &seed_geometries_intersecting_nodes,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
				unsigned int level_of_detail);

		void
		co_register_seed_geometries(
				GLRenderer &renderer,
				const CoRegistrationParameters &co_registration_parameters,
				seed_geometries_spatial_partition_type::node_reference_type seed_geometries_spatial_partition_node,
				const seed_geometries_spatial_partition_node_list_type &parent_seed_geometries_intersecting_node_list,
				const seed_geometries_intersecting_nodes_type &seed_geometries_intersecting_nodes,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node);

		void
		co_register_seed_geometries_with_target_raster(
				GLRenderer &renderer,
				const CoRegistrationParameters &co_registration_parameters,
				const seed_geometries_spatial_partition_node_list_type &parent_seed_geometries_intersecting_node_list,
				const seed_geometries_intersecting_nodes_type &seed_geometries_intersecting_nodes,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node);

		void
		co_register_seed_geometries_with_target_raster(
				GLRenderer &renderer,
				const CoRegistrationParameters &co_registration_parameters,
				const seed_geometries_spatial_partition_node_list_type &seed_geometries_intersecting_node_list,
				const GPlatesMaths::UnitVector3D &cube_face_centre,
				const GLTransform::non_null_ptr_to_const_type &view_transform,
				const GLTransform::non_null_ptr_to_const_type &projection_transform);

		void
		group_seed_co_registrations_by_operation_to_reduce_stage_zero(
				std::vector<SeedCoRegistrationReduceStageLists> &operations_reduce_stage_lists,
				seed_geometries_spatial_partition_type &seed_geometries_spatial_partition,
				const seed_geometries_spatial_partition_node_list_type &seed_geometries_intersecting_node_list);

		void
		co_register_seed_geometries_with_loose_target_raster(
				GLRenderer &renderer,
				const CoRegistrationParameters &co_registration_parameters,
				seed_geometries_spatial_partition_type::node_reference_type seed_geometries_spatial_partition_node,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node);

		void
		group_seed_co_registrations_by_operation(
				const CoRegistrationParameters &co_registration_parameters,
				std::vector<SeedCoRegistrationReduceStageLists> &operations_reduce_stage_lists,
				seed_geometries_spatial_partition_type::node_reference_type seed_geometries_spatial_partition_node,
				const GLUtils::QuadTreeClipSpaceTransform &raster_frustum_to_loose_seed_frustum_clip_space_transform,
				unsigned int reduce_stage_index);

		void
		render_seed_geometries_to_reduce_pyramids(
				GLRenderer &renderer,
				const CoRegistrationParameters &co_registration_parameters,
				unsigned int operation_index,
				const GPlatesMaths::UnitVector3D &cube_face_centre,
				const GLTexture::shared_ptr_type &target_raster_texture,
				const GLTransform::non_null_ptr_to_const_type &target_raster_view_transform,
				const GLTransform::non_null_ptr_to_const_type &target_raster_projection_transform,
				std::vector<SeedCoRegistrationReduceStageLists> &operation_reduce_stage_lists,
				bool are_seed_geometries_bounded);

		unsigned int
		render_seed_geometries_to_reduce_quad_tree_internal_node(
				GLRenderer &renderer,
				RenderSeedCoRegistrationParameters &render_params,
				ReduceQuadTreeInternalNode &reduce_quad_tree_internal_node);

		void
		render_seed_geometries_in_reduce_stage_render_list(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &reduce_stage_texture,
				bool clear_reduce_stage_texture,
				const Operation &operation,
				const GPlatesMaths::UnitVector3D &cube_face_centre,
				const GLTexture::shared_ptr_type &target_raster_texture,
				const GLTransform::non_null_ptr_to_const_type &target_raster_view_transform,
				const GLTransform::non_null_ptr_to_const_type &target_raster_projection_transform,
				const SeedCoRegistrationGeometryLists &seed_co_registration_geometry_lists,
				bool are_seed_geometries_bounded);

		void
		render_bounded_point_region_of_interest_geometries(
				GLRenderer &renderer,
				GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
				GLBuffer::MapBufferScope &map_vertex_buffer_scope,
				const SeedCoRegistrationGeometryLists &geometry_lists,
				const double &region_of_interest_radius);

		void
		render_bounded_point_region_of_interest_geometry(
				GLRenderer &renderer,
				GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
				GLBuffer::MapBufferScope &map_vertex_buffer_scope,
				point_region_of_interest_stream_primitives_type::StreamTarget &point_stream_target,
				point_region_of_interest_stream_primitives_type::Primitives &point_stream_quads,
				const GPlatesMaths::UnitVector3D &point,
				PointRegionOfInterestVertex &vertex,
				const double &tan_region_of_interest_angle);

		void
		render_unbounded_point_region_of_interest_geometries(
				GLRenderer &renderer,
				GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
				GLBuffer::MapBufferScope &map_vertex_buffer_scope,
				const SeedCoRegistrationGeometryLists &geometry_lists,
				const double &region_of_interest_radius);

		void
		render_unbounded_point_region_of_interest_geometry(
				GLRenderer &renderer,
				GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
				GLBuffer::MapBufferScope &map_vertex_buffer_scope,
				point_region_of_interest_stream_primitives_type::StreamTarget &point_stream_target,
				point_region_of_interest_stream_primitives_type::Primitives &point_stream_meshes,
				const GPlatesMaths::UnitVector3D &point,
				PointRegionOfInterestVertex &vertex,
				const double &centre_point_weight,
				const double &tangent_weight);

		void
		render_bounded_line_region_of_interest_geometries(
				GLRenderer &renderer,
				GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
				GLBuffer::MapBufferScope &map_vertex_buffer_scope,
				const SeedCoRegistrationGeometryLists &geometry_lists,
				const double &region_of_interest_radius);

		void
		render_bounded_line_region_of_interest_geometry(
				GLRenderer &renderer,
				GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
				GLBuffer::MapBufferScope &map_vertex_buffer_scope,
				line_region_of_interest_stream_primitives_type::StreamTarget &line_stream_target,
				line_region_of_interest_stream_primitives_type::Primitives &line_stream_quads,
				const GPlatesMaths::GreatCircleArc &line,
				LineRegionOfInterestVertex &vertex,
				const double &tan_region_of_interest_angle);

		void
		render_unbounded_line_region_of_interest_geometries(
				GLRenderer &renderer,
				GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
				GLBuffer::MapBufferScope &map_vertex_buffer_scope,
				const SeedCoRegistrationGeometryLists &geometry_lists,
				const double &region_of_interest_radius);

		void
		render_unbounded_line_region_of_interest_geometry(
				GLRenderer &renderer,
				GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
				GLBuffer::MapBufferScope &map_vertex_buffer_scope,
				line_region_of_interest_stream_primitives_type::StreamTarget &line_stream_target,
				line_region_of_interest_stream_primitives_type::Primitives &line_stream_meshes,
				const GPlatesMaths::GreatCircleArc &line,
				LineRegionOfInterestVertex &vertex,
				const double &arc_point_weight,
				const double &tangent_weight);

		void
		render_single_pixel_size_point_region_of_interest_geometries(
				GLRenderer &renderer,
				GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
				GLBuffer::MapBufferScope &map_vertex_buffer_scope,
				const SeedCoRegistrationGeometryLists &geometry_lists);

		void
		render_single_pixel_wide_line_region_of_interest_geometries(
				GLRenderer &renderer,
				GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
				GLBuffer::MapBufferScope &map_vertex_buffer_scope,
				const SeedCoRegistrationGeometryLists &geometry_lists);

		void
		render_fill_region_of_interest_geometries(
				GLRenderer &renderer,
				GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
				GLBuffer::MapBufferScope &map_vertex_buffer_scope,
				const SeedCoRegistrationGeometryLists &geometry_lists);

		void
		mask_target_raster_with_regions_of_interest(
				GLRenderer &renderer,
				const Operation &operation,
				const GPlatesMaths::UnitVector3D &cube_face_centre,
				const GLTexture::shared_ptr_type &target_raster_texture,
				const GLTexture::shared_ptr_type &region_of_interest_mask_texture,
				GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
				GLBuffer::MapBufferScope &map_vertex_buffer_scope,
				const SeedCoRegistrationGeometryLists &geometry_lists);

		void
		mask_target_raster_with_region_of_interest(
				GLRenderer &renderer,
				GLBuffer::MapBufferScope &map_vertex_element_buffer_scope,
				GLBuffer::MapBufferScope &map_vertex_buffer_scope,
				mask_region_of_interest_stream_primitives_type::StreamTarget &mask_stream_target,
				mask_region_of_interest_stream_primitives_type::Primitives &mask_stream_quads,
				const SeedCoRegistration &seed_co_registration);

		void
		render_reduction_of_reduce_stage(
				GLRenderer &renderer,
				const Operation &operation,
				const ReduceQuadTreeInternalNode &dst_reduce_quad_tree_node,
				unsigned int src_child_x_offset,
				unsigned int src_child_y_offset,
				bool clear_dst_reduce_stage_texture,
				const GLTexture::shared_ptr_type &dst_reduce_stage_texture,
				const GLTexture::shared_ptr_type &src_reduce_stage_texture);

		unsigned int
		find_number_reduce_vertex_array_quads_spanned_by_child_reduce_quad_tree_node(
				const ReduceQuadTreeInternalNode &parent_reduce_quad_tree_node,
				unsigned int child_x_offset,
				unsigned int child_y_offset,
				unsigned int child_quad_tree_node_width_in_quads);

		/**
		 * Renders target raster into render texture and returns true if there was any rendering
		 * into the view frustum (determined by @a view_transform and @a projection_transform).
		 */
		bool
		render_target_raster(
				GLRenderer &renderer,
				const CoRegistrationParameters &co_registration_parameters,
				const GLTexture::shared_ptr_type &target_raster_texture,
				const GLTransform &view_transform,
				const GLTransform &projection_transform);

		static
		GLTexture::shared_ptr_type
		acquire_rgba_float_texture(
				GLRenderer &renderer);

		static
		GLTexture::shared_ptr_type
		acquire_rgba_fixed_texture(
				GLRenderer &renderer);

		void
		return_co_registration_results_to_caller(
				const CoRegistrationParameters &co_registration_parameters);

#if defined(DEBUG_RASTER_COREGISTRATION_RENDER_TARGET)
		void
		debug_fixed_point_render_target(
				GLRenderer &renderer,
				const QString &image_file_basename);

		void
		debug_floating_point_render_target(
				GLRenderer &renderer,
				const QString &image_file_basename,
				bool coverage_is_in_green_channel);
#endif
	};
}

#endif // GPLATES_OPENGL_GLRASTERCOREGISTRATION_H
