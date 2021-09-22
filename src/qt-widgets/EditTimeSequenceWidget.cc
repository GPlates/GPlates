/* $Id: EditTimeSequenceWidget.cc 8310 2010-05-06 15:02:23Z rwatson $ */

/**
 * \file 
 * $Revision: 8310 $
 * $Date: 2010-05-06 17:02:23 +0200 (to, 06 mai 2010) $ 
 * 
 * Copyright (C) 2009, 2010 Geological Survey of Norway
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
#include <QPainter>
#include <QString>
#include <QTableWidgetItem>

#include "EditTimeSequenceWidget.h"
#include "EditTableActionWidget.h"
#include "UninitialisedEditWidgetException.h"
#include "InvalidPropertyValueException.h"

#include "app-logic/ApplicationState.h"
#include "model/ModelUtils.h"
#include "presentation/ViewState.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GpmlArray.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/XsDouble.h"
#include "property-values/XsString.h"

//TODO: check control flow so that sorting/removing-duplicates isn't happening multiple times.
//TODO: (not in this source file but...) The tab order during flowline feature creation is poor; it jumps
// from leftplateID to geometry type....these widgets are spread over several container widgets though.

const bool DEMAND_ZERO_TIME_VALUE_IN_TABLE = true;

namespace
{
	enum ColumnLayout
	{
		COLUMN_TIME, COLUMN_ACTION
	};

	bool
	row_contains_zero(int row, QTableWidget *table)
	{
		bool ok;
		double value = table->item(row,COLUMN_TIME)->data(Qt::DisplayRole).toDouble(&ok);

		if (!ok)
		{
			return false;
		}
		if (qFuzzyCompare(value+1,1.))
		{
			return true;
		}
		return false;
	}
	
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
	 * Allocates QTableWidgetItems and populates a QTableWidget from a
	 * time.
	 *
	 * No checking is done to see if the table is the correct size!
	 * The caller is responsible for adding rows to the table appropriately.
	*/
	bool
	attempt_to_populate_table_row_from_time(
		GPlatesQtWidgets::EditTimeSequenceWidget &time_sequence_widget,
		QTableWidget &table,
		double time)
	{
		static QLocale locale;

		if (time < 0)
		{
			return false;
		}

		QString time_as_string = locale.toString(time);

		if (!(table.findItems(time_as_string,Qt::MatchExactly)).isEmpty())
		{
			return false;
		}

		QTableWidgetItem *item = new QTableWidgetItem;
		item->setData(Qt::DisplayRole,time);
		
		// Add the time cell
		int row = table.rowCount();
		table.insertRow(row);
		table.setItem(row, COLUMN_TIME, item);		
		
		// Add the "Action" cell - we need to set this as uneditable.
		QTableWidgetItem *action_item = new QTableWidgetItem();
		action_item->setFlags(Qt::ItemFlags());
		table.setItem(row, COLUMN_ACTION, action_item);
				
		// Set the current cell to be a cell from the new row, so that an action widget is added to it. 
		table.setCurrentCell(row,COLUMN_ACTION);		
		
		return row;
	}	


	/**
	* Allocates QTableWidgetItems and populates a QTableWidget from a
	* GPlatesMaths::PointOnSphere.
	*
	* No checking is done to see if the table is the correct size!
	* The caller is responsible for adding rows to the table appropriately.
	*/
	void
	populate_table_row_with_blank_time(
		GPlatesQtWidgets::EditTimeSequenceWidget &time_sequence_widget,
		QTableWidget &table,
		int row)
	{
		// Add the time cell.
		
		QTableWidgetItem *item = new QTableWidgetItem();
		item->setData(0,QVariant(QVariant::Double));
		
		table.setItem(row, COLUMN_TIME, item);
		// Add the "Action" cell - we need to set this as uneditable.
		QTableWidgetItem *action_item = new QTableWidgetItem();
		action_item->setFlags(Qt::ItemFlags());
		table.setItem(row, COLUMN_ACTION, action_item);
		// Creating the action_widget is not a memory leak - Qt will take ownership of
		// the action_widget memory, and clean it up when the table row is deleted.
		GPlatesQtWidgets::EditTableActionWidget *action_widget =
			new GPlatesQtWidgets::EditTableActionWidget(
			static_cast<GPlatesQtWidgets::EditTableWidget*>(&time_sequence_widget), &time_sequence_widget);
		table.setCellWidget(row, COLUMN_ACTION, action_widget);
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
		GPlatesQtWidgets::EditTimeSequenceWidget &edit_time_sequence_widget,
		QTableWidget &table)
	{
		static const GPlatesQtWidgets::EditTableActionWidget dummy(&edit_time_sequence_widget, NULL);
		table.horizontalHeader()->resizeSection(COLUMN_ACTION, dummy.width() + 1);
		table.horizontalHeader()->resizeSection(COLUMN_ACTION, dummy.width());
	}	
	
	boost::optional<double>
	get_valid_time(
		QTableWidget &table_widget,
		int row)
	{


		QTableWidgetItem* item = table_widget.item(row,COLUMN_TIME);

		if (item == NULL)
		{
			return boost::none;
		}
		
		static QLocale locale;
		bool ok = false;

//		double time = locale.toDouble(item->text(),&ok);

		// Retreving item->text() seems to give us a decimal representation of the
		// data, even if the display form uses the locale. For example, 6.5 under
		// a Norwegian locale is displayed in the table as "6,5". But when we call
		// item->text(), we get "6.5", and performing a locale.toDouble on it messes it up.
		//
		// Using item->data(...) as below lets me check for any problems with the "ok" boolean. 
		double time = item->data(Qt::DisplayRole).toDouble(&ok);

		if (!ok)
		{
			return boost::none;
		}
		
		if (time < 0)
		{
			time = 0;
			item->setData(0,time);
		}

		return time;

	}

	void
	remove_row(
		int row,
		QTableWidget *table_widget)
	{
		if (DEMAND_ZERO_TIME_VALUE_IN_TABLE && !row_contains_zero(row,table_widget))
		{
			table_widget->removeCellWidget(row,COLUMN_TIME);
			table_widget->removeCellWidget(row,COLUMN_ACTION);
			table_widget->removeRow(row);
		}
	}

	/**
	 * This removes contiguous rows from a QTableWidget specified by the the table widget's 
	 * selectedRanges() function.
	 *
	 * It will only behave correctly if the widget's "selectionMode" is set to "ContiguousSelection". 
	 * In this case, the QList should have only one entry.
	 *
	 *
	 */
	void
	remove_rows(
		QTableWidget *table_widget)
	{
		QList<QTableWidgetSelectionRange> range = table_widget->selectedRanges();


		QList<QTableWidgetSelectionRange>::const_iterator it = range.begin();

		for (; it != range.end() ; ++it)
		{
			int row_to_remove = it->topRow();
			int number_of_rows_to_remove = it->rowCount();

			for (int i = 0 ; i < number_of_rows_to_remove ; ++i)
			{
				qDebug() << "removing: " << i << " of " << number_of_rows_to_remove;
				if (DEMAND_ZERO_TIME_VALUE_IN_TABLE && !row_contains_zero(row_to_remove,table_widget))
				{
					remove_row(row_to_remove,table_widget);
				}
			}
		}
	}

	void
	sort_and_remove_duplicates_from_table(
			QTableWidget *table)
	{
		table->sortItems(COLUMN_TIME,Qt::AscendingOrder);

		if (table->rowCount() == 0)
		{
			return;
		}

		QString previous_time_as_string = table->item(0,COLUMN_TIME)->text();
		for (int count = 1; count < table->rowCount() ; ++count)
		{
			QString current_time_as_string = table->item(count,COLUMN_TIME)->text();
			if (current_time_as_string == previous_time_as_string)
			{
				remove_row(count,table);
			}
			previous_time_as_string = current_time_as_string;
		}
	}
}


GPlatesQtWidgets::EditTimeSequenceWidget::EditTimeSequenceWidget(
                GPlatesAppLogic::ApplicationState &app_state_,
		QWidget *parent_):
	AbstractEditWidget(parent_),
	EditTableWidget(),
        d_current_reconstruction_time(
				app_state_.get_current_reconstruction_time()),
		d_spin_box_delegate(new EditTimeSequenceSpinBoxDelegate(this))
{
    setupUi(this);
    // Set column widths and resizabilty.
    EditTableActionWidget dummy(this, NULL);
    table_times->horizontalHeader()->setSectionResizeMode(COLUMN_TIME, QHeaderView::Stretch);
    table_times->horizontalHeader()->setSectionResizeMode(COLUMN_ACTION, QHeaderView::Fixed);
    table_times->horizontalHeader()->resizeSection(COLUMN_ACTION, dummy.width());
    table_times->horizontalHeader()->setSectionsMovable(true);
    // Set up a minimum row height as well, for the action widgets' sake.
    table_times->verticalHeader()->setDefaultSectionSize(dummy.height());

    // Clear spinboxes and things.
    reset_widget_to_default_values();

    setup_connections();
    update_buttons();

    setFocusProxy(table_times);

	table_times->setItemDelegateForColumn(COLUMN_TIME,d_spin_box_delegate);
	table_times->setSelectionMode(QAbstractItemView::ContiguousSelection);
}


void
GPlatesQtWidgets::EditTimeSequenceWidget::reset_widget_to_default_values()
{

    d_array_ptr = NULL;
    // Reset table.
    table_times->clearContents();
    table_times->setRowCount(0);

	if (DEMAND_ZERO_TIME_VALUE_IN_TABLE)
	{
		insert_single(0.);
	}

    // Reset widgets.
    spinbox_time->setValue(0.0);
    spinbox_from_time->setValue(100.0);
	spinbox_to_time->setValue(0.0);
    spinbox_step_time->setValue(10.0);

    set_clean();
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::EditTimeSequenceWidget::create_property_value_from_widget() const
{
	sort_and_remove_duplicates_from_table(table_times);

	std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> time_periods;

	static const GPlatesPropertyValues::StructuralType gml_time_period_type =
		GPlatesPropertyValues::StructuralType::create_gml("TimePeriod");

	// Find first valid time in table, so we can store it as the "end" part of the first gpml:Array element. 
	boost::optional<double> begin_time,end_time;
	int count;

	for (count = 0; count < table_times->rowCount(); ++count)
	{			   
		end_time = get_valid_time(*table_times,count);
		if (end_time)
		{			 
			break;
		}
	}		
	++count;
	for (int i = count; i < table_times->rowCount(); ++i)
	{
		begin_time = get_valid_time(*table_times,i);
		if (begin_time)
		{
			GPlatesPropertyValues::GeoTimeInstant end_geo_instant(*end_time);
			GPlatesPropertyValues::GeoTimeInstant begin_geo_instant(*begin_time);

			GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type end_gml_instant =
				GPlatesModel::ModelUtils::create_gml_time_instant(end_geo_instant);

			GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type begin_gml_instant =
				GPlatesModel::ModelUtils::create_gml_time_instant(begin_geo_instant);	

			GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type gml_time_period =
				GPlatesPropertyValues::GmlTimePeriod::create(begin_gml_instant,end_gml_instant);

			time_periods.push_back(gml_time_period);

			// Get ready for next iteration; use the current begin_time as the end_time for the next 
			// time-period. 
			end_time = begin_time;
		}

	}

	// There should be at least one time *period* in the array (which is really two time *samples*).
	//
	// FIXME: Currently 'gpml:Array' is currently hardwired (or expected) to be a sequence of time samples.
	// Later, when other template types are supported for 'gpml:Array', we'll need to be able to handle them.
	// For now the we're assuming time periods and one time period needs two time samples.
	if (time_periods.empty())
	{
		throw InvalidPropertyValueException(GPLATES_EXCEPTION_SOURCE,
				tr("The time sequence should contain at least two time samples."));
	}

	return GPlatesPropertyValues::GpmlArray::create(time_periods, gml_time_period_type);
}

bool
GPlatesQtWidgets::EditTimeSequenceWidget::update_property_value_from_widget()
{
	// Remember that the property value pointer may be NULL!

	if (d_array_ptr.get() != NULL) {
		if (is_dirty()) {
			sort_and_remove_duplicates_from_table(table_times);
			update_time_array_from_widget();
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
GPlatesQtWidgets::EditTimeSequenceWidget::update_widget_from_time_period_array(
    GPlatesPropertyValues::GpmlArray &gpml_array)
{

    // Here we assume that the time periods stored in the array are adjoining,
    // and that they are ordered youngest->oldest.
    //
    // We take the end() time of each time period, and add on the begin() time of
    // the previous time period.
    d_array_ptr = &gpml_array;

    static const GPlatesPropertyValues::StructuralType gml_time_period_type =
            GPlatesPropertyValues::StructuralType::create_gml("TimePeriod");

    if (gpml_array.type() != gml_time_period_type)
    {
        return;
    }


    std::vector<GPlatesModel::PropertyValue::non_null_ptr_type>::const_iterator
            it = gpml_array.members().begin(),
            end = gpml_array.members().end();



    table_times->clearContents();
    table_times->setRowCount(0);

#if 0
    ensure_table_size(*table_times, static_cast<int>(times.size()));
#endif

    // We will use the last gml_time_period_ptr after the loop has completed so declare it now.
    GPlatesPropertyValues::GmlTimePeriod* gml_time_period_ptr = NULL;

    for (int row = 0; it != end ; ++it, ++row)
    {
	try
	{
	    gml_time_period_ptr =
		    dynamic_cast<GPlatesPropertyValues::GmlTimePeriod*>((*it).get());

	    GPlatesPropertyValues::GeoTimeInstant geo_time_instant =
		    gml_time_period_ptr->end()->time_position();

	    if (geo_time_instant.is_real())
	    {
		attempt_to_populate_table_row_from_time(*this,*table_times,geo_time_instant.value());
	    }
	}
	catch(const std::bad_cast &)
	{

	}
    }

    // And finish off with the begin() time of the last (oldest) time period.
    if (gml_time_period_ptr)
    {
        GPlatesPropertyValues::GeoTimeInstant geo_time_instant = gml_time_period_ptr->begin()->time_position();
        if (geo_time_instant.is_real())
        {
            attempt_to_populate_table_row_from_time(*this,*table_times,geo_time_instant.value());
        }
    }

    set_clean();

    table_times->setCurrentCell(0,0);
}


void
GPlatesQtWidgets::EditTimeSequenceWidget::update_time_array_from_widget()
{
	std::vector<GPlatesModel::PropertyValue::non_null_ptr_type> time_periods;

	static const GPlatesPropertyValues::StructuralType gml_time_period_type =
		GPlatesPropertyValues::StructuralType::create_gml("TimePeriod");

	// Find first valid time in table, so we can store it as the "end" part of the first gpml:Array element. 
	boost::optional<double> begin_time, end_time;
	int count;

	for (count = 0; count < table_times->rowCount(); ++count)
	{			   
		end_time = get_valid_time(*table_times,count);
		if (end_time)
		{			 
			break;
		}
	}
	++count;
	for (int i = count; i < table_times->rowCount(); ++i)
	{
		begin_time = get_valid_time(*table_times,i);
		if (begin_time)
		{
			GPlatesPropertyValues::GeoTimeInstant end_geo_instant(*end_time);
			GPlatesPropertyValues::GeoTimeInstant begin_geo_instant(*begin_time);

			GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type end_gml_instant =
				GPlatesModel::ModelUtils::create_gml_time_instant(end_geo_instant);

			GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type begin_gml_instant =
				GPlatesModel::ModelUtils::create_gml_time_instant(begin_geo_instant);	

			GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type gml_time_period =
				GPlatesPropertyValues::GmlTimePeriod::create(begin_gml_instant,end_gml_instant);

			time_periods.push_back(gml_time_period);

			// Get ready for next iteration; use the current begin_time as the end_time for the next 
			// time-period. 
			end_time = begin_time;
		}

	}

	d_array_ptr->members() = time_periods;

}

void
GPlatesQtWidgets::EditTimeSequenceWidget::handle_insert_row_above(
		const EditTableActionWidget *action_widget)
{
	int row = get_row_for_action_widget(action_widget);
	if (row >= 0) {
		insert_blank_time_into_table(row);
	}
	update_buttons();
}

void
GPlatesQtWidgets::EditTimeSequenceWidget::handle_insert_row_below(
		const EditTableActionWidget *action_widget)
{
	int row = get_row_for_action_widget(action_widget);
	if (row >= 0) {
		insert_blank_time_into_table(row + 1);
	}
	update_buttons();
}

void
GPlatesQtWidgets::EditTimeSequenceWidget::handle_delete_row(
		const EditTableActionWidget *action_widget)
{
	int row = get_row_for_action_widget(action_widget);
	if (row >= 0) {
		delete_time_from_table(row);
	}
	update_buttons();
}


int
GPlatesQtWidgets::EditTimeSequenceWidget::get_row_for_action_widget(
		const EditTableActionWidget *action_widget)
{
	for (int i = 0; i < table_times->rowCount(); ++i)
	{
		if (table_times->cellWidget(i, COLUMN_ACTION) == action_widget) {
			return i;
		}
	}
	return -1;
}


void
GPlatesQtWidgets::EditTimeSequenceWidget::insert_single(
		double time)
{

	if (!attempt_to_populate_table_row_from_time(*this,*table_times, time))
	{
		return;
	}

	int row = table_times->rowCount();

	// Scroll to show the user the point they just added.
	QTableWidgetItem *table_item_to_scroll_to = table_times->item(row, 0);
	if (table_item_to_scroll_to != NULL) {
		table_times->scrollToItem(table_item_to_scroll_to);
	}
	// Work around a graphical glitch, where the EditTableActionWidgets above the
	// recently scrolled-to row appear to be misaligned.
	work_around_table_graphical_glitch(*this, *table_times);

        // Attempts to trigger a call to handle_current_cell_changed which should add in the
        // action widget. The handle_ function gets called ok, but never seems to add the
        // action widget to a new row....
        //table_times->setCurrentCell(row,COLUMN_ACTION);
        //handle_current_cell_changed(row,COLUMN_TIME,-1,COLUMN_TIME);


	update_buttons();
}


void
GPlatesQtWidgets::EditTimeSequenceWidget::insert_blank_time_into_table(
		int row)
{
	// Insert a new blank row.

	table_times->insertRow(row);
	populate_table_row_with_blank_time(*this, *table_times, row);
	
	// Work around a graphical glitch, where the EditTableActionWidgets above the
	// recently scrolled-to row appear to be misaligned. And yes, the table widget
	// may auto-scroll if we (for instance) insert a row at the end.
	work_around_table_graphical_glitch(*this, *table_times);
	
	// Open up an editor for the first time field.
	QTableWidgetItem *time_item = table_times->item(row, COLUMN_TIME);
	if (time_item != NULL) {
		table_times->setCurrentItem(time_item);
		table_times->editItem(time_item);
	}

}


void
GPlatesQtWidgets::EditTimeSequenceWidget::delete_time_from_table(
		int row)
{

	if (DEMAND_ZERO_TIME_VALUE_IN_TABLE && row_contains_zero(row,table_times))
	{
		return;
	}
	// Before we delete the row, delete the action widget. removeRow() messes with the previous/current
	// row indices, and then calls handle_current_cell_changed, which cannot delete the old action widget, 
	// the upshot being that we end up with a surplus action widget which we can't get rid of. 
	table_times->removeCellWidget(row,COLUMN_ACTION);
	table_times->removeCellWidget(row,COLUMN_TIME);
	// Delete the given row.
	table_times->removeRow(row);

	// Work around a potential graphical glitch involving scrolling, as per the
	// append and insert point functions.
	work_around_table_graphical_glitch(*this, *table_times);
	
	// Check if what we have now is (still) a valid time sequence.
	// FIXME: Do we need to check anything on removal of a time? Should we prevent 
	// an empty table?
	
		
	set_dirty();
	Q_EMIT commit_me();
}


void
GPlatesQtWidgets::EditTimeSequenceWidget::sort_and_commit()
{
	sort_and_remove_duplicates_from_table(table_times);
	set_dirty();
	Q_EMIT commit_me();
}

void
GPlatesQtWidgets::EditTimeSequenceWidget::handle_current_cell_changed(
		int current_row, int current_column, int previous_row, int previous_column)
{
	if ((current_row != previous_row) && current_row >=0)
	{
		if (table_times->cellWidget(previous_row,COLUMN_ACTION))
		{
			table_times->removeCellWidget(previous_row,COLUMN_ACTION);
		}
		GPlatesQtWidgets::EditTableActionWidget *action_widget =
				new GPlatesQtWidgets::EditTableActionWidget(this, this);
		table_times->setCellWidget(current_row, COLUMN_ACTION, action_widget);
	}
	update_buttons();
}

void
GPlatesQtWidgets::EditTimeSequenceWidget::insert_multiple()
{
	double oldest_time = spinbox_from_time->value();
	double youngest_time = spinbox_to_time->value();
	double step = spinbox_step_time->value();
	
	
	if (youngest_time > oldest_time)
	{
		return;
	}
	
	if (step <= 0.)
	{
		return;
	}
	
	double time; // declare here as we use it later outside the loop to check the last time.
	for (time = youngest_time ; time <= oldest_time ; time += step)
	{
		insert_single(time);
	}

	// We may have hopped over the oldest time. Add it here.
	if (time > oldest_time)
	{
		insert_single(oldest_time);
	}


}

void
GPlatesQtWidgets::EditTimeSequenceWidget::handle_spinbox_editing_finished()
{
	sort_and_commit();
}

void
GPlatesQtWidgets::EditTimeSequenceWidget::handle_remove_all()
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

	if (DEMAND_ZERO_TIME_VALUE_IN_TABLE)
	{
		insert_single(0);
	}
	update_buttons();
	sort_and_commit();
}

void
GPlatesQtWidgets::EditTimeSequenceWidget::handle_remove()
{
	remove_rows(table_times);
	update_buttons();
	sort_and_commit();
}

void
GPlatesQtWidgets::EditTimeSequenceWidget::update_buttons()
{
	QList<QTableWidgetSelectionRange> range = table_times->selectedRanges();
	button_remove->setEnabled(!range.isEmpty());
	button_remove_all->setEnabled(table_times->rowCount()>0);
}

void
GPlatesQtWidgets::EditTimeSequenceWidget::handle_use_main_single()
{
    spinbox_time->setValue(d_current_reconstruction_time);
}

void
GPlatesQtWidgets::EditTimeSequenceWidget::handle_use_main_from()
{
    spinbox_from_time->setValue(d_current_reconstruction_time);
}

void
GPlatesQtWidgets::EditTimeSequenceWidget::handle_use_main_to()
{
    spinbox_to_time->setValue(d_current_reconstruction_time);
}

void
GPlatesQtWidgets::EditTimeSequenceWidget::setup_connections()
{
        // Signals for managing data entry focus for the "Insert single time" widgets.
        QObject::connect(button_insert_single, SIGNAL(clicked()),
                        this, SLOT(handle_insert_single()));

        // Signals for clearing the table .
        QObject::connect(button_remove_all, SIGNAL(clicked()),
                this, SLOT(handle_remove_all()));

        QObject::connect(button_remove, SIGNAL(clicked()),
                this, SLOT(handle_remove()));

        // Signals for managing data entry focus for the "Fill with times" widgets.
        QObject::connect(button_insert_multiple, SIGNAL(clicked()),
                this, SLOT(handle_insert_multiple()));

        QObject::connect(table_times, SIGNAL(currentCellChanged(int,int,int,int)),
                   this, SLOT(handle_current_cell_changed(int,int,int,int)));

        QObject::connect(button_use_main_single,
                         SIGNAL(clicked()),
                         this,
                         SLOT(handle_use_main_single()));

        QObject::connect(button_use_main_from,
                         SIGNAL(clicked()),
                         this,
                         SLOT(handle_use_main_from()));

        QObject::connect(button_use_main_to,
                         SIGNAL(clicked()),
                         this,
                         SLOT(handle_use_main_to()));

		QObject::connect(d_spin_box_delegate,SIGNAL(editing_finished()),
						 this,SLOT(handle_spinbox_editing_finished()));

}

void
GPlatesQtWidgets::EditTimeSequenceWidget::handle_single_time_entered()
{
    handle_insert_single();
}


GPlatesQtWidgets::EditTimeSequenceSpinBoxDelegate::EditTimeSequenceSpinBoxDelegate(QObject *parent_):
	QItemDelegate(parent_)
{

}

QWidget*
GPlatesQtWidgets::EditTimeSequenceSpinBoxDelegate::createEditor(
		QWidget *parent_,
		const QStyleOptionViewItem &/* option */,
		const QModelIndex &index) const
{
	QDoubleSpinBox *editor = new QDoubleSpinBox(parent_);
	editor->setDecimals(4);
	editor->setMinimum(0.);
	editor->setMaximum(1000.);
	return editor;
}

void
GPlatesQtWidgets::EditTimeSequenceSpinBoxDelegate::setEditorData(
		QWidget *editor,
		const QModelIndex &index) const
{
	double value = index.model()->data(index, Qt::EditRole).toInt();
	QDoubleSpinBox *spinbox = static_cast<QDoubleSpinBox*>(editor);
	spinbox->setValue(value);
}

void
GPlatesQtWidgets::EditTimeSequenceSpinBoxDelegate::setModelData(
		QWidget *editor,
		QAbstractItemModel *model,
		const QModelIndex &index) const
{
	QDoubleSpinBox *spinbox = static_cast<QDoubleSpinBox*>(editor);

	QVariant value = spinbox->value();
	model->setData(index,value,Qt::EditRole);

	// If we use the closeEditor(QWidget*) signal, for some reason the double contained in the
	// spinbox has been mangled to an integer when we examine the spinbox value again in the
	// slot, and this integer is added to the table. I don't know why this is, but if we emit
	// our own signal here, it appears to prevent this double-to-integer behaviour....
	Q_EMIT editing_finished();
}

void
GPlatesQtWidgets::EditTimeSequenceSpinBoxDelegate::updateEditorGeometry(
		QWidget *editor,
		const QStyleOptionViewItem &option,
		const QModelIndex &/* index */) const
{
	editor->setGeometry(option.rect);
}

void
GPlatesQtWidgets::EditTimeSequenceSpinBoxDelegate::paint(
		QPainter *painter,
		const QStyleOptionViewItem &option,
		const QModelIndex &index) const
{
	painter->drawText(option.rect, Qt::AlignCenter,index.data().toString());
}

