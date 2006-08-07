/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _GPLATES_MATHS_POLYLINEONSPHERE_H_
#define _GPLATES_MATHS_POLYLINEONSPHERE_H_

#include <list>
#include "GreatCircleArc.h"

namespace GPlatesMaths
{
	using namespace GPlatesGlobal;

	/** 
	 * Represents a poly-line on the surface of a sphere. 
	 *
	 * Internally, this is stored as a sequence of great circle arcs.
	 */
	class PolyLineOnSphere
	{
		public:
			typedef std::list< GreatCircleArc > seq_type;
			typedef seq_type::const_iterator const_iterator;


			PolyLineOnSphere() {  }


			const_iterator
			begin() const { return _seq.begin(); }


			const_iterator
			end() const { return _seq.end(); }


			bool
			is_close_to(
			 const PointOnSphere &test_point,
			 const real_t &closeness_inclusion_threshold,
			 const real_t &latitude_exclusion_threshold,
			 real_t &closeness) const;


			void
			push_back(const GreatCircleArc &g) {

				_seq.push_back(g);
			}

		private:
			seq_type _seq;
	};
}

#endif  // _GPLATES_MATHS_POLYLINEONSPHERE_H_
