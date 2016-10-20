/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_LAYERPARAMSVISITOR_H
#define GPLATES_APP_LOGIC_LAYERPARAMSVISITOR_H

#include "utils/SetConst.h"


namespace GPlatesAppLogic
{
	// Forward declarations of supported LayerParams derivations.
	class CoRegistrationLayerParams;
	class RasterLayerParams;
	class ReconstructLayerParams;
	class ReconstructScalarCoverageLayerParams;
	class ScalarField3DLayerParams;
	class TopologyNetworkLayerParams;
	class VelocityFieldCalculatorLayerParams;

	/**
	 * This class is a base class for visitors that visit LayerParams.
	 * For convenience, typedefs are provided below to cover the const and non-const cases.
	 */
	template<bool Const>
	class LayerParamsVisitorBase
	{
	public:

		// Typedefs to give the supported derivations the appropriate const-ness.
		typedef typename GPlatesUtils::SetConst<CoRegistrationLayerParams, Const>::type co_registration_layer_params_type;
		typedef typename GPlatesUtils::SetConst<RasterLayerParams, Const>::type raster_layer_params_type;
		typedef typename GPlatesUtils::SetConst<ReconstructLayerParams, Const>::type reconstruct_layer_params_type;
		typedef typename GPlatesUtils::SetConst<ReconstructScalarCoverageLayerParams, Const>::type reconstruct_scalar_coverage_layer_params_type;
		typedef typename GPlatesUtils::SetConst<ScalarField3DLayerParams, Const>::type scalar_field_3d_layer_params_type;
		typedef typename GPlatesUtils::SetConst<TopologyNetworkLayerParams, Const>::type topology_network_layer_params_type;
		typedef typename GPlatesUtils::SetConst<VelocityFieldCalculatorLayerParams, Const>::type velocity_field_calculator_layer_params_type;

		virtual
		~LayerParamsVisitorBase()
		{  }

		virtual
		void
		visit_co_registration_layer_params(
				co_registration_layer_params_type &params)
		{  }

		virtual
		void
		visit_raster_layer_params(
				raster_layer_params_type &params)
		{  }

		virtual
		void
		visit_reconstruct_layer_params(
				reconstruct_layer_params_type &params)
		{  }

		virtual
		void
		visit_reconstruct_scalar_coverage_layer_params(
				reconstruct_scalar_coverage_layer_params_type &params)
		{  }

		virtual
		void
		visit_scalar_field_3d_layer_params(
				scalar_field_3d_layer_params_type &params)
		{  }

		virtual
		void
		visit_topology_network_layer_params(
				topology_network_layer_params_type &params)
		{  }

		virtual
		void
		visit_velocity_field_calculator_layer_params(
				velocity_field_calculator_layer_params_type &params)
		{  }

	protected:

		LayerParamsVisitorBase()
		{  }
	};


	/**
	 * This is the base class for visitors that visit const LayerParams.
	 */
	typedef LayerParamsVisitorBase<true> ConstLayerParamsVisitor;

	/**
	 * This is the base class for visitors that visit non-const LayerParams.
	 */
	typedef LayerParamsVisitorBase<false> LayerParamsVisitor;

}

#endif // GPLATES_APP_LOGIC_LAYERPARAMSVISITOR_H
