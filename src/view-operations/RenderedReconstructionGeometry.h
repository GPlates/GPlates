/* $Id$ */

/**
 * \file A composite @a RenderedGeometry that contains a @a ReconstructionGeometry.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDRECONSTRUCTIONGEOMETRY_H
#define GPLATES_VIEWOPERATIONS_RENDEREDRECONSTRUCTIONGEOMETRY_H

#include "RenderedGeometry.h"
#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"
#include "model/ReconstructionGeometry.h"


namespace GPlatesViewOperations
{
	class RenderedReconstructionGeometry :
		public RenderedGeometryImpl
	{
	public:
		RenderedReconstructionGeometry(
				GPlatesModel::ReconstructionGeometry::non_null_ptr_type reconstruction_geom,
				RenderedGeometry rendered_geom) :
		d_reconstruction_geom(reconstruction_geom),
		d_rendered_geom(rendered_geom)
		{  }

		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_rendered_reconstruction_geometry(*this);

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

		GPlatesModel::ReconstructionGeometry::non_null_ptr_type
		get_reconstruction_geometry() const
		{
			return d_reconstruction_geom;
		}

	private:
		GPlatesModel::ReconstructionGeometry::non_null_ptr_type d_reconstruction_geom;
		RenderedGeometry d_rendered_geom;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDRECONSTRUCTIONGEOMETRY_H
