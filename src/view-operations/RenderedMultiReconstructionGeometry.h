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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDMULTIRECONSTRUCTIONGEOMETRY_H
#define GPLATES_VIEWOPERATIONS_RENDEREDMULTIRECONSTRUCTIONGEOMETRY_H

#include <vector>

#include "RenderedGeometry.h"
#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"

#include "app-logic/ReconstructionGeometry.h"


namespace GPlatesViewOperations
{
	class RenderedMultiReconstructionGeometry :
		public RenderedGeometryImpl
	{
	public:
		RenderedMultiReconstructionGeometry(
				const std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type> &reconstruction_geoms,
				RenderedGeometry rendered_geom) :
		d_reconstruction_geoms(reconstruction_geoms),
		d_rendered_geom(rendered_geom)
		{  }

		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_rendered_multi_reconstruction_geometry(*this);

			// Also visit our internal rendered geometry.
			d_rendered_geom.accept_visitor(visitor);
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			return d_rendered_geom.test_proximity(criteria);
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_vertex_proximity(
			const GPlatesMaths::ProximityCriteria &criteria) const
		{
			return d_rendered_geom.test_vertex_proximity(criteria);
		}

		const std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type> &
		get_reconstruction_geometries() const
		{
			return d_reconstruction_geoms;
		}

	private:
		std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type> d_reconstruction_geoms;
		RenderedGeometry d_rendered_geom;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDMULTIRECONSTRUCTIONGEOMETRY_H
