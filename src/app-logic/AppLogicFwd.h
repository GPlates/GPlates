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

#ifndef GPLATES_APP_LOGIC_APPLOGICFWD_H
#define GPLATES_APP_LOGIC_APPLOGICFWD_H

#include <boost/shared_ptr.hpp>

//
// This should be one of the few non-system header includes because we're only declaring
// forward declarations in this file.
//
#include "global/PointerTraits.h"


namespace GPlatesAppLogic
{
	/*
	 * Forward declarations.
	 *
	 * The purpose of this file is to reduce header dependencies to speed up compilation and
	 * to also avoid cyclic header dependencies.
	 *
	 * Use of 'PointerTraits' avoids clients having to include the respective headers just to
	 * get the 'non_null_ptr_type' and 'non_null_ptr_to_const_type' nested typedefs.
	 * However, there are times when clients will need to include some respective headers
	 * where the 'non_null_ptr_type', for example, is copied/destroyed since then the
	 * non-NULL intrusive pointer will require the definition of the class it's referring to for this.
	 * This can be minimised by passing const references to 'non_null_ptr_type' instead of
	 * passing by value (ie, instead of copying).
	 *
	 * Any specialisations of PointerTraits that are needed can also be placed in this file
	 * (for most classes should not need any specialisation) - see PointerTraits for more details.
	 *
	 * NOTE: Please keep the PointerTraits declarations alphabetically ordered.
	 * NOTE: Please don't use 'using namespace GPlatesGlobal' as this will pollute the current namespace.
	 */

	class CoRegistrationData;
	typedef GPlatesGlobal::PointerTraits<CoRegistrationData>::non_null_ptr_type coregistration_data_non_null_ptr_type;
	typedef GPlatesGlobal::PointerTraits<const CoRegistrationData>::non_null_ptr_type coregistration_data_non_null_ptr_to_const_type;

	class MultiPointVectorField;
	typedef GPlatesGlobal::PointerTraits<MultiPointVectorField>::non_null_ptr_type multi_point_vector_field_non_null_ptr_type;
	typedef GPlatesGlobal::PointerTraits<const MultiPointVectorField>::non_null_ptr_type multi_point_vector_field_non_null_ptr_to_const_type;

	class RasterLayerProxy;
	typedef GPlatesGlobal::PointerTraits<RasterLayerProxy>::non_null_ptr_type raster_layer_proxy_non_null_ptr_type;
	typedef GPlatesGlobal::PointerTraits<const RasterLayerProxy>::non_null_ptr_type raster_layer_proxy_non_null_ptr_to_const_type;

	class ReconstructedFeatureGeometry;
	typedef GPlatesGlobal::PointerTraits<ReconstructedFeatureGeometry>::non_null_ptr_type reconstructed_feature_geometry_non_null_ptr_type;
	typedef GPlatesGlobal::PointerTraits<const ReconstructedFeatureGeometry>::non_null_ptr_type reconstructed_feature_geometry_non_null_ptr_to_const_type;

	class ReconstructionGeometry;
	typedef GPlatesGlobal::PointerTraits<ReconstructionGeometry>::non_null_ptr_type reconstruction_geometry_non_null_ptr_type;
	typedef GPlatesGlobal::PointerTraits<const ReconstructionGeometry>::non_null_ptr_type reconstruction_geometry_non_null_ptr_to_const_type;

	class ReconstructionLayerProxy;
	typedef GPlatesGlobal::PointerTraits<ReconstructionLayerProxy>::non_null_ptr_type reconstruction_layer_proxy_non_null_ptr_type;
	typedef GPlatesGlobal::PointerTraits<const ReconstructionLayerProxy>::non_null_ptr_type reconstruction_layer_proxy_non_null_ptr_to_const_type;

	class ReconstructionTree;
	typedef GPlatesGlobal::PointerTraits<ReconstructionTree>::non_null_ptr_type reconstruction_tree_non_null_ptr_type;
	typedef GPlatesGlobal::PointerTraits<const ReconstructionTree>::non_null_ptr_type reconstruction_tree_non_null_ptr_to_const_type;

	class ReconstructLayerProxy;
	typedef GPlatesGlobal::PointerTraits<ReconstructLayerProxy>::non_null_ptr_type reconstruct_layer_proxy_non_null_ptr_type;
	typedef GPlatesGlobal::PointerTraits<const ReconstructLayerProxy>::non_null_ptr_type reconstruct_layer_proxy_non_null_ptr_to_const_type;

	class ResolvedRaster;
	typedef GPlatesGlobal::PointerTraits<ResolvedRaster>::non_null_ptr_type resolved_raster_non_null_ptr_type;
	typedef GPlatesGlobal::PointerTraits<const ResolvedRaster>::non_null_ptr_type resolved_raster_non_null_ptr_to_const_type;

	class ResolvedScalarField3D;
	typedef GPlatesGlobal::PointerTraits<ResolvedScalarField3D>::non_null_ptr_type resolved_scalar_field_3d_non_null_ptr_type;
	typedef GPlatesGlobal::PointerTraits<const ResolvedScalarField3D>::non_null_ptr_type resolved_scalar_field_3d_non_null_ptr_to_const_type;

	class ResolvedTopologicalBoundary;
	typedef GPlatesGlobal::PointerTraits<ResolvedTopologicalBoundary>::non_null_ptr_type resolved_topological_boundary_non_null_ptr_type;
	typedef GPlatesGlobal::PointerTraits<const ResolvedTopologicalBoundary>::non_null_ptr_type resolved_topological_boundary_non_null_ptr_to_const_type;

	class ResolvedTopologicalNetwork;
	typedef GPlatesGlobal::PointerTraits<ResolvedTopologicalNetwork>::non_null_ptr_type resolved_topological_network_non_null_ptr_type;
	typedef GPlatesGlobal::PointerTraits<const ResolvedTopologicalNetwork>::non_null_ptr_type resolved_topological_network_non_null_ptr_to_const_type;

	class ScalarField3DLayerProxy;
	typedef GPlatesGlobal::PointerTraits<ScalarField3DLayerProxy>::non_null_ptr_type scalar_field_3d_layer_proxy_non_null_ptr_type;
	typedef GPlatesGlobal::PointerTraits<const ScalarField3DLayerProxy>::non_null_ptr_type scalar_field_3d_layer_proxy_non_null_ptr_to_const_type;

	class VelocityFieldCalculatorLayerProxy;
	typedef GPlatesGlobal::PointerTraits<VelocityFieldCalculatorLayerProxy>::non_null_ptr_type velocity_field_calculator_layer_proxy_non_null_ptr_type;
	typedef GPlatesGlobal::PointerTraits<const VelocityFieldCalculatorLayerProxy>::non_null_ptr_type velocity_field_calculator_layer_proxy_non_null_ptr_to_const_type;

	class TopologyBoundaryResolverLayerProxy;
	typedef GPlatesGlobal::PointerTraits<TopologyBoundaryResolverLayerProxy>::non_null_ptr_type topology_boundary_resolver_layer_proxy_non_null_ptr_type;
	typedef GPlatesGlobal::PointerTraits<const TopologyBoundaryResolverLayerProxy>::non_null_ptr_type topology_boundary_resolver_layer_proxy_non_null_ptr_to_const_type;

	class TopologyNetworkResolverLayerProxy;
	typedef GPlatesGlobal::PointerTraits<TopologyNetworkResolverLayerProxy>::non_null_ptr_type topology_network_resolver_layer_proxy_non_null_ptr_type;
	typedef GPlatesGlobal::PointerTraits<const TopologyNetworkResolverLayerProxy>::non_null_ptr_type topology_network_resolver_layer_proxy_non_null_ptr_to_const_type;

	namespace TopologyUtils
	{
		class ResolvedNetworkForInterpolationQuery;
		typedef boost::shared_ptr<ResolvedNetworkForInterpolationQuery> resolved_network_for_interpolation_query_shared_ptr_type;
	}
}

#endif // GPLATES_APP_LOGIC_APPLOGICFWD_H
