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
 
#ifndef GPLATES_QTWIDGETS_EDITFEATUREPROPERTIESWIDGET_H
#define GPLATES_QTWIDGETS_EDITFEATUREPROPERTIESWIDGET_H

#include <QWidget>
#include <boost/optional.hpp>
#include "EditFeaturePropertiesWidgetUi.h"
#include "qt-widgets/AddPropertyDialog.h"
#include "qt-widgets/EditWidgetGroupBox.h"
#include "gui/FeatureFocus.h"
#include "gui/FeaturePropertyTableModel.h"
#include "model/FeatureHandle.h"


namespace GPlatesQtWidgets
{
	class EditFeaturePropertiesWidget: 
			public QWidget,
			protected Ui_EditFeaturePropertiesWidget 
	{
		Q_OBJECT
		
	public:
		explicit
		EditFeaturePropertiesWidget(
				GPlatesGui::FeatureFocus &feature_focus,
				QWidget *parent_ = NULL);

		virtual
		~EditFeaturePropertiesWidget()
		{  }
		
		GPlatesGui::FeaturePropertyTableModel &
		model()
		{
			return *d_property_model_ptr;
		}


	public slots:

		/**
		 * Updates the dialog to display and edit a new Feature.
		 * Any changes that might be uncommited from the previous Feature will be discarded.
		 *
		 * Called by FeaturePropertiesDialog after the weak_ref is checked for validity.
		 */
		void
		edit_feature(
				GPlatesModel::FeatureHandle::weak_ref feature_ref);

		/**
		 * Used heavily internally, and in one situation by the parent FeaturePropertiesDialog.
		 * Causes any leftover data in line edits, spinboxes etc. to be committed.
		 */
		void
		commit_and_clean_up();

	private slots:
			
		void
		commit_edit_widget_data();

		void
		handle_model_change();

		void
		handle_selection_change(
				const QItemSelection &selected,
				const QItemSelection &deselected);

		void
		delete_selected_property();
		
	private:
		
		void
		set_up_edit_widgets();

		GPlatesGui::FeatureFocus *d_feature_focus_ptr;
	
		GPlatesGui::FeaturePropertyTableModel *d_property_model_ptr;
		GPlatesModel::FeatureHandle::weak_ref d_feature_ref;
		GPlatesQtWidgets::EditWidgetGroupBox *d_edit_widget_group_box_ptr;
		GPlatesQtWidgets::AddPropertyDialog *d_add_property_dialog_ptr;

		/**
		 * Used to remember which property is being edited by the currently-active
		 * Edit widget, so that data can be committed when editing is finished.
		 */
		boost::optional<GPlatesModel::FeatureHandle::properties_iterator> d_selected_property_iterator;
	};
}

#endif  // GPLATES_QTWIDGETS_EDITFEATUREPROPERTIESWIDGET_H
