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
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GEO_GRIDDATA_H_
#define _GPLATES_GEO_GRIDDATA_H_

#include "DrawableData.h"
#include "maths/Basis.h"
#include "maths/PointOnSphere.h"

namespace GPlatesGeo
{
	/**
	 * Data arranged in a rigid regular manner.
	 * @todo We want to be able to support adaptive meshes sometime in
	 *   the future.
	 */
	class GridData : public DrawableData
	{
		public:
			class GridElement;

			//typedef std::vector<GridElement> Grid_t;

			GridData(const DataType_t&,
				 const RotationGroupId_t&,
				 const TimeWindow&,
				 const Attributes_t&,
				 const GPlatesMaths::PointOnSphere&,
				 const GPlatesMaths::Basis&);

			/**
			 * Add @a elem to the grid.
			 */
			//virtual void
			//Add(const GridElement& elem) { _grid.push_back(elem); }

			virtual void
			Accept(Visitor& visitor) const { visitor.Visit(*this); }

			void Draw () const;
			void RotateAndDraw (const GPlatesMaths::FiniteRotation &rot) const;
		private:
			//Grid_t _grid;
			GPlatesMaths::PointOnSphere _origin;
			GPlatesMaths::Basis _basis;
	};
}

#endif  // _GPLATES_GEO_GRIDDATA_H_
