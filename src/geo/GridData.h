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
#include "maths/GridOnSphere.h"
#include "maths/PointOnSphere.h"

namespace GPlatesGeo
{
	class GridElement;

	/**
	 * Data arranged in a rigid regular manner.
	 *
	 * @todo We want to be able to support adaptive meshes sometime in
	 *   the future.
	 * @see GPlatesMaths::GreatCircle, GPlatesMaths::SmallCircle,
	 * 	GPlatesMaths::UnitVector3D, GPlatesMaths::PointOnSphere
	 */
	class GridData : public DrawableData
	{
		public:
			/**
			 * Constructor.
			 * \throws IllegalParametersException
			 *	if circles are not perpendicular
			 */
			GridData (const DataType_t&,
				  const RotationGroupId_t&,
				  const TimeWindow&,
				  const Attributes_t&,
				  const GPlatesMaths::PointOnSphere &origin,
				  const GPlatesMaths::PointOnSphere &sc_step,
				  const GPlatesMaths::PointOnSphere &gc_step);
			~GridData ();

			/**
			 * Return the GPlatesMaths::PointOnSphere corresponding
			 * to the grid location @a x and @a y.
			 */
			GPlatesMaths::PointOnSphere resolve (index_t x, index_t y) const
					{ return _lattice.resolve (x, y); }

			/**
			 * Add @a elem to the grid.
			 */
			void Add (GridElement *element, index_t x1, index_t x2);

			virtual void Accept (Visitor& visitor) const
					{ visitor.Visit(*this); }

			void Draw () const;
			void RotateAndDraw (const GPlatesMaths::FiniteRotation &rot) const;

			const GridElement *get (index_t x, index_t y) const;
			void getDimensions (index_t &x_sz, index_t &y_sz) const;

		private:
			GPlatesMaths::GridOnSphere _lattice;

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
