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

#ifndef _GPLATES_GEO_SPHERICALCOLLECTION_H_
#define _GPLATES_GEO_SPHERICALCOLLECTION_H_

#include <list>
#include "maths/PointOnSphere.h"
#include "maths/types.h"
#include "state/Data.h"

namespace GPlatesGeo
{
	template<
	 typename ProxMetric,
	 class LookupCont = std::list< GPlatesGeo::DrawableData * > >
	class SphericalCollection {
	
		public:
	
			typedef ProxMetric proximity_metric_type;
			typedef LookupCont lookup_container_type;
	
			void
			lookup(
			 lookup_container_type &results,
			 const GPlatesMaths::PointOnSphere &test_pos,
			 GPlatesMaths::real_t prox_thres) const {

				using namespace GPlatesState;

				Data::DrawableMap_type *data = Data::GetDrawableData();
				Data::DrawableMap_type::const_iterator 
				 i = data->begin(), end_i = data->end();

				for ( ; i != end_i; ++i) {

					Data::DrawableDataSet::const_iterator 
					 j = i->second->begin(), end_j = i->second->end();

					for ( ; j != end_j; ++j) {

						GPlatesMaths::real_t dist = 
						 proximity_metrix_type(*j, test_pos);

						if (dist < prox_thres)
							results.push_back(*j);
					}
				}
			}
	
	};
}

#endif  /* _GPLATES_GEO_SPHERICALCOLLECTION_H_ */
