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

#ifndef GPLATES_GUI_LAYERPAINTER_H
#define GPLATES_GUI_LAYERPAINTER_H

#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "ColourPalette.h"
#include "MapProjection.h"
#include "RasterColourPalette.h"
#include "SceneLightingParameters.h"

#include "app-logic/ResolvedRaster.h"
#include "app-logic/ResolvedScalarField3D.h"

#include "maths/UnitQuaternion3D.h"

#include "opengl/GLLight.h"
#include "opengl/GLFilledPolygonsGlobeView.h"
#include "opengl/GLFilledPolygonsMapView.h"
#include "opengl/GLProgramObject.h"
#include "opengl/GLStreamPrimitives.h"
#include "opengl/GLTexture.h"
#include "opengl/GLVertexArray.h"
#include "opengl/GLVertexBuffer.h"
#include "opengl/GLVertexElementBuffer.h"
#include "opengl/GLVisualLayers.h"

#include "view-operations/ScalarField3DRenderParameters.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
}

namespace GPlatesGui
{
	/**
	 * Interface for streaming and queuing and rendering primitives/drawables for a single layer.
	 *
	 * Later this interface will include low-level general purpose symbol rendering (marker/line/fill).
	 */
	class LayerPainter :
			private boost::noncopyable
	{
	public:
		//! Typedef for a vertex element (index).
		typedef GLuint vertex_element_type;

		//! Typedef for a sequence of vertex elements.
		typedef std::vector<vertex_element_type> vertex_element_seq_type;

		//! Typedef for a coloured vertex.
		typedef GPlatesOpenGL::GLColourVertex coloured_vertex_type;

		//! Typedef for a sequence of coloured vertices.
		typedef std::vector<coloured_vertex_type> coloured_vertex_seq_type;

		//! Typedef for a primitives stream containing coloured vertices.
		typedef GPlatesOpenGL::GLDynamicStreamPrimitives<coloured_vertex_type, vertex_element_type>
				stream_primitives_type;

		/**
		 * A vertex of an axially symmetric (about model-space z-axis) triangle mesh.
		 *
		 * This enables the mesh have correct surface lighting (when lighting is supported and enabled).
		 * When the mesh is not lit then the extra lighting-specific vertex attributes are ignored.
		 *
		 * NOTE: In order for mesh surface lighting to work correctly the mesh must be axially
		 * symmetric about its model-space z-axis (ie, the mesh must be created with this in mind).
		 * If this isn't the case then the fragment shader used to light the mesh will not work.
		 *
		 * The mesh normal (used when calculating lighting in vertex/fragment shaders) is determined
		 * by weighting the radial normal and the axial normal. We do this instead of the usual
		 * storing of per-vertex normals because for a cone (used in arrow heads) it is difficult
		 * to get the correct lighting at the cone apex (even when using multiple apex vertices
		 * with same position but with different normals). For more details see
		 * http://stackoverflow.com/questions/15283508/low-polygon-cone-smooth-shading-at-the-tip
		 */
		struct AxiallySymmetricMeshVertex
		{
			AxiallySymmetricMeshVertex()
			{  }

			AxiallySymmetricMeshVertex(
					const GPlatesMaths::Vector3D &world_space_position_,
					GPlatesGui::rgba8_t colour_,
					const GPlatesMaths::UnitVector3D &world_space_x_axis_,
					const GPlatesMaths::UnitVector3D &world_space_y_axis_,
					const GPlatesMaths::UnitVector3D &world_space_z_axis_,
					GLfloat model_space_x_position_,
					GLfloat model_space_y_position_,
					GLfloat radial_normal_weight_,
					GLfloat axial_normal_weight_);


			// These should be declared first (our non-generic attribute binding relies on this)...
			GLfloat world_space_position[3]; // vertex position in world space.
			GPlatesGui::rgba8_t colour; // colour is same size as GLfloat, so no structure packing issues here.

			// Lighting-specific attributes...
			GLfloat world_space_x_axis[3];
			GLfloat world_space_y_axis[3];
			GLfloat world_space_z_axis[3];
			GLfloat model_space_radial_position[2]; // x and y components of model-space vertex position.
			GLfloat radial_and_axial_normal_weights[2]; // normal is weighted by radial (x,y) normal and axial (z) normal.
		};

		//! Typedef for a sequence of axially symmetric mesh vertices.
		typedef std::vector<AxiallySymmetricMeshVertex> axially_symmetric_mesh_vertex_seq_type;

		//! Typedef for a primitives stream containing vertices of an axially symmetric mesh.
		typedef GPlatesOpenGL::GLDynamicStreamPrimitives<AxiallySymmetricMeshVertex, vertex_element_type>
				axially_symmetric_mesh_stream_primitives_type;

		/**
		 * Typedef for an opaque object that caches a particular painting.
		 */
		typedef boost::shared_ptr<void> cache_handle_type;


		/**
		 * Drawables for points, lines and polygons (triangles and quads).
		 */
		class PointLinePolygonDrawables
		{
		public:
			/**
			 * Prepares for streaming vertices.
			 */
			void
			begin_painting();

			/**
			 * Ends streaming vertices and renders any streamed primitives.
			 */
			void
			end_painting(
					GPlatesOpenGL::GLRenderer &renderer,
					GPlatesOpenGL::GLBuffer &vertex_element_buffer_data,
					GPlatesOpenGL::GLBuffer &vertex_buffer_data,
					GPlatesOpenGL::GLVertexArray &vertex_array,
					GPlatesOpenGL::GLVertexArray &unlit_axially_symmetric_mesh_vertex_array,
					GPlatesOpenGL::GLVertexArray &lit_axially_symmetric_mesh_vertex_array,
					GPlatesOpenGL::GLVisualLayers &gl_visual_layers,
					boost::optional<MapProjection::non_null_ptr_to_const_type> map_projection,
					boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
							render_point_line_polygon_lighting_in_globe_view_program_object,
					boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
							render_point_line_polygon_lighting_in_map_view_program_object,
					boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
							render_axially_symmetric_mesh_lighting_program_object);

			/**
			 * Returns the stream for points of size @a point_size.
			 */
			stream_primitives_type &
			get_points_stream(
					float point_size);

			/**
			 * Returns the stream for lines of width @a line_width.
			 */
			stream_primitives_type &
			get_lines_stream(
					float line_width);

			/**
			 * Returns the stream for triangle meshes.
			 *
			 * There's no point size or line width equivalent for polygons
			 * so they all get lumped into a single stream.
			 */
			stream_primitives_type &
			get_triangles_stream();

			/**
			 * Returns the stream for triangle meshes that are rotationally symmetric about an axis.
			 *
			 * An axially symmetric triangle mesh should be symmetric about its model space z-axis
			 * (see @a AxiallySymmetricMeshVertex for more details).
			 *
			 * The triangles in the mesh should have their front (outward-facing) faces oriented
			 * counter-clockwise (the default front-face mode in OpenGL) since back faces
			 * (triangles facing away from the camera) are culled (the default in OpenGL).
			 * This culling is done in case the mesh is semi-transparent (in which case you don't
			 * want to see the back faces because their lighting will be incorrect - it's meant for
			 * the other side of the face).
			 *
			 * As noted above, back faces are culled, so the mesh should ideally be generated such
			 * that its interior is not visible (eg, a closed mesh).
			 *
			 * The use of this stream (for axially symmetric meshes) means surface lighting
			 * (when supported and enabled) will work correctly in the presence of difficult-to-light
			 * objects such as cones (see @a AxiallySymmetricMeshVertex for more details).
			 */
			axially_symmetric_mesh_stream_primitives_type &
			get_axially_symmetric_mesh_triangles_stream();

			/**
			 * Drawables that get filled in their interior (for rendering to the 3D globe view).
			 *
			 * For 'filled' to make any sense these drawables should have a sequence of points that
			 * defines some kind of outline (the outline can be concave or convex).
			 */
			GPlatesOpenGL::GLFilledPolygonsGlobeView::filled_drawables_type &
			get_filled_polygons_globe_view()
			{
				return d_filled_polygons_globe_view;
			}

			/**
			 * Drawables that get filled in their interior (for rendering to a 2D map view).
			 */
			GPlatesOpenGL::GLFilledPolygonsMapView::filled_drawables_type &
			get_filled_polygons_map_view()
			{
				return d_filled_polygons_map_view;
			}

		private:

			/**
			 * Information to render a group of primitives (point, line or triangle primitives).
			 */
			template <class VertexType>
			class Drawables
			{
			public:

				void
				begin_painting();

				void
				end_painting(
						GPlatesOpenGL::GLRenderer &renderer,
						GPlatesOpenGL::GLBuffer &vertex_element_buffer_data,
						GPlatesOpenGL::GLBuffer &vertex_buffer_data,
						GPlatesOpenGL::GLVertexArray &vertex_array,
						GLenum mode);

				// Can only be called between @a begin_painting and @a end_painting.
				GPlatesOpenGL::GLDynamicStreamPrimitives<VertexType, vertex_element_type> &
				get_stream();

				// Can only be called between @a begin_painting and @a end_painting.
				bool
				has_primitives() const;

			private:
				//! The vertex (and vertex elements) stream - only used between @a begin_painting and @a end_painting.
				struct Stream
				{
					Stream() :
						stream_target(stream_primitives)
					{  }

					GPlatesOpenGL::GLDynamicStreamPrimitives<VertexType, vertex_element_type> stream_primitives;
					// Must be declared *after* @a stream_primitives...
					typename GPlatesOpenGL::GLDynamicStreamPrimitives<VertexType, vertex_element_type>::StreamTarget stream_target;
				};

				vertex_element_seq_type d_vertex_elements;
				std::vector<VertexType> d_vertices;

				boost::shared_ptr<Stream> d_stream;

				void
				draw_primitives(
						GPlatesOpenGL::GLRenderer &renderer,
						GPlatesOpenGL::GLBuffer &vertex_element_buffer_data,
						GPlatesOpenGL::GLBuffer &vertex_buffer_data,
						GPlatesOpenGL::GLVertexArray &vertex_array,
						GLenum mode);

				void
				draw_feedback_primitives_to_qpainter(
						GPlatesOpenGL::GLRenderer &renderer,
						GPlatesOpenGL::GLBuffer &vertex_element_buffer_data,
						GPlatesOpenGL::GLBuffer &vertex_buffer_data,
						GPlatesOpenGL::GLVertexArray &vertex_array,
						GLenum mode);
			};


			/**
			 * Typedef for mapping point size to drawable.
			 * All points of the same size will get grouped together.
			 */
			typedef std::map<GPlatesMaths::real_t, Drawables<coloured_vertex_type> > point_size_to_drawables_map_type;

			/**
			 * Typedef for mapping line width to drawable.
			 * All lines of the same width will get grouped together.
			 */
			typedef std::map<GPlatesMaths::real_t, Drawables<coloured_vertex_type> > line_width_to_drawables_map_type;


			point_size_to_drawables_map_type d_point_drawables_map;
			line_width_to_drawables_map_type d_line_drawables_map;

			/**
			 * Regular drawables (coloured vertices).
			 *
			 * There's no point size or line width equivalent for triangles so they can be lumped
			 * into a single drawables group.
			 */
			Drawables<coloured_vertex_type> d_triangle_drawables;

			/**
			 * Axially symmetric drawables.
			 */
			Drawables<AxiallySymmetricMeshVertex> d_axially_symmetric_mesh_triangle_drawables;

			//! For collecting filled polygons during a render call to render to the 3D globe view.
			GPlatesOpenGL::GLFilledPolygonsGlobeView::filled_drawables_type d_filled_polygons_globe_view;

			//! For collecting filled polygons during a render call to render to a 2D map view.
			GPlatesOpenGL::GLFilledPolygonsMapView::filled_drawables_type d_filled_polygons_map_view;


			void
			paint_filled_polygons(
					GPlatesOpenGL::GLRenderer &renderer,
					GPlatesOpenGL::GLVisualLayers &gl_visual_layers,
					boost::optional<MapProjection::non_null_ptr_to_const_type> map_projection);

			/**
			 * Sets generic lighting for point/line/polygon primitives.
			 *
			 * Returns true if lighting is supported and enabled for point/line/polygons,
			 * otherwise does not set any state (ie, just uses existing state).
			 */
			bool
			set_generic_point_line_polygon_lighting_state(
					GPlatesOpenGL::GLRenderer &renderer,
					GPlatesOpenGL::GLVisualLayers &gl_visual_layers,
					boost::optional<MapProjection::non_null_ptr_to_const_type> map_projection,
					boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
							render_point_line_polygon_lighting_in_globe_view_program_object,
					boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
							render_point_line_polygon_lighting_in_map_view_program_object);

			/**
			 * Sets lighting for axially symmetric meshes.
			 *
			 * Returns true if lighting is supported and enabled, otherwise does not set
			 * any state (ie, just uses existing state).
			 */
			bool
			set_axially_symmetric_mesh_lighting_state(
					GPlatesOpenGL::GLRenderer &renderer,
					GPlatesOpenGL::GLVisualLayers &gl_visual_layers,
					boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
							render_axially_symmetric_mesh_lighting_program_object);
		};


		/**
		 * Information to render a text string located at a 2D viewport position.
		 */
		struct TextDrawable2D
		{
			TextDrawable2D(
					QString text_,
					QFont font_,
					const double &world_x_,
					const double &world_y_,
					int x_offset_,
					int y_offset_,
					boost::optional<Colour> colour_,
					boost::optional<Colour> shadow_colour_) :
				text(text_),
				font(font_),
				world_x(world_x_),
				world_y(world_y_),
				x_offset(x_offset_),
				y_offset(y_offset_),
				colour(colour_),
				shadow_colour(shadow_colour_)
			{  }

			QString text;
			QFont font;
			double world_x;
			double world_y;
			int x_offset;
			int y_offset;
			boost::optional<Colour> colour;
			// render drop shadow, if any
			boost::optional<Colour> shadow_colour;
		};


		/**
		 * Information to render a text string located at a 3D world position.
		 *
		 * The 3D world position is transformed using the model-view-projection transform in GLRenderer.
		 */
		struct TextDrawable3D
		{
			TextDrawable3D(
					QString text_,
					QFont font_,
					GPlatesMaths::UnitVector3D world_position_,
					int x_offset_,
					int y_offset_,
					boost::optional<Colour> colour_,
					boost::optional<Colour> shadow_colour_) :
				text(text_),
				font(font_),
				world_position(world_position_),
				x_offset(x_offset_),
				y_offset(y_offset_),
				colour(colour_),
				shadow_colour(shadow_colour_)
			{  }

			QString text;
			QFont font;
			GPlatesMaths::UnitVector3D world_position;
			int x_offset;
			int y_offset;
			boost::optional<Colour> colour;
			// render drop shadow, if any
			boost::optional<Colour> shadow_colour;
		};


		/**
		 * Information to render a raster.
		 */
		struct RasterDrawable
		{
			/**
			 * If @a map_projection_ is specified then the raster is rendered using the specified
			 * 2D map projection, otherwise it's rendered to the 3D globe.
			 */
			RasterDrawable(
					const GPlatesAppLogic::ResolvedRaster::non_null_ptr_to_const_type source_resolved_raster_,
					const RasterColourPalette::non_null_ptr_to_const_type source_raster_colour_palette_,
					const Colour &source_raster_modulate_colour_,
					float normal_map_height_field_scale_factor_) :
				source_resolved_raster(source_resolved_raster_),
				source_raster_colour_palette(source_raster_colour_palette_),
				source_raster_modulate_colour(source_raster_modulate_colour_),
				normal_map_height_field_scale_factor(normal_map_height_field_scale_factor_)
			{  }

			GPlatesAppLogic::ResolvedRaster::non_null_ptr_to_const_type source_resolved_raster;
			RasterColourPalette::non_null_ptr_to_const_type source_raster_colour_palette;
			Colour source_raster_modulate_colour;
			float normal_map_height_field_scale_factor;
		};


		/**
		 * Information to render a scalar field.
		 */
		struct ScalarField3DDrawable
		{
			ScalarField3DDrawable(
					const GPlatesAppLogic::ResolvedScalarField3D::non_null_ptr_to_const_type source_resolved_scalar_field_,
					const GPlatesViewOperations::ScalarField3DRenderParameters &render_parameters_) :
				source_resolved_scalar_field(source_resolved_scalar_field_),
				render_parameters(render_parameters_)
			{  }

			GPlatesAppLogic::ResolvedScalarField3D::non_null_ptr_to_const_type source_resolved_scalar_field;
			GPlatesViewOperations::ScalarField3DRenderParameters render_parameters;
		};


		/**
		 * Constructor.
		 *
		 * @a map_projection is used for painting in a map view (and is none for the 3D globe view).
		 * And currently the 3D globe view uses the depth buffer but the 2D map views don't.
		 */
		explicit
		LayerPainter(
				const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
				boost::optional<MapProjection::non_null_ptr_to_const_type> map_projection = boost::none);

		/**
		 * Initialise objects requiring @a GLRenderer.
		 */
		void
		initialise(
				GPlatesOpenGL::GLRenderer &renderer);

		/**
		 * Must be called before streaming or queuing any primitives.
		 */
		void
		begin_painting(
				GPlatesOpenGL::GLRenderer &renderer);

		/**
		 * Renders any streamed or queued primitives.
		 *
		 * @param surface_occlusion_texture is a viewport-size 2D texture containing the RGBA rendering
		 * of the surface geometries/rasters on the *front* of the globe.
		 * It is only used when rendering sub-surface geometries.
		 */
		cache_handle_type
		end_painting(
				GPlatesOpenGL::GLRenderer &renderer,
				float scale,
				boost::optional<GPlatesOpenGL::GLTexture::shared_ptr_to_const_type> surface_occlusion_texture = boost::none);

		/**
		 * Returns the render.
		 *
		 * NOTE: Can *only* be called between @a begin_painting and @a end_painting.
		 */
		GPlatesOpenGL::GLRenderer &
		get_renderer()
		{
			return d_renderer.get();
		}


		PointLinePolygonDrawables drawables_off_the_sphere;
		PointLinePolygonDrawables drawables_on_the_sphere;

		std::vector<RasterDrawable> rasters;
		std::vector<ScalarField3DDrawable> scalar_fields;
		std::vector<TextDrawable3D> text_drawables_3D;
		std::vector<TextDrawable2D> text_drawables_2D;

	private:

		cache_handle_type
		paint_scalar_fields(
				GPlatesOpenGL::GLRenderer &renderer,
				boost::optional<GPlatesOpenGL::GLTexture::shared_ptr_to_const_type> surface_occlusion_texture);

		cache_handle_type
		paint_rasters(
				GPlatesOpenGL::GLRenderer &renderer);

		void
		paint_text_drawables_2D(
				GPlatesOpenGL::GLRenderer &renderer,
				float scale);

		void
		paint_text_drawables_3D(
				GPlatesOpenGL::GLRenderer &renderer,
				float scale);


		//! References the renderer (is only valid between @a begin_painting and @a end_painting).
		boost::optional<GPlatesOpenGL::GLRenderer &> d_renderer;

		//! For obtaining the OpenGL light and rendering rasters and scalar fields.
		GPlatesOpenGL::GLVisualLayers::non_null_ptr_type d_gl_visual_layers;

		//! Used to stream vertex elements (indices) to.
		GPlatesOpenGL::GLVertexElementBuffer::shared_ptr_type d_vertex_element_buffer;

		//! Used to stream vertices to.
		GPlatesOpenGL::GLVertexBuffer::shared_ptr_type d_vertex_buffer;

		/**
		 * Used when vertices of type @a coloured_vertex_type (streamed to @a d_vertex_buffer).
		 *
		 * This is the standard vertex array - most vertex data is rendered this way.
		 */
		GPlatesOpenGL::GLVertexArray::shared_ptr_type d_vertex_array;

		/**
		 * Used when vertices of type @a AxiallySymmetricMeshVertex are rendered *without* lighting.
		 *
		 * This is the vertex array used for *unlit* axially symmetric meshes.
		 */
		GPlatesOpenGL::GLVertexArray::shared_ptr_type d_unlit_axially_symmetric_mesh_vertex_array;

		/**
		 * Used when vertices of type @a AxiallySymmetricMeshVertex are rendered *with* lighting.
		 *
		 * This is the vertex array used for *lit* axially symmetric meshes.
		 */
		GPlatesOpenGL::GLVertexArray::shared_ptr_type d_lit_axially_symmetric_mesh_vertex_array;

		/**
		 * Used for rendering to a 2D map view (is none for 3D globe view).
		 */
		boost::optional<MapProjection::non_null_ptr_to_const_type> d_map_projection;

		/**
		 * Shader program to render points/lines/polygons with lighting in a 3D *globe* view.
		 *
		 * Is boost::none if not supported by the runtime system -
		 * the fixed-function pipeline is then used (with no lighting).
		 */
		boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
				d_render_point_line_polygon_lighting_in_globe_view_program_object;

		/**
		 * Shader program to render points/lines/polygons with lighting in a 2D *map* view.
		 *
		 * Is boost::none if not supported by the runtime system -
		 * the fixed-function pipeline is then used (with no lighting).
		 */
		boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
				d_render_point_line_polygon_lighting_in_map_view_program_object;

		/**
		 * Shader program for lighting axially symmetric meshes.
		 *
		 * Is boost::none if not supported by the runtime system -
		 * the fixed-function pipeline is then used (with no lighting).
		 */
		boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
				d_render_axially_symmetric_mesh_lighting_program_object;
	};
}

#endif // GPLATES_GUI_LAYERPAINTER_H
