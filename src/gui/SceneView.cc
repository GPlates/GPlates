/**
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

#include <cmath>

#include "SceneView.h"

#include "GlobeCamera.h"
#include "MapCamera.h"
#include "MapProjection.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/MathsUtils.h"

#include "presentation/ViewState.h"


GPlatesGui::SceneView::SceneView(
		GPlatesPresentation::ViewState &view_state) :
	d_projection(view_state.get_projection()),
	d_globe_camera(view_state.get_globe_camera()),
	d_map_camera(view_state.get_map_camera()),
	d_map_projection(view_state.get_map_projection()),
	d_viewport_zoom(view_state.get_viewport_zoom())
{
	// Handle changes in the projection. This includes globe and map projections as well as
	// view projections (switching between orthographic and perspective).
	QObject::connect(
			&d_projection,
			SIGNAL(globe_map_projection_changed(
					const GPlatesGui::Projection::globe_map_projection_type &,
					const GPlatesGui::Projection::globe_map_projection_type &)),
			this,
			SLOT(handle_globe_map_projection_changed(
					const GPlatesGui::Projection::globe_map_projection_type &,
					const GPlatesGui::Projection::globe_map_projection_type &)));
	// Now handle changes to just the viewport projection.
	QObject::connect(
			&d_projection,
			SIGNAL(viewport_projection_changed(
					GPlatesGui::Projection::viewport_projection_type,
					GPlatesGui::Projection::viewport_projection_type)),
			this,
			SLOT(handle_viewport_projection_changed(
					GPlatesGui::Projection::viewport_projection_type,
					GPlatesGui::Projection::viewport_projection_type)));

	// Update our view whenever the globe and map cameras change.
	//
	// Note that the cameras are updated when the zoom changes.
	//
	// Globe camera.
	QObject::connect(
			&d_globe_camera, SIGNAL(camera_changed()),
			this, SLOT(handle_camera_change()));
	// Map camera.
	QObject::connect(
			&d_map_camera, SIGNAL(camera_changed()),
			this, SLOT(handle_camera_change()));
}


const GPlatesGui::Camera &
GPlatesGui::SceneView::get_active_camera() const
{
	return is_globe_active()
			? static_cast<GPlatesGui::Camera &>(d_globe_camera)
			: static_cast<GPlatesGui::Camera &>(d_map_camera);
}


GPlatesGui::Camera &
GPlatesGui::SceneView::get_active_camera()
{
	return is_globe_active()
			? static_cast<GPlatesGui::Camera &>(d_globe_camera)
			: static_cast<GPlatesGui::Camera &>(d_map_camera);
}


bool
GPlatesGui::SceneView::is_globe_active() const
{
	return d_projection.get_globe_map_projection().is_viewing_globe_projection();
}


GPlatesOpenGL::GLViewProjection
GPlatesGui::SceneView::get_view_projection(
		const GPlatesOpenGL::GLViewport &viewport) const
{
	// Note that the projection is 'orthographic' or 'perspective', and hence is only affected by the viewport
	// *aspect ratio*, so it is independent of whether we're using device pixels or device *independent* pixels.
	const double viewport_aspect_ratio = double(viewport.width()) / viewport.height();

	// Get the view-projection transform.
	const GPlatesOpenGL::GLMatrix view_transform = get_active_camera().get_view_transform();
	const GPlatesOpenGL::GLMatrix projection_transform = get_active_camera().get_projection_transform(viewport_aspect_ratio);

	return GPlatesOpenGL::GLViewProjection(viewport, view_transform, projection_transform);
}


double
GPlatesGui::SceneView::current_proximity_inclusion_threshold(
		const GPlatesMaths::PointOnSphere &click_point,
		const GPlatesOpenGL::GLViewport &viewport) const
{
	// Say we pick an epsilon radius of 3 pixels around the click position.
	// The larger this radius, the more relaxed the proximity inclusion threshold.
	//
	// FIXME:  Do we want this constant to instead be a variable set by a per-user preference,
	// to enable users to specify their own epsilon radius?  (For example, users with shaky
	// hands or very high-resolution displays might prefer a larger epsilon radius.)
	//
	// Note: We're specifying device *independent* pixels here.
	//       On high-DPI displays there are more device pixels in the same physical area on screen but we're
	//       more interested in physical area (which is better represented by device *independent* pixels).
	const double device_independent_pixel_inclusion_threshold = 3.0;

	//
	// Limit the maximum angular distance on unit sphere.
	//
	// Globe view: When the click point is at the circumference of the visible globe, a one viewport pixel variation
	//             can result in a large traversal on the globe since the globe surface is tangential to the view there.
	//
	// Map view: When the map view is tilted the click point can intersect the map plane (z=0) at an acute angle such
	//           that one viewport pixel can cover a large area on the map.
	//           Additionally the map projection itself (eg, Rectangular, Mollweide, etc) can further stretch the
	//           viewport pixel (already projected onto map plane z=0) when it's inverse transformed back onto the globe.
	//
	// As such, a small mouse-pointer displacement on-screen can result in significantly different mouse-pointer
	// displacements on the surface of the globe depending on the location of the click point.
	//
	// To take this into account we use the current view and projection transforms (and viewport) to project
	// one screen pixel area onto the globe and find the maximum deviation of this area projected onto the globe
	// (in terms of angular distance on the globe).
	//

	const double max_distance_inclusion_threshold = GPlatesMaths::convert_deg_to_rad(5.0);

	const GPlatesOpenGL::GLViewProjection view_projection = get_view_projection(viewport);

	// If we're viewing the map (instead of globe) then we also need the map projection.
	//
	// This is because, for the map view, we need to project one screen pixel area onto the map plane (z=0) and
	// then inverse transform from the map plane onto the globe (using the map projection, eg, Rectangular, Mollweide, etc).
	// This finds the maximum deviation of this area projected onto the globe (in terms of angular distance on the globe).
	boost::optional<const GPlatesGui::MapProjection &> map_projection;
	if (is_map_active())
	{
		map_projection = d_map_projection;
	}

	// Calculate the maximum distance on the unit-sphere subtended by one viewport pixel projected onto it.
	boost::optional< std::pair<double/*min*/, double/*max*/> > min_max_device_independent_pixel_size =
			view_projection.get_min_max_pixel_size_on_globe(click_point, map_projection);
	// If unable to determine maximum pixel size then just return the maximum allowed proximity threshold.
	if (!min_max_device_independent_pixel_size)
	{
		return std::cos(max_distance_inclusion_threshold);  // Proximity threshold is expected to be a cosine.
	}

	// Multiply the inclusive distance on unit-sphere (associated with one viewport pixel) by the
	// number of inclusive viewport pixels.
	double distance_inclusion_threshold = device_independent_pixel_inclusion_threshold *
			min_max_device_independent_pixel_size->second/*max*/;

	// Clamp to range to the maximum distance inclusion threshold (if necessary).
	if (distance_inclusion_threshold > max_distance_inclusion_threshold)
	{
		distance_inclusion_threshold = max_distance_inclusion_threshold;
	}

	// Proximity threshold is expected to be a cosine.
	return std::cos(distance_inclusion_threshold);
}


GPlatesMaths::PointOnSphere
GPlatesGui::SceneView::get_position_on_globe_at_window_coord(
		double window_x,
		double window_y,
		int window_width,
		int window_height,
		bool &is_on_globe,
		boost::optional<QPointF> &position_on_map_plane) const
{
	// Project screen coordinates into a ray into 3D scene.
	const GPlatesOpenGL::GLIntersect::Ray camera_ray =
			get_active_camera().get_camera_ray_at_window_coord(window_x, window_y, window_width, window_height);

	// Determine where/if the mouse camera ray intersects globe.
	//
	// When the map is active (ie, when globe is inactive) the camera ray is considered to intersect
	// the globe if it intersects the map plane at a position that is inside the map projection boundary.
	if (is_globe_active())
	{
		// Position on map plane (z=0) is not used when the globe is active (ie, when map is inactive).
		position_on_map_plane = boost::none;

		// Update mouse position on globe when globe is active (ie, when map is inactive).
		return get_position_on_globe(camera_ray, is_on_globe);
	}
	else
	{
		return get_position_on_map(camera_ray, is_on_globe, position_on_map_plane);
	}
}


GPlatesMaths::PointOnSphere
GPlatesGui::SceneView::get_position_on_globe(
		const GPlatesOpenGL::GLIntersect::Ray &camera_ray,
		bool &is_on_globe) const
{
	// See if camera ray intersects the globe.
	boost::optional<GPlatesMaths::PointOnSphere> position_on_globe =
			d_globe_camera.get_position_on_globe_at_camera_ray(camera_ray);

	if (position_on_globe)
	{
		// Camera ray, at screen coordinates, intersects the globe.
		is_on_globe = true;
	}
	else
	{
		// Camera ray, at screen coordinates, does NOT intersect the globe.
		is_on_globe = false;

		// Instead get the nearest point on the globe horizon (visible circumference) to the camera ray.
		position_on_globe = d_globe_camera.get_nearest_globe_horizon_position_at_camera_ray(camera_ray);
	}

	return position_on_globe.get();
}


GPlatesMaths::PointOnSphere
GPlatesGui::SceneView::get_position_on_map(
		const GPlatesOpenGL::GLIntersect::Ray &camera_ray,
		bool &is_on_globe,
		boost::optional<QPointF> &position_on_map_plane) const
{
	// See if camera ray at screen coordinates intersects the 2D map plane (z=0).
	//
	// In perspective view it's possible for a screen pixel ray emanating from the camera eye to
	// miss the map plane entirely (even though the map plane is infinite).
	//
	// Given the camera ray, calculate a position on the map *plane* (2D plane with z=0), or
	// none if screen view ray (at screen position) does not intersect the map plane.
	position_on_map_plane = d_map_camera.get_position_on_map_plane_at_camera_ray(camera_ray);

	// Get the position on the globe.
	boost::optional<GPlatesMaths::LatLonPoint> lat_lon_position_on_globe;
	if (position_on_map_plane)
	{
		// Mouse position is on map plane, so see if it's also inside the map projection boundary.
		lat_lon_position_on_globe = d_map_projection.inverse_transform(position_on_map_plane.get());
		if (lat_lon_position_on_globe)
		{
			// Mouse position is inside the map projection boundary (so it is also on the globe).
			is_on_globe = true;
		}
		else
		{
			// Mouse position is NOT inside the map projection boundary (so it is not on the globe).
			is_on_globe = false;

			// Camera ray at screen pixel intersects the map plane but not *within* the map projection boundary.
			//
			// So get the intersection of line segment (from origin to intersection on map plane) with map projection boundary.
			// We'll use that to get a new position on the globe (it can be inverse map projected onto the globe).
			const QPointF map_boundary_point = d_map_projection.get_map_boundary_position(
					QPointF(0, 0),  // map origin
					position_on_map_plane.get());
			lat_lon_position_on_globe = d_map_projection.inverse_transform(map_boundary_point);

			// The map boundary position is guaranteed to be invertible (onto the globe) in the map projection.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					lat_lon_position_on_globe,
					GPLATES_ASSERTION_SOURCE);
		}
	}
	else
	{
		// Mouse position is NOT on the map plane (so it is not on the globe).
		is_on_globe = false;

		// Camera ray at screen pixel does not intersect the map plane.
		//
		// So get the intersection of 2D ray, from map origin in direction of camera ray (projected onto 2D map plane),
		// with map projection boundary.
		const QPointF ray_direction(
				camera_ray.get_direction().x().dval(),
				camera_ray.get_direction().y().dval());
		const QPointF ray_origin(0, 0);  // map origin

		const boost::optional<QPointF> map_boundary_point =
				d_map_camera.get_position_on_map_boundary_intersected_by_2d_camera_ray(ray_direction, ray_origin);
		if (map_boundary_point)
		{
			lat_lon_position_on_globe = d_map_projection.inverse_transform(map_boundary_point.get());

			// The map boundary position is guaranteed to be invertible (onto the globe) in the map projection.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					lat_lon_position_on_globe,
					GPLATES_ASSERTION_SOURCE);
		}
		else
		{
			// The 3D camera ray direction points straight down (ie, camera ray x and y are zero).
			//
			// We shouldn't really get here for a valid camera ray since we already know it did not intersect the
			// 2D map plane and so if it points straight down then it would have intersected the map plane (z=0).
			// However it's possible that at 90 degree tilt the camera eye (in perspective viewing) dips just below
			// the map plane (z=0) due to numerical tolerance and hence just misses the map plane.
			// But even then the camera view direction would be horizontal and with a field-of-view of 90 degrees or less
			// there wouldn't be any screen pixel in the view frustum that could look straight down.
			// So it really should never happen.
			//
			// Arbitrarily choose the North pole (again, we shouldn't get here).
			lat_lon_position_on_globe = GPlatesMaths::LatLonPoint(90, 0);
		}
	}

	// Convert inverse-map-projected lat-lon position to new position on the globe.
	return make_point_on_sphere(lat_lon_position_on_globe.get());
}


void
GPlatesGui::SceneView::handle_camera_change()
{
	// The active camera has been modified and this affects the view-projection transform of the view.
	Q_EMIT view_changed();
}


void
GPlatesGui::SceneView::handle_globe_map_projection_changed(
		const GPlatesGui::Projection::globe_map_projection_type &old_globe_map_projection,
		const GPlatesGui::Projection::globe_map_projection_type &globe_map_projection)
{
	//
	// We could be switching from the globe camera to map camera (or vice versa).
	//
	// If so, then get the camera view orientation, tilt and viewport projection of the old camera
	// (before the projection change) and set them on the new camera (after the projection change).
	//
	// The view orientation is the combined camera look-at position and the orientation rotation around that look-at position.
	//
	// Note: Switching between globe and map cameras (transferring view orientation, tilt and viewport projection)
	//       doesn't necessarily cause the switched-to camera to emit a 'camera_changed' signal.
	//       This is because the view orientation, tilt and viewport projection might not have changed.
	//       This can happen if the user is simply switching back and forth between the globe and map views.
	//       So we'll detect if the 'camera_changed' signal was NOT emitted and essentially handle it ourself
	//       (by directly calling our 'handle_camera_changed' slot).
	//

	// If switching from map to globe projection...
	if (old_globe_map_projection.is_viewing_map_projection() &&
		globe_map_projection.is_viewing_globe_projection())
	{
		// Get *map* camera view orientation, tilt and viewport projection.
		const GPlatesMaths::Rotation map_camera_view_orientation = d_map_camera.get_view_orientation();
		const GPlatesMaths::real_t map_camera_tilt_angle = d_map_camera.get_tilt_angle();
		const GPlatesGui::Projection::viewport_projection_type map_viewport_projection = d_map_camera.get_viewport_projection();

		bool emitted_camera_change_signal = false;

		// Set the *globe* camera view orientation, tilt and viewport projection.
		// Also detect if the 'camera_change' signal was emitted.
		if (map_camera_view_orientation.quat() != d_globe_camera.get_view_orientation().quat())
		{
			d_globe_camera.set_view_orientation(map_camera_view_orientation);
			emitted_camera_change_signal = true;
		}
		if (map_camera_tilt_angle != d_globe_camera.get_tilt_angle())
		{
			d_globe_camera.set_tilt_angle(map_camera_tilt_angle);
			emitted_camera_change_signal = true;
		}
		if (map_viewport_projection != d_globe_camera.get_viewport_projection())
		{
			d_globe_camera.set_viewport_projection(map_viewport_projection);
			emitted_camera_change_signal = true;
		}

		if (!emitted_camera_change_signal)
		{
			// The globe camera didn't actually change (since the last time it was active).
			// But we've switched from the map camera. That's a camera change, so we need to handle it.
			handle_camera_change();
		}
	}
	// else if switching from globe to map projection...
	else if (old_globe_map_projection.is_viewing_globe_projection() &&
			globe_map_projection.is_viewing_map_projection())
	{
		// Get *globe* camera view orientation, tilt and viewport projection.
		const GPlatesMaths::Rotation globe_camera_view_orientation = d_globe_camera.get_view_orientation();
		const GPlatesMaths::real_t globe_camera_tilt_angle = d_globe_camera.get_tilt_angle();
		const GPlatesGui::Projection::viewport_projection_type globe_viewport_projection = d_globe_camera.get_viewport_projection();

		bool emitted_camera_change_signal = false;

		// Set the *map* camera view orientation, tilt and viewport projection.
		// Also detect if the 'camera_change' signal was emitted.
		if (globe_camera_view_orientation.quat() != d_map_camera.get_view_orientation().quat())
		{
			d_map_camera.set_view_orientation(globe_camera_view_orientation);
			emitted_camera_change_signal = true;
		}
		if (globe_camera_tilt_angle != d_map_camera.get_tilt_angle())
		{
			d_map_camera.set_tilt_angle(globe_camera_tilt_angle);
			emitted_camera_change_signal = true;
		}
		if (globe_viewport_projection != d_map_camera.get_viewport_projection())
		{
			d_map_camera.set_viewport_projection(globe_viewport_projection);
			emitted_camera_change_signal = true;
		}

		// Update the map projection.
		//
		// It shouldn't have changed since the last time the map camera was active, but just in case.
		//
		// Note: This doesn't emit a 'camera_changed' signal.
		d_map_projection.set_projection_type(
				globe_map_projection.get_map_projection_type());
		d_map_projection.set_central_meridian(
				globe_map_projection.get_map_central_meridian());

		if (!emitted_camera_change_signal)
		{
			// The map camera didn't actually change (since the last time it was active).
			// But we've switched from the globe camera. That's a camera change, so we need to handle it.
			handle_camera_change();
		}
	}
	else // switching between two map projections and/or changing central meridian in one map projection...
	{
		// Update the map projection.
		d_map_projection.set_projection_type(
				globe_map_projection.get_map_projection_type());
		d_map_projection.set_central_meridian(
				globe_map_projection.get_map_central_meridian());

		// Something changed in the map projection (otherwise we wouldn't be here).
		// So we need to handle that.
		handle_camera_change();
	}
}


void
GPlatesGui::SceneView::handle_viewport_projection_changed(
		GPlatesGui::Projection::viewport_projection_type old_viewport_projection,
		GPlatesGui::Projection::viewport_projection_type viewport_projection)
{
	// Change the viewport projection (orthographic or perspective) of the active camera.
	//
	// Note: This will cause the active camera to emit the 'camera_changed' signal which
	//       will call our 'handle_camera_change' slot.
	get_active_camera().set_viewport_projection(viewport_projection);
}
