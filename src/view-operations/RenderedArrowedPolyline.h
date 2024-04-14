/* $Id: RenderedArrowedPolyline.h 8617 2010-06-03 09:39:53Z rwatson $ */

/**
 * \file 
 * A @a RenderedGeometry derivation for @a PolylineOnSphere.
 * $Revision: 8617 $
 * $Date: 2010-06-03 11:39:53 +0200 (to, 03 jun 2010) $
 * 
 * Copyright (C) 2009, 2010 Geological Survey of Norway
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDARROWEDPOLYLINE_H
#define GPLATES_VIEWOPERATIONS_RENDEREDARROWEDPOLYLINE_H

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"
#include "gui/ColourProxy.h"
#include "maths/PolylineOnSphere.h"

namespace GPlatesViewOperations
{
	class RenderedArrowedPolyline :
		public RenderedGeometryImpl
	{
	public:
		RenderedArrowedPolyline(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type points,
				const GPlatesGui::ColourProxy &colour,
				float arrowhead_size_in_pixels,
				float arrowline_width_hint) :
		d_points(points),
		d_colour(colour),
		d_arrowhead_size_in_pixels(arrowhead_size_in_pixels),
		d_arrowline_width_hint(arrowline_width_hint)
		{  }

		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_rendered_arrowed_polyline(*this);
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			// We may want to allow querying of the arrowed polyline later.
			return d_points->test_proximity(criteria);
		}

		virtual	
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_vertex_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			return d_points->test_vertex_proximity(criteria);
		}

		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
		get_polyline_on_sphere() const
		{
			return d_points;
		}

		const GPlatesGui::ColourProxy &
		get_colour() const
		{
			return d_colour;
		}
		
		//! The size of the arrowhead (in device-independent pixels).
		float
		get_arrowhead_size_in_pixels() const
		{
			return d_arrowhead_size_in_pixels;
		}
		
		//! The arrow line width (in device-independent pixels).
		float
		get_arrowline_width_hint() const
		{
			return d_arrowline_width_hint;
		}

	private:
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type d_points;
		const GPlatesGui::ColourProxy d_colour;
		const float d_arrowhead_size_in_pixels;
		const float d_arrowline_width_hint;		
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDARROWEDPOLYLINE_H
