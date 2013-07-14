/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_GLSCALARFIELD3D_H
#define GPLATES_OPENGL_GLSCALARFIELD3D_H

#include <cstddef> // For std::size_t
#include <utility>
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <opengl/OpenGL.h>
#include <QString>

#include "GLBuffer.h"
#include "GLCompiledDrawState.h"
#include "GLCubeSubdivision.h"
#include "GLFrameBufferObject.h"
#include "GLLight.h"
#include "GLProgramObject.h"
#include "GLScreenRenderTarget.h"
#include "GLShaderObject.h"
#include "GLShaderProgramUtils.h"
#include "GLShaderSource.h"
#include "GLStreamPrimitives.h"
#include "GLTexture.h"
#include "GLVertex.h"
#include "GLVertexArray.h"
#include "GLVertexBuffer.h"
#include "GLVertexElementBuffer.h"
#include "GLViewport.h"

#include "file-io/ScalarField3DFileFormatReader.h"

#include "gui/Colour.h"
#include "gui/ColourPalette.h"

#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/GeometryOnSphere.h"
#include "maths/HierarchicalTriangularMesh.h"

#include "utils/ReferenceCount.h"
#include "utils/SubjectObserverToken.h"

#include "view-operations/ScalarField3DRenderParameters.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * A 3D sub-surface scalar field represented as a cube map of concentric depth layers.
	 */
	class GLScalarField3D :
			public GPlatesUtils::ReferenceCount<GLScalarField3D>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLScalarField3D.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLScalarField3D> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLScalarField3D.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLScalarField3D> non_null_ptr_to_const_type;

		/**
		 * Typedef for an opaque object that caches a particular render of this scalar field.
		 */
		typedef boost::shared_ptr<void> cache_handle_type;

		/**
		 * Typedef for a sequence of cross section geometries (points, multipoints, polylines, polygons).
		 */
		typedef std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> cross_sections_seq_type;

		/**
		 * Typedef for a sequence of surface polygons mask geometries (polylines, polygons).
		 */
		typedef std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> surface_polygons_mask_seq_type;


		/**
		 * Defines surface geometries used as fill masks to limit rendering of isosurface (or cross sections)
		 * to certain volume regions defined by extruding the surface mask towards the globe centre.
		 */
		struct SurfaceFillMask
		{
			/**
			 * Determines if show vertical walls when rendering iso-surface.
			 * Does not apply to cross section rendering.
			 */
			struct ShowWalls
			{
				explicit
				ShowWalls(
						bool only_boundary_walls_ = true) :
					only_boundary_walls(only_boundary_walls_)
				{  }

				bool only_boundary_walls;
			};

			/**
			 * @a show_walls_ does not apply to cross section rendering.
			 */
			SurfaceFillMask(
					const surface_polygons_mask_seq_type &surface_polygons_mask_,
					bool treat_polylines_as_polygons_,
					boost::optional<ShowWalls> show_walls_ = boost::none);

			surface_polygons_mask_seq_type surface_polygons_mask;
			bool treat_polylines_as_polygons;
			boost::optional<ShowWalls> show_walls;
		};


		/**
		 * Returns true if rendering (ray-tracing) of 3D scalar fields are supported on the runtime system.
		 *
		 * This essentially requires graphics hardware supporting OpenGL 3.0.
		 */
		static
		bool
		is_supported(
				GLRenderer &renderer);


		/**
		 * Creates a @a GLScalarField3D object.
		 *
		 * @a scalar_field_filename is the cube map file containing the source of scalar field data.
		 *
		 * @a light determines the light direction and other parameters to use when rendering the scalar field.
		 */
		static
		non_null_ptr_type
		create(
				GLRenderer &renderer,
				const QString &scalar_field_filename,
				const GLLight::non_null_ptr_type &light);

		/**
		 * Set the colour palette.
		 *
		 * The colour palette used to map scalar values (or gradient magnitudes) to colour.
		 * @a colour_palette_value_range specifies the [min, max] range of the values used in the palette.
		 */
		void
		set_colour_palette(
				GLRenderer &renderer,
				const GPlatesGui::ColourPalette<double>::non_null_ptr_to_const_type &colour_palette,
				const std::pair<double, double> &colour_palette_value_range);


		/**
		 * Change to a new scalar field of the same dimensions as the current internal scalar field.
		 *
		 * This method is useful for time-dependent scalar fields with the same dimensions.
		 *
		 * Returns false if the scalar field to be loaded from @a scalar_field_filename has different
		 * dimensions than the current internal scalar field.
		 * In this case you'll need to create a new @a GLScalarField.
		 */
		bool
		change_scalar_field(
				GLRenderer &renderer,
				const QString &scalar_field_filename);


		/**
		 * Returns a subject token that clients can observe to see if they need to update themselves
		 * (such as any cached data we render for them) by getting us to re-render.
		 */
		const GPlatesUtils::SubjectToken &
		get_subject_token() const;


		/**
		 * Renders the scalar field as an iso-surface visible in the view frustum
		 * (determined by the current model-view/projection transforms of @a renderer).
		 *
		 * @a render_mode must an isosurface mode (ie, not 'RENDER_MODE_CROSS_SECTIONS').
		 *
		 * @a surface_fill_mask contains the optional surface polygon geometries that limit rendering
		 * of the isosurface to regions underneath the filled surface geometries.
		 * The surface geometries can optionally include polylines which are filled as if they were polygons.
		 * The vertical walls of the fill mask can optionally be rendered as can the spherical surface mask itself.
		 *
		 * @a test_variables are test variables to optionally use during shader program development.
		 * These variables are in the range [0,1] and come from the scalar field visual layer GUI.
		 * They are passed directly to uniform shader variables in the shader program(s).
		 * @a test_variables can be any size but typically only the first [0,N-1] variables are used
		 * where N is on the order of ten. Extra variables, that don't exist in the shader, will emit
		 * a warning (but that can be ignored during testing).
		 *
		 * @a surface_occlusion_texture is a viewport-size 2D texture containing the RGBA rendering
		 * of the surface geometries/rasters on the *front* of the globe.
		 * If specified it will be used to early terminate ray-tracing for those pixels that are
		 * occluded by surface geometries/rasters (that have alpha of 1.0).
		 *
		 * @a depth_read_texture is a viewport-size 2D texture containing the screen-space depth
		 * (in range [-1,1] as opposed to window coordinate range [0,1]) of sub-surface structures
		 * already rendered.
		 * If specified it will be used to early terminate ray-tracing for those pixels that are
		 * depth-occluded by sub-surface structures.
		 * NOTE: This depth might not contain all sub-surface structures rendered so far so it
		 * shouldn't be used as a replacement for hardware depth testing (ie, hardware depth testing
		 * and depth writes should still both be enabled).
		 *
		 * The screen-space depth (in range [-1,1] as opposed to window coordinate range [0,1]) is
		 * always output by the fragment shader, during rendering, to the draw buffer at index 1
		 * (the scalar field colour is output to draw buffer index 0).
		 * By default this is ignored since default state of glDrawBuffers is NONE for indices >= 1.
		 * But if you attach a viewport-size floating-point texture to a GLFrameBufferObject,
		 * and call GLFrameBufferObject::gl_draw_buffers(), then depth will get written to the
		 * texture for those pixels on the rendered iso-surface (or cross-section).
		 */
		void
		render_iso_surface(
				GLRenderer &renderer,
				cache_handle_type &cache_handle,
				GPlatesViewOperations::ScalarField3DRenderParameters::IsosurfaceDeviationWindowMode deviation_window_mode,
				GPlatesViewOperations::ScalarField3DRenderParameters::IsosurfaceColourMode colour_mode,
				const GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters &isovalue_parameters,
				const GPlatesViewOperations::ScalarField3DRenderParameters::DeviationWindowRenderOptions &deviation_window_render_options,
				const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction,
				const GPlatesViewOperations::ScalarField3DRenderParameters::QualityPerformance &quality_performance,
				const std::vector<float> &test_variables,
				boost::optional<SurfaceFillMask> surface_fill_mask = boost::none,
				boost::optional<GLTexture::shared_ptr_to_const_type> surface_occlusion_texture = boost::none,
				boost::optional<GLTexture::shared_ptr_to_const_type> depth_read_texture = boost::none);


		/**
		 * Renders the scalar field as cross-section(s) visible in the view frustum
		 * (determined by the current model-view/projection transforms of @a renderer).
		 *
		 * The cross-section geometry is formed by vertically extruded the specified cross section
		 * geometry from the maximum scalar field depth to the minimum depth.
		 * The cross section geometry can be points, polylines or polygons.
		 *
		 * @a surface_fill_mask contains the optional surface polygon geometries that limit rendering
		 * of the cross sections to regions underneath the filled surface geometries.
		 * The surface geometries can optionally include polylines which are filled as if they were polygons.
		 *
		 * The surface polygon mask @a surface_polygon_mask is the same as specified in @a render_iso_surface.
		 * Here it is used to mask away parts of cross-sections that lie outside the mask region.
		 *
		 * @a test_variables are test variables to optionally use during shader program development.
		 * These variables are in the range [0,1] and come from the scalar field visual layer GUI.
		 * They are passed directly to uniform shader variables in the shader program(s).
		 * @a test_variables can be any size but typically only the first [0,N-1] variables are used
		 * where N is on the order of ten. Extra variables, that don't exist in the shader, will emit
		 * a warning (but that can be ignored during testing).
		 *
		 * @a surface_occlusion_texture is a viewport-size 2D texture containing the RGBA rendering
		 * of the surface geometries/rasters on the *front* of the globe.
		 * If specified it will be used to cull those pixels that are occluded by surface
		 * geometries/rasters (that have alpha of 1.0) - potentially improving rendering efficiency.
		 *
		 * The screen-space depth (in range [-1,1] as opposed to window coordinate range [0,1]) is
		 * always output by the fragment shader, during rendering, to the draw buffer at index 1
		 * (the scalar field colour is output to draw buffer index 0).
		 * By default this is ignored since default state of glDrawBuffers is NONE for indices >= 1.
		 * But if you attach a viewport-size floating-point texture to a GLFrameBufferObject,
		 * and call GLFrameBufferObject::gl_draw_buffers(), then depth will get written to the
		 * texture for those pixels on the rendered iso-surface (or cross-section).
		 */
		void
		render_cross_sections(
				GLRenderer &renderer,
				cache_handle_type &cache_handle,
				const cross_sections_seq_type &cross_sections,
				GPlatesViewOperations::ScalarField3DRenderParameters::CrossSectionColourMode colour_mode,
				const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction,
				const std::vector<float> &test_variables,
				boost::optional<SurfaceFillMask> surface_fill_mask = boost::none,
				boost::optional<GLTexture::shared_ptr_to_const_type> surface_occlusion_texture = boost::none);

	private:

		/**
		 * The version of GLSL shading language to use in our shaders.
		 *
		 * We're currently leaving this as the default version (1.2) since, although we use OpenGL 3.0
		 * features (GLSL 1.3), we're not currently sure whether MacOS Snow Leopard officially
		 * supports OpenGL 3.0 (and associated GLSL 1.3) so we use the shader '#extension' mechanism
		 * where possible instead. This works for GL_EXT_texture_array but may need to up the GLSL version
		 * if start using other features that don't have GLSL '#extension's.
		 *
		 * Also GLSL 1.2 supports 'gl_FragData' (part of GL_ARB_draw_buffers) so we don't
		 * need to declare '#extension GL_ARB_draw_buffers : enable' in the fragment shader.
		 */
		static const GLShaderSource::ShaderVersion SHADER_VERSION = GLShaderSource::DEFAULT_SHADER_VERSION;

		/**
		 * The resolution of the 1D texture for converting depth radii to layer indices.
		 *
		 * This needs to be reasonably high since non-uniformly spaced depth layers will have
		 * mapping errors within a texel distance of each layer's depth due to the linear filtering,
		 * interpolating between the nearest texel centres surrounding a layer's depth, not being
		 * able to reproduce the non-linear (piecewise linear) mapping in that region.
		 */
		static const unsigned int DEPTH_RADIUS_TO_LAYER_RESOLUTION = 2048;

		/**
		 * The resolution of the 1D texture for converting scalar values (or gradient magnitudes) to colour.
		 *
		 * This needs to be reasonably high since non-uniformly spaced colour slices (in palette) will
		 * have mapping errors within a texel distance of each colour slice boundary due to the linear
		 * filtering, interpolating between the nearest texel centres surrounding a colour slice boundary,
		 * not being able to reproduce the non-linear (piecewise linear) mapping in that region.
		 */
		static const unsigned int COLOUR_PALETTE_RESOLUTION = 2048;

		/**
		 * The (square) texture dimension of the textures in the surface fill mask texture array.
		 *
		 * We set this the texture dimension to a reasonable value for now.
		 * TODO: Adjust the texture dimension to provide more surface fill mask detail when
		 * needed (such as a zoomed view). But keep as power-of-two so that other layers can
		 * typically re-use the acquired texture when we've finished with it - it would be unlikely
		 * to be re-acquired if not something obvious like a power-of-two.
		 */
		static const unsigned int SURFACE_FILL_MASK_RESOLUTION = 512;

		/**
		 * The most texture image units used for any shader program.
		 *
		 * The maximum used is currently in the isosurface shader program:
		 *
		 *  1) sampler2DArray tile_meta_data_sampler;
		 *  2) sampler2DArray field_data_sampler;
		 *  3) sampler2DArray mask_data_sampler;
		 *  4) sampler1D depth_radius_to_layer_sampler;
		 *  5) sampler1D colour_palette_sampler;
		 *  6) sampler2D surface_occlusion_texture_sampler;
		 *  7) sampler2D depth_texture_sampler;
		 *  8) sampler2DArray surface_fill_mask_sampler;
		 *  9) sampler2D volume_fill_depth_range_sampler;
		 * 10) sampler2D volume_fill_walls_sampler;
		 */
		static const unsigned int MAX_TEXTURE_IMAGE_UNITS_USED = 10;

		/**
		 * The maximum number of vertices output by the surface fill mask geometry shader.
		 *
		 * This is one triangle per cube face and 3 vertices per triangle...
		 */
		static const unsigned int SURFACE_FILL_MASK_GEOMETRY_SHADER_MAX_OUTPUT_VERTICES = 6 * 3;

#if 0 // Not currently used...
		/**
		 * We will tessellate a great circle arc, when rendering spherical caps,
		 * if either line segment endpoint is far enough away from the polygon centroid.
		 *
		 * In that case this is the number of subdivisions done (at the globe's surface).
		 * Fewer subdivisions are performed at smaller depths.
		 *
		 * The tessellation is performed using a geometry shader.
		 */
		static const unsigned int SPHERICAL_CAP_NUM_SUBDIVISIONS = 6;

		/**
		 * The maximum number of vertices output by the volume fill spherical cap geometry shaders.
		 *
		 * This is a triangle strip resulting from subdividing an input triangle equally along two
		 * of its edges (but not the third edge)...
		 */
		static const unsigned int VOLUME_FILL_SPHERICAL_CAP_GEOMETRY_SHADER_MAX_OUTPUT_VERTICES =
				2 * SPHERICAL_CAP_NUM_SUBDIVISIONS + 1;
#endif

		/**
		 * The maximum number of vertices output by the volume fill wall geometry shaders.
		 *
		 * This is one quad (two triangles in a triangle strip)...
		 */
		static const unsigned int VOLUME_FILL_WALL_GEOMETRY_SHADER_MAX_OUTPUT_VERTICES = 4;

		/**
		 * The number of bytes in the vertex buffer used to stream.
		 */
		static const unsigned int NUM_BYTES_IN_STREAMING_VERTEX_BUFFER = 4 * 1024 * 1024;

		/**
		 * The minimum number of bytes to stream in the vertex buffer.
		 */
		static const unsigned int MINIMUM_BYTES_TO_STREAM_IN_VERTEX_BUFFER =
				NUM_BYTES_IN_STREAMING_VERTEX_BUFFER / 8;

		/**
		 * The number of bytes in the vertex element (indices) buffer used to stream.
		 */
		static const unsigned int NUM_BYTES_IN_STREAMING_VERTEX_ELEMENT_BUFFER =
				NUM_BYTES_IN_STREAMING_VERTEX_BUFFER / 8/*indices need a lot less space than vertices*/;

		/**
		 * The minimum number of bytes to stream in the vertex element buffer.
		 */
		static const unsigned int MINIMUM_BYTES_TO_STREAM_IN_VERTEX_ELEMENT_BUFFER =
				NUM_BYTES_IN_STREAMING_VERTEX_ELEMENT_BUFFER / 8;


		//! Typedef for vertex elements (indices) used for streaming vertex array.
		typedef GLuint streaming_vertex_element_type;


		/**
		 * A vertex of a cross-section geometry.
		 */
		struct CrossSectionVertex
		{
			GLfloat surface_point[3];
			GLfloat depth_weight;
			// Used to calculate cross-section normal for 2D cross-sections.
			// It is not used for 1D cross-sections.
			GLfloat neighbour_surface_point[3];
			// Is 0.0 for 1D cross-sections.
			// Is -1 or 1 for 2D cross-sections to ensure calculated normal faces forward.
			GLfloat normal_weight;
		};

		//! Typedef for a static stream of cross-section vertices.
		typedef GLStaticStreamPrimitives<CrossSectionVertex, streaming_vertex_element_type>
				cross_section_stream_primitives_type;

		/**
		 * Renders points/multipoints as vertically extruded cross-section 1D (line) geometries.
		 */
		class CrossSection1DGeometryOnSphereVisitor :
				public GPlatesMaths::ConstGeometryOnSphereVisitor
		{
		public:

			CrossSection1DGeometryOnSphereVisitor(
					GLRenderer &renderer,
					const GLVertexElementBuffer::shared_ptr_type &streaming_vertex_element_buffer,
					const GLVertexBuffer::shared_ptr_type &streaming_vertex_buffer,
					const GLVertexArray::shared_ptr_type &vertex_array);

			void
			begin_rendering();

			void
			end_rendering();

			virtual
			void
			visit_multi_point_on_sphere(
					GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere);

			virtual
			void
			visit_point_on_sphere(
					GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere);

		private:

			void
			render_cross_section_1d(
					const GPlatesMaths::UnitVector3D &surface_point);

			GLRenderer &d_renderer;
			GLVertexArray::shared_ptr_type d_vertex_array;
			GLBuffer::MapBufferScope d_map_vertex_element_buffer_scope;
			GLBuffer::MapBufferScope d_map_vertex_buffer_scope;
			cross_section_stream_primitives_type d_stream;
			cross_section_stream_primitives_type::StreamTarget d_stream_target;
			cross_section_stream_primitives_type::Primitives d_stream_primitives;
		};

		/**
		 * Renders polylines/polygons as vertically extruded cross-section 2D (mesh) geometries.
		 */
		class CrossSection2DGeometryOnSphereVisitor :
				public GPlatesMaths::ConstGeometryOnSphereVisitor
		{
		public:

			CrossSection2DGeometryOnSphereVisitor(
					GLRenderer &renderer,
					const GLVertexElementBuffer::shared_ptr_type &streaming_vertex_element_buffer,
					const GLVertexBuffer::shared_ptr_type &streaming_vertex_buffer,
					const GLVertexArray::shared_ptr_type &vertex_array);

			void
			begin_rendering();

			void
			end_rendering();

			virtual
			void
			visit_polygon_on_sphere(
					GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere);

			virtual
			void
			visit_polyline_on_sphere(
					GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere);

		private:

			template <typename GreatCircleArcForwardIter>
			void
			render_cross_sections_2d(
					GreatCircleArcForwardIter begin_arcs,
					GreatCircleArcForwardIter end_arcs);

			void
			render_cross_section_2d(
					const GPlatesMaths::UnitVector3D &start_surface_point,
					const GPlatesMaths::UnitVector3D &end_surface_point);

			GLRenderer &d_renderer;
			GLVertexArray::shared_ptr_type d_vertex_array;
			GLBuffer::MapBufferScope d_map_vertex_element_buffer_scope;
			GLBuffer::MapBufferScope d_map_vertex_buffer_scope;
			cross_section_stream_primitives_type d_stream;
			cross_section_stream_primitives_type::StreamTarget d_stream_target;
			cross_section_stream_primitives_type::Primitives d_stream_primitives;
		};

		/**
		 * A vertex of a surface fill mask geometry.
		 */
		struct SurfaceFillMaskVertex
		{
			GLfloat surface_point[3];
		};

		//! Typedef for a static stream of surface fill mask geometry vertices.
		typedef GLStaticStreamPrimitives<SurfaceFillMaskVertex, streaming_vertex_element_type>
				surface_fill_mask_stream_primitives_type;

		/**
		 * Renders filled polygons (and optionally polylines) to the surface fill mask.
		 */
		class SurfaceFillMaskGeometryOnSphereVisitor :
				public GPlatesMaths::ConstGeometryOnSphereVisitor
		{
		public:

			SurfaceFillMaskGeometryOnSphereVisitor(
					GLRenderer &renderer,
					const GLVertexElementBuffer::shared_ptr_type &streaming_vertex_element_buffer,
					const GLVertexBuffer::shared_ptr_type &streaming_vertex_buffer,
					const GLVertexArray::shared_ptr_type &vertex_array,
					bool include_polylines);

			virtual
			void
			visit_polygon_on_sphere(
					GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere);

			virtual
			void
			visit_polyline_on_sphere(
					GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere);

		private:

			template <typename PointOnSphereForwardIter>
			void
			render_surface_fill_mask(
					const PointOnSphereForwardIter begin_points,
					const unsigned int num_points,
					const GPlatesMaths::UnitVector3D &centroid);

			template <typename PointOnSphereForwardIter>
			void
			render_surface_fill_mask_geometry(
					const PointOnSphereForwardIter begin_points,
					const unsigned int num_points,
					const GPlatesMaths::UnitVector3D &centroid,
					bool &entire_geometry_is_in_stream_target);

			GLRenderer &d_renderer;
			GLVertexArray::shared_ptr_type d_vertex_array;
			GLBuffer::MapBufferScope d_map_vertex_element_buffer_scope;
			GLBuffer::MapBufferScope d_map_vertex_buffer_scope;
			surface_fill_mask_stream_primitives_type d_stream;
			surface_fill_mask_stream_primitives_type::StreamTarget d_stream_target;
			surface_fill_mask_stream_primitives_type::Primitives d_stream_primitives;
			bool d_include_polylines;
		};

		/**
		 * A vertex of a volume fill boundary geometry.
		 */
		struct VolumeFillBoundaryVertex
		{
			GLfloat surface_point[3];
			GLfloat centroid_point[3]; // Polygon (or polyline) centroid - only used for spherical caps.
		};

		//! Typedef for a static stream of volume fill boundary geometry vertices.
		typedef GLStaticStreamPrimitives<VolumeFillBoundaryVertex, streaming_vertex_element_type>
				volume_fill_boundary_stream_primitives_type;

		/**
		 * Class for rendering boundary surface of volume fill region from the same
		 * surface polygons (and optionally polylines) used to generate the surface fill mask.
		 */
		class VolumeFillBoundaryGeometryOnSphereVisitor :
				public GPlatesMaths::ConstGeometryOnSphereVisitor
		{
		public:

			VolumeFillBoundaryGeometryOnSphereVisitor(
					GLRenderer &renderer,
					const GLVertexElementBuffer::shared_ptr_type &streaming_vertex_element_buffer,
					const GLVertexBuffer::shared_ptr_type &streaming_vertex_buffer,
					const GLVertexArray::shared_ptr_type &vertex_array,
					bool include_polylines);

			void
			begin_rendering();

			void
			end_rendering();

			virtual
			void
			visit_polygon_on_sphere(
					GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere);

			virtual
			void
			visit_polyline_on_sphere(
					GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere);

		private:

			template <typename GreatCircleArcForwardIter>
			void
			render_volume_fill_boundary(
					GreatCircleArcForwardIter begin_arcs,
					GreatCircleArcForwardIter end_arcs,
					const GPlatesMaths::UnitVector3D &centroid);

			void
			render_volume_fill_boundary_segment(
					const GPlatesMaths::UnitVector3D &start_surface_point,
					const GPlatesMaths::UnitVector3D &end_surface_point,
					VolumeFillBoundaryVertex &vertex);

			void
			render_stream();

			GLRenderer &d_renderer;
			GLVertexArray::shared_ptr_type d_vertex_array;
			GLBuffer::MapBufferScope d_map_vertex_element_buffer_scope;
			GLBuffer::MapBufferScope d_map_vertex_buffer_scope;
			volume_fill_boundary_stream_primitives_type d_stream;
			volume_fill_boundary_stream_primitives_type::StreamTarget d_stream_target;
			volume_fill_boundary_stream_primitives_type::Primitives d_stream_primitives;
			bool d_include_polylines;
		};

		/**
		 * Used to recurse into a hierarchical triangular mesh to generate white inner sphere.
		 */
		class SphereMeshBuilder
		{
		public:
			//! Constructor.
			SphereMeshBuilder(
					std::vector<GLColourVertex> &vertices,
					std::vector<GLuint> &vertex_elements,
					const GPlatesGui::rgba8_t &colour,
					unsigned int recursion_depth_to_generate_mesh);

			void
			visit(
					const GPlatesMaths::HierarchicalTriangularMeshTraversal::Triangle &triangle,
					const unsigned int &recursion_depth);

		private:

			std::vector<GLColourVertex> &d_vertices;
			std::vector<GLuint> &d_vertex_elements;
			GPlatesGui::rgba8_t d_colour;
			unsigned int d_recursion_depth_to_generate_mesh;
		};


		/**
		 * Used to inform clients that we have been updated.
		 */
		mutable GPlatesUtils::SubjectToken d_subject_token;

		/**
		 * The light (direction) used during lighting.
		 */
		GLLight::non_null_ptr_type d_light;

		//! Keep track of changes to @a d_light.
		mutable GPlatesUtils::ObserverToken d_light_observer_token;

		//! The tile metadata resolution of the current scalar field.
		unsigned int d_tile_meta_data_resolution;

		//! The tile resolution of the current scalar field.
		unsigned int d_tile_resolution;

		//! The number of active tiles of the current scalar field.
		unsigned int d_num_active_tiles;

		//! The number of depth layers of the current scalar field.
		unsigned int d_num_depth_layers;

		//! Minimum depth layer radius (closest to Earth's core).
		float d_min_depth_layer_radius;

		//! Maximum depth layer radius (closest to Earth's surface).
		float d_max_depth_layer_radius;

		//! The radius of each depth layer - ordered from smaller to larger radii.
		std::vector<float> d_depth_layer_radii;

		//! The minimum scalar value across the entire scalar field.
		double d_scalar_min;

		//! The maximum scalar value across the entire scalar field.
		double d_scalar_max;

		//! The mean scalar value across the entire scalar field.
		double d_scalar_mean;

		//! The scalar standard deviation across the entire scalar field.
		double d_scalar_standard_deviation;

		//! The minimum gradient magnitude across the entire scalar field.
		double d_gradient_magnitude_min;

		//! The maximum gradient magnitude across the entire scalar field.
		double d_gradient_magnitude_max;

		//! The mean gradient magnitude across the entire scalar field.
		double d_gradient_magnitude_mean;

		//! The gradient magnitude standard deviation across the entire scalar field.
		double d_gradient_magnitude_standard_deviation;

		/**
		 * Texture array where each texel contains metadata for a tile (tile ID, max/min scalar value).
		 */
		GLTexture::shared_ptr_type d_tile_meta_data_texture_array;

		/**
		 * Texture array containing the field data (scalar value and gradient).
		 */
		GLTexture::shared_ptr_type d_field_data_texture_array;

		/**
		 * Texture array containing the mask data.
		 */
		GLTexture::shared_ptr_type d_mask_data_texture_array;

		/**
		 * 1D texture to map layer depth radii to layer indices (into texture array).
		 */
		GLTexture::shared_ptr_type d_depth_radius_to_layer_texture;

		/**
		 * 1D texture to map scalar values (or gradient magnitudes) to colour.
		 */
		GLTexture::shared_ptr_type d_colour_palette_texture;

		/**
		 * The current range of the colour palette.
		 */
		std::pair<double, double> d_colour_palette_value_range;

		/**
		 * Shader program for rendering an iso-surface.
		 */
		boost::optional<GLProgramObject::shared_ptr_type> d_render_iso_surface_program_object;

		/**
		 * Shader program for rendering a cross-section of an iso-surface.
		 */
		boost::optional<GLProgramObject::shared_ptr_type> d_render_cross_section_program_object;

		/**
		 * Shader program for rendering surface fill mask as optional preliminary step to rendering isosurface.
		 */
		boost::optional<GLProgramObject::shared_ptr_type> d_render_surface_fill_mask_program_object;

#if 0 // Not currently used...
		/**
		 * Shader program for rendering volume fill spherical cap (depth range).
		 */
		boost::optional<GLProgramObject::shared_ptr_type> d_render_volume_fill_spherical_cap_depth_range_program_object;
#endif

		/**
		 * Shader program for rendering volume fill walls (depth range).
		 */
		boost::optional<GLProgramObject::shared_ptr_type> d_render_volume_fill_wall_depth_range_program_object;

		/**
		 * Shader program for rendering volume fill wall surface normals.
		 */
		boost::optional<GLProgramObject::shared_ptr_type> d_render_volume_fill_wall_surface_normals_program_object;

		/**
		 * The (square) texture dimension of the textures in the surface fill mask texture array.
		 */
		unsigned int d_surface_fill_mask_resolution;

		/**
		 * Used to stream indices (vertex elements) for cross-section geometry and surface fill masks.
		 */
		GLVertexElementBuffer::shared_ptr_type d_streaming_vertex_element_buffer;

		/**
		 * Used to stream vertices for cross-section geometry and surface fill masks.
		 */
		GLVertexBuffer::shared_ptr_type d_streaming_vertex_buffer;

		/**
		 * Used to contain cross-section geometries.
		 */
		GLVertexArray::shared_ptr_type d_cross_section_vertex_array;

		/**
		 * Used to contain surface geometries (when rendering surface fill mask).
		 */
		GLVertexArray::shared_ptr_type d_surface_fill_mask_vertex_array;

		/**
		 * Used to contain surface geometries (when rendering volume fill boundary).
		 */
		GLVertexArray::shared_ptr_type d_volume_fill_boundary_vertex_array;

		/**
		 * Vertex array for white inner sphere.
		 */
		GLVertexArray::shared_ptr_type d_white_inner_sphere_vertex_array;

		/**
		 * Compiled draw state for white inner sphere.
		 */
		boost::optional<GLCompiledDrawState::non_null_ptr_to_const_type> d_white_inner_sphere_compiled_draw_state;

		/**
		 * Shader program for rendering white inner sphere (with lighting).
		 */
		boost::optional<GLProgramObject::shared_ptr_type> d_render_white_inner_sphere_program_object;

		/**
		 * Shader program for rendering inner sphere as screen-space depth.
		 */
		boost::optional<GLProgramObject::shared_ptr_type> d_render_depth_range_inner_sphere_program_object;


		//! Constructor.
		GLScalarField3D(
				GLRenderer &renderer,
				const QString &scalar_field_filename,
				const GLLight::non_null_ptr_type &light);

		void
		initialise_inner_sphere(
				GLRenderer &renderer);

		void
		allocate_streaming_vertex_buffers(
				GLRenderer &renderer);

		void
		initialise_cross_section_rendering(
				GLRenderer &renderer);

		void
		initialise_iso_surface_rendering(
				GLRenderer &renderer);

		void
		initialise_surface_fill_mask_rendering(
				GLRenderer &renderer);

		void
		initialise_volume_fill_boundary_rendering(
				GLRenderer &renderer);

		void
		initialise_shader_utils(
				GLRenderer &renderer,
				const GLProgramObject::shared_ptr_type &program_object);

		boost::optional<GLProgramObject::shared_ptr_type>
		create_shader_program(
				GLRenderer &renderer,
				const QString &vertex_shader_source_file_name,
				const QString &fragment_shader_source_file_name,
				// Optional geometry shader source file name and program parameters...
				boost::optional< std::pair<QString, GLShaderProgramUtils::GeometryShaderProgramParameters> > geometry_shader = boost::none,
				const char *shader_defines = "");

		void
		create_tile_meta_data_texture_array(
				GLRenderer &renderer);
		void
		create_field_data_texture_array(
				GLRenderer &renderer);

		void
		create_mask_data_texture_array(
				GLRenderer &renderer);

		void
		create_depth_radius_to_layer_texture(
				GLRenderer &renderer);

		void
		create_colour_palette_texture(
				GLRenderer &renderer);

		GLTexture::shared_ptr_to_const_type
		acquire_surface_fill_mask_texture(
				GLRenderer &renderer);

		void
		load_scalar_field(
				GLRenderer &renderer,
				const GPlatesFileIO::ScalarField3DFileFormat::Reader &scalar_field_reader);

		void
		load_depth_radius_to_layer_texture(
				GLRenderer &renderer);

		void
		load_colour_palette_texture(
				GLRenderer &renderer,
				const GPlatesGui::ColourPalette<double>::non_null_ptr_to_const_type &colour_palette,
				const std::pair<double, double> &colour_palette_value_range);

		void
		set_iso_surface_and_cross_sections_shader_common_variables(
				GLRenderer &renderer,
				const GLProgramObject::shared_ptr_type &program_object,
				unsigned int &current_texture_unit,
				const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction,
				const std::vector<float> &test_variables,
				boost::optional<GLTexture::shared_ptr_to_const_type> surface_occlusion_texture);

		void
		set_shader_test_variables(
				GLRenderer &renderer,
				const GLProgramObject::shared_ptr_type &program_object,
				const std::vector<float> &test_variables);

		//! Returns true if rendered successfully - should always return true but just in case.
		bool
		render_surface_fill_mask(
				GLRenderer &renderer,
				const surface_polygons_mask_seq_type &surface_polygons_mask,
				bool include_polylines,
				GLTexture::shared_ptr_to_const_type &surface_fill_mask_texture);

		//! Returns true if rendered successfully - should always return true but just in case.
		bool
		render_volume_fill_wall_depth_range(
				GLRenderer &renderer,
				const surface_polygons_mask_seq_type &surface_polygons_mask,
				bool include_polylines,
				const GLTexture::shared_ptr_to_const_type &surface_fill_mask_texture,
				const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction,
				boost::optional<GLScreenRenderTarget::shared_ptr_type> &volume_fill_depth_range_screen_render_target);

		//! Returns true if rendered successfully - should always return true but just in case.
		bool
		render_volume_fill_wall_surface_normal_and_depth(
				GLRenderer &renderer,
				const surface_polygons_mask_seq_type &surface_polygons_mask,
				bool include_polylines,
				bool only_show_boundary_walls,
				const GLTexture::shared_ptr_to_const_type &surface_fill_mask_texture,
				const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction,
				boost::optional<GLScreenRenderTarget::shared_ptr_type> &volume_fill_surface_normals_screen_render_target);

		void
		render_cross_sections_1d(
				GLRenderer &renderer,
				const GLVertexElementBuffer::shared_ptr_type &streaming_vertex_element_buffer,
				const GLVertexBuffer::shared_ptr_type &streaming_vertex_buffer,
				const GLVertexArray::shared_ptr_type &cross_section_vertex_array,
				const cross_sections_seq_type &cross_sections);

		void
		render_cross_sections_2d(
				GLRenderer &renderer,
				const GLVertexElementBuffer::shared_ptr_type &streaming_vertex_element_buffer,
				const GLVertexBuffer::shared_ptr_type &streaming_vertex_buffer,
				const GLVertexArray::shared_ptr_type &cross_section_vertex_array,
				const cross_sections_seq_type &cross_sections);

		void
		render_white_inner_sphere(
				GLRenderer &renderer,
				const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction);

		void
		render_inner_sphere_depth_range(
				GLRenderer &renderer,
				const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction);
	};
}

#endif // GPLATES_OPENGL_GLSCALARFIELD3D_H
