/* $Id$ */

/**
 * \file
 * Contains the definition of the GlobeAndMapWidget class.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_GLOBEANDMAPWIDGET_H
#define GPLATES_QTWIDGETS_GLOBEANDMAPWIDGET_H

#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <QtGlobal>
#include <QWidget>

#include "global/PointerTraits.h"

#include "gui/Colour.h"

#include "view-operations/QueryProximityThreshold.h"


 // We only enable the pinch zoom gesture on the Mac.
#if defined(Q_OS_MACOS)
#	define GPLATES_PINCH_ZOOM_ENABLED
#endif


namespace GPlatesGui
{
	class Camera;
	class Colour;
	class ViewportZoom;
}

namespace GPlatesOpenGL
{
	class GLContext;
	class GLVisualLayers;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class GlobeAndMapCanvas;

	/**
	 * This class is responsible for creating and holding the globe and the map,
	 * and for switching between them as appropriate.
	 */
	class GlobeAndMapWidget :
			public QWidget,
			public GPlatesViewOperations::QueryProximityThreshold
	{
		Q_OBJECT

	public:

		//! Use this constructor if you're constructing a fresh GlobeAndMapWidget from scratch.
		GlobeAndMapWidget(
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_ = NULL);

		~GlobeAndMapWidget();


		GlobeAndMapCanvas &
		get_globe_and_map_canvas();

		const GlobeAndMapCanvas &
		get_globe_and_map_canvas() const;


		/**
		 * Return the camera controlling the current view (globe or map camera).
		 */
		GPlatesGui::Camera &
		get_active_camera();

		/**
		 * Return the camera controlling the current view (globe or map camera).
		 */
		const GPlatesGui::Camera &
		get_active_camera() const;


		/**
		 * Returns true if the globe view is currently active.
		 */
		bool
		is_globe_active() const;

		/**
		 * Returns true if the map view is currently active (ie, globe view not active).
		 */
		bool
		is_map_active() const;


		/**
		 * Returns the dimensions of the viewport in device *independent* pixels (ie, widget size).
		 *
		 * Device-independent pixels (widget size) differ from device pixels (OpenGL size).
		 * Widget dimensions are device independent whereas OpenGL uses device pixels
		 * (differing by the device pixel ratio).
		 */
		QSize
		get_viewport_size() const;

		/**
		 * Renders the scene to a QImage of the dimensions specified by @a image_size.
		 *
		 * The specified image size should be in device *independent* pixels (eg, widget dimensions).
		 * The returned image will be a high-DPI image if this canvas has a device pixel ratio greater than 1.0
		 * (in which case the returned QImage will have the same device pixel ratio).
		 *
		 * The image is cleared to @a image_clear_colour prior to rendering.
		 * This colour will show through for pixels not rendered (or pixel rendered with transparent alpha).
		 *
		 * Returns a null QImage if unable to allocate enough memory for the image data.
		 */
		QImage
		render_to_image(
				const QSize &image_size_in_device_independent_pixels,
				const GPlatesGui::Colour &image_clear_colour);

		/**
		 * Returns the OpenGL context.
		 */
		GPlatesGlobal::PointerTraits<GPlatesOpenGL::GLContext>::non_null_ptr_type
		get_gl_context();

		/**
		 * Returns the OpenGL layers used to filled polygons, render rasters and scalar fields.
		 */
		GPlatesGlobal::PointerTraits<GPlatesOpenGL::GLVisualLayers>::non_null_ptr_type
		get_gl_visual_layers();

		void
		update_canvas();

		/**
		 * The proximity inclusion threshold is a dot product (cosine) measure of how close a geometry
		 * must be to a click-point be considered "hit" by the click.
		 *
		 * This will depend on the projection of the globe/map.
		 * For 3D projections the horizon of the globe will need a larger threshold than the centre of the globe.
		 * For 2D projections the threshold will vary with the 'stretch' around the clicked-point.
		 */
		double
		current_proximity_inclusion_threshold(
				const GPlatesMaths::PointOnSphere &click_pos_on_globe) const override;

	Q_SIGNALS:

		void
		repainted(
				bool mouse_down);

	protected:

#ifdef GPLATES_PINCH_ZOOM_ENABLED
		bool
		event(
				QEvent *ev) override;
#endif

	private Q_SLOTS:

		void
		handle_globe_or_map_repainted(
				bool mouse_down);

	private:

		GlobeAndMapWidget(
				const GlobeAndMapWidget *existing_widget,
				QWidget *parent_ = NULL);

		boost::scoped_ptr<GlobeAndMapCanvas> d_globe_and_map_canvas_ptr;

#ifdef GPLATES_PINCH_ZOOM_ENABLED
		GPlatesGui::ViewportZoom &d_viewport_zoom;

		/**
		 * The viewport zoom percentage at the start of a pinch gesture.
		 * The value is boost::none if we're currently not in a pinch gesture.
		 */
		boost::optional<double> d_viewport_zoom_at_start_of_pinch;
#endif
	};
}

#endif  // GPLATES_QTWIDGETS_GLOBEANDMAPWIDGET_H
