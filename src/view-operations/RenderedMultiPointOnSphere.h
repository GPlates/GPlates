/* $Id$ */

/**
 * \file 
 * A @a RenderedGeometry derivation for @a MultiPointOnSphere.
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDMULTIPOINTONSPHERE_H
#define GPLATES_VIEWOPERATIONS_RENDEREDMULTIPOINTONSPHERE_H

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"
#include "gui/ColourProxy.h"
#include "maths/MultiPointOnSphere.h"

namespace GPlatesViewOperations
{
	class RenderedMultiPointOnSphere :
		public RenderedGeometryImpl
	{
	public:
		RenderedMultiPointOnSphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere,
				const GPlatesGui::ColourProxy &colour,
				float point_size_hint) :
		d_multi_point_on_sphere(multi_point_on_sphere),
		d_colour(colour),
		d_point_size_hint(point_size_hint)
		{  }

		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_rendered_multi_point_on_sphere(*this);
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

		const GPlatesGui::ColourProxy &
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
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type d_multi_point_on_sphere;
		GPlatesGui::ColourProxy d_colour;
		float d_point_size_hint;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDMULTIPOINTONSPHERE_H
