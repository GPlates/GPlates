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

#ifndef GPLATES_GEO_POLYLINEFEATURE_H
#define GPLATES_GEO_POLYLINEFEATURE_H

#include "Feature.h"
#include "maths/PolyLineOnSphere.h"

namespace GPlatesGeo {

	/**
	 * A Feature with polyline geometry.
	 */
	class PolyLineFeature : public Feature {
		
		public:

			template< typename properties_iterator >
			PolyLineFeature(
			 properties_iterator begin,
			 properties_iterator end,
			 const GPlatesMaths::PolyLineOnSphere &polyline)
			 : Feature(begin, end), m_polyline(polyline) {  }

			virtual
			~PolyLineFeature();

			/**
			 * XXX This method probably isn't necessary in the long
			 * run.  It is included here to allow the construction of
			 * ReconstructedPolyLineOnSphere objects until someone writes 
			 * the reconstruction calculation code.
			 */
			GPlatesMaths::PolyLineOnSphere
			get_polyline() const { 
				
				return m_polyline;
			}

			virtual
			ReconstructedFeature *
			reconstruct(
			 const GPlatesMaths::FiniteRotationSnapshotTable &table);

		private:

			/**
			 * The geometry of this PolyLineFeature.
			 */
			GPlatesMaths::PolyLineOnSphere m_polyline;
	};
}

#endif
