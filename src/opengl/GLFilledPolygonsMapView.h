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

#ifndef GPLATES_OPENGL_GLFILLEDPOLYGONSMAPVIEW_H
#define GPLATES_OPENGL_GLFILLEDPOLYGONSMAPVIEW_H

#include <vector>
#include <boost/optional.hpp>
#include <opengl/OpenGL.h>
#include <QPointF>

#include "GLVertex.h"
#include "GLVertexArray.h"
#include "GLVertexBuffer.h"
#include "GLVertexElementBuffer.h"

#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * Renders (reconstructed) filled polygons (static or dynamic) using stenciling to generate the
	 * polygon interior fill mask instead of generating a polygon mesh (triangulation).
	 *
	 * The reason for not using polygon meshes is they are expensive to compute (ie, not interactive)
	 * and hence cannot be used for dynamic topological polygons.
	 */
	class GLFilledPolygonsMapView :
			public GPlatesUtils::ReferenceCount<GLFilledPolygonsMapView>
	{
	private:

		//! Typedef for a vertex element (vertex index) of a drawable.
		typedef GLuint drawable_vertex_element_type;

		//! Typedef for a coloured vertex of a drawable.
		typedef GLColourVertex drawable_vertex_type;

		/**
		 * Contains information to render a filled drawable.
		 */
		struct FilledDrawable
		{
			/**
			 * Contains 'gl_draw_range_elements' parameters that locate a geometry inside a vertex array.
			 */
			struct Drawable
			{
				Drawable(
						GLuint start_,
						GLuint end_,
						GLsizei count_,
						GLint indices_offset_) :
					start(start_),
					end(end_),
					count(count_),
					indices_offset(indices_offset_)
				{  }

				GLuint start;
				GLuint end;
				GLsizei count;
				GLint indices_offset;
			};


			//! Create a filled drawable.
			explicit
			FilledDrawable(
					const Drawable &drawable) :
				d_drawable(drawable)
			{  }


			/**
			 * The filled drawable.
			 */
			Drawable d_drawable;
		};

		//! Typedef for a filled drawable.
		typedef FilledDrawable filled_drawable_type;

		//! Typedef for a sequence of filled drawables.
		typedef std::vector<filled_drawable_type> filled_drawable_seq_type;

	public:

		//! A convenience typedef for a shared pointer to a non-const @a GLFilledPolygonsMapView.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLFilledPolygonsMapView> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLFilledPolygonsMapView.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLFilledPolygonsMapView> non_null_ptr_to_const_type;

		/**
		 * Used to accumulate filled drawables for rendering.
		 */
		class FilledDrawables
		{
		public:

			/**
			 * Returns true if any filled drawables have been added.
			 */
			bool
			empty() const
			{
				return d_filled_drawables.empty();
			}

			/**
			 * Clears the filled drawables accumulated so far.
			 *
			 * This is more efficient than creating a new @a FilledDrawables each render since it
			 * minimises re-allocations.
			 */
			void
			clear()
			{
				d_filled_drawables.clear();
				d_drawable_vertices.clear();
				d_drawable_vertex_elements.clear();
				d_current_drawable = boost::none;
			}

			/**
			 * Create a filled drawable from a 2D polyline or polygon @a line_geometry.
			 *
			 * A polygon is formed by closing the first and last points if the geometry is a polyline.
			 * If the geometry is already a polygon then this extra point doesn't affect the filled result.
			 * Note that if the geometry has too few points then it simply won't be used to render the filled drawable.
			 */
			void
			add_filled_polygon(
					const std::vector<QPointF> &line_geometry,
					const GPlatesGui::Colour &colour);

			/**
			 * Begins a single drawable for a filled mesh composed of individually added triangles.
			 */
			void
			begin_filled_triangle_mesh()
			{
				begin_filled_drawable();
			}

			/**
			 * Ends the current filled triangle mesh drawable (started by @a begin_filled_triangle_mesh).
			 */
			void
			end_filled_triangle_mesh()
			{
				end_filled_drawable();
			}

			/**
			 * Adds a coloured triangle to the current filled triangle mesh drawable.
			 *
			 * This must be called between @a begin_filled_triangle_mesh and @a end_filled_triangle_mesh.
			 */
			void
			add_filled_triangle_to_mesh(
					const QPointF &vertex1,
					const QPointF &vertex2,
					const QPointF &vertex3,
					const GPlatesGui::Colour &colour);

		private:

			/**
			 * The vertices of all drawables of the current @a render call.
			 *
			 * NOTE: This is only 'clear'ed at each render call in order to avoid excessive re-allocations
			 * at each @a render call (std::vector::clear shouldn't deallocate).
			 */
			std::vector<drawable_vertex_type> d_drawable_vertices;

			/**
			 * The vertex elements (indices) of all drawables of the current @a render call.
			 *
			 * NOTE: This is only 'clear'ed at each render call in order to avoid excessive re-allocations
			 * at each @a render call (std::vector::clear shouldn't deallocate).
			 */
			std::vector<drawable_vertex_element_type> d_drawable_vertex_elements;

			/**
			 * The filled drawables.
			 */
			std::vector<filled_drawable_type> d_filled_drawables;

			/**
			 * The current drawable.
			 *
			 * Is only valid between @a begin_filled_drawable and @a end_filled_drawable.
			 */
			boost::optional<FilledDrawable::Drawable> d_current_drawable;


			// So can access accumulated vertices/indices/drawables of filled drawables.
			friend class GLFilledPolygonsMapView;


			/**
			 * Begin a new drawable.
			 *
			 * Everything in a drawable is rendered in one draw call and stenciled as a unit.
			 */
			void
			begin_filled_drawable();

			/**
			 * End the current drawable.
			 */
			void
			end_filled_drawable();
		};

		//! Typedef for a group of filled drawables.
		typedef FilledDrawables filled_drawables_type;


		/**
		 * Creates a @a GLFilledPolygonsMapView object.
		 */
		static
		non_null_ptr_type
		create(
				GLRenderer &renderer)
		{
			return non_null_ptr_type(new GLFilledPolygonsMapView(renderer));
		}


		/**
		 * Renders the specified filled drawables.
		 */
		void
		render(
				GLRenderer &renderer,
				const filled_drawables_type &filled_drawables);

	private:

		/**
		 * The vertex buffer containing the vertices of all drawables of the current @a render call.
		 */
		GLVertexBuffer::shared_ptr_type d_drawables_vertex_buffer;

		/**
		 * The vertex buffer containing the vertex elements (indices) of all drawables of the current @a render call.
		 */
		GLVertexElementBuffer::shared_ptr_type d_drawables_vertex_element_buffer;

		/**
		 * The vertex array containing all drawables of the current @a render call.
		 *
		 * All drawables for the current @a render call are stored here.
		 * They'll get flushed/replaced when the next render call is made.
		 */
		GLVertexArray::shared_ptr_type d_drawables_vertex_array;


		//! Constructor.
		GLFilledPolygonsMapView(
				GLRenderer &renderer);

		void
		create_drawables_vertex_array(
				GLRenderer &renderer);

		void
		write_filled_drawables_to_vertex_array(
				GLRenderer &renderer,
				const filled_drawables_type &filled_drawables);
	};
}

#endif // GPLATES_OPENGL_GLFILLEDPOLYGONSMAPVIEW_H
