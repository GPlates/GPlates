/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2011 Geological Survey of Norway.
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
 
#ifndef GPLATES_QTWIDGETS_SCENEVIEW_H
#define GPLATES_QTWIDGETS_SCENEVIEW_H

#include "boost/optional.hpp"
#include <QImage>
#include <QSize>

#include "maths/Rotation.h"

#include "view-operations/QueryProximityThreshold.h"


namespace GPlatesMaths
{
	class LatLonPoint;
}

namespace GPlatesQtWidgets
{

	/**
	 * Base class of GlobeCanvas and MapView.
	 */
	class SceneView :
			public GPlatesViewOperations::QueryProximityThreshold
	{

	public:

		SceneView()
			{};

		virtual
		~SceneView()
			{};

		virtual
		void
		set_camera_viewpoint(
			const GPlatesMaths::LatLonPoint &llp) = 0;

		// FIXME should this be pure virtual? 
		virtual
		void
		set_orientation(
			const GPlatesMaths::Rotation &rotation
			/*bool should_emit_external_signal = true */)
		{ };

		virtual
		boost::optional<GPlatesMaths::Rotation>
		orientation() const = 0;

		virtual
		void
		handle_zoom_change()
		{ };

		virtual
		boost::optional<GPlatesMaths::LatLonPoint>
		camera_llp() const = 0;

		/**
		 * Returns the dimensions of the viewport in device *independent* pixels (ie, widget size).
		 *
		 * Device-independent pixels (widget size) differ from device pixels (OpenGL size).
		 * Widget dimensions are device independent, whereas OpenGL uses device pixels.
		 */
		virtual
		QSize
		get_viewport_size() const = 0;

		/**
		 * Returns the dimensions of the viewport in device pixels (not widget size).
		 *
		 * Device pixels (OpenGL size) differ from device-independent pixels (widget size).
		 * For high DPI displays (eg, Apple Retina), device pixels is typically twice device-independent pixels.
		 * OpenGL uses device pixels, whereas widget dimensions are device independent.
		 */
		virtual
		QSize
		get_viewport_size_in_device_pixels() const = 0;

		/**
		 * Renders the scene to a QImage of the dimensions specified by @a image_size.
		 *
		 * @a image_size is in pixels (not widget size). If the caller is rendering a high-DPI image
		 * they should multiply their widget size by the appropriate device pixel ratio and then call
		 * QImage::setDevicePixelRatio on the returned image.
		 *
		 * Returns a null QImage if unable to allocate enough memory for the image data.
		 */
		virtual
		QImage
		render_to_qimage(
				const QSize &image_size) = 0;

		/**
		 * Paint the scene, as best as possible, by re-directing OpenGL rendering to the specified paint device.
		 *
		 * Normally the scene is rendered directly to the viewport widget using OpenGL.
		 * This method redirects OpenGL rendering to the specified paint device as best as possible
		 * by using OpenGL feedback to capture OpenGL draw commands and redirect them to the specified
		 * paint device - but there is loss of quality when doing this since OpenGL feedback bypasses the
		 * frame buffers (eg, colour/depth buffer) and so those per-pixel compositing effects are lost.
		 *
		 * This is typically used for rendering to an SVG file (QPaintDevice = QSvgGenerator), but could
		 * conceivably be used for a QPaintDevice other than QSvgGenerator - although probably not
		 * likely because rendering vector and raster data to a QImage, for example, directly via OpenGL
		 * is usually desired (ie, render directly to viewport widget using OpenGL and then extract
		 * the composited image from the widget - instead of passing a QImage to this method).
		 *
		 * NOTE: This renders all RenderedGeometryCollection layers (not just RECONSTRUCTION_LAYER).
		 * If you only want only RECONSTRUCTION_LAYER then you need to disable all other layers.
		 */
		virtual
		void
		render_opengl_feedback_to_paint_device(
				QPaintDevice &feedback_paint_device) = 0;

		virtual
		void
		update_canvas() = 0;

		virtual
		void
		move_camera_up() = 0;

		virtual
		void
		move_camera_down() = 0;

		virtual
		void
		move_camera_left() = 0;

		virtual
		void
		move_camera_right() = 0;

		virtual
		void
		rotate_camera_clockwise() = 0;

		virtual
		void
		rotate_camera_anticlockwise() = 0;

		virtual
		void
		reset_camera_orientation() = 0;

	private:
		// Make copy and assignment private to prevent copying/assignment
		SceneView(
			const SceneView &other);

		SceneView &
		operator=(
			const SceneView &other);

	};

}


#endif // GPLATES_QTWIDGETS_SCENEVIEW_H
