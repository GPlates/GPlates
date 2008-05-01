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
#include "gui/FeaturePropertyTableModel.h"
#include "qt-widgets/EditWidgetGroupBox.h"


namespace GPlatesQtWidgets
{
	class AddPropertyDialog: 
			public QDialog,
			protected Ui_AddPropertyDialog 
	{
		Q_OBJECT
		
	public:
		explicit
		AddPropertyDialog(
				GPlatesGui::FeaturePropertyTableModel &property_model,
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

		GPlatesGui::FeaturePropertyTableModel *d_property_model_ptr;
		GPlatesQtWidgets::EditWidgetGroupBox *d_edit_widget_group_box_ptr;
	};
}

#endif  // GPLATES_QTWIDGETS_ADDPROPERTYDIALOG_H
