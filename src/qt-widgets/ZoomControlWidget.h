/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_QTWIDGETS_ZOOMCONTROLWIDGET_H
#define GPLATES_QTWIDGETS_ZOOMCONTROLWIDGET_H

#include <QWidget>
#include "ZoomControlWidgetUi.h"


namespace GPlatesGui
{
	class ViewportZoom;
}


namespace GPlatesQtWidgets
{
	/**
	 * Small widget with a spinbox and three buttons for controlling the
	 * zoom level. This is done as a separate widget for more flexibility
	 * in what we attempt to cram into the ReconstructionViewWidget, and
	 * where.
	 */
	class ZoomControlWidget:
			public QWidget,
			protected Ui_ZoomControlWidget
	{
		Q_OBJECT

	public:
		explicit
		ZoomControlWidget(
				GPlatesGui::ViewportZoom &vzoom,
				QWidget *parent_);

	signals:

		/**
		 * Emitted when the user has entered a new zoom value in the spinbox.
		 * The ReconstructionViewWidget listens for this signal so it can give
		 * the globe keyboard focus again after editing.
		 */
		void
		editing_finished();
	
	public slots:
		
		/**
		 * Focuses the spinbox and highlights text, ready to be replaced.
		 */
		void
		activate_zoom_spinbox();
		
		/**
		 * Sets whether you want the + / - / 1 buttons shown or hidden.
		 * Defaults to false.
		 */
		void
		show_buttons(
				bool show_);

		/**
		 * Sets whether you want the "Zoom:" label shown or hidden.
		 * Defaults to true.
		 */
		void
		show_label(
				bool show_);

	private slots:
		
		/**
		 * In response to a zoom event, this will set the spinbox to reflect the new
		 * zoom level percentage.
		 */
		void
		handle_zoom_changed();

		/**
		 * In response to user spinning to a new zoom percent value and hitting 'enter'.
		 */
		void
		handle_spinbox_changed();
	
	private:
	
		/**
		 * This is a pointer to the viewport zoom we are using to control
		 * the current zoom level (and react to zoom events not caused
		 * by us so we can update our spinbox).
		 */
		GPlatesGui::ViewportZoom *d_viewport_zoom_ptr;
	};
}

#endif  // GPLATES_QTWIDGETS_ZOOMCONTROLWIDGET_H
