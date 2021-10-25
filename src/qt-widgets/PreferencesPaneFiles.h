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
 
#ifndef GPLATES_QTWIDGETS_PREFERENCESPANEFILES_H
#define GPLATES_QTWIDGETS_PREFERENCESPANEFILES_H

#include <QWidget>

#include "gui/ConfigGuiUtils.h"

#include "PreferencesPaneFilesUi.h"


namespace GPlatesAppLogic
{
	// Just to pass in UserPreferences, really - but there's potential for other stuff I guess.
	class ApplicationState;
}

namespace GPlatesQtWidgets
{
	/**
	 * This preference pane provides the controls for various preference settings available
	 * in GPlates via GPlatesAppLogic::UserPreferences. It is embedded inside the PreferencesDialog.
	 *
	 * This one holds all settings related to the Files - default paths and the like.
	 *
	 * Ideally, no actual file configuration will happen in this class; it only deals
	 * with presenting a user-friendly layout of controls. If something has to happen when a
	 * setting gets changed, get a separate class to listen to UserPreferences and respond if
	 * the key is updated. If something checks a preference before doing some operation, it
	 * should just check the appropriate key, not look here. If a preference needs some special
	 * intelligence to select a default, make it a "magic" preference in UserPreferences.cc.
	 *
	 * To add a new preference category, see the class comment of PreferencesDialog.
	 */
	class PreferencesPaneFiles: 
			public QWidget,
			protected Ui_PreferencesPaneFiles
	{
		Q_OBJECT
		
	public:

		enum FileBehaviour
		{
			ALWAYS_DEFAULT_BEHAVIOUR,
			DEFAULT_THEN_LAST_USED_BEHAVIOUR,
			ALWAYS_LAST_USED_BEHAVIOUR,

			NUM_BEHAVIOURS
		};

		static const GPlatesGui::ConfigGuiUtils::ConfigButtonGroupAdapter::button_enum_to_description_map_type &
		build_file_behaviour_description_map()
		{
			static GPlatesGui::ConfigGuiUtils::ConfigButtonGroupAdapter::button_enum_to_description_map_type map;
			//TODO: settle on decent names for these prior to 2.0; consider also if 3 options is overkill.
			map[ALWAYS_DEFAULT_BEHAVIOUR] = "Always_default";
			map[DEFAULT_THEN_LAST_USED_BEHAVIOUR] = "Default_then_last_used";
			map[ALWAYS_LAST_USED_BEHAVIOUR] = "Always_last_used";
			return map;
		}

		explicit
		PreferencesPaneFiles(
				GPlatesAppLogic::ApplicationState &app_state,
				QWidget *parent_ = NULL);


		virtual
		~PreferencesPaneFiles()
		{  }
		
	};
}

#endif  // GPLATES_QTWIDGETS_PREFERENCESPANEFILES_H

