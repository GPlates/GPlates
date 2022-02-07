/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_PRESENTATION_RECONSTRUCTSCALARCOVERAGEVISUALLAYERPARAMS_H
#define GPLATES_PRESENTATION_RECONSTRUCTSCALARCOVERAGEVISUALLAYERPARAMS_H

#include "VisualLayerParams.h"

#include "view-operations/RenderedGeometryParameters.h"


namespace GPlatesPresentation
{
	class ReconstructScalarCoverageVisualLayerParams :
			public VisualLayerParams
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructScalarCoverageVisualLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructScalarCoverageVisualLayerParams> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create(
			GPlatesAppLogic::LayerTaskParams &layer_task_params,
			const GPlatesViewOperations::RenderedGeometryParameters &rendered_geometry_parameters)
		{
			return new ReconstructScalarCoverageVisualLayerParams(layer_task_params, rendered_geometry_parameters);
		}


		virtual
		void
		accept_visitor(
				ConstVisualLayerParamsVisitor &visitor) const
		{
			visitor.visit_reconstruct_scalar_coverage_visual_layer_params(*this);
		}

		virtual
		void
		accept_visitor(
				VisualLayerParamsVisitor &visitor)
		{
			visitor.visit_reconstruct_scalar_coverage_visual_layer_params(*this);
		}

	protected:

		explicit 
		ReconstructScalarCoverageVisualLayerParams( 
				GPlatesAppLogic::LayerTaskParams &layer_task_params,
				const GPlatesViewOperations::RenderedGeometryParameters &rendered_geometry_parameters) :
			VisualLayerParams(layer_task_params)
		{  }

	private:

	};
}

#endif // GPLATES_PRESENTATION_RECONSTRUCTSCALARCOVERAGEVISUALLAYERPARAMS_H
