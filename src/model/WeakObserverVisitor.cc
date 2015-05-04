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

#include "WeakObserverVisitor.h"

// Class definitions of ReconstructedFeatureGeometry derivations are needed below for static casts.
#include "app-logic/DeformedFeatureGeometry.h"
#include "app-logic/ReconstructedFlowline.h"
#include "app-logic/ReconstructedMotionPath.h"
#include "app-logic/ReconstructedScalarCoverage.h"
#include "app-logic/ReconstructedSmallCircle.h"
#include "app-logic/ReconstructedVirtualGeomagneticPole.h"


namespace GPlatesModel
{
	void
	WeakObserverVisitor<FeatureHandle>::visit_deformed_feature_geometry(
			GPlatesAppLogic::DeformedFeatureGeometry &dfg)
	{
		// Default implementation delegates to base class 'ReconstructedFeatureGeometry'.
		visit_reconstructed_feature_geometry(
				static_cast<GPlatesAppLogic::ReconstructedFeatureGeometry &>(dfg));
	}


	void
	WeakObserverVisitor<FeatureHandle>::visit_reconstructed_flowline(
			GPlatesAppLogic::ReconstructedFlowline &rf)
	{
		// Default implementation delegates to base class 'ReconstructedFeatureGeometry'.
		visit_reconstructed_feature_geometry(
				static_cast<GPlatesAppLogic::ReconstructedFeatureGeometry &>(rf));
	}


	void
	WeakObserverVisitor<FeatureHandle>::visit_reconstructed_motion_path(
			GPlatesAppLogic::ReconstructedMotionPath &rmp)
	{
		// Default implementation delegates to base class 'ReconstructedFeatureGeometry'.
		visit_reconstructed_feature_geometry(
				static_cast<GPlatesAppLogic::ReconstructedFeatureGeometry &>(rmp));
	}


	void
	WeakObserverVisitor<FeatureHandle>::visit_reconstructed_small_circle(
			GPlatesAppLogic::ReconstructedSmallCircle &rsc)
	{
		// Default implementation delegates to base class 'ReconstructedFeatureGeometry'.
		visit_reconstructed_feature_geometry(
				static_cast<GPlatesAppLogic::ReconstructedFeatureGeometry &>(rsc));
	}



	void
	WeakObserverVisitor<FeatureHandle>::visit_reconstructed_virtual_geomagnetic_pole(
			GPlatesAppLogic::ReconstructedVirtualGeomagneticPole &rvgp)
	{
		// Default implementation delegates to base class 'ReconstructedFeatureGeometry'.
		visit_reconstructed_feature_geometry(
				static_cast<GPlatesAppLogic::ReconstructedFeatureGeometry &>(rvgp));
	}
}
