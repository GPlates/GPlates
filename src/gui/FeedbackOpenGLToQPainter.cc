/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2009 Geological Survey of Norway
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

#include <exception>

#include <QDebug>
#include <QPainter>
#include <QString>

#include "FeedbackOpenGLToQPainter.h"

#include "opengl/GLRenderer.h"
#include "opengl/GLUtils.h"
#include "opengl/OpenGLException.h"


namespace
{
	struct Vertex
	{
		GLfloat x;
		GLfloat y;
		GLfloat z;
		GLfloat red;
		GLfloat green;
		GLfloat blue;
		GLfloat alpha;
	};

	const int VERTEX_SIZE = 7;


	void
	fill_vertex_data_from_buffer(
			Vertex *vertex,
			const GLfloat *position)
	{
		vertex->x = *position;
		position++;
		vertex->y = *position;
		position++;
		vertex->z = *position;
		position++;
		vertex->red = *position;
		position++;
		vertex->green = *position;
		position++;
		vertex->blue = *position;
		position++;
		vertex->alpha = *position;
	}

	void
	parse_and_draw_polygon_vertices(
		const GLfloat *&buffer_position,
		const QPointF &offset,		
		QPainter *painter,
		int paint_device_height)
	{
		int num_vertices = static_cast<int>(*buffer_position);
		buffer_position++;
		QColor colour;		
		QPolygonF polygon;
		//qDebug("Polygon: ");
		for (int i=0 ; i < num_vertices ; ++i)
		{
			Vertex vertex;
			Vertex* vertex_ptr = &vertex;
			fill_vertex_data_from_buffer(vertex_ptr,buffer_position);		
			colour.setRgbF(
				vertex.red,
				vertex.green,
				vertex.blue,
				vertex.alpha);				
			// Note that OpenGL and Qt y-axes are the reverse of each other...
			QPointF point(vertex.x, paint_device_height - vertex.y);
			point += offset;
			polygon << point;
			buffer_position += VERTEX_SIZE;
			//qDebug() << point.x() << "," << point.y();
		}


		// Draw the polygon, filled with the last grabbed colour, and with no outline. 

		painter->setPen(Qt::NoPen);
		painter->setBrush(colour);
		painter->drawPolygon(polygon);
	}	


	/**
	 * Go through the buffer and count how many of the various token types
	 * we have, and send them to std::cerr. Just out of interest, like.
	 */
	void
	analyse_feedback_buffer(
			const GLfloat *feedback_buffer,
			unsigned int feedback_buffer_size)
	{
		unsigned int count = 0;
		int token;

		// type_count keeps a count of the different token types.
		unsigned type_count[8];
		for (unsigned i = 0; i < 8; i++) {
			type_count[i] = 0;
		}

		while (count < feedback_buffer_size) {
			token = static_cast<int>(feedback_buffer[count]);
			count++;
			switch(token){
				case GL_POINT_TOKEN:
					type_count[0]++;
					break;
				case GL_LINE_TOKEN:
					type_count[1]++;
					break;
				case GL_LINE_RESET_TOKEN:
					type_count[2]++;
					break;
				case GL_POLYGON_TOKEN:
					type_count[3]++;
					break;
				case GL_BITMAP_TOKEN:
					type_count[4]++;
					break;
				case GL_DRAW_PIXEL_TOKEN:
					type_count[5]++;
					break;
				case GL_COPY_PIXEL_TOKEN:
					type_count[6]++;
					break;
				case GL_PASS_THROUGH_TOKEN:
					type_count[7]++;
					break;
				default:
					//qWarning() << "FeedbackOpenGLToQPainter: unrecognised token";
					break;
			} // switch
		} // while

		// show how many of the different token types we found:
		for(unsigned i = 0; i < 8; i++)
		{
			qDebug("%d ", type_count[i]);
		}
		qDebug() << endl;
	}


	/**
	 * Go through the buffer to establish the bounding box.
	 */
	QRectF
	find_bounding_box(
			const GLfloat *feedback_buffer,
			unsigned int feedback_buffer_size)
	{
		int token;
		int num_vertices;

		const GLfloat *buffer_position = feedback_buffer;
		const GLfloat *buffer_end = feedback_buffer + feedback_buffer_size;

		Vertex vertex;
		Vertex* vertex_ptr = &vertex;

		QPolygonF points;
		QPainterPath lines;

		while (buffer_position != buffer_end) {
			token = static_cast<int>(*buffer_position);
			buffer_position++;
			switch(token){
				case GL_POINT_TOKEN:
					fill_vertex_data_from_buffer(vertex_ptr, buffer_position);
					points.push_back(QPointF(vertex.x, -(vertex.y)));
					buffer_position += VERTEX_SIZE;
					break;

				case GL_LINE_TOKEN:
					fill_vertex_data_from_buffer(vertex_ptr, buffer_position);
					lines.moveTo(vertex.x, -(vertex.y));
					buffer_position += VERTEX_SIZE;
					fill_vertex_data_from_buffer(vertex_ptr, buffer_position);
					lines.lineTo(vertex.x, -(vertex.y));
					buffer_position += VERTEX_SIZE;
					break;

				case GL_LINE_RESET_TOKEN:
					fill_vertex_data_from_buffer(vertex_ptr, buffer_position);
					lines.moveTo(vertex.x, -(vertex.y));
					buffer_position += VERTEX_SIZE;
					fill_vertex_data_from_buffer(vertex_ptr, buffer_position);
					lines.lineTo(vertex.x, -(vertex.y));
					buffer_position += VERTEX_SIZE;
					break;
				case GL_POLYGON_TOKEN:
					num_vertices = static_cast<int>(*buffer_position);
					buffer_position++;
					fill_vertex_data_from_buffer(vertex_ptr, buffer_position);
					lines.moveTo(vertex.x, -(vertex.y));
					buffer_position += VERTEX_SIZE;
					for(int i = 1; i < num_vertices; i++){
						fill_vertex_data_from_buffer(vertex_ptr, buffer_position);
						lines.lineTo(vertex.x, -(vertex.y));
						buffer_position +=VERTEX_SIZE;
					}
					break;
				case GL_BITMAP_TOKEN:
					buffer_position += VERTEX_SIZE;
					break;
				case GL_DRAW_PIXEL_TOKEN:
					buffer_position += VERTEX_SIZE;
					break;
				case GL_COPY_PIXEL_TOKEN:
					buffer_position += VERTEX_SIZE;
					break;
				case GL_PASS_THROUGH_TOKEN:
					buffer_position++;
					break;
				default:
					//qWarning() << "FeedbackOpenGLToQPainter: unrecognised token";
					break;
			} // switch
		
		} // while


		QRectF lines_bounding_rect = lines.boundingRect();
		QRectF points_bounding_rect = points.boundingRect();

		QRectF result = lines_bounding_rect.unite(points_bounding_rect);

		return result;
	}


	/**
	 * Go through the feedback buffer and interpret the points/lines as Qt geometrical items,
	 * and send them to the QPainter. 
	 */
	void
	draw_feedback_primitives_to_qpainter(
			QPainter &painter,
			const GLfloat *feedback_buffer,
			unsigned int feedback_buffer_size)
	{
		// Each point encountered in the feedback buffer is converted to a QPointF and 
		// draw using QPainter::drawPoint.

		// Each polyline encountered is converted to a QPolygonF and drawn using QPainter::drawPolyline

		// One circumstance in which we may run into problems with the following treatment, is if we
		// start a new feature at the same coordinate as the previous feature. In this case they will
		// be exported as the same QPolygonF. 

		// One way around this problem would be to render each feature separately to its own GL_FEEDBACK
		// buffer. 

		int token;
		int num_items = 0;

		const GLfloat *buffer_position = feedback_buffer;
		const GLfloat *buffer_end = feedback_buffer + feedback_buffer_size;

		QColor colour = QColor(Qt::black);
		QColor line_colour;

		QPolygonF line;
		QPointF point;

		QPointF last_point(10000.0, 10000.0);
		QPointF first_point_on_line, second_point_on_line;

		Vertex vertex;
		Vertex* vertex_ptr = &vertex;

		// NOTE: We no longer try to centre the geometries in the SVG file.
		// The SVG output is an exact representation of the globe/map viewport.
		// We can provide view controls somewhere in the GUI to centre the globe/map in the
		// viewport if desired, but we don't adjust coordinates on export to SVG.
#if 1
		QPointF offset;
#else
		// Finding the bounding box, and deriving an offset from that, allows me to 
		// centre the image nicely in the SVG file. The offset is applied to each
		// point as we come across them.
		QRectF bounding_box = find_bounding_box(feedback_buffer, feedback_buffer_size);
		QRect rbox = bounding_box.toRect();
		QPointF offset = -bounding_box.topLeft();
#endif

		const int paint_device_height = painter.device()->height();

		painter.setPen(colour);

		while (buffer_position != buffer_end) {

			token = static_cast<int>(*buffer_position);
			buffer_position++;

			switch(token) {

				// Each point is sent directly to the QPainter with its own colour.
				case GL_POINT_TOKEN:

					fill_vertex_data_from_buffer(vertex_ptr, buffer_position);
					point.setX(vertex.x);
					// Note that OpenGL and Qt y-axes are the reverse of each other...
					point.setY(paint_device_height - vertex.y);
		
					point += offset;
					colour.setRgbF(
							vertex.red,
							vertex.green,
							vertex.blue,
							vertex.alpha);
					painter.setPen(colour);
					painter.drawPoint(point);
					buffer_position += VERTEX_SIZE;
					//qDebug() << "Point: " << point.x() << ", " << point.y();
					break;

				// Although GL_LINE_RESET_TOKEN tells us when a new line was begun,
				// (which would tell us when to begin a new QPolygonF and send the previous
				// one to the painter), this does not apply when we have zoomed in and clipped 
				// off the edges of the image. If a line goes off the edge of the visible screen, 
				// for example, there will not necessarily be a GL_LINE_RESET_TOKEN. So to determine 
				// when a new QPolygonF is required, I am checking if the current point has changed
				// from the previous point. And so both the GL_LINE_TOKEN and GL_LINE_RESET_TOKEN
				// cases are treated in the same way.
				case GL_LINE_TOKEN:
				case GL_LINE_RESET_TOKEN:

					fill_vertex_data_from_buffer(vertex_ptr, buffer_position);
					first_point_on_line.setX(vertex.x);
					// Note that OpenGL and Qt y-axes are the reverse of each other...
					first_point_on_line.setY(paint_device_height - vertex.y);
					first_point_on_line += offset;			

					buffer_position += VERTEX_SIZE;
					fill_vertex_data_from_buffer(vertex_ptr, buffer_position);
					colour.setRgbF(
							vertex.red,
							vertex.green,
							vertex.blue,
							vertex.alpha);
					second_point_on_line.setX(vertex.x);
					// Note that OpenGL and Qt y-axes are the reverse of each other...
					second_point_on_line.setY(paint_device_height - vertex.y);
					second_point_on_line += offset;

					if (first_point_on_line != last_point) {
						// We will start a new line. If we also have an old line, we'll send it
						// to the QPainter, and then clear it.
						if (!line.empty()){
							painter.setPen(line_colour);
							painter.drawPolyline(line);
							line.clear();
						}
						line.push_back(first_point_on_line);
						line_colour = colour;
					}

					// If we are starting a new line, we also need to add the second point to the
					// QPolygon.
					// If we are not starting a new line, then we add only the second point to the 
					// QPolygon.
					line.push_back(second_point_on_line);
					last_point = second_point_on_line;
					
					// move along now please.
					buffer_position += VERTEX_SIZE;
					//qDebug() << "Line: " << first_point_on_line.x() << ", " << first_point_on_line.y()
					//	<< "  -->  " << second_point_on_line.x() << ", " << second_point_on_line.y();
					break;

				// Currently we do not draw anything to opengl as a polygon - any imported polygons 
				// (e.g from shapefiles) are rendered as line strings.  So there is no Qt painting
				// business required here. We shouldn't really encounter *any* polygons then, but because
				// I am paranoid, I will leave in the code which at least steps over the polygon
				// data correctly. 
				case GL_POLYGON_TOKEN:
					// Rendered arrow heads are drawn as triangle_fans, which are made up of triangles,
					// which are polygons.
					parse_and_draw_polygon_vertices(buffer_position, offset, &painter, paint_device_height);
					break;
				case GL_BITMAP_TOKEN:
					buffer_position += VERTEX_SIZE;
					break;
				case GL_DRAW_PIXEL_TOKEN:
					buffer_position += VERTEX_SIZE;
					break;
				case GL_COPY_PIXEL_TOKEN:
					buffer_position += VERTEX_SIZE;
					break;
				case GL_PASS_THROUGH_TOKEN:
					buffer_position++;
					break;
				default:
					qWarning() << "FeedbackOpenGLToQPainter: unrecognised token";
					break;
			} // switch
			num_items++;
		} // while


		// draw the last line
		if ( ! line.empty()) {
			painter.setPen(line_colour);
			painter.drawPolyline(line);
			line.clear();
		}
	}
}


GPlatesGui::FeedbackOpenGLToQPainter::FeedbackOpenGLToQPainter(
		unsigned int max_num_points,
		unsigned int max_num_lines,
		unsigned int max_num_triangles) :
	d_feedback_buffer_size(0)
{
	// The '1' for each point/line/triangle is for the feedback token.
	const int feedback_buffer_size =
			// Each point has at most one vertex...
			(1 + VERTEX_SIZE) * max_num_points +
			// Each (clipped or unclipped) line has at most two vertices...
			(1 + 2 * VERTEX_SIZE) * max_num_lines +
			// Each triangle produces at most 4 clipped triangles (each with 3 vertices)...
			(1 + 3 * 4 * VERTEX_SIZE) * max_num_triangles;

	d_feedback_buffer.reset(new GLfloat[feedback_buffer_size]);
	d_feedback_buffer_size = feedback_buffer_size;
}


void
GPlatesGui::FeedbackOpenGLToQPainter::begin_render_vector_geometry(
		GPlatesOpenGL::GLRenderer &renderer)
{
	// Since we're about to directly call OpenGL functions (instead of using GLRenderer) we
	// need to make sure the GLRenderer state is flushed to OpenGL.
	renderer.apply_current_state_to_opengl();

	// Specify our feedback buffer.
	//
	// Using the GL_3D_COLOR flag in the glFeedbackBuffer flag will
	// tell openGL to return data in the form (x,y,z,k) where
	// k is the number of items required to describe the colour.
	// In RGBA mode, k will be 4.
	// Thus we will have a total of 7 (float) values for each item.
	// See, for example:
	// http://www.glprogramming.com/red/chapter13.html
	//
	// TODO: Move this function to 'GLRenderer'.
	glFeedbackBuffer(d_feedback_buffer_size, GL_3D_COLOR, d_feedback_buffer.get());

	// Specify OpenGL feedback mode.
	//
	// TODO: Move this function to 'GLRenderer'.
	//
	// According to http://www.glprogramming.com/red/chapter13.html#name1 , section
	// "Selection", sub-section "The Basic Steps", the return value of glRenderMode has
	// meaning only if the current mode (ie, not the parameter) is either GL_SELECT or
	// GL_FEEDBACK.
	//
	// In fact, according to http://www.glprogramming.com/red/chapter13.html#name2 ,
	// section "Feedback":
	// "For this step, you can ignore the value returned by glRenderMode()."
	glRenderMode(GL_FEEDBACK);

	GPlatesOpenGL::GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesGui::FeedbackOpenGLToQPainter::end_render_vector_geometry(
		GPlatesOpenGL::GLRenderer &renderer)
{
	// Since we're about to directly call OpenGL functions (instead of using GLRenderer) we
	// need to make sure the GLRenderer state is flushed to OpenGL.
	renderer.apply_current_state_to_opengl();

	// Return to regular rendering mode.
	//
	// TODO: Move this function to 'GLRenderer'.
	const GLint num_feedback_items = glRenderMode(GL_RENDER);

	GPlatesOpenGL::GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);

	// According to http://www.glprogramming.com/red/chapter13.html#name1 , section
	// "Selection", sub-section "The Basic Steps", a negative value means that the
	// array has overflowed.
	GPlatesGlobal::Assert<GPlatesOpenGL::OpenGLException>(
			num_feedback_items >= 0,
			GPLATES_ASSERTION_SOURCE,
			"OpenGL feedback buffer overflowed.");

	// Suspend rendering with 'GLRenderer' so we can resume painting with 'QPainter'.
	// At scope exit we can resume rendering with 'GLRenderer'.
	GPlatesOpenGL::GLRenderer::QPainterBlockScope qpainter_block_scope(renderer);
	boost::optional<QPainter &> qpainter = qpainter_block_scope.get_qpainter();
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			qpainter,
			GPLATES_ASSERTION_SOURCE);

	// Set the identity world transform since our feedback geometry data is in *window* coordinates
	// and we don't want it transformed by the current world transform.
	qpainter->setWorldTransform(QTransform()/*identity*/);

	// Draw the feedback primitives to the QPainter.
	draw_feedback_primitives_to_qpainter(
			qpainter.get(),
			d_feedback_buffer.get(),
			num_feedback_items);
}


GPlatesGui::FeedbackOpenGLToQPainter::VectorGeometryScope::VectorGeometryScope(
		FeedbackOpenGLToQPainter &feedback_opengl_to_qpainter,
		GPlatesOpenGL::GLRenderer &renderer) :
	d_feedback_opengl_to_qpainter(feedback_opengl_to_qpainter),
	d_renderer(renderer),
	d_called_end_render(false)
{
	d_feedback_opengl_to_qpainter.begin_render_vector_geometry(renderer);
}


GPlatesGui::FeedbackOpenGLToQPainter::VectorGeometryScope::~VectorGeometryScope()
{
	if (!d_called_end_render)
	{
		// If an exception is thrown then unfortunately we have to lump it since exceptions cannot leave destructors.
		// But we log the exception and the location it was emitted.
		try
		{
			d_feedback_opengl_to_qpainter.end_render_vector_geometry(d_renderer);
		}
		catch (std::exception &exc)
		{
			qWarning() << "FeedbackOpenGLToQPainter: exception thrown during vector geometry scope: " << exc.what();
		}
		catch (...)
		{
			qWarning() << "FeedbackOpenGLToQPainter: exception thrown during vector geometry scope: Unknown error";
		}
	}
}


void
GPlatesGui::FeedbackOpenGLToQPainter::VectorGeometryScope::end_render()
{
	if (!d_called_end_render)
	{
		d_feedback_opengl_to_qpainter.end_render_vector_geometry(d_renderer);
		d_called_end_render = true;
	}
}
