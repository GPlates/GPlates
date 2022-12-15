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

#ifndef GPLATES_GUI_SCENEVIEW_H
#define GPLATES_GUI_SCENEVIEW_H

#include <boost/optional.hpp>
#include <QObject>
#include <QPointF>

#include "Projection.h"

#include "maths/PointOnSphere.h"

#include "opengl/GLIntersectPrimitives.h"
#include "opengl/GLViewport.h"
#include "opengl/GLViewProjection.h"

#include "utils/ReferenceCount.h"


namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesGui
{
	class Camera;
	class GlobeCamera;
	class MapCamera;
	class MapProjection;
	class ViewportZoom;

	/**
	 * The view (including projection) of the scene (globe and map).
	 */
	class SceneView :
			public QObject,
			public GPlatesUtils::ReferenceCount<SceneView>
	{
		Q_OBJECT

	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<SceneView> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const SceneView> non_null_ptr_to_const_type;


		/**
		 * Creates a new @a SceneView object.
		 */
		static
		non_null_ptr_type
		create(
				GPlatesPresentation::ViewState &view_state)
		{
			return non_null_ptr_type(new SceneView(view_state));
		}


		/**
		 * Return the camera controlling the current view (globe or map camera).
		 */
		const Camera &
		get_active_camera() const;

		Camera &
		get_active_camera();


		/**
		 * Returns true if the globe view is currently active.
		 */
		bool
		is_globe_active() const;

		/**
		 * Returns true if the map view is currently active.
		 */
		bool
		is_map_active() const
		{
			return !is_globe_active();
		}


		/**
		 * Return the viewport zoom.
		 */
		const ViewportZoom &
		get_viewport_zoom() const
		{
			return d_viewport_zoom;
		}

		ViewportZoom &
		get_viewport_zoom()
		{
			return d_viewport_zoom;
		}


		/**
		 * Get the view-projection transform of the current view, and combine it with the specified viewport.
		 *
		 * Note that the viewport aspect ratio affects the projection transform.
		 */
		GPlatesOpenGL::GLViewProjection
		get_view_projection(
				const GPlatesOpenGL::GLViewport &viewport) const;

		/**
		 * Returns the position on the globe (in the current globe or map view) at the specified window coordinate.
		 *
		 * When the map is active (ie, when globe is inactive) the window coordinate is considered to intersect
		 * the globe if it intersects the map plane at a position that is inside the map projection boundary.
		 * When the map is active @a position_on_map_plane is set to the position on the map plane if the
		 * window coordinate intersects the map plane (or none if misses). When the globe is active it is set to none.
		 *
		 * If the window coordinate misses the globe in globe view (or is outside map projection boundary in map view)
		 * then the nearest point on the globe horizon visible circumference in globe view (or nearest point on
		 * the map projection boundary in map view) is returned instead. And @a is_on_globe is set to false.
		 *
		 * Window coordinates are typically in the range [0, window_width] and [0, window_height]
		 * where (0, 0) is bottom-left and (window_width, window_height) is top-right of window.
		 * Note that we use the OpenGL convention where 'window_x = 0' is the bottom of the window.
		 * But in Qt it means top, so a Qt mouse y coordinate (for example) needs be inverted
		 * before passing to this method.
		 *
		 * Note that either/both window coordinate could be outside the range[0, window_width] and
		 * [0, window_height], in which case the window coordinate is not associated with a window pixel
		 * inside the viewport (visible projected scene).
		 */
		GPlatesMaths::PointOnSphere
		get_position_on_globe_at_window_coord(
				double window_x,
				double window_y,
				int window_width,
				int window_height,
				bool &is_on_globe,
				boost::optional<QPointF> &position_on_map_plane) const;

		/**
		 * Returns the plane that separates the visible front half of the globe from invisible rear half.
		 *
		 * Note: This only applies to the globe view (not the map view).
		 */
		GPlatesOpenGL::GLIntersect::Plane
		get_globe_camera_front_horizon_plane() const;

		/**
		 * The proximity inclusion threshold is a dot product (cosine) measure of how close a geometry
		 * must be to a click-point be considered "hit" by the click.
		 *
		 * This will depend on the projection of the globe/map.
		 * For 3D projections the horizon of the globe will need a larger threshold than the centre of the globe.
		 * For 2D projections the threshold will vary with the 'stretch' around the clicked-point.
		 *
		 * Note: The viewport should be in device *independent* coordinates.
		 *       This way if a user has a high DPI display (like Apple Retina) the higher pixel resolution
		 *       does not force them to have more accurate mouse clicks.
		 */
		double
		current_proximity_inclusion_threshold(
				const GPlatesMaths::PointOnSphere &click_point,
				const GPlatesOpenGL::GLViewport &viewport) const;


	Q_SIGNALS:
	
		/**
		 * This signal is emitted when the view changes.
		 */
		void
		view_changed();

	private Q_SLOTS:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		handle_camera_change();

		void
		handle_globe_map_projection_changed(
				const GPlatesGui::Projection::globe_map_projection_type &old_globe_map_projection,
				const GPlatesGui::Projection::globe_map_projection_type &globe_map_projection);

		void
		handle_viewport_projection_changed(
				GPlatesGui::Projection::viewport_projection_type old_viewport_projection,
				GPlatesGui::Projection::viewport_projection_type viewport_projection);

	private:

		/**
		 * The projection determines whether the globe view or map view is currently active
		 * (as well as whether the view is orthographic or perspective).
		 */
		Projection &d_projection;

		GlobeCamera &d_globe_camera;
		MapCamera &d_map_camera;

		MapProjection &d_map_projection;

		ViewportZoom &d_viewport_zoom;


		explicit
		SceneView(
				GPlatesPresentation::ViewState &view_state);

		/**
		 * Get position on globe when globe is active (ie, when map is inactive).
		 */
		GPlatesMaths::PointOnSphere
		get_position_on_globe(
				const GPlatesOpenGL::GLIntersect::Ray &camera_ray,
				bool &is_on_globe) const;

		/**
		 * Get position on globe when map is active (ie, when globe is inactive).
		 */
		GPlatesMaths::PointOnSphere
		get_position_on_map(
				const GPlatesOpenGL::GLIntersect::Ray &camera_ray,
				bool &is_on_globe,
				boost::optional<QPointF> &position_on_map_plane) const;
	};
}

#endif // GPLATES_GUI_SCENEVIEW_H
