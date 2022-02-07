/**
 * \file
 * $Revision$
 * $Date$
 *
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
#include <QDir>

#include "app-logic/ApplicationState.h"
#include "app-logic/UserPreferences.h"
#include "presentation/ViewState.h"
#include "qt-widgets/PreferencesPaneFiles.h"
#include "FileIODirectoryConfigurations.h"

GPlatesGui::FileIODirectoryConfigurations::FileIODirectoryConfigurations(
		GPlatesPresentation::ViewState &view_state_ref):
	d_feature_collection_configuration(
		view_state_ref.get_application_state().get_user_preferences(),
		"paths/default_feature_collection_dir",
		"paths/last_used_feature_collection_dir",
		"paths/feature_collection_behaviour"),
	d_project_configuration(
		view_state_ref.get_application_state().get_user_preferences(),
		"paths/default_project_dir",
		"paths/last_used_project_dir",
		"paths/project_behaviour")
{
	initialise_from_user_preferences();
}

void
GPlatesGui::FileIODirectoryConfigurations::initialise_from_user_preferences()
{
	d_feature_collection_configuration.initialise_from_preferences();
	d_project_configuration.initialise_from_preferences();
}

GPlatesGui::DirectoryConfiguration &
GPlatesGui::FileIODirectoryConfigurations::feature_collection_configuration()
{
	return d_feature_collection_configuration;
}

GPlatesGui::DirectoryConfiguration &
GPlatesGui::FileIODirectoryConfigurations::project_configuration()
{
	return d_project_configuration;
}

const QString &
GPlatesGui::DirectoryConfiguration::directory() const
{
#if 0
	qDebug() << "directory:";
	qDebug() << "default: " << d_default_directory;
	qDebug() << "last_used: " << d_last_used_directory;
	qDebug() << "last_used_from_prefs: " << d_last_used_directory_from_prefs;
	qDebug() << "behaviour: " << d_behaviour;
#endif
	switch(d_behaviour)
	{
	case GPlatesQtWidgets::PreferencesPaneFiles::ALWAYS_DEFAULT_BEHAVIOUR:
		return d_default_directory;
	case GPlatesQtWidgets::PreferencesPaneFiles::DEFAULT_THEN_LAST_USED_BEHAVIOUR:
		return d_first_use ? d_default_directory : d_last_used_directory;
	case GPlatesQtWidgets::PreferencesPaneFiles::ALWAYS_LAST_USED_BEHAVIOUR:
		return d_first_use ? d_last_used_directory_from_prefs : d_last_used_directory;
	default:
		return d_default_directory;
	}
}

void
GPlatesGui::DirectoryConfiguration::update_last_used_directory(
		const QString &directory_)
{
	//qDebug() << "updating last used to :" << directory_;
	d_last_used_directory = directory_;

	d_prefs.set_value(
				d_last_used_key_string,
				d_last_used_directory);

	d_first_use = false;
}

const QString &
GPlatesGui::DirectoryConfiguration::last_used_directory() const
{
	return d_last_used_directory;
}

GPlatesGui::DirectoryConfiguration::DirectoryConfiguration(
		GPlatesAppLogic::UserPreferences &prefs,
		const QString &default_key_string,
		const QString &last_used_key_string,
		const QString &behaviour_key_string):
	d_prefs(prefs),
	d_default_key_string(default_key_string),
	d_last_used_key_string(last_used_key_string),
	d_behaviour_key_string(behaviour_key_string),
	d_last_used_directory(QDir::currentPath()),
	d_first_use(true)
{}

void
GPlatesGui::DirectoryConfiguration::initialise_from_preferences()
{
	QString behaviour_string = d_prefs.get_value(d_behaviour_key_string).toString();

	d_behaviour =
			static_cast<GPlatesQtWidgets::PreferencesPaneFiles::FileBehaviour>
			(GPlatesQtWidgets::PreferencesPaneFiles::build_file_behaviour_description_map().key(behaviour_string));

	d_last_used_directory_from_prefs = d_prefs.get_value(d_last_used_key_string).toString();
	d_default_directory = d_prefs.get_value(d_default_key_string).toString();

	d_last_used_directory = d_default_directory;
#if 0
	qDebug() << "Initialise from prefs:";
	qDebug() << "behaviour key: " << d_behaviour_key_string;
	qDebug() << "behaviour: " << d_behaviour;
	qDebug() << d_last_used_directory_from_prefs;
	qDebug() << d_default_directory;
#endif
}
