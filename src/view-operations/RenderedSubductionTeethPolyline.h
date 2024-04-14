/**
 * Copyright (C) 2024 The University of Sydney, Australia
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

#ifndef GPLATES_VIEWOPERATIONS_RENDERED_SUBDUCTION_TEETH_POLYLINE_H
#define GPLATES_VIEWOPERATIONS_RENDERED_SUBDUCTION_TEETH_POLYLINE_H

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"

#include "maths/PolylineOnSphere.h"

#include "gui/ColourProxy.h"


namespace GPlatesViewOperations
{
	/**
	 * A polyline with subduction teeth.
	 */
	class RenderedSubductionTeethPolyline :
		public RenderedGeometryImpl
	{
	public:

		enum class SubductionPolarity { LEFT, RIGHT };

		RenderedSubductionTeethPolyline(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere,
				SubductionPolarity subduction_polarity,
				const GPlatesGui::ColourProxy &colour,
				float line_width_hint,
				float teeth_width_in_pixels,
				float teeth_spacing_to_width_ratio,
				float teeth_height_to_width_ratio) :
		d_polyline_on_sphere(polyline_on_sphere),
		d_subduction_polarity(subduction_polarity),
		d_colour(colour),
		d_line_width_hint(line_width_hint),
		d_teeth_width_in_pixels(teeth_width_in_pixels),
		d_teeth_spacing_to_width_ratio(teeth_spacing_to_width_ratio),
		d_teeth_height_to_width_ratio(teeth_height_to_width_ratio)
		{  }

		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor &visitor)
		{
			visitor.visit_rendered_subduction_teeth_polyline(*this);
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

		SubductionPolarity
		get_subduction_polarity() const
		{
			return d_subduction_polarity;
		}

		const GPlatesGui::ColourProxy &
		get_colour() const
		{
			return d_colour;
		}

		//! The line width (in device-independent pixels).
		float
		get_line_width_hint() const
		{
			return d_line_width_hint;
		}

		//! The width of a tooth (in device-independent pixels).
		float
		get_teeth_width_in_pixels() const
		{
			return d_teeth_width_in_pixels;
		}

		float
		get_teeth_spacing_to_width_ratio() const
		{
			return d_teeth_spacing_to_width_ratio;
		}

		float
		get_teeth_height_to_width_ratio() const
		{
			return d_teeth_height_to_width_ratio;
		}

	private:
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type d_polyline_on_sphere;
		SubductionPolarity d_subduction_polarity;
		GPlatesGui::ColourProxy d_colour;
		float d_line_width_hint;
		float d_teeth_width_in_pixels;
		float d_teeth_spacing_to_width_ratio;
		float d_teeth_height_to_width_ratio;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDERED_SUBDUCTION_TEETH_POLYLINE_H
