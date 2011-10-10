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

#include "PreferencesDialog.h"

#include "QtWidgetUtils.h"
#include "app-logic/ApplicationState.h"
#include "app-logic/UserPreferences.h"
#include "utils/ConfigBundleUtils.h"
#include "gui/ConfigValueDelegate.h"


GPlatesQtWidgets::PreferencesDialog::PreferencesDialog(
		GPlatesAppLogic::ApplicationState &app_state,
		QWidget *parent_):
	GPlatesDialog(parent_, Qt::Dialog)
{
	setupUi(this);
	// It is very easy to accidentally leave a QStackedWidget on the wrong page after
	// editing with the Designer.
	stack_settings_ui->setCurrentIndex(0);
	
	// Connect up our basic signals and slots so the Category UI works.
	connect(list_categories, SIGNAL(currentRowChanged(int)),
			stack_settings_ui, SLOT(setCurrentIndex(int)));
	
	// Create and install the Table Of Every Preference Imaginable.
	QTableView *advanced_settings_table_ptr = GPlatesUtils::link_config_interface_to_table(
			app_state.get_user_preferences(), this);
	QtWidgetUtils::add_widget_to_placeholder(advanced_settings_table_ptr, advanced_settings_placeholder);
	
	GPlatesGui::ConfigValueDelegate *delegate = new GPlatesGui::ConfigValueDelegate(this);
	advanced_settings_table_ptr->setItemDelegate(delegate);
}


