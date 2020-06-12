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
#include <QStackedLayout>
#include <QtGlobal>
#include <QWidget>

#include "global/PointerTraits.h"

#include "gui/ColourScheme.h"

#include "maths/LatLonPoint.h"

#include "view-operations/QueryProximityThreshold.h"


// We only enable the pinch zoom gesture on the Mac.
#if defined(Q_OS_MAC)
#	define GPLATES_PINCH_ZOOM_ENABLED
#endif

namespace GPlatesGui
{
	class ViewportProjection;
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
	class GlobeCanvas;
	class MapCanvas;
	class MapView;
	class SceneView;

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

		//! Use this constructor if you want to make a clone of an existing GlobeAndMapWidget.
		GlobeAndMapWidget(
				const GlobeAndMapWidget *existing_globe_and_map_widget_ptr,
				GPlatesGui::ColourScheme::non_null_ptr_type colour_scheme,
				QWidget *parent_ = NULL);

		GlobeAndMapWidget *
		clone_with_shared_opengl_context(
				QWidget *parent_ = NULL);

		~GlobeAndMapWidget();

		GlobeCanvas &
		get_globe_canvas();

		const GlobeCanvas &
		get_globe_canvas() const;
		
		MapView &
		get_map_view();

		const MapView &
		get_map_view() const;

		SceneView &
		get_active_view();

		const SceneView &
		get_active_view() const;

		bool
		is_globe_active() const;

		bool
		is_map_active() const;

		boost::optional<GPlatesMaths::LatLonPoint>
		get_camera_llp() const;

		virtual
		QSize
		sizeHint() const;

		/**
		 * Returns the dimensions of the viewport in device *independent* pixels (ie, widget size).
		 *
		 * Device-independent pixels (widget size) differ from device pixels (OpenGL size).
		 * Widget dimensions are device independent, whereas OpenGL uses device pixels.
		 */
		QSize
		get_viewport_size() const;

		/**
		 * Returns the dimensions of the viewport in device pixels (not widget size).
		 *
		 * Device pixels (OpenGL size) differ from device-independent pixels (widget size).
		 * For high DPI displays (eg, Apple Retina), device pixels is typically twice device-independent pixels.
		 * OpenGL uses device pixels, whereas widget dimensions are device independent.
		 */
		QSize
		get_viewport_size_in_device_pixels() const;

		/**
		 * Renders the scene to a QImage of the dimensions specified by @a image_size.
		 *
		 * @a image_size is in pixels (not widget size). If the caller is rendering a high-DPI image
		 * they should multiply their widget size by the appropriate device pixel ratio and then call
		 * QImage::setDevicePixelRatio on the returned image.
		 *
		 * Returns a null QImage if unable to allocate enough memory for the image data.
		 */
		QImage
		render_to_qimage(
				const QSize &image_size);

		/**
		 * Returns the OpenGL context for the active view.
		 *
		 * Since both the globe and map views use OpenGL this returns whichever is currently active.
		 * Note that you should still call "GLContext::make_current()" before using the context
		 * to ensure it is the currently active OpenGL context.
		 */
		GPlatesGlobal::PointerTraits<GPlatesOpenGL::GLContext>::non_null_ptr_type
		get_active_gl_context();

		/**
		 * Returns the OpenGL layers used to filled polygons, render rasters and scalar fields.
		 */
		GPlatesGlobal::PointerTraits<GPlatesOpenGL::GLVisualLayers>::non_null_ptr_type
		get_active_gl_visual_layers();

		void
		update_canvas();

		virtual
		double
		current_proximity_inclusion_threshold(
				const GPlatesMaths::PointOnSphere &click_pos_on_globe) const;

		void
		set_zoom_enabled(
				bool enabled);

	Q_SIGNALS:

		void
		update_tools_and_status_message();

		void
		resized(
				int new_width, int new_height);

		void
		repainted(
				bool mouse_down);

	protected:

#ifdef GPLATES_PINCH_ZOOM_ENABLED
		virtual
		bool
		event(
				QEvent *ev);
#endif

		virtual
		void
		resizeEvent(
				QResizeEvent *resize_event);

		/**
		 * This is a virtual override of the function in QWidget.
		 *
		 * To quote the QWidget documentation:
		 *
		 * This event handler, for event event, can be reimplemented in a subclass to
		 * receive wheel events for the widget.
		 *
		 * If you reimplement this handler, it is very important that you ignore() the
		 * event if you do not handle it, so that the widget's parent can interpret it.
		 *
		 * The default implementation ignores the event.
		 */
		virtual
		void
		wheelEvent(
				QWheelEvent *event);

	private Q_SLOTS:

		void
		init();

		void
		handle_zoom_change();

		void
		about_to_change_projection(
				const GPlatesGui::ViewportProjection &view_projection);

		void
		change_projection(
			const GPlatesGui::ViewportProjection &view_projection);

		void
		handle_globe_or_map_repainted(
				bool mouse_down);

	private:

		GlobeAndMapWidget(
				const GlobeAndMapWidget *existing_widget,
				QWidget *parent_ = NULL);

		void
		make_signal_slot_connections();

		GPlatesPresentation::ViewState &d_view_state;

		boost::scoped_ptr<GlobeCanvas> d_globe_canvas_ptr;
		boost::scoped_ptr<MapView> d_map_view_ptr; // NOTE: Must be declared *after* d_globe_canvas_ptr.
		QStackedLayout *d_layout;

		/**
		 * Which of globe and map is currently active.
		 */
		SceneView *d_active_view_ptr;

		/**
		 * The camera position of the currently active view.
		 *
		 * Is boost::none if unable to retrieve the camera position from the currently active view.
		 */
		boost::optional<GPlatesMaths::LatLonPoint> d_active_camera_llp;

		/**
		 * Whether zooming (via mouse wheel or pinch gesture) is enabled.
		 */
		bool d_zoom_enabled;

#ifdef GPLATES_PINCH_ZOOM_ENABLED
		/**
		 * The viewport zoom percentage at the start of a pinch gesture.
		 * The value is boost::none if we're currently not in a pinch gesture.
		 */
		boost::optional<double> viewport_zoom_at_start_of_pinch;
#endif
	};
}

#endif  // GPLATES_QTWIDGETS_GLOBEANDMAPWIDGET_H
