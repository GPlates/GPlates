/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_GLOBEORIENTATION_H
#define GPLATES_GUI_GLOBEORIENTATION_H

#include "maths/types.h"
#include "maths/UnitVector3D.h"
#include "maths/PointOnSphere.h"


namespace GPlatesGui
{
	/**
	 * This class is an abstract interface for globe orientations.
	 */
	class GlobeOrientation
	{
	public:
		virtual
		~GlobeOrientation()
		{  }

		/**
		 * Return the axis of the accumulated rotation of the globe.
		 */
		virtual
		const GPlatesMaths::UnitVector3D &
		rotation_axis() const = 0;

		/**
		 * Return the angle of the accumulated rotation of the globe.
		 *
		 * As always, the rotation angle is in radians.
		 */
		virtual
		const GPlatesMaths::real_t &
		rotation_angle() const = 0;

		/**
		 * Apply the reverse of the accumulated rotation of the globe
		 * to the supplied point.
		 */
		virtual
		const GPlatesMaths::PointOnSphere
		reverse_orient_point(
				const GPlatesMaths::PointOnSphere &pos) const = 0;

		/**
		 * Set a new handle at the given position.
		 *
		 * The model which this class provides for globe-reorientation is the following:
		 * You place a "handle" on the globe at some position, then move the handle to
		 * re-orient the globe.
		 *
		 * The position of the handle conveniently coincides with the position at which the
		 * mouse-button is pressed to start a drag motion, and the subsequent motion of the
		 * handle follows the motion of the mouse pointer.
		 */
		virtual
		void
		set_new_handle_at_pos(
				const GPlatesMaths::PointOnSphere &pos) = 0;

		/**
		 * Move the already-set handle to the given position, changing
		 * the orientation of the globe in the process.
		 */
		virtual
		void
		move_handle_to_pos(
				const GPlatesMaths::PointOnSphere &pos) = 0;
	};
}

#endif  // GPLATES_GUI_GLOBEORIENTATION_H
