/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 The University of Sydney, Australia
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
		 * Return the accumulated rotation of the globe.
		 */
		const GPlatesMaths::Rotation &
		rotation() const
		{
			return d_accum_rot;
		}

		/**
		 * Apply the accumulated rotation of the globe to the supplied geometry.
		 *
		 * This operation is used by the ReconstructionPoleWidget.
		 */
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		orient_geometry(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geom) const
		{
			return (d_accum_rot * geom);
		}

		/**
		 * Apply the accumulated rotation of the globe to the supplied
		 * point.
		 */
		const GPlatesMaths::PointOnSphere
		orient_point(
				const GPlatesMaths::PointOnSphere &pos) const
		{
			return (d_accum_rot * pos);
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

	public slots:
	
		/**
		 * For keyboard camera controls to use: nudge the camera 'up' by a few degrees.
		 */
		void
		move_camera_up();

		/**
		 * For keyboard camera controls to use: nudge the camera 'down' by a few degrees.
		 */
		void
		move_camera_down();

		/**
		 * For keyboard camera controls to use: nudge the camera 'left' by a few degrees.
		 */
		void
		move_camera_left();

		/**
		 * For keyboard camera controls to use: nudge the camera 'right' by a few degrees.
		 */
		void
		move_camera_right();

		/**
		 * For keyboard camera controls to use: rotate the camera clockwise by a few degrees.
		 */
		void
		rotate_camera_clockwise();

		/**
		 * For keyboard camera controls to use: rotate the camera anticlockwise by a few degrees.
		 */
		void
		rotate_camera_anticlockwise();
		
		/**
		 * Rotate the camera such that the poles are oriented vertically (with North at the top
		 * of the screen). The camera should remain centred on its current (lat,lon) coordinate.
		 */
		void
		orient_poles_vertically();

	signals:
		void
		orientation_changed();

	private:
	
		/**
		 * How far to nudge or rotate the camera when using the move_camera_*
		 * functions, in degrees.
		 */
		static const double s_nudge_camera_amount;

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

		// Apparently QObject isn't copy-constructable.  So, let's make this class
		// non-copy-constructable too.
		SimpleGlobeOrientation(
				const SimpleGlobeOrientation &);

		// Apparently QObject isn't copy-assignable.  So, let's make this class
		// non-copy-assignable too.
		SimpleGlobeOrientation &
		operator=(
				const SimpleGlobeOrientation &);
	};
}

#endif  // GPLATES_GUI_SIMPLEGLOBEORIENTATION_H
