/* $Id$ */

/**
 * \file 
 * A @a RenderedGeometry derivation for @a PointOnSphere.
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDPOINTONSPHERE_H
#define GPLATES_VIEWOPERATIONS_RENDEREDPOINTONSPHERE_H

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"
#include "gui/Colour.h"
#include "maths/PointOnSphere.h"

namespace GPlatesViewOperations
{
	class RenderedPointOnSphere :
		public RenderedGeometryImpl
	{
	public:
		RenderedPointOnSphere(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				const GPlatesGui::Colour &colour,
				float point_size_hint) :
			d_point_on_sphere(point_on_sphere),
			d_colour(colour),
			d_point_size_hint(point_size_hint)
		{  }

		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_rendered_point_on_sphere(*this);
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			return d_point_on_sphere.test_proximity(criteria);
		}
		
		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_vertex_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			return d_point_on_sphere.test_vertex_proximity(criteria);
		}		

		const GPlatesMaths::PointOnSphere &
		get_point_on_sphere() const
		{
			return d_point_on_sphere;
		}

		const GPlatesGui::Colour &
		get_colour() const
		{
			return d_colour;
		}

		float
		get_point_size_hint() const
		{
			return d_point_size_hint;
		}

	private:
		GPlatesMaths::PointOnSphere d_point_on_sphere;
		GPlatesGui::Colour d_colour;
		float d_point_size_hint;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDPOINTONSPHERE_H
