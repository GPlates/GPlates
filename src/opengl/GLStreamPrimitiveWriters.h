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

#ifndef GPLATES_OPENGL_GLSTREAMPRIMITIVEWRITERS_H
#define GPLATES_OPENGL_GLSTREAMPRIMITIVEWRITERS_H

#include <vector>


namespace GPlatesOpenGL
{
	/*
	 * All stream writer template classes in this file are designed to be write-only streams
	 * of arbitrary element data (used for either vertex attribute data or vertex element data).
	 *
	 * 'VertexType' represents the vertex attribute data to be written.
	 * It must be default-constructible.
	 *
	 * 'VertexElementType' is the integer type used to represent vertex elements (indices) and
	 * must be one of 'GLuint', 'GLushort' or 'GLubyte'.
	 *
	 * All stream writer template classes (don't need to be copyable though) should have the following interface:
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
	 */


	/**
	 * Stream writer class to write to a fixed size buffer.
	 *
	 * This is one way to write to a vertex buffer (using its gl_map_buffer/gl_unmap_buffer interface).
	 *
	 * This is useful when you don't know how many vertices/indices you're going to stream and
	 * you're not going to re-use the vertices/indices.
	 * In this case you just want to fill up a fixed size vertex buffer and, when its full, send
	 * it off to the GPU and then continue filling the vertex buffer up again, etc.
	 * In this scenario it doesn't make sense to determine the number of vertices beforehand and
	 * allocate a vertex buffer of the appropriate size (ie, that's not a very optimal way to draw).
	 *
	 * Since we could be writing to a vertex buffer object we might only have write access
	 * to the memory - so this class acts like a raw pointer but only allows writes and not reads.
	 */
	template <typename StreamElementType>
	class GLStaticBufferStreamWriter
	{
	public:
		GLStaticBufferStreamWriter(
				StreamElementType *stream,
				unsigned int max_num_stream_elements) :
			d_stream(stream),
			d_current_stream_element(stream)
		{  }

		//! Writes the specified stream element and increments write pointer to the next element.
		void
		write(
				const StreamElementType &stream_element)
		{
			*d_current_stream_element = stream_element;
			++d_current_stream_element;
		}

		//! Returns the number of stream elements written so far.
		unsigned int
		count() const
		{
			return d_current_stream_element - d_stream;
		}

		//! Returns the number of stream elements that can still be written (that there is space for).
		unsigned int
		remaining() const
		{
			return d_stream + d_max_num_stream_elements - d_current_stream_element;
		}

	private:
		StreamElementType *d_stream;
		unsigned int d_max_num_stream_elements;

		StreamElementType *d_current_stream_element;
	};


	/**
	 * Stream writer class to write to a variable size buffer using a std::vector.
	 *
	 * This is one way to write to a vertex buffer - by streaming the data into a std::vector
	 * and then finally passing that to the vertex buffer using its gl_buffer_data interface.
	 *
	 * This is useful when you don't know how many vertices/indices you're going to stream but
	 * you only need to do it once (such as initialised a static vertex buffer that gets reused
	 * many times).
	 */
	template <typename StreamElementType>
	class GLDynamicBufferStreamWriter
	{
	public:
		explicit
		GLDynamicBufferStreamWriter(
				std::vector<StreamElementType> &stream) :
			d_stream(stream),
			d_max_stream_elements(stream.max_size())
		{  }

		//! Appends the specified stream element.
		void
		write(
				const StreamElementType &stream_element)
		{
			d_stream.push_back(stream_element);
		}

		/**
		 * Returns the number of stream elements in the 'std::vector' passed into constructor.
		 *
		 * NOTE: This depends on whether a non-empty 'std::vector' was passed into the constructor or not.
		 */
		unsigned int
		count() const
		{
			return d_stream.size();
		}

		/**
		 * Since a std::vector can grow arbitrarily large it's unlikely the limit will ever
		 * be reached so just return the maximum size of vector (don't bother subtracting off
		 * the number of elements currently in the vector - this function gets called very many
		 * times and it's really unnecessary since the limit will never be reached anyway).
		 */
		unsigned int
		remaining() const
		{
			return d_max_stream_elements;
		}

	private:
		std::vector<StreamElementType> &d_stream;
		unsigned int d_max_stream_elements;
	};
}

#endif // GPLATES_OPENGL_GLSTREAMPRIMITIVEWRITERS_H
