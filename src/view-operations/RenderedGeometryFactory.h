/* $Id$ */

/**
 * \file 
 * Partially abstract interface for creating @a RenderedGeometry types.
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


namespace GPlatesViewOperations
{
	/**
	 * Partially abstract interface for creating @a RenderedGeometry types.
	 *
	 * This interface is partially abstract because some types of @a RenderedGeometry
	 * contain implementations that are dependent on the canvas view (there
	 * is a globe canvas view and a map projections canvas view).
	 * An example is dashed lines - which vary the number of dashes as the user
	 * zooms the viewport in order to keep the dashed line from zooming.
	 */
	class RenderedGeometryFactory
	{
	public:
		/**
		 * Typedef for sequence of @a RenderedGeometry objects.
		 */
		typedef std::vector<RenderedGeometry> rendered_geometry_seq_type;

		virtual
		~RenderedGeometryFactory()
		{  }

		/**
		 * Default point size hint (roughly one screen-space pixel).
		 * This is an integer instead of float because it should always be one.
		 * If this value makes your default point too small then have a multiplying
		 * factor in your rendered implementation.
		 */
		static const int DEFAULT_POINT_SIZE_HINT = 1;

		/**
		 * Default line width hint (roughly one screen-space pixel).
		 * This is an integer instead of float because it should always be one.
		 * If this value makes your default line width too small then have a multiplying
		 * factor in your rendered implementation.
		 */
		static const int DEFAULT_LINE_WIDTH_HINT = 1;

		/**
		 * Default colour (white).
		 */
		static const GPlatesGui::Colour DEFAULT_COLOUR;

		//
		// Non-abstract methods
		//

		/**
		 * Creates a @a RenderedGeometry for a @a GeometryOnSphere.
		 *
		 * Both @a point_size_hint and @a line_width_hint are needed since
		 * the caller may not know what type of geometry it passed us. 
		 */
		virtual
		RenderedGeometry
		create_rendered_geometry_on_sphere(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::Colour &colour = DEFAULT_COLOUR,
				float point_size_hint = DEFAULT_POINT_SIZE_HINT,
				float line_width_hint = DEFAULT_LINE_WIDTH_HINT);

		/**
		 * Creates a @a RenderedGeometry for a @a PointOnSphere.
		 */
		virtual
		RenderedGeometry
		create_rendered_point_on_sphere(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				const GPlatesGui::Colour &colour = DEFAULT_COLOUR,
				float point_size_hint = DEFAULT_POINT_SIZE_HINT)
		{
			return create_rendered_point_on_sphere(
					point_on_sphere.clone_as_point(), colour, point_size_hint);
		}

		/**
		 * Creates a @a RenderedGeometry for a @a PointOnSphere.
		 */
		virtual
		RenderedGeometry
		create_rendered_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::Colour &colour = DEFAULT_COLOUR,
				float point_size_hint = DEFAULT_POINT_SIZE_HINT);

		/**
		 * Creates a @a RenderedGeometry for a @a MultiPointOnSphere.
		 */
		virtual
		RenderedGeometry
		create_rendered_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::Colour &colour = DEFAULT_COLOUR,
				float point_size_hint = DEFAULT_POINT_SIZE_HINT);

		/**
		 * Creates a @a RenderedGeometry for a @a PolylineOnSphere.
		 */
		virtual
		RenderedGeometry
		create_rendered_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::Colour &colour = DEFAULT_COLOUR,
				float line_width_hint = DEFAULT_LINE_WIDTH_HINT);

		/**
		 * Creates a @a RenderedGeometry for a @a PolygonOnSphere.
		 */
		virtual
		RenderedGeometry
		create_rendered_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::Colour &colour = DEFAULT_COLOUR,
				float line_width_hint = DEFAULT_LINE_WIDTH_HINT);

		//
		// Abstract methods
		//

		/**
		 * Creates a dashed polyline from the specified @a PolylineOnSphere.
		 *
		 * The individual polyline segments are dashed in such a way that they
		 * look continuous across the entire polyline.
		 */
		virtual
		RenderedGeometry
		create_rendered_dashed_polyline(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::Colour &colour = DEFAULT_COLOUR) = 0;

		/**
		 * Creates a sequence of dashed polyline rendered geometries from
		 * the specified @a PolylineOnSphere - one for each line segment.
		 *
		 * The individual polyline segments are dashed in such a way that they
		 * look continuous across the entire polyline.
		 * The difference with the above method is each segment can be queried individually.
		 */
		virtual
		rendered_geometry_seq_type
		create_rendered_dashed_polyline_segments_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::Colour &colour = DEFAULT_COLOUR) = 0;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYFACTORY_H
