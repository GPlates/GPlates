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


namespace GPlatesPresentation
{
	class ViewState;
}

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
				GPlatesPresentation::ViewState &view_state_,
				QWidget *parent_ = NULL);

		virtual
		~EditFeaturePropertiesWidget()
		{
			// The view does not take ownership of the model.
			// See http://doc.trolltech.com/4.4/qabstractitemview.html#setModel
			delete d_property_model_ptr;
		}
		
		GPlatesGui::FeaturePropertyTableModel &
		model()
		{
			return *d_property_model_ptr;
		}

		/**
		 * Called by AddPropertyDialog to perform the actual model magic.
		 */
		void
		append_property_value_to_feature(
				GPlatesModel::PropertyValue::non_null_ptr_type property_value,
				const GPlatesModel::PropertyName &property_name);

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
		 * Call this to blank edit widgets and get ready for the next feature.
		 */
		void
		clean_up();

		/**
		 * Causes any leftover data in line edits, spinboxes etc. to be committed.
		 */
		void
		commit_edit_widget_data();

	private slots:
		
		/**
		 * Wipes the EditFeaturePropertiesWidget clean without causing any leftover
		 * data to be commited (as that feature no longer exists).
		 */
		void
		handle_feature_deletion();

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

		/**
		 * This is the feature focus which tracks changes to the currently focused feature.
		 */
		GPlatesGui::FeatureFocus *d_feature_focus_ptr;
		
		GPlatesGui::FeaturePropertyTableModel *d_property_model_ptr;
		GPlatesModel::FeatureHandle::weak_ref d_feature_ref;
		GPlatesQtWidgets::EditWidgetGroupBox *d_edit_widget_group_box_ptr;
		GPlatesQtWidgets::AddPropertyDialog *d_add_property_dialog_ptr;

		/**
		 * Used to remember which property is being edited by the currently-active
		 * Edit widget, so that data can be committed when editing is finished.
		 */
		boost::optional<GPlatesModel::FeatureHandle::children_iterator> d_selected_property_iterator;
	};
}

#endif  // GPLATES_QTWIDGETS_EDITFEATUREPROPERTIESWIDGET_H
