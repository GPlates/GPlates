/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include <cstddef> // std::size_t
#include <iostream>
#include <vector>
#include <boost/noncopyable.hpp>
#include <QGridLayout>
#include <QWidget>
#include <QDebug>

#include "TopologySectionsTable.h"

#include "FeatureFocus.h"
#include "TopologySectionsContainer.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/LayerProxyUtils.h"

#include "presentation/ViewState.h"

#include "qt-widgets/ActionButtonBox.h"
#include "qt-widgets/InsertionPointWidget.h"


namespace
{
	bool
	is_row_prev_neighbor_of_insert( int r, int i, int table_count )
	{
		// check insert at top of table 
		if ( i == 0 ) {
			// is row last entry in table ? 
			if (r == (table_count - 1) )
				return true;
		} else {
			// is row just above insert ?
			if ( r == (i - 1) )
				return true;
		}
		return false;
	}

	bool
	is_row_next_neighbor_of_insert( int r, int i, int table_count )
	{
		// check insert at end of table
		if ( i == (table_count - 1) ) {
			// is row first entry in table ? 
			if (r == 0)
				return true;
		} else {
			// is row just below insert ?
			if ( r == i + 1)
				return true;
		}
		return false;
	}


		

	bool
	check_row_validity(
			const GPlatesGui::TopologySectionsContainer::TableRow &entry)
	{
		return entry.get_feature_ref().is_valid();
	}
	
	bool
	check_row_validity_geom(
			const GPlatesGui::TopologySectionsContainer::TableRow &entry)
	{
		return entry.get_geometry_property().is_still_valid();
	}

	bool
	check_row_validity_reconstructed_geometry(
			const GPlatesGui::TopologySectionsContainer::TableRow &entry,
			const GPlatesAppLogic::Reconstruction &reconstruction)
	{
		// Get the RFGs (and generate if not already), for all active ReconstructLayer's, that reference the feature.
		std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> found_rfgs;
		GPlatesAppLogic::LayerProxyUtils::find_reconstructed_feature_geometries_of_feature(
				found_rfgs,
				entry.get_feature_ref(),
				reconstruction);

		// If we found any RFGs in the current reconstruction then it means the topological section
		// feature exists at the current reconstruction time (of 'reconstruction').
		return !found_rfgs.empty();
	}
	
	
	const QString
	get_invalid_row_message(
			const GPlatesGui::TopologySectionsContainer::TableRow &entry)
	{
		QString feature_id_qstr = GPlatesUtils::make_qstring(entry.get_feature_id());
		QString message = QObject::tr("(Unresolvable feature reference to \"%1\")")
				.arg(feature_id_qstr);
		return message;
	}


	/**
	 * Tiny convenience class to help suppress the @a QTableWidget::cellChanged()
	 * notification in situations where we are updating the table data
	 * programatically. This allows @a react_cell_changed() to differentiate
	 * between changes made by us, and changes made by the user.
	 *
	 * For it to work properly, you must declare one in any TopologySectionsTable
	 * method that directly mucks with table cell data.
	 */
	struct TableUpdateGuard :
			public boost::noncopyable
	{
		TableUpdateGuard(
				bool &guard_flag_ref):
			d_guard_flag_ptr(&guard_flag_ref)
		{
			// Nesting these guards is an error.
			Q_ASSERT(*d_guard_flag_ptr == false);
			*d_guard_flag_ptr = true;
		}
		
		~TableUpdateGuard()
		{
			*d_guard_flag_ptr = false;
		}
		
		bool *d_guard_flag_ptr;
	};
}


GPlatesGui::TopologySectionsTable::TopologySectionsTable(
		QTableWidget &table,
		TopologySectionsContainer &boundary_container,
		TopologySectionsContainer &interior_container,
		GPlatesPresentation::ViewState &view_state):
	d_table(&table),
	d_boundary_container_ptr(&boundary_container),
	d_interior_container_ptr(&interior_container),
	d_action_box_row(-1),
	d_remove_action(new QAction(&table)),
	d_insert_above_action(new QAction(&table)),
	d_insert_below_action(new QAction(&table)),
	d_cancel_insertion_point_action(new QAction(&table)),
	d_suppress_update_notification_guard(false),
	d_application_state(view_state.get_application_state()),
	d_feature_focus_ptr(&view_state.get_feature_focus())
{
	// Set up the actions we can use.
	set_up_actions();
	
	// connect to all containers
	set_up_connections_to_container( d_boundary_container_ptr );
	set_up_connections_to_container( d_interior_container_ptr );

	// set the default pointer
	d_container_ptr = d_boundary_container_ptr;

	// Set up the basic properties of the table.
	set_up_table();

	//
	update_table();

	// Enable the table to receive mouse move events, so we can show/hide buttons
	// based on the row being hovered over.
	d_table->setMouseTracking(true);

	QObject::connect(
		d_table, 
		SIGNAL(cellEntered(int, int)),
		this, 
		SLOT(react_cell_entered(int, int)));

	// Some cells may be user-editable: listen for changes.
	QObject::connect(
		d_table, 
		SIGNAL(cellChanged(int, int)),
		this, 
		SLOT(react_cell_changed(int, int)));

	// Adjust focus via clicking on table rows.
	QObject::connect(
		d_table, 
		SIGNAL(cellClicked(int, int)),
		this, 
		SLOT(react_cell_clicked(int, int)));
}

void
GPlatesGui::TopologySectionsTable::set_up_connections_to_container(
	TopologySectionsContainer *container_ptr)
{
	QObject::connect(
		container_ptr, 
		SIGNAL(do_update()),
		this, 
		SLOT(update_table()));

	QObject::connect(
		container_ptr, 
		SIGNAL(cleared()),
		this, 
		SLOT(clear_table()));

	QObject::connect(
		container_ptr, 
		SIGNAL(insertion_point_moved(GPlatesGui::TopologySectionsContainer::size_type)),
		this, 
		SLOT(update_table()));

	QObject::connect(
		container_ptr, 
		SIGNAL(entry_removed(GPlatesGui::TopologySectionsContainer::size_type)),
		this, 
		SLOT(update_table()));

	QObject::connect(
		container_ptr,
		SIGNAL(entries_inserted(
			GPlatesGui::TopologySectionsContainer::size_type,
			GPlatesGui::TopologySectionsContainer::size_type,
			GPlatesGui::TopologySectionsContainer::const_iterator,
			GPlatesGui::TopologySectionsContainer::const_iterator)),
		this,
		SLOT(update_table()));

	QObject::connect(
		container_ptr, 
		SIGNAL(entry_modified(GPlatesGui::TopologySectionsContainer::size_type)),
		this, 
		SLOT(topology_section_modified(GPlatesGui::TopologySectionsContainer::size_type)));

	QObject::connect(
		container_ptr, 
		SIGNAL(focus_feature_at_index( GPlatesGui::TopologySectionsContainer::size_type)),
		this, 
		SLOT(react_focus_feature_at_index(GPlatesGui::TopologySectionsContainer::size_type)));

	QObject::connect(
		container_ptr, 
		SIGNAL(container_change(GPlatesGui::TopologySectionsContainer *)),
		this, 
		SLOT(react_container_change(GPlatesGui::TopologySectionsContainer *)));

}

void
GPlatesGui::TopologySectionsTable::clear_table()
{
	remove_action_box();
	d_table->setRowCount(0);
}


void
GPlatesGui::TopologySectionsTable::update_table()
{
	update_table_row_count();
	for (int i = 0; i < d_table->rowCount(); ++i) {
		update_table_row(i);
	}
}


void
GPlatesGui::TopologySectionsTable::react_cell_entered(
		int row,
		int col)
{
	move_action_box(row);
}


void
GPlatesGui::TopologySectionsTable::react_cell_clicked(
		int row,
		int col)
{
	// NOTE: I'm finding it a little difficult to click (and toggle) the reverse flag
	// with the default checkbox supplied by QTableWidget. Something just makes it more
	// difficult to click properly.
	// Not that I have time to do it now, but a possible solution is to not treat it
	// as a Qt::ItemIsCheckable or whatever, but instead implement our own toggle button
	// and put it in there. Or maybe just make it an icon for reversed/not reversed,
	// and toggle it by reacting to this click event.

	focus_feature_at_row(row);
}


void
GPlatesGui::TopologySectionsTable::react_cell_changed(
		int row,
		int col)
{
	if (d_suppress_update_notification_guard) {
		return;
	}

	update_data_from_table(row);
}


void
GPlatesGui::TopologySectionsTable::react_remove_clicked()
{
	TopologySectionsContainer::size_type my_index = 
		convert_table_row_to_data_index(get_current_action_box_row());

	if (my_index > d_container_ptr->size()) 
	{
		return;
	}

	// Remove this entry from the table.
	d_container_ptr->remove_at(my_index);
}


void
GPlatesGui::TopologySectionsTable::react_insert_above_clicked()
{
	// Insert "Above me", i.e. insertion point should replace this row.
	TopologySectionsContainer::size_type my_index = 
		convert_table_row_to_data_index(get_current_action_box_row());

	TopologySectionsContainer::size_type target_index = my_index;

	if (target_index == d_container_ptr->insertion_point()) 
	{
		// It's already there. Nevermind.
		return;
	}

	d_container_ptr->move_insertion_point(target_index);
}


void
GPlatesGui::TopologySectionsTable::react_insert_below_clicked()
{
	// Insert "Below me", i.e. insertion point should be this row + 1.
	TopologySectionsContainer::size_type my_index = 
		convert_table_row_to_data_index(get_current_action_box_row());

	TopologySectionsContainer::size_type target_index = my_index + 1;

	if (target_index == d_container_ptr->insertion_point()) 
	{
		// It's already there. Nevermind.
		return;
	}
	
	d_container_ptr->move_insertion_point(target_index);
}


void
GPlatesGui::TopologySectionsTable::react_cancel_insertion_point_clicked()
{
	d_container_ptr->reset_insertion_point();
}


void
GPlatesGui::TopologySectionsTable::topology_section_modified(
		GPlatesGui::TopologySectionsContainer::size_type topology_sections_container_index)
{
	update_table_row(
			convert_data_index_to_table_row(topology_sections_container_index));
}


int
GPlatesGui::TopologySectionsTable::get_current_action_box_row()
{
	return d_action_box_row;
}


void
GPlatesGui::TopologySectionsTable::move_action_box(
		int row)
{
	if (row == get_current_action_box_row()) {
		return;
	}

	// Remove the Action Box from the previous location.
	remove_action_box();

	// Add the Action Box to the new location.
	if (row >= 0 && row < d_table->rowCount()) {
		// We set the action box row even if it's the same as the insertion point row.
		// But we don't draw the action box if it's the same as the insertion point row.
		//
		// This has the effect of registering the current row as the action box row so
		// that the action box will get drawn as soon as the insertion point moves -
		// in this case it will move when the insertion point is reset.
		//
		// This needs to be done because the only time the action box is moved is when
		// the mouse enters a cell which only happens when the mouse pointer moves and this
		// won't happen when clicking on the 'reset insertion point' button next to the
		// insertion arrow - but clicking on that button does move the insertion arrow
		// leaving the row free for the action box.
		d_action_box_row = row;
		if (row != get_current_insertion_point_row())
		{
			set_action_box_widget(row);
		}
	}
}


void
GPlatesGui::TopologySectionsTable::remove_action_box()
{
	const int old_row = get_current_action_box_row();

	// Don't remove the insertion point arrow if the last row of our
	// action box happened to be the same as the insertion point row.
	if (old_row == get_current_insertion_point_row()) {
		return;
	}

	if (old_row >= 0 && old_row < d_table->rowCount()) {
		d_action_box_row = -1;
		// While the 4.3 documentation does not indicate how Qt treats the
		// cell widget, the 4.4 documentation enlightens us; it takes
		// ownership. Unfortunately, there is no 'takeCellWidget', so
		// we cannot get it back once it has grabbed it, only remove it.
		// http://doc.trolltech.com/4.4/qtablewidget.html#removeCellWidget
		// Rest assured, the ActionButtonBox destructor does get called
		// (by the QTableWidget) at this point.
		d_table->removeCellWidget(old_row, TopologySectionsTableColumns::COLUMN_ACTIONS);
	}
}


void
GPlatesGui::TopologySectionsTable::set_action_box_widget(
		int row)
{
	// Create the Action Box in the new location.
	if (row >= 0 && row < d_table->rowCount()) {
		// While the 4.3 documentation does not indicate how Qt treats the
		// cell widget, the 4.4 documentation enlightens us; it takes
		// ownership. Unfortunately, there is no 'takeCellWidget', so
		// we cannot get it back once it has grabbed it, only remove it.
		// http://doc.trolltech.com/4.4/qtablewidget.html#setCellWidget
		d_table->setCellWidget(
				row, TopologySectionsTableColumns::COLUMN_ACTIONS, create_new_action_box());
	}
}


int
GPlatesGui::TopologySectionsTable::get_current_insertion_point_row()
{
	return static_cast<int>(d_container_ptr->insertion_point());
}


void
GPlatesGui::TopologySectionsTable::set_insertion_point_widget(
		int row)
{
	// Add the Insertion Point to the new location.
	if (row >= 0 && row < d_table->rowCount()) {
		// While the 4.3 documentation does not indicate how Qt treats the
		// cell widget, the 4.4 documentation enlightens us; it takes
		// ownership. Unfortunately, there is no 'takeCellWidget', so
		// we cannot get it back once it has grabbed it, only remove it.
		// http://doc.trolltech.com/4.4/qtablewidget.html#setCellWidget
		d_table->setCellWidget(row, TopologySectionsTableColumns::COLUMN_ACTIONS,
				new GPlatesQtWidgets::InsertionPointWidget(*d_cancel_insertion_point_action, d_table));
	}
}


void
GPlatesGui::TopologySectionsTable::remove_insertion_point_widget(
		int row)
{
	if (row >= 0 && row < d_table->rowCount() && row != get_current_action_box_row()) {
		// While the 4.3 documentation does not indicate how Qt treats the
		// cell widget, the 4.4 documentation enlightens us; it takes
		// ownership. Unfortunately, there is no 'takeCellWidget', so
		// we cannot get it back once it has grabbed it, only remove it.
		// http://doc.trolltech.com/4.4/qtablewidget.html#removeCellWidget
		// Rest assured, the InsertionPointWidget destructor does get called
		// (by the QTableWidget) at this point.
		d_table->removeCellWidget(row, TopologySectionsTableColumns::COLUMN_ACTIONS);
	}
}


int
GPlatesGui::TopologySectionsTable::convert_data_index_to_table_row(
		TopologySectionsContainer::size_type index)
{
	int row = static_cast<int>(index);
	if (index >= d_container_ptr->insertion_point()) {
		++row;
	}
	return row;
}


GPlatesGui::TopologySectionsContainer::size_type
GPlatesGui::TopologySectionsTable::convert_table_row_to_data_index(
		int row)
{
	TopologySectionsContainer::size_type index = static_cast<TopologySectionsContainer::size_type>(row);
	if (index == d_container_ptr->insertion_point()) 
	{
		// While the visual row requested corresponds to the special insertion point,
		// and does not match any entry in the data vector, we can at least return
		// the index of data that new entries would be inserted at.
		return index;
	} else if (index > d_container_ptr->insertion_point()) {
		--index;
	}
	// else: index must be < the insertion point, so no adjustment necessary.
	
	return index;
}


void
GPlatesGui::TopologySectionsTable::set_up_actions()
{
	static const QIcon remove_icon(":/tango_emblem_unreadable_22.png");
	static const QIcon insert_above_icon(":/gnome_go_top_22.png");
	static const QIcon insert_below_icon(":/gnome_go_bottom_22.png");
	static const QIcon cancel_insertion_point_icon(":/insertion_point_cancel_22.png");

	// Set up icons and text for actions.
	d_remove_action->setIcon(remove_icon);
	d_remove_action->setToolTip(tr("Click to remove this section from the topology."));
	d_insert_above_action->setIcon(insert_above_icon);
	d_insert_above_action->setToolTip(tr("Move the insertion point to the row above this section. New features will be added to the topology before this one."));
	d_insert_below_action->setIcon(insert_below_icon);
	d_insert_below_action->setToolTip(tr("Move the insertion point to the row below this section. New features will be added to the topology after this one."));
	d_cancel_insertion_point_action->setIcon(cancel_insertion_point_icon);
	d_cancel_insertion_point_action->setToolTip(tr("Cancel this insertion point. New features will be added to the end of the table."));

	// Connect actions to our handlers.
	QObject::connect(d_remove_action, SIGNAL(triggered()),
			this, SLOT(react_remove_clicked()));
	QObject::connect(d_insert_above_action, SIGNAL(triggered()),
			this, SLOT(react_insert_above_clicked()));
	QObject::connect(d_insert_below_action, SIGNAL(triggered()),
			this, SLOT(react_insert_below_clicked()));
	QObject::connect(d_cancel_insertion_point_action, SIGNAL(triggered()),
			this, SLOT(react_cancel_insertion_point_clicked()));
}


GPlatesQtWidgets::ActionButtonBox *
GPlatesGui::TopologySectionsTable::create_new_action_box()
{
	GPlatesQtWidgets::ActionButtonBox *box = new GPlatesQtWidgets::ActionButtonBox(3, 22, d_table);
	// Add our actions (as tool buttons) to this action box.
	box->add_action(d_remove_action);
	box->add_action(d_insert_above_action);
	box->add_action(d_insert_below_action);
	return box;
}


void
GPlatesGui::TopologySectionsTable::set_up_table()
{
	// Get the columns.
	d_column_heading_infos = TopologySectionsTableColumns::get_column_heading_infos();

	d_table->setColumnCount(d_column_heading_infos.size());
	// Set names and tooltips.
	for (std::size_t column = 0; column < d_column_heading_infos.size(); ++column) {
		// Construct a QTableWidgetItem to be used as a 'header' item.
		QTableWidgetItem *item = new QTableWidgetItem(tr(d_column_heading_infos[column].label));
		item->setToolTip(tr(d_column_heading_infos[column].tooltip));
		// Add that item to the table.
		d_table->setHorizontalHeaderItem(column, item);
	}
	// Set widths and stretching.
	for (std::size_t column = 0; column < d_column_heading_infos.size(); ++column) {
		d_table->horizontalHeader()->setSectionResizeMode(column, d_column_heading_infos[column].resize_mode);
		d_table->horizontalHeader()->resizeSection(column, d_column_heading_infos[column].width);
	}
	
	// Height of each column should be enough for the action buttons.
	// Hardcoded numbers suck, but asking the buttons for their height wasn't working.
	d_table->verticalHeader()->setDefaultSectionSize(34);
	// But don't show the vertical header.
	d_table->verticalHeader()->hide();
	// Depending on how it looks when we have real data in there, we may wish to use this:
	d_table->horizontalHeader()->setStretchLastSection(true);
	// Don't make column labels bold just because we clicked on a row.
	d_table->horizontalHeader()->setHighlightSections(false);
}


void
GPlatesGui::TopologySectionsTable::update_table_row_count()
{
	// One row for each data entry.
	int rows = static_cast<int>( d_container_ptr->size() );
	// Plus one for the insertion point.
	++rows;
//// qDebug() << "TST: update_table_row_count: rows = " << rows;
	d_table->setRowCount(rows);
// // qDebug() << "TST: update_table_row_count: END";
}


void
GPlatesGui::TopologySectionsTable::update_table_row(
		int row)
{
	if (row < 0 || row >= d_table->rowCount()) {
		return;
	}

	// We are changing the table programmatically. We don't want cellChanged events.
	TableUpdateGuard guard(d_suppress_update_notification_guard);

	// Reset the current row to the default state so we can render a new row
	// without concern with what was previously there or what was the row state
	// of things such as column spanning.
	reset_row(row);

	// Render different row types according to context.
	if (row == get_current_insertion_point_row()) {
		// Draw our magic insertion point row here.
		render_insertion_point_row(row);
	} else {
		// Render the action box for the current row if necessary.
		// The action box only gets drawn on rows that are not the insertion point row.
		if (row == get_current_action_box_row())
		{
			set_action_box_widget(row);
		}

		// Map this table row to an entry in the data vector.
		TopologySectionsContainer::size_type index = convert_table_row_to_data_index(row);
		const TopologySectionsContainer::TableRow &entry = d_container_ptr->at(index);

#if 0
qDebug() << "update_table_row()"
	<< ": r = " << row 
	<< "; i = " << get_current_insertion_point_row()  
	<< "; c = " << d_table->rowCount();
#endif

		if (check_row_validity(entry)) 
		{
			if ( ! check_row_validity_geom(entry)) {
				// Draw a yellow row
				render_valid_row(row, entry, QColor("#FFFF00") );
			} else {
				if ( is_row_prev_neighbor_of_insert(
						row, get_current_insertion_point_row(), d_table->rowCount() ) )
				{
					// Draw a nice normal valid row, with color for prev neighbor.
					render_valid_row(row, entry, QColor("blue") ); // green
				}
				else if ( is_row_next_neighbor_of_insert(
						row, get_current_insertion_point_row(), d_table->rowCount() ) )
				{
					// Draw a nice normal valid row, with color for next neighbor.
					render_valid_row(row, entry, QColor("green") ); // green
				}
				else if ( !check_row_validity_reconstructed_geometry(entry, d_application_state.get_current_reconstruction()) )
				{
					// Draw a nice normal valid row, with color indicating section does not
					// contribute to the topology at the current reconstruction time.
					render_valid_row(row, entry, Qt::gray );
				}
				else 
				{
					// Draw a nice normal valid row.
					render_valid_row(row, entry); 
				}

			}

		} else {
			// Draw a red invalid row.
			render_invalid_row(row, get_invalid_row_message(entry));
		}
	}
}


void
GPlatesGui::TopologySectionsTable::render_valid_row(
		int row,
		const TopologySectionsContainer::TableRow &row_data,
		QColor bg)
{
	// Iterate over each non-action column in this row and set the table cells to either a
	// widget or a regular QTableWidgetItem.
	for (std::size_t column = TopologySectionsTableColumns::COLUMN_ACTIONS;
		column < d_column_heading_infos.size();
		++column)
	{
		if (d_column_heading_infos[column].should_edit_cell_with_widget(row_data))
		{
			install_edit_cell_widget(row, column);
			continue;
		}

		install_table_widget_item(row, column, row_data, bg);
	}
}


void
GPlatesGui::TopologySectionsTable::render_invalid_row(
		int row,
		QString reason)
{
	static const QColor invalid_fg = Qt::black;
	static const QColor invalid_bg = QColor("#FF6149"); // red 

	// Table cells start off as NULL QTableWidgetItems. Fill in the blanks if needed.
	int description_column = TopologySectionsTableColumns::COLUMN_ACTIONS + 1;
	int description_span = d_column_heading_infos.size() - description_column;
	QTableWidgetItem *description_item = new QTableWidgetItem;
	d_table->setItem(row, description_column, description_item);

	// Set more default flags.
	description_item->setFlags(Qt::ItemIsEnabled);
	description_item->setText(reason);
	description_item->setData(Qt::ForegroundRole, invalid_fg);
	description_item->setData(Qt::BackgroundRole, invalid_bg);
	d_table->setSpan(row, description_column, 1, description_span);
}


void
GPlatesGui::TopologySectionsTable::render_insertion_point_row(
		int row)
{
	static const QColor insertion_fg = Qt::darkGray;

	// Table cells start off as NULL QTableWidgetItems. Fill in the blanks if needed.
	QTableWidgetItem *actions_item = new QTableWidgetItem;
	d_table->setItem(row, TopologySectionsTableColumns::COLUMN_ACTIONS, actions_item);
	
	// Set default flags.
	actions_item->setFlags(Qt::ItemIsEnabled);

	// Table cells start off as NULL QTableWidgetItems. Fill in the blanks if needed.
	int description_column = TopologySectionsTableColumns::COLUMN_ACTIONS + 1;
	int description_span = d_column_heading_infos.size() - description_column;
	QTableWidgetItem *description_item = new QTableWidgetItem;
	d_table->setItem(row, description_column, description_item);
	
	// Set more default flags.
	description_item->setFlags(Qt::ItemIsEnabled);
	description_item->setText(tr("This insertion point indicates where new topology sections will be added."));
	description_item->setData(Qt::ForegroundRole, insertion_fg);
	d_table->setSpan(row, description_column, 1, description_span);

	// Put the insertion point where it belongs.
	set_insertion_point_widget(row);
}


void
GPlatesGui::TopologySectionsTable::update_data_from_table(
		int row)
{
	if (row < 0 || row >= d_table->rowCount()) {
		return;
	}

	// Map this table row to an entry in the data vector.
	TopologySectionsContainer::size_type index = convert_table_row_to_data_index(row);
	if (index > d_container_ptr->size()) {
		return;
	}
	
	// Create a temporary copy of the TableRow, which we will modify column-by-column.
	TopologySectionsContainer::TableRow temp_entry = d_container_ptr->at(index);
	// Iterate over each column in this row, and update the back-end data
	// based on whatever accessors are defined.
	for (std::size_t column = 0; column < d_column_heading_infos.size(); ++column) {
		QTableWidgetItem *item = d_table->item(row, column);
		if (item != NULL) {
			// Call mutator function to access QTableWidgetItem and update data in the vector.
			d_column_heading_infos[column].mutator(temp_entry, *item);
		}
	}
	// Replace the old entry in the TopologySectionsContainer with the new, updated one.
	d_container_ptr->update_at(index, temp_entry);
}



void
GPlatesGui::TopologySectionsTable::focus_feature_at_row(
		int row)
{
	// Get the appropriate details about the feature from the container,
	TopologySectionsContainer::size_type index = convert_table_row_to_data_index(row);
// // qDebug() << "TST: focus_feature_at_row: row = " << row << "; index = " << index << "; rowCount =" << d_table->rowCount();

	// Check outer bounds
	if (row < 0 || row >= d_table->rowCount() ) 
	{
// // qDebug() << "TST: focus_feature_at_row: row < 0 || >= rowCount";
		return;
	}

	// Clicking the special Insertion Point row should have no effect.
	if (row == get_current_insertion_point_row()) 
	{
// qDebug() << "TST: focus_feature_at_row: row == get_cur_insert";
		return;
	}


	const TopologySectionsContainer::TableRow &trow = d_container_ptr->at(index);

	if (!trow.get_feature_ref().is_valid() )
	{
// qDebug() << "TST: focus_feature_at_row: ! trow.get_feature_ref().is_valid() )";
	}

	if (! trow.get_geometry_property().is_still_valid() )
	{
// qDebug() << "TST: focus_feature_at_row: ! trow.get_geometry_property().is_still_valid() ";
	}
		
	// Do we have enough information?
	if (trow.get_feature_ref().is_valid() && trow.get_geometry_property().is_still_valid()) 
	{
		// Then adjust the focus.
		d_feature_focus_ptr->set_focus(trow.get_feature_ref(), trow.get_geometry_property());
	
		// And provide visual feedback for user.
		d_table->selectRow(row);
	}
// qDebug() << "TST: END";
}

void
GPlatesGui::TopologySectionsTable::react_focus_feature_at_index(
		GPlatesGui::TopologySectionsContainer::size_type index)
{
	d_table->selectRow( convert_data_index_to_table_row(index) );
}

void
GPlatesGui::TopologySectionsTable::react_container_change(
		 GPlatesGui::TopologySectionsContainer *ptr)
{
	//// qDebug() << "TST: react_container_change()";
	// set the pointer
	d_container_ptr = ptr;
}

void
GPlatesGui::TopologySectionsTable::install_table_widget_item(
		int row,
		int column,
		const TopologySectionsContainer::TableRow &row_data,
		QColor bg)
{
	// Table cells start off as NULL QTableWidgetItems, so we need to make new ones
	// initially. We could re-use an existing one if it is still there from the last
	// time we made one, but it is better (in this case) to replace them with new
	// QTableWidgetItems as we get into formatting trouble otherwise - for example,
	// when a row that was the insertion points (with spanning colouring etc) gets
	// replaced with a data row.
	// QTableWidget handles the memory of these things.
	QTableWidgetItem *item = new QTableWidgetItem;
	d_table->setItem(row, column, item);
	
	// Set default flags and alignment for all table cells in this column.
	item->setTextAlignment(d_column_heading_infos[column].data_alignment);
	item->setFlags(d_column_heading_infos[column].data_flags);
	
	item->setData(Qt::BackgroundRole, bg);

	// Call accessor function to put raw data into the table.
	d_column_heading_infos[column].accessor(row_data, *item);
}


void
GPlatesGui::TopologySectionsTable::install_edit_cell_widget(
		int row,
		int column)
{
	// Get the index into the topological sections container for the current row.
	const TopologySectionsContainer::size_type sections_container_index =
			convert_table_row_to_data_index(row);

	// Create our own widget to edit the cell with.
	QWidget *edit_cell_widget = d_column_heading_infos[column].create_edit_cell_widget(
			d_table,
			*d_container_ptr,
			sections_container_index);

	std::cout << "install_edit_cell_widget" << std::endl;

	d_table->setCellWidget(row, column, edit_cell_widget);
}


void
GPlatesGui::TopologySectionsTable::remove_cell(
		int row,
		int column)
{
	if (d_table->cellWidget(row, column))
	{
		d_table->removeCellWidget(row, column);
	}
	else
	{
		// Delete the QTableWidgetItem so that it doesn't get drawn under the
		// new cell widget we are about to install in its place.
		// NOTE: 'delete' does nothing when NULL.
		delete d_table->takeItem(row, column);
	}
}


void
GPlatesGui::TopologySectionsTable::remove_cells(
		int row)
{
	for (std::size_t column = 0; column < d_column_heading_infos.size(); ++column)
	{
		remove_cell(row, column);
	}
}


void
GPlatesGui::TopologySectionsTable::reset_row(
		int row)
{
	// Remove all cells in the current row.
	remove_cells(row);

	// Undo any effect the Insertion Point row may have caused.
	const int description_column = TopologySectionsTableColumns::COLUMN_ACTIONS + 1;
	
	if(d_table->columnSpan(row, description_column) != 1 ||
	   d_table->rowSpan(row, description_column) !=1 )
	{
		//only reset span when there is any span
		d_table->setSpan(row, description_column, 1, 1);
	}
}
