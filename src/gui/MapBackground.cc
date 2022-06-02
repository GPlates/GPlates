/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <QDebug>

#include "MapBackground.h"

#include "Colour.h"
#include "ProjectionException.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "opengl/GL.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLStreamPrimitives.h"
#include "opengl/GLVertexArray.h"
#include "opengl/GLVertexUtils.h"
#include "opengl/GLViewProjection.h"

#include "presentation/ViewState.h"

#include "utils/CallStackTracker.h"


namespace
{
	// Vertex and fragment shader source code to render map background.
	const char *VERTEX_SHADER_SOURCE =
		R"(
			uniform mat4 view_projection;
			
			layout(location = 0) in vec4 position;
			
			void main (void)
			{
				gl_Position = view_projection * position;
			}
		)";
	const char *FRAGMENT_SHADER_SOURCE =
		R"(
			uniform vec4 background_colour;
			
			layout(location = 0) out vec4 colour;

			void main (void)
			{
				colour = background_colour;
			}
		)";

	// Vertex stream.
	typedef GPlatesOpenGL::GLVertexUtils::Vertex vertex_type;
	typedef GLuint vertex_element_type;
	typedef GPlatesOpenGL::GLDynamicStreamPrimitives<vertex_type, vertex_element_type> stream_primitives_type;

	// Projection coordinates.
	typedef std::pair<double, double> projection_coord_type;

	/**
	 * The number of line segments along a line of latitude.
	 */
	const int LINE_OF_LATITUDE_NUM_SEGMENTS = 100;

	/**
	 * The number of line segments along a line of longitude.
	 *
	 * This is more than for lines of latitude because a line of longitude can curve in the map projection
	 * (whereas lines of latitude are straight, for all the map projections we currently support anyway).
	 */
	const int LINE_OF_LONGITUDE_NUM_SEGMENTS = 400;

	/**
	 * The angular spacing a points along a line of latitude.
	 */
	const double LINE_OF_LATITUDE_DELTA_LONGITUDE = 360.0 / LINE_OF_LATITUDE_NUM_SEGMENTS;

	/**
	 * The angular spacing a points along a line of longitude.
	 */
	const double LINE_OF_LONGITUDE_DELTA_LATITUDE = 180.0 / LINE_OF_LONGITUDE_NUM_SEGMENTS;


	/**
	 * Projects the specified latitude/longitude using the specified map projection.
	 */
	projection_coord_type
	project_lat_lon(
			double lat,
			double lon,
			const GPlatesGui::MapProjection &projection)
	{
		projection.forward_transform(lon, lat);
		return projection_coord_type(lon, lat);
	}


	void
	stream_background(
			stream_primitives_type &stream,
			const GPlatesGui::MapProjection &projection)
	{
		stream_primitives_type::Primitives triangle_mesh(stream);

		const bool ok = triangle_mesh.begin_primitive(
				(LINE_OF_LONGITUDE_NUM_SEGMENTS + 1) * (LINE_OF_LATITUDE_NUM_SEGMENTS + 1)/*max_num_vertices*/,
				3 * 2 * LINE_OF_LONGITUDE_NUM_SEGMENTS * LINE_OF_LATITUDE_NUM_SEGMENTS/*max_num_vertex_elements*/);

		// Since we added vertices/indices to a std::vector we shouldn't have run out of space.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				ok,
				GPLATES_ASSERTION_SOURCE);

		const double lat_0 = -90;
		const double lon_0 = projection.central_meridian() - 180;

		// Project the vertices using the map projection.
		try
		{
			// Add the mesh vertices.
			for (int y = 0; y < LINE_OF_LONGITUDE_NUM_SEGMENTS + 1; ++y)
			{
				GPlatesMaths::real_t lat;
				// Stay slightly inside the map boundary to avoid any potential map projection
				// issues (eg, due to numerical precision).
				if (y == 0)
				{
					lat = lat_0 + 1e-8;
				}
				else if (y == LINE_OF_LONGITUDE_NUM_SEGMENTS)
				{
					lat = lat_0 + 180 - 1e-8;
				}
				else
				{
					lat = lat_0 + y * LINE_OF_LONGITUDE_DELTA_LATITUDE;
				}

				for (int x = 0; x < LINE_OF_LATITUDE_NUM_SEGMENTS + 1; ++x)
				{
					GPlatesMaths::real_t lon;
					// Stay slightly inside the map boundary to avoid any potential map projection
					// issues (eg, due to numerical precision).
					if (x == 0)
					{
						lon = lon_0 + 1e-8;
					}
					else if (x == LINE_OF_LATITUDE_NUM_SEGMENTS)
					{
						lon = lon_0 + 360 - 1e-8;
					}
					else
					{
						lon = lon_0 + x * LINE_OF_LATITUDE_DELTA_LONGITUDE;
					}

					const projection_coord_type projected_coord =
							project_lat_lon(lat.dval(), lon.dval(), projection);
					const vertex_type vertex(projected_coord.first, projected_coord.second, 0/*z*/);

					triangle_mesh.add_vertex(vertex);
				}
			}
		}
		catch (const GPlatesGui::ProjectionException &exc)
		{
			// Ignore exception.
			qWarning() << exc;
		}

		// Add the mesh vertex elements.
		for (int y = 0; y < LINE_OF_LONGITUDE_NUM_SEGMENTS; ++y)
		{
			for (int x = 0; x < LINE_OF_LATITUDE_NUM_SEGMENTS; ++x)
			{
				// First triangle of current quad.
				triangle_mesh.add_vertex_element(y * (LINE_OF_LATITUDE_NUM_SEGMENTS + 1) + x);
				triangle_mesh.add_vertex_element(y * (LINE_OF_LATITUDE_NUM_SEGMENTS + 1) + (x + 1));
				triangle_mesh.add_vertex_element((y + 1) * (LINE_OF_LATITUDE_NUM_SEGMENTS + 1) + x);

				// Second triangle of current quad.
				triangle_mesh.add_vertex_element((y + 1) * (LINE_OF_LATITUDE_NUM_SEGMENTS + 1) + (x + 1));
				triangle_mesh.add_vertex_element((y + 1) * (LINE_OF_LATITUDE_NUM_SEGMENTS + 1) + x);
				triangle_mesh.add_vertex_element(y * (LINE_OF_LATITUDE_NUM_SEGMENTS + 1) + (x + 1));
			}
		}

		triangle_mesh.end_primitive();
	}


	void
	create_background(
			std::vector<vertex_type> &vertices,
			std::vector<vertex_element_type> &vertex_elements,
			const GPlatesGui::MapProjection &map_projection)
	{
		stream_primitives_type stream;

		stream_primitives_type::StreamTarget stream_target(stream);
		stream_target.start_streaming(
				boost::in_place(boost::ref(vertices)),
				boost::in_place(boost::ref(vertex_elements)));

		stream_background(stream, map_projection);

		stream_target.stop_streaming();
	}


	void
	load_background(
			GPlatesOpenGL::GL &gl,
			GPlatesOpenGL::GLVertexArray::shared_ptr_type vertex_array,
			GPlatesOpenGL::GLBuffer::shared_ptr_type vertex_buffer,
			GPlatesOpenGL::GLBuffer::shared_ptr_type vertex_element_buffer,
			const std::vector<vertex_type> &vertices,
			const std::vector<vertex_element_type> &vertex_elements)
	{
		// Bind vertex array object.
		gl.BindVertexArray(vertex_array);

		// Bind vertex element buffer object to currently bound vertex array object.
		gl.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_element_buffer);

		// Transfer vertex element data to currently bound vertex element buffer object.
		glBufferData(
				GL_ELEMENT_ARRAY_BUFFER,
				vertex_elements.size() * sizeof(vertex_elements[0]),
				vertex_elements.data(),
				GL_STATIC_DRAW);

		// Bind vertex buffer object (used by vertex attribute arrays, not vertex array object).
		gl.BindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		// Transfer vertex data to currently bound vertex buffer object.
		glBufferData(
				GL_ARRAY_BUFFER,
				vertices.size() * sizeof(vertices[0]),
				vertices.data(),
				GL_STATIC_DRAW);

		// Specify vertex attributes (position) in currently bound vertex buffer object.
		// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
		// to currently bound vertex array object.
		gl.EnableVertexAttribArray(0);
		gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_type), BUFFER_OFFSET(vertex_type, x));
	}


	void
	compile_link_program(
			GPlatesOpenGL::GL &gl,
			GPlatesOpenGL::GLProgram::shared_ptr_type program)
	{
		// Add this scope to the call stack trace printed if exception thrown in this scope (eg, failure to compile/link shader).
		TRACK_CALL_STACK();

		// Vertex shader source.
		GPlatesOpenGL::GLShaderSource vertex_shader_source;
		vertex_shader_source.add_code_segment(VERTEX_SHADER_SOURCE);

		// Vertex shader.
		GPlatesOpenGL::GLShader::shared_ptr_type vertex_shader = GPlatesOpenGL::GLShader::create(gl, GL_VERTEX_SHADER);
		vertex_shader->shader_source(vertex_shader_source);
		vertex_shader->compile_shader();

		// Fragment shader source.
		GPlatesOpenGL::GLShaderSource fragment_shader_source;
		fragment_shader_source.add_code_segment(FRAGMENT_SHADER_SOURCE);

		// Fragment shader.
		GPlatesOpenGL::GLShader::shared_ptr_type fragment_shader = GPlatesOpenGL::GLShader::create(gl, GL_FRAGMENT_SHADER);
		fragment_shader->shader_source(fragment_shader_source);
		fragment_shader->compile_shader();

		// Vertex-fragment program.
		program->attach_shader(vertex_shader);
		program->attach_shader(fragment_shader);
		program->link_program();
	}
}


GPlatesGui::MapBackground::MapBackground(
		GPlatesOpenGL::GL &gl,
		const MapProjection &map_projection,
		const Colour &colour) :
	d_map_projection(map_projection),
	d_view_state(nullptr),
	d_constant_colour(colour),
	d_program(GPlatesOpenGL::GLProgram::create(gl)),
	d_vertex_array(GPlatesOpenGL::GLVertexArray::create(gl)),
	d_vertex_buffer(GPlatesOpenGL::GLBuffer::create(gl)),
	d_vertex_element_buffer(GPlatesOpenGL::GLBuffer::create(gl)),
	d_num_vertex_indices(0)
{
	// Make sure we leave the OpenGL global state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	compile_link_program(gl, d_program);
}


GPlatesGui::MapBackground::MapBackground(
		GPlatesOpenGL::GL &gl,
		const MapProjection &map_projection,
		const GPlatesPresentation::ViewState &view_state) :
	d_map_projection(map_projection),
	d_view_state(&view_state),
	d_program(GPlatesOpenGL::GLProgram::create(gl)),
	d_vertex_array(GPlatesOpenGL::GLVertexArray::create(gl)),
	d_vertex_buffer(GPlatesOpenGL::GLBuffer::create(gl)),
	d_vertex_element_buffer(GPlatesOpenGL::GLBuffer::create(gl)),
	d_num_vertex_indices(0)
{
	// Make sure we leave the OpenGL global state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	compile_link_program(gl, d_program);
}


void
GPlatesGui::MapBackground::paint(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection)
{
	// Make sure we leave the OpenGL global state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	// Check whether we need to (re)generate the background due to changed map projection.
	//
	// Note: This will always happen on the first paint.
	const MapProjectionSettings map_projection_settings = d_map_projection.get_projection_settings();
	if (d_last_seen_map_projection_settings != map_projection_settings)
	{
		d_last_seen_map_projection_settings = map_projection_settings;

		std::vector<vertex_type> vertices;
		std::vector<vertex_element_type> vertex_elements;
		create_background(vertices, vertex_elements, d_map_projection);
		d_num_vertex_indices = vertex_elements.size();

		load_background(gl, d_vertex_array, d_vertex_buffer, d_vertex_element_buffer, vertices, vertex_elements);
	}

	gl.UseProgram(d_program);

	// Check whether we need to (re)set the background colour.
	//
	// Note: This will always happen on the first paint.
	if (d_view_state) // Check whether the view state's background colour has changed (if we're tracking it).
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				!d_constant_colour,
				GPLATES_ASSERTION_SOURCE);

		if (d_last_seen_colour != d_view_state->get_background_colour())
		{
			d_last_seen_colour = d_view_state->get_background_colour();

			// Set the background colour on currently bound program.
			glUniform4f(
					d_program->get_uniform_location("background_colour"),
					d_last_seen_colour->red(), d_last_seen_colour->green(), d_last_seen_colour->blue(), d_last_seen_colour->alpha());
		}
	}
	else // the colour is constant (but still need to set it on first paint)...
	{
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				d_constant_colour,
				GPLATES_ASSERTION_SOURCE);

		if (d_last_seen_colour != d_constant_colour)
		{
			d_last_seen_colour = d_constant_colour;

			// Set the background colour on currently bound program.
			glUniform4f(
					d_program->get_uniform_location("background_colour"),
					d_last_seen_colour->red(), d_last_seen_colour->green(), d_last_seen_colour->blue(), d_last_seen_colour->alpha());
		}
	}

	// Set view projection matrix in the currently bound program.
	GLfloat view_projection_float_matrix[16];
	view_projection.get_view_projection_transform().get_float_matrix(view_projection_float_matrix);
	glUniformMatrix4fv(
			d_program->get_uniform_location("view_projection"),
			1, GL_FALSE/*transpose*/, view_projection_float_matrix);

	// Bind the vertex array.
	gl.BindVertexArray(d_vertex_array);

	glDrawElements(
			GL_TRIANGLES,
			d_num_vertex_indices/*count*/,
			GPlatesOpenGL::GLVertexUtils::ElementTraits<vertex_element_type>::type,
			nullptr/*indices_offset*/);
}
