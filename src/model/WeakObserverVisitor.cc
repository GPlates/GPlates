/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#include "WeakObserverVisitor.h"

#include "app-logic/ReconstructedFlowline.h"
#include "app-logic/ReconstructedMotionPath.h"
#include "app-logic/ReconstructedSmallCircle.h"
#include "app-logic/ReconstructedVirtualGeomagneticPole.h"
#include "app-logic/ResolvedTopologicalBoundary.h"
#include "app-logic/ResolvedTopologicalLine.h"
#include "app-logic/TopologyReconstructedFeatureGeometry.h"


void
GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle>::visit_reconstructed_flowline(
		GPlatesAppLogic::ReconstructedFlowline &rf)
{
	// Default implementation delegates to base class 'ReconstructedFeatureGeometry'.
	visit_reconstructed_feature_geometry(rf);
}


void
GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle>::visit_reconstructed_motion_path(
		GPlatesAppLogic::ReconstructedMotionPath &rmp)
{
	// Default implementation delegates to base class 'ReconstructedFeatureGeometry'.
	visit_reconstructed_feature_geometry(rmp);
}


void
GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle>::visit_reconstructed_small_circle(
		GPlatesAppLogic::ReconstructedSmallCircle &rsc)
{
	// Default implementation delegates to base class 'ReconstructedFeatureGeometry'.
	visit_reconstructed_feature_geometry(rsc);
}


void
GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle>::visit_reconstructed_virtual_geomagnetic_pole(
		GPlatesAppLogic::ReconstructedVirtualGeomagneticPole &rvgp)
{
	// Default implementation delegates to base class 'ReconstructedFeatureGeometry'.
	visit_reconstructed_feature_geometry(rvgp);
}


void
GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle>::visit_resolved_topological_boundary(
		GPlatesAppLogic::ResolvedTopologicalBoundary &rtb)
{
	// Default implementation delegates to base class 'ResolvedTopologicalGeometry'.
	visit_resolved_topological_geometry(rtb);
}


void
GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle>::visit_resolved_topological_line(
		GPlatesAppLogic::ResolvedTopologicalLine &rtl)
{
	// Default implementation delegates to base class 'ResolvedTopologicalGeometry'.
	visit_resolved_topological_geometry(rtl);
}


void
GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle>::visit_topology_reconstructed_feature_geometry(
		GPlatesAppLogic::TopologyReconstructedFeatureGeometry &trfg)
{
	// Default implementation delegates to base class 'ReconstructedFeatureGeometry'.
	visit_reconstructed_feature_geometry(trfg);
}
