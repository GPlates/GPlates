/* $Id$ */

/**
 * \file 
 * A @a RenderedGeometry derivation for @a PointOnSphere.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010, 2011 Geological Survey of Norway
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

#include "maths/SmallCircle.h"

#include "gui/ColourProxy.h"
#include "maths/ProximityCriteria.h"
#include "maths/SmallCircleProximityHitDetail.h"


namespace GPlatesViewOperations
{
	class RenderedSmallCircle :
		public RenderedGeometryImpl
	{
	public:

		RenderedSmallCircle(
				const GPlatesMaths::SmallCircle &small_circle,
				const GPlatesGui::ColourProxy &colour,
			float line_width_hint) :
			d_small_circle(small_circle),
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
			/**
			 * See PointOnSphere class for a discussion of the idea of closeness of two points.
			 * We can use a similar measure here based on the dot-product of the test point and
			 * the point on the small circle which lies closest to that test point. 
			 *
			 * To do this we need to determine the point on the small circle closest to the test point.
			 * We could do this by forming the great circle passing through the small circle centre and the
			 * test point, and work out the intersect of this with the small circle, and then we'd have to
			 * determine which of the two intercept points lies closest to the test point.
			 * 
			 * Alternatively we can determine the angle between the test point and the centre, 
			 * and subtract the angle-radius of the small circle, and take the cosine of the result.
			 */
			GPlatesMaths::Real test_point_to_centre_cos_angle = GPlatesMaths::dot(
					criteria.test_point().position_vector(),
					d_small_circle.axis_vector());

			GPlatesMaths::Real angle = GPlatesMaths::acos(test_point_to_centre_cos_angle);

			GPlatesMaths::Real difference = d_small_circle.colatitude() - angle;
			
			GPlatesMaths::Real closeness = GPlatesMaths::cos(difference);

			if (closeness.dval() > criteria.closeness_inclusion_threshold())
			{
				return make_maybe_null_ptr(GPlatesMaths::SmallCircleProximityHitDetail::create(
					closeness.dval()));
			}
			else
			{
				return GPlatesMaths::ProximityHitDetail::null;
			}

		}
		
		
		const GPlatesMaths::SmallCircle &
		get_small_circle() const
		{
			return d_small_circle;
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

		GPlatesMaths::SmallCircle d_small_circle;

		GPlatesGui::ColourProxy d_colour;
		float d_line_width_hint;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDSMALLCIRCLE_H
