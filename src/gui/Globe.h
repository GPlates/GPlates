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

#ifndef _GPLATES_GUI_GLOBE_H_
#define _GPLATES_GUI_GLOBE_H_

#include "Colour.h"
#include "OpaqueSphere.h"
#include "SphericalGrid.h"
#include "maths/UnitVector3D.h"
#include "maths/PointOnSphere.h"
#include "maths/Rotation.h"
#include "geo/DataGroup.h"

namespace GPlatesGui
{
	class Globe
	{
		public:
			Globe() :
			 _sphere(Colour(0.35, 0.35, 0.35)),
			 _grid(NUM_CIRCLES_LAT, NUM_CIRCLES_LON, Colour::WHITE),
			 _handle_pos(GPlatesMaths::UnitVector3D::xBasis()),
			 _cumul_rot(GPlatesMaths::Rotation::Create(
			             GPlatesMaths::UnitVector3D::zBasis(),
			             0.0)),
			 _rev_cumul_rot(GPlatesMaths::Rotation::Create(
			                GPlatesMaths::UnitVector3D::zBasis(),
			                0.0)) {  }

			~Globe() {  }

			// currently does nothing.
			void
			SetTransparency(bool trans = true) {  }

			void SetNewHandlePos(const
			 GPlatesMaths::PointOnSphere &pos);

			void UpdateHandlePos(const
			 GPlatesMaths::PointOnSphere &pos);

			GPlatesMaths::PointOnSphere Orient(const
			 GPlatesMaths::PointOnSphere &pos);

			void Paint();

		private:
			/**
			 * The solid earth.
			 */
			OpaqueSphere _sphere;

			/**
			 * Lines of lat and lon on surface of earth.
			 */
			SphericalGrid _grid;

			/**
			 * The current position of the "handle".
			 * Move this handle to spin the globe.
			 */
			GPlatesMaths::PointOnSphere _handle_pos;

			/**
			 * The accumulated rotation of the globe.
			 */
			GPlatesMaths::Rotation _cumul_rot;

			/**
			 * The REVERSE of the accumulated rotation of the globe.
			 * [Used by the frequently-used @a Orient function.]
			 */
			GPlatesMaths::Rotation _rev_cumul_rot;

			/**
			 * One circle of latitude every 30 degrees.
			 */
			static const unsigned NUM_CIRCLES_LAT = 5;

			/**
			 * One circle of longitude every 30 degrees.
			 */
			static const unsigned NUM_CIRCLES_LON = 6;
	};
}

#endif  /* _GPLATES_GUI_GLOBE_H_ */
