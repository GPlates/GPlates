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

#ifndef _GPLATES_GEO_GRIDDATA_H_
#define _GPLATES_GEO_GRIDDATA_H_

#include "GeologicalData.h"
#include "maths/Vector2D.h"
#include "maths/types.h"  /* Basis2D_t */

namespace GPlatesGeo
{
	/**
	 * Data arranged in a rigid regular manner.
	 * @todo We want to be able to support adaptive meshes sometime in
	 *   the future.
	 */
	class GridData : public GeologicalData
	{
		public:
			class GridElement;

			typedef std::vector<GridElement> Grid_t;

			GridData(const DataType_t&, const RotationGroupId_t&,
				const Attributes_t&, const Grid_t&);

			/**
			 * Add @a elem to the grid.
			 */
			virtual void
			Add(const GridElement& elem) { _grid.push_back(elem); }

			virtual void
			Accept(Visitor& visitor) const { visitor.Visit(*this); }

		private:
			Grid_t _grid;
			GPlatesMaths::Vector2D _origin;
			GPlatesMaths::Basis2D_t _basis;
	};
}

#endif  // _GPLATES_GEO_GRIDDATA_H_
