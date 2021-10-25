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
 
#ifndef GPLATES_PRESENTATION_VISUALLAYERPARAMSVISITOR_H
#define GPLATES_PRESENTATION_VISUALLAYERPARAMSVISITOR_H

#include "utils/SetConst.h"


namespace GPlatesPresentation
{
	// Forward declarations of supported VisualLayerParams derivations.
	class RasterVisualLayerParams;
	class ReconstructVisualLayerParams;
	class ScalarField3DVisualLayerParams;
	class TopologyBoundaryVisualLayerParams;
	class TopologyNetworkVisualLayerParams;
	class VelocityFieldCalculatorVisualLayerParams;

	/**
	 * This class is a base class for visitors that visit VisualLayerParams.
	 * For convenience, typedefs are provided below to cover the const and non-const cases.
	 */
	template<bool Const>
	class VisualLayerParamsVisitorBase
	{
	public:

		// Typedefs to give the supported derivations the appropriate const-ness.
		typedef typename GPlatesUtils::SetConst<TopologyNetworkVisualLayerParams, Const>::type 
			topology_network_visual_layer_params_type;
		typedef typename GPlatesUtils::SetConst<RasterVisualLayerParams, Const>::type raster_visual_layer_params_type;
		typedef typename GPlatesUtils::SetConst<ReconstructVisualLayerParams, Const>::type reconstruct_visual_layer_params_type;
		typedef typename GPlatesUtils::SetConst<ScalarField3DVisualLayerParams, Const>::type scalar_field_3d_visual_layer_params_type;
		typedef typename GPlatesUtils::SetConst<TopologyBoundaryVisualLayerParams, Const>::type topology_boundary_visual_layer_params_type;


		typedef typename GPlatesUtils::SetConst<VelocityFieldCalculatorVisualLayerParams, Const>::type 
			velocity_field_calculator_visual_layer_params_type;

		virtual
		~VisualLayerParamsVisitorBase()
		{  }

		virtual
		void
		visit_raster_visual_layer_params(
				raster_visual_layer_params_type &params)
		{  }

		virtual
		void
		visit_reconstruct_visual_layer_params(
				reconstruct_visual_layer_params_type &params)
		{  }

		virtual
		void
		visit_scalar_field_3d_visual_layer_params(
				scalar_field_3d_visual_layer_params_type &params)
		{  }

		virtual
		void
		visit_topology_boundary_visual_layer_params(
				topology_boundary_visual_layer_params_type &params)
		{  }

		virtual
		void
		visit_topology_network_visual_layer_params(
				topology_network_visual_layer_params_type &params)
		{  }

		virtual
		void
		visit_velocity_field_calculator_visual_layer_params(
				velocity_field_calculator_visual_layer_params_type &params)
		{  }

	protected:

		VisualLayerParamsVisitorBase()
		{  }
	};


	/**
	 * This is the base class for visitors that visit const VisualLayerParams.
	 */
	typedef VisualLayerParamsVisitorBase<true> ConstVisualLayerParamsVisitor;

	/**
	 * This is the base class for visitors that visit non-const VisualLayerParams.
	 */
	typedef VisualLayerParamsVisitorBase<false> VisualLayerParamsVisitor;

}

#endif // GPLATES_PRESENTATION_VISUALLAYERPARAMSVISITOR_H
