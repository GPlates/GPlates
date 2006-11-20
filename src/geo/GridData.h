/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
			 * \throws InvalidParametersException
			 *	if circles are not perpendicular
			 */
			GridData (const DataType_t&,
				  const RotationGroupId_t&,
				  const TimeWindow&,
				  const std::string &first_header_line,
				  const std::string &second_header_line,
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

			/// Return the grid lattice
			const GPlatesMaths::GridOnSphere &getLattice () const
							{ return _lattice; }

			/**
			 * Add @a elem to the grid.
			 */
			void Add_Elem (GridElement *element, index_t x1, index_t x2);

			virtual void Accept (Visitor& visitor) const
					{ visitor.Visit(*this); }

			void Draw ();
			void RotateAndDraw (const GPlatesMaths::FiniteRotation &rot);

			const GridElement *get (index_t x, index_t y) const;
			void getDimensions (index_t &x_sz, index_t &y_sz) const;

			float min () const { return min_val; }
			float max () const { return max_val; }

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
			float min_val, max_val;
	};
}

#endif  // _GPLATES_GEO_GRIDDATA_H_
