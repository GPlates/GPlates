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

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>
#include <boost/utility/in_place_factory.hpp>
#include <QDebug>

#include "MapGrid.h"

#include "Colour.h"
#include "ProjectionException.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/MathsUtils.h"

#include "opengl/GL.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLStreamPrimitives.h"
#include "opengl/GLVertexArray.h"
#include "opengl/GLVertexUtils.h"
#include "opengl/GLViewProjection.h"

#include "utils/CallStackTracker.h"


namespace
{
	// Vertex and fragment shader source code to render grid lines in the 2D map views.
	const char *VERTEX_SHADER_SOURCE =
		R"(
			uniform mat4 view_projection;
			
			layout(location = 0) in vec4 position;
			layout(location = 1) in vec4 colour;

			out vec4 line_colour;
			
			void main (void)
			{
				gl_Position = view_projection * position;
				
				line_colour = colour;
			}
		)";
	const char *FRAGMENT_SHADER_SOURCE =
		R"(
			in vec4 line_colour;
			
			layout(location = 0) out vec4 colour;

			void main (void)
			{
				colour = line_colour;
			}
		)";

	// Vertex stream.
	typedef GPlatesOpenGL::GLVertexUtils::ColourVertex vertex_type;
	typedef GLuint vertex_element_type;
	typedef GPlatesOpenGL::GLDynamicStreamPrimitives<vertex_type, vertex_element_type> stream_primitives_type;

	// Projection coordinates.
	typedef std::pair<double, double> projection_coord_type;
	typedef std::vector<projection_coord_type> projection_coord_seq_type;

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


	/**
	 * Draw lines of latitude.
	 */
	void
	stream_lines_of_lat(
			stream_primitives_type &stream,
			const GPlatesGui::MapProjection &map_projection,
			const double &lat_0,
			const double &lon_0,
			const double &delta_lat,
			const GPlatesGui::rgba8_t &colour)
	{
		bool ok = true;

		stream_primitives_type::LineStrips stream_line_strips(stream);

		// Lines of latitude.
		GPlatesMaths::real_t lat = lat_0;
		bool loop = true;
		while (loop)
		{
			if (lat <= -90)
			{
				loop = false;
				lat = -90;
			}

			// Project the vertices using the map projection.
			projection_coord_seq_type projected_coords;
			try
			{
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

					projected_coords.push_back(
							project_lat_lon(lat.dval(), lon.dval(), map_projection));
				}
			}
			catch (const GPlatesGui::ProjectionException &exc)
			{
				// Ignore exception.
				qWarning() << exc;

				// Only draw if no projection exceptions thrown.
				continue;
			}

			// Stream the projected vertices into the line stream.
			stream_line_strips.begin_line_strip();

			for (const projection_coord_type &projected_coord : projected_coords)
			{
				const vertex_type vertex(
						projected_coord.first, projected_coord.second, 0/*z*/, colour);

				ok = ok && stream_line_strips.add_vertex(vertex);
			}

			stream_line_strips.end_line_strip();

			lat -= delta_lat;
		}

		// Since we added vertices/indices to a std::vector we shouldn't have run out of space.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				ok,
				GPLATES_ASSERTION_SOURCE);
	}


	/**
	 * Draw lines of longitude.
	 */
	void
	stream_lines_of_lon(
			stream_primitives_type &stream,
			const GPlatesGui::MapProjection &map_projection,
			const double &lat_0,
			const double &lon_0,
			const double &delta_lon,
			const GPlatesGui::rgba8_t &colour)
	{
		bool ok = true;

		stream_primitives_type::LineStrips stream_line_strips(stream);

		// Lines of longitude.
		GPlatesMaths::real_t lon = lon_0;
		bool loop = true;
		while (loop)
		{
			if (lon >= lon_0 + 360)
			{
				loop = false;
				lon = lon_0 + 360;
			}

			// Project the vertices using the map projection.
			projection_coord_seq_type projected_coords;
			try
			{
				for (int y = 0; y < LINE_OF_LONGITUDE_NUM_SEGMENTS + 1; ++y)
				{
					GPlatesMaths::real_t lat;
					// Stay slightly inside the map boundary to avoid any potential map projection
					// issues (eg, due to numerical precision).
					if (y == 0)
					{
						lat = lat_0 - 1e-8;
					}
					else if (y == LINE_OF_LONGITUDE_NUM_SEGMENTS)
					{
						lat = lat_0 - 180 + 1e-8;
					}
					else
					{
						lat = lat_0 - y * LINE_OF_LONGITUDE_DELTA_LATITUDE;
					}

					projected_coords.push_back(
							project_lat_lon(lat.dval(), lon.dval(), map_projection));
				}
			}
			catch (const GPlatesGui::ProjectionException &exc)
			{
				// Ignore exception.
				qWarning() << exc;

				// Only draw if no projection exceptions thrown.
				continue;
			}

			// Stream the projected vertices into the line stream.
			stream_line_strips.begin_line_strip();

			for (const projection_coord_type &projected_coord : projected_coords)
			{
				const vertex_type vertex(
						projected_coord.first, projected_coord.second, 0/*z*/, colour);

				ok = ok && stream_line_strips.add_vertex(vertex);
			}

			stream_line_strips.end_line_strip();

			lon += delta_lon;
		}

		// Since we added vertices/indices to a std::vector we shouldn't have run out of space.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				ok,
				GPLATES_ASSERTION_SOURCE);
	}


	void
	create_grid_lines(
			std::vector<vertex_type> &vertices,
			std::vector<vertex_element_type> &vertex_elements,
			const GPlatesGui::MapProjection &map_projection,
			const GPlatesMaths::Real &delta_lat,
			const GPlatesMaths::Real &delta_lon,
			const GPlatesGui::rgba8_t &colour)
	{
		stream_primitives_type stream;

		stream_primitives_type::StreamTarget stream_target(stream);

		stream_target.start_streaming(
				boost::in_place(boost::ref(vertices)),
				boost::in_place(boost::ref(vertex_elements)));

		const double lat_0 = 90;
		const double lon_0 = map_projection.central_meridian() - 180;

		if (delta_lat != 0.0)
		{
			stream_lines_of_lat(stream, map_projection, lat_0, lon_0, delta_lat.dval(), colour);
		}

		if (delta_lon != 0.0)
		{
			stream_lines_of_lon(stream, map_projection, lat_0, lon_0, delta_lon.dval(), colour);
		}

		stream_target.stop_streaming();
	}


	void
	load_grid_lines(
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
		gl.BufferData(
				GL_ELEMENT_ARRAY_BUFFER,
				vertex_elements.size() * sizeof(vertex_elements[0]),
				vertex_elements.data(),
				GL_STATIC_DRAW);

		// Bind vertex buffer object (used by vertex attribute arrays, not vertex array object).
		gl.BindBuffer(GL_ARRAY_BUFFER, vertex_buffer);

		// Transfer vertex data to currently bound vertex buffer object.
		gl.BufferData(
				GL_ARRAY_BUFFER,
				vertices.size() * sizeof(vertices[0]),
				vertices.data(),
				GL_STATIC_DRAW);

		// Specify vertex attributes (position and colour) in currently bound vertex buffer object.
		// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
		// to currently bound vertex array object.
		gl.EnableVertexAttribArray(0);
		gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_type), BUFFER_OFFSET(vertex_type, x));
		gl.EnableVertexAttribArray(1);
		gl.VertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(vertex_type), BUFFER_OFFSET(vertex_type, colour));
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
		vertex_shader->shader_source(gl, vertex_shader_source);
		vertex_shader->compile_shader(gl);

		// Fragment shader source.
		GPlatesOpenGL::GLShaderSource fragment_shader_source;
		fragment_shader_source.add_code_segment(FRAGMENT_SHADER_SOURCE);

		// Fragment shader.
		GPlatesOpenGL::GLShader::shared_ptr_type fragment_shader = GPlatesOpenGL::GLShader::create(gl, GL_FRAGMENT_SHADER);
		fragment_shader->shader_source(gl, fragment_shader_source);
		fragment_shader->compile_shader(gl);

		// Vertex-fragment program.
		program->attach_shader(gl, vertex_shader);
		program->attach_shader(gl, fragment_shader);
		program->link_program(gl);
	}
}


GPlatesGui::MapGrid::MapGrid(
		GPlatesOpenGL::GL &gl,
		const MapProjection &map_projection,
		const GraticuleSettings &graticule_settings) :
	d_map_projection(map_projection),
	d_graticule_settings(graticule_settings),
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
GPlatesGui::MapGrid::paint(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection)
{
	// Make sure we leave the OpenGL global state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	const MapProjectionSettings map_projection_settings = d_map_projection.get_projection_settings();

	// Check whether we need to (re)generate the grid lines.
	//
	// Note: This will always happen on the first paint.
	if (d_last_seen_graticule_settings != d_graticule_settings ||
		d_last_seen_map_projection_settings != map_projection_settings)
	{
		std::vector<vertex_type> vertices;
		std::vector<vertex_element_type> vertex_elements;
		create_grid_lines(
				vertices,
				vertex_elements,
				d_map_projection,
				GPlatesMaths::convert_rad_to_deg(d_graticule_settings.get_delta_lat()),
				GPlatesMaths::convert_rad_to_deg(d_graticule_settings.get_delta_lon()),
				Colour::to_rgba8(d_graticule_settings.get_colour()));
		d_num_vertex_indices = vertex_elements.size();

		load_grid_lines(gl, d_vertex_array, d_vertex_buffer, d_vertex_element_buffer, vertices, vertex_elements);

		d_last_seen_graticule_settings = d_graticule_settings;
		d_last_seen_map_projection_settings = map_projection_settings;
	}

	// Set the anti-aliased line state.
	gl.Enable(GL_LINE_SMOOTH);
	gl.Hint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	gl.LineWidth(d_graticule_settings.get_line_width_hint());

	//
	// For alpha-blending we want:
	//
	//   RGB = A_src * RGB_src + (1-A_src) * RGB_dst
	//     A =     1 *   A_src + (1-A_src) *   A_dst
	//
	// ...so we need to use separate (src,dst) blend factors for the RGB and alpha channels...
	//
	//   RGB uses (A_src, 1 - A_src)
	//     A uses (    1, 1 - A_src)
	//
	// ...this enables the destination to be a texture that is subsequently blended into the final scene.
	// In this case the destination alpha must be correct in order to properly blend the texture into the final scene.
	// However if we're rendering directly into the scene (ie, no render-to-texture) then destination alpha is not
	// actually used (since only RGB in the final scene is visible) and therefore could use same blend factors as RGB.
	//
	gl.Enable(GL_BLEND);
	gl.BlendFuncSeparate(
		GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,  // RGB
		GL_ONE, GL_ONE_MINUS_SRC_ALPHA);       // Alpha

	// Use the shader program.
	gl.UseProgram(d_program);

	// Set view projection matrix in the currently bound program.
	GLfloat view_projection_float_matrix[16];
	view_projection.get_view_projection_transform().get_float_matrix(view_projection_float_matrix);
	gl.UniformMatrix4fv(
			d_program->get_uniform_location(gl, "view_projection"),
			1, GL_FALSE/*transpose*/, view_projection_float_matrix);

	// Bind the vertex array.
	gl.BindVertexArray(d_vertex_array);

	// Draw the grid lines.
	gl.DrawElements(
			GL_LINES,
			d_num_vertex_indices/*count*/,
			GPlatesOpenGL::GLVertexUtils::ElementTraits<vertex_element_type>::type,
			nullptr/*indices_offset*/);
}
