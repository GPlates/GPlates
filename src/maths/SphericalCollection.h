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
#include "maths/PointOnSphere.h"
#include "maths/types.h"

namespace GPlatesMaths
{
	template<
	 typename Elem,
	 typename ProxMetric,
	 class LookupCont = std::list< Elem > >
	class SphericalCollection {
	
		public:
			typedef Elem       value_type;
			typedef ProxMetric proximity_metric_type;
			typedef LookupCont lookup_container_type;
	
			SphericalCollection() { }

			template< typename Iterator >
			SphericalCollection(Iterator begin, Iterator end)
			 : m_elem_container(begin, end) {  }

			~SphericalCollection() { }

			/**
			 * Insert points into @a results according to how close they 
			 * are to @a test_pos.
			 */
			virtual 
			void
			lookup(
			 lookup_container_type &results,
			 const PointOnSphere &test_pos,
			 real_t prox_thres) const {

				/*
				 * XXX No attempt is made to sort these results according to
				 * proximity to test_pos.
				 */
				container_type::const_iterator 
				 i = m_elem_container.begin(), end = m_elem_container.end();

				for ( ; i != end; ++i) {

					real_t dist = 
					 proximity_metric_type(*i, test_pos);

					if (dist < prox_thres)
						results.push_back(*i);
				}
			}

		private:
			typedef std::list< Elem > container_type;

			container_type m_elem_container;
	
	};
}

#endif  /* GP_MATHS_SPHERICALCOLLECTION_H */
