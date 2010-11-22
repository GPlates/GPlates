/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_MANAGEFEATURECOLLECTIONSSTATEWIDGET_H
#define GPLATES_QTWIDGETS_MANAGEFEATURECOLLECTIONSSTATEWIDGET_H

#include <QWidget>

#include "ManageFeatureCollectionsStateWidgetUi.h"

#include "app-logic/FeatureCollectionFileState.h"


namespace GPlatesQtWidgets
{
	class ManageFeatureCollectionsDialog;
	
	
	class ManageFeatureCollectionsStateWidget:
			public QWidget, 
			protected Ui_ManageFeatureCollectionsStateWidget
	{
		Q_OBJECT
		
	public:
		explicit
		ManageFeatureCollectionsStateWidget(
				ManageFeatureCollectionsDialog &feature_collections_dialog,
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref,
				bool active,
				bool enable,
				QWidget *parent_ = NULL);

		/**
		 * Updates the state of StateWidget by checking/unchecking checkboxes and
		 * disabling/enabling buttons as necessary.
		 */
		void
		update_state(
				bool active,
				bool enable);
	
		GPlatesAppLogic::FeatureCollectionFileState::file_reference
		get_file_reference() const
		{
			return d_file_reference;
		}
	
	private slots:
		
		void
		handle_active_checked(
				bool checked);

		void
		handle_active_toggled();
	
	private:
	
		/**
		 * Reconfigures the button icon, tooltip etc. to indicate state.
		 */
		void
		set_button_properties(
				bool is_active,
				bool is_enabled);
	
		ManageFeatureCollectionsDialog &d_feature_collections_dialog;
		GPlatesAppLogic::FeatureCollectionFileState::file_reference d_file_reference;
		
	};
}

#endif  // GPLATES_QTWIDGETS_MANAGEFEATURECOLLECTIONSSTATEWIDGET_H
