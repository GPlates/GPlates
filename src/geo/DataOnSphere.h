/* $Id$ */

/**
 * @file 
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
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GEO_DATAONSPHERE_H_
#define _GPLATES_GEO_DATAONSPHERE_H_

#include "GeologicalData.h"
#include "maths/FiniteRotation.h"

namespace GPlatesGeo
{
	/**
	 * Being a child of this class signifies that the
	 * child can be said to have a location on the sphere.
	 * This class is as thin as toilet paper.
	 */
	class DataOnSphere : public GeologicalData
	{
		public:
			virtual void
			Rotate(const GPlatesMaths::FiniteRotation &) const = 0;

		protected:
			DataOnSphere(const DataType_t& dt,
			             const RotationGroupId_t& rg,
			             const TimeWindow& tw,
			             const Attributes_t& attrs)
			 : GeologicalData(dt, rg, tw, attrs) {  }
	};
}

#endif  /* _GPLATES_GEO_DATAONSPHERE_H_ */
