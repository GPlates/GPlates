/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 The University of Sydney, Australia
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

#ifndef _GPLATES_GUI_GLOBE_H_
#define _GPLATES_GUI_GLOBE_H_

#include "Colour.h"
#include "OpaqueSphere.h"
#include "SphericalGrid.h"
#include "Texture.h"
#include "NurbsRenderer.h"
#include "SimpleGlobeOrientation.h"
#include "maths/UnitVector3D.h"
#include "maths/PointOnSphere.h"
#include "maths/Rotation.h"

namespace GPlatesGui
{
	class Globe
	{

		public:
			Globe() :
			 _sphere(Colour(0.35f, 0.35f, 0.35f)),
			 _grid(NUM_CIRCLES_LAT, NUM_CIRCLES_LON, Colour::WHITE)
			 {  }

			~Globe() {  }

			SimpleGlobeOrientation &
			orientation()
			{
				return m_globe_orientation;
			}

			// currently does nothing.
			void
			SetTransparency(bool trans = true) {  }

			void SetNewHandlePos(const
			 GPlatesMaths::PointOnSphere &pos);

			void UpdateHandlePos(const
			 GPlatesMaths::PointOnSphere &pos);

			GPlatesMaths::PointOnSphere Orient(const
			 GPlatesMaths::PointOnSphere &pos);

			void
			initialise_texture();

			void Paint();
			
			/*
			 * A special version of the globe's Paint() method more suitable
			 * for vector output
			 */
			void paint_vector_output();

			void
			toggle_raster_image();

			void
			enable_raster_display();

			void
			disable_raster_display();

			Texture &
			texture(){
				return d_texture;
			}

		private:
			/**
			 * The NurbsRenderer used to draw large GreatCircleArcs.
			 */
			NurbsRenderer d_nurbs_renderer;

			/**
			 * The solid earth.
			 */
			OpaqueSphere _sphere;


			/**
			 * A (single) texture to be texture-mapped over the sphere surface.
			 */
			Texture d_texture;

			/**
			 * Lines of lat and lon on surface of earth.
			 */
			SphericalGrid _grid;

			/**
			 * The accumulated orientation of the globe.
			 */
			SimpleGlobeOrientation m_globe_orientation;

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
