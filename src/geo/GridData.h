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
	 * We define a grid by specifying a \ref GPlatesMaths::GreatCircle (GC)
	 * and a \ref GPlatesMaths::SmallCircle (SC). The GC is to be
	 * coincident with one longitudinal border of the grid, whilst the SC
	 * is to be coincident with the latitudinal border of the grid that is
	 * closer to the equator. In the case of grids that straddle the
	 * equator either latitudinal border can be chosen.
	 *
	 * Instead of actually passing a GC and SC to the GridData constructor,
	 * we pass three \ref GPlatesMaths::PointOnSphere objects that define
	 * the origin of the grid, the first grid point along the GC, and the
	 * first grid point along the SC.
	 *
	 * The <strong>primary corner</strong> of the grid is defined to be the
	 * anterior intersection of the GC and the SC. If the normal vectors of
	 * the GC and SC are \f$ \hat{n_G} \f$ and \f$ \hat{n_S} \f$
	 * respectively, then the primary corner is given by
	 * \f$ \hat{P} = \hat{n_G} \times \hat{n_S} \f$ (note the order!).
	 * We assume that the grids are always <em>regular</em>, so the GC and
	 * SC normals will be perpendicular, and so \f$ \hat{P} \f$ will indeed
	 * be a unit vector (see \ref GPlatesMaths::UnitVector3D), and thus a
	 * \ref GPlatesMaths::PointOnSphere.
	 *
	 * Because of the construction method, it is not possible to have the
	 * primary corner at either pole (latitude \f$ = \pm 90^\circ \f$).
	 * This is implicitly enforced by the \ref GPlatesMaths::SmallCircle
	 * constructor, since a meridian GC can only intersect with a polar
	 * normal SC at a pole if the SC is degenerate (i.e. zero radius).
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
				  const GPlatesMaths::PointOnSphere &gc_step,
				  const GPlatesMaths::PointOnSphere &sc_step);
			~GridData ();

			/**
			 * Add @a elem to the grid.
			 */
			void Add (const GridElement *element, index_t x1, index_t x2);

			virtual void Accept (Visitor& visitor) const
					{ visitor.Visit(*this); }

			void Draw () const;
			void RotateAndDraw (const GPlatesMaths::FiniteRotation &rot) const;
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
