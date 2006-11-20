/**
 * @file 
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef GPLATES_GEO_RECONSTRUCTEDPOINTFEATURE_H
#define GPLATES_GEO_RECONSTRUCTEDPOINTFEATURE_H

#include "ReconstructedFeature.h"
#include "PointFeature.h"
#include "maths/PointOnSphere.h"

namespace GPlatesGeo {

	class ReconstructedPointFeature : public ReconstructedFeature {

		public:

			ReconstructedPointFeature(
			 const GPlatesMaths::FiniteRotationSnapshotTable &table,
			 PointFeature &point_feature);

			virtual
			~ReconstructedPointFeature() {  }

		private:

			/**
			 * The PointFeature for which this is the reconstruction.
			 */
			PointFeature *m_point_feature;

			/**
			 * The reconstruction of the geometry of m_point_feature
			 * according to m_finite_rotation_table.
			 */
			GPlatesMaths::PointOnSphere m_reconstructed_point;

	};
}

#endif
