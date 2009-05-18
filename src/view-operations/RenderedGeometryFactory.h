/* $Id$ */

/**
 * \file 
 * Object factory for creating @a RenderedGeometry types.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include "RenderedGeometry.h"
#include "gui/Colour.h"
#include "maths/GeometryOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/MultiPointOnSphere.h"
#include "model/ReconstructionGeometry.h"


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
		 * Determines the default size of an arrowhead relative to the arrow line.
		 */
		const float DEFAULT_RATIO_SIZE_ARROWHEAD_TO_ARROWLINE = 1 / 3.0f;
	}

	/**
	 * Creates a @a RenderedGeometry for a @a GeometryOnSphere.
	 *
	 * Both @a point_size_hint and @a line_width_hint are needed since
	 * the caller may not know what type of geometry it passed us. 
	 */
	RenderedGeometry
	create_rendered_geometry_on_sphere(
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type,
			const GPlatesGui::Colour &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
			float point_size_hint = RenderedGeometryFactory::DEFAULT_POINT_SIZE_HINT,
			float line_width_hint = RenderedGeometryFactory::DEFAULT_LINE_WIDTH_HINT);

	/**
	 * Creates a @a RenderedGeometry for a @a PointOnSphere.
	 */
	RenderedGeometry
	create_rendered_point_on_sphere(
			GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type,
			const GPlatesGui::Colour &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
			float point_size_hint = RenderedGeometryFactory::DEFAULT_POINT_SIZE_HINT);

	/**
	 * Creates a @a RenderedGeometry for a @a PointOnSphere.
	 */
	inline
	RenderedGeometry
	create_rendered_point_on_sphere(
			const GPlatesMaths::PointOnSphere &point_on_sphere,
			const GPlatesGui::Colour &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
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
			const GPlatesGui::Colour &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
			float point_size_hint = RenderedGeometryFactory::DEFAULT_POINT_SIZE_HINT);

	/**
	 * Creates a @a RenderedGeometry for a @a PolylineOnSphere.
	 */
	RenderedGeometry
	create_rendered_polyline_on_sphere(
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
			const GPlatesGui::Colour &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
			float line_width_hint = RenderedGeometryFactory::DEFAULT_LINE_WIDTH_HINT);

	/**
	 * Creates a @a RenderedGeometry for a @a PolygonOnSphere.
	 */
	RenderedGeometry
	create_rendered_polygon_on_sphere(
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type,
			const GPlatesGui::Colour &colour = RenderedGeometryFactory::DEFAULT_COLOUR,
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
	 * The ratio of arrowhead size to arrow line length remains constant such that longer arrows
	 * have proportionately larger arrowheads.
	 * And tiny arrows have tiny arrowheads that may not even be visible (if smaller than a pixel).
	 *
	 * @param start the start position of the arrow.
	 * @param arrow_direction the direction of the arrow (does not have to be unit-length).
	 * @param ratio_unit_vector_direction_to_globe_radius determines projected length
	 *        of a unit-vector arrow. There is no default value for this because it is not
	 *        known how the user has already scaled @a arrow_direction which is dependent
	 *        on the user's calculations (something this interface should not know about).
	 * @param colour is the colour of the arrow (body and head).
	 * @param arrowline_width_hint width of line (or body) of arrow.
	 * @param ratio_size_arrowhead_to_arrowline the size of the arrowhead relative to the arrow line.
	 */
	RenderedGeometry
	create_rendered_direction_arrow(
			const GPlatesMaths::PointOnSphere &start,
			const GPlatesMaths::Vector3D &arrow_direction,
			const float ratio_unit_vector_direction_to_globe_radius,
			const GPlatesGui::Colour &colour =
					RenderedGeometryFactory::DEFAULT_COLOUR,
			const float arrowline_width_hint =
					RenderedGeometryFactory::DEFAULT_LINE_WIDTH_HINT,
 			/*const*/ float ratio_size_arrowhead_to_arrowline =
					RenderedGeometryFactory::DEFAULT_RATIO_SIZE_ARROWHEAD_TO_ARROWLINE);

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
			const GPlatesGui::Colour &colour = RenderedGeometryFactory::DEFAULT_COLOUR);

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
			const GPlatesGui::Colour &colour = RenderedGeometryFactory::DEFAULT_COLOUR);

	/**
	 * Creates a composite @a RenderedGeometry containing another @a RenderedGeometry
	 * and a @a ReconstructionGeometry associated with it.
	 */
	RenderedGeometry
	create_rendered_reconstruction_geometry(
			GPlatesModel::ReconstructionGeometry::non_null_ptr_type reconstruction_geom,
			RenderedGeometry rendered_geom);
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYFACTORY_H
