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

#include "Colour.h"
#include "ColourProxy.h"
#include "ColourScheme.h"
#include "LayerPainter.h"

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
				const double &device_independent_pixel_to_map_space_ratio,
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
		visit_rendered_point_on_sphere(
				const GPlatesViewOperations::RenderedPointOnSphere &rendered_point_on_sphere);

		virtual
		void
		visit_rendered_multi_point_on_sphere(
				const GPlatesViewOperations::RenderedMultiPointOnSphere &rendered_multi_point_on_sphere);

		virtual
		void
		visit_rendered_coloured_multi_point_on_sphere(
				const GPlatesViewOperations::RenderedColouredMultiPointOnSphere &rendered_coloured_multi_point_on_sphere);
		
		virtual
		void
		visit_rendered_polyline_on_sphere(
				const GPlatesViewOperations::RenderedPolylineOnSphere &rendered_polyline_on_sphere);
		
		virtual
		void
		visit_rendered_coloured_polyline_on_sphere(
				const GPlatesViewOperations::RenderedColouredPolylineOnSphere &rendered_coloured_polyline_on_sphere);

		virtual
		void
		visit_rendered_subduction_teeth_polyline(
				const GPlatesViewOperations::RenderedSubductionTeethPolyline &rendered_subduction_teeth_polyline);

		virtual
		void
		visit_rendered_polygon_on_sphere(
				const GPlatesViewOperations::RenderedPolygonOnSphere &rendered_polygon_on_sphere);

		virtual
		void
		visit_rendered_coloured_polygon_on_sphere(
				const GPlatesViewOperations::RenderedColouredPolygonOnSphere &rendered_coloured_polygon_on_sphere);


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

			/**
			 * For polylines, each point in the wrapped polyline can be considered to be an
			 * interpolation of two end points of an original polyline segment.
			 *
			 * For polygons, each point in a ring of the wrapped polygon (excluding tessellated dateline points)
			 * can be considered to be an interpolation of two end points of an original polygon ring segment.
			 *
			 * This is used to interpolate colours defined at the original points onto the new
			 * (wrapped and tessellated) points.
			 */
			struct InterpolateOriginalSegment
			{
				InterpolateOriginalSegment(
						const double &interpolate_ratio_,
						unsigned int original_segment_index_,
						unsigned int original_geometry_part_index_ = 0/*default for polylines*/) :
					interpolate_ratio(interpolate_ratio_),
					original_segment_index(original_segment_index_),
					original_geometry_part_index(original_geometry_part_index_)
				{  }

				double interpolate_ratio;
				unsigned int original_segment_index;
				unsigned int original_geometry_part_index;
			};

			//! Typedef for a sequence of optional @a InterpolateOriginalSegment.
			typedef std::vector< boost::optional<InterpolateOriginalSegment> > interpolate_original_segment_seq_type;


			/**
			 * Add a vertex to the current geometry.
			 *
			 * Set @a is_original_point to true if the vertex is from the original geometry.
			 * Set to false if vertex is a dateline wrapped vertex or a new tessellated vertex.
			 *
			 * Set @a is_on_dateline to true if the vertex is on the dateline.
			 * This includes dateline wrapped vertices and tessellated vertices of a segment on the dateline.
			 *
			 * Set @a interpolate_original_segment to the interpolate information (or none if
			 * a tessellated point along dateline - applies to polygons only).
			 */
			void
			add_vertex(
					const QPointF &vertex,
					bool is_original_point,
					bool is_on_dateline,
					boost::optional<InterpolateOriginalSegment> interpolate_original_segment = boost::none)
			{
				d_vertices.push_back(vertex);
				d_is_original_point_flags.push_back(is_original_point);
				d_is_on_dateline_flags.push_back(is_on_dateline);
				d_interpolate_original_segments.push_back(interpolate_original_segment);
			}

			/**
			 * Call this *after* adding a part of a line geometry's vertices.
			 *
			 * For polylines there is only one part per geometry (polyline).
			 * For polygons there is one part per ring (exterior and interiors).
			 *
			 * Any vertices added after this call will contribute to the next geometry part.
			 */
			void
			add_geometry_part()
			{
				d_geometry_parts.push_back(d_vertices.size());
			}

			/**
			 * Call this *after* adding the last geometry part for a geometry.
			 *
			 * For polylines there is only one part per geometry (polyline).
			 * For polygons there is one part per ring (exterior and interiors).
			 *
			 * Any vertices added after this call will contribute to the next geometry part.
			 */
			void
			add_geometry()
			{
				d_geometries.push_back(d_geometry_parts.size());
			}


			/**
			 * Returns vertices in all geometries.
			 */
			const std::vector<QPointF> &
			get_vertices() const
			{
				return d_vertices;
			}

			/**
			 * Returns the boolean flags indicating whether vertices in @a get_vertices (at same indices)
			 * are original (unwrapped and untessellated) vertices (see @a add_vertex).
			 */
			const std::vector<bool> &
			get_is_original_point_flags() const
			{
				return d_is_original_point_flags;
			}

			/**
			 * Returns the boolean flags indicating whether vertices in @a get_vertices (at same indices)
			 * are on the dateline (see @a add_vertex).
			 */
			const std::vector<bool> &
			get_is_on_dateline_flags() const
			{
				return d_is_on_dateline_flags;
			}

			/**
			 * Returns optional interpolate information for vertices in @a get_vertices (at same indices)
			 * that can be used to interpolate colours at original vertices.
			 *
			 * For polylines, all vertices have interpolate information. However, for polygons, the
			 * vertices tessellated along the dateline do not lie along original segments and hence
			 * these vertices have no interpolate information.
			 */
			const interpolate_original_segment_seq_type &
			get_interpolate_original_segments() const
			{
				return d_interpolate_original_segments;
			}

			/**
			 * Returns a vertex index (into @a get_vertices) for each geometry part
			 * that represents 'one past' the last vertex in that geometry part.
			 */
			const std::vector<unsigned int> &
			get_geometry_parts()
			{
				return d_geometry_parts;
			}

			/**
			 * Returns a vertex index (into @a get_vertices) for each geometry
			 * that represents 'one past' the last vertex in the last part in that geometry.
			 */
			const std::vector<unsigned int> &
			get_geometries()
			{
				return d_geometries;
			}

		private:

			std::vector<QPointF> d_vertices;
			std::vector<bool> d_is_original_point_flags;
			std::vector<bool> d_is_on_dateline_flags;
			std::vector< boost::optional<InterpolateOriginalSegment> > d_interpolate_original_segments;

			std::vector<unsigned int> d_geometry_parts;
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

		//! The size of one device-independent pixel in (post projection) map space units.
		const double d_device_independent_pixel_to_map_space_ratio;

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
		 * Determines the colour of vector geometries.
		 *
		 * Returns colour of a ColourProxy using our colour scheme.
		 *
		 * TODO: Remove colour schemes when full symbology implemented.
		 * We're no longer really using colour schemes (via colour proxies) anymore since
		 * the Python colouring code generates colours directly (ie, our ColourProxy objects have
		 * colours stored internally instead of delegating to a colour scheme).
		 */
		boost::optional<Colour>
		get_vector_geometry_colour(
				const ColourProxy &colour_proxy)
		{
			return colour_proxy.get_colour(d_colour_scheme);
		}

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
		 * Project and tessellate great circle arcs of an *unwrapped* polyline.
		 */
		void
		project_and_tessellate_unwrapped_polyline(
				DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
				const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &polyline_on_sphere);

		/**
		 * Project and tessellate great circle arcs of an *unwrapped* polygon.
		 */
		void
		project_and_tessellate_unwrapped_polygon(
				DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
				const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &polygon_on_sphere);

		/**
		 * Project and tessellate great circle arcs of an *unwrapped* polyline, or ring (part) of a polygon.
		 */
		template <typename GreatCircleArcForwardIter>
		void
		project_and_tessellate_unwrapped_geometry_part(
				DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
				const GreatCircleArcForwardIter &begin_arcs,
				const GreatCircleArcForwardIter &end_arcs,
				unsigned int geometry_part_index = 0/*default for polylines*/);

		/**
		 * Project and tessellate a *wrapped* polyline.
		 */
		void
		project_tessellated_wrapped_polyline(
				DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
				const GPlatesMaths::DateLineWrapper::LatLonPolyline &wrapped_polyline);

		/**
		 * Project and tessellate a *wrapped* polygon.
		 */
		void
		project_tessellated_wrapped_polygon(
				DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
				const GPlatesMaths::DateLineWrapper::LatLonPolygon &wrapped_polygon);

		/**
		 * Project and tessellate a *wrapped* ring (part) of a polygon.
		 */
		void
		project_tessellated_wrapped_ring(
				DatelineWrappedProjectedLineGeometry &dateline_wrapped_projected_line_geometry,
				const GPlatesMaths::DateLineWrapper::lat_lon_points_seq_type &lat_lon_points,
				const std::vector<GPlatesMaths::DateLineWrapper::LatLonPolygon::point_flags_type> &point_flags,
				const GPlatesMaths::DateLineWrapper::LatLonPolygon::interpolate_original_segment_seq_type &interpolate_original_segments);

		/**
		 * Paints a *filled* line geometry (polyline or polygon) as a filled polygon.
		 */
		template <typename LineGeometryType>
		void
		paint_fill_geometry(
				GPlatesOpenGL::GLFilledPolygonsMapView::filled_drawables_type &filled_polygons,
				const typename LineGeometryType::non_null_ptr_to_const_type &line_geometry,
				rgba8_t rgba8_color);

		/**
		 * Paints a line geometry (polyline or polygon).
		 */
		template <typename LineGeometryType>
		void
		paint_line_geometry(
				const typename LineGeometryType::non_null_ptr_to_const_type &line_geometry,
				rgba8_t rgba8_color,
				stream_primitives_type &lines_stream,
				boost::optional<double> arrow_head_size = boost::none);

		/**
		 * Paints a polyline with per-vertex colouring.
		 */
		void
		paint_vertex_coloured_polyline(
				const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &polyline,
				const std::vector<Colour> &original_vertex_colours,
				stream_primitives_type &lines_stream);

		/**
		 * Paints a polygon with per-vertex colouring.
		 */
		void
		paint_vertex_coloured_polygon(
				const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &polygon,
				const std::vector<Colour> &original_vertex_colours,
				stream_primitives_type &lines_stream);

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
