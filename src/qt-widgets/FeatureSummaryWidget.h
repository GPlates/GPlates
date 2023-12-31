/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_FEATURESUMMARYWIDGET_H
#define GPLATES_QTWIDGETS_FEATURESUMMARYWIDGET_H

#include <QWidget>

#include "ui_FeatureSummaryWidgetUi.h"
#include "TaskPanelWidget.h"

#include "app-logic/FeatureCollectionFileState.h"
#include "model/FeatureHandle.h"
#include "presentation/ViewState.h"


// An effort to reduce the dependency spaghetti currently plaguing the GUI.
namespace GPlatesGui
{
	class FeatureFocus;
}


namespace GPlatesQtWidgets
{
	class FeatureSummaryWidget :
			public TaskPanelWidget, 
			protected Ui_FeatureSummaryWidget
	{
		Q_OBJECT
		
	public:

		explicit
		FeatureSummaryWidget(
				GPlatesPresentation::ViewState &view_state_,
				QWidget *parent_ = NULL);

		virtual
		void
		handle_activation();

		virtual
		QString
		get_clear_action_text() const;

		virtual
		bool
		clear_action_enabled() const;

		virtual
		void
		handle_clear_action_triggered();

	public Q_SLOTS:
		
		void
		clear();

		void
		display_feature(
				GPlatesGui::FeatureFocus &feature_focus);
	
	private:

		void
		hide_plate_id_fields_as_appropriate();

		/**
		 * The loaded feature collection files.
		 * We need this to look up file names from FeatureHandle weakrefs.
		 */
		GPlatesAppLogic::FeatureCollectionFileState &d_file_state;

		GPlatesGui::FeatureFocus &d_feature_focus;
	};
}

#endif  // GPLATES_QTWIDGETS_FEATURESUMMARYWIDGET_H

