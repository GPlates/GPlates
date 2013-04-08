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

		//! Typedef for a vertex element (vertex index) of a polygon.
		typedef GLuint polygon_vertex_element_type;

		//! Typedef for a coloured vertex of a polygon.
		typedef GLColourVertex polygon_vertex_type;

		/**
		 * Contains information to render a filled polygon.
		 */
		struct FilledPolygon
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


			//! Create a filled polygon from a polygon (fan) mesh drawable.
			explicit
			FilledPolygon(
					const Drawable &polygon_mesh_drawable) :
				d_polygon_mesh_drawable(polygon_mesh_drawable)
			{  }


			/**
			 * The filled polygon's (fan) mesh.
			 */
			Drawable d_polygon_mesh_drawable;
		};

		//! Typedef for a filled polygon.
		typedef FilledPolygon filled_polygon_type;

		//! Typedef for a sequence of filled polygons.
		typedef std::vector<filled_polygon_type> filled_polygon_seq_type;

	public:

		//! A convenience typedef for a shared pointer to a non-const @a GLFilledPolygonsMapView.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLFilledPolygonsMapView> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLFilledPolygonsMapView.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLFilledPolygonsMapView> non_null_ptr_to_const_type;

		/**
		 * Used to accumulate filled polygons for rendering.
		 */
		class FilledPolygons
		{
		public:

			/**
			 * Create a filled polygon from a 2D polyline or polygon @a line_geometry.
			 *
			 * A polygon is formed by closing the first and last points if the geometry is a polyline.
			 * If the geometry is already a polygon then this extra point doesn't affect the filled result.
			 * Note that if the geometry has too few points then it simply won't be used to render the filled polygon.
			 */
			void
			add_filled_polygon(
					const std::vector<QPointF> &line_geometry,
					const GPlatesGui::Colour &colour);

			/**
			 * Returns true if any filled polygons have been added.
			 */
			bool
			empty() const
			{
				return d_polygon_filled_drawables.empty();
			}

			/**
			 * Clears the filled polygons accumulated so far.
			 *
			 * This is more efficient than creating a new @a FilledPolygons each render since it
			 * minimises re-allocations.
			 */
			void
			clear()
			{
				d_polygon_filled_drawables.clear();
				d_polygon_vertices.clear();
				d_polygon_vertex_elements.clear();
			}

		private:

			/**
			 * The vertices of all polygons of the current @a render call.
			 *
			 * NOTE: This is only 'clear'ed at each render call in order to avoid excessive re-allocations
			 * at each @a render call (std::vector::clear shouldn't deallocate).
			 */
			std::vector<polygon_vertex_type> d_polygon_vertices;

			/**
			 * The vertex elements (indices) of all polygons of the current @a render call.
			 *
			 * NOTE: This is only 'clear'ed at each render call in order to avoid excessive re-allocations
			 * at each @a render call (std::vector::clear shouldn't deallocate).
			 */
			std::vector<polygon_vertex_element_type> d_polygon_vertex_elements;

			/**
			 * The filled polygon drawables.
			 */
			std::vector<filled_polygon_type> d_polygon_filled_drawables;

			// So can access accumulated vertices/indices/drawables of filled polygons.
			friend class GLFilledPolygonsMapView;
		};

		//! Typedef for a group of filled polygons.
		typedef FilledPolygons filled_polygons_type;


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
		 * Renders the specified filled polygons.
		 */
		void
		render(
				GLRenderer &renderer,
				const filled_polygons_type &filled_polygons);

	private:

		/**
		 * The vertex buffer containing the vertices of all polygons of the current @a render call.
		 */
		GLVertexBuffer::shared_ptr_type d_polygons_vertex_buffer;

		/**
		 * The vertex buffer containing the vertex elements (indices) of all polygons of the current @a render call.
		 */
		GLVertexElementBuffer::shared_ptr_type d_polygons_vertex_element_buffer;

		/**
		 * The vertex array containing all polygons of the current @a render call.
		 *
		 * All polygons for the current @a render call are stored here.
		 * They'll get flushed/replaced when the next render call is made.
		 */
		GLVertexArray::shared_ptr_type d_polygons_vertex_array;


		//! Constructor.
		GLFilledPolygonsMapView(
				GLRenderer &renderer);

		void
		create_polygons_vertex_array(
				GLRenderer &renderer);

		void
		write_filled_polygon_meshes_to_vertex_array(
				GLRenderer &renderer,
				const filled_polygons_type &filled_polygons);
	};
}

#endif // GPLATES_OPENGL_GLFILLEDPOLYGONSMAPVIEW_H
