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

#include "app-logic/AppLogicFwd.h"
#include "app-logic/ReconstructionGeometry.h"

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
		 * Determines the size of symbol rendered geometries.
		 */
		const unsigned int DEFAULT_SYMBOL_SIZE = 1;

		/**
		 * Creates a @a RenderedGeometry for a @a GeometryOnSphere.
		 *
		 * Both @a point_size_hint and @a line_width_hint are needed since
		 * the caller may not know what type of geometry it passed us. 
		 */
		RenderedGeometry
		create_rendered_geometry_on_sphere(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				float point_size_hint = DEFAULT_POINT_SIZE_HINT,
				float line_width_hint = DEFAULT_LINE_WIDTH_HINT,
				bool fill_polygon = false,
				const boost::optional<GPlatesGui::Symbol> &symbol = boost::none);

		/**
		 * Creates a @a RenderedGeometry for a @a PointOnSphere.
		 */
		RenderedGeometry
		create_rendered_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				float point_size_hint = DEFAULT_POINT_SIZE_HINT);

		/**
		 * Creates a @a RenderedGeometry for a @a PointOnSphere.
		 */
		inline
		RenderedGeometry
		create_rendered_point_on_sphere(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				float point_size_hint = DEFAULT_POINT_SIZE_HINT)
		{
			return create_rendered_point_on_sphere(
					point_on_sphere.clone_as_point(), colour, point_size_hint);
		}

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
		 */
		RenderedGeometry
		create_rendered_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				float line_width_hint = DEFAULT_LINE_WIDTH_HINT);

		/**
		 * Creates a @a RenderedGeometry for a @a PolygonOnSphere.
		 */
		RenderedGeometry
		create_rendered_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				float line_width_hint = DEFAULT_LINE_WIDTH_HINT,
				bool filled = false);

		/**
		 * Creates a @a RenderedGeometry for a coloured edge surface mesh.
		 */
		RenderedGeometry
		create_rendered_coloured_edge_surface_mesh(
				const RenderedColouredEdgeSurfaceMesh::edge_seq_type &mesh_edges,
				const RenderedColouredEdgeSurfaceMesh::vertex_seq_type &mesh_vertices,
				float line_width_hint = DEFAULT_LINE_WIDTH_HINT);

		/**
		 * Creates a @a RenderedGeometry for a coloured triangle surface mesh.
		 */
		RenderedGeometry
		create_rendered_coloured_triangle_surface_mesh(
				const RenderedColouredTriangleSurfaceMesh::triangle_seq_type &mesh_triangles,
				const RenderedColouredTriangleSurfaceMesh::vertex_seq_type &mesh_vertices);

		/**
		 * Creates a @a RenderedGeometry for a resolved raster.
		 */
		RenderedGeometry
		create_rendered_resolved_raster(
				const GPlatesAppLogic::resolved_raster_non_null_ptr_to_const_type &resolved_raster,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette,
				const GPlatesGui::Colour &raster_modulate_colour = GPlatesGui::Colour::get_white());

		/**
		 * Creates a @a RenderedGeometry for a resolved 3D scalar field.
		 */
		RenderedGeometry
		create_rendered_resolved_scalar_field_3d(
				const GPlatesAppLogic::resolved_scalar_field_3d_non_null_ptr_to_const_type &resolved_scalar_field,
				const ScalarField3DRenderParameters &scalar_field_render_parameters);

		/**
		 * Creates a single direction arrow consisting of an arc line segment on the globe's surface
		 * with an arrowhead at the end.
		 *
		 * The length of the arc line will automatically scale with viewport zoom such that
		 * the projected length (onto the viewport window) remains constant.
		 *
		 * Note: because the projected length remains constant with zoom, arrows near each other,
		 * and pointing in the same direction, may overlap when zoomed out.
		 *
		 * The @a ratio_unit_vector_direction_to_globe_radius parameter is the length ratio of a
		 * unit-length direction arrow to the globe's radius when the zoom is such that the globe
		 * exactly (or pretty closely) fills the viewport window (either top-to-bottom if wide-screen
		 * otherwise left-to-right).
		 * Additional scaling occurs when @a arrow_direction is not a unit vector.
		 *
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
		 * @param ratio_unit_vector_direction_to_globe_radius determines projected length
		 *        of a unit-vector arrow. There is no default value for this because it is not
		 *        known how the user has already scaled @a arrow_direction which is dependent
		 *        on the user's calculations (something this interface should not know about).
		 * @param colour is the colour of the arrow (body and head).
		 * @param ratio_arrowhead_size_to_globe_radius the size of the arrowhead relative to the
				  globe radius when the globe fills the viewport window (this is a view-dependent scalar).
		 * @param arrowline_width_hint width of line (or body) of arrow.
		 */
		RenderedGeometry
		create_rendered_direction_arrow(
				const GPlatesMaths::PointOnSphere &start,
				const GPlatesMaths::Vector3D &arrow_direction,
				const float ratio_unit_vector_direction_to_globe_radius,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				const float ratio_arrowhead_size_to_globe_radius = DEFAULT_RATIO_ARROWHEAD_SIZE_TO_GLOBE_RADIUS,
				const float arrowline_width_hint = DEFAULT_LINE_WIDTH_HINT);

		/**
		 * Creates a composite @a RenderedGeometry containing another @a RenderedGeometry
		 * and a @a ReconstructionGeometry associated with it.
		 */
		RenderedGeometry
		create_rendered_reconstruction_geometry(
				GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type reconstruction_geom,
				RenderedGeometry rendered_geom);

		/**
		 * Creates a @a RenderedGeometry for text.
		 */
		RenderedGeometry
		create_rendered_string(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type,
				const QString &,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				const GPlatesGui::ColourProxy &shadow_colour = GPlatesGui::ColourProxy(boost::none),
				int x_offset = 0,
				int y_offset = 0,
				const QFont &font = QFont());

		/**
		 * Creates a @a RenderedGeometry for text.
		 */
		inline
		RenderedGeometry
		create_rendered_string(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				const QString &string,
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				const GPlatesGui::ColourProxy &shadow_colour = GPlatesGui::ColourProxy(boost::none),
				int x_offset = 0,
				int y_offset = 0,
				const QFont &font = QFont())
		{
			return create_rendered_string(
					point_on_sphere.clone_as_point(), string, colour, shadow_colour, x_offset, y_offset, font);
		}

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
				const float ratio_arrowhead_size_to_globe_radius = DEFAULT_RATIO_ARROWHEAD_SIZE_TO_GLOBE_RADIUS,
				const float arrowline_width_hint = DEFAULT_LINE_WIDTH_HINT);


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
				const GPlatesGui::ColourProxy &colour = DEFAULT_COLOUR,
				const unsigned int size = DEFAULT_SYMBOL_SIZE,
				const float line_width_hint = DEFAULT_LINE_WIDTH_HINT,
				const double scale_x = 0,
				const double scale_y = 0,
				const double angle = 0);
	}
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYFACTORY_H
