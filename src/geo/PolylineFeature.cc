/**
 * @file 
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
 *  (under the name "PolyLineFeature.cc")
 * Copyright (C) 2006 The University of Sydney, Australia
 *  (under the name "PolylineFeature.cc")
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

#include "PolylineFeature.h"
#include "ReconstructedPolylineFeature.h"

GPlatesGeo::ReconstructedFeature *
GPlatesGeo::PolylineFeature::reconstruct(
 const GPlatesMaths::FiniteRotationSnapshotTable &table) {

	// ...
	return new ReconstructedPolylineFeature(table, *this);
}

