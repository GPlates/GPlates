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
				float scalar_field_iso_value,
				const GPlatesGui::ColourPalette<double>::non_null_ptr_to_const_type &scalar_field_colour_palette,
				const std::vector<float> &shader_test_variables) :
			d_resolved_scalar_field(resolved_scalar_field),
			d_scalar_field_iso_value(scalar_field_iso_value),
			d_scalar_field_colour_palette(scalar_field_colour_palette),
			d_shader_test_variables(shader_test_variables)
		{  }

		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_resolved_scalar_field_3d(*this);
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

		/**
		 * The iso-value to render the iso-surface at.
		 */
		float
		get_scalar_field_iso_value() const
		{
			return d_scalar_field_iso_value;
		}

		GPlatesGui::ColourPalette<double>::non_null_ptr_to_const_type
		get_scalar_field_colour_palette() const
		{
			return d_scalar_field_colour_palette;
		}

		const std::vector<float> &
		get_shader_test_variables() const
		{
			return d_shader_test_variables;
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
		 * The iso-value to render the iso-surface at.
		 */
		float d_scalar_field_iso_value;

		/**
		 * The colour palette used to colour the scalar field.
		 */
		GPlatesGui::ColourPalette<double>::non_null_ptr_to_const_type d_scalar_field_colour_palette;

		/**
		 * Used during test/development of the scalar field shader program.
		 */
		std::vector<float> d_shader_test_variables;
	};
}

#endif // GPLATES_VIEW_OPERATIONS_RENDEREDRESOLVEDSCALARFIELD3D_H
