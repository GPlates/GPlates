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

#include "maths/MathsUtils.h"

#include "presentation/ViewState.h"


GPlatesGui::SceneView::SceneView(
		GPlatesPresentation::ViewState &view_state) :
	d_projection(view_state.get_projection()),
	d_globe_camera(view_state.get_globe_camera()),
	d_map_camera(view_state.get_map_camera()),
	d_map_projection(view_state.get_map_projection())
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
		const GPlatesOpenGL::GLViewport &viewport,
		boost::optional<const GPlatesOpenGL::GLMatrix &> tile_projection_transform_opt) const
{
	// Note that the projection is 'orthographic' or 'perspective', and hence is only affected by the viewport
	// *aspect ratio*, so it is independent of whether we're using device pixels or device *independent* pixels.
	const double viewport_aspect_ratio = double(viewport.width()) / viewport.height();

	// Get the view-projection transform.
	GPlatesOpenGL::GLMatrix view_transform = get_active_camera().get_view_transform();
	GPlatesOpenGL::GLMatrix projection_transform = get_active_camera().get_projection_transform(viewport_aspect_ratio);

	if (tile_projection_transform_opt)
	{
		// The projection transform associated with the tile is post-multiplied with the projection transform of the scene view.
		//
		// Note: The view transform is unaffected by the tile (only the projection transform is affected).
		GPlatesOpenGL::GLMatrix tile_projection_transform = tile_projection_transform_opt.get();
		tile_projection_transform.gl_mult_matrix(projection_transform);

		projection_transform = tile_projection_transform;
	}

	return GPlatesOpenGL::GLViewProjection(viewport, view_transform, projection_transform);
}


GPlatesOpenGL::GLIntersect::Plane
GPlatesGui::SceneView::get_globe_camera_front_horizon_plane() const
{
	return d_globe_camera.get_front_globe_horizon_plane();
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
