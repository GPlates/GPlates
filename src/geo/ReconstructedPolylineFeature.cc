/**
 * @file 
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "ReconstructedPolylineFeature.h"
#include "PolylineFeature.h"

GPlatesGeo::ReconstructedPolylineFeature::ReconstructedPolylineFeature(
 const GPlatesMaths::FiniteRotationSnapshotTable &table,
 PolylineFeature &polyline_feature) 
 : ReconstructedFeature(table),
   m_polyline_feature(&polyline_feature), 
   m_reconstructed_polyline(polyline_feature.get_polyline())  /* FIXME rotate m_reconstructed_polyline */
{

}
