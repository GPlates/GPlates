/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <Qt>		// for Qt::Alignment, Qt::AlignmentFlag, Qt::ItemFlag
#include <QHeaderView>
#include <QVariant>
#include <QDebug>
#include <vector>
#include <boost/noncopyable.hpp>
#include "TopologySectionsTable.h"

#include "model/PropertyName.h"
#include "model/FeatureHandle.h"
#include "model/FeatureHandleWeakRefBackInserter.h"

#include "property-values/GpmlPlateId.h"
#include "property-values/XsString.h"
#include "utils/UnicodeStringUtils.h"
#include "feature-visitors/PropertyValueFinder.h"

#include "qt-widgets/ActionButtonBox.h"
#include "qt-widgets/InsertionPointWidget.h"

#include "gui/TopologySectionsContainer.h"


#define NUM_ELEMS(a) (sizeof(a) / sizeof((a)[0]))
#define NUM_ELEMS_INT(a) static_cast<int>(NUM_ELEMS(a))

namespace
{
	/**
	 * Resolves a FeatureId reference in a TableRow (or doesn't,
	 * if it can't be resolved.)
	 *
	 * FIXME:Quickie implementation. Could be better, to give user
	 * more feedback.
	 */
	void
	resolve_feature_id(
			GPlatesGui::TopologySectionsContainer::TableRow &entry)
	{

		std::vector<GPlatesModel::FeatureHandle::weak_ref> back_ref_targets;
		entry.d_feature_id.find_back_ref_targets(
				GPlatesModel::append_as_weak_refs(back_ref_targets));
		
		if (back_ref_targets.size() == 1) {
			GPlatesModel::FeatureHandle::weak_ref weakref = *back_ref_targets.begin();
			entry.d_feature_ref = weakref;
		} else {
			static const GPlatesModel::FeatureHandle::weak_ref null_ref;
			entry.d_feature_ref = null_ref;
		}
	}

	/**
	 * Defines a function used to turn a TopologySectionsContainer::TableRow
	 * into an appropriate cell of data in the QTableWidget.
	 */
	typedef void (*table_accessor_type)(
			const GPlatesGui::TopologySectionsContainer::TableRow &row_data,
			QTableWidgetItem &cell);

	/**
	 * Defines a function used to modify a TopologySectionsContainer::TableRow
	 * based on whatever user-entered data is in the QTableWidgetItem.
	 */
	typedef void (*table_mutator_type)(
			GPlatesGui::TopologySectionsContainer::TableRow &row_data,
			const QTableWidgetItem &cell);

	// Table accessor functions:
	// These functions take the raw data and modify a QTableWidgetItem
	// to display the data appropriately.
	
	void
	null_data_accessor(
			const GPlatesGui::TopologySectionsContainer::TableRow &,
			QTableWidgetItem &)
	{  }

	void
	get_data_reversed_flag(
			const GPlatesGui::TopologySectionsContainer::TableRow &row_data,
			QTableWidgetItem &cell)
	{
		cell.setCheckState(row_data.d_reverse? Qt::Checked : Qt::Unchecked);
	}


	void
	get_data_feature_type(
			const GPlatesGui::TopologySectionsContainer::TableRow &row_data,
			QTableWidgetItem &cell)
	{
		if (row_data.d_feature_ref.is_valid()) {
			cell.setData(Qt::DisplayRole, QVariant(GPlatesUtils::make_qstring_from_icu_string(
					row_data.d_feature_ref->feature_type().build_aliased_name() )));
		}
	}
			

	void
	get_data_reconstruction_plate_id(
			const GPlatesGui::TopologySectionsContainer::TableRow &row_data,
			QTableWidgetItem &cell)
	{
		static const GPlatesModel::PropertyName plate_id_property_name =
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
		
		if (row_data.d_feature_ref.is_valid()) {
			// Declare a pointer to a const GpmlPlateId. This is used as a return value;
			// It is passed by reference into the get_property_value() call below,
			// which sets it.
			const GPlatesPropertyValues::GpmlPlateId *property_return_value = NULL;
			
			// Attempt to find the property name and value we are interested in.
			if (GPlatesFeatureVisitors::get_property_value(
					*row_data.d_feature_ref,
					plate_id_property_name,
					property_return_value)) {
				// Convert it to something Qt can display.
				const GPlatesModel::integer_plate_id_type &plate_id = property_return_value->value();
				cell.setData(Qt::DisplayRole, QVariant(static_cast<quint32>(plate_id)));
			} else {
				// Feature resolves, but no reconstructionPlateId.
				cell.setData(Qt::DisplayRole, QObject::tr("<none>"));
			}
		}
	}


	void
	get_data_feature_name(
			const GPlatesGui::TopologySectionsContainer::TableRow &row_data,
			QTableWidgetItem &cell)
	{
		static const GPlatesModel::PropertyName gml_name_property_name =
				GPlatesModel::PropertyName::create_gml("name");
		
		if (row_data.d_feature_ref.is_valid()) {
			// FIXME: As in other situations involving gml:name, we -do- want to
			// address the gml:codeSpace issue someday.
			
			// Declare a pointer to a const XsString. This is used as a return value;
			// It is passed by reference into the get_property_value() call below,
			// which sets it.
			const GPlatesPropertyValues::XsString *property_return_value = NULL;
			
			// Attempt to find the property name and value we are interested in.
			if (GPlatesFeatureVisitors::get_property_value(
					*row_data.d_feature_ref,
					gml_name_property_name,
					property_return_value)) {
				// Convert it to something Qt can display.
				QString name_qstr = GPlatesUtils::make_qstring(property_return_value->value());
				cell.setData(Qt::DisplayRole, name_qstr);
			} else {
				// Feature resolves, but no name property.
				cell.setData(Qt::DisplayRole, QString(""));
			}
		}
	}

	// Table mutator functions:
	// These functions take a QTableWidgetItem with user-entered values
	// and update the raw data appropriately.

	void
	null_data_mutator(
			GPlatesGui::TopologySectionsContainer::TableRow &,
			const QTableWidgetItem &)
	{  }

	void
	set_data_reversed_flag(
			GPlatesGui::TopologySectionsContainer::TableRow &row_data,
			const QTableWidgetItem &cell)
	{
		// Convert the QTableWidgetItem's checked status into a bool.
		bool new_reverse_flag = false;
		if (cell.checkState() == Qt::Checked) {
			new_reverse_flag = true;
		}
		
		// Modify the TableRow data.
		if (new_reverse_flag != row_data.d_reverse) {
			row_data.d_reverse = new_reverse_flag;
			// Note: the update_data_from_table() method will push this table row into
			// the d_container_ptr vector, which will ultimately emit signals to notify
			// others about the updated data.
		}
	}


	/**
	 * Defines characteristics of each column of the table.
	 */
	struct ColumnHeadingInfo
	{
		const char *label;
		const char *tooltip;
		int width;
		QHeaderView::ResizeMode resize_mode;
		QFlags<Qt::AlignmentFlag> data_alignment;
		QFlags<Qt::ItemFlag> data_flags;
		table_accessor_type accessor;
		table_mutator_type mutator;
	};

	/**
	 * Convenience enum so we can refer to columns by name
	 * in this file, to make it easier to talk about the
	 * 'special' columns like "Actions". Keep it in sync with
	 * the column_heading_info_table below.
	 */
	enum ColumnLayout
	{
		COLUMN_ACTIONS, COLUMN_REVERSE, COLUMN_FEATURE_TYPE, COLUMN_PLATE_ID, COLUMN_NAME,
		NUM_COLUMNS
	};

	/**
	 * The column header information table. This gets used
	 * to set up the QTableWidget just the way we like it.
	 * 
	 * Don't forget to update the enumeration above if you
	 * add or remove columns.
	 */
	static const ColumnHeadingInfo column_heading_info_table[] = {
		{ QT_TR_NOOP("Actions"), QT_TR_NOOP("Buttons in this column allow you to remove sections and change where new sections will be added."),
				104, QHeaderView::Fixed,	// FIXME: make this dynamic based on what the buttons need.
				Qt::AlignCenter,
				0,
				null_data_accessor, null_data_mutator },

		{ QT_TR_NOOP("Reverse"), QT_TR_NOOP("Controls whether the coordinates of the section will be applied in natural or reverse order."),
				60, QHeaderView::Fixed,
				Qt::AlignCenter,
				Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable,
				get_data_reversed_flag, set_data_reversed_flag },

		{ QT_TR_NOOP("Feature type"), QT_TR_NOOP("The type of this feature"),
				140, QHeaderView::ResizeToContents,
				Qt::AlignLeft | Qt::AlignVCenter,
				Qt::ItemIsEnabled | Qt::ItemIsSelectable,
				get_data_feature_type, null_data_mutator },

		{ QT_TR_NOOP("Plate ID"), QT_TR_NOOP("The plate ID used to reconstruct this feature"),
				60,  QHeaderView::ResizeToContents,
				Qt::AlignCenter,
				Qt::ItemIsEnabled | Qt::ItemIsSelectable,
				get_data_reconstruction_plate_id, null_data_mutator },

		{ QT_TR_NOOP("Name"), QT_TR_NOOP("A convenient label for this feature"),
				140, QHeaderView::ResizeToContents,
				Qt::AlignLeft | Qt::AlignVCenter,
				Qt::ItemIsEnabled | Qt::ItemIsSelectable,
				get_data_feature_name, null_data_mutator },
	};
	
	
	bool
	check_row_validity(
			const GPlatesGui::TopologySectionsContainer::TableRow &entry)
	{
		return entry.d_feature_ref.is_valid();
	}
	
	
	const QString
	get_invalid_row_message(
			const GPlatesGui::TopologySectionsContainer::TableRow &entry)
	{
		QString feature_id_qstr = GPlatesUtils::make_qstring(entry.d_feature_id);
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


	/**
	 * Create controls for testing while the thing is running!
	 *
	 * I set this up because I needed to check the results of
	 * non-table client code using the TopologySectionsContainer,
	 * but no client code existed on my branch yet.
	 * 
	 * e.g.
	 * box = new ActionButtonBox(4, 22, NULL);
	 * add_test_action_for_slot(box, d_container_ptr, SLOT(clear()));
	 */
	void
	add_test_action_for_slot(
			GPlatesQtWidgets::ActionButtonBox *box,
			QObject *target,
			const char *slot_spec)
	{
		QAction *action = new QAction(QString(slot_spec), box);
		action->setToolTip(QString("%1::%2").arg(target->metaObject()->className()).arg(slot_spec));
		box->add_action(action);
		QObject::connect(action, SIGNAL(triggered()),
				target, slot_spec);
	}
	
	void
	make_testing_controls(
			GPlatesGui::TopologySectionsContainer *container_ptr)
	{
		// Make an ActionButtonBox behave like a window (easy with Qt) and add some test actions to it.
		GPlatesQtWidgets::ActionButtonBox *testing_controls = new GPlatesQtWidgets::ActionButtonBox(4, 22, NULL);
		add_test_action_for_slot(testing_controls, container_ptr, SLOT(clear()));
		add_test_action_for_slot(testing_controls, container_ptr, SLOT(reset_insertion_point()));
		add_test_action_for_slot(testing_controls, container_ptr, SLOT(insert_test_data()));
		add_test_action_for_slot(testing_controls, container_ptr, SLOT(move_insertion_point_idx_4()));
		add_test_action_for_slot(testing_controls, container_ptr, SLOT(remove_idx_2()));
		testing_controls->show();
	}
}


GPlatesGui::TopologySectionsTable::TopologySectionsTable(
		QTableWidget &table,
		TopologySectionsContainer &container,
		GPlatesGui::FeatureFocus &feature_focus):
	d_table(&table),
	d_container_ptr(&container),
	d_action_box_row(-1),
	d_remove_action(new QAction(&table)),
	d_insert_above_action(new QAction(&table)),
	d_insert_below_action(new QAction(&table)),
	d_cancel_insertion_point_action(new QAction(&table)),
	d_suppress_update_notification_guard(false),
	d_feature_focus_ptr(&feature_focus)
{
	// Set up the actions we can use.
	set_up_actions();
	
	// Set up the basic properties of the table.
	set_up_table();

#if 0	// Disabling special testing controls. Once we merge with the platepolygon branch, they won't be necessary.	
	make_testing_controls(d_container_ptr);
#endif

	update_table();

	// Listen to events from our container.
	QObject::connect(d_container_ptr, SIGNAL(cleared()),
			this, SLOT(clear_table()));
	QObject::connect(d_container_ptr, SIGNAL(insertion_point_moved(GPlatesGui::TopologySectionsContainer::size_type)),
			this, SLOT(update_table()));
	QObject::connect(d_container_ptr, SIGNAL(entry_removed(GPlatesGui::TopologySectionsContainer::size_type)),
			this, SLOT(update_table()));
	QObject::connect(d_container_ptr, SIGNAL(entries_inserted(GPlatesGui::TopologySectionsContainer::size_type,GPlatesGui::TopologySectionsContainer::size_type)),
			this, SLOT(update_table()));
	QObject::connect(d_container_ptr, SIGNAL(entries_modified(GPlatesGui::TopologySectionsContainer::size_type,GPlatesGui::TopologySectionsContainer::size_type)),
			this, SLOT(update_table()));

	QObject::connect(d_container_ptr, SIGNAL(focus_feature_at_index( GPlatesGui::TopologySectionsContainer::size_type)),
			this, SLOT(react_focus_feature_at_index(GPlatesGui::TopologySectionsContainer::size_type)));

	// Enable the table to receive mouse move events, so we can show/hide buttons
	// based on the row being hovered over.
	d_table->setMouseTracking(true);
	QObject::connect(d_table, SIGNAL(cellEntered(int, int)),
			this, SLOT(react_cell_entered(int, int)));

	// Some cells may be user-editable: listen for changes.
	QObject::connect(d_table, SIGNAL(cellChanged(int, int)),
			this, SLOT(react_cell_changed(int, int)));

	// Adjust focus via clicking on table rows.
	QObject::connect(d_table, SIGNAL(cellClicked(int, int)),
			this, SLOT(react_cell_clicked(int, int)));
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
	update_cell_widgets();
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
	TopologySectionsContainer::size_type my_index = convert_table_row_to_data_index(get_current_action_box_row());
	if (my_index > d_container_ptr->size()) {
		return;
	}
	
	// May as well remove the action box for the row which is about to vanish.
	remove_action_box();

	// Remove this entry from the table.
	d_container_ptr->remove_at(my_index);
	
	// Now it's gone, we need to adjust the position of a few things.
	// Note: We could figure out the new row under the mouse cursor, via
	// QTableView::rowAt(int y), but that would involve getting the mouse location via
	// reimplementing QWidget::mouseMoveEvent() on the table. Of course, we can also take
	// a wild guess and say that the row under the cursor is the previous row + 1, since
	// everything will have shuffled up after the deletion.
	move_action_box(convert_data_index_to_table_row(my_index));
	update_cell_widgets();
}


void
GPlatesGui::TopologySectionsTable::react_insert_above_clicked()
{
	// Insert "Above me", i.e. insertion point should replace this row.
	TopologySectionsContainer::size_type my_index = convert_table_row_to_data_index(get_current_action_box_row());
	TopologySectionsContainer::size_type target_index = my_index;
	if (target_index == d_container_ptr->insertion_point()) {
		// It's already there. Nevermind.
		return;
	}

	d_container_ptr->move_insertion_point(target_index);
	
	// Put the action box back on row that triggered this (which may have moved).
	move_action_box(convert_data_index_to_table_row(my_index));
	update_cell_widgets();
}


void
GPlatesGui::TopologySectionsTable::react_insert_below_clicked()
{
	// Insert "Below me", i.e. insertion point should be this row + 1.
	TopologySectionsContainer::size_type my_index = convert_table_row_to_data_index(get_current_action_box_row());
	TopologySectionsContainer::size_type target_index = my_index + 1;
	if (target_index == d_container_ptr->insertion_point()) {
		// It's already there. Nevermind.
		return;
	}
	
	d_container_ptr->move_insertion_point(target_index);

	// Put the action box back on row that triggered this (which may have moved).
	move_action_box(convert_data_index_to_table_row(my_index));
	update_cell_widgets();
}


void
GPlatesGui::TopologySectionsTable::react_cancel_insertion_point_clicked()
{
	// Moving the insertion point will probably affect the row that the
	// action box is on. Let's make a note of it, before we do the reshuffling.
	TopologySectionsContainer::size_type action_box_index = convert_table_row_to_data_index(get_current_action_box_row());

	d_container_ptr->reset_insertion_point();
	update_table();

	// Put the action box back on the right row.
	move_action_box(convert_data_index_to_table_row(action_box_index));

	// Put the insertion point where it belongs.
	update_cell_widgets();
}


void
GPlatesGui::TopologySectionsTable::update_cell_widgets()
{
	for (int i = 0; i < d_table->rowCount(); ++i) {
		remove_insertion_point_widget(i);
	}
	set_insertion_point_widget(get_current_insertion_point_row());
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
	if (row == get_current_action_box_row() || row == get_current_insertion_point_row()) {
		return;
	}
	// Remove the Action Box from the previous location.
	remove_action_box();
	// Add the Action Box to the new location.
	if (row >= 0 && row < d_table->rowCount()) {
		d_action_box_row = row;
		// While the 4.3 documentation does not indicate how Qt treats the
		// cell widget, the 4.4 documentation enlightens us; it takes
		// ownership. Unfortunately, there is no 'takeCellWidget', so
		// we cannot get it back once it has grabbed it, only remove it.
		// http://doc.trolltech.com/4.4/qtablewidget.html#setCellWidget
		d_table->setCellWidget(row, COLUMN_ACTIONS, create_new_action_box());
	}
}


void
GPlatesGui::TopologySectionsTable::remove_action_box()
{
	int old_row = get_current_action_box_row();
	if (old_row >= 0 && old_row < d_table->rowCount()) {
		d_action_box_row = -1;
		// While the 4.3 documentation does not indicate how Qt treats the
		// cell widget, the 4.4 documentation enlightens us; it takes
		// ownership. Unfortunately, there is no 'takeCellWidget', so
		// we cannot get it back once it has grabbed it, only remove it.
		// http://doc.trolltech.com/4.4/qtablewidget.html#removeCellWidget
		// Rest assured, the ActionButtonBox destructor does get called
		// (by the QTableWidget) at this point.
		d_table->removeCellWidget(old_row, COLUMN_ACTIONS);
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
		d_table->setCellWidget(row, COLUMN_ACTIONS,
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
		d_table->removeCellWidget(row, COLUMN_ACTIONS);
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
	if (index == d_container_ptr->insertion_point()) {
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
	d_table->setColumnCount(NUM_ELEMS_INT(column_heading_info_table));
	// Set names and tooltips.
	for (int column = 0; column < NUM_ELEMS_INT(column_heading_info_table); ++column) {
		// Construct a QTableWidgetItem to be used as a 'header' item.
		QTableWidgetItem *item = new QTableWidgetItem(tr(column_heading_info_table[column].label));
		item->setToolTip(tr(column_heading_info_table[column].tooltip));
		// Add that item to the table.
		d_table->setHorizontalHeaderItem(column, item);
	}
	// Set widths and stretching.
	for (int column = 0; column < NUM_ELEMS_INT(column_heading_info_table); ++column) {
		d_table->horizontalHeader()->setResizeMode(column, column_heading_info_table[column].resize_mode);
		d_table->horizontalHeader()->resizeSection(column, column_heading_info_table[column].width);
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
	int rows = static_cast<int>(d_container_ptr->size());
	// Plus one for the insertion point.
	++rows;

	d_table->setRowCount(rows);
}


void
GPlatesGui::TopologySectionsTable::update_table_row(
		int row)
{
	if (row < 0 || row >= d_table->rowCount()) {
		return;
	}
	
	// Render different row types according to context.
	if (row == get_current_insertion_point_row()) {
		// Draw our magic insertion point row here.
		render_insertion_point(row);
	} else {
		// Map this table row to an entry in the data vector.
		TopologySectionsContainer::size_type index = convert_table_row_to_data_index(row);
//		const TopologySectionsContainer::TableRow &entry = d_container_ptr->at(index);
//FIXME: CHEATING. I need a non-const so I can resolve the damn thing, but maybe I shouldn't
// be doing that here. I would prefer to pass in a const reference to the original
// for the render_ functions, but perhaps a copy will suffice for now.
		TopologySectionsContainer::TableRow entry = d_container_ptr->at(index);
		resolve_feature_id(entry);
		if (check_row_validity(entry)) {
			// Draw a nice normal valid row.
			render_table_row(row, entry);
		} else {
			// Draw a big red invalid row.
			render_invalid_row(row, get_invalid_row_message(entry));
		}
	}
}


void
GPlatesGui::TopologySectionsTable::render_table_row(
		int row,
		const TopologySectionsContainer::TableRow &row_data)
{
	// We are changing the table programmatically. We don't want cellChanged events.
	TableUpdateGuard guard(d_suppress_update_notification_guard);
	
	// Undo any effect the Insertion Point row may have caused.
	int description_column = COLUMN_ACTIONS + 1;
	d_table->setSpan(row, description_column, 1, 1);
	remove_insertion_point_widget(row);

	// Iterate over each column in this row, and set table cells based on whatever
	// accessors are defined.
	for (int column = 0; column < NUM_ELEMS_INT(column_heading_info_table); ++column) {
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
		item->setTextAlignment(column_heading_info_table[column].data_alignment);
		item->setFlags(column_heading_info_table[column].data_flags);

		// Call accessor function to put raw data into the table.
		table_accessor_type accessor_fn = column_heading_info_table[column].accessor;
		(*accessor_fn)(row_data, *item);
	}
}


void
GPlatesGui::TopologySectionsTable::render_insertion_point(
		int row)
{
	static const QColor insertion_fg = Qt::darkGray;

	// We are changing the table programmatically. We don't want cellChanged events.
	TableUpdateGuard guard(d_suppress_update_notification_guard);

	// Table cells start off as NULL QTableWidgetItems. Fill in the blanks if needed.
	QTableWidgetItem *actions_item = new QTableWidgetItem;
	d_table->setItem(row, COLUMN_ACTIONS, actions_item);
	
	// Set default flags.
	actions_item->setFlags(Qt::ItemIsEnabled);

	// Table cells start off as NULL QTableWidgetItems. Fill in the blanks if needed.
	int description_column = COLUMN_ACTIONS + 1;
	int description_span = NUM_COLUMNS - description_column;
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
GPlatesGui::TopologySectionsTable::render_invalid_row(
		int row,
		QString reason)
{
	static const QColor invalid_fg = Qt::black;
	static const QColor invalid_bg = QColor("#FF6149");

	// We are changing the table programmatically. We don't want cellChanged events.
	TableUpdateGuard guard(d_suppress_update_notification_guard);

	// Undo any effect the Insertion Point row may have caused.
	remove_insertion_point_widget(row);

	// Table cells start off as NULL QTableWidgetItems. Fill in the blanks if needed.
	int description_column = COLUMN_ACTIONS + 1;
	int description_span = NUM_COLUMNS - description_column;
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
	for (int column = 0; column < NUM_ELEMS_INT(column_heading_info_table); ++column) {
		QTableWidgetItem *item = d_table->item(row, column);
		if (item != NULL) {
			// Call mutator function to access QTableWidgetItem and update data in the vector.
			table_mutator_type mutator_fn = column_heading_info_table[column].mutator;
			(*mutator_fn)(temp_entry, *item);
		}
	}
	// Replace the old entry in the TopologySectionsContainer with the new, updated one.
	d_container_ptr->update_at(index, temp_entry);
}



void
GPlatesGui::TopologySectionsTable::focus_feature_at_row(
		int row)
{
	if (row < 0 || row >= d_table->rowCount()) {
		return;
	}
	// Clicking the special Insertion Point row should have no effect.
	if (row == get_current_insertion_point_row()) {
		return;
	}

	// Get the appropriate details about the feature from the container,
	TopologySectionsContainer::size_type index = convert_table_row_to_data_index(row);

	const TopologySectionsContainer::TableRow &trow = d_container_ptr->at(index);

	// Do we have enough information?
	if (trow.d_feature_ref.is_valid() && trow.d_geometry_property_opt) {
		// Then adjust the focus.
		d_feature_focus_ptr->set_focus(trow.d_feature_ref, *trow.d_geometry_property_opt);
	
		// And provide visual feedback for user.
		d_table->selectRow(row);
	}
}

void
GPlatesGui::TopologySectionsTable::react_focus_feature_at_index(
		GPlatesGui::TopologySectionsContainer::size_type index)
{
	d_table->selectRow( convert_data_index_to_table_row(index) );
}

