/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
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
 
#ifndef GPLATES_OPENGL_GLSTREAMPRIMITIVES_H
#define GPLATES_OPENGL_GLSTREAMPRIMITIVES_H

#include <algorithm>
#include <cstddef> // For std::size_t
#include <iterator>
#include <vector>
#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLCompositeDrawable.h"
#include "GLContext.h"
#include "GLDrawable.h"
#include "GLVertexArray.h"
#include "GLVertexArrayDrawable.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Use this when you want to stream points, lines, triangles, quads or polygon (convex)
	 * to the graphics hardware.
	 *
	 * This class is designed to be used in situations where you want to gather primitives
	 * into one vertex array for efficiency in rendering.
	 *
	 * It is based on the familiar immediate mode rendering interface of 'glBegin' / 'glEnd'.
	 *
	 * This class does take advantage of vertex re-use (via vertex indices) but it is limited
	 * to the re-use you get with line strips/loops and triangles strips/fans.
	 * If you have a complex mesh you are better off generating your own vertex and index
	 * buffers to take maximal advantage of vertex indexing and hence vertex re-use.
	 * Graphics cards usually have a small cache of recently transformed vertices and if
	 * a previously transformed vertex is indexed again relatively soon then it can
	 * access the transformed vertex cache for a speed improvement.
	 *
	 * It is best *not* to mix different primitives types together - use separate stream
	 * objects for that. When you switch primitive types mid-stream a new drawable will
	 * be started, effectively minimising your ability to gather primitives into one drawable.
	 * And in the worst case you'll end up with one drawable for each primitive.
	 * Primitive types that can be mixed together without incurring this overhead are:
	 * Points    - GL_POINTS
	 * Lines     - GL_LINES, GL_LINE_STRIP and GL_LINE_LOOP can be mixed
	 * Triangles - GL_TRIANGLES, GL_TRIANGLE_STRIP and GL_TRIANGLE_FAN can be mixed
	 * Quads     - GL_QUADS and GL_QUAD_STRIP can be mixed
	 * NOTE: GL_POLYGON is excluded for implementation reasons (polygon has an arbitrary number of
	 * vertices which causes problems when it overlaps the boundary of two vertex arrays - however
	 * the other primitives have a maximum overlap of two vertices which we do handle).
	 *
	 * This class takes care of breaking up the stream into finite sized vertex arrays
	 * since vertex indices are 16-bit effectively limiting the number of vertices per
	 * array to 65536.
	 */
	template <class VertexType>
	class GLStreamPrimitives :
			public GPlatesUtils::ReferenceCount<GLStreamPrimitives<VertexType> >
	{
	public:
		//! Typedef for this class.
		typedef GLStreamPrimitives<VertexType> this_type;

		//! A convenience typedef for a shared pointer to a non-const @a GLStreamPrimitives.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLStreamPrimitives> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLStreamPrimitives.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLStreamPrimitives> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLStreamPrimitives object.
		 */
		static
		non_null_ptr_type
		create(
				const GLVertexArray::VertexPointer &vertex_pointer,
				const boost::optional<GLVertexArray::ColorPointer> &colour_pointer = boost::none,
				const boost::optional<GLVertexArray::NormalPointer> &normal_pointer = boost::none,
				const boost::optional<std::vector<boost::optional<GLVertexArray::TexCoordPointer> > > &
						tex_coord_pointers = boost::none)
		{
			return non_null_ptr_type(new GLStreamPrimitives(
					vertex_pointer, colour_pointer, normal_pointer, tex_coord_pointers));
		}


		/**
		 * Returns a drawable that represents all primitives added so far.
		 *
		 * The returned drawable might, in turn, be composed of more than one drawable if:
		 * - more than one vertex array was required (because of the limitation
		 *   on number of vertices per array due to 16-bit vertex index size), or
		 * - different types of primitives were added.
		 *
		 * If no primitives were added then @a false is returned.
		 */
		boost::optional<GLDrawable::non_null_ptr_to_const_type>
		get_drawable();


		//////////////////////////////////////////////////////////////////////////////////
		// NOTE: It's easier if you attach primitive streaming objects to 'this' stream //
		// (such as @a GLStreamLineStrips, @a GLStreamTriangleStrips etc) than it is to //
		// directly use the methods below.                                              //
		//////////////////////////////////////////////////////////////////////////////////

		/**
		 * Adds a point primitive.
		 */
		void
		add_point(
				const VertexType &vertex);

		/**
		 * Adds a line primitive with the specified start and end vertices.
		 */
		void
		add_line(
				const VertexType &start_vertex,
				const VertexType &end_vertex);

		/**
		 * Adds a line primitive whose start vertex is the end of the last line added.
		 *
		 * This methods supports line strips and line loops.
		 *
		 * @throws @a PreconditionViolationError if the last call was not @a add_line.
		 */
		void
		add_line(
				const VertexType &end_vertex);

		/**
		 * Adds a triangle primitive with the specified vertices.
		 */
		void
		add_triangle(
				const VertexType &first_vertex,
				const VertexType &second_vertex,
				const VertexType &third_vertex);

		/**
		 * Adds a triangle primitive whose third vertex is @a third_vertex.
		 *
		 * Its first and second vertices are the second-last and last vertices added
		 * with the previous @a add_triangle call(s).
		 *
		 * This methods supports triangle strips.
		 *
		 * @throws @a PreconditionViolationError if the last call was not @a add_triangle.
		 */
		void
		add_triangle(
				const VertexType &third_vertex);

		/**
		 * Adds a triangle primitive whose third vertex is @a third_vertex.
		 *
		 * Its first and second vertices are the last and second-last vertices added
		 * with the previous @a add_triangle call(s).
		 * Note the reverse of the first and second vertices compared to @a add_triangle above.
		 *
		 * This methods supports triangle strips.
		 *
		 * @throws @a PreconditionViolationError if the last call was not @a add_triangle.
		 */
		void
		add_triangle_reversed(
				const VertexType &third_vertex);

		/**
		 * Adds a triangle primitive whose first and third vertices
		 * are @a first_vertex and @a third_vertex.
		 *
		 * Its second vertex is the last vertex added with the previous @a add_triangle call(s).
		 *
		 * This methods supports triangle fans.
		 *
		 * @throws @a PreconditionViolationError if the last call was not @a add_triangle.
		 */
		void
		add_triangle(
				const VertexType &first_vertex,
				const VertexType &third_vertex);


	private:
		/**
		 * The basic primitive types handled.
		 */
		enum PrimitiveType
		{
			PRIMITIVE_POINT,    // For GL_POINTS
			PRIMITIVE_LINE,     // For GL_LINES, GL_LINE_STRIP and GL_LINE_LOOP
			PRIMITIVE_TRIANGLE, // For GL_TRIANGLES, GL_TRIANGLE_STRIP and GL_TRIANGLE_FAN
			PRIMITIVE_QUAD,     // For GL_QUADS and GL_QUAD_STRIP.

			/*
			 * NOTE: GL_POLYGON is excluded for implementation reasons (a polygon has an
			 * arbitrary number of vertices which causes problems when it overlaps the boundary
			 * of two vertex arrays - however the other primitives have a maximum overlap of
			 * two vertices which we do handle).
			 */
			/*PRIMITIVE_POLYGON,*/  // For GL_POLYGON

			PRIMITIVE_NONE
		};

		/**
		 * Typedef for a vertex index.
		 *
		 * We use 16-bit indices as that's what most graphics hardware prefers and
		 * it makes for a good batch size to send to the hardware.
		 */
		typedef GLushort vertex_element_type;

		//! Typedef for an array of vertex indices.
		typedef std::vector<vertex_element_type> vertex_element_seq_type;

		//! Typedef for an array of vertices.
		typedef std::vector<VertexType> vertex_seq_type;

		//! Typedef for a sequence of drawables.
		typedef std::vector<GLDrawable::non_null_ptr_to_const_type> drawable_seq_type;

		/**
		 * The maximum number of vertices that can be indexed.
		 *
		 * This depends on the size of the vertex index type.
		 * For example, a 16-bit GLushort can index at most 65536 vertices.
		 */
		static const unsigned int MAX_NUM_VERTICES_PER_DRAWABLE =
				(1 << (8 * sizeof(vertex_element_type)));

		//
		// Contain information about what a vertex structure looks like.
		// See class @a GLVertexArray for more information.
		//
		GLVertexArray::VertexPointer d_vertex_pointer;
		boost::optional<GLVertexArray::ColorPointer> d_colour_pointer;
		boost::optional<GLVertexArray::NormalPointer> d_normal_pointer;
		// Each element in the vector represents a texture unit (0, 1, 2, ...).
		// If an element is optional it means no texture coordinates are specified for that unit.
		boost::optional<std::vector<boost::optional<GLVertexArray::TexCoordPointer> > > d_tex_coord_pointers;

		/**
		 * Contains vertices used for the current drawable.
		 */
		vertex_seq_type d_current_vertices;

		/**
		 * Contains vertices indices used for the current drawable.
		 */
		vertex_element_seq_type d_current_vertex_elements;

		/**
		 * The current type of primitive we are processing.
		 * This is determined by the 'gl_begin...' / 'gl_end...' methods.
		 */
		PrimitiveType d_current_primitive_type;

		/**
		 * List of all drawables generated so far.
		 */
		drawable_seq_type d_drawables;


		//! Constructor.
		GLStreamPrimitives(
				const GLVertexArray::VertexPointer &vertex_pointer,
				const boost::optional<GLVertexArray::ColorPointer> &colour_pointer,
				const boost::optional<GLVertexArray::NormalPointer> &normal_pointer,
				const boost::optional<std::vector<boost::optional<GLVertexArray::TexCoordPointer> > > &
						tex_coord_pointers);

		/**
		 * Puts any vertices/indices accumulated, since the last call
		 * to @a flush_primitives, into a new drawable.
		 * The drawable will only contains primitives of the current primitive type.
		 */
		void
		flush_primitives();

		/**
		 * Clears the primitives that have been (or would be) used for the current drawable.
		 *
		 * NOTE: Does not clear the list of drawables accumulated so far.
		 */
		void
		clear_primitives();
	};


	/**
	 * Attach to @a GLStreamPrimitives to stream point primitives.
	 *
	 * 'VertexType' must be default-constructible.
	 */
	template <class VertexType>
	class GLStreamPoints
	{
	public:
		explicit
		GLStreamPoints(
				GLStreamPrimitives<VertexType> &stream_primitives) :
			d_stream_primitives(stream_primitives)
		{  }

		void
		begin_points()
		{
			// Nothing to do
		}

		void
		add_vertex(
				const VertexType &vertex)
		{
			d_stream_primitives.add_point(vertex);
		}

		void
		end_points()
		{
			// Nothing to do
		}

	private:
		GLStreamPrimitives<VertexType> &d_stream_primitives;
	};


	/**
	 * Attach to @a GLStreamPrimitives to stream line primitives.
	 *
	 * 'VertexType' must be default-constructible.
	 */
	template <class VertexType>
	class GLStreamLines
	{
	public:
		explicit
		GLStreamLines(
				GLStreamPrimitives<VertexType> &stream_primitives) :
			d_stream_primitives(stream_primitives),
			d_first_vertex(true)
		{  }

		void
		begin_lines()
		{
			d_first_vertex = true;
		}

		void
		add_vertex(
				const VertexType &vertex);

		void
		end_lines()
		{
			// Nothing to do
		}

	private:
		GLStreamPrimitives<VertexType> &d_stream_primitives;
		VertexType d_start_vertex;
		bool d_first_vertex;
	};


	/**
	 * Attach to @a GLStreamPrimitives to stream line strip primitives.
	 *
	 * 'VertexType' must be default-constructible.
	 */
	template <class VertexType>
	class GLStreamLineStrips
	{
	public:
		explicit
		GLStreamLineStrips(
				GLStreamPrimitives<VertexType> &stream_primitives) :
			d_stream_primitives(stream_primitives),
			d_vertex_index(0)
		{  }

		bool
		is_start_of_strip() const
		{
			return d_vertex_index == 0;
		}

		void
		begin_line_strip()
		{
			d_vertex_index = 0;
		}

		void
		add_vertex(
				const VertexType &vertex);

		void
		end_line_strip()
		{
			// Nothing to do
		}

	private:
		GLStreamPrimitives<VertexType> &d_stream_primitives;
		VertexType d_start_vertex;
		unsigned int d_vertex_index;
	};


	/**
	 * Attach to @a GLStreamPrimitives to stream line loop primitives.
	 *
	 * 'VertexType' must be default-constructible.
	 */
	template <class VertexType>
	class GLStreamLineLoops
	{
	public:
		explicit
		GLStreamLineLoops(
				GLStreamPrimitives<VertexType> &stream_primitives) :
			d_stream_primitives(stream_primitives),
			d_vertex_index(0)
		{  }

		bool
		is_start_of_loop() const
		{
			return d_vertex_index == 0;
		}

		void
		begin_line_loop()
		{
			d_vertex_index = 0;
		}

		void
		add_vertex(
				const VertexType &vertex);

		void
		end_line_loop();

	private:
		GLStreamPrimitives<VertexType> &d_stream_primitives;
		VertexType d_start_vertex;
		unsigned int d_vertex_index;
	};


	/**
	 * Attach to @a GLStreamPrimitives to stream triangle primitives.
	 *
	 * 'VertexType' must be default-constructible.
	 */
	template <class VertexType>
	class GLStreamTriangles
	{
	public:
		explicit
		GLStreamTriangles(
				GLStreamPrimitives<VertexType> &stream_primitives) :
			d_stream_primitives(stream_primitives),
			d_vertex_index(0)
		{  }

		void
		begin_triangles()
		{
			d_vertex_index = 0;
		}

		void
		add_vertex(
				const VertexType &vertex);

		void
		end_triangles()
		{
			// Nothing to do
		}

	private:
		GLStreamPrimitives<VertexType> &d_stream_primitives;
		VertexType d_triangles_vertices[2];
		unsigned int d_vertex_index;
	};


	/**
	 * Attach to @a GLStreamPrimitives to stream triangle strip primitives.
	 *
	 * 'VertexType' must be default-constructible.
	 */
	template <class VertexType>
	class GLStreamTriangleStrips
	{
	public:
		explicit
		GLStreamTriangleStrips(
				GLStreamPrimitives<VertexType> &stream_primitives) :
			d_stream_primitives(stream_primitives),
			d_vertex_index(0)
		{  }

		void
		begin_triangle_strip()
		{
			d_vertex_index = 0;
		}

		void
		add_vertex(
				const VertexType &vertex);

		void
		end_triangle_strip()
		{
			// Nothing to do
		}

	private:
		GLStreamPrimitives<VertexType> &d_stream_primitives;
		VertexType d_triangles_vertices[2];
		unsigned int d_vertex_index;
	};


	/**
	 * Attach to @a GLStreamPrimitives to stream triangle fan primitives.
	 *
	 * 'VertexType' must be default-constructible.
	 */
	template <class VertexType>
	class GLStreamTriangleFans
	{
	public:
		explicit
		GLStreamTriangleFans(
				GLStreamPrimitives<VertexType> &stream_primitives) :
			d_stream_primitives(stream_primitives),
			d_vertex_index(0)
		{  }

		void
		begin_triangle_fan()
		{
			d_vertex_index = 0;
		}

		void
		add_vertex(
				const VertexType &vertex);

		void
		end_triangle_fan()
		{
			// Nothing to do
		}

	private:
		GLStreamPrimitives<VertexType> &d_stream_primitives;
		VertexType d_triangles_vertices[2];
		unsigned int d_vertex_index;
	};
}

////////////////////
// Implementation //
////////////////////

namespace GPlatesOpenGL
{
	template <class VertexType>
	GLStreamPrimitives<VertexType>::GLStreamPrimitives(
			const GLVertexArray::VertexPointer &vertex_pointer,
			const boost::optional<GLVertexArray::ColorPointer> &colour_pointer,
			const boost::optional<GLVertexArray::NormalPointer> &normal_pointer,
			const boost::optional<std::vector<boost::optional<GLVertexArray::TexCoordPointer> > > &
					tex_coord_pointers) :
		d_vertex_pointer(vertex_pointer),
		d_colour_pointer(colour_pointer),
		d_normal_pointer(normal_pointer),
		d_tex_coord_pointers(tex_coord_pointers),
		d_current_primitive_type(PRIMITIVE_NONE)
	{
	}


	template <class VertexType>
	void
	GLStreamPrimitives<VertexType>::add_point(
			const VertexType &vertex)
	{
		// It's a new point so flush the currently batched primitives
		// if it's a new type of primitive.
		if (d_current_primitive_type != PRIMITIVE_POINT)
		{
			flush_primitives();
			d_current_primitive_type = PRIMITIVE_POINT;
		}

		// Need space for one vertex.
		if (d_current_vertices.size() > MAX_NUM_VERTICES_PER_DRAWABLE - 1)
		{
			flush_primitives();
		}

		const vertex_element_type vertex_index = d_current_vertices.size();

		// Add the point's vertex index.
		d_current_vertex_elements.push_back(vertex_index);

		// Add the point's vertex to the list of vertices.
		d_current_vertices.push_back(vertex);
	}


	template <class VertexType>
	void
	GLStreamPrimitives<VertexType>::add_line(
			const VertexType &start_vertex,
			const VertexType &end_vertex)
	{
		// It's a new line segment so flush the currently batched primitives
		// if it's a new type of primitive.
		if (d_current_primitive_type != PRIMITIVE_LINE)
		{
			flush_primitives();
			d_current_primitive_type = PRIMITIVE_LINE;
		}

		// Need space for at least two vertices.
		if (d_current_vertices.size() > MAX_NUM_VERTICES_PER_DRAWABLE - 2)
		{
			flush_primitives();
		}

		const vertex_element_type start_vertex_index = d_current_vertices.size();
		const vertex_element_type end_vertex_index = start_vertex_index + 1;

		// Add the line's vertex indices.
		d_current_vertex_elements.push_back(start_vertex_index);
		d_current_vertex_elements.push_back(end_vertex_index);

		// Add the line's vertices to the list of vertices.
		d_current_vertices.push_back(start_vertex);
		d_current_vertices.push_back(end_vertex);
	}


	template <class VertexType>
	void
	GLStreamPrimitives<VertexType>::add_line(
			const VertexType &end_vertex)
	{
		// Our start vertex is the end of the last line added.

		// Where in the middle of a line so the primitive type should not have changed.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_current_primitive_type == PRIMITIVE_LINE,
				GPLATES_ASSERTION_SOURCE);

		// Need space for at least one vertex (the end vertex).
		if (d_current_vertices.size() > MAX_NUM_VERTICES_PER_DRAWABLE - 1)
		{
			flush_primitives();
		}

		const vertex_element_type end_vertex_index = d_current_vertices.size();

		// If a new vertex array just got created it will have the last two vertices in it.
		const vertex_element_type start_vertex_index = end_vertex_index - 1;

		// Add the line's vertex indices.
		d_current_vertex_elements.push_back(start_vertex_index);
		d_current_vertex_elements.push_back(end_vertex_index);

		// Add the line's end vertex to the list of vertices.
		// The start vertex is already in there.
		d_current_vertices.push_back(end_vertex);
	}


	template <class VertexType>
	void
	GLStreamPrimitives<VertexType>::add_triangle(
			const VertexType &first_vertex,
			const VertexType &second_vertex,
			const VertexType &third_vertex)
	{
		// It's a new triangle so flush the currently batched primitives
		// if it's a new type of primitive.
		if (d_current_primitive_type != PRIMITIVE_TRIANGLE)
		{
			flush_primitives();
			d_current_primitive_type = PRIMITIVE_TRIANGLE;
		}

		// Need space for at least three vertices.
		if (d_current_vertices.size() > MAX_NUM_VERTICES_PER_DRAWABLE - 3)
		{
			flush_primitives();
		}

		const vertex_element_type first_vertex_index = d_current_vertices.size();

		// Add the triangle's vertex indices.
		d_current_vertex_elements.push_back(first_vertex_index);
		d_current_vertex_elements.push_back(first_vertex_index + 1);
		d_current_vertex_elements.push_back(first_vertex_index + 2);

		// Add the triangle's vertices to the list of vertices.
		d_current_vertices.push_back(first_vertex);
		d_current_vertices.push_back(second_vertex);
		d_current_vertices.push_back(third_vertex);
	}


	template <class VertexType>
	void
	GLStreamPrimitives<VertexType>::add_triangle(
			const VertexType &third_vertex)
	{
		// Where in the middle of a triangle so the primitive type should not have changed.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_current_primitive_type == PRIMITIVE_TRIANGLE,
				GPLATES_ASSERTION_SOURCE);

		// Need space for at least one vertex.
		if (d_current_vertices.size() > MAX_NUM_VERTICES_PER_DRAWABLE - 1)
		{
			flush_primitives();
		}

		const vertex_element_type third_vertex_index = d_current_vertices.size();

		// Add the triangle's vertex indices.
		// If a new vertex array just got created it will have the last two vertices in it.
		d_current_vertex_elements.push_back(third_vertex_index - 2);
		d_current_vertex_elements.push_back(third_vertex_index - 1);
		d_current_vertex_elements.push_back(third_vertex_index);

		// Add the triangle's third vertex to the list of vertices.
		d_current_vertices.push_back(third_vertex);
	}


	template <class VertexType>
	void
	GLStreamPrimitives<VertexType>::add_triangle_reversed(
			const VertexType &third_vertex)
	{
		// Where in the middle of a triangle so the primitive type should not have changed.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_current_primitive_type == PRIMITIVE_TRIANGLE,
				GPLATES_ASSERTION_SOURCE);

		// Need space for at least one vertex.
		if (d_current_vertices.size() > MAX_NUM_VERTICES_PER_DRAWABLE - 1)
		{
			flush_primitives();
		}

		const vertex_element_type third_vertex_index = d_current_vertices.size();

		// Add the triangle's vertex indices.
		// If a new vertex array just got created it will have the last two vertices in it.
		// Note the reverse of the first and second vertices compared to 'add_triangle()' above.
		d_current_vertex_elements.push_back(third_vertex_index - 1);
		d_current_vertex_elements.push_back(third_vertex_index - 2);
		d_current_vertex_elements.push_back(third_vertex_index);

		// Add the triangle's third vertex to the list of vertices.
		d_current_vertices.push_back(third_vertex);
	}


	template <class VertexType>
	void
	GLStreamPrimitives<VertexType>::add_triangle(
			const VertexType &first_vertex,
			const VertexType &third_vertex)
	{
		// Where in the middle of a triangle so the primitive type should not have changed.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_current_primitive_type == PRIMITIVE_TRIANGLE,
				GPLATES_ASSERTION_SOURCE);

		// Need space for at least two vertices.
		if (d_current_vertices.size() > MAX_NUM_VERTICES_PER_DRAWABLE - 2)
		{
			flush_primitives();
		}

		const vertex_element_type first_vertex_index = d_current_vertices.size();
		const vertex_element_type third_vertex_index = first_vertex_index + 1;

		// If a new vertex array just got created it will have the last two vertices in it.
		const vertex_element_type second_vertex_index = first_vertex_index - 1;

		// Add the triangle's vertex indices.
		d_current_vertex_elements.push_back(first_vertex_index);
		d_current_vertex_elements.push_back(second_vertex_index);
		d_current_vertex_elements.push_back(third_vertex_index);

		// Add the triangle's first and third vertices to the list of vertices.
		d_current_vertices.push_back(first_vertex);
		d_current_vertices.push_back(third_vertex);
	}


	template <class VertexType>
	boost::optional<GLDrawable::non_null_ptr_to_const_type>
	GLStreamPrimitives<VertexType>::get_drawable()
	{
		// Flush any current primitives to a drawable first.
		flush_primitives();

		if (d_drawables.empty())
		{
			return boost::none;
		}

		if (d_drawables.size() == 1)
		{
			// Return the single drawable.
			return d_drawables[0];
		}

		// More than one drawable was created so return a composite drawable.

		GLCompositeDrawable::non_null_ptr_type composite_drawable = GLCompositeDrawable::create();

		BOOST_FOREACH(const GLDrawable::non_null_ptr_to_const_type &drawable, d_drawables)
		{
			composite_drawable->add_drawable(drawable);
		}

		return static_cast<GLDrawable::non_null_ptr_to_const_type>(composite_drawable);
	}


	template <class VertexType>
	void
	GLStreamPrimitives<VertexType>::flush_primitives()
	{
		if (d_current_primitive_type == PRIMITIVE_NONE ||
			d_current_vertex_elements.empty())
		{
			// Nothing to do.
			return;
		}

		// If we have vertex indices then we should also have vertices.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				!d_current_vertices.empty(),
				GPLATES_ASSERTION_SOURCE);
		// Create a vertex array using the currently batched vertices.
		GPlatesOpenGL::GLVertexArray::shared_ptr_type vertex_array =
				GPlatesOpenGL::GLVertexArray::create(d_current_vertices);

		//
		// Specify the structure of the vertex as defined by the client.
		//

		vertex_array->gl_enable_client_state(GL_VERTEX_ARRAY);
		vertex_array->gl_vertex_pointer(d_vertex_pointer);

		if (d_colour_pointer)
		{
			vertex_array->gl_enable_client_state(GL_COLOR_ARRAY);
			vertex_array->gl_color_pointer(d_colour_pointer.get());
		}

		if (d_normal_pointer)
		{
			vertex_array->gl_normal_pointer(d_normal_pointer.get());
		}

		if (d_tex_coord_pointers)
		{
			const std::vector<boost::optional<GLVertexArray::TexCoordPointer> > &tex_coord_pointers =
					d_tex_coord_pointers.get();

			for (std::size_t n = 0; n < tex_coord_pointers.size(); ++n)
			{
				const boost::optional<GLVertexArray::TexCoordPointer> &tex_coord_pointer =
						tex_coord_pointers[n];
				if (tex_coord_pointer)
				{
					vertex_array->gl_client_active_texture_ARB(
							GLContext::get_texture_parameters().gl_texture0_ARB + n);
					vertex_array->gl_tex_coord_pointer(tex_coord_pointer.get());
				}
			}
		}

		// Create a vertex element array using the currently batched vertex indices.
		GPlatesOpenGL::GLVertexElementArray::non_null_ptr_type vertex_element_array =
				GPlatesOpenGL::GLVertexElementArray::create(d_current_vertex_elements);

		// Convert our primitive enumeration to corresponding the OpenGL enum.
		GLenum mode = GL_POINTS;
		switch (d_current_primitive_type)
		{
		case PRIMITIVE_POINT:
			mode = GL_POINTS;
			break;

		case PRIMITIVE_LINE:
			mode = GL_LINES;
			break;

		case PRIMITIVE_TRIANGLE:
			mode = GL_TRIANGLES;
			break;

		case PRIMITIVE_QUAD:
			mode = GL_QUADS;
			break;

		case PRIMITIVE_NONE:
		default:
			GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		}

		// Specify what to draw.
		vertex_element_array->gl_draw_range_elements_EXT(
				mode,
				0/*start*/,
				d_current_vertices.size() - 1/*end*/,
				d_current_vertex_elements.size()/*count*/,
				GL_UNSIGNED_SHORT/*type*/, // This must match the vertex_element_type
				0 /*indices_offset*/);

		// Create a drawable using the vertex array and vertex element array.
		GPlatesOpenGL::GLVertexArrayDrawable::non_null_ptr_to_const_type vertex_array_drawable =
				GPlatesOpenGL::GLVertexArrayDrawable::create(vertex_array, vertex_element_array);

		d_drawables.push_back(vertex_array_drawable);

		// Clear the primitives so we can start a new drawable.
		clear_primitives();
	}


	template <class VertexType>
	void
	GLStreamPrimitives<VertexType>::clear_primitives()
	{
		// Copy the last two vertices (if any) to the beginning of our new vertex list.
		// We do this because we might be in the middle of a line strip, line loop,
		// triangle strip, triangle fan or quad strip where vertices for the next line,
		// triangle or quad are contained in the previous vertex array.
		vertex_seq_type end_of_last_vertex_array;
		const int num_vertices_to_copy =
				(std::min)(static_cast<std::size_t>(2), d_current_vertices.size());
		typename vertex_seq_type::iterator copy_vertex_begin_iter = d_current_vertices.end();
		std::advance(copy_vertex_begin_iter, -num_vertices_to_copy);
		std::copy(
				copy_vertex_begin_iter,
				d_current_vertices.end(),
				std::back_inserter(end_of_last_vertex_array));

		// Clear the vertices.
		d_current_vertices.clear();
		// Add the last two vertices if any.
		d_current_vertices.insert(d_current_vertices.end(),
				end_of_last_vertex_array.begin(), end_of_last_vertex_array.end());

		// Clear the vertex indices.
		d_current_vertex_elements.clear();
	}


	template <class VertexType>
	void
	GLStreamLines<VertexType>::add_vertex(
			const VertexType &vertex)
	{
		if (d_first_vertex)
		{
			// Store vertex for later.
			d_start_vertex = vertex;
			d_first_vertex = false;
		}
		else
		{
			d_stream_primitives.add_line(d_start_vertex, vertex);
			d_first_vertex = true;
		}
	}


	template <class VertexType>
	void
	GLStreamLineStrips<VertexType>::add_vertex(
			const VertexType &vertex)
	{
		if (d_vertex_index >= 2) // Test most common case first...
		{
			// Add a line using the current vertex and the last vertex added (implicit).
			d_stream_primitives.add_line(vertex);
			// No need to increment vertex index anymore.
		}
		else if (d_vertex_index == 0)
		{
			d_start_vertex = vertex;
			++d_vertex_index;
		}
		else // d_vertex_index == 1
		{
			d_stream_primitives.add_line(d_start_vertex, vertex);
			++d_vertex_index;
		}
	}
	

	template <class VertexType>
	void
	GLStreamLineLoops<VertexType>::add_vertex(
			const VertexType &vertex)
	{
		if (d_vertex_index >= 2) // Test most common case first...
		{
			// Add a line using the current vertex and the last vertex added (implicit).
			d_stream_primitives.add_line(vertex);
			// No need to increment vertex index anymore.
		}
		else if (d_vertex_index == 0)
		{
			d_start_vertex = vertex;
			++d_vertex_index;
		}
		else // d_vertex_index == 1
		{
			d_stream_primitives.add_line(d_start_vertex, vertex);
			++d_vertex_index;
		}
	}


	template <class VertexType>
	void
	GLStreamLineLoops<VertexType>::end_line_loop()
	{
		// Only need to close the loop if more than two vertices.
		if (d_vertex_index > 2)
		{
			// Add a line using the last vertex added (implicit) and the start vertex.
			d_stream_primitives.add_line(d_start_vertex);
		}
	}
	

	template <class VertexType>
	void
	GLStreamTriangles<VertexType>::add_vertex(
			const VertexType &vertex)
	{
		if (d_vertex_index < 2)
		{
			// The first or second vertex of the triangle.
			d_triangles_vertices[d_vertex_index] = vertex;
			++d_vertex_index;
		}
		else // d_vertex_index == 2
		{
			// We have the third triangle vertex so submit the triangle.
			d_stream_primitives.add_triangle(
					d_triangles_vertices[0],
					d_triangles_vertices[1],
					vertex);
			d_vertex_index = 0;
		}
	}
	

	template <class VertexType>
	void
	GLStreamTriangleStrips<VertexType>::add_vertex(
			const VertexType &vertex)
	{
		if (d_vertex_index > 2) // Handle most common case first...
		{
			if ((d_vertex_index & 1) == 0)
			{
				d_stream_primitives.add_triangle(vertex);
			}
			else
			{
				d_stream_primitives.add_triangle_reversed(vertex);
			}
			++d_vertex_index;
		}
		else if (d_vertex_index < 2)
		{
			// The first or second vertex of the triangle strip.
			d_triangles_vertices[d_vertex_index] = vertex;
			++d_vertex_index;
		}
		else // d_vertex_index == 2
		{
			// We have the third triangle vertex so submit our first triangle.
			d_stream_primitives.add_triangle(
					d_triangles_vertices[0],
					d_triangles_vertices[1],
					vertex);
			++d_vertex_index;
		}
	}
	

	template <class VertexType>
	void
	GLStreamTriangleFans<VertexType>::add_vertex(
			const VertexType &vertex)
	{
		if (d_vertex_index > 2) // Handle most common case first...
		{
			// Add a triangle using its first and third vertices.
			// Its second vertex will be the third vertex of the last triangles added.
			// The reason for specifying the first vertex of the triangle (even though
			// its already been specified by a previous triangle) is because a triangle
			// must fall within a finite contiguous range of vertices and that is because
			// we need a finite overlap when we move from one vertex array to another.
			// A triangle fan has an arbitrary overlap unless we re-specify the fan apex
			// vertex for each triangle of the fan.
			d_stream_primitives.add_triangle(d_triangles_vertices[0], vertex);
			// No need to increment vertex index anymore.
		}
		else if (d_vertex_index < 2)
		{
			// The first or second vertex of the triangle strip.
			d_triangles_vertices[d_vertex_index] = vertex;
			++d_vertex_index;
		}
		else // d_vertex_index == 2
		{
			// We have the third triangle vertex so submit our first triangle.
			d_stream_primitives.add_triangle(
					d_triangles_vertices[0],
					d_triangles_vertices[1],
					vertex);
			++d_vertex_index;
		}
	}
}

#endif // GPLATES_OPENGL_GLSTREAMPRIMITIVES_H
