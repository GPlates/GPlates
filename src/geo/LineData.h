/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
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
 * Authors:
 *   Hamish Law <hlaw@es.usyd.edu.au>
 */

#ifndef _GPLATES_LINEDATA_H_
#define _GPLATES_LINEDATA_H_

#include <vector>
#include "GeologicalData.h"
#include "PointData.h"

namespace GPlatesGeo
{
	/**
	 * Data corresponding to a line on a sphere.
	 * @invariant Number of line elements is greater than or equal to 2.
	 */
	class LineData : public GeologicalData
	{
		public:
			/**
			 * Convenience typedef.
			 * A Line_t is just a sequence of PointData.
			 */
			typedef std::vector<PointData> Line_t;
			
			LineData(const DataType_t&, const RotationGroupId_t&,
				const Attributes_t&, const PointData& P, const PointData& Q);

			/** 
			 * Add a point to the end of the line.
			 */
			virtual void
			Add(const PointData& P) { _line.push_back(P); }
			
			virtual void
			Accept(Visitor& visitor) const { visitor.Visit(*this); }
			
			/** 
			 * Enumerative access the PointData constituting this line.
			 * @warning Access to this iterator will allow the client
			 *   to put the object into an illegal state (num points < 2).
			 */
			Line_t::iterator
			GetIterator() { return _line.begin(); }
			
			/**
			 * Restricted enumerative access the PointData constituting
			 * this line.
			 */
			Line_t::const_iterator
			GetIterator() const { return _line.begin(); }

		private:
			Line_t _line;
	};
}

#endif  // _GPLATES_LINEDATA_H_
