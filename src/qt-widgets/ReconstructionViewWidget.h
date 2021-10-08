/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_RECONSTRUCTIONVIEWWIDGET_H
#define GPLATES_QTWIDGETS_RECONSTRUCTIONVIEWWIDGET_H

#ifdef HAVE_PYTHON
// We need to include this _before_ any Qt headers get included because
// of a moc preprocessing problems with a feature called 'slots' in the
// python header file object.h
# include <boost/python.hpp>
#endif

#include <QWidget>
#include "ReconstructionViewWidgetUi.h"


namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesQtWidgets
{
	class ViewportWindow;
	class GlobeCanvas;


	class ReconstructionViewWidget:
			public QWidget, 
			protected Ui_ReconstructionViewWidget
	{
		Q_OBJECT
		
	public:
		explicit
		ReconstructionViewWidget(
				ViewportWindow &view_state,
				QWidget *parent_ = NULL);

		static
		inline
		double
		min_reconstruction_time()
		{
			// This value denotes the present-day.
			return 0.0;
		}

		static
		inline
		double
		max_reconstruction_time()
		{
			// This value denotes a time 10000 million years ago.
			return 10000.0;
		}

		static
		bool
		is_valid_reconstruction_time(
				const double &time);

		double
		reconstruction_time() const
		{
			return spinbox_reconstruction_time->value();
		}

		double
		zoom_percent() const
		{
			return spinbox_zoom_percent->value();
		}

		GlobeCanvas &
		globe_canvas() const
		{
			return *d_canvas_ptr;
		}

	public slots:
		void
		activate_time_spinbox()
		{
			spinbox_reconstruction_time->setFocus();
			spinbox_reconstruction_time->selectAll();
		}

		void
		set_reconstruction_time(
				double new_recon_time);

		void
		increment_reconstruction_time()
		{
			set_reconstruction_time(reconstruction_time() + 1.0);
		}

		void
		decrement_reconstruction_time()
		{
			set_reconstruction_time(reconstruction_time() - 1.0);
		}

		void
		propagate_reconstruction_time()
		{
			emit reconstruction_time_changed(reconstruction_time());
		}

		void
		recalc_camera_position();

		void
		update_mouse_pointer_position(
				const GPlatesMaths::PointOnSphere &new_virtual_pos,
				bool is_on_globe);
		
		void
		activate_zoom_spinbox()
		{
			spinbox_zoom_percent->setFocus();
			spinbox_zoom_percent->selectAll();
		}
		
		void
		set_zoom(
				double new_zoom_percent)
		{
			spinbox_zoom_percent->setValue(new_zoom_percent);
		}
		
		void
		propagate_zoom_percent()
		{
			emit zoom_changed(zoom_percent());
		}

	signals:
		void
		reconstruction_time_changed(
				double new_reconstruction_time);
		
		void
		zoom_changed(
				double new_zoom_percent);

	private:
		GlobeCanvas *d_canvas_ptr;

	};
}

#endif  // GPLATES_QTWIDGETS_RECONSTRUCTIONVIEWWIDGET_H
