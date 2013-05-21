/* $Id$ */

/**
 * \file 
 * A @a RenderedGeometry derivation for @a PolylineOnSphere.
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDPOLYLINEONSPHERE_H
#define GPLATES_VIEWOPERATIONS_RENDEREDPOLYLINEONSPHERE_H

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"

#include "gui/Colour.h"
#include "gui/ColourProxy.h"

#include "maths/PointInPolygon.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PolylineProximityHitDetail.h"
#include "maths/ProximityCriteria.h"


namespace GPlatesViewOperations
{
	class RenderedPolylineOnSphere :
		public RenderedGeometryImpl
	{
	public:
		RenderedPolylineOnSphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere,
				const GPlatesGui::ColourProxy &colour,
				float line_width_hint,
				bool filled,
				const GPlatesGui::Colour &fill_modulate_colour) :
		d_polyline_on_sphere(polyline_on_sphere),
		d_colour(colour),
		d_line_width_hint(line_width_hint),
		d_is_filled(filled),
		d_fill_modulate_colour(fill_modulate_colour)
		{  }

		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_rendered_polyline_on_sphere(*this);
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type hit =
					d_polyline_on_sphere->test_proximity(criteria);
			if (hit)
			{
				return hit;
			}

			// If the polyline is filled then see if the test point is inside the filled polyline's interior.
			if (get_is_filled() &&
				// Need at least three vertices for a polygon...
				d_polyline_on_sphere->number_of_vertices() > 2)
			{
				// Create a temporary polygon from the polyline and test the point against it.
				const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type filled_polyline =
						GPlatesMaths::PolygonOnSphere::create_on_heap(
								d_polyline_on_sphere->vertex_begin(),
								d_polyline_on_sphere->vertex_end());

				// We don't need a fast point-in-polygon test since this is typically a user click
				// point (ie, a single point tested against the polygon)...
				const GPlatesMaths::PointInPolygon::Result point_in_polygon_result =
						GPlatesMaths::PointInPolygon::is_point_in_polygon(
								criteria.test_point(),
								*filled_polyline);

				if (point_in_polygon_result == GPlatesMaths::PointInPolygon::POINT_INSIDE_POLYGON)
				{
					// The point is inside the filled polygon, hence it touches the fill region and
					// therefore has a closeness distance of zero (which is a dot product closeness of 1.0).
					return make_maybe_null_ptr(
							GPlatesMaths::PolylineProximityHitDetail::create(
									d_polyline_on_sphere,
									1.0/*closeness*/));
				}
			}

			// No hit.
			return GPlatesMaths::ProximityHitDetail::null;
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_vertex_proximity(
			const GPlatesMaths::ProximityCriteria &criteria) const
		{
			return d_polyline_on_sphere->test_vertex_proximity(criteria);
		}
		
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
		get_polyline_on_sphere() const
		{
			return d_polyline_on_sphere;
		}

		const GPlatesGui::ColourProxy &
		get_colour() const
		{
			return d_colour;
		}

		float
		get_line_width_hint() const
		{
			return d_line_width_hint;
		}

		bool
		get_is_filled() const
		{
			return d_is_filled;
		}

		const GPlatesGui::Colour &
		get_fill_modulate_colour() const
		{
			return d_fill_modulate_colour;
		}

	private:
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type d_polyline_on_sphere;
		GPlatesGui::ColourProxy d_colour;
		float d_line_width_hint;
		bool d_is_filled;
		GPlatesGui::Colour d_fill_modulate_colour;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDPOLYLINEONSPHERE_H
