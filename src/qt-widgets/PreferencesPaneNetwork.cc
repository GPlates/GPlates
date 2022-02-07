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

#include "PreferencesPaneNetwork.h"

#include "QtWidgetUtils.h"
#include "app-logic/ApplicationState.h"
#include "app-logic/UserPreferences.h"
#include "gui/ConfigGuiUtils.h"


GPlatesQtWidgets::PreferencesPaneNetwork::PreferencesPaneNetwork(
		GPlatesAppLogic::ApplicationState &app_state,
		QWidget *parent_):
	QWidget(parent_)
{
	setupUi(this);
	GPlatesAppLogic::UserPreferences &prefs = app_state.get_user_preferences();
	
	// Network Proxy UserPreferences link:-
	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(lineedit_proxy_url, prefs,
			"net/proxy/url", toolbutton_reset_proxy);
	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(checkbox_use_proxy, prefs,
			"net/proxy/enabled", toolbutton_reset_proxy);
	// Proxy URL only available if 'enabled' is checked. Note use of 'toggled' over 'clicked'.
	// This is so that the when net/proxy/enabled changes, the checkbox changes and the
	// line edit is disabled/enabled appropriately.
	connect(checkbox_use_proxy, SIGNAL(toggled(bool)), lineedit_proxy_url, SLOT(setEnabled(bool)));

	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(port_spinbox, prefs,
		"net/server/port", reset_port_button);
	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(listen_local_checkbox, prefs,
		"net/server/local",reset_port_button);
	
}


