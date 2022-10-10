/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <cmath>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <QString>

#include "Stars.h"

#include "Colour.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "opengl/GLMatrix.h"
#include "opengl/GLShader.h"
#include "opengl/GLShaderSource.h"
#include "opengl/GLVertexArray.h"
#include "opengl/GLViewProjection.h"

#include "presentation/ViewState.h"

#include "utils/CallStackTracker.h"


namespace
{
	// Vertex and fragment shader source code to renders stars (points) in the background of the scene.
	const QString STARS_VERTEX_SHADER_SOURCE_FILE_NAME = ":/opengl/stars.vert";
	const QString STARS_FRAGMENT_SHADER_SOURCE_FILE_NAME = ":/opengl/stars.frag";
}


const GPlatesGui::Colour GPlatesGui::Stars::DEFAULT_COLOUR(0.75f, 0.75f, 0.75f);


GPlatesGui::Stars::Stars(
		const GPlatesGui::Colour &colour) :
	d_colour(colour),
	d_num_small_star_vertices(0),
	d_num_small_star_vertex_indices(0),
	d_num_large_star_vertices(0),
	d_num_large_star_vertex_indices(0)
{
}


void
GPlatesGui::Stars::initialise_gl(
		GPlatesOpenGL::GL &gl)
{
	create_shader_program(gl);

	std::vector<vertex_type> vertices;
	std::vector<vertex_element_type> vertex_elements;
	create_stars(vertices, vertex_elements);
	load_stars(gl, vertices, vertex_elements);
}

void
GPlatesGui::Stars::shutdown_gl(
		GPlatesOpenGL::GL &gl)
{
	d_vertex_element_buffer.reset();
	d_vertex_buffer.reset();
	d_vertex_array.reset();
	d_program.reset();
}


void
GPlatesGui::Stars::render(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		int device_pixel_ratio,
		const double &radius_multiplier)
{
	// Make sure we leave the OpenGL global state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	// Disable depth testing and depth writes.
	// Stars are rendered in the background and don't really need depth sorting.
	gl.Disable(GL_DEPTH_TEST);
	gl.DepthMask(GL_FALSE);

	// Enabling depth clamping disables the near and far clip planes (and clamps depth values outside).
	// This means the stars (which are beyond the far clip plane) get rendered (with the far depth 1.0).
	// However it means (for orthographic projection) that stars behind the viewer also get rendered.
	// Note that this doesn't happen for perspective projection since the 4 side planes form a pyramid
	// with apex at the view/camera position (and these 4 planes remove anything behind the viewer).
	// To get around this we clip to the near plane ourselves (using gl_ClipDistance in the shader).
	gl.Enable(GL_DEPTH_CLAMP);
	gl.Enable(GL_CLIP_DISTANCE0);

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
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	// Use the shader program.
	gl.UseProgram(d_program);

	// Set view projection matrix in the currently bound program.
	GLfloat view_projection_float_matrix[16];
	view_projection.get_view_projection_transform().get_float_matrix(view_projection_float_matrix);
	gl.UniformMatrix4fv(
			d_program->get_uniform_location(gl, "view_projection"),
			1, GL_FALSE/*transpose*/, view_projection_float_matrix);

	// Set the radius multiplier.
	//
	// This is used for the 2D map views to expand the positions of the stars radially so that they're
	// outside the map bounding sphere. A value of 1.0 works for the 3D globe view.
	gl.Uniform1f(
			d_program->get_uniform_location(gl, "radius_multiplier"),
			radius_multiplier);

	// Bind the vertex array.
	gl.BindVertexArray(d_vertex_array);

	// We specify/adjust the point size in the vertex shader.
	gl.Enable(GL_PROGRAM_POINT_SIZE);

	// Small stars point size.
	//
	// Note: Multiply point sizes to account for ratio of device pixels to device *independent* pixels.
	//       On high-DPI displays there are more pixels in the same physical area on screen and so
	//       without increasing the point size the points would look too small.
	gl.Uniform1f(
			d_program->get_uniform_location(gl, "point_size"),
			SMALL_STARS_SIZE * device_pixel_ratio);

	// Draw the small stars.
	gl.DrawRangeElements(
			GL_POINTS,
			0/*start*/,
			d_num_small_star_vertices - 1/*end*/,
			d_num_small_star_vertex_indices/*count*/,
			GPlatesOpenGL::GLVertexUtils::ElementTraits<vertex_element_type>::type,
			nullptr/*indices_offset*/);

	// Large stars point size.
	//
	// Note: Multiply point sizes to account for ratio of device pixels to device *independent* pixels.
	//       On high-DPI displays there are more pixels in the same physical area on screen and so
	//       without increasing the point size the points would look too small.
	gl.Uniform1f(
			d_program->get_uniform_location(gl, "point_size"),
			LARGE_STARS_SIZE * device_pixel_ratio);

	// Draw the large stars.
	// They come after the small stars in the vertex array.
	gl.DrawRangeElements(
			GL_POINTS,
			d_num_small_star_vertices/*start*/,
			d_num_small_star_vertices + d_num_large_star_vertices - 1/*end*/,
			d_num_large_star_vertex_indices/*count*/,
			GPlatesOpenGL::GLVertexUtils::ElementTraits<vertex_element_type>::type,
			GPlatesOpenGL::GLVertexUtils::buffer_offset(d_num_small_star_vertex_indices * sizeof(vertex_element_type))/*indices_offset*/);
}


void
GPlatesGui::Stars::create_shader_program(
		GPlatesOpenGL::GL &gl)
{
	// Add this scope to the call stack trace printed if exception thrown in this scope (eg, failure to compile/link shader).
	TRACK_CALL_STACK();

	// Vertex shader source.
	GPlatesOpenGL::GLShaderSource vertex_shader_source;
	vertex_shader_source.add_code_segment_from_file(STARS_VERTEX_SHADER_SOURCE_FILE_NAME);

	// Vertex shader.
	GPlatesOpenGL::GLShader::shared_ptr_type vertex_shader = GPlatesOpenGL::GLShader::create(gl, GL_VERTEX_SHADER);
	vertex_shader->shader_source(gl, vertex_shader_source);
	vertex_shader->compile_shader(gl);

	// Fragment shader source.
	GPlatesOpenGL::GLShaderSource fragment_shader_source;
	fragment_shader_source.add_code_segment_from_file(STARS_FRAGMENT_SHADER_SOURCE_FILE_NAME);

	// Fragment shader.
	GPlatesOpenGL::GLShader::shared_ptr_type fragment_shader = GPlatesOpenGL::GLShader::create(gl, GL_FRAGMENT_SHADER);
	fragment_shader->shader_source(gl, fragment_shader_source);
	fragment_shader->compile_shader(gl);

	// Vertex-fragment program.
	d_program = GPlatesOpenGL::GLProgram::create(gl);
	d_program->attach_shader(gl, vertex_shader);
	d_program->attach_shader(gl, fragment_shader);
	d_program->link_program(gl);

	gl.UseProgram(d_program);

	// Set the star colour (it never changes).
	gl.Uniform4f(
			d_program->get_uniform_location(gl, "star_colour"),
			d_colour.red(), d_colour.green(), d_colour.blue(), d_colour.alpha());
}


void
GPlatesGui::Stars::create_stars(
		std::vector<vertex_type> &vertices,
		std::vector<vertex_element_type> &vertex_elements)
{
	// Set up the random number generator.
	// It generates doubles uniformly from -1.0 to 1.0 inclusive.
	// Note that we use a fixed seed (0), so that the pattern of stars does not
	// change between sessions. This is useful when trying to reproduce screenshots
	// between sessions.
	boost::mt19937 gen(0);
	boost::uniform_real<> dist(-1.0, 1.0);
	boost::function< double () > rand(
			boost::variate_generator<boost::mt19937&, boost::uniform_real<> >(gen, dist));

	stream_primitives_type stream;

	stream_primitives_type::StreamTarget stream_target(stream);

	stream_target.start_streaming(
			boost::in_place(boost::ref(vertices)),
			boost::in_place(boost::ref(vertex_elements)));

	// Stream the small stars.
	stream_stars(stream, rand, NUM_SMALL_STARS);

	d_num_small_star_vertices = stream_target.get_num_streamed_vertices();
	d_num_small_star_vertex_indices = stream_target.get_num_streamed_vertex_elements();

	stream_target.stop_streaming();

	// We re-start streaming so that we can get a separate stream count for the large stars.
	// However the large stars still get appended onto 'vertices' and 'vertex_elements'.
	stream_target.start_streaming(
			boost::in_place(boost::ref(vertices)),
			boost::in_place(boost::ref(vertex_elements)));

	// Stream the large stars.
	stream_stars(stream, rand, NUM_LARGE_STARS);

	d_num_large_star_vertices = stream_target.get_num_streamed_vertices();
	d_num_large_star_vertex_indices = stream_target.get_num_streamed_vertex_elements();

	stream_target.stop_streaming();

	// We're using 16-bit indices (ie, 65536 vertices) so make sure we've not exceeded that many vertices.
	// Shouldn't get close really but check to be sure.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			vertices.size() - 1 <= GPlatesOpenGL::GLVertexUtils::ElementTraits<vertex_element_type>::MAX_INDEXABLE_VERTEX,
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesGui::Stars::stream_stars(
		stream_primitives_type &stream,
		boost::function< double () > &rand,
		unsigned int num_stars)
{
	bool ok = true;

	stream_primitives_type::Points stream_points(stream);
	stream_points.begin_points();

	unsigned int points_generated = 0;
	while (points_generated != num_stars)
	{
		// See http://mathworld.wolfram.com/SpherePointPicking.html for a discussion
		// of picking points uniformly on the surface of a sphere.
		// We use the method attributed to Marsaglia (1972).
		double x_1 = rand();
		double x_2 = rand();
		double x_1_sq = x_1 * x_1;
		double x_2_sq = x_2 * x_2;

		double stuff_under_sqrt = 1 - x_1_sq - x_2_sq;
		if (stuff_under_sqrt < 0)
		{
			continue;
		}
		double sqrt_part = std::sqrt(stuff_under_sqrt);

		double x = 2 * x_1 * sqrt_part;
		double y = 2 * x_2 * sqrt_part;
		double z = 1 - 2 * (x_1_sq + x_2_sq);

		// Randomising the distance to the stars gives a nicer 3D effect.
		double radius = RADIUS + rand();

		vertex_type vertex(x * radius, y * radius, z * radius);
		ok = ok && stream_points.add_vertex(vertex);

		++points_generated;
	}

	stream_points.end_points();

	// Since we added vertices/indices to a std::vector we shouldn't have run out of space.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			ok,
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesGui::Stars::load_stars(
		GPlatesOpenGL::GL &gl,
		const std::vector<vertex_type> &vertices,
		const std::vector<vertex_element_type> &vertex_elements)
{
	// Transfer vertex element data to the vertex element buffer object.
	d_vertex_element_buffer = GPlatesOpenGL::GLBuffer::create(gl);
	gl.NamedBufferStorage(
			d_vertex_element_buffer,
			vertex_elements.size() * sizeof(vertex_elements[0]),
			vertex_elements.data(),
			0/*flags*/);

	// Transfer vertex data to the vertex buffer object.
	d_vertex_buffer = GPlatesOpenGL::GLBuffer::create(gl);
	gl.NamedBufferStorage(
			d_vertex_buffer,
			vertices.size() * sizeof(vertices[0]),
			vertices.data(),
			0/*flags*/);

	d_vertex_array = GPlatesOpenGL::GLVertexArray::create(gl);

	// Bind vertex element buffer object to the vertex array object.
	gl.VertexArrayElementBuffer(d_vertex_array, d_vertex_element_buffer);

	// Bind vertex buffer object to vertex array object.
	gl.VertexArrayVertexBuffer(d_vertex_array, 0/*bindingindex*/, d_vertex_buffer, 0/*offset*/, sizeof(vertex_type));

	// Specify vertex attributes (position) in the vertex buffer object.
	gl.EnableVertexArrayAttrib(d_vertex_array, 0);
	gl.VertexArrayAttribFormat(d_vertex_array, 0, 3, GL_FLOAT, GL_FALSE, ATTRIB_OFFSET_IN_VERTEX(vertex_type, x));
	gl.VertexArrayAttribBinding(d_vertex_array, 0, 0/*bindingindex*/);
}
