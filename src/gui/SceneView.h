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

#include "Projection.h"

#include "maths/PointOnSphere.h"

#include "opengl/GLMatrix.h"
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
		 * Creates a new @a Scene object.
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

		/**
		 * Return the camera controlling the current view (globe or map camera).
		 */
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
		 * Get the view-projection transform of the current view, and combine it with the specified viewport.
		 *
		 * An optional tile projection transform can pre-multiply the projection transform of the current view
		 * (see @a GLTileRender).
		 */
		GPlatesOpenGL::GLViewProjection
		get_view_projection(
				const GPlatesOpenGL::GLViewport &viewport,
				boost::optional<const GPlatesOpenGL::GLMatrix &> tile_projection_transform = boost::none) const;

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


		explicit
		SceneView(
				GPlatesPresentation::ViewState &view_state);
	};
}

#endif // GPLATES_GUI_SCENEVIEW_H
