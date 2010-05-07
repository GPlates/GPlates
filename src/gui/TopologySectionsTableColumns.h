/* $Id$ */


/**
 * \file 
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
 
#ifndef GPLATES_GUI_TOPOLOGYSECTIONSTABLECOLUMNS_H
#define GPLATES_GUI_TOPOLOGYSECTIONSTABLECOLUMNS_H

#include <vector>
#include <Qt>		// for Qt::Alignment, Qt::AlignmentFlag, Qt::ItemFlag
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QWidget>

#include "TopologySectionsContainer.h"

#include "property-values/GeoTimeInstant.h"


namespace GPlatesGui
{
	namespace TopologySectionsTableColumns
	{
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


		/**
		 * Defines a function used to used to determine whether a cell widget should be
		 * created that allows the user to modify a TopologySectionsContainer::TableRow.
		 */
		typedef bool (*should_install_edit_cell_widget_type)(
				const GPlatesGui::TopologySectionsContainer::TableRow &row_data);


		/**
		 * Defines a function used to create a cell widget that allows the user
		 * to modify a TopologySectionsContainer::TableRow.
		 */
		typedef QWidget * (*create_edit_cell_widget_type)(
				QTableWidget *table_widget,
				GPlatesGui::TopologySectionsContainer &sections_container,
				GPlatesGui::TopologySectionsContainer::size_type sections_container_index);


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
			should_install_edit_cell_widget_type should_edit_cell_with_widget;
			create_edit_cell_widget_type create_edit_cell_widget;
		};


		/**
		 * Returns the column header information table. This gets used
		 * to set up the QTableWidget just the way we like it.
		 */
		std::vector<ColumnHeadingInfo>
		get_column_heading_infos();


		/**
		 * The "actions" column is the zero column.
		 * All other columns represent or affect actual data in the topological sections container.
		 */
		const int COLUMN_ACTIONS = 0;


		/**
		 * A widget to edit the begin/end times of a topological section independently
		 * of the begin/end times of the feature the topological section is referencing.
		 *
		 * This has to be in a header file so that the Q_OBJECT macro is detected and
		 * a corresponding "moc_*" file is added to the build. Otherwise it would be hidden
		 * inside the ".cc" file.
		 *
		 * This widget is similar to EditTimePeriodWidget - some refactoring could be done
		 * but probably not worth it.
		 */
		class EditTimeWidget :
				public QWidget
		{
			Q_OBJECT
			
		public:
			//! Whether this widget is tracking begin or end time.
			enum Time { BEGIN_TIME, END_TIME };

			EditTimeWidget(
					Time begin_or_end_time,
					GPlatesGui::TopologySectionsContainer &sections_container,
					GPlatesGui::TopologySectionsContainer::size_type sections_container_index,
					QWidget *parent_);

#if 0
			~EditTimeWidget();

			virtual
			void
			focusInEvent(
					QFocusEvent *focus_event);

			virtual
			void
			focusOutEvent(
					QFocusEvent *focus_event);

			virtual
			bool
			eventFilter(
					QObject *obj,
					QEvent *event);
#endif

		private slots:
			void
			set_spinbox_time_in_topology_section(
					double time);

			void
			set_distant_time_checkstate(
					int);

		private:
			Time d_begin_or_end_time;
			TopologySectionsContainer &d_sections_container;
			TopologySectionsContainer::size_type d_sections_container_index;
			TopologySectionsContainer::TableRow d_table_row;

			QDoubleSpinBox *d_time_spinbox;
			QCheckBox *d_distant_time_checkbox;


			/**
			 * Returns the begin or end time of the current topological section if exists or
			 * the begin/end time of the feature referenced by it.
			 */
			GPlatesPropertyValues::GeoTimeInstant
			get_time_from_topology_section() const;
		};
	}
}

#endif // GPLATES_GUI_TOPOLOGYSECTIONSTABLECOLUMNS_H
