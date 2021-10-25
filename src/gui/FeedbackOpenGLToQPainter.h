/* $Id$ */
/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, Geological Survey of Norway
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_FEEDBACKOPENGLTOQPAINTER_H
#define GPLATES_GUI_FEEDBACKOPENGLTOQPAINTER_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_array.hpp>
#include <QImage>

#include "opengl/GLTileRender.h"
#include "opengl/GLTransform.h"
#include "opengl/GLViewport.h"
#include "opengl/OpenGL.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
}

namespace GPlatesGui
{
	/**
	 * Used to feedback OpenGL rendering into a QPainter.
	 *
	 * For vector geometry such as points, polylines and polygons the mechanism of OpenGL feedback
	 * is used in order to render to a QPainter's paint device.
	 * Currently we're using base OpenGL feedback which only works with the fixed-function pipeline
	 * and so it doesn't currently work with vertex shaders.
	 *
	 * TODO: Implement OpenGL 2/3 feedback extensions to enable feedback from vertex shaders.
	 *
	 * For rasters the results of rendering are returned in a QImage which is then sent to the QPainter.
	 * This QImage based rendering can also be used for vector geometry if desired (for example, if
	 * rendering to SVG and would like to keep non-geological vector geometries separate from
	 * geological vector geometries then can render the former to a QImage and it'll end up as an
	 * embeded image in the SVG file).
	 */
	class FeedbackOpenGLToQPainter
	{
	public:

		/**
		 * Begins OpenGL feedback of (fixed-function pipeline) vector geometries.
		 *
		 * Constructs a feedback buffer that supports the specified number of points, lines and triangles
		 * regardless of whether they will be clipped by the view frustum or not.
		 */
		void
		begin_render_vector_geometry(
				GPlatesOpenGL::GLRenderer &renderer,
				unsigned int max_num_points,
				unsigned int max_num_lines,
				unsigned int max_num_triangles);

		/**
		 * Ends OpenGL feedback of (fixed-function pipeline) vector geometries and
		 * renders the projected vector geometry to the QPainter set up on the GLRenderer.
		 *
		 * @throws PreconditionViolationError if @a renderer was not set up with a QPainter.
		 * @throws OpenGLException if feedback buffer is not large enough for the vector geometries
		 * rendered since @a begin_render_vector_geometry.
		 */
		void
		end_render_vector_geometry(
				GPlatesOpenGL::GLRenderer &renderer);

		/**
		 * RAII class to call @a begin_render_vector_geometry and @a end_render_vector_geometry over a scope.
		 */
		class VectorGeometryScope :
				private boost::noncopyable
		{
		public:
			VectorGeometryScope(
					FeedbackOpenGLToQPainter &feedback_opengl_to_qpainter,
					GPlatesOpenGL::GLRenderer &renderer,
					unsigned int max_num_points,
					unsigned int max_num_lines,
					unsigned int max_num_triangles);

			~VectorGeometryScope();

			//! Opportunity to end rendering before the scope exits (when destructor is called).
			void
			end_render();

		private:
			FeedbackOpenGLToQPainter &d_feedback_opengl_to_qpainter;
			GPlatesOpenGL::GLRenderer &d_renderer;
			bool d_called_end_render;
		};


		/**
		 * Begins arbitrary rendering to an internal QImage of dimensions matching the
		 * paint device of the QPainter attached to @a renderer.
		 *
		 * This is not actually OpenGL feedback - instead it is arbitrary rendering that is
		 * captured in a QImage which is then sent to the QPainter attached to GLRenderer.
		 *
		 * @a max_point_size_and_line_width specifies the maximum line width and point size of any lines
		 * and/or points to be rendered into the image. This is only used if tiling is needed to render
		 * to the image, ie, if the main framebuffer is smaller than the image (paint device dimensions).
		 * The default value of zero can be used if no points or lines are rendered (eg, only rasters).
		 *
		 * NOTE: Rendering of tiles is performed using the current frame buffer which is either
		 * the main frame buffer or the currently bound frame buffer object (if one is bound).
		 * The contents of the frame buffer will be corrupted by the tile rendering so you should
		 * save/restore the frame buffer if you need to keep the (colour) frame buffer intact.
		 *
		 * @throws PreconditionViolationError if @a renderer was not set up with a QPainter.
		 */
		void
		begin_render_image(
				GPlatesOpenGL::GLRenderer &renderer,
				const double &max_point_size_and_line_width = 0);

		/**
		 * Begins a tile (sub-region) of the current image.
		 *
		 * The returned transform is an adjustment to the projection transform normally used to
		 * render to the image. The adjustment should be pre-multiplied with the normal projection
		 * transform and the result used as the actual projection transform.
		 * This ensures only the tile region of the view frustum is rendered to.
		 *
		 * @a image_tile_viewport and @a image_tile_scissor_rect are the rectangles
		 * specified internally to @a gl_viewport and @a gl_scissor, respectively.
		 * The viewport contains the tile border required to prevent clipping of wide lines and fat points.
		 * A stencil rectangle prevents rasterization of pixels outside the actual tile region.
		 * NOTE: You do not need to call @a gl_viewport or @a gl_scissor (they are done internally).
		 *
		 * If @a save_restore_state is true then the OpenGL state is restored at @a end_render_image_tile.
		 *
		 * Must be called inside a @a begin_render_image / @a end_render_image pair.
		 */
		GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type
		begin_render_image_tile(
				GPlatesOpenGL::GLRenderer &renderer,
				bool save_restore_state = true,
				GPlatesOpenGL::GLViewport *image_tile_viewport = NULL,
				GPlatesOpenGL::GLViewport *image_tile_scissor_rect = NULL);

		/**
		 * Ends the current tile (sub-region) of the current image.
		 *
		 * Returns true if another tile needs to be rendered in which case another
		 * @a begin_render_image_tile / @a end_render_image_tile pair must be rendered.
		 * For example:
		 *    tile_render.begin_render_image(...);
		 *    do
		 *    {
		 *        tile_render.begin_render_image_tile();
		 *        ... // Render scene.
		 *    } while (!tile_render.end_render_image_tile());
		 *    tile_render.end_render_image();
		 *
		 * Must be called inside a @a begin_render_image / @a end_render_image pair.
		 */
		bool
		end_render_image_tile(
				GPlatesOpenGL::GLRenderer &renderer);

		/**
		 * Ends arbitrary rendering to a QImage.
		 *
		 * This is not actually OpenGL feedback - instead it is arbitrary rendering that is
		 * captured in a QImage which is then sent to the QPainter attached to GLRenderer.
		 *
		 * @throws PreconditionViolationError if @a renderer was not set up with a QPainter.
		 */
		void
		end_render_image(
				GPlatesOpenGL::GLRenderer &renderer);

		/**
		 * RAII class to call @a begin_render_image and @a end_render_image over a scope.
		 */
		class ImageScope :
				private boost::noncopyable
		{
		public:
			ImageScope(
					FeedbackOpenGLToQPainter &feedback_opengl_to_qpainter,
					GPlatesOpenGL::GLRenderer &renderer,
					const double &max_point_size_and_line_width = 0);

			~ImageScope();

			/**
			 * Begins a tile of the current image - see 'begin_render_image_tile()'.
			 */
			GPlatesOpenGL::GLTransform::non_null_ptr_to_const_type
			begin_render_tile(
					bool save_restore_state = true,
					GPlatesOpenGL::GLViewport *image_tile_viewport = NULL,
					GPlatesOpenGL::GLViewport *image_tile_scissor_rect = NULL);

			/**
			 * Ends the current tile of the current image - see 'end_render_image_tile()'.
			 */
			bool
			end_render_tile();

			//! Opportunity to end rendering before the scope exits (when destructor is called).
			void
			end_render();

		private:
			FeedbackOpenGLToQPainter &d_feedback_opengl_to_qpainter;
			GPlatesOpenGL::GLRenderer &d_renderer;
			bool d_called_end_render_tile;
			bool d_called_end_render;
		};

	private:

		/**
		 * Used when rendering vector geometries.
		 */
		struct VectorRender
		{
			explicit
			VectorRender(
					int feedback_buffer_size_) :
				feedback_buffer(new GLfloat[feedback_buffer_size_]),
				feedback_buffer_size(feedback_buffer_size_)
			{  }

			boost::scoped_array<GLfloat> feedback_buffer;
			int feedback_buffer_size;
		};

		/**
		 * Used when performing arbitrary rendering to an image.
		 */
		struct ImageRender
		{
			explicit
			ImageRender(
					const QImage &image_,
					const GPlatesOpenGL::GLTileRender &tile_render_) :
				image(image_),
				tile_render(tile_render_),
				save_restore_tile_state(false)
			{  }

			QImage image;
			GPlatesOpenGL::GLTileRender tile_render;
			bool save_restore_tile_state;
		};


		boost::optional<VectorRender> d_vector_render;
		boost::optional<ImageRender> d_image_render;

	};
}

#endif // GPLATES_GUI_FEEDBACKOPENGLTOQPAINTER_H
