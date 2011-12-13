/* $Id: PreferencesPaneFiles.cc 12626 2011-12-09 06:51:06Z mchin $ */

/**
 * \file 
 * $Revision: 12626 $
 * $Date: 2011-12-09 17:51:06 +1100 (Fri, 09 Dec 2011) $ 
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

#include "PreferencesPanePython.h"

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



GPlatesQtWidgets::PreferencesPanePython::PreferencesPanePython(
		GPlatesAppLogic::ApplicationState &app_state,
		QWidget *parent_):
	QWidget(parent_)
{
	setupUi(this);
	GPlatesAppLogic::UserPreferences &prefs = app_state.get_user_preferences();
	
	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(python_home, prefs,
			"python/python_home", reset_python_home);
	link_dir_chooser_button(python_home_button, python_home);
		
	// Python Script Locations UserPreferences link:-
	
	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(lineedit_python_system_script_dir, prefs,
			"paths/python_system_script_dir", toolbutton_reset_python_system_script_dir);
	link_dir_chooser_button(toolbutton_choose_python_system_script_dir, lineedit_python_system_script_dir);

	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(lineedit_python_user_script_dir, prefs,
			"paths/python_user_script_dir", toolbutton_reset_python_user_script_dir);
	link_dir_chooser_button(toolbutton_choose_python_user_script_dir, lineedit_python_user_script_dir);
	
	
	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(show_python_fail_dlg, prefs,
			"python/show_python_init_fail_dialog", reset_python_home);
	
}


