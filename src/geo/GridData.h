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
 *   Hamish Law <hlaw@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GEO_GRIDDATA_H_
#define _GPLATES_GEO_GRIDDATA_H_

#include "DrawableData.h"
#include "global/types.h"
#include "maths/GreatCircle.h"
#include "maths/SmallCircle.h"

namespace GPlatesGeo
{
	class GridElement;

	/**
	 * Data arranged in a rigid regular manner.
	 * @todo We want to be able to support adaptive meshes sometime in
	 *   the future.
	 */
	class GridData : public DrawableData
	{
		public:
			GridData(const DataType_t&,
				 const RotationGroupId_t&,
				 const TimeWindow&,
				 const Attributes_t&,
				 const GPlatesMaths::GreatCircle&,
				 const GPlatesMaths::SmallCircle&);
			~GridData ();

			/**
			 * Add @a elem to the grid.
			 */
			void
			Add (const GridElement *element, index_t x1, index_t x2);

			virtual void
			Accept(Visitor& visitor) const { visitor.Visit(*this); }

			void Draw () const;
			void RotateAndDraw (const GPlatesMaths::FiniteRotation &rot) const;
		private:
			GPlatesMaths::GreatCircle _major;
			GPlatesMaths::SmallCircle _minor;

			typedef const GridElement *GridElementPtr;
			struct GridRow {
				index_t offset, length;
				GridElementPtr *data;
			};
			typedef struct GridRow *GridRowPtr;

			struct Grid {
				index_t offset, length;
				GridRowPtr *rows;
			};

			Grid *grid;
	};
}

#endif  // _GPLATES_GEO_GRIDDATA_H_
