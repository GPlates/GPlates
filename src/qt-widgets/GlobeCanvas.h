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

#include "global/types.h"
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
		virtual 
		void 
		initializeGL();

		virtual
		void 
		resizeGL(
				int width, 
				int height);

		virtual
		void
		paintGL();

		virtual
		void
		mousePressEvent(
				QMouseEvent *event);

		virtual 
		void 
		mouseMoveEvent(
				QMouseEvent *event);

		virtual 
		void 
		mouseReleaseEvent(
				QMouseEvent *event);

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

		GPlatesMaths::real_t
		get_universe_coord_y(
				int screen_x);

		GPlatesMaths::real_t
		get_universe_coord_z(
				int screen_y);

		void
		clear_canvas(
				const QColor& color = Qt::black);
	};

}

#endif
