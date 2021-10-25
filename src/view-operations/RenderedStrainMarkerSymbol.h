/**
 * \file
 * $Revision: 8672 $
 * $Date: 2010-06-10 07:00:38 +0200 (to, 10 jun 2010) $
 *
 * Copyright (C) 2010 Geological Survey of Norway
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDSTRAINMARKERSYMBOL_H
#define GPLATES_VIEWOPERATIONS_RENDEREDSTRAINMARKERSYMBOL_H

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"

#include "maths/PointOnSphere.h"


namespace GPlatesViewOperations
{
	/**
	 * Rendered  strain marker geometry. 
	 */
	class RenderedStrainMarkerSymbol :
			public RenderedGeometryImpl
	{
	public:

		RenderedStrainMarkerSymbol(
				const GPlatesMaths::PointOnSphere &centre,
				unsigned int size,
				double scale_x,
				double scale_y,
				double angle) :
			d_centre(centre),
			d_size(size),
			d_scale_x(scale_x),
			d_scale_y(scale_y),
			d_angle(angle)
		{  }


		virtual
		void
		accept_visitor(
		ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_rendered_strain_marker_symbol(*this);
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
		const GPlatesMaths::ProximityCriteria &criteria) const
		{
			return d_centre.test_proximity(criteria);
		}


		const GPlatesMaths::PointOnSphere &
		get_centre() const
		{
			return d_centre;
		}

		unsigned int
		get_size() const
		{
			return d_size;
		}

		double
		get_scale_x() const
		{
			return d_scale_x;
		}

		double
		get_scale_y() const
		{
			return d_scale_y;
		}

		double
		get_angle() const
		{
			return d_angle;
		}

	private:

		GPlatesMaths::PointOnSphere d_centre;

		unsigned int d_size;
		double d_scale_x;
		double d_scale_y;
		double d_angle;

	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDSTRAINMARKERSYMBOL_H
