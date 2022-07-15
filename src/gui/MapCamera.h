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
#include "MapProjection.h"

#include "maths/PointOnSphere.h"
#include "maths/Rotation.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

#include "opengl/GLIntersectPrimitives.h"


namespace GPlatesGui
{
	class ViewportZoom;

	/**
	 * Handles viewing of the map (eye location, view direction, projection).
	 */
	class MapCamera :
			public Camera
	{
	public:

		MapCamera(
				MapProjection &map_projection,
				ViewportProjection::Type viewport_projection,
				ViewportZoom &viewport_zoom);


		/**
		 * Same as @a get_look_at_position_on_map but returned as a position in 3D space.
		 */
		GPlatesMaths::Vector3D
		get_look_at_position() const override;

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
		 * The position on the map (z=0 plane) that the view is looking at.
		 */
		const QPointF &
		get_look_at_position_on_map() const;

		/**
		 * Move the current look-at position to the specified look-at position along the line segment between them.
		 *
		 * If the specified look-at position is outside the map projection boundary then it is changed to be
		 * on the boundary (just inside it actually within a tolerance) where the line segment
		 * (between old and new look-at positions) intersects the map projection boundary.
		 *
		 * Note that this does not change the current rotation or tilt angle.
		 *
		 * If @a only_emit_if_changed is true then only emits 'camera_changed' signal if camera changed.
		 */
		void
		move_look_at_position_on_map(
				QPointF look_at_position_on_map,
				bool only_emit_if_changed = true);


		/**
		 * The position on the globe (unit sphere) that the view is looking at.
		 *
		 * This is the equivalent to the map-projection inverse of @a get_look_at_position_on_map
		 * (ie, the actual look-at position on the z=0 map plane inverse projected back onto the globe).
		 */
		GPlatesMaths::PointOnSphere
		get_look_at_position_on_globe() const override;

		/**
		 * Move the current look-at position to the specified look-at position on the globe.
		 *
		 * This pans the view along the line segment (on the map plane) between the
		 * current and new look-at positions. The view and up directions are not changed.
		 *
		 * Note that this does not change the current tilt angle.
		 *
		 * If @a only_emit_if_changed is true then only emits 'camera_changed' signal if camera changed.
		 */
		void
		move_look_at_position_on_globe(
				const GPlatesMaths::PointOnSphere &look_at_position_on_globe,
				bool only_emit_if_changed = true) override;


		/**
		 * The camera view orientation (excluding tilt).
		 *
		 * This is obtained indirectly by combining the look-at position and rotation angle.
		 */
		GPlatesMaths::Rotation
		get_view_orientation() const override;

		/**
		 * Set the camera view orientation (excluding tilt).
		 *
		 * This is set indirectly in the camera by first extracting a look-at position and rotation angle.
		 *
		 * Note that this does not change the current tilt angle.
		 *
		 * If @a only_emit_if_changed is true then only emits 'camera_changed' signal if camera changed.
		 */
		void
		set_view_orientation(
				const GPlatesMaths::Rotation &view_orientation,
				bool only_emit_if_changed = true) override;


		/**
		 * The angle (in radians), around the look-at position, of the un-tilted camera "up" direction
		 * relative to the direction of the North pole.
		 *
		 * A positive rotation angle indicates a rotation *anti-clockwise* relative to North.
		 *
		 * Note: North is always along the global y-axis (from South to North).
		 *       It is currently unaffected by the map projection's local North
		 *       (at any local point along the map projection's curved line of longitude).
		 */
		GPlatesMaths::real_t
		get_rotation_angle() const override
		{
			return d_rotation_angle;
		}

		/**
		 * Rotate the view so that the camera un-tilted "up" direction has an angle, around the look-at position,
		 * of @a rotation_angle radians relative to the direction of the North pole.
		 *
		 * A positive rotation angle indicates a rotation *anti-clockwise* relative to North.
		 *
		 * Note: North is always along the global y-axis (from South to North).
		 *       It is currently unaffected by the map projection's local North
		 *       (at any local point along the map projection's curved line of longitude).
		 *
		 * Note: An *anti-clockwise* rotation of camera (relative to North) causes the map to
		 *       rotate *clockwise* relative to the camera (and vice versa).
		 *
		 * Note: This is a rotation of the moving camera around the fixed map.
		 *       We don't actually rotate the map, instead rotating the view frame
		 *       (look-at position and view/up directions) to achieve the same effect.
		 *
		 * If @a only_emit_if_changed is true then only emits 'camera_changed' signal if camera changed.
		 */
		void
		set_rotation_angle(
				GPlatesMaths::real_t rotation_angle,
				bool only_emit_if_changed = true) override;


		/**
		 * The angle (in radians) that the view direction tilts.
		 *
		 * The tilt angle is clamped to the range [0, PI/2].
		 *
		 * Zero angle implies looking straight down on the map plane (z=0). And PI/2 (90 degrees) means the view
		 * direction is looking tangentially at the map plane (at look-at position) and the up
		 * direction is pointing outward from the map plane (along z-axis).
		 */
		GPlatesMaths::real_t
		get_tilt_angle() const override
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
				GPlatesMaths::real_t tilt_angle,
				bool only_emit_if_changed = true) override;


		/**
		 * Pan the current look-at position "up" by the specified angle (in radians).
		 *
		 * If @a scale_by_viewport_zoom is true (defaults to true) then scales the angle by the current viewport zoom
		 * (divide angle by the viewport zoom factor so that there's less panning for zoomed-in views).
		 *
		 * As with @a move_look_at_position_on_map, if the newly panned look-at position is outside the
		 * map projection boundary then it is only panned to the boundary.
		 */
		void
		pan_up(
				GPlatesMaths::real_t angle = DEFAULT_PAN_ROTATE_TILT_RADIANS,
				bool scale_by_viewport_zoom = true,
				bool only_emit_if_changed = true) override;

		/**
		 * Pan the current look-at position "right" by the specified angle (in radians).
		 *
		 * If @a scale_by_viewport_zoom is true (defaults to true) then scales the angle by the current viewport zoom
		 * (divide angle by the viewport zoom factor so that there's less panning for zoomed-in views).
		 *
		 * As with @a move_look_at_position_on_map, if the newly panned look-at position is outside the
		 * map projection boundary then it is only panned to the boundary.
		 */
		void
		pan_right(
				GPlatesMaths::real_t angle = DEFAULT_PAN_ROTATE_TILT_RADIANS,
				bool scale_by_viewport_zoom = true,
				bool only_emit_if_changed = true) override;


		/**
		 * Rotate the view "anticlockwise", around the current look-at position, by the specified angle (in radians).
		 *
		 * The view and up directions are rotated.
		 *
		 * Note that this does not change the current tilt angle.
		 */
		void
		rotate_anticlockwise(
				GPlatesMaths::real_t angle = DEFAULT_PAN_ROTATE_TILT_RADIANS,
				bool only_emit_if_changed = true) override;


		/**
		 * Tilt the view "more" (view is more tilted) by the specified angle (in radians).
		 *
		 * The view and up directions are tilted.
		 *
		 * Note that this does not change the current rotation angle.
		 */
		void
		tilt_more(
				GPlatesMaths::real_t angle = DEFAULT_PAN_ROTATE_TILT_RADIANS,
				bool only_emit_if_changed = true) override;


		/**
		 * Returns the position on the map plane (z=0) at the specified window coordinate.
		 *
		 * Window coordinates are typically in the range [0, window_width] and [0, window_height]
		 * where (0, 0) is bottom-left and (window_width, window_height) is top-right of window.
		 * Note that we use the OpenGL convention where 'window_x = 0' is the bottom of the window.
		 * But in Qt it means top, so a Qt mouse y coordinate (for example) needs be inverted
		 * before passing to this method.
		 *
		 * Note that either/both window coordinate could be outside the range[0, window_width] and
		 * [0, window_height], in which case the position on the map plane is not visible either.
		 *
		 * Note: This is equivalent to calling @a get_camera_ray_at_window_coord to get a ray
		 *       followed by calling @a get_position_on_map_plane_at_camera_ray with that ray.
		 */
		boost::optional<QPointF>
		get_position_on_map_plane_at_window_coord(
				double window_x,
				double window_y,
				int window_width,
				int window_height) const
		{
			// See if camera ray intersects the map plane (passing through z=0).
			return get_position_on_map_plane_at_camera_ray(
					// Project screen coordinates into a ray into 3D scene (containing 2D map plane)...
					get_camera_ray_at_window_coord(window_x, window_y, window_width, window_height));
		}


		/**
		 * Returns the position on the map plane (z=0) where the specified camera ray intersects, or
		 * none if does not intersect.
		 *
		 * Note that the ray could be outside the view frustum (not associated with a visible screen pixel),
		 * in which case the position on the map plane is not visible either.
		 *
		 * The @a camera_ray can be obtained from mouse coordinates using @a get_camera_ray_at_window_coord.
		 *
		 * Note: For *orthographic* viewing the negative or positive side of the ray can intersect the plane
		 *       (since the view rays are parallel and so if we ignore the near/far clip planes then
		 *       everything in the infinitely long rectangular prism is visible).
		 *       Whereas for *perspective* viewing only the positive side of the ray can intersect the plane
		 *       (since the view rays emanate/diverge from a single eye location and so, ignoring the
		 *       near/far clip planes, only the front infinitely long pyramid with apex at eye is visible).
		 */
		boost::optional<QPointF>
		get_position_on_map_plane_at_camera_ray(
				const GPlatesOpenGL::GLIntersect::Ray &camera_ray) const;


		/**
		 * Returns position on map boundary (actually near boundary but just inside) in the direction of
		 * the specified 2D ray from the specified 2d ray origin (which must be inside the map boundary).
		 *
		 * Uses bisection iteration to get position close to the intersection of 2D ray and map boundary.
		 *
		 * Returns none if the specified 2D ray direction is zero length.
		 *
		 * Note: This method is typically called when a 3D camera ray misses the 2D map plane (z=0).
		 *       In which case a 2D ray is typically then created, with its 2D ray origin being a 2D point
		 *       *inside* the map boundary (such as the map origin or the camera look-at position on the map)
		 *       and its 2D ray direction being the 3D camera ray direction projected onto the map plane.
		 *
		 * NOTE: The specified 2d ray origin must be INSIDE the map boundary (to avoid an exception being thrown).
		 */
		boost::optional<QPointF>
		get_position_on_map_boundary_intersected_by_2d_camera_ray(
				const QPointF &ray_direction,
				const QPointF &ray_origin = QPointF(0, 0)) const;

	protected:

		/**
		 * Returns the aspect ratio that is optimally suited to the map view.
		 *
		 * This is the ratio of the map extent in the longitude direction divided by the latitude direction.
		 * This is typically 2.0 since in the default orientation in Rectangular projection the width is twice the height.
		 *
		 * This actually varies depending on the map projection but we'll just use the Rectangular projection as a basis.
		 *
		 * Note that this applies to both orthographic and perspective views.
		 */
		double
		get_optimal_aspect_ratio() const override
		{
			return 2.0;
		}

		/**
		 * In orthographic viewing mode, this is *half* the distance between top and bottom clip planes of
		 * the orthographic view frustum (rectangular prism) at default zoom (ie, a zoom factor of 1.0).
		 */
		double
		get_orthographic_half_height_extent_at_default_zoom() const override
		{
			// The map height (in the Rectangular projection at default orientation) is 180.0 and the half height is 90.
			// So this is slightly larger than that (due to framing ratio).
			// This means the Rectangular map projection at default orientation will fit just inside the
			// top and bottom clip planes if the aspect ratio is greater than the optimal aspect ratio.
			return FRAMING_RATIO_OF_MAP_IN_VIEWPORT * (MAP_LATITUDE_EXTENT_IN_MAP_SPACE / 2.0);
		}

		/**
		 * In perspective viewing mode, this is the distance from the eye position to the look-at
		 * position for the default zoom (ie, a zoom factor of 1.0).
		 */
		double
		get_perspective_viewing_distance_from_eye_to_look_at_for_at_default_zoom() const override;

		/**
		 * Return the radius of the sphere that bounds the map.
		 *
		 * This includes a reasonable amount of extra space around the map to include objects
		 * off the map such as velocity arrows.
		 */
		double
		get_bounding_radius() const override;

	private:

		struct ViewFrame
		{
			ViewFrame(
					const GPlatesMaths::UnitVector3D &view_direction_,
					const GPlatesMaths::UnitVector3D &up_direction_) :
				view_direction(view_direction_),
				up_direction(up_direction_)
			{  }

			GPlatesMaths::UnitVector3D view_direction;
			GPlatesMaths::UnitVector3D up_direction;
		};

		/**
		 * Helper class for a value type that depends on the map projection (detects changed projection settings).
		 */
		template <typename ValueType>
		class MapProjectionCache
		{
		public:

			MapProjectionCache() :
				d_value()
			{  }

			explicit
			MapProjectionCache(
					const ValueType &value) :
				d_value(value)
			{  }

			/**
			 * Returns true if map projection settings match internally stored settings.
			 *
			 * Will return false until @a set is first called.
			 */
			bool
			is_valid(
					const MapProjectionSettings &map_projection_settings) const
			{
				return d_map_projection_settings == map_projection_settings;
			}

			void
			set(
					const ValueType &value,
					const MapProjectionSettings &map_projection_settings)
			{
				d_value = value;
				d_map_projection_settings = map_projection_settings;
			}

			const ValueType &
			get() const
			{
				return d_value;
			}

			ValueType &
			get()
			{
				return d_value;
			}

		private:
			ValueType d_value;
			boost::optional<MapProjectionSettings> d_map_projection_settings;
		};


		MapProjection &d_map_projection;

		/**
		 * Radius of the sphere that bounds the map (including extra space for objects off the map).
		 *
		 * This is updated when the map projection changes.
		 */
		mutable MapProjectionCache<double> d_map_bounding_radius;

		/**
		 * The look-at position on the map plane (contained inside map projection of the globe).
		 *
		 * This is updated when the map projection changes.
		 */
		mutable MapProjectionCache<QPointF> d_look_at_position_on_map;

		/**
		 * The look-at position on the globe.
		 */
		GPlatesMaths::PointOnSphere d_look_at_position_on_globe;

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
		mutable boost::optional<ViewFrame> d_cached_view_frame;


		QPointF
		convert_position_on_globe_to_map(
				const GPlatesMaths::PointOnSphere &position_on_globe) const;

		boost::optional<GPlatesMaths::PointOnSphere>
		convert_position_on_map_to_globe(
				const QPointF &position_on_map) const;

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
			d_cached_view_frame = boost::none;
		}


		/**
		 * At the initial zoom, and untilted view, this creates a little space between the map boundary and the viewport.
		 *
		 * When the viewport is resized, the map will be scaled accordingly.
		 * The value of this constant is purely cosmetic.
		 */
		static const double FRAMING_RATIO_OF_MAP_IN_VIEWPORT;

		/**
		 * Extent of map projection in latitude direction (just using the Rectangular projection as a basis).
		 *
		 * Note: This actually matches what we'd get if we used class @a MapProjection with a Rectangular projection
		 *       and queried the difference in forward map projected 'y' coordinate from North to South pole.
		 */
		static constexpr double MAP_LATITUDE_EXTENT_IN_MAP_SPACE = 180.0;

		/**
		 * The initial position on the globe that the camera looks at.
		 */
		static const GPlatesMaths::PointOnSphere INITIAL_LOOK_AT_POSITION_ON_GLOBE;

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
