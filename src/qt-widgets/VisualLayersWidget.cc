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

#include "VisualLayersWidget.h"

#include "AddNewLayerDialog.h"
#include "QtWidgetUtils.h"
#include "ViewportWindow.h"
#include "VisualLayersListView.h"

#include "gui/Dialogs.h"

#include "utils/ComponentManager.h"


GPlatesQtWidgets::VisualLayersWidget::VisualLayersWidget(
		GPlatesPresentation::VisualLayers &visual_layers,
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	QWidget(parent_),
	d_visual_layers(visual_layers),
	d_application_state(application_state),
	d_view_state(view_state),
	d_viewport_window(viewport_window)
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

	if(GPlatesUtils::ComponentManager::instance().is_enabled(GPlatesUtils::ComponentManager::Component::python()))
	{
		QObject::connect(
				colouring_button,
				SIGNAL(clicked()),
				&d_viewport_window->dialogs(),
				SLOT(pop_up_draw_style_dialog()));
	}
	else
	{
		QObject::connect(
				colouring_button,
				SIGNAL(clicked()),
				this,
				SLOT(handle_colouring_button_clicked()));
	}

	QObject::connect(
				button_show_all,
				SIGNAL(clicked()),
				this,
				SLOT(handle_show_all_button_clicked()));

	QObject::connect(
				button_hide_all,
				SIGNAL(clicked()),
				this,
				SLOT(handle_hide_all_button_clicked()));

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


void
GPlatesQtWidgets::VisualLayersWidget::handle_colouring_button_clicked()
{
	d_viewport_window->dialogs().pop_up_colouring_dialog();
}

void
GPlatesQtWidgets::VisualLayersWidget::handle_show_all_button_clicked()
{
	d_visual_layers.show_all();
}

void
GPlatesQtWidgets::VisualLayersWidget::handle_hide_all_button_clicked()
{
	d_visual_layers.hide_all();
}

