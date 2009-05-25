/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2009 Geological Survey of Norway
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

#include <vector>
#include <iostream>

#include <QDebug>

#include <QPainter>
#include <QString>
#include <QSvgGenerator>
#include <QtOpenGL/qgl.h>

#include "qt-widgets/SceneView.h"
#include "OpenGLException.h"
#include "SvgExport.h"


namespace
{
	const int VERTEX_SIZE = 7;

	struct SvgExportException { };

	void
	clear_gl_errors()
	{	
		while (glGetError() != GL_NO_ERROR)
		{};
	}

	GLenum
	check_gl_errors(
		const char *message = "")
	{
		GLenum error = glGetError();
		if (error != GL_NO_ERROR)
		{
			std::cerr << message << ": openGL error: " << gluErrorString(error) << std::endl;
			throw GPlatesGui::OpenGLException(GPLATES_EXCEPTION_SOURCE,
					"OpenGL error in SvgExport");
		}
		return error;
	}

	struct vertex_data
	{
		GLfloat x;
		GLfloat y;
		GLfloat z;
		GLfloat red;
		GLfloat green;
		GLfloat blue;
		GLfloat alpha;
	};


	void
	fill_vertex_data_from_buffer(
			vertex_data *vertex,
			std::vector<GLfloat>::const_iterator position)
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

	/**
	 * Try drawing to the opengl feedback buffer, and return the number of items
	 * in the buffer. If the buffer was not large enough, a negative number
	 * is returned.
	 */
	GLint
	draw_to_feedback_buffer(
			std::vector<GLfloat> &feedback_buffer,
			//GPlatesQtWidgets::GlobeCanvas *canvas)
			GPlatesQtWidgets::SceneView *scene_view)
	{

		clear_gl_errors();

		// Using the GL_3D_COLOR flag in the glFeedbackBuffer flag will
		// tell openGL to return data in the form (x,y,z,k) where
		// k is the number of items required to describe the colour.
		// In RGBA mode, k will be 4.
		// Thus we will have a total of 7 (float) values for each item.
		// See, for example:
		// http://www.glprogramming.com/red/chapter13.html

		// FIXME:  Assert that the size is > 0, or else feedback_buffer[0] will not exist.
		if (feedback_buffer.size() == 0)
		{
			throw SvgExportException();
		}


		glFeedbackBuffer(static_cast<GLint>(feedback_buffer.size()), GL_3D_COLOR,
				&feedback_buffer[0]);

		// According to http://www.glprogramming.com/red/chapter13.html#name1 , section
		// "Selection", sub-section "The Basic Steps", the return value of glRenderMode has
		// meaning only if the current mode (ie, not the parameter) is either GL_SELECT or
		// GL_FEEDBACK.
		//
		// In fact, according to http://www.glprogramming.com/red/chapter13.html#name2 ,
		// section "Feedback":
		// "For this step, you can ignore the value returned by glRenderMode()."
		glRenderMode(GL_FEEDBACK);

		check_gl_errors("After glRenderMode(GL_FEEDBACK)");

		clear_gl_errors();

		scene_view->draw_svg_output();

		check_gl_errors("After scene_view->draw_svg_output");

		clear_gl_errors();

		GLint num_items = glRenderMode(GL_RENDER);

		check_gl_errors("After glRenderMode(GL_RENDER)");

		// According to http://www.glprogramming.com/red/chapter13.html#name1 , section
		// "Selection", sub-section "The Basic Steps", a negative value means that the
		// array has overflowed.
		if (num_items < 0) {
			// do something about this
			//std::cerr << "Negative value returned from glRenderMode(GL_RENDER)." <<  std::endl;
		}

		return num_items;
	}


	/**
	 * Go through the buffer and count how many of the various token types
	 * we have, and send them to std::cerr. Just out of interest, like.
	 */
	void
	analyse_feedback_buffer(
			const std::vector<GLfloat> &feedback_buffer)
	{
		std::vector<GLfloat>::size_type count = 0;
		int token;

		// type_count keeps a count of the different token types.
		unsigned type_count[8];
		for (unsigned i = 0; i < 8; i++) {
			type_count[i] = 0;
		}

		while (count < feedback_buffer.size()) {
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
				//	std::cerr << "hmmm unrecognised token" << std::endl;
					break;
			} // switch
		} // while

		// show how many of the different token types we found:
		for(unsigned i = 0; i < 8; i++){
			std::cerr << type_count[i] << " ";
		}
		std::cout << std::endl;
	}


	/**
	 * Go throught the buffer to establish the bounding box, so that we can use this
	 * to centre the svg image nicely in the file.
	 */
	QRectF
	find_bounding_box(
			const std::vector<GLfloat> &buffer)
	{
		int token;
		int num_vertices;

		std::vector<GLfloat>::const_iterator buffer_position = buffer.begin();
		std::vector<GLfloat>::const_iterator buffer_end = buffer.end();

		vertex_data vertex;
		vertex_data* vertex_ptr = &vertex;

		QPolygonF points;
		QPainterPath lines;

		//std::cerr << "entering find_bounding_box" << std::endl;
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
					for(int i = 1; i < num_vertices; i++){
						buffer_position +=VERTEX_SIZE;
						fill_vertex_data_from_buffer(vertex_ptr, buffer_position);
						lines.lineTo(vertex.x, -(vertex.y));
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
					//std::cerr << "unrecognised token" << std::endl;
					break;
			} // switch
		
		} // while


		QRectF lines_bounding_rect = lines.boundingRect();
		QRectF points_bounding_rect = points.boundingRect();

		QRectF result = lines_bounding_rect.unite(points_bounding_rect);

		return result;
	}


	/**
	 * Go through the buffer and interpret the points/lines as Qt geometrical items,
	 * and send them to the QPainter. 
	 */
	void
	draw_to_svg_file(
			QString filename,
			const std::vector<GLfloat> &feedback_buffer)
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
		int num_vertices;
		int num_items = 0;

		std::vector<GLfloat>::const_iterator buffer_position = feedback_buffer.begin();
		std::vector<GLfloat>::const_iterator buffer_end = feedback_buffer.end();

		QColor colour = QColor(Qt::black);
		QColor line_colour;

		QPolygonF line;
		QPointF point;

		QPointF last_point(10000.0, 10000.0);
		QPointF first_point_on_line, second_point_on_line;

		vertex_data vertex;
		vertex_data* vertex_ptr = &vertex;

		// Finding the bounding box, and deriving an offset from that, allows me to 
		// centre the image nicely in the SVG file. The offset is applied to each
		// point as we come across them.
		QRectF bounding_box = find_bounding_box(feedback_buffer);
		QRect rbox = bounding_box.toRect();
		QPointF offset = -bounding_box.topLeft();

		QSvgGenerator* svg_generator = new QSvgGenerator;

		if (svg_generator == NULL) {
			throw (SvgExportException() );
		}

		svg_generator->setSize(rbox.size());
		svg_generator->setFileName(filename);

		QPainter painter(svg_generator);

		// We don't have any white features any more, so I'm removing this
		// light-blue background. The background will be white.
#if 0
		// Add a non-white background rectangle so that white-coloured features show up. 
		QRectF background_rectangle = bounding_box;
		background_rectangle.translate(offset);
		painter.fillRect(background_rectangle,QColor(230,230,255));
#endif

		painter.setPen(colour);

		while (buffer_position != buffer_end) {

			token = static_cast<int>(*buffer_position);
			buffer_position++;

			switch(token) {

				// Each point is sent directly to the QPainter with its own colour.
				case GL_POINT_TOKEN:

					fill_vertex_data_from_buffer(vertex_ptr, buffer_position);
					point.setX(vertex.x);
					point.setY(-(vertex.y));
		
					point += offset;
					colour.setRgbF(
							vertex.red,
							vertex.green,
							vertex.blue,
							vertex.alpha);
					painter.setPen(colour);
					painter.drawPoint(point);
					buffer_position += VERTEX_SIZE;
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
					colour.setRgbF(
							vertex.red,
							vertex.green,
							vertex.blue,
							vertex.alpha);
					first_point_on_line.setX(vertex.x);
					first_point_on_line.setY(-(vertex.y));
					first_point_on_line += offset;			

					buffer_position += VERTEX_SIZE;
					fill_vertex_data_from_buffer(vertex_ptr, buffer_position);
					second_point_on_line.setX(vertex.x);
					second_point_on_line.setY(-(vertex.y));
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
					break;

				// Currently we do not draw anything to opengl as a polygon - any imported polygons 
				// (e.g from shapefiles) are rendered as line strings.  So there is no Qt painting
				// business required here. We shouldn't really encounter *any* polygons then, but because
				// I am paranoid, I will leave in the code which at least steps over the polygon
				// data correctly. 
				case GL_POLYGON_TOKEN:
					num_vertices = static_cast<int>(*buffer_position);
					buffer_position++;
					buffer_position += (VERTEX_SIZE*num_vertices);
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
				//	std::cerr << "hmmm unrecognised token" << std::endl;
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

		painter.end();

		delete svg_generator;
	}
}

bool
GPlatesGui::SvgExport::create_svg_output(
	QString filename,
	GPlatesQtWidgets::SceneView *scene_view)
{
	const unsigned long MAX_BUFFER_SIZE = static_cast<unsigned long>(1e7);

	unsigned long buffer_size = 100000;

	GLint filled_size = 0;

	// Rather than allocating an array manually, we'll use a std::vector instance on the stack.
	// This has the benefits:
	//  (i) It's easier to resize (and keep track of the correct size).
	//  (ii) We won't accidentally leak memory (particularly in the event of an exception).
	//
	// According to Meyers01, "Effective STL", Item 16 "Know how to pass vector and string data
	// to legacy APIs", the elements in a vector are guaranteed by the C++ Standard to be
	// stored in contiguous memory, just like the elements in an array.
	std::vector<GLfloat> feedback_buffer(buffer_size);

	SvgExport svg_export;

	try{

		filled_size = draw_to_feedback_buffer(feedback_buffer,scene_view);

		// To minimise the size of the allocated buffer, we begin with a buffer size of 10000,
		// and check if it's big enough for our view. If not, we try a bigger value, up to a
		// limit of MAX_BUFFER_SIZE. 
		while ( ( filled_size < 0) &&
			(( buffer_size*=10) <= MAX_BUFFER_SIZE)) {

				feedback_buffer.resize(buffer_size);
				filled_size = draw_to_feedback_buffer(feedback_buffer,scene_view);

		} 

		if (filled_size < 0) {
			//std::cerr << "Feedback buffer not filled." << std::endl;
			return false;
		}

		// If we have a size we are happy with, resize the vector, and send it
		// to the svg function. 
		feedback_buffer.resize(filled_size);	

		//std::cerr << "Resized buffer size: " << feedback_buffer.size() << std::endl;

		//analyse_feedback_buffer(feedback_buffer);

		draw_to_svg_file(filename, feedback_buffer);

	}
	catch (const OpenGLException &e){
		e.write(std::cerr);
		return false;
	}
	catch (const SvgExportException &) {
		std::cerr << "An exception was caught in SvgExport." << std::endl;
		return false;
	}


	return true;
}


GPlatesGui::SvgExport::~SvgExport()
{
	glRenderMode(GL_RENDER);
	clear_gl_errors();
}
