/* $Id$ */

/**
 * \file 
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

#include "VisualLayersWidget.h"

#include "AddNewLayerDialog.h"
#include "QtWidgetUtils.h"
#include "VisualLayersListView.h"


GPlatesQtWidgets::VisualLayersWidget::VisualLayersWidget(
		GPlatesPresentation::VisualLayers &visual_layers,
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	QWidget(parent_),
	d_visual_layers(visual_layers),
	d_application_state(application_state),
	d_view_state(view_state)
{
	setupUi(this);

	QListView *list = new VisualLayersListView(
			d_visual_layers,
			application_state,
			view_state,
			viewport_window,
			this);

	QtWidgetUtils::add_widget_to_placeholder(list, layers_list_placeholder_widget);
	list->setFocus();

	QObject::connect(
			add_new_layer_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_add_new_layer_button_clicked()));

	// Hide things for now...
	control_widget->hide();
}


GPlatesQtWidgets::VisualLayersWidget::~VisualLayersWidget()
{  }


void
GPlatesQtWidgets::VisualLayersWidget::handle_add_new_layer_button_clicked()
{
	if (!d_add_new_layer_dialog)
	{
		d_add_new_layer_dialog.reset(
				new AddNewLayerDialog(
					d_application_state,
					d_view_state,
					this));
	}

	d_add_new_layer_dialog->exec();
}

