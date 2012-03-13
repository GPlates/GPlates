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

#include <algorithm> // std::swap
#include <iterator>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <opengl/OpenGL.h>

#include "GLContext.h"
#include "GLStreamPrimitiveWriters.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesOpenGL
{
	/**
	 * Use this when you want to stream points, lines, line strips, triangles, triangle strips or
	 * triangle fans into the write-only memory of a vertex buffer and a vertex element buffer.
	 *
	 * It is based on the familiar immediate mode rendering interface of 'glBegin' / 'glEnd'.
	 *
	 * NOTE: The streamed data is written as indexed points, lines or triangles (ie, no fans or strips
	 * even if the input data was in that form).
	 * So the 'mode' parameter of glDrawRangeElements, for example, is always one of:
	 *  - GL_POINTS,
	 *  - GL_LINES, or
	 *  - GL_TRIANGLES.
	 * NOTE: When using the nested class 'Quads' it is still streamed as GL_TRIANGLES.
	 *
	 * This class does take advantage of vertex re-use (via vertex indices) but it is limited
	 * to the re-use you get with line strips/loops and triangles strips/fans.
	 * If you have a complex mesh you might be better off generating your own vertex and index
	 * buffers to take maximal advantage of vertex indexing and hence vertex re-use.
	 * Graphics cards usually have a small cache of recently transformed vertices and if
	 * a previously transformed vertex is indexed again relatively soon then it can
	 * access the transformed vertex cache for a speed improvement.
	 *
	 * 'VertexType' represents the vertex attribute data to be stored in the vertex buffer memory.
	 * It must be default-constructible.
	 *
	 * 'VertexElementType' is the integer type used to represent vertex elements (indices) and
	 * must be one of 'GLuint', 'GLushort' or 'GLubyte'.
	 *
	 * 'StreamWriterType' must be a template class (doesn't need to be copyable though) with the following interface:
	 *
	 *   template <typename StreamElementType>
	 *   class StreamWriter
	 *   {
	 *   public:
	 *     // Writes 'stream_element' to the stream.
	 *     void write(const StreamElementType &stream_element);
	 *
	 *     // Returns number of elements written so far.
	 *     unsigned int count() const;
	 *
	 *     // Returns number of elements that can be written before stream is full.
	 *     // For continuously growing streams (eg, std::vector) this could be max int.
	 *     unsigned int remaining() const;
	 *   };
	 *
	 * For convenience there are two GLStreamPrimitive classes with different stream writers:
	 * 1) GLStaticStreamPrimitives - writes to static (fixed size) vertex/index buffers, and
	 * 2) GLDynamicStreamPrimitives - writes to dynamic (std::vector) vertex/index buffers.
	 */
	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	class GLStreamPrimitives :
			private boost::noncopyable
	{
	public:
		//! Typedef for this class.
		typedef GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType> stream_primitives_type;

		//! Typedef for a vertex index.
		typedef VertexElementType vertex_element_type;

		//! Typedef for the vertex type.
		typedef VertexType vertex_type;


		//! Constructor.
		GLStreamPrimitives() :
			d_begin_vertex_stream_count(0),
			d_begin_vertex_element_stream_count(0)
		{  }


		/**
		 * Attach to @a GLStreamPrimitives to stream point primitives.
		 */
		class Points
		{
		public:
			explicit
			Points(
					stream_primitives_type &stream_primitives) :
				d_stream_primitives(&stream_primitives)
			{  }

			void
			begin_points()
			{
				// Nothing to do
			}

			/**
			 * Adds a point.
			 *
			 * Returns false if the vertex buffer (or vertex element buffer) is full.
			 * In this case you only need to re-submit the last vertex submitted (ie, @a vertex in the
			 * call that failed) in order to continue streaming into the same begin/end pair -
			 * once new buffers have been provided.
			 */
			bool
			add_vertex(
					const vertex_type &vertex)
			{
				return d_stream_primitives->add_point(vertex);
			}

			void
			end_points()
			{
				// Nothing to do
			}

		private:
			stream_primitives_type *d_stream_primitives;
		};

		/**
		 * Attach to @a GLStreamPrimitives to stream line primitives.
		 */
		class Lines
		{
		public:
			explicit
			Lines(
					stream_primitives_type &stream_primitives) :
				d_stream_primitives(stream_primitives),
				d_first_vertex(true)
			{  }

			void
			begin_lines()
			{
				d_first_vertex = true;
			}

			/**
			 * Adds a start or end vertex depending (1st, 3rd, 5th, etc adds are start vertices).
			 *
			 * Returns false if the vertex buffer (or vertex element buffer) is full.
			 * In this case you only need to re-submit the last vertex submitted (ie, @a vertex in the
			 * call that failed) in order to continue streaming into the same begin/end pair -
			 * once new buffers have been provided.
			 *
			 * NOTE: If you only add one vertex to a @a begin_lines / @a end_lines pair then it
			 * doesn't get output to the stream.
			 * If you output an odd number of vertices then only the even number get output (for same reason).
			 */
			bool
			add_vertex(
					const vertex_type &vertex);

			void
			end_lines()
			{
				// Nothing to do
			}

		private:
			stream_primitives_type &d_stream_primitives;
			vertex_type d_start_vertex;
			bool d_first_vertex;
		};

		/**
		 * Attach to @a GLStreamPrimitives to stream line strip primitives.
		 */
		class LineStrips
		{
		public:
			explicit
			LineStrips(
					stream_primitives_type &stream_primitives) :
				d_stream_primitives(stream_primitives),
				d_vertex_index(0)
			{  }

			void
			begin_line_strip()
			{
				d_vertex_index = 0;
			}

			/**
			 * The first two vertices of a strip contribute one line segment whereas subsequent
			 * vertices contribute one line segment (due to vertex sharing).
			 *
			 * Returns false if the vertex buffer (or vertex element buffer) is full.
			 * In this case you only need to re-submit the last vertex submitted (ie, @a vertex in the
			 * call that failed) in order to continue streaming into the same begin/end pair -
			 * once new buffers have been provided.
			 *
			 * NOTE: If you only add one vertex to a line strip then it doesn't get output to the stream.
			 */
			bool
			add_vertex(
					const vertex_type &vertex);

			void
			end_line_strip()
			{
				// Nothing to do
			}

		private:
			stream_primitives_type &d_stream_primitives;
			vertex_type d_start_vertex;
			vertex_type d_last_shared_vertex;
			unsigned int d_vertex_index;
		};

		/**
		 * Attach to @a GLStreamPrimitives to stream line loop primitives.
		 */
		class LineLoops
		{
		public:
			explicit
			LineLoops(
					stream_primitives_type &stream_primitives) :
				d_line_strips(stream_primitives),
				d_vertex_index(0),
				d_closed_line_loop(true)
			{  }

			void
			begin_line_loop();

			/**
			 * The first two vertices of a loop contribute one line segment whereas subsequent
			 * vertices contribute one line segment (due to vertex sharing) and when the line loop
			 * is ended (with @a end_line_loop) the start vertex is output to close the loop.
			 *
			 * Returns false if the vertex buffer (or vertex element buffer) is full.
			 * In this case you only need to re-submit the last vertex submitted (ie, @a vertex in the
			 * call that failed) in order to continue streaming into the same begin/end pair -
			 * once new buffers have been provided.
			 *
			 * NOTE: If you only add one vertex to a @a begin_lines / @a end_lines pair then it
			 * doesn't get output to the stream.
			 */
			bool
			add_vertex(
					const vertex_type &vertex);

			/**
			 * The last line segment is formed from the last and first vertices added if at least
			 * three vertices have been added to the loop, otherwise it's not closed off (because
			 * it's a single line segment, or no line segment if only zero or one vertex added).
			 *
			 * Returns false if the vertex buffer (or vertex element buffer) is full.
			 * In this case you need to call @a end_line_loop again (see comment in @a add_vertex).
			 */
			bool
			end_line_loop();

		private:
			LineStrips d_line_strips;
			vertex_type d_start_vertex;
			unsigned int d_vertex_index;
			bool d_closed_line_loop;
		};

		/**
		 * Attach to @a GLStreamPrimitives to stream triangle primitives.
		 */
		class Triangles
		{
		public:
			explicit
			Triangles(
					stream_primitives_type &stream_primitives) :
				d_stream_primitives(stream_primitives),
				d_vertex_index(0)
			{  }

			void
			begin_triangles()
			{
				d_vertex_index = 0;
			}

			/**
			 * Add triangles as groups of three vertices.
			 *
			 * Returns false if the vertex buffer (or vertex element buffer) is full.
			 * In this case you only need to re-submit the last vertex submitted (ie, @a vertex in the
			 * call that failed) in order to continue streaming into the same begin/end pair -
			 * once new buffers have been provided.
			 */
			bool
			add_vertex(
					const vertex_type &vertex);

			void
			end_triangles()
			{
				// Nothing to do
			}

		private:
			stream_primitives_type &d_stream_primitives;
			vertex_type d_triangles_vertices[2];
			unsigned int d_vertex_index;
		};

		/**
		 * Attach to @a GLStreamPrimitives to stream triangle strip primitives.
		 */
		class TriangleStrips
		{
		public:
			explicit
			TriangleStrips(
					stream_primitives_type &stream_primitives) :
				d_stream_primitives(stream_primitives),
				d_vertex_index(0),
				d_reverse_triangle_winding(0)
			{  }

			void
			begin_triangle_strip()
			{
				d_vertex_index = d_reverse_triangle_winding = 0;
			}

			/**
			 * Add triangles as a triangle strip - the first two vertices start the strip and
			 * subsequent vertices continue the extend the strip (one triangle per vertex).
			 *
			 * Returns false if the vertex buffer (or vertex element buffer) is full.
			 * In this case you only need to re-submit the last vertex submitted (ie, @a vertex in the
			 * call that failed) in order to continue streaming into the same begin/end pair -
			 * once new buffers have been provided.
			 */
			bool
			add_vertex(
					const vertex_type &vertex);

			void
			end_triangle_strip()
			{
				// Nothing to do
			}

		private:
			stream_primitives_type &d_stream_primitives;
			vertex_type d_triangles_vertices[2];
			unsigned int d_vertex_index;

			//! Only needed in case vertex buffer is full and user re-submits an odd-numbered vertex.
			unsigned int d_reverse_triangle_winding;
		};

		/**
		 * Attach to @a GLStreamPrimitives to stream triangle fan primitives.
		 */
		class TriangleFans
		{
		public:
			explicit
			TriangleFans(
					stream_primitives_type &stream_primitives) :
				d_stream_primitives(stream_primitives),
				d_vertex_index(0),
				d_apex_vertex_element(0)
			{  }

			void
			begin_triangle_fan()
			{
				d_vertex_index = 0;
			}

			/**
			 * The first vertex added to a triangle fan is the fan apex and subsequent vertices
			 * form the boundary of the fan.
			 *
			 * Returns false if the vertex buffer (or vertex element buffer) is full.
			 * In this case you only need to re-submit the last vertex submitted (ie, @a vertex in the
			 * call that failed) in order to continue streaming into the same begin/end pair -
			 * once new buffers have been provided.
			 */
			bool
			add_vertex(
					const vertex_type &vertex);

			void
			end_triangle_fan()
			{
				// Nothing to do
			}

		private:
			stream_primitives_type &d_stream_primitives;
			vertex_type d_triangles_vertices[2];
			unsigned int d_vertex_index;
			unsigned int d_apex_vertex_element;
		};

		/**
		 * Attach to @a GLStreamPrimitives to stream individual vertex-indexed triangle meshes.
		 *
		 * This supports user-specified vertex indexing rather than the implicit indexing in the other
		 * primitive types (such as triangle strips and fans) and hence allows for more complex primitives.
		 *
		 * NOTE: This can stream meshes that are larger than the stream buffers - although this
		 * is not a good idea.
		 */
		class TriangleMeshes
		{
		public:
			explicit
			TriangleMeshes(
					stream_primitives_type &stream_primitives) :
				d_stream_primitives(stream_primitives),
				d_base_vertex_element(0),
				d_reordered_vertices(false)
			{  }

			void
			begin_mesh()
			{
				d_mesh_vertices.clear();
				d_reordered_mesh_vertex_elements.clear();
				d_base_vertex_element = d_stream_primitives.get_base_vertex_element();
				d_reordered_vertices = false;
			}

			/**
			 * Adds the next vertex to the mesh.
			 *
			 * Returns false if the vertex buffer is full.
			 * In this case you only need to re-submit the last vertex submitted (ie, the @a vertex
			 * in the call that failed) in order to continue streaming into the same begin/end pair
			 * - once a new buffer has been provided.
			 */
			bool
			add_vertex(
					const vertex_type &vertex);

			/**
			 * Adds a triangle to the mesh.
			 *
			 * The triangle references three vertices already added using @a add_vertex.
			 *
			 * For example, a quad could be:
			 *   begin_mesh();
			 *   add_vertex(); // vertex 0
			 *   add_vertex(); // vertex 1
			 *   add_vertex(); // vertex 2
			 *   add_triangle(0, 1, 2);
			 *   add_vertex(); // vertex 3
			 *   add_triangle(0, 2, 3);
			 *   end_mesh();
			 *
			 * NOTE: The vertices indexed by this triangle *must* have already been added.
			 *
			 * It is also best to add a triangle as soon as its vertices have been added.
			 * For example, if you add all the vertices first and then add the triangles last then it's
			 * possible that the vertex buffer filled up while adding the vertices - in which case
			 * when the triangles are added the vertices (which are buffered internally) must
			 * be re-added to the vertex buffer (effectively wasting the originally added vertices).
			 * However for small meshes and reasonably large vertex buffers this will happen
			 * relatively infrequently.
			 *
			 * Returns false if the vertex buffer (or vertex element buffer) is full.
			 * In this case you only need to re-submit the last triangle submitted (ie, the three
			 * vertex elements in the call that failed) in order to continue streaming into the
			 * same begin/end pair - once the new buffers have been provided.
			 */
			bool
			add_triangle(
					vertex_element_type first_vertex_element,
					vertex_element_type second_vertex_element,
					vertex_element_type third_vertex_element);

			void
			end_mesh()
			{
				// Nothing to do
			}

		private:
			stream_primitives_type &d_stream_primitives;
			std::vector<vertex_type> d_mesh_vertices;
			std::vector< boost::optional<vertex_element_type> > d_reordered_mesh_vertex_elements;
			unsigned int d_base_vertex_element;
			bool d_reordered_vertices;

			void
			reorder_vertices();
		};

		/**
		 * Attach to @a GLStreamPrimitives to stream quad primitives.
		 *
		 * WARNING: Even though quad primitives are streamed in, the final output is in the form of
		 * triangles (ie, GL_TRIANGLES) and should *not* be rendered as GL_QUADS.
		 * The reason for this is the graphics hardware breaks everything down to triangles anyway
		 * (and, for this reason, OpenGL 3 has deprecated GL_QUADS) - most current hardware has
		 * post transform-and-lighting hardware to convert quads to triangles but there is no
		 * guarantee this special-purpose hardware will be there in future hardware.
		 */
		class Quads
		{
		public:
			explicit
			Quads(
					stream_primitives_type &stream_primitives) :
				d_stream_primitives(stream_primitives),
				d_vertex_index(0)
			{  }

			void
			begin_quads()
			{
				d_vertex_index = 0;
			}

			/**
			 * Add quads as groups of four vertices.
			 *
			 * Returns false if the vertex buffer (or vertex element buffer) is full.
			 * In this case you only need to re-submit the last vertex submitted (ie, @a vertex in the
			 * call that failed) in order to continue streaming into the same begin/end pair -
			 * once new buffers have been provided.
			 *
			 * WARNING: Even though quad primitives are streamed in, the final output is in the form of
			 * triangles (ie, GL_TRIANGLES) and should *not* be rendered as GL_QUADS.
			 */
			bool
			add_vertex(
					const vertex_type &vertex);

			void
			end_quads()
			{
				// Nothing to do
			}

		private:
			stream_primitives_type &d_stream_primitives;
			vertex_type d_quads_vertices[3];
			unsigned int d_vertex_index;
		};


		/**
		 * Attach to @a GLStreamPrimitives to stream arbitrary primitives where the stream overflow
		 * check is done at the beginning of the primitive instead of checking at each vertex.
		 *
		 * The type of primitives (points, lines, line strips, triangles, triangle fans, triangle meshes, etc)
		 * is up to the caller to define using appropriate calls to @a add_vertex and @a add_vertex_element.
		 *
		 * NOTE: This class is for performance critical code where the overhead of checking for stream
		 * overflow at each vertex is too costly (eg, when streaming very large numbers of simple primitives).
		 * Otherwise it's easier to just use the other nested classes.
		 */
		class Primitives
		{
		public:
			explicit
			Primitives(
					stream_primitives_type &stream_primitives) :
				d_stream_primitives(stream_primitives),
				d_base_vertex_element(0)
			{  }

			/**
			 * Returns true if the stream has enough space to accommodate the specified number
			 * of vertices and vertex elements.
			 *
			 * You should limit calls to @a add_vertex and @a add_vertex_element to the specified maximum values.
			 *
			 * If false is returned then you'll need to stop streaming, render what has been streamed
			 * so far and then start streaming again with new buffers.
			 */
			bool
			begin_primitive(
					unsigned int max_num_vertices,
					unsigned int max_num_vertex_elements)
			{
				// Make sure we are currently streaming vertices/indices to a stream target.
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_stream_primitives.d_vertex_stream && d_stream_primitives.d_vertex_element_stream,
						GPLATES_ASSERTION_SOURCE);

				d_base_vertex_element = d_stream_primitives.get_base_vertex_element();

				// Return true if there is enough space for the specified number of vertices and vertex elements.
				return d_stream_primitives.d_vertex_stream->remaining() >= max_num_vertices &&
					d_stream_primitives.d_vertex_element_stream->remaining() >= max_num_vertex_elements;
			}

			/**
			 * Adds a vertex to the current primitive.
			 *
			 * NOTE: This does *not* check for overflow in the stream buffer (and hence has no return value).
			 */
			void
			add_vertex(
					const vertex_type &vertex)
			{
				d_stream_primitives.d_vertex_stream->write(vertex);
			}

			/**
			 * Adds a vertex element to the current primitive.
			 *
			 * NOTE: This does *not* check for overflow in the stream buffer (and hence has no return value).
			 */
			void
			add_vertex_element(
					vertex_element_type vertex_element)
			{
				d_stream_primitives.d_vertex_element_stream->write(d_base_vertex_element + vertex_element);
			}

			void
			end_primitive()
			{
				// Nothing to do
			}

		private:
			stream_primitives_type &d_stream_primitives;
			unsigned int d_base_vertex_element;
		};


		/**
		 * RAII class to start and stop streaming over a scope and also to temporarily interrupt
		 * streaming when the vertex buffer or vertex element buffer is full (or when client
		 * decides to render the stream contents).
		 */
		class StreamTarget :
				private boost::noncopyable
		{
		public:
			//! Constructor - does *not* start streaming.
			StreamTarget(
					stream_primitives_type &stream_primitives);

			//! Stops streaming (if has been started but not stopped).
			~StreamTarget();

			/**
			 * Starts a target for streaming primitives (a vertex buffer and vertex element buffer).
			 *
			 * The constructor arguments to StreamWriterType (for vertices) should be passed into
			 * @a vertex_stream_writer_constructor_args using 'boost::in_place(a, b, c, ...)' where
			 * a, b, c, etc are the constructor arguments.
			 * The same principle applies to the StreamWriterType for vertex elements.
			 *
			 * For example, for a @a GLDynamicStreamPrimitives:
			 *
			 *   std::vector<vertex_type> vertices;
			 *   std::vector<vertex_element_type> vertex_elements;
			 *   GLDynamicStreamPrimitives<vertex_type, vertex_element_type>::StreamTarget stream_target(stream);
			 *   stream_target.start_streaming(
			 *      boost::in_place(boost::ref(vertices)),
			 *      boost::in_place(boost::ref(vertex_elements)));
			 *
			 * ...note, in the above example, that boost::ref was only needed because we want to
			 * pass a *non-const* reference instead of a const reference (it wasn't to avoid
			 * copying the vertices because boost::in_place doesn't do that).
			 *
			 * NOTE: A typical usage of this method is to use it with 'GLBuffer::gl_map_buffer_stream' on
			 * a vertex buffer and a vertex element buffer and then call 'GLBuffer::gl_flush_buffer_stream'
			 * and 'GLBuffer::gl_unmap_buffer' after calling @a stop_streaming.
			 * For this you would use @a GLStaticStreamPrimitives.
			 *
			 * This way @a GLStreamPrimitives is used to fill up a vertex buffer and vertex element
			 * buffer which can then be used for rendering.
			 *
			 * Note that a single begin/end pair of a streaming primitive (such as
			 * 'LineStrips::begin_line_strips' / 'LineStrips::end_line_strips') can be interrupted
			 * by an @a stop_streaming followed by a @a start_streaming (for example, to allocate
			 * a new buffer for vertices/indices). However streaming calls to @a LineStrips,
			 * for example, can only happen between a @a start_streaming / @a stop_streaming pair.
			 * For example:
			 *    GLStaticStreamPrimitives<...> stream_primitives;
			 *    GLStaticStreamPrimitives<...>::StreamTarget stream_target(stream_primitives);
			 *    stream_target.start_streaming(...);
			 *      GLStaticStreamPrimitives<...>::LineStrips line_strips(stream_primives);
			 *      line_strips.begin_line_strip();
			 *        ...
			 *        if (!line_strips.add_vertex(vertex))
			 *        {
			 *          stream_target.stop_streaming();
			 *          draw(); // Vertex buffer is full so draw using it now.
			 *          ... // Allocate a new buffer to continue streaming into.
			 *          stream_target.start_streaming(...); // Pass in the new buffer(s).
			 *          line_strips.add_vertex(vertex); // Re-submit the same vertex to continue.
			 *        }
			 *        ...
			 *      line_strips.end_line_strip();
			 *    stream_target.stop_streaming();
			 *    draw(); // Draw using the vertex buffer streamed to.
			 */
			template <class VertexStreamWriterConstructorArgs, class VertexElementStreamWriterConstructorArgs>
			void
			start_streaming(
					const VertexStreamWriterConstructorArgs &vertex_stream_writer_constructor_args,
					const VertexElementStreamWriterConstructorArgs &vertex_element_stream_writer_constructor_args);

			/**
			 * Stops a target for streaming primitives.
			 *
			 * If you used @a Points for streaming then draw the streamed vertices with GL_POINTS.
			 * If you used @a Lines or @a LineStrips for streaming then draw the streamed vertices with GL_LINES.
			 * If you used @a Triangles, @a TriangleStrips or @a TriangleFans for streaming then draw
			 * the streamed vertices with GL_TRIANGLES.
			 *
			 * NOTE: If you use @a Quads then draw the streamed vertices with GL_TRIANGLES (not GL_QUADS).
			 */
			void
			stop_streaming();

			/**
			 * Returns the count of vertices already in the stream writer at the last call to @a start_streaming.
			 */
			unsigned int
			get_start_streaming_vertex_count() const;

			/**
			 * Returns the count of vertex elements already in the stream writer at the last call to @a start_streaming.
			 */
			unsigned int
			get_start_streaming_vertex_element_count() const;

			/**
			 * Returns the number of vertices streamed since the last call to @a start_streaming.
			 *
			 * NOTE: If called after @a stop_streaming (and before a subsequent @a start_streaming) then
			 * returns the number streamed when @a stop_streaming was called.
			 */
			unsigned int
			get_num_streamed_vertices() const;

			/**
			 * Returns the number of vertex elements streamed since the last call to @a start_streaming.
			 *
			 * NOTE: If called after @a stop_streaming (and before a subsequent @a start_streaming) then
			 * returns the number streamed when @a stop_streaming was called.
			 */
			unsigned int
			get_num_streamed_vertex_elements() const;

		private:
			stream_primitives_type &d_stream_primitives;
			unsigned int d_start_streaming_vertex_count;
			unsigned int d_start_streaming_vertex_element_count;
			unsigned int d_num_streamed_vertices;
			unsigned int d_num_streamed_vertex_elements;
			bool d_targeting_stream;
		};

	private:
		//! Typedef for a vertex stream writer.
		typedef StreamWriterType<vertex_type> vertex_stream_writer_type;

		//! Typedef for a vertex element stream writer.
		typedef StreamWriterType<vertex_element_type> vertex_element_stream_writer_type;


		/**
		 * Where the output vertices are written (a write-only wrapper around vertex buffer memory).
		 *
		 * Only valid between @a begin_streaming / @a end_streaming.
		 */
		boost::optional<vertex_stream_writer_type> d_vertex_stream;

		/**
		 * The vertex stream count at the most recent call to @a begin_streaming.
		 */
		unsigned int d_begin_vertex_stream_count;

		/**
		 * Where the output vertex elements are written (a write-only wrapper around vertex element buffer memory).
		 *
		 * Only valid between @a begin_streaming / @a end_streaming.
		 */
		boost::optional<vertex_element_stream_writer_type> d_vertex_element_stream;

		/**
		 * The vertex element stream count at the most recent call to @a begin_streaming.
		 */
		unsigned int d_begin_vertex_element_stream_count;


		/**
		 * Starts a target for streaming primitives (a vertex buffer and vertex element buffer).
		 */
		template <class VertexStreamWriterConstructorArgs, class VertexElementStreamWriterConstructorArgs>
		void
		begin_streaming(
				const VertexStreamWriterConstructorArgs &vertex_stream_writer_constructor_args,
				const VertexElementStreamWriterConstructorArgs &vertex_element_stream_writer_constructor_args,
				unsigned int &begin_streaming_vertex_count,
				unsigned int &begin_streaming_vertex_element_count);

		/**
		 * Returns the number of vertices currently in the vertex stream writer.
		 *
		 * NOTE: Only valid when called between @a begin_streaming / @a end_streaming.
		 */
		unsigned int
		get_base_vertex_element() const;

		/**
		 * Returns the number of vertices streamed since the last call to @a begin_streaming.
		 *
		 * NOTE: Only valid when called between @a begin_streaming / @a end_streaming.
		 */
		unsigned int
		get_num_streamed_vertices() const;

		/**
		 * Returns the number of vertex elements streamed since the last call to @a begin_streaming.
		 *
		 * NOTE: Only valid when called between @a begin_streaming / @a end_streaming.
		 */
		unsigned int
		get_num_streamed_vertex_elements() const;

		/**
		 * Stops a target for streaming primitives and returned the number of vertices/indices streamed
		 * since the last call to @a begin_streaming.
		 */
		void
		end_streaming(
				unsigned int &num_streamed_vertices,
				unsigned int &num_streamed_vertex_elements);

		/**
		 * Adds a point primitive.
		 *
		 * Returns false if the vertex buffer (or vertex element buffer) is full.
		 */
		bool
		add_point(
				const vertex_type &vertex);

		/**
		 * Adds a line primitive with the specified start and end vertices.
		 *
		 * Returns false if the vertex buffer (or vertex element buffer) is full.
		 */
		bool
		add_line(
				const vertex_type &start_vertex,
				const vertex_type &end_vertex);

		/**
		 * Adds a line primitive whose start vertex is the end of the last line added.
		 *
		 * This methods supports line strips and line loops.
		 *
		 * Returns false if the vertex buffer (or vertex element buffer) is full.
		 */
		bool
		add_line(
				const vertex_type &end_vertex);

		/**
		 * Adds a triangle primitive with the specified vertices.
		 *
		 * Returns false if the vertex buffer (or vertex element buffer) is full.
		 */
		bool
		add_triangle(
				const vertex_type &first_vertex,
				const vertex_type &second_vertex,
				const vertex_type &third_vertex);

		/**
		 * Adds a triangle primitive with the specified vertices.
		 *
		 * However the triangle winding order is the reverse of the normal order.
		 * The triangles vertex indices are ordered as:
		 *   (second_vertex, first_vertex, third_vertex)
		 *
		 * This methods supports triangle strips.
		 *
		 * Returns false if the vertex buffer (or vertex element buffer) is full.
		 */
		bool
		add_triangle_reversed(
				const vertex_type &first_vertex,
				const vertex_type &second_vertex,
				const vertex_type &third_vertex);

		/**
		 * Adds a triangle primitive whose third vertex is @a third_vertex.
		 *
		 * Its first and second vertices are the second-last and last vertices added
		 * with the previous @a add_triangle call(s).
		 *
		 * This methods supports triangle strips.
		 *
		 * Returns false if the vertex buffer (or vertex element buffer) is full.
		 */
		bool
		add_triangle(
				const vertex_type &third_vertex);

		/**
		 * Adds a triangle primitive whose third vertex is @a third_vertex.
		 *
		 * Its first and second vertices are the last and second-last vertices added
		 * with the previous @a add_triangle call(s).
		 * Note the reverse of the first and second vertices compared to @a add_triangle above.
		 *
		 * This methods supports triangle strips.
		 *
		 * Returns false if the vertex buffer (or vertex element buffer) is full.
		 */
		bool
		add_triangle_reversed(
				const vertex_type &third_vertex);

		/**
		 * Adds a triangle primitive whose first and third vertices
		 * are @a first_vertex and @a third_vertex.
		 *
		 * Its second vertex is the last vertex added with the previous @a add_triangle call(s).
		 *
		 * This methods supports triangle fans.
		 *
		 * Returns false if the vertex buffer (or vertex element buffer) is full.
		 */
		bool
		add_triangle(
				const vertex_type &first_vertex,
				const vertex_type &third_vertex);


		/**
		 * Adds a vertex only (no vertex elements are added).
		 *
		 * This method should be used with the overload of @a add_triangle accepting vertex *elements*.
		 *
		 * Returns false if the vertex buffer is full.
		 */
		bool
		add_vertex(
				const vertex_type &vertex);

		/**
		 * Adds a triangle primitive by indexing the specified vertices (and added with @a add_vertex).
		 *
		 * The indices are relative to the beginning of the stream writer (*not* relative to when
		 * @a begin_streaming is called) because they must be relative to the beginning of storage).
		 *
		 * Returns false if the vertex element buffer is full.
		 */
		bool
		add_triangle(
				vertex_element_type first_vertex_element,
				vertex_element_type second_vertex_element,
				vertex_element_type third_vertex_element);
	};


	/**
	 * A type of @a GLStreamPrimitives that writes to static (fixed size) vertex/index buffers.
	 *
	 * It is defined via inheritance since C++ does not support templated typedefs.
	 */
	template <class VertexType, typename VertexElementType>
	class GLStaticStreamPrimitives :
			public GLStreamPrimitives<VertexType, VertexElementType, GLStaticBufferStreamWriter>
	{  };


	/**
	 * A type of @a GLStreamPrimitives that writes to dynamic (std::vector) vertex/index buffers.
	 *
	 * It is defined via inheritance since C++ does not support templated typedefs.
	 */
	template <class VertexType, typename VertexElementType>
	class GLDynamicStreamPrimitives :
			public GLStreamPrimitives<VertexType, VertexElementType, GLDynamicBufferStreamWriter>
	{  };
}

////////////////////
// Implementation //
////////////////////

namespace GPlatesOpenGL
{
	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	template <class VertexStreamWriterConstructorArgs, class VertexElementStreamWriterConstructorArgs>
	void
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::begin_streaming(
			const VertexStreamWriterConstructorArgs &vertex_stream_writer_constructor_args,
			const VertexElementStreamWriterConstructorArgs &vertex_element_stream_writer_constructor_args,
			unsigned int &begin_streaming_vertex_count,
			unsigned int &begin_streaming_vertex_element_count)
	{
		// Make sure 'end_streaming' was called (or this is the first we're called).
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				!d_vertex_element_stream && !d_vertex_element_stream,
				GPLATES_ASSERTION_SOURCE);

		// The stream writers are constructed directly in their boost::optional wrappers
		// where their constructor parameters are in the 'in_place_factory' objects passed in -
		// these were created by the user using 'boost::in_place'.
		// boost::optional will construct/assign a new object using them.
		// This means no copy-assignment of objects - the assignment operator of boost::optional
		// just works with in-place factories - boost is cool !

		// Destination to start streaming vertices.
		d_vertex_stream = vertex_stream_writer_constructor_args;
		// The initial stream count (can be non-zero if passed a non-empty std::vector for example).
		d_begin_vertex_stream_count = d_vertex_stream->count();

		// Destination to start streaming vertex elements.
		d_vertex_element_stream = vertex_element_stream_writer_constructor_args;
		// The initial stream count (can be non-zero if passed a non-empty std::vector for example).
		d_begin_vertex_element_stream_count = d_vertex_element_stream->count();

		// Initialise caller's parameters.
		begin_streaming_vertex_count = d_begin_vertex_stream_count;
		begin_streaming_vertex_element_count = d_begin_vertex_element_stream_count;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	unsigned int
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::get_base_vertex_element() const
	{
		// Make sure we are between 'begin_streaming' and 'end_streaming'.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_vertex_stream,
				GPLATES_ASSERTION_SOURCE);

		return d_vertex_stream->count();
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	unsigned int
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::get_num_streamed_vertices() const
	{
		// Make sure we are between 'begin_streaming' and 'end_streaming'.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_vertex_stream,
				GPLATES_ASSERTION_SOURCE);

		return d_vertex_stream->count() - d_begin_vertex_stream_count;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	unsigned int
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::get_num_streamed_vertex_elements() const
	{
		// Make sure we are between 'begin_streaming' and 'end_streaming'.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_vertex_element_stream,
				GPLATES_ASSERTION_SOURCE);

		return d_vertex_element_stream->count() - d_begin_vertex_element_stream_count;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	void
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::end_streaming(
			unsigned int &num_streamed_vertices,
			unsigned int &num_streamed_vertex_elements)
	{
		// Make sure 'begin_streaming' was called.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_vertex_stream && d_vertex_element_stream,
				GPLATES_ASSERTION_SOURCE);

		// The number of streamed vertices/indices since 'begin_streaming'.
		num_streamed_vertices = d_vertex_stream->count() - d_begin_vertex_stream_count;
		num_streamed_vertex_elements = d_vertex_element_stream->count() - d_begin_vertex_element_stream_count;

		// Streams can no longer be used until the next 'begin_streaming' call.
		d_vertex_stream = boost::none;
		d_vertex_element_stream = boost::none;
		d_begin_vertex_stream_count = 0;
		d_begin_vertex_element_stream_count = 0;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::add_point(
			const vertex_type &vertex)
	{
		// Make sure we are between 'begin_streaming' and 'end_streaming'.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_vertex_stream && d_vertex_element_stream,
				GPLATES_ASSERTION_SOURCE);

		// If there is not enough space for the vertices or indices.
		if (!d_vertex_stream->remaining() || !d_vertex_element_stream->remaining())
		{
			return false;
		}

		const vertex_element_type vertex_index = d_vertex_stream->count();

		// Add the vertex.
		d_vertex_stream->write(vertex);

		// Add the vertex element.
		d_vertex_element_stream->write(vertex_index);

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::add_line(
			const vertex_type &start_vertex,
			const vertex_type &end_vertex)
	{
		// Make sure we are between 'begin_streaming' and 'end_streaming'.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_vertex_stream && d_vertex_element_stream,
				GPLATES_ASSERTION_SOURCE);

		// If there is not enough space for the vertices or indices.
		if (d_vertex_stream->remaining() < 2 || d_vertex_element_stream->remaining() < 2)
		{
			return false;
		}

		const vertex_element_type start_vertex_index = d_vertex_stream->count();
		const vertex_element_type end_vertex_index = start_vertex_index + 1;

		// Add the line's vertices to the list of vertices.
		d_vertex_stream->write(start_vertex);
		d_vertex_stream->write(end_vertex);

		// Add the line's vertex indices.
		d_vertex_element_stream->write(start_vertex_index);
		d_vertex_element_stream->write(end_vertex_index);

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::add_line(
			const vertex_type &end_vertex)
	{
		// Make sure we are between 'begin_streaming' and 'end_streaming'.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_vertex_stream && d_vertex_element_stream,
				GPLATES_ASSERTION_SOURCE);

		// If there is not enough space for the vertices or indices.
		if (!d_vertex_stream->remaining() || d_vertex_element_stream->remaining() < 2)
		{
			return false;
		}

		const vertex_element_type end_vertex_index = d_vertex_stream->count();
		const vertex_element_type start_vertex_index = end_vertex_index - 1;

		// Add the line's end vertex to the list of vertices.
		// The start vertex is already in there.
		d_vertex_stream->write(end_vertex);

		// Add the line's vertex indices.
		d_vertex_element_stream->write(start_vertex_index);
		d_vertex_element_stream->write(end_vertex_index);

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::add_triangle(
			const vertex_type &first_vertex,
			const vertex_type &second_vertex,
			const vertex_type &third_vertex)
	{
		// Make sure we are between 'begin_streaming' and 'end_streaming'.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_vertex_stream && d_vertex_element_stream,
				GPLATES_ASSERTION_SOURCE);

		// If there is not enough space for the vertices or indices.
		if (d_vertex_stream->remaining() < 3 || d_vertex_element_stream->remaining() < 3)
		{
			return false;
		}

		const vertex_element_type first_vertex_index = d_vertex_stream->count();

		// Add the triangle's vertices to the list of vertices.
		d_vertex_stream->write(first_vertex);
		d_vertex_stream->write(second_vertex);
		d_vertex_stream->write(third_vertex);

		// Add the triangle's vertex indices.
		d_vertex_element_stream->write(first_vertex_index);
		d_vertex_element_stream->write(first_vertex_index + 1);
		d_vertex_element_stream->write(first_vertex_index + 2);

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::add_triangle_reversed(
			const vertex_type &first_vertex,
			const vertex_type &second_vertex,
			const vertex_type &third_vertex)
	{
		// Make sure we are between 'begin_streaming' and 'end_streaming'.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_vertex_stream && d_vertex_element_stream,
				GPLATES_ASSERTION_SOURCE);

		// If there is not enough space for the vertices or indices.
		if (d_vertex_stream->remaining() < 3 || d_vertex_element_stream->remaining() < 3)
		{
			return false;
		}

		const vertex_element_type first_vertex_index = d_vertex_stream->count();

		// Add the triangle's vertices to the list of vertices.
		d_vertex_stream->write(first_vertex);
		d_vertex_stream->write(second_vertex);
		d_vertex_stream->write(third_vertex);

		// Add the triangle's vertex indices.
		// Note the reverse order of the first and second indices.
		d_vertex_element_stream->write(first_vertex_index + 1);
		d_vertex_element_stream->write(first_vertex_index);
		d_vertex_element_stream->write(first_vertex_index + 2);

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::add_triangle(
			const vertex_type &third_vertex)
	{
		// Make sure we are between 'begin_streaming' and 'end_streaming'.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_vertex_stream && d_vertex_element_stream,
				GPLATES_ASSERTION_SOURCE);

		// If there is not enough space for the vertices or indices.
		if (!d_vertex_stream->remaining() || d_vertex_element_stream->remaining() < 3)
		{
			return false;
		}

		const vertex_element_type third_vertex_index = d_vertex_stream->count();

		// Add the triangle's third vertex to the list of vertices.
		d_vertex_stream->write(third_vertex);

		// Add the triangle's vertex indices.
		d_vertex_element_stream->write(third_vertex_index - 2);
		d_vertex_element_stream->write(third_vertex_index - 1);
		d_vertex_element_stream->write(third_vertex_index);

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::add_triangle_reversed(
			const vertex_type &third_vertex)
	{
		// Make sure we are between 'begin_streaming' and 'end_streaming'.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_vertex_stream && d_vertex_element_stream,
				GPLATES_ASSERTION_SOURCE);

		// If there is not enough space for the vertices or indices.
		if (!d_vertex_stream->remaining() || d_vertex_element_stream->remaining() < 3)
		{
			return false;
		}

		const vertex_element_type third_vertex_index = d_vertex_stream->count();

		// Add the triangle's third vertex to the list of vertices.
		d_vertex_stream->write(third_vertex);

		// Add the triangle's vertex indices.
		// Note the reverse of the first and second vertices compared to 'add_triangle()' above.
		d_vertex_element_stream->write(third_vertex_index - 1);
		d_vertex_element_stream->write(third_vertex_index - 2);
		d_vertex_element_stream->write(third_vertex_index);

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::add_triangle(
			const vertex_type &first_vertex,
			const vertex_type &third_vertex)
	{
		// Make sure we are between 'begin_streaming' and 'end_streaming'.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_vertex_stream && d_vertex_element_stream,
				GPLATES_ASSERTION_SOURCE);

		// If there is not enough space for the vertices or indices.
		if (d_vertex_stream->remaining() < 2 || d_vertex_element_stream->remaining() < 3)
		{
			return false;
		}

		const vertex_element_type first_vertex_index = d_vertex_stream->count();
		const vertex_element_type third_vertex_index = first_vertex_index + 1;
		const vertex_element_type second_vertex_index = first_vertex_index - 1;

		// Add the triangle's first and third vertices to the list of vertices.
		d_vertex_stream->write(first_vertex);
		d_vertex_stream->write(third_vertex);

		// Add the triangle's vertex indices.
		d_vertex_element_stream->write(first_vertex_index);
		d_vertex_element_stream->write(second_vertex_index);
		d_vertex_element_stream->write(third_vertex_index);

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::add_vertex(
			const vertex_type &vertex)
	{
		// Make sure we are between 'begin_streaming' and 'end_streaming'.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_vertex_stream,
				GPLATES_ASSERTION_SOURCE);

		// If there is not enough space for one vertex.
		if (!d_vertex_stream->remaining())
		{
			return false;
		}

		// Add the vertex.
		d_vertex_stream->write(vertex);

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::add_triangle(
			vertex_element_type first_vertex_element,
			vertex_element_type second_vertex_element,
			vertex_element_type third_vertex_element)
	{
		// Make sure we are between 'begin_streaming' and 'end_streaming'.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_vertex_element_stream,
				GPLATES_ASSERTION_SOURCE);

		// If there is not enough space for the indices.
		if (d_vertex_element_stream->remaining() < 3)
		{
			return false;
		}

		// Add the triangle's vertex indices.
		d_vertex_element_stream->write(first_vertex_element);
		d_vertex_element_stream->write(second_vertex_element);
		d_vertex_element_stream->write(third_vertex_element);

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::Lines::add_vertex(
			const vertex_type &vertex)
	{
		if (d_first_vertex)
		{
			// Store vertex for later.
			d_start_vertex = vertex;
			d_first_vertex = false;
		}
		else
		{
			if (!d_stream_primitives.add_line(d_start_vertex, vertex))
			{
				// Return immediately so the user can re-submit the vertex once they've started a new
				// buffer(s) - in which case the next call will attempt to add the same line again.
				return false;
			}

			d_first_vertex = true;
		}

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::LineStrips::add_vertex(
			const vertex_type &vertex)
	{
		if (d_vertex_index >= 2) // Test most common case first...
		{
			// Add a line using the current vertex and the last vertex added (implicit).
			if (!d_stream_primitives.add_line(vertex))
			{
				// To enable the user to re-submit the same vertex again (after they've allocated
				// more vertex/index buffer space) we will also need to re-submit the previous vertex.
				d_vertex_index = 1;
				d_start_vertex = d_last_shared_vertex;

				return false;
			}

			// Record the last shared vertex in case user needs to re-submit a vertex.
			d_last_shared_vertex = vertex;

			// No need to increment vertex index anymore.
		}
		else if (d_vertex_index == 0)
		{
			d_start_vertex = vertex;
			++d_vertex_index;
		}
		else // d_vertex_index == 1
		{
			if (!d_stream_primitives.add_line(d_start_vertex, vertex))
			{
				// Return immediately so the user can re-submit the vertex once they've started a new
				// buffer(s) - in which case the next call will attempt to add the same line again.
				return false;
			}

			// Record the last shared vertex in case user needs to re-submit a vertex.
			d_last_shared_vertex = vertex;
			++d_vertex_index;
		}

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	void
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::LineLoops::begin_line_loop()
	{
		d_vertex_index = 0;
		d_closed_line_loop = false;
		d_line_strips.begin_line_strip();
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::LineLoops::add_vertex(
			const vertex_type &vertex)
	{
		if (!d_line_strips.add_vertex(vertex))
		{
			// Return immediately so the user can re-submit the vertex once they've started a new
			// buffer(s) - in which case the next call will attempt to add the same line again.
			return false;
		}

		// Record start vertex so we can close the loop when 'end_line_loop()' is called.
		if (d_vertex_index == 0)
		{
			d_start_vertex = vertex;
		}

		++d_vertex_index;

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::LineLoops::end_line_loop()
	{
		d_line_strips.end_line_strip();

		// We need at least a triangle to be able to close the loop.
		if (d_vertex_index > 2)
		{
			if (!d_line_strips.add_vertex(d_start_vertex))
			{
				// Return immediately so the user can re-submit the vertex once they've started a new
				// buffer(s) - in which case the next call will attempt to add the same line again.
				return false;
			}
		}

		d_closed_line_loop = true;

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::Triangles::add_vertex(
			const vertex_type &vertex)
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
			if (!d_stream_primitives.add_triangle(d_triangles_vertices[0], d_triangles_vertices[1], vertex))
			{
				// Return immediately so the user can re-submit the vertex once they've started a new
				// buffer(s) - in which case the next call will attempt to add the same triangle again.
				return false;
			}

			d_vertex_index = 0;
		}

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::TriangleStrips::add_vertex(
			const vertex_type &vertex)
	{
		if (d_vertex_index > 2) // Handle most common case first...
		{
			// If we're submitting an even-number vertex (or an odd-number vertex with reverse triangle winding)...
			if (((d_vertex_index & 1) ^ d_reverse_triangle_winding) == 0)
			{
				if (!d_stream_primitives.add_triangle(vertex))
				{
					// Realign the order of the last two vertices to match the user's submission order.
					if (((d_vertex_index - 1) & 1) == 0)
					{
						// The most recently submitted vertex (prior to 'vertex') is at offset 0.
						// Change it so it's at offset 1.
						std::swap(d_triangles_vertices[0], d_triangles_vertices[1]);
					}

					// To enable the user to re-submit the same vertex again (after they've allocated
					// more vertex/index buffer space) we will also need to re-submit the previous two vertices.
					d_vertex_index = 2;

					// The current triangle winding is normal so the re-submitted triangle should also.
					d_reverse_triangle_winding = 0;

					// Return immediately so the user can re-submit the vertex once they've started a new
					// buffer(s) - in which case the next call will attempt to add the same triangle again.
					return false;
				}
			}
			else
			{
				if (!d_stream_primitives.add_triangle_reversed(vertex))
				{
					// Realign the order of the last two vertices to match the user's submission order.
					if (((d_vertex_index - 1) & 1) == 0)
					{
						// The most recently submitted vertex (prior to 'vertex') is at offset 0.
						// Change it so it's at offset 1.
						std::swap(d_triangles_vertices[0], d_triangles_vertices[1]);
					}

					// To enable the user to re-submit the same vertex again (after they've allocated
					// more vertex/index buffer space) we will also need to re-submit the previous two vertices.
					d_vertex_index = 2;

					// The current triangle winding is reversed so the re-submitted triangle should also.
					d_reverse_triangle_winding = 1;

					// Return immediately so the user can re-submit the vertex once they've started a new
					// buffer(s) - in which case the next call will attempt to add the same triangle again.
					return false;
				}
			}

			// Record the last two submitted vertices in case user needs to re-submit a vertex.
			// Doing it this way means we only need to do one copy instead of two.
			d_triangles_vertices[d_vertex_index & 1] = vertex;

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
			// Or we have a re-submit due to a full vertex buffer.
			if (d_reverse_triangle_winding)
			{
				// Note that vertices are always submitted (re-submitted) in the original order.
				if (!d_stream_primitives.add_triangle_reversed(d_triangles_vertices[0], d_triangles_vertices[1], vertex))
				{
					// Return immediately so the user can re-submit the vertex once they've started a new
					// buffer(s) - in which case the next call will attempt to add the same triangle again.
					return false;
				}
			}
			else
			{
				// Note that vertices are always submitted (re-submitted) in the original order.
				if (!d_stream_primitives.add_triangle(d_triangles_vertices[0], d_triangles_vertices[1], vertex))
				{
					// Return immediately so the user can re-submit the vertex once they've started a new
					// buffer(s) - in which case the next call will attempt to add the same triangle again.
					return false;
				}
			}

			// Record the last two submitted vertices in case user needs to re-submit a vertex.
			// Doing it this way means we only need to do one copy instead of two.
			d_triangles_vertices[d_vertex_index & 1] = vertex;
			++d_vertex_index;
		}

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::TriangleFans::add_vertex(
			const vertex_type &vertex)
	{
		if (d_vertex_index > 2) // Handle most common case first...
		{
			// Make sure we are between 'begin_streaming' and 'end_streaming'.
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					d_stream_primitives.d_vertex_stream && d_stream_primitives.d_vertex_element_stream,
					GPLATES_ASSERTION_SOURCE);

			// If there is not enough space for one vertex and three indices.
			if (!d_stream_primitives.d_vertex_stream->remaining() ||
				d_stream_primitives.d_vertex_element_stream->remaining() < 3)
			{
				// To enable the user to re-submit the same vertex again (after they've allocated
				// more vertex/index buffer space) we will also need to re-submit the previous two vertices.
				d_vertex_index = 2;

				// Return immediately so the user can re-submit the vertex once they've started a new
				// buffer(s) - in which case the next call will attempt to add the same triangle again.
				return false;
			}

			// Record the last two submitted vertices in case user needs to re-submit a vertex.
			// We only need one copy since the apex vertex never changes.
			d_triangles_vertices[1] = vertex;

			// Add the triangle's third vertex to the stream.
			const vertex_element_type third_vertex_element = d_stream_primitives.d_vertex_stream->count();
			d_stream_primitives.d_vertex_stream->write(vertex);

			// The second vertex is the third vertex of the last triangle added which
			// is the last vertex added to the stream.
			const vertex_element_type second_vertex_element = third_vertex_element - 1;

			// The first vertex is the apex vertex.

			// Add the triangle's vertex indices.
			d_stream_primitives.d_vertex_element_stream->write(d_apex_vertex_element);
			d_stream_primitives.d_vertex_element_stream->write(second_vertex_element);
			d_stream_primitives.d_vertex_element_stream->write(third_vertex_element);

			// No need to increment vertex index anymore.
		}
		else if (d_vertex_index < 2)
		{
			// The first or second vertex of the triangle fan.
			d_triangles_vertices[d_vertex_index] = vertex;
			++d_vertex_index;
		}
		else // d_vertex_index == 2
		{
			// The first vertex added is the apex vertex - record its position in the vertex buffer
			// so we can index it later (all triangles in the fan index the apex vertex).
			d_apex_vertex_element = d_stream_primitives.d_vertex_stream->count();

			// We have the third triangle vertex so submit our first triangle.
			if (!d_stream_primitives.add_triangle(d_triangles_vertices[0], d_triangles_vertices[1], vertex))
			{
				// Return immediately so the user can re-submit the vertex once they've started a new
				// buffer(s) - in which case the next call will attempt to add the same triangle again.
				return false;
			}

			// Record the last two submitted vertices in case user needs to re-submit a vertex.
			// We only need one copy since the apex vertex never changes.
			d_triangles_vertices[1] = vertex;
			++d_vertex_index;
		}

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::TriangleMeshes::add_vertex(
			const vertex_type &vertex)
	{
		// Make sure we are between 'begin_streaming' and 'end_streaming'.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_stream_primitives.d_vertex_stream,
				GPLATES_ASSERTION_SOURCE);

		// If there is not enough space for one vertex.
		if (!d_stream_primitives.d_vertex_stream->remaining())
		{
			reorder_vertices();

			// Return immediately so the user can re-submit the vertex once they've started a new
			// buffer(s) - in which case the next call will attempt to add the same vertex again.
			return false;
		}

		if (d_reordered_vertices)
		{
			// Here there's no longer a one-to-one association between vertices and indices.
			// So we need to keep track of which vertex elements map to which vertices.
			d_reordered_mesh_vertex_elements.push_back(d_stream_primitives.d_vertex_stream->count());
		}

		// Add the vertex to the stream.
		d_stream_primitives.d_vertex_stream->write(vertex);

		// Keep track of the vertex in case the stream buffer fills up and we need to re-submit it later.
		d_mesh_vertices.push_back(vertex);

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::TriangleMeshes::add_triangle(
			vertex_element_type first_relative_vertex_element,
			vertex_element_type second_relative_vertex_element,
			vertex_element_type third_relative_vertex_element)
	{
		// Number of vertices in the current mesh.
		const unsigned int num_mesh_vertices = d_mesh_vertices.size();

		// Make sure we are between 'begin_streaming' and 'end_streaming' *and*
		// make sure enough vertices have been added otherwise the indices can reference garbage.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_stream_primitives.d_vertex_stream &&
					first_relative_vertex_element < num_mesh_vertices &&
					second_relative_vertex_element < num_mesh_vertices &&
					third_relative_vertex_element < num_mesh_vertices,
				GPLATES_ASSERTION_SOURCE);

		if (d_reordered_vertices)
		{
			// Here there's no longer a one-to-one association between vertices and indices.
			// We need to determine if a vertex should get (re)added to the stream buffer.
			// Only those vertices used in new triangles get (re)added - this avoids (re)adding
			// *all* the mesh vertices every time a stream buffer fills up.

			// We could be adding up to three vertices here so make sure there's enough room.
			if (d_stream_primitives.d_vertex_stream->remaining() < 3)
			{
				// Return immediately so the user can re-submit the triangle once they've started a new
				// buffer(s) - in which case the next call will attempt to add the same triangle again.
				return false;
			}

			// See if the first triangle vertex has been added.
			boost::optional<vertex_element_type> &first_vertex_element =
					d_reordered_mesh_vertex_elements[first_relative_vertex_element];
			if (!first_vertex_element)
			{
				// Keep track of the vertex element in case vertex is indexed again.
				first_vertex_element = d_stream_primitives.d_vertex_stream->count();
				// (Re)add the vertex to the stream.
				d_stream_primitives.d_vertex_stream->write(d_mesh_vertices[first_relative_vertex_element]);
			}

			// See if the second triangle vertex has been added.
			boost::optional<vertex_element_type> &second_vertex_element =
					d_reordered_mesh_vertex_elements[second_relative_vertex_element];
			if (!second_vertex_element)
			{
				// Keep track of the vertex element in case vertex is indexed again.
				second_vertex_element = d_stream_primitives.d_vertex_stream->count();
				// (Re)add the vertex to the stream.
				d_stream_primitives.d_vertex_stream->write(d_mesh_vertices[second_relative_vertex_element]);
			}

			// See if the third triangle vertex has been added.
			boost::optional<vertex_element_type> &third_vertex_element =
					d_reordered_mesh_vertex_elements[third_relative_vertex_element];
			if (!third_vertex_element)
			{
				// Keep track of the vertex element in case vertex is indexed again.
				third_vertex_element = d_stream_primitives.d_vertex_stream->count();
				// (Re)add the vertex to the stream.
				d_stream_primitives.d_vertex_stream->write(d_mesh_vertices[third_relative_vertex_element]);
			}

			// Here there's no longer a one-to-one association between vertices and indices.
			if (!d_stream_primitives.add_triangle(
				first_vertex_element.get(),
				second_vertex_element.get(),
				third_vertex_element.get()))
			{
				// Return immediately so the user can re-submit the triangle once they've started a new
				// buffer(s) - in which case the next call will attempt to add the same triangle again.
				return false;
			}

			return true;
		}

		// This is the most common case and happens when the stream buffer(s) have not filled up
		// during submission of the current mesh - we want to keep this path reasonably optimal.
		// Here there's a one-to-one association between vertices and indices.
		if (!d_stream_primitives.add_triangle(
			d_base_vertex_element + first_relative_vertex_element,
			d_base_vertex_element + second_relative_vertex_element,
			d_base_vertex_element + third_relative_vertex_element))
		{
			// We have to assume that the vertex buffer will get re-allocated even though only the
			// vertex *element* buffer needs re-allocation (with the current interface the caller
			// can't know which buffer needs re-allocation - we don't have two return codes - it
			// actually ends up just being easier for the client to re-map both buffers).
			reorder_vertices();

			// Return immediately so the user can re-submit the triangle once they've started a new
			// buffer(s) - in which case the next call will attempt to add the same triangle again.
			return false;
		}

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	void
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::TriangleMeshes::reorder_vertices()
	{
		// Set all the values to 'boost::none' so that they'll get recalculated when we add more triangles.
		// In this way only those vertices that are indexed by subsequently added triangles will be
		// added again to the vertex buffer (ie, not all the vertices).
		if (d_reordered_vertices)
		{
			std::fill(d_reordered_mesh_vertex_elements.begin(), d_reordered_mesh_vertex_elements.end(), boost::none);
		}
		else
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_reordered_mesh_vertex_elements.empty(),
					GPLATES_ASSERTION_SOURCE);
			// Defaults to 'boost::none'.
			d_reordered_mesh_vertex_elements.resize(d_mesh_vertices.size());

			d_reordered_vertices = true;
		}
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	bool
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::Quads::add_vertex(
			const vertex_type &vertex)
	{
		if (d_vertex_index < 2)
		{
			// The first or second vertex of the quad.
			d_quads_vertices[d_vertex_index] = vertex;
			++d_vertex_index;
		}
		else if (d_vertex_index == 2)
		{
			// We have the third quad vertex so submit the first triangle of the quad.
			if (!d_stream_primitives.add_triangle(d_quads_vertices[0], d_quads_vertices[1], vertex))
			{
				// Return immediately so the user can re-submit the vertex once they've started a new
				// buffer(s) - in which case the next call will attempt to add the same triangle again.
				return false;
			}

			// The third vertex of the quad.
			d_quads_vertices[d_vertex_index] = vertex;
			++d_vertex_index;
		}
		else // d_vertex_index == 3 ...
		{
			// We have the fourth quad vertex so submit the second triangle of the quad.
			if (!d_stream_primitives.add_triangle(d_quads_vertices[0], d_quads_vertices[2], vertex))
			{
				// Return immediately so the user can re-submit the vertex once they've started a new
				// buffer(s) - in which case the next call will attempt to add the same triangle again.
				return false;
			}

			d_vertex_index = 0;
		}

		return true;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::StreamTarget::StreamTarget(
			stream_primitives_type &stream_primitives) :
		d_stream_primitives(stream_primitives),
		d_start_streaming_vertex_count(0),
		d_start_streaming_vertex_element_count(0),
		d_num_streamed_vertices(0),
		d_num_streamed_vertex_elements(0),
		d_targeting_stream(false)
	{
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::StreamTarget::~StreamTarget()
	{
		if (d_targeting_stream)
		{
			// Since this is a destructor we cannot let any exceptions escape.
			// If one is thrown we just have to lump it and continue on.
			try
			{
				stop_streaming();
			}
			catch (...)
			{  }
		}
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	template <class VertexStreamWriterConstructorArgs, class VertexElementStreamWriterConstructorArgs>
	void
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::StreamTarget::start_streaming(
			const VertexStreamWriterConstructorArgs &vertex_stream_writer_constructor_args,
			const VertexElementStreamWriterConstructorArgs &vertex_element_stream_writer_constructor_args)
	{
		// Make sure 'stop_streaming' was called, or this is first time called.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				!d_targeting_stream,
				GPLATES_ASSERTION_SOURCE);

		d_stream_primitives.begin_streaming(
				vertex_stream_writer_constructor_args,
				vertex_element_stream_writer_constructor_args,
				d_start_streaming_vertex_count,
				d_start_streaming_vertex_element_count);

		d_targeting_stream = true;
		d_num_streamed_vertices = 0;
		d_num_streamed_vertex_elements = 0;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	void
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::StreamTarget::stop_streaming()
	{
		// Make sure 'start_streaming' was called.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_targeting_stream,
				GPLATES_ASSERTION_SOURCE);

		d_targeting_stream = false;
		d_stream_primitives.end_streaming(d_num_streamed_vertices, d_num_streamed_vertex_elements);
		// Don't reset stream counts so clients can still query them.
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	unsigned int
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::StreamTarget::get_start_streaming_vertex_count() const
	{
		return d_start_streaming_vertex_count;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	unsigned int
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::StreamTarget::get_start_streaming_vertex_element_count() const
	{
		return d_start_streaming_vertex_element_count;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	unsigned int
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::StreamTarget::get_num_streamed_vertices() const
	{
		return d_targeting_stream ? d_stream_primitives.get_num_streamed_vertices() : d_num_streamed_vertices;
	}


	template <class VertexType, typename VertexElementType, template <class> class StreamWriterType>
	unsigned int
	GLStreamPrimitives<VertexType, VertexElementType, StreamWriterType>::StreamTarget::get_num_streamed_vertex_elements() const
	{
		return d_targeting_stream ? d_stream_primitives.get_num_streamed_vertex_elements() : d_num_streamed_vertex_elements;
	}
}

#endif // GPLATES_OPENGL_GLSTREAMPRIMITIVES_H
