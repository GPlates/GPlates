/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2022 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_MAPCAMERA_H
#define GPLATES_GUI_MAPCAMERA_H

#include <utility>  // std::pair
#include <boost/optional.hpp>
#include <QPointF>

#include "Camera.h"
#include "GlobeProjectionType.h"

#include "maths/PointOnSphere.h"
#include "maths/Rotation.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

#include "opengl/GLIntersectPrimitives.h"


namespace GPlatesGui
{
	class MapProjection;
	class ViewportZoom;

	/**
	 * Handles viewing of the map (eye location, view direction, projection).
	 */
	class MapCamera :
			public Camera
	{
	public:


		explicit
		MapCamera(
				MapProjection &map_projection,
				ViewportZoom &viewport_zoom);


		/**
		 * The position on the map (z=0 plane) that the view is looking at.
		 */
		const QPointF &
		get_look_at_position_on_map() const;

		/**
		 * Same as @a get_look_at_position_on_map but returned a position in 3D space.
		 */
		GPlatesMaths::Vector3D
		get_look_at_position() const override
		{
			const QPointF &position_on_map = get_look_at_position_on_map();
			return GPlatesMaths::Vector3D(position_on_map.x(), position_on_map.y(), 0);
		}

		/**
		 * The view direction.
		 *
		 * For perspective viewing this is from the eye position to the look-at position.
		 */
		GPlatesMaths::UnitVector3D
		get_view_direction() const override;

		/**
		 * The 'up' vector for the view orientation.
		 */
		GPlatesMaths::UnitVector3D
		get_up_direction() const override;


		/**
		 * The translation (in map plane) that the view pans in the map plane.
		 */
		const QPointF &
		get_pan() const
		{
			return d_pan;
		}

		/**
		 * Set the camera translation that the view pans in the map plane.
		 */
		void
		set_pan(
				const QPointF &pan,
				bool only_emit_if_changed = true);


		/**
		 * The angle (in radians) that the view direction rotates around the map plane normal.
		 *
		 * This is the rotation of the moving camera around the fixed map.
		 *
		 * Note that we don't actually rotate the map, instead rotating the view frame
		 * (look-at position and view/up directions) to achieve the same effect.
		 */
		GPlatesMaths::real_t
		get_rotation_angle() const
		{
			return d_rotation_angle;
		}

		/**
		 * Set the camera rotation angle (in radians).
		 *
		 * If @a only_emit_if_changed is true then only emits 'camera_changed' signal if camera changed.
		 */
		void
		set_rotation_angle(
				const GPlatesMaths::real_t &rotation_angle,
				bool only_emit_if_changed = true);


		/**
		 * The angle (in radians) that the view direction tilts.
		 *
		 * Zero angle implies looking straight down on the map plane (z=0). And PI/2 (90 degrees) means the view
		 * direction is looking tangentially at the map plane (at look-at position) and the up
		 * direction is pointing outward from the map plane (along z-axis).
		 */
		GPlatesMaths::real_t
		get_tilt_angle() const
		{
			return d_tilt_angle;
		}

		/**
		 * Set the angle (in radians) that the view direction tilts.
		 *
		 * If @a only_emit_if_changed is true then only emits 'camera_changed' signal if camera changed.
		 */
		void
		set_tilt_angle(
				const GPlatesMaths::real_t &tilt_angle,
				bool only_emit_if_changed = true);

		/**
		 * Move the current look-at position to the specified look-at position along the line segment between them.
		 *
		 * Note that this does not change the current tilt angle.
		 *
		 * If @a only_emit_if_changed is true then only emits 'camera_changed' signal if camera changed.
		 */
		void
		move_look_at_position(
				const QPointF &new_look_at_position,
				bool only_emit_if_changed = true);

		/**
		 * Rotate the view so that the "up" direction points towards the North pole when @a reorientation_angle is zero.
		 *
		 * @a reorientation_angle, in radians, is [0, PI] for anti-clockwise view orientation with respect
		 * to North pole (note map appears to rotate clockwise relative to camera), and [0,-PI] for
		 * clockwise view orientation (note map appears to rotate anti-clockwise relative to camera).
		 */
		void
		reorient_up_direction(
				const GPlatesMaths::real_t &reorientation_angle = 0,
				bool only_emit_if_changed = true);

		/**
		 * Pan the current look-at position "up" by the specified angle (in radians).
		 */
		void
		pan_up(
				const GPlatesMaths::real_t &angle,
				bool only_emit_if_changed = true);

		/**
		 * Same as @a pan_up but pans "down".
		 */
		void
		pan_down(
				const GPlatesMaths::real_t &angle,
				bool only_emit_if_changed = true)
		{
			pan_up(-angle, only_emit_if_changed);
		}

		/**
		 * Pan the current look-at position "left" by the specified angle (in radians).
		 */
		void
		pan_left(
				const GPlatesMaths::real_t &angle,
				bool only_emit_if_changed = true)
		{
			pan_right(-angle, only_emit_if_changed);
		}

		/**
		 * Same as @a pan_left but pans "right".
		 */
		void
		pan_right(
				const GPlatesMaths::real_t &angle,
				bool only_emit_if_changed = true);

		/**
		 * Rotate the view "clockwise", around the current look-at position, by the specified angle (in radians).
		 *
		 * The view and up directions are rotated.
		 *
		 * Note that this does not change the current tilt angle.
		 */
		void
		rotate_clockwise(
				const GPlatesMaths::real_t &angle,
				bool only_emit_if_changed = true)
		{
			rotate_anticlockwise(-angle, only_emit_if_changed);
		}

		/**
		 * Same as @a rotate_clockwise but rotates "anti-clockwise".
		 */
		void
		rotate_anticlockwise(
				const GPlatesMaths::real_t &angle,
				bool only_emit_if_changed = true);


		/**
		 * Returns the position on the map plane (z=0) where the specified camera ray intersects, or
		 * none if does not intersect.
		 *
		 * Note that the ray could be outside the view frustum (not associated with a visible screen pixel),
		 * in which case the position on the map plane is not visible either.
		 *
		 * The @a camera_ray can be obtained from mouse coordinates using @a get_camera_ray_at_window_coord.
		 */
		static
		boost::optional<QPointF>
		get_position_on_map_at_camera_ray(
				const GPlatesOpenGL::GLIntersect::Ray &camera_ray);

	protected:

		/**
		 * In perspective viewing mode, this is the distance from the eye position to the look-at
		 * position for the default zoom (ie, a zoom factor of 1.0).
		 */
		double
		get_distance_from_eye_to_look_at_for_perspective_viewing_at_default_zoom() const override;

	private:

		struct ViewFrame
		{
			ViewFrame(
					const QPointF &look_at_position_,
					const GPlatesMaths::UnitVector3D &view_direction_,
					const GPlatesMaths::UnitVector3D &up_direction_) :
				look_at_position(look_at_position_),
				view_direction(view_direction_),
				up_direction(up_direction_)
			{  }

			QPointF look_at_position;
			GPlatesMaths::UnitVector3D view_direction;
			GPlatesMaths::UnitVector3D up_direction;
		};

		MapProjection &d_map_projection;

		QPointF d_pan;

		/**
		 * The angle (in radians) that the view direction rotates around the map plane normal.
		 */
		GPlatesMaths::real_t d_rotation_angle;

		/**
		 * The angle that the view direction tilts.
		 *
		 * Zero angle implies looking straight down on the map and 90 degrees means the view
		 * direction is looking tangentially at the map plane (at look-at position).
		 */
		GPlatesMaths::real_t d_tilt_angle;

		/**
		 * The view frame (look-at position and view/up directions) is calculated/cached from the
		 * view orientation and view tilt.
		 */
		mutable boost::optional<ViewFrame> d_view_frame;


		QPointF
		convert_pan_from_view_to_map_frame(
				const QPointF &pan_in_view_frame) const;

		/**
		 * Update @a d_view_frame using the current view orientation and view tilt.
		 */
		void
		cache_view_frame() const;

		/**
		 * The view frame needs updating.
		 */
		void
		invalidate_view_frame() const
		{
			d_view_frame = boost::none;
		}




		/**
		 * Ratio of the map extent in the longitude direction divided by the latitude direction.
		 *
		 * This varies depending on the map projection so we'll just use the Rectangular projection as a basis.
		 */
		static constexpr double MAP_LONGITUDE_TO_LATITUDE_EXTENT_RATIO_IN_MAP_SPACE = 2.0;
		//! Extent of map projection in longitude direction (just using the Rectangular projection as a basis).
		static constexpr double MAP_LONGITUDE_EXTENT_IN_MAP_SPACE = 360.0;
		//! Extent of map projection in latitude direction (just using the Rectangular projection as a basis).
		static constexpr double MAP_LATITUDE_EXTENT_IN_MAP_SPACE =
				MAP_LONGITUDE_EXTENT_IN_MAP_SPACE / MAP_LONGITUDE_TO_LATITUDE_EXTENT_RATIO_IN_MAP_SPACE;

		/**
		 * At the initial zoom, and untilted view, this creates a little space between the map boundary and the viewport.
		 *
		 * When the viewport is resized, the map will be scaled accordingly.
		 * The value of this constant is purely cosmetic.
		 */
		static const double FRAMING_RATIO_OF_MAP_IN_VIEWPORT;

		/**
		 * The initial position on the map that the camera looks at.
		 */
		static const QPointF INITIAL_LOOK_AT_POSITION;

		/**
		 * Initial view direction.
		 */
		static const GPlatesMaths::UnitVector3D INITIAL_VIEW_DIRECTION;

		/**
		 * Initial up direction (orthogonal to view direction).
		 */
		static const GPlatesMaths::UnitVector3D INITIAL_UP_DIRECTION;
	};
}

#endif // GPLATES_GUI_MAPCAMERA_H
