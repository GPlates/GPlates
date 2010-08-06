/* $Id$ */

/**
 * \file Draws rendered geometries in a specific @a RenderedGeometryLayer onto 3d orthographic
 * globe using OpenGL.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_GLOBERENDEREDGEOMETRYLAYERPAINTER_H
#define GPLATES_GUI_GLOBERENDEREDGEOMETRYLAYERPAINTER_H

#include <map>
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <QGLWidget>

#include "Colour.h"
#include "ColourScheme.h"
#include "GlobeVisibilityTester.h"
#include "TextRenderer.h"
#include "RenderSettings.h"

#include "maths/types.h"
#include "maths/Vector3D.h"

#include "opengl/GLRenderGraphInternalNode.h"
#include "opengl/GLStreamPrimitives.h"
#include "opengl/GLUNurbsRenderer.h"

#include "view-operations/RenderedGeometry.h"
#include "view-operations/RenderedGeometryVisitor.h"


namespace GPlatesViewOperations
{
	class RenderedGeometryLayer;
}

namespace GPlatesGui
{
	/**
	 * Handles drawing rendered geometries in a single layer by drawing the
	 * opaque primitives first followed by the transparent primitives.
	 */
	class GlobeRenderedGeometryLayerPainter :
			public GPlatesViewOperations::ConstRenderedGeometryVisitor,
			private boost::noncopyable
	{
	public:
		GlobeRenderedGeometryLayerPainter(
				const GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer,
				const double &inverse_viewport_zoom_factor,
				const GPlatesOpenGL::GLUNurbsRenderer::non_null_ptr_type &nurbs_renderer,
				RenderSettings &render_settings,
				TextRenderer::ptr_to_const_type text_renderer_ptr,
				const GlobeVisibilityTester &visibility_tester,
				ColourScheme::non_null_ptr_type colour_scheme) :
			d_rendered_geometry_layer(rendered_geometry_layer),
			d_nurbs_renderer(nurbs_renderer),
			d_inverse_zoom_factor(inverse_viewport_zoom_factor),
			d_render_settings(render_settings),
			d_text_renderer_ptr(text_renderer_ptr),
			d_visibility_tester(visibility_tester),
			d_colour_scheme(colour_scheme),
			d_scale(1.0f)
		{  }


		/**
		 * Draws the sequence of rendered geometries passed into constructor.
		 */
		void
		paint(
				const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &rendered_layer_node);

		void
		set_scale(
				float scale)
		{
			d_scale = scale;
		}

	private:
	
		virtual
		void
		visit_rendered_ellipse(
				const GPlatesViewOperations::RenderedEllipse &rendered_ellipse);
	
	
		virtual
		void
		visit_rendered_point_on_sphere(
				const GPlatesViewOperations::RenderedPointOnSphere &rendered_point_on_sphere);

		virtual
		void
		visit_rendered_multi_point_on_sphere(
				const GPlatesViewOperations::RenderedMultiPointOnSphere &rendered_multi_point_on_sphere);

		virtual
		void
		visit_rendered_polyline_on_sphere(
				const GPlatesViewOperations::RenderedPolylineOnSphere &rendered_polyline_on_sphere);

		virtual
		void
		visit_rendered_polygon_on_sphere(
				const GPlatesViewOperations::RenderedPolygonOnSphere &rendered_polygon_on_sphere);

		virtual
		void
		visit_resolved_raster(
				const GPlatesViewOperations::RenderedResolvedRaster &rendered_resolved_raster);

		virtual
		void
		visit_rendered_direction_arrow(
				const GPlatesViewOperations::RenderedDirectionArrow &rendered_direction_arrow);
				
		virtual
		void
		visit_rendered_small_circle(
				const GPlatesViewOperations::RenderedSmallCircle &rendered_small_circle);
				
		virtual
		void
		visit_rendered_small_circle_arc(
				const GPlatesViewOperations::RenderedSmallCircleArc &rendered_small_circle_arc);				

		virtual
		void
		visit_rendered_string(
				const GPlatesViewOperations::RenderedString &rendered_string);


		/**
		 * Every primitive type has one or more vertices of this type.
		 */
		struct Vertex
		{
			GLfloat x, y, z;
			rgba8_t colour;
		};

		//! Typedef for a sequence of drawables.
		typedef std::vector<GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type> drawable_seq_type;

		/**
		 * Lines can be rendered using either regular vertex line primitives or nurbs curves.
		 */
		struct LineDrawables
		{
			LineDrawables(
					const GPlatesOpenGL::GLStreamPrimitives<Vertex>::non_null_ptr_type &stream_);

			GPlatesOpenGL::GLStreamPrimitives<Vertex>::non_null_ptr_type stream;
			drawable_seq_type nurbs_drawables;
		};


		/**
		 * Drawables for points, lines and polygons (triangles and quads).
		 */
		class PointLinePolygonDrawables
		{
		public:
			//! Constructor.
			PointLinePolygonDrawables();

			/**
			 * Returns the stream for points of size @a point_size.
			 */
			GPlatesOpenGL::GLStreamPrimitives<Vertex> &
			get_point_drawables(
					float point_size);

			/**
			 * Returns the stream and nurbs drawable list for lines of width @a line_width.
			 */
			LineDrawables &
			get_line_drawables(
					float line_width);

			/**
			 * Returns the stream for the triangle mesh drawables.
			 *
			 * There's no point size of line width equivalent for polygons
			 * so they all get lumped into a single stream.
			 */
			GPlatesOpenGL::GLStreamPrimitives<Vertex> &
			get_triangle_drawables()
			{
				return *d_triangle_drawables;
			}

			/**
			 * Returns the stream for the quad mesh drawables.
			 */
			GPlatesOpenGL::GLStreamPrimitives<Vertex> &
			get_quad_drawables()
			{
				return *d_quad_drawables;
			}

			
			/**
			 * Adds all drawables for all primitives types as child render graph nodes
			 * to @a render_graph_parent_node.
			 */
			void
			add_drawables(
					GPlatesOpenGL::GLRenderGraphInternalNode &render_graph_parent_node);

		private:
			/**
			 * Typedef for mapping point size to drawable.
			 * The stream generates the drawables.
			 * All points of the same size will get grouped together.
			 */
			typedef std::map<
					GPlatesMaths::real_t,
					GPlatesOpenGL::GLStreamPrimitives<Vertex>::non_null_ptr_type >
							point_size_to_drawables_map_type;

			/**
			 * Typedef for mapping line width to drawable.
			 * All lines of the same width will get grouped together.
			 */
			typedef std::map<
					GPlatesMaths::real_t,
					LineDrawables> line_width_to_drawables_map_type;


			point_size_to_drawables_map_type d_point_drawables_map;
			line_width_to_drawables_map_type d_line_drawables_map;

			/*
			 * There's no point size or line width equivalent for polygons (triangles and quads)
			 * so they can be lumped into a single streams.
			 *
			 * Although we keep a separate stream for triangles and quads since they are
			 * different primitive types and streaming is more efficient that way.
			 */
			GPlatesOpenGL::GLStreamPrimitives<Vertex>::non_null_ptr_type d_triangle_drawables;
			GPlatesOpenGL::GLStreamPrimitives<Vertex>::non_null_ptr_type d_quad_drawables;

			void
			add_points_drawable(
					float point_size,
					const GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type &points_drawable,
					GPlatesOpenGL::GLRenderGraphInternalNode &render_graph_parent_node);

			boost::optional<GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type>
			get_lines_drawable(
					LineDrawables &line_drawables);

			void
			add_lines_drawable(
					float line_width,
					const GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type &lines_drawable,
					GPlatesOpenGL::GLRenderGraphInternalNode &render_graph_parent_node);

			static
			GPlatesOpenGL::GLStreamPrimitives<Vertex>::non_null_ptr_type
			create_stream();
		};


		/**
		 * Parameters that are only available when @a paint is called.
		 */
		class PaintParams
		{
		public:
			PaintParams(
					const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &
							text_off_the_sphere_node_) :
				text_off_the_sphere_node(text_off_the_sphere_node_)
			{  }

			PointLinePolygonDrawables drawables_off_the_sphere;
			PointLinePolygonDrawables opaque_drawables_on_the_sphere;
			PointLinePolygonDrawables translucent_drawables_on_the_sphere;

			GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type text_off_the_sphere_node;
		};


		//! Parameters that are only available when @a paint is called.
		boost::optional<PaintParams> d_paint_params;

		const GPlatesViewOperations::RenderedGeometryLayer &d_rendered_geometry_layer;
		GPlatesOpenGL::GLUNurbsRenderer::non_null_ptr_type d_nurbs_renderer;
		const double d_inverse_zoom_factor;

		//! Rendering flags for determining what gets shown
		RenderSettings &d_render_settings;

		//! For rendering text
		TextRenderer::ptr_to_const_type d_text_renderer_ptr;

		//! For determining whether a particular point on the globe is visible or not
		GlobeVisibilityTester d_visibility_tester;

		//! For assigning colours to RenderedGeometry
		ColourScheme::non_null_ptr_type d_colour_scheme;

		//! When rendering scaled globes that are meant to be a scaled version of another
		float d_scale;

		//! Multiplying factor to get point size of 1.0f to look like one screen-space pixel.
		static const float POINT_SIZE_ADJUSTMENT;

		//! Multiplying factor to get line width of 1.0f to look like one screen-space pixel.
		static const float LINE_WIDTH_ADJUSTMENT;

		/**
		 * Visit each rendered geometry in our sequence.
		 */
		void
		visit_rendered_geoms();

		/**
		 * Sets up alpha-blending and point/line anti-aliasing state.
		 */
		void
		set_translucent_state(
				GPlatesOpenGL::GLRenderGraphInternalNode &render_graph_node);

		/**
		 * Determines the colour of a RenderedGeometry type using d_colour_scheme
		 */
		template <class T>
		inline
		boost::optional<Colour>
		get_colour_of_rendered_geometry(
				const T &geom);

		/**
		 * Paints great circle arcs of polylines and polygons.
		 */
		template <typename GreatCircleArcForwardIter>
		void
		paint_great_circle_arcs(
				const GreatCircleArcForwardIter &begin_arcs,
				const GreatCircleArcForwardIter &end_arcs,
				const Colour &colour,
				LineDrawables &line_drawables,
				GPlatesOpenGL::GLUNurbsRenderer &nurbs_renderer);

		/**
		 * Paints an ellipse.
		 */
		void
		paint_ellipse(
				const GPlatesViewOperations::RenderedEllipse &rendered_ellipse,
				const Colour &colour,
				LineDrawables &line_drawables);

		/**
		 * Paints a cone (for arrow head).
		 */
		void
		paint_cone(
				const GPlatesMaths::Vector3D &apex,
				const GPlatesMaths::Vector3D &cone_axis,
				rgba8_t rgba8_color,
				GPlatesOpenGL::GLStreamPrimitives<Vertex> &stream);
	};
}

#endif // GPLATES_GUI_GLOBERENDEREDGEOMETRYLAYERPAINTER_H
