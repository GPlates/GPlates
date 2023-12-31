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

#ifndef GPLATES_GUI_TOPOLOGYSECTIONSTABLE_H
#define GPLATES_GUI_TOPOLOGYSECTIONSTABLE_H

#include <vector>
#include <QAction>
#include <QObject>
#include <QPointer>
#include <QTableWidget>

#include "TopologySectionsContainer.h"
#include "TopologySectionsTableColumns.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class ActionButtonBox;
}

namespace GPlatesGui
{
	class FeatureFocus;

	/**
	 * Class to manage a QTableWidget plus the items within, to display
	 * the sections of topology the user is currently building up via
	 * the plate polygon tool.
	 * 
	 * This table includes an "Insertion Point", to show the user where
	 * new topology sections will be added.
	 */
	class TopologySectionsTable :
			public QObject
	{
		Q_OBJECT
	public:
		
		explicit
		TopologySectionsTable(
				QTableWidget &table,
				TopologySectionsContainer &boundary_container,
				TopologySectionsContainer &interior_container,
				GPlatesPresentation::ViewState &view_state);

		virtual
		~TopologySectionsTable()
		{  }

	public Q_SLOTS:

		/**
		 * Updates the table rows from the data vector.
		 */
		void
		update_table();

		void
		focus_feature_at_row(
				int row);

		void
		react_focus_feature_at_index(
				GPlatesGui::TopologySectionsContainer::size_type index);

		void
		react_container_change(GPlatesGui::TopologySectionsContainer*);

	private Q_SLOTS:
		void
		react_cell_entered(
				int row,
				int col);

		void
		react_cell_clicked(
				int row,
				int col);

		void
		react_cell_changed(
				int row,
				int col);

		void
		react_remove_clicked();

		void
		react_insert_above_clicked();

		void
		react_insert_below_clicked();

		void
		react_cancel_insertion_point_clicked();

		/**
		 * Removes and deletes QTableWidgetItems from the table.
		 */
		void
		clear_table();

		//! A table row in the topology sections container was modified.
		void
		topology_section_modified(
				GPlatesGui::TopologySectionsContainer::size_type topology_sections_container_index);
		
	private:

		/**
		 * Returns the current table row associated with the ActionButtonBox.
		 * Will return -1 in the event that the actions are not visible on any row.
		 */
		int
		get_current_action_box_row();

		void
		move_action_box(
				int row);

		void
		remove_action_box();

		void
		set_action_box_widget(
				int row);


		/**
		 * Returns the visual row associated with the Insertion Point.
		 */
		int
		get_current_insertion_point_row();
		
		
		void
		set_insertion_point_widget(
				int row);
		
		void
		remove_insertion_point_widget(
				int row);

		/**
		 * Convert between items of data in the vector and rows on the QTableWidget,
		 * accounting for the presence of an 'insertion point' row.
		 */
		int
		convert_data_index_to_table_row(
				TopologySectionsContainer::size_type index);

		/**
		 * Convert between items of data in the vector and rows on the QTableWidget,
		 * accounting for the presence of an 'insertion point' row.
		 */
		TopologySectionsContainer::size_type
		convert_table_row_to_data_index(
				int row);

		/**
		 * Configures and connects up our QActions.
		 */
		void
		set_up_actions();

		/**
		 * Configures and connects up our QActions.
		 */
		void
		set_up_connections_to_container(TopologySectionsContainer *ptr);

		/**
		 * Assigns our custom actions to a newly created ActionButtonBox,
		 * and returns said box.
		 */
		GPlatesQtWidgets::ActionButtonBox *
		create_new_action_box();

		/**
		 * Sets columns and other properties of the QTableWidget.
		 */
		void
		set_up_table();
		
		/**
		 * Updates the number of visual rows in the table.
		 */
		void
		update_table_row_count();

		/**
		 * Updates data in table cells for one visual row.
		 */
		void
		update_table_row(
				int row);

		/**
		 * Updates data in table cells for one visual row so that it
		 * matches the given TableRow struct.
		 */
		void
		render_valid_row(
				int row,
				const TopologySectionsContainer::TableRow &row_data,
				QColor bg = Qt::white);

		/**
		 * Updates data in table cells for one visual row to draw
		 * the special 'insertion point' row.
		 */
		void
		render_insertion_point_row(
				int row);

		/**
		 * Updates data in table cells for one visual row so that it
		 * displays a warning about the data for this row being invalid.
		 */
		void
		render_invalid_row(
				int row,
				QString reason);

		/**
		 * The inverse of @a update_table_row(); 
		 * Called after the user has edited the table, it checks each
		 * column of the given row to see if a suitable function is
		 * defined, and calls it to convert the QTableWidgetItem cell
		 * back into the back-end data.
		 */
		void
		update_data_from_table(
				int row);

		/**
		 * Install a QTableWidgetItem for the cell at @a row and @a column.
		 */
		void
		install_table_widget_item(
				int row,
				int column,
				const TopologySectionsContainer::TableRow &row_data,
				QColor bg);

		/**
		 * Create and install a widget to edit the cell at @a row and @a column
		 * (as opposed to using a QTableWidgetItem).
		 */
		void
		install_edit_cell_widget(
				int row,
				int column);

		//! Removes cell (either widget or QTableWidgetItem) at @a row and @a column.
		void
		remove_cell(
				int row,
				int column);

		//! Removes all cells in @a row.
		void
		remove_cells(
				int row);

		//! Reset @a row to the default state so we can render a new row.
		void
		reset_row(
				int row);


		/**
		 * The QTableWidget we are managing.
		 */
		QPointer<QTableWidget> d_table;
		
		/**
		 * The underlying data. Stored as pointer so we could change it later,
		 * if we wanted to.
		 */
		TopologySectionsContainer *d_container_ptr;
		TopologySectionsContainer *d_boundary_container_ptr;
		TopologySectionsContainer *d_interior_container_ptr;

		/**
		 * Column information for setting up the table columns and converting
		 * data to/from the topology sections container.
		 */
		std::vector<TopologySectionsTableColumns::ColumnHeadingInfo> d_column_heading_infos;


		/**
		 * The row that the ActionButtonBox we display is in.
		 */
		int d_action_box_row;

		/**
		 * Remove Action. Parented to the table, managed by Qt.
		 */
		QAction *d_remove_action;

		/**
		 * Insert Above Action. Parented to the table, managed by Qt.
		 */
		QAction *d_insert_above_action;

		/**
		 * Insert Below Action. Parented to the table, managed by Qt.
		 */
		QAction *d_insert_below_action;

		/**
		 * Cancel Insertion Point Action. Parented to the table, managed by Qt.
		 */
		QAction *d_cancel_insertion_point_action;

		/**
		 * This flag is set by instantiating a TableUpdateGuard
		 * (in the .cc file) at any scope where we are directly modifying
		 * table cells programmatically.
		 * It is then read by @a react_cell_changed() to allow it to differentiate
		 * between user cell modifications and our own table updates.
		 */
		bool d_suppress_update_notification_guard;

		/**
		 * Application state used to retrieve the current reconstruction.
		 */
		GPlatesAppLogic::ApplicationState &d_application_state;

		/**
		 * Feature focus, so that the user can click on entries in the table
		 * and adjust the focus from there.
		 * FeatureFocus even gets a special version of @a set_focus() called
		 * just for this class! How special.
		 */
		FeatureFocus *d_feature_focus_ptr;
		
	};
}
#endif // GPLATES_GUI_TOPOLOGYSECTIONSTABLE_H

