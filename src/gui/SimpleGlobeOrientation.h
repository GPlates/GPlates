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

#ifndef GPLATES_GUI_SINGLEGLOBEORIENTATION_H
#define GPLATES_GUI_SINGLEGLOBEORIENTATION_H

#include "maths/types.h"
#include "maths/Rotation.h"
#include "maths/UnitVector3D.h"
#include "maths/PointOnSphere.h"

namespace GPlatesGui {

	/**
	 * This class represents the simplest type of globe orientation:
	 * one which is unrelated to any other globe orientation (that is,
	 * changes to this globe orientation do not affect any other globe
	 * orientation, and vice-versa).
	 */
	class SimpleGlobeOrientation {

	 public:

		SimpleGlobeOrientation() :
		 m_handle_pos(GPlatesMaths::UnitVector3D::xBasis()),
		 m_accum_rot(GPlatesMaths::Rotation::Create(
		  GPlatesMaths::UnitVector3D::zBasis(), 0.0)),
		 m_rev_accum_rot(GPlatesMaths::Rotation::Create(
		  GPlatesMaths::UnitVector3D::zBasis(), 0.0)) {  }

		~SimpleGlobeOrientation() {  }

		/**
		 * Return the axis of the accumulated rotation of the globe.
		 */
		GPlatesMaths::UnitVector3D
		rotation_axis() const;

		/**
		 * Return the angle of the accumulated rotation of the globe.
		 *
		 * As always, the rotation angle is in radians.
		 */
		GPlatesMaths::real_t
		rotation_angle() const;

		/**
		 * Apply the reverse of the accumulated rotation of the globe
		 * to the supplied point.
		 */
		GPlatesMaths::PointOnSphere
		reverse_orient_point(
		 const GPlatesMaths::PointOnSphere &pos) const;

		/**
		 * Set a new handle at the given position.
		 */
		void
		set_new_handle_at_pos(
		 const GPlatesMaths::PointOnSphere &pos);

		/**
		 * Move the already-set handle to the given position, changing
		 * the orientation of the globe in the process.
		 */
		void
		move_handle_to_pos(
		 const GPlatesMaths::PointOnSphere &pos);

	 private:

		/**
		 * The current position of the "handle".
		 * Move this handle to change the globe orientation.
		 */
		GPlatesMaths::PointOnSphere m_handle_pos;

		/**
		 * The accumulated rotation of the globe.
		 */
		GPlatesMaths::Rotation m_accum_rot;

		/**
		 * The <b>reverse</b> of the accumulated rotation of the globe.
		 */
		GPlatesMaths::Rotation m_rev_accum_rot;

	};

}

#endif  /* GPLATES_GUI_SINGLEGLOBEORIENTATION_H */
