/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include <QFileDialog>

#include "PreferencesPaneFiles.h"

#include "QtWidgetUtils.h"
#include "app-logic/ApplicationState.h"
#include "app-logic/UserPreferences.h"
#include "gui/ConfigGuiUtils.h"

namespace
{
	// Could probably be moved to QtUtils code.
	void
	link_dir_chooser_button(
			QAbstractButton *button,
			QLineEdit *lineedit)
	{
		QFileDialog *chooser = new QFileDialog(button);
		chooser->setFileMode(QFileDialog::Directory);
		chooser->setDirectory(lineedit->text());	// Could be more Clever, but this will do for most people.
		QObject::connect(button, SIGNAL(clicked()), chooser, SLOT(exec()));
		QObject::connect(chooser, SIGNAL(fileSelected(QString)), lineedit, SLOT(setText(QString)));
		// A bit of a hack to make it look like a user edit of the lineedit, not a programattical edit.
		// Otherwise, the UserPrefs link won't trigger.
		QObject::connect(chooser, SIGNAL(fileSelected(QString)), lineedit, SIGNAL(editingFinished()));
	}
}



GPlatesQtWidgets::PreferencesPaneFiles::PreferencesPaneFiles(
		GPlatesAppLogic::ApplicationState &app_state,
		QWidget *parent_):
	QWidget(parent_)
{
	setupUi(this);
	GPlatesAppLogic::UserPreferences &prefs = app_state.get_user_preferences();
	
	// Loading and Saving UserPreferences link:-
	
	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(lineedit_default_export_dir, prefs,
			"paths/default_export_dir", toolbutton_reset_default_export_dir);
	link_dir_chooser_button(toolbutton_choose_default_export_dir, lineedit_default_export_dir);
	// TODO: Default Load path, default Save path, options for whether those paths should
	// just follow wherever the user last successfully saved / loaded.
	
	// Python Script Locations UserPreferences link:-
	
	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(lineedit_python_system_script_dir, prefs,
			"paths/python_system_script_dir", toolbutton_reset_python_system_script_dir);
	link_dir_chooser_button(toolbutton_choose_python_system_script_dir, lineedit_python_system_script_dir);

	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(lineedit_python_user_script_dir, prefs,
			"paths/python_user_script_dir", toolbutton_reset_python_user_script_dir);
	link_dir_chooser_button(toolbutton_choose_python_user_script_dir, lineedit_python_user_script_dir);
	
	
	// Recent Sessions UserPreferences link:-
	
	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(spinbox_recent_sessions_max_size, prefs,
			"session/recent/max_size", toolbutton_reset_recent_sessions_settings);
	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(checkbox_auto_save_session_on_quit, prefs,
			"session/auto_save_on_quit", toolbutton_reset_recent_sessions_settings);
	
}


