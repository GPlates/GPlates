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
#include "FeaturePropertiesDialogUi.h"

#include "gui/FeatureFocus.h"
#include "model/FeatureHandle.h"
#include "QueryFeaturePropertiesWidget.h"
#include "EditFeaturePropertiesWidget.h"
#include "ViewFeatureGeometriesWidget.h"


namespace GPlatesQtWidgets
{
	class ViewportWindow;

	class FeaturePropertiesDialog:
			public QDialog,
			protected Ui_FeaturePropertiesDialog
	{
		Q_OBJECT
		
	public:
		explicit
		FeaturePropertiesDialog(
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				GPlatesGui::FeatureFocus &feature_focus,
				QWidget *parent_ = NULL);

		virtual
		~FeaturePropertiesDialog()
		{  }

	public slots:

		/**
		 * Display the given feature, which may or may not be different
		 * to the previous feature viewed.
		 */
		void
		display_feature(
				GPlatesModel::FeatureHandle::weak_ref feature_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type);
		
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
		setVisible(bool visible);

	signals:
	
	private slots:
		
		void
		handle_tab_change(
				int index);

	private:
		
		/**
		 * The Feature observed by the dialog.
		 * Note that this could become invalid at any time.
		 * FeaturePropertiesDialog will check for this and disable widgets if necessary
		 * in the refresh_display() method.
		 */
		GPlatesModel::FeatureHandle::weak_ref d_feature_ref;

		GPlatesQtWidgets::QueryFeaturePropertiesWidget *d_query_feature_properties_widget;
		GPlatesQtWidgets::EditFeaturePropertiesWidget *d_edit_feature_properties_widget;
		GPlatesQtWidgets::ViewFeatureGeometriesWidget *d_view_feature_geometries_widget;
	};
}

#endif  // GPLATES_QTWIDGETS_FEATUREPROPERTIESDIALOG_H
