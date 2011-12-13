/* $Id: PreferencesPaneFiles.h 12626 2011-12-09 06:51:06Z mchin $ */

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
 
#ifndef GPLATES_QTWIDGETS_PREFERENCESPYTHONPANE_H
#define GPLATES_QTWIDGETS_PREFERENCESPYTHONPANE_H

#include <QWidget>

#include "PreferencesPanePythonUi.h"


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
	class PreferencesPanePython: 
			public QWidget,
			protected Ui_PreferencesPanePython
	{
		Q_OBJECT
		
	public:

		explicit
		PreferencesPanePython(
				GPlatesAppLogic::ApplicationState &app_state,
				QWidget *parent_ = NULL);


		virtual
		~PreferencesPanePython()
		{  }
		
	};
}

#endif  // GPLATES_QTWIDGETS_PREFERENCESPYTHONPANE_H

