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
 
#ifndef GPLATES_QTWIDGETS_FEATUREPROPERTIESDIALOG_H
#define GPLATES_QTWIDGETS_FEATUREPROPERTIESDIALOG_H

#include <QDialog>

#include "ui_FeaturePropertiesDialogUi.h"

#include "EditFeaturePropertiesWidget.h"
#include "GPlatesDialog.h"
#include "QueryFeaturePropertiesWidget.h"
#include "ViewFeatureGeometriesWidget.h"

#include "gui/FeatureFocus.h"

#include "model/FeatureHandle.h"


namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class ChangeFeatureTypeDialog;

	class FeaturePropertiesDialog :
			public GPlatesDialog,
			protected Ui_FeaturePropertiesDialog
	{
		Q_OBJECT
		
	public:

		explicit
		FeaturePropertiesDialog(
				GPlatesPresentation::ViewState &view_state_,
				QWidget *parent_ = NULL);

		virtual
		~FeaturePropertiesDialog()
		{  }

	public Q_SLOTS:

		/**
		 * Display the given feature, which may or may not be different
		 * to the previous feature viewed.
		 */
		void
		display_feature(
				GPlatesGui::FeatureFocus &feature_focus);
		
		/**
		 * Update the current display from whatever feature the dialog
		 * was last viewing.
		 *
		 * This gets called from display_feature(), and checks to see if the
		 * internal d_feature_ref is valid before calling for widgets to update
		 * themselves. If it is invalid, refresh_display() will disable the tab
		 * interface and the widgets contained within.
		 */
		void
		refresh_display();
		
		void
		choose_query_widget_and_open();

		void
		choose_edit_widget_and_open();

		void
		choose_geometries_widget_and_open();

		/**
		 * We need to reimplement setVisible() because reimplementing closeEvent() is not
		 * enough - the default buttonbox "Close" button only appears to hide the dialog.
		 */
		void
		setVisible(
				bool visible);
	
	private Q_SLOTS:
		
		void
		handle_tab_change(
				int index);

		void
		pop_up_change_feature_type_dialog();

	private:
		
		/**
		 * The Feature observed by the dialog.
		 * Note that this could become invalid at any time.
		 * FeaturePropertiesDialog will check for this and disable widgets if necessary
		 * in the refresh_display() method.
		 */
		GPlatesModel::FeatureHandle::weak_ref d_feature_ref;

		/**
		 * The @a ReconstructedFeatureGeometry associated with the feature that is in focus.
		 */
		GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type d_focused_rg;

		QueryFeaturePropertiesWidget *d_query_feature_properties_widget;
		EditFeaturePropertiesWidget *d_edit_feature_properties_widget;
		ViewFeatureGeometriesWidget *d_view_feature_geometries_widget;

		/**
		 * Allows the user to change the feature type of the currently selected
		 * feature and also fix up any geometry properties that are no longer valid.
		 */
		ChangeFeatureTypeDialog *d_change_feature_type_dialog;
	};
}

#endif  // GPLATES_QTWIDGETS_FEATUREPROPERTIESDIALOG_H
