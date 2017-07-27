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

#ifndef GPLATES_GUI_FILE_IO_DIRECTORY_CONFIGURATIONS_H
#define GPLATES_GUI_FILE_IO_DIRECTORY_CONFIGURATIONS_H

#include "boost/noncopyable.hpp"

#include "app-logic/UserPreferences.h"
#include "qt-widgets/PreferencesPaneFiles.h"

namespace GPlatesPresentation
{
    class ViewState;
}

namespace GPlatesGui
{
    class DirectoryConfiguration
    {
    public:

	DirectoryConfiguration(
		GPlatesAppLogic::UserPreferences &prefs,
		const QString &default_key_string,
		const QString &last_used_key_string,
		const QString &behaviour_key_string
		);
	void
	initialise_from_preferences();

	const QString &
	directory() const;

	void
	update_last_used_directory(
		const QString &directory);

	const QString &
	last_used_directory() const;

    private:

	GPlatesAppLogic::UserPreferences& d_prefs;
	QString d_default_key_string;
	QString d_last_used_key_string;
	QString d_behaviour_key_string;

	QString d_default_directory;
	QString d_last_used_directory;
	QString d_last_used_directory_from_prefs;
	GPlatesQtWidgets::PreferencesPaneFiles::FileBehaviour d_behaviour;
	bool d_first_use;
    };

    class FileIODirectoryConfigurations:
	    private boost::noncopyable
    {
    public:
	FileIODirectoryConfigurations(
		GPlatesPresentation::ViewState &view_state);

	DirectoryConfiguration &
	feature_collection_configuration();

	DirectoryConfiguration &
	project_configuration();

    private:

	void
	initialise_from_user_preferences();

	DirectoryConfiguration d_feature_collection_configuration;

	DirectoryConfiguration d_project_configuration;
    };
}

#endif // GPLATES_GUI_FILE_IO_DIRECTORY_CONFIGURATIONS_H
