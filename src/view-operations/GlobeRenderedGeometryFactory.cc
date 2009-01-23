/* $Id$ */

/**
 * \file 
 * Extends @a RenderedGeometryFactory to handle creation of @a RenderedGeometry objects
 * that are dependent on the 3D Globe view.
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

#include <vector>

#include "GlobeRenderedGeometryFactory.h"
#include "maths/PolylineOnSphere.h"


GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::GlobeRenderedGeometryFactory::create_rendered_dashed_polyline(
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline,
		const GPlatesGui::Colour &colour)
{
	// Until this is implemented we'll just return a regular polyline.
	return create_rendered_polyline_on_sphere(polyline, colour, 1.0f);
}

GPlatesViewOperations::GlobeRenderedGeometryFactory::rendered_geometry_seq_type
GPlatesViewOperations::GlobeRenderedGeometryFactory::create_rendered_dashed_polyline_segments_on_sphere(
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline,
		const GPlatesGui::Colour &colour)
{
	//
	// Until this is implemented we'll just return a regular polyline for
	// each segment of the input polyline.
	//

	// The sequence we'll return.
	rendered_geometry_seq_type polyline_seq;
	polyline_seq.reserve(polyline->number_of_segments());

	// Iterate over great circle arcs of the input polyline.
	GPlatesMaths::PolylineOnSphere::const_iterator polyline_iter;
	for (polyline_iter = polyline->begin();
		polyline_iter != polyline->end();
		++polyline_iter)
	{
		// Start and end point of current segment.
		GPlatesMaths::PointOnSphere segment[2] =
		{
			// Start and end point of current segment.
			polyline_iter->start_point(),
			polyline_iter->end_point()
		};

		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_segment =
			GPlatesMaths::PolylineOnSphere::create_on_heap(segment, segment + 2);

		const RenderedGeometry rendered_geom_segment =
			create_rendered_polyline_on_sphere(polyline_segment, colour, 1.0f);

		polyline_seq.push_back(rendered_geom_segment);
	}

	return polyline_seq;
}
