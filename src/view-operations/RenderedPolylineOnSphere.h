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
#include "gui/ColourProxy.h"
#include "maths/PolylineOnSphere.h"

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
			return d_polyline_on_sphere->test_proximity(criteria);
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
