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
 
#ifndef GPLATES_QTWIDGETS_PREFERENCESPANEKINEMATICGRAPHS_H
#define GPLATES_QTWIDGETS_PREFERENCESPANEKINEMATICGRAPHS_H

#include <QWidget>

#include "ui_PreferencesPaneKinematicGraphsUi.h"

// TODO: Add suitable explanatory labels - check how the other panes are done.

namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesQtWidgets
{

	class KinematicGraphsConfigurationWidget;

	/**
	 * This preference pane provides the controls for various preference settings available
	 * in GPlates via GPlatesAppLogic::UserPreferences. It is embedded inside the PreferencesDialog.
	 *
	 * This one holds all settings related to the KinematicGraphs - default settings for
	 * velocity calculations etc.
	 *
	 * This class was based on the other PreferencesPane...classes, specifically the PreferencesPaneFiles class.
	 *
	 * To add a new preference category, see the class comment of PreferencesDialog.
	 */
	class PreferencesPaneKinematicGraphs:
			public QWidget,
			protected Ui_PreferencesPaneKinematicGraphs
	{
		Q_OBJECT
		
	public:

		explicit
		PreferencesPaneKinematicGraphs(
				GPlatesAppLogic::ApplicationState &app_state,
				QWidget *parent_ = NULL);


		virtual
		~PreferencesPaneKinematicGraphs()
		{  }

	private:

		KinematicGraphsConfigurationWidget *d_configuration_widget;
		
	};
}

#endif  // GPLATES_QTWIDGETS_PREFERENCESPANKINEMATICGRAPHS_H

