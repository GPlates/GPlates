/* $Id$ */

/**
 * @file 
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

#ifndef _GPLATES_GEO_DRAWABLEDATA_H_
#define _GPLATES_GEO_DRAWABLEDATA_H_

#include "GeologicalData.h"
#include "maths/FiniteRotation.h"

namespace GPlatesGeo
{
	/**
	 * Being a child of this class signifies that the child
	 * is "drawable", ie. it can be displayed on the screen.
	 */
	class DrawableData : public GeologicalData
	{
		public:
			virtual void
			Draw() const = 0;

			virtual void
			RotateAndDraw(const GPlatesMaths::FiniteRotation &)
			 const = 0;

			static GPlatesMaths::real_t
			ProximityToPointOnSphere(
			 const DrawableData *data,
			 const GPlatesMaths::PointOnSphere &pos) {

				return data->proximity(pos);
			}

		protected:
			DrawableData(const DataType_t& dt,
			             const RotationGroupId_t& rg,
			             const TimeWindow& tw,
			             const Attributes_t& attrs)
			 : GeologicalData(dt, rg, tw, attrs) {  }

			virtual GPlatesMaths::real_t
			proximity(const GPlatesMaths::PointOnSphere &pos) 
			 const = 0;
	};
}

#endif  /* _GPLATES_GEO_DRAWABLEDATA_H_ */
