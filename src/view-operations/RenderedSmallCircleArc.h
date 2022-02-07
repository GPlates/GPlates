/* $Id$ */

/**
 * \file 
 * A @a RenderedGeometry derivation for @a PointOnSphere.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDSMALLCIRCLEARC_H
#define GPLATES_VIEWOPERATIONS_RENDEREDSMALLCIRCLEARC_H

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"

#include "maths/SmallCircleArc.h"

#include "gui/ColourProxy.h"

namespace GPlatesViewOperations
{
	class RenderedSmallCircleArc :
		public RenderedGeometryImpl
	{
	public:
		RenderedSmallCircleArc(
				const GPlatesMaths::SmallCircleArc &small_circle_arc,
				const GPlatesGui::ColourProxy &colour,
				float line_width_hint) :
		d_small_circle_arc(small_circle_arc),
		d_colour(colour),
		d_line_width_hint(line_width_hint)
		{  }

		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_rendered_small_circle_arc(*this);
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

		const GPlatesMaths::SmallCircleArc &
		get_small_circle_arc() const
		{
			return d_small_circle_arc;
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
		//! The small circle arc.
		GPlatesMaths::SmallCircleArc d_small_circle_arc;
		
		GPlatesGui::ColourProxy d_colour;
		float d_line_width_hint;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDSMALLCIRCLEARC_H
