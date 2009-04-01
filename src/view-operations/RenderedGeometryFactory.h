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
	 * Creates a dashed polyline from the specified @a PolylineOnSphere.
	 *
	 * The individual polyline segments are dashed in such a way that they
	 * look continuous across the entire polyline.
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
