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

#include "ReconstructedPointFeature.h"
#include "PointFeature.h"

GPlatesGeo::ReconstructedPointFeature::ReconstructedPointFeature(
 const GPlatesMaths::FiniteRotationSnapshotTable &table,
 PointFeature &point_feature) 
 : ReconstructedFeature(table),
   m_point_feature(&point_feature), 
   m_reconstructed_point(point_feature.get_point())  /* FIXME rotate m_reconstructed_point */
{

}
