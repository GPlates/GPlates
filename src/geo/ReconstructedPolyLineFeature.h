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

#ifndef GPLATES_GEO_RECONSTRUCTEDPOLYLINEFEATURE_H
#define GPLATES_GEO_RECONSTRUCTEDPOLYLINEFEATURE_H

#include "ReconstructedFeature.h"
#include "PolyLineFeature.h"
#include "maths/PolyLineOnSphere.h"

namespace GPlatesGeo {

	class ReconstructedPolyLineFeature : public ReconstructedFeature {

		public:

			ReconstructedPolyLineFeature(
			 const GPlatesMaths::FiniteRotationSnapshotTable &table,
			 PolyLineFeature &polyline_feature);

			virtual
			~ReconstructedPolyLineFeature() {  }

		private:

			/**
			 * The PolyLineFeature for which this is the reconstruction.
			 */
			PolyLineFeature *m_polyline_feature;

			/**
			 * The reconstruction of the geometry of m_point_feature
			 * according to m_finite_rotation_table.
			 */
			GPlatesMaths::PolyLineOnSphere m_reconstructed_polyline;

	};
}

#endif
