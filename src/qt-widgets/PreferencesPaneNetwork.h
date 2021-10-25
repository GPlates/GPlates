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
 
#ifndef GPLATES_QTWIDGETS_PREFERENCESPANENETWORK_H
#define GPLATES_QTWIDGETS_PREFERENCESPANENETWORK_H

#include <QWidget>

#include "ui_PreferencesPaneNetworkUi.h"


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
	 * This one holds all settings related to the Network.
	 *
	 * Ideally, no actual network configuration will happen in this class; it only deals
	 * with presenting a user-friendly layout of controls. If something has to happen when a
	 * setting gets changed, get a separate class to listen to UserPreferences and respond if
	 * the key is updated. If something checks a preference before doing some operation, it
	 * should just check the appropriate key, not look here. If a preference needs some special
	 * intelligence to select a default, make it a "magic" preference in UserPreferences.cc.
	 *
	 * To add a new preference category, see the class comment of PreferencesDialog.
	 */
	class PreferencesPaneNetwork: 
			public QWidget,
			protected Ui_PreferencesPaneNetwork
	{
		Q_OBJECT
		
	public:

		explicit
		PreferencesPaneNetwork(
				GPlatesAppLogic::ApplicationState &app_state,
				QWidget *parent_ = NULL);


		virtual
		~PreferencesPaneNetwork()
		{  }
		
	};
}

#endif  // GPLATES_QTWIDGETS_PREFERENCESPANENETWORK_H

