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
#include "TextRenderer.h"

#include "app-logic/ResolvedRaster.h"
#include "app-logic/ResolvedScalarField3D.h"

#include "maths/UnitQuaternion3D.h"

#include "opengl/GLLight.h"
#include "opengl/GLMultiResolutionFilledPolygons.h"
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
	 * Currently this only applies to the globe view (not map views).
	 * Later this interface will be cleaned up and used by all views and also include
	 * low-level general purpose symbol rendering (marker/line/fill).
	 */
	class LayerPainter :
			private boost::noncopyable
	{
	public:
		//! Typedef for a vertex element (index).
		typedef GLuint vertex_element_type;

		//! Typedef for a coloured vertex.
		typedef GPlatesOpenGL::GLColourVertex coloured_vertex_type;

		//! Typedef for a sequence of coloured vertices.
		typedef std::vector<coloured_vertex_type> coloured_vertex_seq_type;

		//! Typedef for a sequence of vertex elements.
		typedef std::vector<vertex_element_type> vertex_element_seq_type;

		//! Typedef for a primitives stream containing coloured vertices.
		typedef GPlatesOpenGL::GLDynamicStreamPrimitives<coloured_vertex_type, vertex_element_type>
				stream_primitives_type;

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
					GPlatesOpenGL::GLVisualLayers &gl_visual_layers,
					boost::optional<MapProjection::non_null_ptr_to_const_type> map_projection,
					boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
							render_point_line_polygon_lighting_program_object);

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
			 * There's no point size of line width equivalent for polygons
			 * so they all get lumped into a single stream.
			 */
			stream_primitives_type &
			get_triangles_stream()
			{
				return d_triangle_drawables.get_stream();
			}

			/**
			 * Drawables that get filled in their interior.
			 *
			 * For 'filled' to make any sense these drawables should have a sequence of points that
			 * defines some kind of outline (the outline can be concave or convex).
			 */
			GPlatesOpenGL::GLMultiResolutionFilledPolygons::filled_polygons_type &
			get_filled_polygons()
			{
				return d_filled_polygons;
			}

		private:
			/**
			 * Information to render a group of primitives (point, line or triangle primitives).
			 */
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

				stream_primitives_type &
				get_stream()
				{
					return d_stream->stream_primitives;
				}

			private:
				//! The vertex (and vertex elements) stream - only used between @a begin_painting and @a end_painting.
				struct Stream
				{
					Stream() :
						stream_target(stream_primitives)
					{  }

					stream_primitives_type stream_primitives;
					// Must be declared *after* @a stream_primitives...
					stream_primitives_type::StreamTarget stream_target;
				};

				vertex_element_seq_type d_vertex_elements;
				coloured_vertex_seq_type d_vertices;

				boost::shared_ptr<Stream> d_stream;
			};


			/**
			 * Typedef for mapping point size to drawable.
			 * All points of the same size will get grouped together.
			 */
			typedef std::map<GPlatesMaths::real_t, Drawables> point_size_to_drawables_map_type;

			/**
			 * Typedef for mapping line width to drawable.
			 * All lines of the same width will get grouped together.
			 */
			typedef std::map<GPlatesMaths::real_t, Drawables> line_width_to_drawables_map_type;


			point_size_to_drawables_map_type d_point_drawables_map;
			line_width_to_drawables_map_type d_line_drawables_map;

			/*
			 * There's no point size or line width equivalent for triangles so they can be lumped
			 * into a single drawables group.
			 */
			Drawables d_triangle_drawables;

			//! For collecting filled polygons during a render call.
			GPlatesOpenGL::GLMultiResolutionFilledPolygons::filled_polygons_type d_filled_polygons;
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
					const Colour &source_raster_modulate_colour_) :
				source_resolved_raster(source_resolved_raster_),
				source_raster_colour_palette(source_raster_colour_palette_),
				source_raster_modulate_colour(source_raster_modulate_colour_)
			{  }

			GPlatesAppLogic::ResolvedRaster::non_null_ptr_to_const_type source_resolved_raster;
			RasterColourPalette::non_null_ptr_to_const_type source_raster_colour_palette;
			Colour source_raster_modulate_colour;
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
				const TextRenderer &text_renderer,
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
		PointLinePolygonDrawables opaque_drawables_on_the_sphere;
		PointLinePolygonDrawables translucent_drawables_on_the_sphere;

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
				const TextRenderer &text_renderer,
				float scale);

		void
		paint_text_drawables_3D(
				GPlatesOpenGL::GLRenderer &renderer,
				const TextRenderer &text_renderer,
				float scale);


		//! References the renderer (is only valid between @a begin_painting and @a end_painting).
		boost::optional<GPlatesOpenGL::GLRenderer &> d_renderer;

		//! For obtaining the OpenGL light and rendering rasters and scalar fields.
		GPlatesOpenGL::GLVisualLayers::non_null_ptr_type d_gl_visual_layers;

		//! Used to stream vertex elements (indices) to.
		GPlatesOpenGL::GLVertexElementBuffer::shared_ptr_type d_vertex_element_buffer;

		//! Used to stream vertices to.
		GPlatesOpenGL::GLVertexBuffer::shared_ptr_type d_vertex_buffer;

		//! Used to bind vertices and vertex elements of @a d_vertex_element_buffer and @a d_vertex_buffer.
		GPlatesOpenGL::GLVertexArray::shared_ptr_type d_vertex_array;

		/**
		 * Used for rendering to a 2D map view (is none for 3D globe view).
		 */
		boost::optional<MapProjection::non_null_ptr_to_const_type> d_map_projection;

		/**
		 * Shader program to render points/lines/polygons with lighting.
		 *
		 * Is boost::none if not supported by the runtime system -
		 * the fixed-function pipeline is then used (with no lighting).
		 */
		boost::optional<GPlatesOpenGL::GLProgramObject::shared_ptr_type>
				d_render_point_line_polygon_lighting_program_object;
	};
}

#endif // GPLATES_GUI_LAYERPAINTER_H
