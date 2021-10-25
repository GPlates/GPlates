/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include "ReconstructionGeometryVisitor.h"

// Class definitions of ReconstructedFeatureGeometry derivations are needed below for static pointer casts.
#include "ReconstructedFlowline.h"
#include "ReconstructedMotionPath.h"
#include "ReconstructedSmallCircle.h"
#include "ReconstructedVirtualGeomagneticPole.h"
#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalLine.h"
#include "TopologyReconstructedFeatureGeometry.h"


template <class ReconstructionGeometryType>
void
GPlatesAppLogic::ReconstructionGeometryVisitorBase<ReconstructionGeometryType>::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstructed_flowline_type> &rf)
{
	// Default implementation delegates to base class 'ReconstructedFeatureGeometry'.
	visit(GPlatesUtils::static_pointer_cast<reconstructed_feature_geometry_type>(rf));
}


template <class ReconstructionGeometryType>
void
GPlatesAppLogic::ReconstructionGeometryVisitorBase<ReconstructionGeometryType>::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstructed_motion_path_type> &rmp)
{
	// Default implementation delegates to base class 'ReconstructedFeatureGeometry'.
	visit(GPlatesUtils::static_pointer_cast<reconstructed_feature_geometry_type>(rmp));
}


template <class ReconstructionGeometryType>
void
GPlatesAppLogic::ReconstructionGeometryVisitorBase<ReconstructionGeometryType>::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstructed_small_circle_type> &rsc)
{
	// Default implementation delegates to base class 'ReconstructedFeatureGeometry'.
	visit(GPlatesUtils::static_pointer_cast<reconstructed_feature_geometry_type>(rsc));
}


template <class ReconstructionGeometryType>
void
GPlatesAppLogic::ReconstructionGeometryVisitorBase<ReconstructionGeometryType>::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstructed_virtual_geomagnetic_pole_type> &rvgp)
{
	// Default implementation delegates to base class 'ReconstructedFeatureGeometry'.
	visit(GPlatesUtils::static_pointer_cast<reconstructed_feature_geometry_type>(rvgp));
}


template <class ReconstructionGeometryType>
void
GPlatesAppLogic::ReconstructionGeometryVisitorBase<ReconstructionGeometryType>::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_boundary_type> &rtb)
{
	// Default implementation delegates to base class 'ResolvedTopologicalGeometry'.
	visit(GPlatesUtils::static_pointer_cast<resolved_topological_geometry_type>(rtb));
}


template <class ReconstructionGeometryType>
void
GPlatesAppLogic::ReconstructionGeometryVisitorBase<ReconstructionGeometryType>::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_line_type> &rtl)
{
	// Default implementation delegates to base class 'ResolvedTopologicalGeometry'.
	visit(GPlatesUtils::static_pointer_cast<resolved_topological_geometry_type>(rtl));
}


template <class ReconstructionGeometryType>
void
GPlatesAppLogic::ReconstructionGeometryVisitorBase<ReconstructionGeometryType>::visit(
		const GPlatesUtils::non_null_intrusive_ptr<topology_reconstructed_feature_geometry_type> &trfg)
{
	// Default implementation delegates to base class 'ReconstructedFeatureGeometry'.
	visit(GPlatesUtils::static_pointer_cast<reconstructed_feature_geometry_type>(trfg));
}


// Explicit template instantiation for the two types of visitors:
// 1) ReconstructionGeometryVisitor
// 2) ConstReconstructionGeometryVisitor
// 
template class GPlatesAppLogic::ReconstructionGeometryVisitorBase<GPlatesAppLogic::ReconstructionGeometry>;
template class GPlatesAppLogic::ReconstructionGeometryVisitorBase<const GPlatesAppLogic::ReconstructionGeometry>;
