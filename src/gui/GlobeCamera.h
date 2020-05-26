/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_GLOBECAMERA_H
#define GPLATES_GUI_GLOBECAMERA_H

#include <QObject>

#include "GlobeProjectionType.h"

#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"


namespace GPlatesGui
{
	class ViewportZoom;

	class GlobeCamera :
			public QObject
	{
		Q_OBJECT

	public:
		/**
		 * At the initial zoom, the smaller dimension of the viewport will be @a FRAMING_RATIO_OF_GLOBE_IN_VIEWPORT
		 * times the diameter of the globe. This creates a little space between the globe circumference and the viewport.
		 * When the viewport is resized, the globe will be scaled accordingly.
		 *
		 * The value of this constant is purely cosmetic.
		 */
		static const double FRAMING_RATIO_OF_GLOBE_IN_VIEWPORT;


		explicit
		GlobeCamera(
				ViewportZoom &viewport_zoom);

		void
		set_projection_type(
				GlobeProjection::Type projection_type);

		GlobeProjection::Type
		get_projection_type() const
		{
			return d_projection_type;
		}

		/**
		 * The view direction.
		 *
		 * For perspective viewing this is from the eye position to the look-at position.
		 */
		const GPlatesMaths::UnitVector3D &
		get_view_direction() const
		{
			return d_view_direction;
		}

		/**
		 * The position on the globe that the view is looking at.
		 */
		const GPlatesMaths::UnitVector3D &
		get_look_at_position() const
		{
			return d_look_at_position;
		}

		/**
		 * The 'up' vector for the view orientation.
		 */
		const GPlatesMaths::UnitVector3D &
		get_up_direction() const
		{
			return d_up_direction;
		}

		/**
		 * The camera (eye) location for perspective viewing.
		 *
		 * NOTE: For orthographic viewing there is no real eye location since the view rays
		 *       are parallel and hence the eye location is at infinity.
		 */
		GPlatesMaths::Vector3D
		get_perspective_eye_position() const;

		/**
		 * Returns the left/right/bottom/top parameters of the 'glOrtho()' function, given the
		 * specified viewport dimensions.
		 *
		 * The current viewport zoom affects these parameters.
		 */
		void
		get_orthographic_left_right_bottom_top(
				unsigned int viewport_width,
				unsigned int viewport_height,
				double &ortho_left,
				double &ortho_right,
				double &ortho_bottom,
				double &ortho_top) const;

		/**
		 * Returns the aspect ratio and field-of-view (in y-axis) parameters of the 'gluPerspective()' function,
		 * given the specified viewport dimensions.
		 */
		void
		get_perspective_aspect_ratio_and_fovy(
				unsigned int viewport_width,
				unsigned int viewport_height,
				double &aspect_ratio,
				double &fovy_degrees) const;

	Q_SIGNALS:
	
		/**
		 * This signal should only be emitted if the camera is actually different to what it was.
		 */
		void
		camera_changed();

	private Q_SLOTS:

		void
		handle_zoom_changed();

	private:
		ViewportZoom &d_viewport_zoom;

		GlobeProjection::Type d_projection_type;

		GPlatesMaths::UnitVector3D d_view_direction;
		GPlatesMaths::UnitVector3D d_look_at_position;
		GPlatesMaths::UnitVector3D d_up_direction;

		/**
		 * In perspective viewing mode, this is the distance from the eye position to the look-at
		 * position for the default zoom (ie, a zoom factor of 1.0).
		 */
		double d_distance_eye_to_look_at_for_perspective_viewing_at_default_zoom;


		static const double PERSPECTIVE_FIELD_OF_VIEW_DEGREES;

		static
		double
		calc_distance_eye_to_look_at_for_perspective_viewing_at_default_zoom();
	};
}

#endif // GPLATES_GUI_GLOBECAMERA_H
