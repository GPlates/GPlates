/**
 * \file
 * $Revision: 12148 $
 * $Date: 2011-08-18 14:01:47 +0200 (Thu, 18 Aug 2011) $
 *
 * Copyright (C) 2015 Geological Survey of Norway
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


#include "PreferencesPaneKinematicGraphs.h"

#include "KinematicGraphsConfigurationWidget.h"
#include "QtWidgetUtils.h"
#include "app-logic/ApplicationState.h"
#include "app-logic/UserPreferences.h"
#include "gui/ConfigGuiUtils.h"




GPlatesQtWidgets::PreferencesPaneKinematicGraphs::PreferencesPaneKinematicGraphs(
		GPlatesAppLogic::ApplicationState &app_state,
		QWidget *parent_):
	QWidget(parent_),
	d_configuration_widget(new KinematicGraphsConfigurationWidget())
{
	setupUi(this);

	QGridLayout *layout_ = new QGridLayout(placeholder_widget);
	layout_->addWidget(d_configuration_widget);

	GPlatesAppLogic::UserPreferences &prefs = app_state.get_user_preferences();

	// TODO: Add reset button - this will need to be in the Widget, and hence also usable from the KinematicGraphs dialog's settings.
	// TODO: Settle on key names before 1.5
	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(d_configuration_widget->delta_time_spinbox(), prefs,
														  "tools/kinematics/velocity_delta_time",0);

	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(d_configuration_widget->velocity_yellow_spinbox(), prefs,
                                                          "tools/kinematics/velocity_warning_1",0);

	GPlatesGui::ConfigGuiUtils::link_widget_to_preference(d_configuration_widget->velocity_red_spinbox(), prefs,
                                                          "tools/kinematics/velocity_warning_2",0);

	GPlatesGui::ConfigGuiUtils::link_button_group_to_preference(d_configuration_widget->velocity_method_button_group(), prefs,
																"tools/kinematics/velocity_method",0);

}


