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

#include <QDialogButtonBox>
#include <QListWidget>
#include <QStackedWidget>
#include <QScrollArea>

#include "PreferencesDialog.h"

#include "PreferencesPaneFiles.h"
#include "PreferencesPaneNetwork.h"
#include "PreferencesPaneView.h"

#include "QtWidgetUtils.h"
#include "app-logic/ApplicationState.h"
#include "app-logic/UserPreferences.h"
#include "gui/ConfigGuiUtils.h"


GPlatesQtWidgets::PreferencesDialog::PreferencesDialog(
		GPlatesAppLogic::ApplicationState &app_state,
		QWidget *parent_):
	GPlatesDialog(parent_, Qt::Dialog)
{
	setupUi(this);
	
	// All the Preference Panes except the Advanced pane are set up here, in order:-
	int index = 0;
	add_pane(index++, tr("View"), new PreferencesPaneView(app_state, this), false);
	add_pane(index++, tr("Files"), new PreferencesPaneFiles(app_state, this), true);	// might get large enough to need scrolling.
	add_pane(index++, tr("Network"), new PreferencesPaneNetwork(app_state, this), false);
	
	
	// It is very easy to accidentally leave a QStackedWidget on the wrong page after
	// editing with the Designer. And in this case we've been mucking about with it in code
	// anyway - forcing the stack to the first page is the way to go.
	stack_settings_ui->setCurrentIndex(0);
	
	// Connect up our basic signals and slots so the Category UI works.
	connect(list_categories, SIGNAL(currentRowChanged(int)),
			stack_settings_ui, SLOT(setCurrentIndex(int)));
	
	// Create and install the Table Of Every Preference Imaginable.
	d_cfg_table = 
			GPlatesGui::ConfigGuiUtils::link_config_interface_to_table(
					app_state.get_user_preferences(), true, this);
	QtWidgetUtils::add_widget_to_placeholder(d_cfg_table, advanced_settings_placeholder);
}



void
GPlatesQtWidgets::PreferencesDialog::add_pane(
		int index,
		const QString &category_label,
		QWidget *pane_widget,
		bool scrolling)
{
	// If a scrolling pane is requested, we first wrap the real pane_widget with a QScrollArea.
	if (scrolling) {
		QScrollArea *scroll_pane = new QScrollArea(stack_settings_ui);
		scroll_pane->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		scroll_pane->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
		scroll_pane->setWidget(pane_widget);		// Takes ownership of pane_widget.
		pane_widget = scroll_pane;						// the wrapping is complete; switch pointers.
	}
	
	// The left-hand list of Category choices must match the order of stacked widgets,
	// so we will set them both up here:-
	list_categories->insertItem(index, category_label);
	stack_settings_ui->insertWidget(index, pane_widget);		// QStackedWidget takes ownership.
}


