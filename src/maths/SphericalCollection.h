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

#ifndef GP_MATHS_SPHERICALCOLLECTION_H
#define GP_MATHS_SPHERICALCOLLECTION_H

#include <list>
#include <algorithm>
#include "maths/PointOnSphere.h"
#include "maths/types.h"

namespace GPlatesMaths
{
	/**
	 * Represents a collection of elements on the globe.
	 *
	 * Intantiate with something like:
	 * @code
	 * typedef GPlatesMaths::SphericalCollection< 
	 *  const GPlatesGeo::DrawableData *,
	 *  &GPlatesGeo::DrawableData::ProximityToPointOnSphere > SC;
	 * @endcode
	 */
	template<
	 typename Elem,
	 GPlatesMaths::real_t (*ProxMetric)(Elem, const GPlatesMaths::PointOnSphere &),
	 typename LookupCont = std::list< Elem > >
	class SphericalCollection {
	
		public:
			typedef Elem       value_type;
			typedef LookupCont lookup_container_type;
	
			SphericalCollection() { }

			template< typename Iterator >
			SphericalCollection(Iterator begin, Iterator end)
			 : m_elem_container(begin, end) {  }

			virtual
			~SphericalCollection() { }

			/**
			 * Insert points into @a results according to how close they 
			 * are to @a test_pos.
			 *
			 * @pre @a results is sorted according to 
			 *   @a proximity_metric_type.
			 * @post @a results is sorted according to
			 *   @a proximity_metric_type, and includes any points in
			 *   this collection that fall within @a prox_thres of
			 *   @a test_pos.
			 */
			virtual 
			void
			lookup(
			 lookup_container_type &results,
			 const PointOnSphere &test_pos,
			 real_t prox_thres) const {

				typename container_type::const_iterator 
				 i   = m_elem_container.begin(), 
				 end = m_elem_container.end();

				for ( ; i != end; ++i) {

					real_t dist = ProxMetric(*i, test_pos);

					if (dist < prox_thres)
						insert(*i, dist, test_pos, results);
				}
			}

		private:
			typedef std::list< Elem > container_type;

			container_type m_elem_container;

			/**
			 * Place @a elem, which lies @a dist units from @a test_pos,
			 * into @a results such that the sorted criterion of @a results
			 * is preserved.
			 */
			void
			insert(
			 Elem elem, 
			 const real_t &dist,
			 const PointOnSphere &test_pos,
			 lookup_container_type &results) const {
				
				typename lookup_container_type::iterator
				 i   = results.begin(),
				 end = results.end();

				for ( ; i != end; ++i)
					if (dist < ProxMetric(*i, test_pos))
						break;
					
				results.insert(i, elem);
			}
	};
}

#endif  /* GP_MATHS_SPHERICALCOLLECTION_H */
