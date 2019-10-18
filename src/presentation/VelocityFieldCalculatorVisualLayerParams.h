/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_PRESENTATION_VELOCITYFIELDCALCULATORVISAULLAYERPARAMS_H
#define GPLATES_PRESENTATION_VELOCITYFIELDCALCULATORVISAULLAYERPARAMS_H

#include "VisualLayerParams.h"

#include "ViewState.h"

#include "view-operations/RenderedGeometryParameters.h"


namespace GPlatesPresentation
{
	class VelocityFieldCalculatorVisualLayerParams :
			public VisualLayerParams
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<VelocityFieldCalculatorVisualLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const VelocityFieldCalculatorVisualLayerParams> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create(
				GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params,
				ViewState &view_state)
		{
			return new VelocityFieldCalculatorVisualLayerParams(layer_params, view_state);
		}


		float
		get_arrow_body_scale() const
		{
			return d_arrow_body_scale;
		}

		//! Set the arrow body scale of rendered arrows.
		void
		set_arrow_body_scale(
				float arrow_body_scale)
		{
			d_arrow_body_scale = arrow_body_scale;
			emit_modified();
		}


		float
		get_arrowhead_scale() const
		{
			return d_arrowhead_scale;
		}

		//! Set the arrowhead scale of rendered arrows.
		void
		set_arrowhead_scale(
				float arrowhead_scale)
		{
			d_arrowhead_scale = arrowhead_scale;
			emit_modified();
		}


		float
		get_arrow_spacing() const
		{
			return d_arrow_spacing;
		}

		/**
		 * Set the screen-space spacing of rendered arrows.
		 *
		 * A value of zero has the special meaning of unlimited density (ie, no limit on number of arrows).
		 * NOTE: Small values can cause large memory usage.
		 */
		void
		set_arrow_spacing(
				float arrow_spacing)
		{
			d_arrow_spacing = arrow_spacing;
			emit_modified();
		}


		virtual
		void
		accept_visitor(
				ConstVisualLayerParamsVisitor &visitor) const
		{
			visitor.visit_velocity_field_calculator_visual_layer_params(*this);
		}

		virtual
		void
		accept_visitor(
				VisualLayerParamsVisitor &visitor)
		{
			visitor.visit_velocity_field_calculator_visual_layer_params(*this);
		}

	protected:

		VelocityFieldCalculatorVisualLayerParams( 
				GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params,
				ViewState &view_state) :
			VisualLayerParams(layer_params, view_state),
			d_arrow_spacing(view_state.get_rendered_geometry_parameters()
					.get_reconstruction_layer_arrow_spacing()),
			d_arrow_body_scale(view_state.get_rendered_geometry_parameters()
					.get_reconstruction_layer_ratio_arrow_unit_vector_direction_to_globe_radius()),
			d_arrowhead_scale(view_state.get_rendered_geometry_parameters()
					.get_reconstruction_layer_ratio_arrowhead_size_to_globe_radius())
		{  }

	private:

		float d_arrow_spacing;
		float d_arrow_body_scale;
		float d_arrowhead_scale;
	};
}

#endif // GPLATES_PRESENTATION_VELOCITYFIELDCALCULATORVISAULLAYERPARAMS_H
