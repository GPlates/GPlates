/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_SPHERICALGRID_H
#define GPLATES_GUI_SPHERICALGRID_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "GraticuleSettings.h"

#include "opengl/GLDrawable.h"
#include "opengl/GLStateSet.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
}

namespace GPlatesMaths
{
	class UnitVector3D;
}

namespace GPlatesGui
{
	class SphericalGrid :
			private boost::noncopyable
	{
	public:

		SphericalGrid(
				const GraticuleSettings &graticule_settings);

		/**
		 * Paints lines of latitude and longitude on the surface of the sphere.
		 */
		void
		paint(
				GPlatesOpenGL::GLRenderer &renderer);

		/**
		 * Paints the circumference for vector output.
		 */
		void
		paint_circumference(
				GPlatesOpenGL::GLRenderer &renderer,
				const GPlatesMaths::UnitVector3D &axis,
				double angle_in_deg);

	private:

		const GraticuleSettings &d_graticule_settings;
		GPlatesOpenGL::GLStateSet::non_null_ptr_to_const_type d_state_set;
		
		boost::optional<GraticuleSettings> d_last_seen_graticule_settings;
		boost::optional<GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type> d_grid_drawable;
	};
}

#endif  // GPLATES_GUI_SPHERICALGRID_H
