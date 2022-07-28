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

#include <opengl/OpenGL3.h>  // Should be included at TOP of ".cc" file.

#include <cmath>
#include <boost/function.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <QString>

#include "Stars.h"

#include "Colour.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "opengl/GL.h"
#include "opengl/GLMatrix.h"
#include "opengl/GLShader.h"
#include "opengl/GLShaderSource.h"
#include "opengl/GLStreamPrimitives.h"
#include "opengl/GLVertexArray.h"
#include "opengl/GLVertexUtils.h"
#include "opengl/GLViewProjection.h"

#include "presentation/ViewState.h"

#include "utils/CallStackTracker.h"


namespace
{
	// Vertex and fragment shader source code to render stars (points) in the 3D globe views (perspective and orthographic).
	const char *VERTEX_SHADER_SOURCE =
		R"(
			uniform mat4 view_projection;
			
			layout(location = 0) in vec4 position;
			
			void main (void)
			{
				gl_Position = view_projection * position;
				
				// We enabled GL_DEPTH_CLAMP to disable the far clipping plane, but it also disables
				// near plane (which we still want), so we'll handle that ourself.
				//
				// Inside near clip plane means:
				//
				//   -gl_Position.w <= gl_Position.z
				//
				// ...or:
				//
				//   gl_Position.z + gl_Position.w >= 0
				//
				gl_ClipDistance[0] = gl_Position.z + gl_Position.w;
			}
		)";
	const char *FRAGMENT_SHADER_SOURCE =
		R"(
			uniform vec4 star_colour;
			
			layout(location = 0) out vec4 colour;

			void main (void)
			{
				colour = star_colour;
			}
		)";

	const GLfloat SMALL_STARS_SIZE = 1.4f;
	const GLfloat LARGE_STARS_SIZE = 2.1f;

	const unsigned int NUM_SMALL_STARS = 4250;
	const unsigned int NUM_LARGE_STARS = 3750;

	// Points sit on a sphere of this radius.
	// Note that ideally, we'd have these points at infinity - but we can't do
	// that, because we use an orthographic projection for the globe...
	const GLfloat RADIUS = 7.0f;

	typedef GPlatesOpenGL::GLVertexUtils::Vertex vertex_type;
	typedef GLushort vertex_element_type;
	typedef GPlatesOpenGL::GLDynamicStreamPrimitives<vertex_type, vertex_element_type> stream_primitives_type;


	void
	stream_stars(
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
	create_stars(
			std::vector<vertex_type> &vertices,
			std::vector<vertex_element_type> &vertex_elements,
			unsigned int &num_small_star_vertices,
			unsigned int &num_small_star_vertex_indices,
			unsigned int &num_large_star_vertices,
			unsigned int &num_large_star_vertex_indices,
			boost::function< double () > &rand)
	{
		stream_primitives_type stream;

		stream_primitives_type::StreamTarget stream_target(stream);

		stream_target.start_streaming(
				boost::in_place(boost::ref(vertices)),
				boost::in_place(boost::ref(vertex_elements)));

		// Stream the small stars.
		stream_stars(stream, rand, NUM_SMALL_STARS);

		num_small_star_vertices = stream_target.get_num_streamed_vertices();
		num_small_star_vertex_indices = stream_target.get_num_streamed_vertex_elements();

		stream_target.stop_streaming();

		// We re-start streaming so that we can get a separate stream count for the large stars.
		// However the large stars still get appended onto 'vertices' and 'vertex_elements'.
		stream_target.start_streaming(
				boost::in_place(boost::ref(vertices)),
				boost::in_place(boost::ref(vertex_elements)));

		// Stream the large stars.
		stream_stars(stream, rand, NUM_LARGE_STARS);

		num_large_star_vertices = stream_target.get_num_streamed_vertices();
		num_large_star_vertex_indices = stream_target.get_num_streamed_vertex_elements();

		stream_target.stop_streaming();

		// We're using 16-bit indices (ie, 65536 vertices) so make sure we've not exceeded that many vertices.
		// Shouldn't get close really but check to be sure.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				vertices.size() - 1 <= GPlatesOpenGL::GLVertexUtils::ElementTraits<vertex_element_type>::MAX_INDEXABLE_VERTEX,
				GPLATES_ASSERTION_SOURCE);
	}


	void
	load_stars(
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

		// Specify vertex attributes (position) in currently bound vertex buffer object.
		// This transfers each vertex attribute array (parameters + currently bound vertex buffer object)
		// to currently bound vertex array object.
		gl.EnableVertexAttribArray(0);
		gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_type), BUFFER_OFFSET(vertex_type, x));
	}


	void
	compile_link_program(
			GPlatesOpenGL::GL &gl,
			GPlatesOpenGL::GLProgram::shared_ptr_type program,
			const GPlatesGui::Colour &colour)
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

		gl.UseProgram(program);

		// Set the star colour (it never changes).
		gl.Uniform4f(
				program->get_uniform_location(gl, "star_colour"),
				colour.red(), colour.green(), colour.blue(), colour.alpha());
	}


	void
	draw_stars(
			GPlatesOpenGL::GL &gl,
			unsigned int num_small_star_vertices,
			unsigned int num_small_star_vertex_indices,
			unsigned int num_large_star_vertices,
			unsigned int num_large_star_vertex_indices,
			int device_pixel_ratio)
	{
		// Small stars size.
		//
		// Note: Multiply point sizes to account for ratio of device pixels to device *independent* pixels.
		//       On high-DPI displays there are more pixels in the same physical area on screen and so
		//       without increasing the point size the points would look too small.
		gl.PointSize(SMALL_STARS_SIZE * device_pixel_ratio);

		// Draw the small stars.
		gl.DrawRangeElements(
				GL_POINTS,
				0/*start*/,
				num_small_star_vertices - 1/*end*/,
				num_small_star_vertex_indices/*count*/,
				GPlatesOpenGL::GLVertexUtils::ElementTraits<vertex_element_type>::type,
				nullptr/*indices_offset*/);

		// Large stars size.
		//
		// Note: Multiply point sizes to account for ratio of device pixels to device *independent* pixels.
		//       On high-DPI displays there are more pixels in the same physical area on screen and so
		//       without increasing the point size the points would look too small.
		gl.PointSize(LARGE_STARS_SIZE * device_pixel_ratio);

		// Draw the large stars.
		// They come after the small stars in the vertex array.
		gl.DrawRangeElements(
				GL_POINTS,
				num_small_star_vertices/*start*/,
				num_small_star_vertices + num_large_star_vertices - 1/*end*/,
				num_large_star_vertex_indices/*count*/,
				GPlatesOpenGL::GLVertexUtils::ElementTraits<vertex_element_type>::type,
				GPlatesOpenGL::GLVertexUtils::buffer_offset(num_small_star_vertex_indices * sizeof(vertex_element_type))/*indices_offset*/);
	}
}


GPlatesGui::Stars::Stars(
		GPlatesOpenGL::GL &gl,
		GPlatesPresentation::ViewState &view_state,
		const GPlatesGui::Colour &colour) :
	d_view_state(view_state),
	d_program(GPlatesOpenGL::GLProgram::create(gl)),
	d_vertex_array(GPlatesOpenGL::GLVertexArray::create(gl)),
	d_vertex_buffer(GPlatesOpenGL::GLBuffer::create(gl)),
	d_vertex_element_buffer(GPlatesOpenGL::GLBuffer::create(gl)),
	d_num_small_star_vertices(0),
	d_num_small_star_vertex_indices(0),
	d_num_large_star_vertices(0),
	d_num_large_star_vertex_indices(0)
{
	// Make sure we leave the OpenGL global state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

	compile_link_program(gl, d_program, colour);

	// Set up the random number generator.
	// It generates doubles uniformly from -1.0 to 1.0 inclusive.
	// Note that we use a fixed seed (0), so that the pattern of stars does not
	// change between sessions. This is useful when trying to reproduce screenshots
	// between sessions.
	boost::mt19937 gen(0);
	boost::uniform_real<> dist(-1.0, 1.0);
	boost::function< double () > rand(
			boost::variate_generator<boost::mt19937&, boost::uniform_real<> >(gen, dist));

	std::vector<vertex_type> vertices;
	std::vector<vertex_element_type> vertex_elements;
	create_stars(
			vertices,
			vertex_elements,
			d_num_small_star_vertices, d_num_small_star_vertex_indices,
			d_num_large_star_vertices, d_num_large_star_vertex_indices,
			rand);

	load_stars(gl, d_vertex_array, d_vertex_buffer, d_vertex_element_buffer, vertices, vertex_elements);
}


void
GPlatesGui::Stars::paint(
		GPlatesOpenGL::GL &gl,
		const GPlatesOpenGL::GLViewProjection &view_projection,
		int device_pixel_ratio)
{
	if (!d_view_state.get_show_stars())
	{
		return;
	}

	// Make sure we leave the OpenGL global state the way it was.
	GPlatesOpenGL::GL::StateScope save_restore_state(gl);

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

	// Bind the vertex array.
	gl.BindVertexArray(d_vertex_array);

	draw_stars(
			gl,
			d_num_small_star_vertices,
			d_num_small_star_vertex_indices,
			d_num_large_star_vertices,
			d_num_large_star_vertex_indices,
			device_pixel_ratio);
}
