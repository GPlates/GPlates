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

#include "app-logic/ReconstructionGeometry.h"
#include "app-logic/ReconstructRasterPolygons.h"

#include "gui/Colour.h"
#include "gui/ColourProxy.h"
#include "gui/RasterColourPalette.h"

#include "maths/GeometryOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/SmallCircle.h"

#include "property-values/Georeferencing.h"
#include "property-values/GpmlRasterBandNames.h"
#include "property-values/RawRaster.h"
#include "property-values/TextContent.h"


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
		 * Creates a @a RenderedGeometry for a @a GeometryOnSphere.
		 *
		 * Both @a point_size_hint and @a line_width_hint are needed since
		 * the caller may not know what type of geometry it passed us. 
		 */
		RenderedGeometry
		create_rendered_geometry_on_sphere(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
				float point_size_hint = RenderedGeometryFactory::DEFAULT_POINT_SIZE_HINT,
				float line_width_hint = RenderedGeometryFactory::DEFAULT_LINE_WIDTH_HINT);

		/**
		 * Creates a @a RenderedGeometry for a @a PointOnSphere.
		 */
		RenderedGeometry
		create_rendered_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
				float point_size_hint = RenderedGeometryFactory::DEFAULT_POINT_SIZE_HINT);

		/**
		 * Creates a @a RenderedGeometry for a @a PointOnSphere.
		 */
		inline
		RenderedGeometry
		create_rendered_point_on_sphere(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
				float point_size_hint = RenderedGeometryFactory::DEFAULT_POINT_SIZE_HINT)
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
				const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
				float point_size_hint = RenderedGeometryFactory::DEFAULT_POINT_SIZE_HINT);

		/**
		 * Creates a @a RenderedGeometry for a @a PolylineOnSphere.
		 */
		RenderedGeometry
		create_rendered_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
				float line_width_hint = RenderedGeometryFactory::DEFAULT_LINE_WIDTH_HINT);

		/**
		 * Creates a @a RenderedGeometry for a @a PolygonOnSphere.
		 */
		RenderedGeometry
		create_rendered_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
				float line_width_hint = RenderedGeometryFactory::DEFAULT_LINE_WIDTH_HINT);

		/**
		 * Creates a @a RenderedGeometry for a resolved raster.
		 *
		 * @a layer is the layer the resolved raster was created in.
		 * FIXME: This is a temporary solution to tracking persistent OpenGL objects.
		 */
		RenderedGeometry
		create_rendered_resolved_raster(
				const GPlatesAppLogic::Layer &layer,
				const GPlatesPropertyValues::TextContent &raster_band_name,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette,
				const double &reconstruction_time,
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
				const std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &proxied_rasters,
				const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &raster_band_names,
				const boost::optional<GPlatesAppLogic::ReconstructRasterPolygons::non_null_ptr_to_const_type> &
						reconstruct_raster_polygons = boost::none,
				const boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> &
						age_grid_georeferencing = boost::none,
				const boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> > &
						age_grid_proxied_rasters = boost::none,
				const boost::optional<GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type> &
						age_grid_raster_band_names = boost::none);

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
				const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
				const float ratio_arrowhead_size_to_globe_radius =
						RenderedGeometryFactory::DEFAULT_RATIO_ARROWHEAD_SIZE_TO_GLOBE_RADIUS,
				const float arrowline_width_hint =
						RenderedGeometryFactory::DEFAULT_LINE_WIDTH_HINT);

		/**
		 * Creates a dashed polyline from the specified @a PolylineOnSphere.
		 *
		 * The individual polyline segments are dashed in such a way that they
		 * look continuous across the entire polyline.
		 *
		 * Note: currently implemented as a solid line.
		 */
		RenderedGeometry
		create_rendered_dashed_polyline(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR);

		/**
		 * Creates a sequence of dashed polyline rendered geometries from
		 * the specified @a PolylineOnSphere - one for each line segment.
		 *
		 * The individual polyline segments are dashed in such a way that they
		 * look continuous across the entire polyline.
		 * The difference with the above method is each segment can be queried individually.
		 *
		 * Note: currently implemented as a solid line segments.
		 */
		RenderedGeometryFactory::rendered_geometry_seq_type
		create_rendered_dashed_polyline_segments_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR);

		/**
		 * Creates a composite @a RenderedGeometry containing another @a RenderedGeometry
		 * and a @a ReconstructionGeometry associated with it.
		 */
		RenderedGeometry
		create_rendered_reconstruction_geometry(
				GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type reconstruction_geom,
				RenderedGeometry rendered_geom);
	}

	/**
	 * Creates a @a RenderedGeometry for text.
	 */
	RenderedGeometry
	create_rendered_string(
			GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type,
			const QString &,
			const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
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
			const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
			const GPlatesGui::ColourProxy &shadow_colour = GPlatesGui::ColourProxy(boost::none),
			int x_offset = 0,
			int y_offset = 0,
			const QFont &font = QFont())
	{
		return create_rendered_string(
				point_on_sphere.clone_as_point(), string, colour, shadow_colour, x_offset, y_offset, font);
	}

	/**
	 * Creates a @a RenderedGeometry for a @a MultiPointOnSphere.
	 */
	RenderedGeometry
	create_rendered_multi_point_on_sphere(
			GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type,
			const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
			float point_size_hint = RenderedGeometryFactory::DEFAULT_POINT_SIZE_HINT);

	/**
	 * Creates a @a RenderedGeometry for a @a PolylineOnSphere.
	 */
	RenderedGeometry
	create_rendered_polyline_on_sphere(
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
			const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
			float line_width_hint = RenderedGeometryFactory::DEFAULT_LINE_WIDTH_HINT);

	/**
	 * Creates a @a RenderedGeometry for a @a PolygonOnSphere.
	 */
	RenderedGeometry
	create_rendered_polygon_on_sphere(
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type,
			const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
			float line_width_hint = RenderedGeometryFactory::DEFAULT_LINE_WIDTH_HINT);		

	/**
	 * Creates a @a RenderedGeometry for a @a SmallCircle.
	 */
	RenderedGeometry
	create_rendered_small_circle(
		const GPlatesMaths::PointOnSphere &centre,
		const GPlatesMaths::Real &radius_in_radians,
		const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
		float line_width_hint = RenderedGeometryFactory::DEFAULT_LINE_WIDTH_HINT);						
	
	/**
	 * Creates a @a RenderedGeometry for a @a SmallCircleArc.
	 */
	RenderedGeometry
	create_rendered_small_circle_arc(
		const GPlatesMaths::PointOnSphere &centre,
		const GPlatesMaths::PointOnSphere &start_point,
		const GPlatesMaths::Real &arc_length_in_radians,
		const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
		float line_width_hint = RenderedGeometryFactory::DEFAULT_LINE_WIDTH_HINT);				

	/**
	 * Creates a @a RenderedGeometry for an @a Ellipse.                                                                  
	 */
	RenderedGeometry
	create_rendered_ellipse(
		const GPlatesMaths::PointOnSphere &centre,
		const GPlatesMaths::Real &semi_major_axis_radians,
		const GPlatesMaths::Real &semi_minor_axis_radians,
		const GPlatesMaths::GreatCircle &axis,
		const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
		float line_width_hint = RenderedGeometryFactory::DEFAULT_LINE_WIDTH_HINT);


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
			const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
			const float ratio_arrowhead_size_to_globe_radius =
					RenderedGeometryFactory::DEFAULT_RATIO_ARROWHEAD_SIZE_TO_GLOBE_RADIUS,
			const float arrowline_width_hint =
					RenderedGeometryFactory::DEFAULT_LINE_WIDTH_HINT);

	/**
	 * Creates a dashed polyline from the specified @a PolylineOnSphere.
	 *
	 * The individual polyline segments are dashed in such a way that they
	 * look continuous across the entire polyline.
	 *
	 * Note: currently implemented as a solid line.
	 */
	RenderedGeometry
	create_rendered_dashed_polyline(
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
			const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR);

	/**
	 * Creates a sequence of dashed polyline rendered geometries from
	 * the specified @a PolylineOnSphere - one for each line segment.
	 *
	 * The individual polyline segments are dashed in such a way that they
	 * look continuous across the entire polyline.
	 * The difference with the above method is each segment can be queried individually.
	 *
	 * Note: currently implemented as a solid line segments.
	 */
	RenderedGeometryFactory::rendered_geometry_seq_type
	create_rendered_dashed_polyline_segments_on_sphere(
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
			const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR);

	/**
	 * Creates a composite @a RenderedGeometry containing another @a RenderedGeometry
	 * and a @a ReconstructionGeometry associated with it.
	 */
	RenderedGeometry
	create_rendered_reconstruction_geometry(
			GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type reconstruction_geom,
			RenderedGeometry rendered_geom);

	/**
	 *  Creates a polyline rendered geometry with an arrowhead on each segment.                                                                      
	 */		
	RenderedGeometry
	create_rendered_arrowed_polyline(
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
			const GPlatesGui::ColourProxy &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
			const float ratio_arrowhead_size_to_globe_radius =
				RenderedGeometryFactory::DEFAULT_RATIO_ARROWHEAD_SIZE_TO_GLOBE_RADIUS,
			const float arrowline_width_hint =
				RenderedGeometryFactory::DEFAULT_LINE_WIDTH_HINT);
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYFACTORY_H
