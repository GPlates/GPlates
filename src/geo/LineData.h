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

#ifndef _GPLATES_LINEDATA_H_
#define _GPLATES_LINEDATA_H_

#include <vector>
#include "DrawableData.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"

namespace GPlatesGeo
{
	/**
	 * Data corresponding to a line on a sphere.
	 * @invariant Number of line elements is greater than or equal to 2.
	 */
	class LineData : public DrawableData
	{
		public:
			LineData(const DataType_t&, 
			         const RotationGroupId_t&,
			         const TimeWindow&,
				 const std::string &first_header_line,
				 const std::string &second_header_line,
			         const Attributes_t&, 
			         const GPlatesMaths::PolylineOnSphere&);
			
			virtual void
			Accept(Visitor& visitor) const { visitor.Visit(*this); }
			
#if 0
			/** 
			 * Enumerative access the PointData constituting this line.
			 * @warning Access to this iterator will allow the client
			 *   to put the object into an illegal state (num points < 2).
			 */
			GPlatesMaths::PolylineOnSphere::iterator
			Begin() { return _line.begin(); }

			GPlatesMaths::PolylineOnSphere::iterator
			End() { return _line.end(); }
#endif
			/**
			 * Restricted enumerative access the PointData constituting
			 * this line.
			 */
			GPlatesMaths::PolylineOnSphere::const_iterator
			Begin() const { return _line.begin(); }

			GPlatesMaths::PolylineOnSphere::const_iterator
			End() const { return _line.end(); }

			void
			Draw();

			void
			RotateAndDraw(const GPlatesMaths::FiniteRotation &);

		protected:
			virtual GPlatesMaths::real_t
			proximity(const GPlatesMaths::PointOnSphere &pos) 
			 const;
			
		private:
			GPlatesMaths::PolylineOnSphere _line;
	};
}

#endif  // _GPLATES_LINEDATA_H_
