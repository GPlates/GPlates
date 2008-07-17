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
 
#ifndef GPLATES_QTWIDGETS_ADDPROPERTYDIALOG_H
#define GPLATES_QTWIDGETS_ADDPROPERTYDIALOG_H

#include <QDialog>
#include "AddPropertyDialogUi.h"
#include "qt-widgets/EditWidgetGroupBox.h"


namespace GPlatesQtWidgets
{
	class EditFeaturePropertiesWidget;
	class ViewportWindow;
	
	class AddPropertyDialog: 
			public QDialog,
			protected Ui_AddPropertyDialog 
	{
		Q_OBJECT
		
	public:
		/**
		 * Constructs the Add Property Dialog instance.
		 * The reference to the EditFeaturePropertiesWidget is necessary for
		 * this dialog to add new properties; this is done purely to keep
		 * any model-editing functionality in EditFeaturePropertiesWidget.
		 */
		explicit
		AddPropertyDialog(
				GPlatesQtWidgets::EditFeaturePropertiesWidget &edit_widget,
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				QWidget *parent_ = NULL);

		virtual
		~AddPropertyDialog()
		{  }
			
	public slots:
	
		/**
		 * Resets dialog components to default state.
		 */
		void
		reset();

		/**
		 * Pops up the AddPropertyDialog as a modal dialog,
		 * after resetting itself to default values.
		 */
		void
		pop_up();

	private slots:
		
		void
		set_appropriate_property_value_type(
				int index);

		void
		set_appropriate_edit_widget();
		
		void
		add_property();
		
	private:
		
		void
		set_up_add_property_box();

		void
		set_up_edit_widgets();

		GPlatesQtWidgets::EditFeaturePropertiesWidget *d_edit_feature_properties_widget_ptr;
		const GPlatesQtWidgets::ViewportWindow *d_view_state_ptr;
		GPlatesQtWidgets::EditWidgetGroupBox *d_edit_widget_group_box_ptr;
	};
}

#endif  // GPLATES_QTWIDGETS_ADDPROPERTYDIALOG_H
