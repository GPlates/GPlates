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
#include <boost/scoped_array.hpp>

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
		 * Constructs a feedback buffer that supports the specified number of points, lines and triangles
		 * regardless of whether they will be clipped by the view frustum or not.
		 *
		 * Note that the lines are individual line segments (not polylines).
		 */
		FeedbackOpenGLToQPainter(
				unsigned int max_num_points,
				unsigned int max_num_lines,
				unsigned int max_num_triangles);


		/**
		 * Begins OpenGL feedback of (fixed-function pipeline) vector geometries.
		 */
		void
		begin_render_vector_geometry(
				GPlatesOpenGL::GLRenderer &renderer);

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
					GPlatesOpenGL::GLRenderer &renderer);

			~VectorGeometryScope();

			//! Opportunity to end rendering before the scope exits (when destructor is called).
			void
			end_render();

		private:
			FeedbackOpenGLToQPainter &d_feedback_opengl_to_qpainter;
			GPlatesOpenGL::GLRenderer &d_renderer;
			bool d_called_end_render;
		};

	private:

		boost::scoped_array<GLfloat> d_feedback_buffer;
		int d_feedback_buffer_size;

	};
}

#endif // GPLATES_GUI_FEEDBACKOPENGLTOQPAINTER_H
