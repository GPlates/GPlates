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

#include "PreferencesPaneView.h"

#include "QtWidgetUtils.h"
#include "app-logic/ApplicationState.h"
#include "app-logic/UserPreferences.h"
#include "gui/ConfigGuiUtils.h"


GPlatesQtWidgets::PreferencesPaneView::PreferencesPaneView(
		GPlatesAppLogic::ApplicationState &app_state,
		QWidget *parent_):
	QWidget(parent_)
{
	setupUi(this);
	GPlatesAppLogic::UserPreferences &prefs = app_state.get_user_preferences();
	
	// View Time UserPreferences link:-
	
	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(spinbox_time_range_start, prefs,
			"view/animation/default_time_range_start", toolbutton_reset_time_range);
	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(spinbox_time_range_end, prefs,
			"view/animation/default_time_range_end", toolbutton_reset_time_range);
	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(spinbox_time_range_increment, prefs,
			"view/animation/default_time_increment", toolbutton_reset_time_range);

	// Misc view options link:-
	
	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(checkbox_show_stars, prefs,
			"view/show_stars", NULL);		// Not much point to a 'reset' button here.
	
}


