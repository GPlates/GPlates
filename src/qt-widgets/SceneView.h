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

#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>
#include <QImage>
#include <QSize>

#include "maths/Rotation.h"

#include "view-operations/QueryProximityThreshold.h"


namespace GPlatesMaths
{
	class LatLonPoint;
}

namespace  GPlatesGui
{
	class Camera;
}

namespace GPlatesQtWidgets
{

	/**
	 * Base class of GlobeCanvas and MapCanvas.
	 */
	class SceneView :
			public GPlatesViewOperations::QueryProximityThreshold,
			private boost::noncopyable
	{

	public:

		SceneView()
		{  }

		virtual
		~SceneView()
		{  }


		/**
		 * Return the camera controlling the view.
		 */
		virtual
		const GPlatesGui::Camera &
		get_camera() const = 0;

		/**
		 * Return the camera controlling the view.
		 */
		virtual
		GPlatesGui::Camera &
		get_camera() = 0;


		/**
		 * Returns the dimensions of the viewport in device *independent* pixels (ie, widget size).
		 *
		 * Device-independent pixels (widget size) differ from device pixels (OpenGL size).
		 * Widget dimensions are device independent whereas OpenGL uses device pixels
		 * (differing by the device pixel ratio).
		 */
		virtual
		QSize
		get_viewport_size() const = 0;

		/**
		 * Renders the scene to a QImage of the dimensions specified by @a image_size.
		 *
		 * The specified image size should be in device *independent* pixels (eg, widget dimensions).
		 * The returned image will be a high-DPI image if this canvas has a device pixel ratio greater than 1.0
		 * (in which case the returned QImage will have the same device pixel ratio).
		 *
		 * Returns a null QImage if unable to allocate enough memory for the image data.
		 */
		virtual
		QImage
		render_to_qimage(
				const QSize &image_size_in_device_independent_pixels) = 0;

		/**
		 * Paint the scene, as best as possible, by re-directing OpenGL rendering to the specified paint device.
		 *
		 * Normally the scene is rendered directly to the viewport widget using OpenGL.
		 * This method redirects OpenGL rendering to the specified paint device as best as possible
		 * by using OpenGL feedback to capture OpenGL draw commands and redirect them to the specified
		 * paint device - but there is loss of quality when doing this since OpenGL feedback bypasses the
		 * framebuffers (eg, colour/depth buffer) and so those per-pixel compositing effects are lost.
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


		/**
		 * Update/redraw the canvas.
		 */
		virtual
		void
		update_canvas() = 0;
	};
}

#endif // GPLATES_QTWIDGETS_SCENEVIEW_H
