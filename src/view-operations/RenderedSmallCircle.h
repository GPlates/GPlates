/* $Id$ */

/**
 * \file 
 * A @a RenderedGeometry derivation for @a PointOnSphere.
 * $Revision$
 * $Date$
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDSMALLCIRCLE_H
#define GPLATES_VIEWOPERATIONS_RENDEREDSMALLCIRCLE_H

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"
#include "gui/ColourProxy.h"


namespace GPlatesViewOperations
{
	class RenderedSmallCircle :
		public RenderedGeometryImpl
	{
	public:

		RenderedSmallCircle(
			const GPlatesMaths::PointOnSphere &centre,
			const GPlatesMaths::Real &radius_in_radians,
			const GPlatesGui::ColourProxy &colour,
			float line_width_hint) :
			d_centre(centre),
			d_radius_in_radians(radius_in_radians),
			d_colour(colour),
			d_line_width_hint(line_width_hint)
		{  }		
		

		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_rendered_small_circle(*this);
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			// FIXME: We'll probably want to perform proximity tests on small circles
			// further down the line. 
			return NULL;
		}
		
		const GPlatesMaths::PointOnSphere &
		get_centre() const
		{
			return d_centre;
		}
		
		GPlatesMaths::Real
		get_radius_in_radians() const
		{
			return d_radius_in_radians;
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

	private:

		GPlatesMaths::PointOnSphere d_centre;
		GPlatesMaths::Real d_radius_in_radians;

		GPlatesGui::ColourProxy d_colour;
		float d_line_width_hint;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDSMALLCIRCLE_H
