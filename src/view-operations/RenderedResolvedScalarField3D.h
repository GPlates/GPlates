/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_VIEW_OPERATIONS_RENDEREDRESOLVEDSCALARFIELD3D_H
#define GPLATES_VIEW_OPERATIONS_RENDEREDRESOLVEDSCALARFIELD3D_H

#include <vector>
#include <boost/optional.hpp>

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"
#include "ScalarField3DRenderParameters.h"

#include "app-logic/ResolvedScalarField3D.h"

#include "gui/ColourPalette.h"


namespace GPlatesViewOperations
{
	class RenderedResolvedScalarField3D :
			public RenderedGeometryImpl
	{
	public:

		explicit
		RenderedResolvedScalarField3D(
				const GPlatesAppLogic::ResolvedScalarField3D::non_null_ptr_to_const_type &resolved_scalar_field,
				const ScalarField3DRenderParameters &render_parameters) :
			d_resolved_scalar_field(resolved_scalar_field),
			d_render_parameters(render_parameters)
		{  }

		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_rendered_resolved_scalar_field_3d(*this);
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			// TODO: Implement query into scalar field active tiles (and active tile mask).
			return GPlatesMaths::ProximityHitDetail::null;
		}

		GPlatesAppLogic::ResolvedScalarField3D::non_null_ptr_to_const_type
		get_resolved_scalar_field_3d() const
		{
			return d_resolved_scalar_field;
		}

		const ScalarField3DRenderParameters &
		get_render_parameters() const
		{
			return d_render_parameters;
		}

		/**
		 * Returns the reconstruction time at which scalar field is resolved.
		 */
		const double &
		get_reconstruction_time() const
		{
			return d_resolved_scalar_field->get_reconstruction_time();
		}

	private:
		/**
		 * The resolved scalar field.
		 */
		GPlatesAppLogic::ResolvedScalarField3D::non_null_ptr_to_const_type d_resolved_scalar_field;

		/**
		 * Parameters that determine how to render the scalar field.
		 */
		ScalarField3DRenderParameters d_render_parameters;
	};
}

#endif // GPLATES_VIEW_OPERATIONS_RENDEREDRESOLVEDSCALARFIELD3D_H
