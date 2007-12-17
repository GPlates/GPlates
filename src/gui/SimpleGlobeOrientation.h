/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_SIMPLEGLOBEORIENTATION_H
#define GPLATES_GUI_SIMPLEGLOBEORIENTATION_H

#include <QObject>

#include "GlobeOrientation.h"
#include "maths/Rotation.h"


namespace GPlatesGui
{
	/**
	 * This class represents the simplest type of globe orientation:
	 * one which is unrelated to any other globe orientation (that is,
	 * changes to this globe orientation do not affect any other globe
	 * orientation, and vice-versa).
	 */
	class SimpleGlobeOrientation:
			public QObject, public GlobeOrientation
	{
		Q_OBJECT

	public:
		SimpleGlobeOrientation() :
			d_handle_pos(GPlatesMaths::UnitVector3D::xBasis()),
			d_accum_rot(GPlatesMaths::Rotation::create(
					GPlatesMaths::UnitVector3D::zBasis(), 0.0)),
			d_rev_accum_rot(GPlatesMaths::Rotation::create(
					GPlatesMaths::UnitVector3D::zBasis(), 0.0))
		{  }

		~SimpleGlobeOrientation()
		{  }

		/**
		 * Return the axis of the accumulated rotation of the globe.
		 */
		const GPlatesMaths::UnitVector3D &
		rotation_axis() const
		{
			return d_accum_rot.axis();
		}

		/**
		 * Return the angle of the accumulated rotation of the globe.
		 *
		 * As always, the rotation angle is in radians.
		 */
		const GPlatesMaths::real_t &
		rotation_angle() const
		{
			return d_accum_rot.angle();
		}

		/**
		 * Apply the reverse of the accumulated rotation of the globe
		 * to the supplied point.
		 */
		const GPlatesMaths::PointOnSphere
		reverse_orient_point(
				const GPlatesMaths::PointOnSphere &pos) const
		{
			return (d_rev_accum_rot * pos);
		}

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
		void
		set_new_handle_at_pos(
				const GPlatesMaths::PointOnSphere &pos)
		{
			d_handle_pos = pos;
		}

		/**
		 * Move the already-set handle to the given position, changing
		 * the orientation of the globe in the process.
		 */
		void
		move_handle_to_pos(
				const GPlatesMaths::PointOnSphere &pos);

	signals:
		void
		orientation_changed();

	private:

		/**
		 * The current position of the "handle".
		 * Move this handle to change the globe orientation.
		 */
		GPlatesMaths::PointOnSphere d_handle_pos;

		/**
		 * The accumulated rotation of the globe.
		 */
		GPlatesMaths::Rotation d_accum_rot;

		/**
		 * The @em reverse of the accumulated rotation of the globe.
		 */
		GPlatesMaths::Rotation d_rev_accum_rot;
	};
}

#endif  // GPLATES_GUI_SIMPLEGLOBEORIENTATION_H
