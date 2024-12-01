/* $Id$ */

/**
 * \file 
 * Object factory for creating @a RenderedGeometry types.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
 * Copyright (C) 2010 Geological Survey of Norway
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYFACTORY_H
#define GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYFACTORY_H

#include <vector>
#include <boost/optional.hpp>
#include <QString>
#include <QFont>

#include "RenderedGeometry.h"
#include "RenderedColouredEdgeSurfaceMesh.h"
#include "RenderedColouredTriangleSurfaceMesh.h"
#include "RenderedRadialArrow.h"

#include "app-logic/ReconstructionGeometry.h"
#include "app-logic/ResolvedScalarField3D.h"
#include "app-logic/ResolvedRaster.h"

#include "gui/Colour.h"
#include "gui/ColourPalette.h"
#include "gui/ColourProxy.h"
#include "gui/Symbol.h"
#include "gui/RasterColourPalette.h"

#include "maths/GeometryOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/SmallCircle.h"
#include "maths/SmallCircleArc.h"

#include "property-values/Georeferencing.h"
#include "property-values/GpmlRasterBandNames.h"
#include "property-values/RawRaster.h"
#include "property-values/TextContent.h"

#include "view-operations/ScalarField3DRenderParameters.h"


namespace GPlatesAppLogic
{
	class Layer;
}

namespace GPlatesMaths
{
	class Vector3D;
}

namespace GPlatesViewOperations
{
	/*
	 * Object factory functions for creating @a RenderedGeometry types.
	 */

	namespace RenderedGeometryFactory
	{
		/**
		 * Typedef for sequence of @a RenderedGeometry objects.
		 */
		typedef std::vector<RenderedGeometry> rendered_geometry_seq_type;

		/**
		 * Default point size hint (roughly one screen-space pixel).
		 * This is an integer instead of float because it should always be one.
		 * If this value makes your default point too small then have a multiplying
		 * factor in your rendered implementation.
		 */
		const int DEFAULT_POINT_SIZE_HINT = 1;

		/**
		 * Default line width hint (roughly one screen-space pixel).
		 * This is an integer instead of float because it should always be one.
		 * If this value makes your default line width too small then have a multiplying
		 * factor in your rendered implementation.
		 */
		const int DEFAULT_LINE_WIDTH_HINT = 1;

		/**
		 * Default colour (white).
		 */
		const GPlatesGui::Colour DEFAULT_COLOUR = GPlatesGui::Colour::get_white();

		/**
		 * Determines the default size of an arrowhead relative to the globe radius
		 * when the globe fills the viewport window.
		 * This is a view-dependent scalar.
		 */
		const float DEFAULT_RATIO_ARROWHEAD_SIZE_TO_GLOBE_RADIUS = 0.03f;

		/**
		 * Determines the default size of an arrowhead (in device-independent pixels).
		 * This is a view-dependent scalar.
		 */
		const float DEFAULT_ARROWHEAD_SIZE_IN_PIXELS = 8.0f;

		/**
		 * Determines the default ratio of the width of an arrowline relative to the size of its arrowhead.
		 *
		 * The size of an arrowhead is actually its length (along the arrowline), not its width -
		 * the arrowhead width to arrowhead length ratio is fixed in the rendering engine in order
		 * to give the arrowhead a nice shape.
		 */
		const float DEFAULT_RATIO_ARROWLINE_WIDTH_TO_ARROWHEAD_SIZE = 0.2f;

		/**
		 * Determines the default width of a subduction tooth (in device-independent pixels).
		 * This is a view-dependent scalar.
		 */
		const float DEFAULT_SUBDUCTION_TEETH_WIDTH_IN_PIXELS = 8.0f;

		/**
		 * Determines the default spacing-to-width ratio of subduction teeth.
		 */
		const float DEFAULT_SUBDUCTION_TEETH_SPACING_TO_WIDTH_RATIO = 2.0;

		/**
		 * Determines the default height-to-width ratio of a subduction tooth .
		 */
		const float DEFAULT_SUBDUCTION_TEETH_HEIGHT_TO_WIDTH_RATIO = 2.0f / 3.0f;

		/**
		 * Determines the size of symbol rendered geometries.
		 */
		const unsigned int DEFAULT_SYMBOL_SIZE = 1;

		/**
		 * Creates a @a RenderedGeometry for a @a GeometryOnSphere.
		 *
		 * Both @a point_size_hint and @a line_width_hint are needed since
		 * the caller may not know what type of geometry it passed us.
		 *
		 * Note that @a symbol only applies to @a PointOnSphere geometries.
		 */
		RenderedGeometry
		create_rendered_geometry_on_sphere(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				float point_size_hint = DEFAULT_POINT_SIZE_HINT,
				float line_width_hint = DEFAULT_LINE_WIDTH_HINT,
				bool fill_polygon = false,
				bool fill_polyline = false,
				const GPlatesGui::Colour &fill_modulate_colour = DEFAULT_COLOUR,
				const boost::optional<GPlatesGui::Symbol> &symbol = boost::none);

		/**
		 * Creates a @a RenderedGeometry for a @a PointOnSphere.
		 */
		RenderedGeometry
		create_rendered_point_on_sphere(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				float point_size_hint = DEFAULT_POINT_SIZE_HINT);

		/**
		 * Creates a @a RenderedGeometry for a @a MultiPointOnSphere.
		 */
		RenderedGeometry
		create_rendered_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				float point_size_hint = DEFAULT_POINT_SIZE_HINT);

		/**
		 * Creates a @a RenderedGeometry for a @a PolylineOnSphere.
		 *
		 * If @a filled is true then the polyline is treated like a polygon when filling.
		 */
		RenderedGeometry
		create_rendered_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				float line_width_hint = DEFAULT_LINE_WIDTH_HINT,
				bool filled = false,
				const GPlatesGui::Colour &fill_modulate_colour = DEFAULT_COLOUR);

		/**
		 * Creates a @a RenderedGeometry for a @a PolygonOnSphere.
		 */
		RenderedGeometry
		create_rendered_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				float line_width_hint = DEFAULT_LINE_WIDTH_HINT,
				bool filled = false,
				const GPlatesGui::Colour &fill_modulate_colour = DEFAULT_COLOUR);


		/**
		 * Creates a @a RenderedGeometry for a @a GeometryOnSphere with per-point colouring.
		 *
		 * Both @a point_size_hint and @a line_width_hint are needed since
		 * the caller may not know what type of geometry it passed us.
		 *
		 * Note that @a symbol only applies to @a PointOnSphere geometries.
		 */
		RenderedGeometry
		create_rendered_coloured_geometry_on_sphere(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type,
				const std::vector<GPlatesGui::ColourProxy> &point_colours,
				float point_size_hint = DEFAULT_POINT_SIZE_HINT,
				float line_width_hint = DEFAULT_LINE_WIDTH_HINT,
				const boost::optional<GPlatesGui::Symbol> &symbol = boost::none);

		/**
		 * Creates a @a RenderedGeometry for a @a MultiPointOnSphere with per-point colouring.
		 *
		 * NOTE: The number of multipoint points must match the number of colours.
		 */
		RenderedGeometry
		create_rendered_coloured_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type,
				const std::vector<GPlatesGui::ColourProxy> &point_colours,
				float point_size_hint = DEFAULT_POINT_SIZE_HINT);

		/**
		 * Creates a @a RenderedGeometry for a @a PolylineOnSphere with per-point colouring.
		 *
		 * NOTE: The number of polyline points must match the number of colours.
		 */
		RenderedGeometry
		create_rendered_coloured_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
				const std::vector<GPlatesGui::ColourProxy> &point_colours,
				float line_width_hint = DEFAULT_LINE_WIDTH_HINT);

		/**
		 * Creates a @a RenderedGeometry for a @a PolygonOnSphere with per-point colouring.
		 *
		 * NOTE: The number of points in the *exterior* ring of polygon must match the number of colours.
		 */
		RenderedGeometry
		create_rendered_coloured_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type,
				const std::vector<GPlatesGui::ColourProxy> &point_colours,
				float line_width_hint = DEFAULT_LINE_WIDTH_HINT);


		/**
		 * Creates a @a RenderedGeometry for a coloured edge surface mesh.
		 */
		RenderedGeometry
		create_rendered_coloured_edge_surface_mesh(
				const RenderedColouredEdgeSurfaceMesh::edge_seq_type &mesh_edges,
				const RenderedColouredEdgeSurfaceMesh::vertex_seq_type &mesh_vertices,
				const RenderedColouredEdgeSurfaceMesh::colour_seq_type &mesh_colours,
				bool use_vertex_colours,
				float line_width_hint = DEFAULT_LINE_WIDTH_HINT);

		/**
		 * Creates a @a RenderedGeometry for a coloured triangle surface mesh.
		 */
		RenderedGeometry
		create_rendered_coloured_triangle_surface_mesh(
				const RenderedColouredTriangleSurfaceMesh::triangle_seq_type &mesh_triangles,
				const RenderedColouredTriangleSurfaceMesh::vertex_seq_type &mesh_vertices,
				const RenderedColouredTriangleSurfaceMesh::colour_seq_type &mesh_colours,
				bool use_vertex_colours,
				const GPlatesGui::Colour &fill_modulate_colour = DEFAULT_COLOUR);

		/**
		 * Creates a @a RenderedGeometry for a resolved raster.
		 */
		RenderedGeometry
		create_rendered_resolved_raster(
				const GPlatesAppLogic::ResolvedRaster::non_null_ptr_to_const_type &resolved_raster,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette,
				const GPlatesGui::Colour &raster_modulate_colour = GPlatesGui::Colour::get_white(),
				float normal_map_height_field_scale_factor = 1);

		/**
		 * Creates a @a RenderedGeometry for a resolved 3D scalar field.
		 */
		RenderedGeometry
		create_rendered_resolved_scalar_field_3d(
				const GPlatesAppLogic::ResolvedScalarField3D::non_null_ptr_to_const_type &resolved_scalar_field,
				const ScalarField3DRenderParameters &scalar_field_render_parameters);

		/**
		 * Creates a single arrow, tangential to the globe's surface, consisting of a straight line
		 * segment (in 3D globe view) and an arc (in 2D map views) with an arrowhead at the end.
		 *
		 * The length of the arrow will automatically scale with viewport zoom such that
		 * the projected length (onto the viewport window) remains constant.
		 *
		 * Note: because the projected length remains constant with zoom, arrows near each other,
		 * and pointing in the same direction, may overlap when zoomed out.
		 *
		 * The @a ratio_unit_vector_direction_to_globe_radius parameter is the length ratio of a
		 * unit-length arrow to the globe's radius when the zoom is such that the globe exactly
		 * (or pretty closely) fills the viewport window (either top-to-bottom if wide-screen
		 * otherwise left-to-right).
		 * Additional scaling occurs when @a arrow_direction is not a unit vector.
		 * For example, a value of 0.1 for @a ratio_unit_vector_direction_to_globe_radius
		 * will result in a velocity of magnitude 0.5 being drawn as an arc line of
		 * distance 0.1 * 0.5 = 0.05 multiplied by the globe's radius when the globe is zoomed
		 * such that it fits the viewport window.
		 *
		 * In a similar manner the projected size of the arrowhead remains constant with zoom
		 * except when the arrowhead size becomes greater than half the arrow length in which
		 * scaling changes such that the arrow head is always half the arrow length - this
		 * makes the arrowhead size scale to zero with the arrow length.
		 * And tiny arrows have tiny arrowheads that may not even be visible (if smaller than a pixel).
		 *
		 * @param start the start position of the arrow.
		 * @param arrow_direction the direction of the arrow (does not have to be unit-length).
		 *        Note that even though the direction is not constrained to be tangential to the globe's
		 *        surface (because it can be an arbitrary vector), in the 2D map views only the
		 *        tangential component is rendered.
		 * @param ratio_unit_vector_direction_to_globe_radius determines projected length
		 *        of a unit-vector arrow. There is no default value for this because it is not
		 *        known how the user has already scaled @a arrow_direction which is dependent
		 *        on the user's calculations (something this interface should not know about).
		 * @param colour is the colour of the arrow (body and head).
		 * @param ratio_arrowhead_size_to_globe_radius the size of the arrowhead relative to the
				  globe radius when the globe fills the viewport window (this is a view-dependent scalar).
		 * @param globe_view_ratio_arrowline_width_to_arrowhead_size width of line (or body) of arrow
		 *        as a ratio of the size of the arrowhead (as rendered in 3D globe view).
		 * @param map_view_arrowline_width_hint width of line (or body) of arrow (as rendered in 2D map views).
		 */
		RenderedGeometry
		create_rendered_tangential_arrow(
				const GPlatesMaths::PointOnSphere &start,
				const GPlatesMaths::Vector3D &arrow_direction,
				const float ratio_unit_vector_direction_to_globe_radius,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				const float ratio_arrowhead_size_to_globe_radius = DEFAULT_RATIO_ARROWHEAD_SIZE_TO_GLOBE_RADIUS,
				const float globe_view_ratio_arrowline_width_to_arrowhead_size = DEFAULT_RATIO_ARROWLINE_WIDTH_TO_ARROWHEAD_SIZE,
				const float map_view_arrowline_width_hint = DEFAULT_LINE_WIDTH_HINT);

		/**
		 * Creates a single arrow, radial (or normal) to the globe's surface, consisting of a line
		 * segment with an arrowhead at the end (in the 3D globe view) and a symbol (in 2D map views).
		 *
		 * The length of the arrow will automatically scale with viewport zoom such that
		 * the projected length (onto the viewport window) remains constant.
		 *
		 * The @a arrow_projected_length parameter is the length of the arrow when the zoom is such
		 * that the globe exactly (or pretty closely) fills the viewport window (either top-to-bottom
		 * if wide-screen otherwise left-to-right).
		 * For example, a value of 0.1 for @a arrow_projected_length will result in an arrow length
		 * of 0.1 multiplied by the globe's radius when the globe is zoomed such that it fits the
		 * viewport window.
		 *
		 * @param position on the globe/map.
		 * @param arrow_projected_length the length of the arrow relative to the globe radius when
		 *        the globe fills the viewport window (this is a view-dependent scalar).
		 *        Only in globe view.
		 * @param arrowhead_projected_size the size of the arrowhead relative to the globe radius when
		 *        the globe fills the viewport window (this is a view-dependent scalar).
		 *        Only in globe view.
		 * @param ratio_arrowhead_size_to_globe_radius the size of the arrowhead relative to the
		 *        globe radius when the globe fills the viewport window (this is a view-dependent scalar).
		 *        Only in globe view.
		 * @param arrow_colour the colour of the arrow (body and head).
		 *        Only in globe view.
		 * @param symbol_type type of symbol to draw in map view and at base of arrow in globe view.
		 * @param symbol_size size of symbol in *scene* coordinates (only in map view).
		 *        In globe view the symbol size matches the size of the arrow (cylindrical) body.
		 * @param symbol_colour colour of the symbol.
		 */
		RenderedGeometry
		create_rendered_radial_arrow(
				const GPlatesMaths::PointOnSphere &position,
				float arrow_projected_length,
				float arrowhead_projected_size = DEFAULT_RATIO_ARROWHEAD_SIZE_TO_GLOBE_RADIUS,
				float ratio_arrowline_width_to_arrowhead_size = DEFAULT_RATIO_ARROWLINE_WIDTH_TO_ARROWHEAD_SIZE,
				const GPlatesGui::ColourProxy &arrow_colour = DEFAULT_COLOUR,
				RenderedRadialArrow::SymbolType symbol_type = RenderedRadialArrow::SYMBOL_FILLED_CIRCLE,
				float symbol_size = DEFAULT_SYMBOL_SIZE,
				const GPlatesGui::ColourProxy &symbol_colour = DEFAULT_COLOUR);

		/**
		 * Creates a composite @a RenderedGeometry containing another @a RenderedGeometry
		 * and a @a ReconstructionGeometry associated with it.
		 */
		RenderedGeometry
		create_rendered_reconstruction_geometry(
				GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type reconstruction_geom,
				RenderedGeometry rendered_geom);

		/**
		 * Creates a composite @a RenderedGeometry containing another @a RenderedGeometry
		 * and multiple @a ReconstructionGeometry objects associated with it.
		 */
		RenderedGeometry
		create_rendered_multi_reconstruction_geometry(
				const std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type> &reconstruction_geoms,
				RenderedGeometry rendered_geom);

		/**
		 * Creates a @a RenderedGeometry for text.
		 */
		RenderedGeometry
		create_rendered_string(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				const QString &string,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				const GPlatesGui::ColourProxy &shadow_colour = GPlatesGui::ColourProxy(boost::none),
				int x_offset = 0,
				int y_offset = 0,
				const QFont &font = QFont());

		/**
		 * Creates a @a RenderedGeometry for a @a SmallCircle.
		 */
		RenderedGeometry
		create_rendered_small_circle(
				const GPlatesMaths::SmallCircle &small_circle,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				float line_width_hint = DEFAULT_LINE_WIDTH_HINT);						
		
		/**
		 * Creates a @a RenderedGeometry for a @a SmallCircleArc.
		 */
		RenderedGeometry
		create_rendered_small_circle_arc(
				const GPlatesMaths::SmallCircleArc &small_circle_arc,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				float line_width_hint = DEFAULT_LINE_WIDTH_HINT);				

		/**
		 * Creates a @a RenderedGeometry for an @a Ellipse.                                                                  
		 */
		RenderedGeometry
		create_rendered_ellipse(
				const GPlatesMaths::PointOnSphere &centre,
				const GPlatesMaths::Real &semi_major_axis_radians,
				const GPlatesMaths::Real &semi_minor_axis_radians,
				const GPlatesMaths::GreatCircle &axis,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				float line_width_hint = DEFAULT_LINE_WIDTH_HINT);

		/**
		 *  Creates a polyline rendered geometry with an arrowhead on each segment.                                                                      
		 */		
		RenderedGeometry
		create_rendered_arrowed_polyline(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				const float arrowhead_size_in_pixels = DEFAULT_ARROWHEAD_SIZE_IN_PIXELS,
				const float arrowline_width_hint = DEFAULT_LINE_WIDTH_HINT);

		/**
		 * Creates a @a RenderedGeometry for a @a PolylineOnSphere that has subduction teeth.
		 *
		 * If @a subduction_polarity_is_left is true then the overriding plate is on the left (otherwise on the right).
		 */
		RenderedGeometry
		create_rendered_subduction_teeth_polyline(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline,
				bool subduction_polarity_is_left,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				float line_width_hint = DEFAULT_LINE_WIDTH_HINT,
				float teeth_width_in_pixels = DEFAULT_SUBDUCTION_TEETH_WIDTH_IN_PIXELS,
				float teeth_spacing_to_width_ratio = DEFAULT_SUBDUCTION_TEETH_SPACING_TO_WIDTH_RATIO,
				float teeth_height_to_width_ratio = DEFAULT_SUBDUCTION_TEETH_HEIGHT_TO_WIDTH_RATIO);


		/**
		* Creates a symbol defined by @a symbol that is centred at @a centre.
		*
		* Symbol will be rendered on a tangent plane at the centre.
		*
		* @a line_width_hint is used if symbol contains any lines (eg, cross symbol).
		*/
		RenderedGeometry
		create_rendered_symbol(
				const GPlatesMaths::PointOnSphere &centre,
				const GPlatesGui::Symbol &symbol,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				float line_width_hint = DEFAULT_LINE_WIDTH_HINT);

		/**
		* Creates a triangle centred at @a centre. Triangle will
		* be rendered on a tangent plane at the centre.
		*/
		RenderedGeometry
		create_rendered_triangle_symbol(
			const GPlatesMaths::PointOnSphere &centre,
			const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
			const unsigned int size = DEFAULT_SYMBOL_SIZE,
			const bool filled = TRUE,
			const float line_width_hint = DEFAULT_LINE_WIDTH_HINT);


		/**
		 * Creates a square centred at @a centre. Square will
		 * be rendered on a tangent plane at the centre.
		 */
		RenderedGeometry
		create_rendered_square_symbol(
				const GPlatesMaths::PointOnSphere &centre,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				const unsigned int size = DEFAULT_SYMBOL_SIZE,
				const bool filled = TRUE,
				const float line_width_hint = DEFAULT_LINE_WIDTH_HINT);

		/**
		 * Creates a circle centred at @a centre. 
		 */
		RenderedGeometry
		create_rendered_circle_symbol(
				const GPlatesMaths::PointOnSphere &centre,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				const unsigned int size = DEFAULT_SYMBOL_SIZE,
				const bool filled = TRUE,
				const float line_width_hint = DEFAULT_LINE_WIDTH_HINT);

		/**
		 * Creates a cross centred at @a centre.  Cross will be north-south aligned and
		 * rendered on a tangent plane at the centre.
		 */
		RenderedGeometry
		create_rendered_cross_symbol(
				const GPlatesMaths::PointOnSphere &centre,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				const unsigned int size = DEFAULT_SYMBOL_SIZE,
				const float line_width_hint = DEFAULT_LINE_WIDTH_HINT);
		/**
		 * Creates a cross centred at @a centre.  StrainMarker will be aligned via angle and
		 * rendered on a tangent plane at the centre.
		 */
		RenderedGeometry
		create_rendered_strain_marker_symbol(
				const GPlatesMaths::PointOnSphere &centre,
				const unsigned int size = DEFAULT_SYMBOL_SIZE,
				const double scale_x = 0,
				const double scale_y = 0,
				const double angle = 0);
	}
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYFACTORY_H
