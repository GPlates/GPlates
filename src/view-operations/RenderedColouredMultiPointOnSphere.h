/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDCOLOUREDMULTIPOINTONSPHERE_H
#define GPLATES_VIEWOPERATIONS_RENDEREDCOLOUREDMULTIPOINTONSPHERE_H

#include <vector>

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"

#include "app-logic/GeometryUtils.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/ColourProxy.h"

#include "maths/MultiPointOnSphere.h"


namespace GPlatesViewOperations
{
	class RenderedColouredMultiPointOnSphere :
		public RenderedGeometryImpl
	{
	public:
		RenderedColouredMultiPointOnSphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere,
				const std::vector<GPlatesGui::ColourProxy> &point_colours,
				float point_size_hint) :
		d_multi_point_on_sphere(multi_point_on_sphere),
		d_point_colours(point_colours),
		d_point_size_hint(point_size_hint)
		{
			// Number of colours must match number of geometry points.
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					point_colours.size() ==
						GPlatesAppLogic::GeometryUtils::get_num_geometry_exterior_points(*multi_point_on_sphere),
					GPLATES_ASSERTION_SOURCE);
		}

		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_rendered_coloured_multi_point_on_sphere(*this);
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			return d_multi_point_on_sphere->test_proximity(criteria);
		}
		
		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_vertex_proximity(
			const GPlatesMaths::ProximityCriteria &criteria) const
		{
			return d_multi_point_on_sphere->test_vertex_proximity(criteria);
		}		

		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type
		get_multi_point_on_sphere() const
		{
			return d_multi_point_on_sphere;
		}

		const std::vector<GPlatesGui::ColourProxy> &
		get_point_colours() const
		{
			return d_point_colours;
		}

		float
		get_point_size_hint() const
		{
			return d_point_size_hint;
		}

	private:
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type d_multi_point_on_sphere;
		std::vector<GPlatesGui::ColourProxy> d_point_colours;
		float d_point_size_hint;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDCOLOUREDMULTIPOINTONSPHERE_H
