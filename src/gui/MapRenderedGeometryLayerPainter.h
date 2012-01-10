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

#include <proj_api.h>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include "Colour.h"
#include "ColourScheme.h"
#include "LayerPainter.h"
#include "PersistentOpenGLObjects.h"
#include "TextRenderer.h"
#include "RenderSettings.h"

#include "maths/DateLineWrapper.h"
#include "maths/LatLonPoint.h"

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
				const PersistentOpenGLObjects::non_null_ptr_type &persistent_opengl_objects,
				const double &inverse_viewport_zoom_factor,
				GPlatesGui::RenderSettings &render_settings,
				const TextRenderer::non_null_ptr_to_const_type &text_renderer_ptr,
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
		visit_rendered_direction_arrow(
				const GPlatesViewOperations::RenderedDirectionArrow &rendered_direction_arrow);

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
		visit_resolved_raster(
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

		//! Typedef for a wrapped line geometry (polyline or polygon) containing lat/lon points.
		typedef boost::shared_ptr<GPlatesMaths::DateLineWrapper::lat_lon_points_seq_type> lat_lon_line_geometry_type;


		//! Used to project vertices of rendered geometries to the map.
		MapProjection::non_null_ptr_to_const_type d_map_projection;

		const GPlatesViewOperations::RenderedGeometryLayer &d_rendered_geometry_layer;

		/**
		 * Keeps track of OpenGL-related objects that persist from one render to the next.
		 */
		PersistentOpenGLObjects::non_null_ptr_type d_persistent_opengl_objects;

		const double d_inverse_zoom_factor;

		//! Rendering flags for determining what gets shown
		RenderSettings &d_render_settings;

		//! For rendering text
		TextRenderer::non_null_ptr_to_const_type d_text_renderer_ptr;

		//! For assigning colours to RenderedGeometry
		ColourScheme::non_null_ptr_type d_colour_scheme;

		//! When rendering scaled maps that are meant to be a scaled version of another
		float d_scale;

		/**
		 * Wraps polylines/polygons to [-180,180] longitude about the central meridian of the map projection.
		 */
		GPlatesMaths::DateLineWrapper::non_null_ptr_type d_dateline_wrapper;

		/**
		 * Rotation for central meridian (of map projection) with non-zero longitude to place geometries
		 * in a reference frame centred at longitude zero (where the dateline wrapper can be used).
		 */
		GPlatesMaths::Rotation d_central_meridian_reference_frame_rotation;

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
		 * Paints line geometries (polylines and polygons).
		 */
		template <typename LineGeometryType>
		void
		paint_line_geometry(
				const typename LineGeometryType::non_null_ptr_to_const_type &line_geometry,
				const Colour &colour,
				stream_primitives_type &lines_stream,
				boost::optional<double> arrow_head_size = boost::none);

		/**
		 * Paints *wrapped* line geometries (polylines and polygons).
		 */
		template <typename LatLonPointForwardIter>
		void
		paint_wrapped_line_geometry(
				const LatLonPointForwardIter &begin_lat_lon_points,
				const LatLonPointForwardIter &end_lat_lon_points,
				const Colour &colour,
				stream_primitives_type &lines_stream,
				boost::optional<double> arrow_head_size);

		/**
		 * Paints great circle arcs of *unwrapped* polylines and polygons.
		 */
		template <typename GreatCircleArcForwardIter>
		void
		paint_unwrapped_line_geometry(
				const GreatCircleArcForwardIter &begin_arcs,
				const GreatCircleArcForwardIter &end_arcs,
				const Colour &colour,
				stream_primitives_type &lines_stream,
				boost::optional<double> arrow_head_size);

		void
		paint_arrow_head(
				const QPointF &arrow_head_apex,
				const QPointF &arrow_head_direction,
				const double &arrowhead_size,
				rgba8_t rgba8_color);

		/**
		 * Returns the map projected screen coordinates of the specified point
		 * (in the central meridian reference frame).
		 */
		QPointF
		get_projected_wrapped_position(
				const GPlatesMaths::LatLonPoint &central_meridian_reference_frame_point) const;

		/**
		 * Returns the map projected screen coordinates of the specified point.
		 */
		QPointF
		get_projected_unwrapped_position(
				const GPlatesMaths::PointOnSphere &point_on_sphere) const;
	};
}

#endif  // GPLATES_GUI_MAPCANVASPAINTER_H