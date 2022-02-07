/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
 * Copyright (C) 2016 Geological Survey of Norway
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
	groupbox_scripting->setVisible(false);

	QFont font_ = label_feature_collections->font();
	font_.setBold(true);
	label_feature_collections->setFont(font_);

	font_ = label_projects->font();
	font_.setBold(true);
	label_projects->setFont(font_);

	font_ = label_export->font();
	font_.setBold(true);
	label_export->setFont(font_);

	GPlatesAppLogic::UserPreferences &prefs = app_state.get_user_preferences();

	// Creating a QButtonGroup in Designer may not be immediately obvious by the way...select the button(s) you wish to belong to the
	// button group, right click and select "Assign to button group",

	// Associate radio buttons with enum ids - feature collection buttons
	buttongroup_feature_collections_behaviour->setId(radio_feature_collections_always_default,ALWAYS_DEFAULT_BEHAVIOUR);
	buttongroup_feature_collections_behaviour->setId(radio_feature_collections_default_then_last_used,DEFAULT_THEN_LAST_USED_BEHAVIOUR);
	buttongroup_feature_collections_behaviour->setId(radio_feature_collections_always_last_used,ALWAYS_LAST_USED_BEHAVIOUR);

	// Associate radio buttons with enum ids - project file buttons
	buttongroup_projects_behaviour->setId(radio_projects_always_default,ALWAYS_DEFAULT_BEHAVIOUR);
	buttongroup_projects_behaviour->setId(radio_projects_default_then_last_used,DEFAULT_THEN_LAST_USED_BEHAVIOUR);
	buttongroup_projects_behaviour->setId(radio_projects_always_last_used,ALWAYS_LAST_USED_BEHAVIOUR);
	
	// Loading and Saving UserPreferences link:-
	// Feature collections:

	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(lineedit_default_feature_collection_dir, prefs,
			"paths/default_feature_collection_dir", toolbutton_reset_default_feature_collection_dir);
	link_dir_chooser_button(toolbutton_choose_default_feature_collection_dir, lineedit_default_feature_collection_dir);

	GPlatesGui::ConfigGuiUtils::link_button_group_to_preference(buttongroup_feature_collections_behaviour,
																prefs,
																"paths/feature_collection_behaviour",
																build_file_behaviour_description_map(),
																0);

	// Loading and Saving UserPreferences link:-
	// Projects:

	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(lineedit_default_project_dir, prefs,
			"paths/default_project_dir", toolbutton_reset_default_project_dir);
	link_dir_chooser_button(toolbutton_choose_default_project_dir, lineedit_default_project_dir);

	GPlatesGui::ConfigGuiUtils::link_button_group_to_preference(buttongroup_projects_behaviour,
																prefs,
																"paths/project_behaviour",
																build_file_behaviour_description_map(),
																0);

	// Loading and Saving UserPreferences link:-
	// Exports:
	
	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(lineedit_default_export_dir, prefs,
			"paths/default_export_dir", toolbutton_reset_default_export_dir);
	link_dir_chooser_button(toolbutton_choose_default_export_dir, lineedit_default_export_dir);

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


