/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_VISUALLAYERSWIDGET_H
#define GPLATES_QTWIDGETS_VISUALLAYERSWIDGET_H

#include <boost/scoped_ptr.hpp>
#include <QWidget>

#include "ui_VisualLayersWidgetUi.h"

#include "gui/VisualLayersProxy.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	class ViewState;
	class VisualLayers;
}

namespace GPlatesQtWidgets
{
	// Forward declarations.
	class AddNewLayerDialog;
	class ViewportWindow;

	/**
	 * VisualLayersWidget displays the contents of VisualLayers. It displays a
	 * widget for each visual layer, as well as tools to manipulate those layers.
	 */
	class VisualLayersWidget :
			public QWidget,
			protected Ui_VisualLayersWidget
	{
		Q_OBJECT
		
	public:

		explicit
		VisualLayersWidget(
				GPlatesPresentation::VisualLayers &visual_layers,
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				QWidget *parent_ = NULL);

		virtual
		~VisualLayersWidget();

	private Q_SLOTS:

		void
		handle_add_new_layer_button_clicked();

		void
		handle_colouring_button_clicked();

		void
		handle_show_all_button_clicked();

		void
		handle_hide_all_button_clicked();

	private:

		/**
		 * A wrapper around VisualLayers to invert the ordering for the user interface.
		 */
		GPlatesGui::VisualLayersProxy d_visual_layers;
		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		ViewportWindow *d_viewport_window;

		boost::scoped_ptr<AddNewLayerDialog> d_add_new_layer_dialog;
	};
}

#endif  // GPLATES_QTWIDGETS_VISUALLAYERSWIDGET_H
