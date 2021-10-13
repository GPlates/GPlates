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
			GPlatesAppLogic::LayerTaskParams &layer_task_params)
		{
			return new VelocityFieldCalculatorVisualLayerParams( layer_task_params );
		}

		bool
		show_delaunay_vectors() const
		{
			return d_show_delaunay_vectors;
		}

		void
		set_show_delaunay_vectors(
				bool b)
		{
			d_show_delaunay_vectors = b;
			emit_modified();
		}

		bool
		show_constrained_vectors() const
		{
			return d_show_constrained_vectors;
		}

		void
		set_show_constrained_vectors(
				bool b)
		{
			d_show_constrained_vectors = b;
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

		explicit 
		VelocityFieldCalculatorVisualLayerParams( 
				GPlatesAppLogic::LayerTaskParams &layer_task_params ) :
			VisualLayerParams(layer_task_params),
			d_show_delaunay_vectors(true),
			d_show_constrained_vectors(true)
		{  }

	private:

		bool d_show_delaunay_vectors;
		bool d_show_constrained_vectors;
	};
}

#endif // GPLATES_PRESENTATION_VELOCITYFIELDCALCULATORVISAULLAYERPARAMS_H
