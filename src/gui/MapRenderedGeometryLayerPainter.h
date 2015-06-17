/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2010 The Geological Survey of Norway
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

#ifndef GPLATES_GUI_MAPCANVASPAINTER_H
#define GPLATES_GUI_MAPCANVASPAINTER_H

#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <proj_api.h>

#include "Colour.h"
#include "ColourScheme.h"
#include "LayerPainter.h"
#include "RenderSettings.h"

#include "maths/DateLineWrapper.h"
#include "maths/LatLonPoint.h"

#include "opengl/GLFilledPolygonsMapView.h"
#include "opengl/GLVisualLayers.h"

#include "presentation/VisualLayers.h"

#include "view-operations/RenderedGeometry.h"
#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryCollectionVisitor.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryLayer;
}

namespace GPlatesGui
{
	class MapProjection;

	/**
	 * Handles drawing rendered geometries in a single layer by drawing the
	 * opaque primitives first followed by the transparent primitives.
	 */
	class MapRenderedGeometryLayerPainter:
			public GPlatesViewOperations::ConstRenderedGeometryCollectionVisitor<
				GPlatesPresentation::VisualLayers::rendered_geometry_layer_seq_type>,
			public boost::noncopyable
	{
	public:
		/**
		 * Typedef for an opaque object that caches a particular painting.
		 */
		typedef boost::shared_ptr<void> cache_handle_type;


		MapRenderedGeometryLayerPainter(
				const MapProjection::non_null_ptr_to_const_type &map_projection,
				const GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer,
				const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
				const double &inverse_viewport_zoom_factor,
				const GPlatesGui::RenderSettings &render_settings,
				ColourScheme::non_null_ptr_type colour_scheme);


		/**
		 * Draws the sequence of rendered geometries passed into constructor.
		 */
		cache_handle_type
		paint(
				GPlatesOpenGL::GLRenderer &renderer,
				LayerPainter &layer_painter);

		void
		set_scale(
				float scale)
		{
			d_scale = scale;
		}

	private:

		//
		// Please keep these geometries ordered alphabetically.
		//

		virtual
		void
		visit_rendered_arrowed_polyline(
				const GPlatesViewOperations::RenderedArrowedPolyline &rendered_arrowed_polyline);


		virtual
		void
		visit_rendered_cross_symbol(
				const GPlatesViewOperations::RenderedCrossSymbol &rendered_cross_symbol);

		virtual
		void
		visit_rendered_radial_arrow(
				const GPlatesViewOperations::RenderedRadialArrow &rendered_radial_arrow);

		virtual
		void
		visit_rendered_tangential_arrow(
				const GPlatesViewOperations::RenderedTangentialArrow &rendered_tangential_arrow);

		virtual
		void
		visit_rendered_ellipse(
				const GPlatesViewOperations::RenderedEllipse &rendered_ellipse);


		virtual
		void
		visit_rendered_multi_point_on_sphere(
				const GPlatesViewOperations::RenderedMultiPointOnSphere &rendered_multi_point_on_sphere);

		virtual
		void
		visit_rendered_point_on_sphere(
				const GPlatesViewOperations::RenderedPointOnSphere &rendered_point_on_sphere);


		virtual
		void
		visit_rendered_polygon_on_sphere(
				const GPlatesViewOperations::RenderedPolygonOnSphere &rendered_polygon_on_sphere);
	
		
		virtual
		void
		visit_rendered_polyline_on_sphere(
				const GPlatesViewOperations::RenderedPolylineOnSphere &rendered_polyline_on_sphere);

		virtual
		void
		visit_rendered_coloured_edge_surface_mesh(
			const GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh &rendered_coloured_edge_surface_mesh);

		virtual
		void
		visit_rendered_coloured_triangle_surface_mesh(
				const GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh &rendered_coloured_triangle_surface_mesh);

		virtual
		void
		visit_rendered_resolved_raster(
				const GPlatesViewOperations::RenderedResolvedRaster &rendered_resolved_raster);

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
		visit_rendered_square_symbol(
			const GPlatesViewOperations::RenderedSquareSymbol &rendered_square_symbol);

		virtual
		void
		visit_rendered_circle_symbol(
			const GPlatesViewOperations::RenderedCircleSymbol &rendered_circle_symbol);

		virtual
		void
		visit_rendered_string(
				const GPlatesViewOperations::RenderedString &rendered_string);

		virtual
		void
		visit_rendered_triangle_symbol(
				const GPlatesViewOperations::RenderedTriangleSymbol &rendered_triangle_symbol);

	private:

		/**
		 * Contains the results of dateline wrapping and map projecting a polyline or polygon.
		 */
		class DatelineWrappedProjectedLineGeometry
		{
		public:

			void
			add_vertex(
					const QPointF &vertex)
			{
				d_vertices.push_back(vertex);
			}

			//! Call this *after* adding the great circle arc's vertices.
			void
			add_great_circle_arc()
			{
				d_great_circle_arcs.push_back(d_vertices.size());
			}

			//! Call this *after* adding the geometry's great circle arcs.
			void
			add_geometry()
			{
				d_geometries.push_back(d_great_circle_arcs.size());
			}


			/**
			 * Returns vertices in all geometries (and in all great circle arcs).
			 *
			 * These are indexed by the indices returned in @a get_great_circle_arcs.
			 */
			const std::vector<QPointF> &
			get_vertices() const
			{
				return d_vertices;
			}

			/**
			 * Returns a vertex index (into @a get_vertices) for each great circle arc (in all geometries)
			 * that represents 'one past' the last vertex in that great circle arc.
			 *
			 * These are indexed by the indices returned in @a get_geometries.
			 */
			const std::vector<unsigned int> &
			get_great_circle_arcs()
			{
				return d_great_circle_arcs;
			}

			/**
			 * Returns a great circle arc index (into @a get_great_circle_arcs) for each geometry
			 * that represents 'one past' the last great circle arc in that geometry.
			 */
			const std::vector<unsigned int> &
			get_geometries()
			{
				return d_geometries;
			}

		private:

			std::vector<QPointF> d_vertices;
			std::vector<unsigned int> d_great_circle_arcs;
			std::vector<unsigned int> d_geometries;
		};

		//! Typedef for a vertex element (index).
		typedef LayerPainter::vertex_element_type vertex_element_type;

		//! Typedef for a coloured vertex.
		typedef LayerPainter::coloured_vertex_type coloured_vertex_type;

		//! Typedef for a sequence of coloured vertices.
		typedef LayerPainter::coloured_vertex_seq_type coloured_vertex_seq_type;

		//! Typedef for a sequence of vertex elements.
		typedef LayerPainter::vertex_element_seq_type vertex_element_seq_type;

		//! Typedef for a primitives stream containing coloured vertices.
		typedef LayerPainter::stream_primitives_type stream_primitives_type;


		//! Used to project vertices of rendered geometries to the map.
		MapProjection::non_null_ptr_to_const_type d_map_projection;

		const GPlatesViewOperations::RenderedGeometryLayer &d_rendered_geometry_layer;

		/**
		 * Keeps track of OpenGL-related objects that persist from one render to the next.
		 */
		GPlatesOpenGL::GLVisualLayers::non_null_ptr_type d_gl_visual_layers;

		const double d_inverse_zoom_factor;

		//! Rendering flags for determining what gets shown
		const RenderSettings &d_render_settings;

		//! For assigning colours to RenderedGeometry
		ColourScheme::non_null_ptr_type d_colour_scheme;

		//! When rendering scaled maps that are meant to be a scaled version of another
		float d_scale;

		/**
		 * Wraps polylines/polygons to [-180,180] longitude about the central meridian of the map projection.
		 */
		GPlatesMaths::DateLineWrapper::non_null_ptr_type d_dateline_wrapper;

		/**
		 * Used to paint when the @a paint method is called.
		 *
		 * Is only valid during @a paint.
		 */
		boost::optional<LayerPainter &> d_layer_painter;


		//! For hard-coded tweaking of the size of points
		static const float POINT_SIZE_ADJUSTMENT;
		
		//! For hard-coded tweaking of the width of lines
		static const float LINE_WIDTH_ADJUSTMENT;


		/**
		 * Visit each rendered geometry in our sequence (or spatial partition).
		 */
		void
		visit_rendered_geometries(
				GPlatesOpenGL::GLRenderer &renderer);

		/**
		 * Determines the colour of a RenderedGeometry type using d_colour_scheme
		 */
		template <class T>
		inline
		boost::optional<Colour>
		get_colour_of_rendered_geometry(
				const T &geom);

		/**
		 * Dateline wraps and map projects polylines.
		 */
		void
		dateline_wrap_and_project_line_geometry(
				DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
				const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &polyline_on_sphere);

		/**
		 * Dateline wraps and map projects polygons.
		 */
		void
		dateline_wrap_and_project_line_geometry(
				DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
				const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &polygon_on_sphere);

		/**
		 * Project and tessellate *wrapped* line geometries (polylines and polygons).
		 */
		void
		project_tessellated_wrapped_line_geometry(
				DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
				const GPlatesMaths::DateLineWrapper::lat_lon_points_seq_type &lat_lon_points,
				const std::vector<unsigned int> &untessellated_arc_end_point_indices,
				bool is_polygon);

		/**
		 * Project and tessellate great circle arcs of *unwrapped* polylines and polygons.
		 */
		template <typename GreatCircleArcForwardIter>
		void
		project_and_tessellate_unwrapped_line_geometry(
				DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
				const GreatCircleArcForwardIter &begin_arcs,
				const GreatCircleArcForwardIter &end_arcs);

		/**
		 * Paints a *filled* line geometry (polyline or polygon) as a filled polygon.
		 */
		template <typename LineGeometryType>
		void
		paint_fill_geometry(
				GPlatesOpenGL::GLFilledPolygonsMapView::filled_drawables_type &filled_polygons,
				const typename LineGeometryType::non_null_ptr_to_const_type &line_geometry,
				const Colour &colour);

		/**
		 * Paints a line geometry (polyline or polygon).
		 */
		template <typename LineGeometryType>
		void
		paint_line_geometry(
				const typename LineGeometryType::non_null_ptr_to_const_type &line_geometry,
				const Colour &colour,
				stream_primitives_type &lines_stream,
				boost::optional<double> arrow_head_size = boost::none);

		void
		paint_arrow_head(
				const QPointF &arrow_head_apex,
				const QPointF &arrow_head_direction,
				const double &arrowhead_size,
				rgba8_t rgba8_color);

		/**
		 * Returns the map projected screen coordinates of the specified point.
		 */
		QPointF
		get_projected_wrapped_position(
				const GPlatesMaths::LatLonPoint &lat_lon_point) const;

		/**
		 * Returns the map projected screen coordinates of the specified point.
		 */
		QPointF
		get_projected_unwrapped_position(
				const GPlatesMaths::PointOnSphere &point_on_sphere) const;
	};
}

#endif  // GPLATES_GUI_MAPCANVASPAINTER_H
