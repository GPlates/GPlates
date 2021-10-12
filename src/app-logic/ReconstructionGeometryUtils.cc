/* $Id$ */

/**
 * \file Convenience functions for @a ReconstructionGeometry.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "ReconstructionGeometryUtils.h"

#include "model/ReconstructedFeatureGeometry.h"

bool
GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstructed_feature_geometry(
		GPlatesModel::ReconstructionGeometry *reconstruction_geom,
		GPlatesModel::ReconstructedFeatureGeometry **reconstructed_feature_geom)
{
	// We use a dynamic cast here (despite the fact that dynamic casts
	// are generally considered bad form) because we only care about
	// one specific derivation.  There's no "if ... else if ..." chain,
	// so I think it's not super-bad form.  (The "if ... else if ..."
	// chain would imply that we should be using polymorphism --
	// specifically, the double-dispatch of the Visitor pattern --
	// rather than updating the "if ... else if ..." chain each time a
	// new derivation is added.)
	*reconstructed_feature_geom = dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(
			reconstruction_geom);

	return *reconstructed_feature_geom != NULL;
}
