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

#ifndef GPLATES_GEO_POINTFEATURE_H
#define GPLATES_GEO_POINTFEATURE_H

#include "Feature.h"
#include "maths/PointOnSphere.h"

namespace GPlatesGeo {

	/**
	 * A Feature with point-geometry.
	 */
	class PointFeature : public Feature {
		
		public:

			template< typename properties_iterator >
			PointFeature(
			 properties_iterator begin,
			 properties_iterator end,
			 const GPlatesMaths::PointOnSphere &point)
			 : Feature(begin, end), m_point(point) {  }

			virtual
			~PointFeature() {  }

			/**
			 * XXX This method probably isn't necessary in the long
			 * run.  It is included here to allow the construction of
			 * ReconstructedPointOnSphere objects until someone writes 
			 * the reconstruction calculation code.
			 */
			GPlatesMaths::PointOnSphere
			get_point() const { 
				
				return m_point;
			}

			virtual
			ReconstructedFeature *
			reconstruct(
			 const GPlatesMaths::FiniteRotationSnapshotTable &table);

		private:

			/**
			 * The geometry of this PointFeature.
			 */
			GPlatesMaths::PointOnSphere m_point;
	};
}

#endif
