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
 *   James Boyden <jboyden@es.usyd.edu.au>
 */

#ifndef _GPLATES_GEO_POINTDATA_H_
#define _GPLATES_GEO_POINTDATA_H_

#include "GeologicalData.h"
#include "maths/PointOnSphere.h"

namespace GPlatesGeo
{
	/** 
	 * A PointOnSphere augmented with GeneralisedData.
	 */
	class PointData : public GeologicalData
	{
		public:
			PointData(const DataType_t &,
			          const RotationGroupId_t &,
			          const Attributes_t &,
			          const GPlatesMaths::PointOnSphere &);

			virtual void
			Accept(Visitor& visitor) const { visitor.Visit(*this); }

			GPlatesMaths::PointOnSphere
			GetPointOnSphere() const { return _point; }

		private:
			/** 
			 * The location of the data.
			 */
			GPlatesMaths::PointOnSphere _point;
	};
}

#endif  // _GPLATES_GEO_POINTDATA_H_
