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

#ifndef GPLATES_GUI_GLOBEORIENTATION_H
#define GPLATES_GUI_GLOBEORIENTATION_H

#include "maths/types.h"
#include "maths/UnitVector3D.h"
#include "maths/PointOnSphere.h"

namespace GPlatesGui {

	/**
	 * This class is an abstract interface for globe orientations.
	 */
	class GlobeOrientation {

	 public:

		virtual
		~GlobeOrientation() {  }

		/**
		 * Return the axis of the accumulated rotation of the globe.
		 */
		virtual
		GPlatesMaths::UnitVector3D
		rotation_axis() const = 0;

		/**
		 * Return the angle of the accumulated rotation of the globe.
		 *
		 * As always, the rotation angle is in radians.
		 */
		virtual
		GPlatesMaths::real_t
		rotation_angle() const = 0;

		/**
		 * Apply the reverse of the accumulated rotation of the globe
		 * to the supplied point.
		 */
		virtual
		GPlatesMaths::PointOnSphere
		reverse_orient_point(
		 const GPlatesMaths::PointOnSphere &pos) const = 0;

		/**
		 * Set a new handle at the given position.
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

#endif  /* GPLATES_GUI_GLOBEORIENTATION_H */
