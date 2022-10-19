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

#ifndef GPLATES_GUI_STARS_H
#define GPLATES_GUI_STARS_H

#include <vector>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "Colour.h"

#include "opengl/GLBuffer.h"
#include "opengl/GLContextLifetime.h"
#include "opengl/GLProgram.h"
#include "opengl/GLStreamPrimitives.h"
#include "opengl/GLVertexArray.h"
#include "opengl/GLVertexUtils.h"
#include "opengl/OpenGL.h"  // For Class GL and the OpenGL constants/typedefs


namespace GPlatesOpenGL
{
	class GLViewProjection;
}

namespace GPlatesGui
{
	/**
	 * Draws a random collection of stars in the background.
	 */
	class Stars:
			public GPlatesOpenGL::GLContextLifetime
	{
	public:

		//! Default colour of the stars.
		static const GPlatesGui::Colour DEFAULT_COLOUR;


		explicit
		Stars(
				const GPlatesGui::Colour &colour = DEFAULT_COLOUR);


		/**
		 * The OpenGL context has been created.
		 */
		void
		initialise_gl(
				GPlatesOpenGL::GL &gl) override;

		/**
		 * The OpenGL context is about to be destroyed.
		 */
		void
		shutdown_gl(
				GPlatesOpenGL::GL &gl) override;


		/**
		 * Render the stars.
		 *
		 * Note: @a device_pixel_ratio is used on high-DPI displays where there are more pixels in the
		 *       same physical area on screen and so the point size of the stars is increased to compensate.
		 *
		 * Note: @a radius_multiplier is useful for the 2D map views to expand the positions of the stars radially
		 *       so that they're outside the map bounding sphere. The default of 1.0 works for the 3D globe view.
		 */
		void
		render(
				GPlatesOpenGL::GL &gl,
				const GPlatesOpenGL::GLViewProjection &view_projection,
				int device_pixel_ratio,
				const double &radius_multiplier = 1.0);

	private:

		static constexpr GLfloat SMALL_STARS_SIZE = 1.4f;
		static constexpr GLfloat LARGE_STARS_SIZE = 2.4f;

		static const unsigned int NUM_SMALL_STARS = 4250;
		static const unsigned int NUM_LARGE_STARS = 3750;

		// Points sit on a sphere of this radius (note that the Earth globe has radius 1.0).
		// Ideally we'd have these points at infinity, but a large distance works well.
		static const constexpr GLfloat RADIUS = 7.0f;

		typedef GPlatesOpenGL::GLVertexUtils::Vertex vertex_type;
		typedef GLushort vertex_element_type;
		typedef GPlatesOpenGL::GLDynamicStreamPrimitives<vertex_type, vertex_element_type> stream_primitives_type;


		void
		create_shader_program(
				GPlatesOpenGL::GL &gl);

		void
		create_stars(
				std::vector<vertex_type> &vertices,
				std::vector<vertex_element_type> &vertex_elements);

		void
		stream_stars(
				stream_primitives_type &stream,
				boost::function< double () > &rand,
				unsigned int num_stars);

		void
		load_stars(
				GPlatesOpenGL::GL &gl,
				const std::vector<vertex_type> &vertices,
				const std::vector<vertex_element_type> &vertex_elements);


		//! Colour of the stars.
		GPlatesGui::Colour d_colour;

		//! Shader program to render stars.
		GPlatesOpenGL::GLProgram::shared_ptr_type d_program;

		GPlatesOpenGL::GLVertexArray::shared_ptr_type d_vertex_array;
		GPlatesOpenGL::GLBuffer::shared_ptr_type d_vertex_buffer;
		GPlatesOpenGL::GLBuffer::shared_ptr_type d_vertex_element_buffer;

		unsigned int d_num_small_star_vertices;
		unsigned int d_num_small_star_vertex_indices;
		unsigned int d_num_large_star_vertices;
		unsigned int d_num_large_star_vertex_indices;
	};
}

#endif  // GPLATES_GUI_STARS_H
