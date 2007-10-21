/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 The University of Sydney, Australia
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
 
#ifndef GPLATES_GUI_GLOBECANVAS_H
#define GPLATES_GUI_GLOBECANVAS_H

#ifdef HAVE_PYTHON
// We need to include this _before_ any Qt headers get included because
// of a moc preprocessing problems with a feature called 'slots' in the
// python header file object.h
# include <boost/python.hpp>
#endif

#include <QtOpenGL/qgl.h>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "gui/Globe.h"
#include "gui/ViewportZoom.h"

#include "model/Reconstruction.h"
#include "maths/PolylineOnSphere.h"

namespace GPlatesQtWidgets 
{
	// Remove this when there is a ViewState class.
	class ViewportWindow;

	class QueryFeaturePropertiesDialog;

	class GlobeCanvas:
			public QGLWidget 
	{
		Q_OBJECT

	public:
		explicit
		GlobeCanvas(
				ViewportWindow &view_state,
				QWidget *parent_ = 0);

		void
		set_reconstruction(
				const GPlatesModel::Reconstruction::non_null_ptr_type &new_recon)
		{
			d_reconstruction_ptr = new_recon.get();
		}

		void
		draw_polyline(
				const GPlatesMaths::PolylineOnSphere &polyline);

		void
		draw_point(
				const GPlatesMaths::PointOnSphere &point);

		void
		update_canvas();

		void
		clear_data();

		void
		zoom_in();

		void
		zoom_out();

		void
		zoom_reset();

#if 0
		typedef std::pair< std::string, std::string > line_header_type;
#endif

	protected:
		/**
		 * This is a virtual override of the function in QGLWidget.
		 *
		 * To quote the QGLWidget documentation:
		 *
		 * This virtual function is called once before the first call to paintGL() or
		 * resizeGL(), and then once whenever the widget has been assigned a new
		 * QGLContext.  Reimplement it in a subclass.
		 *
		 * This function should set up any required OpenGL context rendering flags,
		 * defining display lists, etc.
		 *
		 * There is no need to call makeCurrent() because this has already been done when
		 * this function is called.
		 */
		virtual 
		void 
		initializeGL();

		/**
		 * This is a virtual override of the function in QGLWidget.
		 *
		 * To quote the QGLWidget documentation:
		 *
		 * This virtual function is called whenever the widget has been resized.  The new
		 * size is passed in width and height.  Reimplement it in a subclass.
		 *
		 * There is no need to call makeCurrent() because this has already been done when
		 * this function is called.
		 */
		virtual
		void 
		resizeGL(
				int width, 
				int height);

		/**
		 * This is a virtual override of the function in QGLWidget.
		 *
		 * To quote the QGLWidget documentation:
		 *
		 * This virtual function is called whenever the widget needs to be painted.
		 * Reimplement it in a subclass.
		 *
		 * There is no need to call makeCurrent() because this has already been done when
		 * this function is called.
		 */
		virtual
		void
		paintGL();

		/**
		 * This is a virtual override of the function in QWidget.
		 *
		 * To quote the QWidget documentation:
		 *
		 * This event handler, for event event, can be reimplemented in a subclass to
		 * receive mouse press events for the widget.
		 *
		 * If you create new widgets in the mousePressEvent() the mouseReleaseEvent() may
		 * not end up where you expect, depending on the underlying window system (or X11
		 * window manager), the widgets' location and maybe more.
		 *
		 * The default implementation implements the closing of popup widgets when you
		 * click outside the window.  For other widget types it does nothing.
		 */
		virtual
		void
		mousePressEvent(
				QMouseEvent *event);

		/**
		 * This is a virtual override of the function in QWidget.
		 *
		 * To quote the QWidget documentation:
		 *
		 * This event handler, for event event, can be reimplemented in a subclass to
		 * receive mouse move events for the widget.
		 *
		 * If mouse tracking is switched off, mouse move events only occur if a mouse
		 * button is pressed while the mouse is being moved.  If mouse tracking is switched
		 * on, mouse move events occur even if no mouse button is pressed.
		 *
		 * QMouseEvent::pos() reports the position of the mouse cursor, relative to this
		 * widget.  For press and release events, the position is usually the same as the
		 * position of the last mouse move event, but it might be different if the user's
		 * hand shakes.  This is a feature of the underlying window system, not Qt.
		 */
		virtual 
		void 
		mouseMoveEvent(
				QMouseEvent *event);

		/**
		 * This is a virtual override of the function in QWidget.
		 *
		 * To quote the QWidget documentation:
		 *
		 * This event handler, for event event, can be reimplemented in a subclass to
		 * receive mouse release events for the widget.
		 */
		virtual 
		void 
		mouseReleaseEvent(
				QMouseEvent *event);

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

	signals:
		void
		current_global_pos_changed(
				double latitude,
				double longtitude);

		void
		current_global_pos_off_globe();

		void
		no_items_selected_by_click();

		void
		current_zoom_changed(
				double zoom_percent);

#if 0
		void
		items_selected(
				std::vector< line_header_type > &items);
#endif

		void
		left_mouse_button_clicked();

	private:
		ViewportWindow *d_view_state_ptr;

		boost::intrusive_ptr<GPlatesModel::Reconstruction> d_reconstruction_ptr;
		boost::shared_ptr<QueryFeaturePropertiesDialog> d_query_feature_properties_dialog_ptr;

		int d_width;
		int d_height;

		long d_mouse_x;
		long d_mouse_y;

		double d_smaller_dim;
		double d_larger_dim;

		GPlatesGui::Globe d_globe;
		GPlatesGui::ViewportZoom d_viewport_zoom;

		void
		handle_zoom_change();

		void
		set_view();

		void
		get_dimensions();

		void
		handle_mouse_motion();

		void
		handle_right_mouse_down();

		void
		handle_left_mouse_down();

		void
		handle_right_mouse_drag();

		void
		handle_wheel_rotation(
				int delta);

		double
		get_universe_coord_y(
				int screen_x);

		double
		get_universe_coord_z(
				int screen_y);

		void
		clear_canvas(
				const QColor& color = Qt::black);
	};

}

#endif
