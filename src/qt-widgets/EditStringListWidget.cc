/* $Id$ */

/**
 * \file 
 * Contains the definitions of the members of class EditStringListWidget.
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

#include <vector>
#include <boost/optional.hpp>
#include <boost/none.hpp>

#include <QDebug>
#include <QHeaderView>
#include <QLocale>
#include <QMessageBox>
#include <QString>
#include <QTableWidgetItem>

#include "EditStringListWidget.h"
#include "EditTableActionWidget.h"
#include "UninitialisedEditWidgetException.h"
#include "InvalidPropertyValueException.h"

#include "utils/UnicodeStringUtils.h"


namespace
{
	enum ColumnLayout
	{
		COLUMN_ELEMENT, COLUMN_ACTION
	};
	
	
	/**
	 * Fetches the appropriate action widget given a row number.
	 * May return NULL.
	 */
	GPlatesQtWidgets::EditTableActionWidget *
	get_action_widget_for_row(
			QTableWidget &table,
			int row)
	{
		if (row < 0 || row >= table.rowCount()) {
			return NULL;
		}
		if (table.cellWidget(row, COLUMN_ACTION) == NULL) {
			return NULL;
		}
		GPlatesQtWidgets::EditTableActionWidget *action_widget =
				static_cast<GPlatesQtWidgets::EditTableActionWidget *>(
						table.cellWidget(row, COLUMN_ACTION));
		return action_widget;
	}
	
	
	/**
	 * Uses rowCount() and setRowCount() to ensure the table has at least
	 * @a rows rows available. If the table has more rows currently allocated,
	 * this function does not shrink the table.
	 *
	 * The number of rows in the table after the operation is returned.
	 */
	int
	ensure_table_size(
			QTableWidget &table,
			int rows)
	{
		if (table.rowCount() < rows) {
			table.setRowCount(rows);
		}
		return table.rowCount();
	}
	
	/**
	 * Append the string @a str to @a table.
	 *
	 * Don't emit any signals or anything:  This will be handled by the calling function when
	 * appropriate.
	 */
	void
	append_string_to_table(
			QTableWidget &table,
			const QString &str)
	{
		// Append a row to the table.
		int which_row = table.rowCount();
		table.insertRow(which_row);

		// Set the element cell.
		QTableWidgetItem *element_item = new QTableWidgetItem;
		element_item->setData(Qt::DisplayRole, str);
		table.setItem(which_row, COLUMN_ELEMENT, element_item);

		// Add the "Action" cell.
		// We need to set this as uneditable.
		QTableWidgetItem *action_item = new QTableWidgetItem;
		action_item->setFlags(0);
		table.setItem(which_row, COLUMN_ACTION, action_item);

		// Set the "current cell" (ie, the cell which has the focus) to be a cell from the
		// new row, so that an action widget is displayed.
		table.setCurrentCell(which_row, COLUMN_ACTION);
	}	


	/**
	 * Insert an empty string element at already-inserted row @a which_row of @a table.
	 *
	 * Allocates a QTableWidgetItem and populates a QTableWidget from a
	 *
	 * No checking is done to see if the table is the correct size!
	 * The caller is responsible for adding rows to the table appropriately.
	 */
	void
	populate_table_row_with_empty_string_element(
		GPlatesQtWidgets::EditStringListWidget &string_list_widget,
		QTableWidget &table,
		int which_row)
	{
		// Set the element cell.
		QTableWidgetItem *element_item = new QTableWidgetItem;
		element_item->setData(Qt::DisplayRole, QString());
		table.setItem(which_row, COLUMN_ELEMENT, element_item);

		// Add the "Action" cell.
		// We need to set this as uneditable.
		QTableWidgetItem *action_item = new QTableWidgetItem;
		action_item->setFlags(0);
		table.setItem(which_row, COLUMN_ACTION, action_item);

		// FIXME: What do these next lines mean?
		// Why are they in this function, but not the function above?

		// Creating the action_widget is not a memory leak - Qt will take ownership of
		// the action_widget memory, and clean it up when the table row is deleted.
		GPlatesQtWidgets::EditTableActionWidget *action_widget =
			new GPlatesQtWidgets::EditTableActionWidget(
			&string_list_widget, &string_list_widget);

		table.setCellWidget(which_row, COLUMN_ACTION, action_widget);
	}


	/**
	 * Work around a graphical glitch, where the EditTableActionWidgets around the
	 * recently scrolled-to row appear to be misaligned.
	 * 
	 * This graphical glitch appears most prominently when appending a point to the
	 * table, but can also appear due to auto-scrolling when inserting a new row above
	 * or below via action widget buttons, and most likely this can also happen
	 * during row deletion, so, better safe than sorry.
	 */
	void
	work_around_table_graphical_glitch(
		GPlatesQtWidgets::EditStringListWidget &edit_string_list_widget,
		QTableWidget &table)
	{
		static const GPlatesQtWidgets::EditTableActionWidget dummy(&edit_string_list_widget, NULL);
		table.horizontalHeader()->resizeSection(COLUMN_ACTION, dummy.width() + 1);
		table.horizontalHeader()->resizeSection(COLUMN_ACTION, dummy.width());
	}	


	inline
	const boost::optional<QString>
	get_element_string(
			QTableWidget &table_widget,
			int row)
	{
		QTableWidgetItem* item = table_widget.item(row, COLUMN_ELEMENT);
		if (item == NULL) {
			return boost::none;
		}
		return boost::optional<QString>(item->text());
	}


	void
	populate_element_vector_from_table(
			std::vector<GPlatesPropertyValues::TextContent> &elements,
			QTableWidget &table_elements)
	{
		int table_row_count = table_elements.rowCount();
		for (int i = 0; i < table_row_count; ++i) {
			boost::optional<QString> elem_str = get_element_string(table_elements, i);
			if ( ! elem_str) {
				// For some reason no item existed at this row index.
				continue;
			}
			// We retain strings even if they're empty, since an empty string is still
			// a valid string, and there may be a reason why a user wants an empty
			// string in the list.
			GPlatesUtils::UnicodeString ustr = GPlatesUtils::make_icu_string_from_qstring(*elem_str);
			elements.push_back(GPlatesPropertyValues::TextContent(ustr));
		}
	}
}


GPlatesQtWidgets::EditStringListWidget::EditStringListWidget(
		QWidget *parent_):
	AbstractEditWidget(parent_),
	EditTableWidget()
{
	setupUi(this);

	// Set column widths and resizabilty.
	EditTableActionWidget dummy(this, NULL);
	table_elements->horizontalHeader()->setResizeMode(COLUMN_ELEMENT, QHeaderView::Stretch);
	table_elements->horizontalHeader()->setResizeMode(COLUMN_ACTION, QHeaderView::Fixed);
	table_elements->horizontalHeader()->resizeSection(COLUMN_ACTION, dummy.width());
	table_elements->horizontalHeader()->setMovable(true);
	// Set up a minimum row height as well, for the action widgets' sake.
	table_elements->verticalHeader()->setDefaultSectionSize(dummy.height());

	// Clear spinboxes and things.
	reset_widget_to_default_values();

	QObject::connect(button_append_element, SIGNAL(clicked()),
			this, SLOT(handle_append_element_button_clicked()));

	// FIXME: Find the right signal to look for. This one (cellActivated) kinda works,
	// but what happens is, user changes value, user hits enter, value goes in cell,
	// user hits enter again, cellActivated(). We need something better - but cellChanged()
	// fires when we're populating the table...

	QObject::connect(table_elements, SIGNAL(cellActivated(int, int)),
			this, SLOT(handle_cell_changed(int, int)));
#if 0
	QObject::connect(table_elements, SIGNAL(cellChanged(int, int)),
			this, SLOT(handle_cell_changed(int, int)));
#endif

	QObject::connect(table_elements, SIGNAL(currentCellChanged(int,int,int,int)),
			this, SLOT(handle_current_cell_changed(int,int,int,int)));

	setFocusProxy(table_elements);
}


void
GPlatesQtWidgets::EditStringListWidget::reset_widget_to_default_values()
{
	d_string_list_ptr = NULL;

	// Reset table.
	table_elements->clearContents();
	table_elements->setRowCount(0);

	// Reset widgets.
	textedit_element->clear();

	set_clean();
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::EditStringListWidget::create_property_value_from_widget() const
{
	std::vector<GPlatesPropertyValues::TextContent> elements;
	populate_element_vector_from_table(elements, *table_elements);
	return GPlatesPropertyValues::GpmlStringList::create_swap(elements);
}


bool
GPlatesQtWidgets::EditStringListWidget::update_property_value_from_widget()
{
	// Remember that the property value pointer may be NULL!
	if (d_string_list_ptr.get() != NULL) {
		if (is_dirty()) {
			std::vector<GPlatesPropertyValues::TextContent> elements;
			populate_element_vector_from_table(elements, *table_elements);
			d_string_list_ptr->swap(elements);

			set_clean();
			return true;
		} else {
			return false;
		}
	} else {
		throw UninitialisedEditWidgetException(GPLATES_EXCEPTION_SOURCE);
	}
	
}


void
GPlatesQtWidgets::EditStringListWidget::update_widget_from_string_list(
		GPlatesPropertyValues::GpmlStringList &gpml_string_list)
{
	d_string_list_ptr = &gpml_string_list;

	table_elements->clearContents();
	table_elements->setRowCount(0);
#if 0  // FIXME:  Why is this commented out?  Is this no longer necessary?
	ensure_table_size(*table_elements, static_cast<int>(gpml_string_list.size()));	
#endif

	GPlatesPropertyValues::GpmlStringList::const_iterator iter = gpml_string_list.begin();
	GPlatesPropertyValues::GpmlStringList::const_iterator end = gpml_string_list.end();

	for (int row = 0; iter != end ; ++iter, ++row) {
		QString str = GPlatesUtils::make_qstring_from_icu_string(iter->get());
		append_string_to_table(*table_elements, str);
	}
	set_clean();
	table_elements->setCurrentCell(0,0);
}


void
GPlatesQtWidgets::EditStringListWidget::handle_insert_row_above(
		const EditTableActionWidget *action_widget)
{
	int row = get_row_for_action_widget(action_widget);
	if (row >= 0) {
		insert_empty_string_element_into_table(row);
	}
}

void
GPlatesQtWidgets::EditStringListWidget::handle_insert_row_below(
		const EditTableActionWidget *action_widget)
{
	int row = get_row_for_action_widget(action_widget);
	if (row >= 0) {
		insert_empty_string_element_into_table(row + 1);
	}
}


void
GPlatesQtWidgets::EditStringListWidget::handle_delete_row(
		const EditTableActionWidget *action_widget)
{
	int row = get_row_for_action_widget(action_widget);
	if (row >= 0) {
		delete_row(row);
	}
}


void
GPlatesQtWidgets::EditStringListWidget::handle_cell_changed(
		int row,
		int column)
{
	if (get_element_string(*table_elements, row))
	{
		commit_changes();
	}
}


void
GPlatesQtWidgets::EditStringListWidget::handle_append_element_button_clicked()
{
	append_string_element(textedit_element->toPlainText());
	textedit_element->setFocus();
	textedit_element->selectAll();
}


void
GPlatesQtWidgets::EditStringListWidget::handle_cell_activated(
		int row, 
		int column)
{

}


int
GPlatesQtWidgets::EditStringListWidget::get_row_for_action_widget(
		const EditTableActionWidget *action_widget)
{
	int table_row_count = table_elements->rowCount();
	for (int i = 0; i < table_row_count; ++i) {
		if (table_elements->cellWidget(i, COLUMN_ACTION) == action_widget) {
			return i;
		}
	}
	return -1;
}


void
GPlatesQtWidgets::EditStringListWidget::append_string_element(
		const QString &str)
{
	append_string_to_table(*table_elements, str);

	// Scroll to show the user the point they just added.
	int row = table_elements->rowCount();
	QTableWidgetItem *table_item_to_scroll_to = table_elements->item(row, 0);
	if (table_item_to_scroll_to != NULL) {
		table_elements->scrollToItem(table_item_to_scroll_to);
	}
	// Work around a graphical glitch, where the EditTableActionWidgets above the
	// recently scrolled-to row appear to be misaligned.
	work_around_table_graphical_glitch(*this, *table_elements);

	set_dirty();
	emit commit_me();
}


void
GPlatesQtWidgets::EditStringListWidget::insert_empty_string_element_into_table(
		int row)
{
	// Insert a new blank row.
	table_elements->insertRow(row);
	populate_table_row_with_empty_string_element(*this, *table_elements, row);

	// Work around a graphical glitch, where the EditTableActionWidgets above the
	// recently scrolled-to row appear to be misaligned. And yes, the table widget
	// may auto-scroll if we (for instance) insert a row at the end.
	work_around_table_graphical_glitch(*this, *table_elements);

	// Open up an editor for the string element cell.
	QTableWidgetItem *elem_item = table_elements->item(row, COLUMN_ELEMENT);
	if (elem_item != NULL) {
		table_elements->setCurrentItem(elem_item);
		table_elements->editItem(elem_item);
	}

}


void
GPlatesQtWidgets::EditStringListWidget::delete_row(
		int row)
{
	// Before we delete the row, delete the action widget.
	// removeRow() messes with the previous/current row indices, and then calls
	// handle_current_cell_changed, which cannot delete the old action widget, the upshot being
	// that we end up with a surplus action widget which we can't get rid of. 
	table_elements->removeCellWidget(row, COLUMN_ACTION);
	// Delete the given row.
	table_elements->removeRow(row);

	// Work around a potential graphical glitch involving scrolling, as per the append and
	// insert point functions.
	work_around_table_graphical_glitch(*this, *table_elements);

	// Check if what we have now is (still) a valid time sequence.
	// FIXME: Do we need to check anything on removal of a time? Should we prevent 
	// an empty table?

	set_dirty();
	emit commit_me();
}


void
GPlatesQtWidgets::EditStringListWidget::commit_changes()
{
	set_dirty();
	emit commit_me();
}


void
GPlatesQtWidgets::EditStringListWidget::handle_current_cell_changed(
		int current_row,
		int current_column,
		int previous_row,
		int previous_column)
{
	// We move the action widget around so that it's in the currently-selected row only.
	if ((current_row != previous_row) && (current_row >= 0)) {
		// The row has changed, so we need to move the action widget.
		if (table_elements->cellWidget(previous_row, COLUMN_ACTION)) {
			table_elements->removeCellWidget(previous_row, COLUMN_ACTION);
		}
		GPlatesQtWidgets::EditTableActionWidget *action_widget =
				new GPlatesQtWidgets::EditTableActionWidget(this, this);
		table_elements->setCellWidget(current_row, COLUMN_ACTION, action_widget);
	}
}


#if 0
void
GPlatesQtWidgets::EditStringListWidget::handle_remove_all()
{
	QMessageBox message_box(this);
	message_box.setWindowTitle("Edit Time Sequence");
	message_box.setText("Remove all times?");
	QPushButton *remove_button = message_box.addButton(QObject::tr("Remove"),QMessageBox::AcceptRole);
	message_box.setStandardButtons(QMessageBox::Cancel);
	message_box.setDefaultButton(QMessageBox::Cancel);

	message_box.exec();

	if (message_box.clickedButton() != remove_button)
	{
		return;
	}

	table_times->clearContents();
	table_times->setRowCount(0);
}
#endif
