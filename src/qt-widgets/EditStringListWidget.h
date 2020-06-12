/* $Id$ */

/**
 * \file 
 * Contains the definition of class EditStringListWidget.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_EDITSTRINGLIST_H
#define GPLATES_QTWIDGETS_EDITSTRINGLIST_H

#include <boost/intrusive_ptr.hpp>

#include "AbstractEditWidget.h"
#include "EditTableWidget.h"
#include "ui_EditStringListWidgetUi.h"

#include "property-values/GpmlStringList.h"



namespace GPlatesQtWidgets
{
	class EditTableActionWidget;

	class EditStringListWidget:
			public AbstractEditWidget,
			public EditTableWidget,
			protected Ui_EditStringListWidget
	{
		Q_OBJECT
	public:
		explicit
		EditStringListWidget(
				QWidget *parent_ = NULL);

		/**
		 * Clear the widget contents and any data structures.
		 *
		 * This is used by the constructor to initialise the widget.
		 */
		virtual
		void
		reset_widget_to_default_values();

		/**
		 * Update the widget contents from @a gpml_string_list.
		 *
		 * This runs through the string list, appending each element to the table.
		 */
		void
		update_widget_from_string_list(
				GPlatesPropertyValues::GpmlStringList &gpml_string_list);

		/**
		 * Create a new instance of a property-value derivation, based upon the widget
		 * contents.
		 */
		virtual
		GPlatesModel::PropertyValue::non_null_ptr_type
		create_property_value_from_widget() const;

		/**
		 * Update the property-value instance from which this widget was populated, with
		 * the values currently in this widget.
		 */
		virtual
		bool
		update_property_value_from_widget();

		virtual
		void
		handle_insert_row_above(
				const EditTableActionWidget *action_widget);

		virtual
		void
		handle_insert_row_below(
				const EditTableActionWidget *action_widget);
	
		virtual
		void
		handle_delete_row(
				const EditTableActionWidget *action_widget);
	
		/**
		 * Append a new string element to the table.
		 *
		 * This function is used by @a update_widget_from_string_list.
		 *
		 * This function will also scroll the table to the newly-appended element.
		 */
		void
		append_string_element(
				const QString &str);

	private Q_SLOTS:
	
		/**
		 * Handle a change to the data contained in a cell.
		 *
		 * This is currently connected to the QTableWidget signal cellActivated:
		 * http://doc.trolltech.com/4.3/qtablewidget.html#cellActivated
		 * The reasons are provided in a comment in the constructor.
		 */
		void
		handle_cell_changed(
				int row,
				int column);

		void
		handle_append_element_button_clicked();

		/**
		 * Handle a change to which cell has the focus.
		 *
		 * This is currently connected to the QTableWidget signal currentCellChanged:
		 * http://doc.trolltech.com/4.3/qtablewidget.html#currentCellChanged
		 */
		void
		handle_current_cell_changed(
				int currentRow,
				int currentColumn,
				int previousRow,
				int previousColumn);

		/**
		 * Handle when a cell is activated.
		 *
		 * (This function is not currently invoked by any code.)
		 */
		void
		handle_cell_activated(int row, int column);

	private:
		
		/**
		 * Finds the current table row associated with the EditTableActionWidget.
		 * Will return -1 in the event that the action widget provided is not in
		 * the table.
		 */
		int
		get_row_for_action_widget(
				const EditTableActionWidget *action_widget);
		
		/**
		 * Insert a new empty string to the table.
		 *
		 * The new row will be inserted to take the place of row index @a row.
		 */
		void
		insert_empty_string_element_into_table(
				int row);
		
		/**
		 * Removes a single row from the table.
		 */
		void
		delete_row(
				int row);

		/**
		 * Commit changes, by emitting the commit signal.
		 */
		void
		commit_changes();

		/**
		 * This boost::intrusive_ptr is used to remember the property value which
		 * was last loaded into this editing widget. This is done so that the
		 * edit widget can directly update the property value later.
		 *
		 * We need to use a reference-counting pointer to make sure the property
		 * value doesn't disappear while this edit widget is active; however, since
		 * the property value is not known at the time the widget is created,
		 * the pointer may be NULL and this must be checked for.
		 *
		 * The pointer will also be NULL when the edit widget is being used for
		 * adding brand new properties to the model.
		 */
		boost::intrusive_ptr<GPlatesPropertyValues::GpmlStringList> d_string_list_ptr;
	

	};
}

#endif  // GPLATES_QTWIDGETS_EDITSTRINGLISTWIDGET_H

